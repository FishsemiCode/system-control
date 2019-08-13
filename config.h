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

class Config {
public:
    static Config* get_instance();
    char* get_board_endpoint_name();
    char* get_camera_endpoint_name();
    char* get_rc_socket_name();
    char* get_video_stream_ip_address();
    int get_board_system_id();
    int get_board_comp_id();
    int get_camera_system_id();
    int get_camera_comp_id();
    bool get_support_multiple_camera();
    bool get_support_camera_capture();
    char* get_wifi_ap_ip_address();
    char* get_wifi_ip_address_prefix();
	char* get_router_controller_name();
    int get_cpu_temperature_high_value();
    int get_battery_level_low_value();
    bool get_board_control_enabled();
    bool get_camera_control_enabled();
    bool get_wifi_control_enabled();
    bool get_d2d_tracker_enabled();
    bool get_in_air();
    void load_config(const char* filename);

private:
    Config();
    bool get_string_value(char** value, const char* delimiters);
    bool get_int_value(int* value, const char* delimiters);
    bool get_bool_value(bool* value, const char* delimiters);

    static Config* _instance;
    char* _board_endpoint_name;
    char* _camera_endpoint_name;
    char* _rc_socket_name;
    char* _video_stream_ip_address;
    int _board_system_id;
    int _board_comp_id;
    int _camera_system_id;
    int _camera_comp_id;
    bool _support_multiple_camera;
    bool _support_camera_capture;
    char* _wifi_ap_ip_address;
    char* _wifi_ip_address_prefix;
    char* _router_controller_name;
    int _cpu_temperature_high_value;
    int _battery_level_low_value;
    bool _board_control_enabled;
    bool _camera_control_enabled;
    bool _wifi_control_enabled;
    bool _d2d_tracker_enabled;
    bool _in_air;
};
