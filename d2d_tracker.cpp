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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <sys/un.h>
#include <netinet/in.h>

#include <utils/Log.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include "config.h"
#include "d2d_tracker.h"

#undef LOG_TAG
#define LOG_TAG "D2dTracker"
#define D2D_SOCKET_NAME "d2dinfo"
#define D2D_MAX_MESSAGE_BYTES 100
// potential need for dual controllers
#define SOCKET_MAX_NUM 2

// d2d info message tag definition
#define D2D_SERVICE_STATUS_TAG      "SRV_STAT"
#define D2D_SIGNAL_STRENGTH_TAG     "RSRP"
#define D2D_UL_GRANT_BANDWIDTH_TAG  "UL_BW"
#define D2D_UL_DATA_RATE_TAG        "UL_RATE"
#define D2D_SNR_TAG                 "SNR"

D2dTracker::D2dTracker() : ModuleThread{"D2dTracker"}
{
    char prop_buf[10] = {0};
    _last_tp_snr = 100;
    _last_service_status = DISCONNECTED;
    property_get("persist.camera.bitrate.adjust.mode", prop_buf, "0");
    _bit_rate_adjust_mode = atoi(prop_buf); // 0: by throughput  1: by snr
    _d2d_info_fd = -1;
    _rc_fd = -1;
    _router_fd = -1;
    _camera_service = CameraService::get_instance();
    bzero((void*)&_d2d_info, sizeof(_d2d_info));

}

bool D2dTracker::start()
{
    if(_open_socket()) {
        return ModuleThread::start();
    }
    return false;
}

bool D2dTracker::_open_socket()
{
    // socket to receive d2d info from d2d service
    _d2d_info_fd = socket_local_server(D2D_SOCKET_NAME,
                                       ANDROID_SOCKET_NAMESPACE_ABSTRACT,
                                       SOCK_STREAM);

    if(_d2d_info_fd < 0) {
        ALOGE("fail to create d2d info socket");
        return false;
    }
    listen(_d2d_info_fd, SOCKET_MAX_NUM);
    _add_read_fd(_d2d_info_fd, TYPE_OTHER_FD);

    // socket to send message to rc service
    _rc_fd = _get_domain_socket(NULL, 0,
                                Config::get_instance()->get_rc_socket_name(),
                                TYPE_DOMAIN_SOCK);
    if(_rc_fd < 0) {
        ALOGE("fail to create rc socket");
    }

    // socket to send message to mavlink router
    _router_fd = _get_domain_socket(NULL, 0,
                                    Config::get_instance()->get_board_endpoint_name(),
                                    TYPE_DOMAIN_SOCK_ABSTRACT);
    if(_router_fd < 0) {
        ALOGE("fail to create router socket");
    }
    return true;
}

bool D2dTracker::_handle_read(int fd, int type)
{
    int acceptFD;
    int length;
    struct sockaddr peeraddr;
    socklen_t socklen = sizeof(peeraddr);

    if((fd != _d2d_info_fd) || (type != TYPE_OTHER_FD)) {
       return false;
    }
    acceptFD = accept(fd, &peeraddr, &socklen);
    ALOGV("D2dTracker accept done");

    if (acceptFD < 0) {
       ALOGE("error accepting on d2d socket: %d\n", errno);
       return false;
    }

    length = _read_d2d_info(acceptFD);
    if (length <= 0) {
        return false;
    }
    close(acceptFD);

    return _process_d2d_info();
}

int D2dTracker::_read_d2d_info(int socket) {
    char buffer[D2D_MAX_MESSAGE_BYTES] = {0};
    char delims[] = " ";
    char *msg_tag = NULL;
    char *value   = NULL;
    int  msg_len  = 0;
    int  read_len = 0;

    // First, read the length of the message
    read_len = recv(socket, &msg_len, sizeof(int), 0);
    ALOGV("read %d bytes from socket", read_len);
    if (read_len <= 0) {
        ALOGV("Hit EOS reading message length");
        return -1;
    } else {
        msg_len = ntohl(msg_len);
        ALOGV("the length of received d2d info message is %d bytes", msg_len);
    }

    // Then, read the message body with specific length
    read_len = recv(socket, &buffer, msg_len, 0);
    ALOGV("read %d bytes from socket", read_len);
    if (read_len <= 0) {
        ALOGE("Hit EOS reading message length, failed to read message body");
        return -1;
    }

    msg_tag = strtok(buffer, delims);
    if (msg_tag != NULL) {
        value = strtok(NULL, delims);
        if (value == NULL) {
            ALOGE("subsequent strtok failed, ignore");
            return -1;
        }

        if (!strcmp(msg_tag, D2D_SERVICE_STATUS_TAG)) {
            _d2d_info.service_status = (!strcmp(value, "1")) ? CONNECTED : DISCONNECTED;
        } else if (!strcmp(msg_tag, D2D_SIGNAL_STRENGTH_TAG)) {
            _d2d_info.rsrp = atoi(value);
        } else if (!strcmp(msg_tag, D2D_UL_GRANT_BANDWIDTH_TAG)) {
            _d2d_info.ul_bandwidth = atoi(value);
        } else if (!strcmp(msg_tag, D2D_UL_DATA_RATE_TAG)) {
            _d2d_info.ul_rate = atoi(value);
        } else if (!strcmp(msg_tag, D2D_SNR_TAG)) {
            _d2d_info.snr = atoi(value);
        } else {
           ALOGE("unknown d2d info message tag, ignore");
           return -1;
        }
    } else {
        ALOGE("strtok failed, ignore");
        return -1;
    }

    ALOGV("d2d_info: service_status = %d, rsrp = %d, ul_bandwidth = %d, ul_rate = %d, snr = %d",
           _d2d_info.service_status,
           _d2d_info.rsrp,
           _d2d_info.ul_bandwidth,
           _d2d_info.ul_rate,
           _d2d_info.snr);
    return msg_len;
}

bool D2dTracker::_process_d2d_info()
{
    int len;
    uint8_t packet[MAVLINK_MAX_PACKET_LEN];
    uint8_t rssi;
    uint8_t noise;

    if(_d2d_info.service_status == CONNECTED) {
        if(_bit_rate_adjust_mode == 0) {
            if (_d2d_info.ul_bandwidth != _last_tp_snr) {
                _last_tp_snr = _d2d_info.ul_bandwidth;
                _camera_service->set_bitrate(_last_tp_snr);
                ALOGV("received tp is %d", _last_tp_snr);
            }
        }else {
            if (_d2d_info.snr != _last_tp_snr) {
                _last_tp_snr = _d2d_info.snr;
                _camera_service->set_bitrate(_last_tp_snr);
                ALOGV("received snr is %d", _last_tp_snr);
            }
        }
    } else {
        _camera_service->set_bitrate(-99);   //-99 will indicate codec to enter the dummy state
        ALOGD("indicate codec to enter the dummy state");
    }

    if ((_d2d_info.service_status == CONNECTED) &&  (_last_service_status == DISCONNECTED)) {
        _camera_service->request_idr();
        ALOGD("reconnected, request idr, service_status is %d", _d2d_info.service_status);
    }
    _last_service_status = _d2d_info.service_status;

    // calculate rssi and noise
    if ((_d2d_info.rsrp > 0) || (_d2d_info.rsrp < -255)
        || (_d2d_info.rsrp - _d2d_info.snr > 0)
        || (_d2d_info.rsrp - _d2d_info.snr < -255)) {
            ALOGE("received rsrp or snr is out of range!!! %d %d",
                   _d2d_info.rsrp, _d2d_info.snr);
            return false;
    }
    rssi = abs(_d2d_info.rsrp);
    noise = abs(_d2d_info.rsrp - _d2d_info.snr);

    // send msg to rc service, 2 bytes: 1:rssi, 2:noise
    if(_rc_fd >= 0) {
        packet[0] = rssi;
        packet[1] = noise;
        _send_message(_rc_fd, packet, 2, NULL, 0);
    }

    // send msg to mavlink router
    len = _get_radio_packet(packet, rssi, noise);
    return _send_message(_router_fd, packet, len, NULL, 0);
}

ssize_t D2dTracker::_get_radio_packet(uint8_t *pBuf, uint8_t rssi, uint8_t noise)
{
    mavlink_message_t msg;
    mavlink_msg_radio_status_pack(Config::get_instance()->get_board_system_id(),
                                  Config::get_instance()->get_board_comp_id(),
                                  &msg,
                                  rssi,/*rssi*/ 0,/*remrssi*/ 0,/*txbuf*/
                                  noise,/*noise*/ 0,/*remnoise*/
                                  0,/*rxerrors*/ 0/*fixed*/);

    return mavlink_msg_to_send_buffer(pBuf, &msg);
}
