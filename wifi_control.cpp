/*
 * Copyright (C) 2019 FishSemi Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <asm/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <err.h>
#include <iostream>
#include <vector>
#include <string>
#include <utils/threads.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cutils/log.h>

#include "config.h"
#include "wifi_control.h"

#define IP_ADDR_LEN 4
#define ETHER_ADDR_LEN 6
#define SEND_BUFSIZE 256
#define RECV_BUFSIZE 512
#define ETHER_ADDRSTRLEN 18
#define NUD_MONITOR (NUD_STALE | NUD_REACHABLE | NUD_FAILED)

#define WIFI_CONTROL_SOCKET_NAME "wificontrol"
#define ADD_ENDPOINT "ADD_ENDPOINT"
#define REMOVE_ENDPOINT "REMOVE_ENDPOINT"
#undef LOG_TAG
#define LOG_TAG "WifiControl"

WifiControl::WifiControl()
    : ModuleThread {"WifiControl"}
{
    _open_socket();
}

bool WifiControl::_open_socket()
{
    _send_fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    bzero((void*)&_send_addr, sizeof(_send_addr));
    _send_addr.nl_family = AF_NETLINK;
    _send_addr.nl_pid = 0;
    _send_addr.nl_groups = 0;

    if (connect(_send_fd, (struct sockaddr *)&_send_addr, sizeof(_send_addr)) < 0) {
        ALOGE("netlink connect to kernel failed");
        goto fail;
    }

    _recv_fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    bzero((void*)&_recv_addr, sizeof(_recv_addr));
    _recv_addr.nl_family = AF_NETLINK;
    _recv_addr.nl_pid = 0;
    _recv_addr.nl_groups = RTMGRP_NEIGH;
    if (bind(_recv_fd, (struct sockaddr *)&_recv_addr, sizeof(_recv_addr)) < 0) {
        ALOGE("bind to broadcast netlink socket failed");
        goto fail;
    }
    _add_read_fd(_recv_fd, TYPE_OTHER_FD);

    // fd to send/recv message to mavlink router
    // It's expected blocking to read response
    _router_fd = _get_domain_socket(WIFI_CONTROL_SOCKET_NAME,
                                    TYPE_DOMAIN_SOCK_ABSTRACT,
                                    false);

    if (_router_fd < 0) {
        ALOGE("opening datagram socket failure");
        goto fail;
    }
    return true;

fail:
    if(_send_fd >= 0) {
        ::close(_send_fd);
        _send_fd = -1;
    }
    if(_recv_fd >= 0) {
        ::close(_recv_fd);
        _recv_fd = -1;
    }
    return false;
}

int WifiControl::_parse_msg(struct nlmsghdr *nlh, int *ifindex, char *ipaddr, char *macaddr)
{
    int len;
    int *ip;
    char *data;
    struct ndmsg *ndm;
    struct rtattr *attr;
    ndm = (struct ndmsg *)NLMSG_DATA(nlh);

    ALOGV("mlmsg_type: %hu, ndm_state: %hu", nlh->nlmsg_type, ndm->ndm_state);
    if (ndm->ndm_state & NUD_MONITOR) {
        //attr = NDA_RTA(ndm);
        //len = NDA_PAYLOAD(nlh);
        *ifindex = ndm->ndm_ifindex;
        attr = (struct rtattr *)((char *)ndm + NLMSG_ALIGN(sizeof(struct ndmsg)));
        len = NLMSG_PAYLOAD(nlh, sizeof(struct ndmsg));
        for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
            ALOGV("rta_type: %hu, rta_len: %hu, attr payload len: %lu", attr->rta_type, \
                attr->rta_len, RTA_PAYLOAD(attr));

            if (attr->rta_type == NDA_DST) {
                data = (char *)RTA_DATA(attr);
                ip = (int *)RTA_DATA(attr);
                sprintf(ipaddr, "%u.%u.%u.%u",
                    *data, *(data + 1), *(data + 2), *(data + 3));

                ALOGD("New neighbor address: %s, unsigned int type: %u", ipaddr, *ip);
            }

            if (attr->rta_type == NDA_LLADDR) {
                data = (char *)RTA_DATA(attr);
                sprintf(macaddr, "%2x:%2x:%2x:%2x:%2x:%2x",
                    *data, *(data + 1), *(data + 2), *(data + 3),
                    *(data + 4), *(data + 5));

                ALOGD("New neighbor MAC address: %s", macaddr);
            }
        }
    }

    return ndm->ndm_state;
}

int WifiControl::_ip_probe(int ifindex, const char *ipaddr, char *macaddr)
{
    char *data;
    struct iovec iov;
    struct msghdr msg;
    struct {
        struct nlmsghdr nlh;
        struct ndmsg ndm;
        char buf[SEND_BUFSIZE];
    } req;
    struct rtattr *attr;

    memset(&req, 0, sizeof(req));
    req.nlh.nlmsg_type = RTM_NEWNEIGH;
    req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_REPLACE;
    req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
    req.ndm.ndm_family = AF_INET;
    req.ndm.ndm_state = NUD_PROBE;
    req.ndm.ndm_ifindex = ifindex;

    attr = (struct rtattr *)((char *)NLMSG_DATA(&req.nlh) + NLMSG_ALIGN(sizeof(struct ndmsg)));
    attr->rta_type = NDA_DST;
    attr->rta_len = RTA_LENGTH(IP_ADDR_LEN);
    inet_pton(AF_INET, ipaddr, RTA_DATA(attr));
    req.nlh.nlmsg_len = NLMSG_ALIGN(req.nlh.nlmsg_len) + RTA_LENGTH(IP_ADDR_LEN);
    ALOGD("probe ip: %s, unsigned int type: %u", ipaddr, *(int *)RTA_DATA(attr));

    attr = (struct rtattr *)((char *)attr + NLMSG_ALIGN(attr->rta_len));
    attr->rta_type = NDA_LLADDR;
    attr->rta_len = RTA_LENGTH(ETHER_ADDR_LEN);
    data = (char *)RTA_DATA(attr);
    *data = strtol(macaddr, 0, 16);
    *(data + 1) = strtol(macaddr + 3, 0, 16);
    *(data + 2) = strtol(macaddr + 6, 0, 16);
    *(data + 3) = strtol(macaddr + 9, 0, 16);
    *(data + 4) = strtol(macaddr + 12, 0, 16);
    *(data + 5) = strtol(macaddr + 15, 0, 16);
    req.nlh.nlmsg_len = NLMSG_ALIGN(req.nlh.nlmsg_len) + RTA_LENGTH(ETHER_ADDR_LEN);
    ALOGD("probe mac: %s", macaddr);

    iov = {&req, req.nlh.nlmsg_len};
    msg = {&_send_addr, sizeof(_send_addr), &iov, 1, NULL, 0, 0};
    sendmsg(_send_fd, &msg, 0);

    return 0;
}

bool WifiControl::_add_wifi_client_endpoint(const char *ipaddr)
{
    bool ret = false;
    char buf[SEND_BUFSIZE] = {0};
    sprintf(buf, ADD_ENDPOINT":%s", ipaddr);
    if (_send_command_to_router(buf, strlen(buf))) {
        return _handle_command_ack(ADD_ENDPOINT, &ret);
    }
    return false;
}

bool WifiControl::_remove_wifi_client_endpoint(const char *ipaddr)
{
    bool ret = false;
    char buf[SEND_BUFSIZE] = {0};
    sprintf(buf, REMOVE_ENDPOINT":%s", ipaddr);
    if (_send_command_to_router(buf, strlen(buf))) {
        return _handle_command_ack(REMOVE_ENDPOINT, &ret);
    }
    return false;
}

bool WifiControl::_handle_read(int fd, int fdtype)
{
    int type, state;
    struct msghdr msg;
    struct nlmsghdr *nlh;
    struct iovec iov;
    char ipaddr[INET_ADDRSTRLEN];
    char macaddr[ETHER_ADDRSTRLEN];
    char buf[RECV_BUFSIZE];
    int ifindex;
    int len;
    bool ret;
    char* prefix = Config::get_instance()->get_wifi_ip_address_prefix();

    if(fd != _recv_fd || fdtype != TYPE_OTHER_FD) {
        return false;
    }

    iov = {buf, sizeof(buf)};
    msg = {&_recv_addr, sizeof(_recv_addr), &iov, 1, NULL, 0, 0};
    bzero(buf, sizeof(buf));
    len = recvmsg(_recv_fd, &msg, 0);

    if (len <= 0) {
        return false;
    }

    if(!_in_wifi_ap_mode()) {
        ALOGD("not in ap mode, ignore message");
        return true;
    }
    for (nlh = (struct nlmsghdr *)buf; NLMSG_OK(nlh, (uint32_t)len); nlh = NLMSG_NEXT(nlh, len)) {
        type = nlh->nlmsg_type;
        ALOGV("Recevied neighbor message");
        if (type == RTM_NEWNEIGH || type == RTM_DELNEIGH) {
            state = _parse_msg(nlh, &ifindex, ipaddr, macaddr);
            if (!strncmp(ipaddr, prefix, strlen(prefix)) && (state & NUD_MONITOR)) {
                std::vector<std::string>::iterator it = station.begin();
                for (it; it < station.end(); it++) {
                    if (std::string(ipaddr) == (*it)) {
                        ALOGD("%s is in endpoint list", (*it).c_str());
                        break;
                    }
                }
                // add wifi client in any case
                if (state == NUD_REACHABLE) {
                    ret = _add_wifi_client_endpoint(ipaddr);
                }
                if (it == station.end()) {
                    if (state == NUD_STALE)
                        _ip_probe(ifindex, ipaddr, macaddr);
                    if (state == NUD_REACHABLE && ret) {
                        station.push_back(std::string(ipaddr));
                    }
                } else if (state == NUD_FAILED) {
                    if (_remove_wifi_client_endpoint(ipaddr)) {
                        station.erase(it);
                    }
                }
            }
        }
        if (type == NLMSG_DONE)
            break;
    }
    return true;
}

bool WifiControl::_send_command_to_router(char* data, int len)
{
    if(!_send_message(_router_fd, data, len,
                      Config::get_instance()->get_router_controller_name(),
                      TYPE_DOMAIN_SOCK_ABSTRACT)) {
        ALOGE("msg failed sending to router: %s", data);
        return false;
    }

    ALOGI("msg sent to router: %s", data);
    return true;
}

bool WifiControl::_handle_command_ack(const char* cmd, bool *ret)
{
    int bytes = 0;
    int retry = 0;
    char *p = NULL;
    char ack[RECV_BUFSIZE] = {0};
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(_router_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        ALOGE("can not set timeout for router fd: %d", errno);
        return false;
    }
    // wait 1 second for the response
    while (retry++ < 10) {
        bytes = ::recvfrom(_router_fd, ack, sizeof(ack), 0, NULL, NULL);
        if (bytes < 0) {
            ALOGI("error to read ack from router: %d", errno);
            continue;
        }
        if (bytes == 0) {
            ALOGI("can not read ack from router");
            continue;
        }
        ALOGI("ack from router: %s", ack);

        p =strchr(ack, ':');
        if (p != NULL) {
            if (!strncmp(cmd, ack, p-ack)) {
                *ret = strcmp("OK", p+1) ? false : true;
                return true;
            }
        }
        return false;
    }
    return false;
}

char* WifiControl::_get_wifi_ip_address() {
    struct sockaddr_in* addr;
    char* addr_string;
    struct ifreq ifr;
    int fd, ret;

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    fd = socket(AF_INET,SOCK_DGRAM,0);
    if (fd == -1) {
        return nullptr;
    }

    ret = ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);

    if (ret < 0) {
        return nullptr;
    } else {
        addr = (struct sockaddr_in*) &ifr.ifr_addr;
    }

    addr_string = inet_ntoa(addr->sin_addr);
    ALOGD("wifi ip is %s", addr_string);
    return addr_string;
}

bool WifiControl::_in_wifi_ap_mode()
{
    char* local_ip = _get_wifi_ip_address();
    if(local_ip == nullptr || strcmp(local_ip, Config::get_instance()->get_wifi_ap_ip_address()) != 0) {
        return false;
    }
    return true;
}

