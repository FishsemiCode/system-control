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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <utils/Log.h>

#include "config.h"
#include "board_control.h"

#ifdef LAMP_SIGNAL_EXIST
#include "LampSignalService.h"
#include <binder/IServiceManager.h>
#endif

#undef LOG_TAG
#define LOG_TAG "BoardControl"
#define POLLING_RATE_TIMEOUT_MS (500)
#define TIME_SYNC_RATE 1
#define BOARD_CONTROL_SOCK_NAME "boardcontrol"

BoardControl::BoardControl()
    : ModuleThread{"BoardControl"}
    , _timer_fd(-1)
    , _sock_fd(-1)
    , _last_board_temperature(0)
    , _time_sync_counter(0)
    , _time_sync_done(false)
    , _cpu_temp(-1)
    , _battery_level(-1)
{
    _system_id = Config::get_instance()->get_board_system_id();
    _comp_id = Config::get_instance()->get_board_comp_id();

#ifdef LAMP_SIGNAL_EXIST
    _last_temp_state = NOT_WORKING;
    _last_battery_state = NOT_WORKING;
    sp<IBinder> binder = defaultServiceManager()->getService(String16(LampSignalService::getServiceName()));
    if (binder == nullptr) {
        ALOGE("failed to get service: %s", LampSignalService::getServiceName());
    } else {
        _listener = interface_cast<ISystemStatusListener>(binder);
        if (_listener == nullptr) {
            ALOGE("failed to cast LampSignalService interface");
        }
    }
#endif

}

bool BoardControl::start()
{
    if (!_add_timer(&_timer_fd, POLLING_RATE_TIMEOUT_MS)) {
        ALOGE("Unable to add timerfd");
        goto fail;
    }
    _sock_fd = _get_domain_socket(BOARD_CONTROL_SOCK_NAME,
                                 TYPE_DOMAIN_SOCK_ABSTRACT);
    if (_sock_fd < 0) {
        ALOGE("Unable to create sockfd");
        goto fail;
    }
    if (!_add_read_fd(_sock_fd, TYPE_DATAGRAM_SOCK_FD)) {
        ALOGE("Unable to add _sock_fd to epoll");
        goto fail;
    }
    return ModuleThread::start();

fail:
    if (_timer_fd >= 0) {
        ::close(_timer_fd);
        _timer_fd = -1;
    }
    if (_sock_fd >= 0) {
        ::close(_sock_fd);
        _sock_fd = -1;
    }
    return false;
}

bool BoardControl::_handle_timeout(int fd)
{
    int ret;
    int temperature;
    if (_timer_fd == fd) {
        if (Config::get_instance()->get_in_air()) {
            // sendout board temperature, only in air
            ret = _get_board_temperature(&temperature);
            if (ret == 0 && temperature != _last_board_temperature) {
                _last_board_temperature = temperature;
                 ALOGD("temperature changed to %d", temperature);
                _send_board_temperature_message(_last_board_temperature/10);
            }
            // sync time request with gcs, only in air
            _send_time_sync_request();
#ifdef LAMP_SIGNAL_EXIST
        } else if (_listener != nullptr) {
            // update cpu temp
            _get_cpu_temperature(&_cpu_temp);
            // update battery state
            _get_battery_stat(&_battery_level);
            _signal_lamp_service();
#endif
        }
        return true;
    }
    return false;
}

bool BoardControl::_process_data(int fd, uint8_t* buf, int len,
                   struct sockaddr* src_addr, int addrlen)
{
    uint32_t msgid;
    uint8_t src_sysid, src_compid;
    uint8_t payload_len;
    uint8_t* payload;
    mavlink_message_t msg;
    if(fd != _sock_fd) {
        return false;
    }
    if (!_parse_mavlink_pack(buf, len, &msg)) {
        return false;
    }
    if (msg.msgid == MAVLINK_MSG_ID_TIMESYNC) {
        timeval tv;
        mavlink_timesync_t timesync;
        mavlink_msg_timesync_decode(&msg, &timesync);

        if (!Config::get_instance()->get_in_air()) {
            // acting as time server, respond the request
            if ((timesync.tc1 > 0) && (timesync.ts1 == 0)) {
                if (gettimeofday(&tv, NULL) == 0) {
                    ALOGD("response time sync from %ld to %ld", timesync.tc1, tv.tv_sec);
                    _send_time_sync_message(timesync.tc1, tv.tv_sec);
                } else {
                    ALOGE("gettimeofday failed! %d", errno);
                }
            }
        } else {
            // acting as time client, set time according to the response
            if(timesync.ts1 > 0) {
                _time_sync_done = true;
            }
            if (gettimeofday(&tv, NULL) == 0) {
                int64_t delta = timesync.ts1 - tv.tv_sec;
                if(delta > 60 || delta < -60) {
                    ALOGD("try to set time from %ld to %ld", tv.tv_sec, timesync.ts1);
                    tv.tv_sec = timesync.ts1;
                    tv.tv_usec = 0;
                    if(settimeofday(&tv, NULL) != 0) {
                        ALOGE("failed to settimeofday! %d", errno);
                    }
                }
            } else {
                ALOGE("gettimeofday failed! %d", errno);
            }
        }
    }
    return true;
}

void BoardControl::_send_time_sync_request()
{
    uint32_t sync_rate = TIME_SYNC_RATE;
    if (_time_sync_done) {
        sync_rate *= 60;
    }
    if (++_time_sync_counter >= sync_rate) {
        _time_sync_counter = 0;
        timeval tv;
        if (gettimeofday(&tv, NULL) == 0) {
            ALOGV("gettimeofday %ld", tv.tv_sec);
            _send_time_sync_message(tv.tv_sec, 0);
        } else {
            ALOGD("gettimeofday failed!");
        }
    }
}

bool BoardControl::_send_time_sync_message(int64_t tc1, int64_t ts1)
{
    uint8_t packet[MAVLINK_MAX_PACKET_LEN];
    int len;
    mavlink_message_t msg;

    mavlink_msg_timesync_pack(_system_id, _comp_id, &msg, tc1, ts1);
    len = mavlink_msg_to_send_buffer(packet, &msg);
    return _send_message(_sock_fd, packet, len,
                         Config::get_instance()->get_board_endpoint_name(),
                         TYPE_DOMAIN_SOCK_ABSTRACT);
}

bool BoardControl::_send_board_temperature_message(int16_t temp)
{
    uint8_t packet[MAVLINK_MAX_PACKET_LEN];
    int len;
    mavlink_message_t msg;

    mavlink_msg_scaled_pressure_pack(_system_id, _comp_id, &msg,
                                         0, 0, 0, temp);

    len = mavlink_msg_to_send_buffer(packet, &msg);
    return _send_message(_sock_fd, packet, len,
                         Config::get_instance()->get_board_endpoint_name(),
                         TYPE_DOMAIN_SOCK_ABSTRACT);
}

int BoardControl::_get_board_temperature(int* temp)
{
    FILE *fp;
    char line[256];

    fp = popen("cat /sys/bus/iio/devices/iio:device1/in_voltage2_adc2_input", "r");
    if (fp == NULL) {
        ALOGE("Failed to read in_voltage2_adc2_input!");
        return -1;
    }

    /* Read the output a line at a time - output it. */
    if (fgets(line, sizeof(line)-1, fp) != NULL) {
        ALOGV("in_voltage2_adc2_input is %s", line);
        *temp = atoi(line);
    } else {
        ALOGE("reading null from in_voltage2_adc2_input!");
        return -1;
    }
    pclose(fp);
    return 0;
}

int BoardControl::_get_cpu_temperature(int* temp)
{
    FILE *fp;
    char line[256];

    fp = popen("cat /sys/class/thermal/thermal_zone4/temp", "r");
    if (fp == NULL) {
        ALOGE("Failed to read thermal_zone4 temp!");
        return -1;
    }

    /* Read the output a line at a time - output it. */
    if (fgets(line, sizeof(line)-1, fp) != NULL) {
        ALOGV("cpu temp is %s", line);
        *temp = atoi(line);
    } else {
        ALOGE("reading null from thermal_zone4 temp!");
        return -1;
    }
    pclose(fp);
    return 0;
}

int BoardControl::_get_battery_stat(int* battery_level)
{
    FILE *fp;
    char line[256];

    fp = popen("cat /sys/class/power_supply/battery/capacity", "r");
    if (fp == NULL) {
        ALOGE("Failed to read battery capacity!");
        return -1;
    }

    /* Read the output a line at a time - output it. */
    if (fgets(line, sizeof(line)-1, fp) != NULL) {
        ALOGV("battery level is %s", line);
        *battery_level = atoi(line);
    } else {
        ALOGE("reading null from battery capacity!");
        return -1;
    }
    pclose(fp);
    return 0;
}

void BoardControl::_signal_lamp_service()
{
#ifdef LAMP_SIGNAL_EXIST
    if (_listener == nullptr) {
       ALOGE("_listener invalid");
       return;
    }
    SystemStatus state;
    if (_cpu_temp > Config::get_instance()->get_cpu_temperature_high_value()) {
        state = TEMP_ABNORMAL;
    } else {
        state = TEMP_NORMAL;
    }
    if (state != _last_temp_state) {
        _last_temp_state = state;
        ALOGD("cpu temp state changed to %d", state);
    }
    _listener->onStatusChanged(_last_temp_state, false);
    if (_battery_level < Config::get_instance()->get_battery_level_low_value()) {
        state = POWER_ABNORMAL;
    } else {
        state = POWER_NORMAL;
    }
    if (state != _last_battery_state) {
        _last_battery_state = state;
        ALOGD("battery state changed to %d", state);
    }
    _listener->onStatusChanged(_last_battery_state, false);
#endif
}
