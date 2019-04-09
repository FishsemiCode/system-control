/*
 * Copyright (C) 2019 Pinecone Inc. All rights reserved.
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

#include "board_control.h"
#include "d2d_tracker.h"
#include "camera_control.h"
#include "config.h"

#define CONFIG_FILE "/system/etc/system-control.conf"

int main(int argc, char *argv[])
{
    BoardControl* board_control;
    D2dTracker* d2d_tracker;
    CameraControl* camera_control;
    const char* filename = CONFIG_FILE;

    if (argc > 1) {
        filename = argv[1];
    }
    Config::get_instance()->load_config(filename);

    board_control = new BoardControl();
    board_control->start();
    d2d_tracker = new D2dTracker();
    d2d_tracker->start();
    camera_control = new CameraControl();
    camera_control->start();

    board_control->wait_exit();
    d2d_tracker->wait_exit();
    camera_control->wait_exit();

    return 0;
}
