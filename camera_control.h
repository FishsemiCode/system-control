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

#include <sys/un.h>
#include "camera_service.h"
#include "module_thread.h"

class CameraControl : public ModuleThread {
public:
    CameraControl();
    void broadcast_heartbeat();
    bool camera_ready();

protected:
    static void _heartbeat_timeout(union sigval v);
    bool _start_heartbeat();
    virtual bool _process_data(int fd, uint8_t* buf, int len,
                               struct sockaddr* src_addr, int addrlen) override;
    void _send_ack(int cmd, bool success);
    void _send_mavlink_msg(mavlink_message_t* pMsg);
    void _handle_camera_info_request();
    void _handle_camera_video_stream_request();
    void _handle_camera_set_video_stream_settings(mavlink_message_t *payload);
    void _handle_video_start_streaming(int id);
    void _handle_video_stop_streaming();
    void _handle_video_start_recording();
    void _handle_video_stop_recording();
    void _handle_capture_photo_image();
    void _handle_request_capture_status();
    void _handle_camera_settings_request();
    void _handle_set_camera_mode(int mode);
    void _handle_storage_info_request();
    void _send_camera_setting_info(int mode);
    int _open_camera_waiton_busy();
    int _start_preview_waiton_busy();
    int _stop_preview_waiton_busy();
    int _start_video_recording_waiton_busy();
    int _stop_video_recording_waiton_busy();
private:
    uint8_t _system_id;
    uint8_t _comp_id;
    uint8_t _src_sys_id;
    uint8_t _src_comp_id;
    int _preview_width;
    int _preview_height;
    bool _is_camera_ready;
    uint32_t _uid;
    int32_t _camera_id;
    int32_t _camera_count;
    int _router_fd;
    timer_t _timer_id;
    CameraService* _cam_service;
};
