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

#pragma once

#include <string>
#include <vector>
#include <linux/netlink.h>
#include <sys/un.h>
#include "module_thread.h"

class WifiControl : public ModuleThread {
public:
    WifiControl();

protected:
    virtual bool _handle_read(int fd, int type);
    bool _open_socket();
    int _parse_msg(struct nlmsghdr *nlh, int *ifindex, char *ipaddr, char *macaddr);
    int _ip_probe(int ifindex, const char *ipaddr, char *macaddr);
    bool _add_wifi_client_endpoint(const char *ipaddr);
    bool _remove_wifi_client_endpoint(const char *ipaddr);
    bool _send_command_to_router(char* buf, int len);
    bool _handle_command_ack(const char* cmd, bool *ret);
    char* _get_wifi_ip_address();
    bool _in_wifi_ap_mode();

private:
    int _send_fd;
    int _recv_fd;
    int _router_fd;
    struct sockaddr_nl _recv_addr;
    struct sockaddr_nl _send_addr;
    std::vector<std::string> station;
};
