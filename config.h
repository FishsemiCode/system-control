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
    void load_config();

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
};
