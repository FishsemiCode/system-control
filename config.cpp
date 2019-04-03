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

#include <stdlib.h>
#include <utils/Log.h>

#include "mavlink.h"
#include "config.h"

#undef LOG_TAG
#define LOG_TAG "Config"
#define CONFIG_FILE "/system/etc/system-control.conf"
#define MAX_LINES 32
#define MAX_LINE_TEXT 128

#define DEFAULT_BOARD_ENDPOINT_NAME     ((char*)"boardendpoint")
#define DEFAULT_CAMERA_ENDPOINT_NAME    ((char*)"cameraendpoint")
#define DEFAULT_RC_SOCKET_NAME          ((char*)"/tmp/unix_radio")
#define DEFAULT_VIDEO_STREAM_IP_ADDRESS ((char*)"192.168.0.10")
#define DEFAULT_BOARD_SYSTEM_ID         10
#define DEFAULT_BOARD_COMP_ID           MAV_COMP_ID_SYSTEM_CONTROL
#define DEFAULT_CAMERA_SYSTEM_ID        10
#define DEFAULT_CAMERA_COMP_ID          MAV_COMP_ID_CAMERA

Config* Config::_instance = nullptr;

Config::Config()
    : _board_endpoint_name(DEFAULT_BOARD_ENDPOINT_NAME)
    , _camera_endpoint_name(DEFAULT_CAMERA_ENDPOINT_NAME)
    , _rc_socket_name(DEFAULT_RC_SOCKET_NAME)
    , _video_stream_ip_address(DEFAULT_VIDEO_STREAM_IP_ADDRESS)
    , _board_system_id(DEFAULT_BOARD_SYSTEM_ID)
    , _board_comp_id(DEFAULT_BOARD_COMP_ID)
    , _camera_system_id(DEFAULT_CAMERA_SYSTEM_ID)
    , _camera_comp_id(DEFAULT_CAMERA_COMP_ID)
    , _support_multiple_camera(false)
    , _support_camera_capture(false)
{
}

Config* Config::get_instance()
{
    if (_instance == nullptr) {
        _instance = new Config();
    }
    return _instance;
}

char* Config::get_board_endpoint_name()
{
    return _board_endpoint_name;
}

char* Config::get_camera_endpoint_name()
{
    return _camera_endpoint_name;
}

char* Config::get_rc_socket_name()
{
    return _rc_socket_name;
}

char* Config::get_video_stream_ip_address()
{
    return _video_stream_ip_address;
}

int Config::get_board_system_id()
{
    return _board_system_id;
}

int Config::get_board_comp_id()
{
    return _board_comp_id;
}

int Config::get_camera_system_id()
{
    return _camera_system_id;
}

int Config::get_camera_comp_id()
{
    return _camera_comp_id;
}

bool Config::get_support_multiple_camera()
{
    return _support_multiple_camera;
}

bool Config::get_support_camera_capture()
{
    return _support_camera_capture;
}

void Config::load_config()
{
    char buffer[MAX_LINES][MAX_LINE_TEXT];
    const char* delimiters = " =;\n";
    size_t linesRead = 0;
    char* string;

    FILE *pFile = fopen(CONFIG_FILE, "r");
    if (!pFile) {
        return;
    }

    while (linesRead < MAX_LINES && fgets(buffer[linesRead], MAX_LINE_TEXT, pFile) != NULL) {
        if (buffer[linesRead][0] == '\n' || buffer[linesRead][0] == '#') {
            continue;
        }

        string = strtok(buffer[linesRead], delimiters);
        if (!string) {
            continue;
        }

        if (strcmp(string, "board_endpoint_name") == 0) {
            get_string_value(&_board_endpoint_name, delimiters);
        } else if (strcmp(string, "camera_endpoint_name") == 0) {
            get_string_value(&_camera_endpoint_name, delimiters);
        } else if (strcmp(string, "rc_socket_name") == 0) {
            get_string_value(&_rc_socket_name, delimiters);
        } else if (strcmp(string, "video_stream_ip_address") == 0) {
            get_string_value(&_video_stream_ip_address, delimiters);
        } else if (strcmp(string, "board_system_id") == 0) {
            get_int_value(&_board_system_id, delimiters);
        } else if (strcmp(string, "board_comp_id") == 0) {
            get_int_value(&_board_comp_id, delimiters);
        } else if (strcmp(string, "camera_system_id") == 0) {
            get_int_value(&_camera_system_id, delimiters);
        } else if (strcmp(string, "camera_comp_id") == 0) {
            get_int_value(&_camera_comp_id, delimiters);
        } else if (strcmp(string, "support_multiple_camera") == 0) {
            get_bool_value(&_support_multiple_camera, delimiters);
        } else if (strcmp(string, "support_camera_capture") == 0) {
            get_bool_value(&_support_camera_capture, delimiters);
        } else {
            continue;
        }
        linesRead++;
    }
    fclose(pFile);
}

bool Config::get_string_value(char** value, const char* delimiters) {
    char* string = strtok(NULL, delimiters);
    if (!string) {
        return false;
    }
    *value = strdup(string);
    return true;
}

bool Config::get_int_value(int* value, const char* delimiters) {
    char* string = strtok(NULL, delimiters);
    if (!string) {
        return false;
    }
    *value = atoi(string);
    return true;
}

bool Config::get_bool_value(bool* value, const char* delimiters) {
    char* string = strtok(NULL, delimiters);
    if (!string) {
        return false;
    }
    if (strcmp(string, "true") == 0) {
        *value = true;
    } else if (strcmp(string, "false") == 0) {
        *value = false;
    } else {
        ALOGE("invalid bool value [%s]", string);
        return false;
    }
    return true;
}
