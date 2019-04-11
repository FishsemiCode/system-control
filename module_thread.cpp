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

#include <assert.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <cutils/log.h>
#include "module_thread.h"

#undef LOG_TAG
#define LOG_TAG "ModuleThread"

#define MSEC_PER_SEC  1000
#define NSEC_PER_MSEC (1000 * 1000)

ModuleThread::ModuleThread(const char* name)
    : _exit(false),
      _module_name(name)
{
    _rx_buffer = (uint8_t *) malloc(RX_BUF_SIZE);
    assert(_rx_buffer);
    _epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (_epoll_fd == -1) {
        ALOGE("creat epoll failed %s", _module_name);
    }
}

ModuleThread::~ModuleThread()
{
    free(_rx_buffer);
}

bool ModuleThread::start()
{
    return start_thread();
}

void ModuleThread::stop()
{
    _exit = true;
}

void ModuleThread::_thread_entry()
{
    const int max_events = 4;
    struct epoll_event events[max_events];
    int r;
    int i;

    while (!_exit) {
        r = epoll_wait(_epoll_fd, events, max_events, -1);
        if (r < 0 && errno == EINTR) {
            continue;
        }
        for (i = 0; i < r; i++) {
            poll_event_data* d = (poll_event_data*)(events[i].data.ptr);
            ModuleThread* p = static_cast<ModuleThread*>(d->module);
            if (events[i].events & EPOLLIN) {
                p->_handle_read(d->fd, d->type);
            }
        }
    }
}

bool ModuleThread::_add_read_fd(int fd, int type)
{
    struct epoll_event epev = { };

    epev.events = EPOLLIN;
    epev.data.ptr = new poll_event_data{this, fd, type};

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &epev) < 0) {
        ALOGE("Could not add domain sock fd %d to epoll in %s", fd, _module_name);
        return false;
    }
    return true;
}

bool ModuleThread::_add_timer(int* fd, uint32_t timeout_msec)
{
    struct itimerspec ts;

    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd < 0) {
        ALOGE("Unable to create timerfd");
        return false;
    }

    ts.it_interval.tv_sec = timeout_msec / MSEC_PER_SEC;
    ts.it_interval.tv_nsec = (timeout_msec % MSEC_PER_SEC) * NSEC_PER_MSEC;
    ts.it_value.tv_sec = ts.it_interval.tv_sec;
    ts.it_value.tv_nsec = ts.it_interval.tv_nsec;
    timerfd_settime(timer_fd, 0, &ts, NULL);

    if(_add_read_fd(timer_fd, TYPE_TIMER_FD)) {
        *fd = timer_fd;
        return true;
    } else {
        ::close(timer_fd);
        return false;
    }
}

int ModuleThread::_get_domain_socket(const char* sock_name, int type)
{
    int flags = 0;
    int fd = -1;
    struct sockaddr_un sockaddr;
    int sockaddr_len;

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd == -1) {
        ALOGE("Could not create socket");
        return -1;
    }

    if (sock_name != NULL) {
        bzero((void*)&sockaddr, sizeof(sockaddr));
        sockaddr.sun_family = AF_UNIX;
        if(type == TYPE_DOMAIN_SOCK_ABSTRACT) {
            sockaddr.sun_path[0] = 0;
            strcpy(sockaddr.sun_path+1, sock_name);
            sockaddr_len = strlen(sock_name) + offsetof(struct sockaddr_un, sun_path) + 1;
        } else {
            strcpy(sockaddr.sun_path, sock_name);
            sockaddr_len = sizeof(sockaddr);
        }
        if (bind(fd, (struct sockaddr *) &sockaddr, sockaddr_len)) {
            ALOGE("Error binding socket to %s", sock_name);
            goto fail;
        }
    }

    if ((flags = fcntl(fd, F_GETFL, 0) == -1)) {
        ALOGE("Error getfl for fd");
        goto fail;
    }
    if (fcntl(fd, F_SETFL, O_NONBLOCK | flags) < 0) {
        ALOGE("Error setting socket fd as non-blocking");
        goto fail;
    }

    ALOGD("get domain socket [%d] [%s]", fd, sock_name ? sock_name : " ");
    return fd;

fail:
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
    return -1;
}

bool ModuleThread::_handle_read(int fd, int type)
{
    uint64_t val = 0;
    int ret;
    struct sockaddr_un src_addr;
    socklen_t addrlen;

    if(type == TYPE_TIMER_FD) {
        ret = read(fd, &val, sizeof(val));
        if (ret < 1 || val == 0) {
            return false;
        }
        return _handle_timeout(fd);
    } else if (type == TYPE_DATAGRAM_SOCK_FD) {
        bzero((void*)&src_addr, sizeof(src_addr));
        addrlen = sizeof(src_addr);
        ssize_t r = ::recvfrom(fd, _rx_buffer, RX_BUF_SIZE, 0, (struct sockaddr*)&src_addr, &addrlen);

        if (r == -1) {
            if(errno != EAGAIN) {
               ALOGE("_handle_read receive from fd error %d", errno);
            }
            return false;
        }
        if (r == 0) {
            ALOGE("_handle_read receive empty data");
            return false;
        }
        return _process_data(fd, _rx_buffer, r, (struct sockaddr*)&src_addr, addrlen);
    } else {
        ALOGE("_handle_read should be overriden to read other fd in %s", _module_name);
        return false;
    }
}

bool ModuleThread::_handle_timeout(int fd)
{
    return true;
}

bool ModuleThread::_process_data(int fd, uint8_t* buf, int len,
                   struct sockaddr* src_addr, int addrlen)
{
    return true;
}

bool ModuleThread::_parse_mavlink_pack(uint8_t* buffer, uint32_t len, mavlink_message_t* msg)
{
    uint32_t i;
    mavlink_status_t comm;
    for (i=0; i < len; i++) {
       if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], msg, &comm)) {
           return true;
       }
    }
    ALOGE("mavlink message parse failure in %s", _module_name);
    return false;
}

bool ModuleThread::_send_message(int fd, const void *buf, size_t len,
                               const struct sockaddr *dest_addr, socklen_t addrlen)
{
    ssize_t r = ::sendto(fd, buf, len, 0, dest_addr, addrlen);
    if (r < 0) {
        ALOGE("send [%d] failed in %s errno %d", fd, _module_name, errno);
        return false;
    }
    if (r != (ssize_t)len) {
        ALOGE("send [%d] failed in %s, expected %zu but sent %zd", fd, _module_name, len, r);
        return false;
    }
    ALOGV("sent message success to [%d] in %s", fd, _module_name);
    return true;
}

bool ModuleThread::_send_message(int fd, const void *buf, size_t len,
                               const char* server_name, int server_type)
{
    struct sockaddr_un sockaddr;
    socklen_t sockaddr_len;

    if (server_name == nullptr) {
        ALOGE("server is null!");
        return false;
    }
    bzero((void*)&sockaddr, sizeof(sockaddr));
    sockaddr.sun_family = AF_UNIX;
    if(server_type == TYPE_DOMAIN_SOCK_ABSTRACT) {
        sockaddr.sun_path[0] = 0;
        strcpy(sockaddr.sun_path+1, server_name);
        sockaddr_len = strlen(server_name) + offsetof(struct sockaddr_un, sun_path) + 1;
    } else {
        strcpy(sockaddr.sun_path, server_name);
        sockaddr_len = sizeof(sockaddr);
    }

    return _send_message(fd, buf, len, (const struct sockaddr*)&sockaddr, sockaddr_len);
}
