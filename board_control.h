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
#include "module_thread.h"

class BoardControl : public ModuleThread {
public:
    BoardControl();
    virtual bool start() override;

protected:
    virtual bool _handle_timeout(int fd) override;
    virtual bool _process_data(int fd, uint8_t* buf, int len,
                               struct sockaddr* src_addr, int addrlen) override;
    void _send_time_sync();
    bool _send_time_sync_message(int64_t time);
    bool _send_board_temperature_message(int16_t temp);
    int _get_cpu_temperature(int* temp);
    int _get_board_temperature(int* temp);

private:
    int _timer_fd;
    int _sock_fd;
    int _last_board_temperature;
    uint8_t _system_id;
    uint8_t _comp_id;
    uint32_t _time_sync_counter;
    bool _time_sync_done;
};
