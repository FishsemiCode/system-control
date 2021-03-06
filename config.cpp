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

#include <stdlib.h>
#include <errno.h>
#include <utils/Log.h>

#include "mavlink.h"
#include "config.h"

#undef LOG_TAG
#define LOG_TAG "Config"
#define MAX_LINES 32
#define MAX_LINE_TEXT 128

#define NULL_STRING                     ((char*)"")
#define NULL_STRING_CONFIG              ((char*)"null")
#define DEFAULT_BOARD_ENDPOINT_NAME     ((char*)"boardendpoint")
#define DEFAULT_CAMERA_ENDPOINT_NAME    ((char*)"cameraendpoint")
#define DEFAULT_RC_SOCKET_NAME          ((char*)"/tmp/unix_radio")
#define DEFAULT_VIDEO_STREAM_IP_ADDRESS ((char*)"192.168.0.10")
#define DEFAULT_BOARD_SYSTEM_ID         10
#define DEFAULT_BOARD_COMP_ID           MAV_COMP_ID_SYSTEM_CONTROL
#define DEFAULT_CAMERA_SYSTEM_ID        10
#define DEFAULT_CAMERA_COMP_ID          MAV_COMP_ID_CAMERA
#define DEFAULT_WIFI_AP_IP_ADDRESS      ((char*)"192.168.43.1")
#define DEFAULT_WIFI_IP_ADDRESS_PREFIX  ((char*)"192.168.43.")
#define DEFAULT_ROUTER_CONTROLLER_NAME  ((char*)"routercontroller")
#define DEFAULT_CPU_TEMPERATURE_HIGH_V  95000 //(95 degrees Celsius)
#define DEFAULT_BATTERY_LEVEL_LOW_V     20
#define DEFAULT_BOARD_CONTROL_ENABLED   false
#define DEFAULT_CAMERA_CONTROL_ENABLED  false
#define DEFAULT_WIFI_CONTROL_ENABLED    false
#define DEFAULT_D2D_TRACKER_ENABLED     false
#define DEFAULT_IN_AIR                  true

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
	, _wifi_ap_ip_address(DEFAULT_WIFI_AP_IP_ADDRESS)
	, _wifi_ip_address_prefix(DEFAULT_WIFI_IP_ADDRESS_PREFIX)
	, _router_controller_name(DEFAULT_ROUTER_CONTROLLER_NAME)
	, _cpu_temperature_high_value(DEFAULT_CPU_TEMPERATURE_HIGH_V)
	, _battery_level_low_value(DEFAULT_BATTERY_LEVEL_LOW_V)
	, _board_control_enabled(DEFAULT_BOARD_CONTROL_ENABLED)
	, _camera_control_enabled(DEFAULT_CAMERA_CONTROL_ENABLED)
	, _wifi_control_enabled(DEFAULT_WIFI_CONTROL_ENABLED)
	, _d2d_tracker_enabled(DEFAULT_D2D_TRACKER_ENABLED)
	, _in_air(DEFAULT_IN_AIR)
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

char* Config::get_wifi_ap_ip_address()
{
    return _wifi_ap_ip_address;
}

char* Config::get_wifi_ip_address_prefix()
{
    return _wifi_ip_address_prefix;
}

char* Config::get_router_controller_name()
{
    return _router_controller_name;
}

int Config::get_cpu_temperature_high_value()
{
    return _cpu_temperature_high_value;
}

int Config::get_battery_level_low_value()
{
    return _battery_level_low_value;
}

bool Config::get_board_control_enabled()
{
    return _board_control_enabled;
}

bool Config::get_camera_control_enabled()
{
    return _camera_control_enabled;
}

bool Config::get_wifi_control_enabled()
{
    return _wifi_control_enabled;
}

bool Config::get_d2d_tracker_enabled()
{
    return _d2d_tracker_enabled;
}

bool Config::get_in_air()
{
    return _in_air;
}

void Config::load_config(const char* filename)
{
    char buffer[MAX_LINES][MAX_LINE_TEXT];
    const char* delimiters = " =;\n";
    size_t linesRead = 0;
    char* string;

    if(filename == nullptr) {
        return;
    }
    FILE *pFile = fopen(filename, "r");
    if (!pFile) {
        ALOGE("Failed to open config file %s, %d", filename, errno);
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
        } else if (strcmp(string, "wifi_ap_ip_address") == 0) {
            get_string_value(&_wifi_ap_ip_address, delimiters);
        } else if (strcmp(string, "wifi_ip_address_prefix") == 0) {
            get_string_value(&_wifi_ip_address_prefix, delimiters);
        } else if (strcmp(string, "router_controller_name") == 0) {
            get_string_value(&_router_controller_name, delimiters);
        } else if (strcmp(string, "cpu_temperature_high_value") == 0) {
            get_int_value(&_cpu_temperature_high_value, delimiters);
        } else if (strcmp(string, "battery_level_low_value") == 0) {
            get_int_value(&_battery_level_low_value, delimiters);
        } else if (strcmp(string, "board_control_enabled") == 0) {
            get_bool_value(&_board_control_enabled, delimiters);
        } else if (strcmp(string, "camera_control_enabled") == 0) {
            get_bool_value(&_camera_control_enabled, delimiters);
        } else if (strcmp(string, "wifi_control_enabled") == 0) {
            get_bool_value(&_wifi_control_enabled, delimiters);
        } else if (strcmp(string, "d2d_tracker_enabled") == 0) {
            get_bool_value(&_d2d_tracker_enabled, delimiters);
        } else if (strcmp(string, "in_air") == 0) {
            get_bool_value(&_in_air, delimiters);
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
    if (!strcmp(string, NULL_STRING_CONFIG)) {
        *value = NULL_STRING;
    } else {
        *value = strdup(string);
    }
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
