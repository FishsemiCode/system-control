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
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <errno.h>
#include <mavlink.h>
#include "thread_base.h"

#define RX_BUF_SIZE 1024

enum {
    TYPE_DOMAIN_SOCK,
    TYPE_DOMAIN_SOCK_ABSTRACT
};

enum {
    TYPE_DATAGRAM_SOCK_FD,
    TYPE_TIMER_FD,
    TYPE_OTHER_FD
};

struct __attribute__((packed)) mavlink_router_mavlink2_header {
    uint8_t magic;
    uint8_t payload_len;
    uint8_t incompat_flags;
    uint8_t compat_flags;
    uint8_t seq;
    uint8_t sysid;
    uint8_t compid;
    uint32_t msgid : 24;
};

struct poll_event_data {
    void* module;
    int fd;
    int type;
};

class ModuleThread : public ThreadBase {
public:
    ModuleThread(const char* name);
    virtual ~ModuleThread();
    virtual bool start();
    virtual void stop();

protected:
    virtual void _thread_entry() override;
    bool _add_read_fd(int fd, int type);
    bool _add_timer(int* fd, uint32_t timeout_msec);
    int _get_domain_socket(const char* sock_name, int type, bool non_block = true);
    virtual bool _handle_read(int fd, int type);
    virtual bool _handle_timeout(int fd);
    virtual bool _process_data(int fd, uint8_t* buf, int len,
                              struct sockaddr* src_addr, int addrlen);
    bool _parse_mavlink_pack(uint8_t* buffer, uint32_t len, mavlink_message_t* msg);
    bool _send_message(int fd, const void *buf, size_t len,
                       const struct sockaddr *dest_addr, socklen_t addrlen);
    bool _send_message(int fd, const void *buf, size_t len,
                       const char* server_name, int server_type);

private:
    uint8_t* _rx_buffer;
    int _epoll_fd;
    bool _exit;
    const char* _module_name;
};
