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

#include <cutils/log.h>
#include "board_control.h"
#include "d2d_tracker.h"
#ifdef CAMERA_EXIST
#include "camera_control.h"
#endif
#include "wifi_control.h"
#include "config.h"
#undef LOG_TAG
#define LOG_TAG "SystemControl"

#define CONFIG_FILE "/system/etc/system-control.conf"

int main(int argc, char *argv[])
{
    BoardControl* board_control = nullptr;
    D2dTracker* d2d_tracker = nullptr;
#ifdef CAMERA_EXIST
    CameraControl* camera_control = nullptr;
#endif
    WifiControl* wifi_control = nullptr;
    const char* filename = CONFIG_FILE;

    ALOGD("start");

    if (argc > 1) {
        filename = argv[1];
    }
    Config::get_instance()->load_config(filename);

    if(Config::get_instance()->get_board_control_enabled()) {
        ALOGD("start board control module");
        board_control = new BoardControl();
        board_control->start();
    }
    if(Config::get_instance()->get_d2d_tracker_enabled()) {
        ALOGD("start d2d tracker module");
        d2d_tracker = new D2dTracker();
        d2d_tracker->start();
    }
#ifdef CAMERA_EXIST
    if(Config::get_instance()->get_camera_control_enabled()) {
        ALOGD("start camera control module");
        camera_control = new CameraControl();
        camera_control->start();
    }
#endif
    if(Config::get_instance()->get_wifi_control_enabled()) {
        ALOGD("start wifi control module");
        wifi_control = new WifiControl();
        wifi_control->start();
    }
    if(board_control) {
        board_control->wait_exit();
    }
    if(d2d_tracker) {
        d2d_tracker->wait_exit();
    }
#ifdef CAMERA_EXIST
    if(camera_control) {
        camera_control->wait_exit();
    }
#endif
    if(wifi_control) {
        wifi_control->wait_exit();
    }

    ALOGD("exit");
    return 0;
}
