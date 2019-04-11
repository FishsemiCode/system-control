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
#include "camera_service.h"

typedef enum {
    CONNECTED = 0,
    DISCONNECTED
} D2D_SERVICE_STATUS;

struct d2d_info {
    /* used by QGC at controller side */
    D2D_SERVICE_STATUS service_status;    // connected or disconnected
    int rsrp;                             // LTE signal strength in "dBm"
    /* used by camera module at plane side */
    int ul_bandwidth;                     // uplink granted bandwidth in "kbps"
    int ul_rate;                          // uplink bit rate in "kbps"
    int snr;                              // signal-to-noise radio in "dB"
};

class D2dTracker : public ModuleThread {
public:
    D2dTracker();
    virtual bool start() override;

protected:
    virtual bool _handle_read(int fd, int type) override;
    bool _open_socket();
    int _read_d2d_info(int fd);
    bool _process_d2d_info();
    ssize_t _get_radio_packet(uint8_t *pBuf, uint8_t rssi, uint8_t noise);

private:
    int _last_tp_snr;
    int _last_service_status;
    int _bit_rate_adjust_mode;
    int _d2d_info_fd;
    int _rc_fd;
    int _router_fd;
    CameraService* _camera_service;
    d2d_info _d2d_info;
};
