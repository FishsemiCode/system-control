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

#include <assert.h>
#include <unistd.h>
#include <sys/un.h>
#include <cutils/properties.h>
#include "mavlink.h"
#include "config.h"
#include "camera_control.h"

#undef LOG_TAG
#define LOG_TAG "CameraControl"
#define MAX_RTSP_URI_LEN 100
#define CAMERA_CONTROL_SOCKET_NAME "cameracontrol"

static int g_image_index = -1;

CameraControl::CameraControl() : ModuleThread{"CameraControl"}
    , _src_sys_id(0)
    , _src_comp_id(0)
    , _preview_width(0)
    , _preview_height(0)
    , _is_camera_ready(false)
    , _timer_id(NULL)
{
    char prop_value[PROP_VALUE_MAX];
    timeval tv;
    gettimeofday(&tv, NULL);
    _uid = (uint8_t)tv.tv_usec;
    _uid = (_uid << 16) | getpid();
    _cam_service = CameraService::get_instance();
    if (Config::get_instance()->get_support_multiple_camera()) {
        _cam_service->get_camera_id(&_camera_id, &_camera_count);
    } else {
        _camera_id = 0;
        _camera_count = 1;
    }
    _uid = (_uid & 0x0FFFFFF) | (_camera_count << 24);
    ALOGD("uid is %u, cam count is %u", _uid, _camera_count);
    _system_id = Config::get_instance()->get_camera_system_id();
    _comp_id = Config::get_instance()->get_camera_comp_id();

    _router_fd = _get_domain_socket(CAMERA_CONTROL_SOCKET_NAME,
                                    TYPE_DOMAIN_SOCK_ABSTRACT);
    if (_router_fd < 0) {
        ALOGE("opening _router_fd socket failure");
    } else {
        _add_read_fd(_router_fd, TYPE_DATAGRAM_SOCK_FD);
    }
    _start_heartbeat();
}

void CameraControl::broadcast_heartbeat()
{
    mavlink_message_t msg;

    mavlink_msg_heartbeat_pack(_system_id, _comp_id, &msg, MAV_TYPE_GENERIC,
                               MAV_AUTOPILOT_INVALID, MAV_MODE_PREFLIGHT, _uid,
                               Config::get_instance()->get_support_camera_capture() ? MAV_STATE_ACTIVE : 0);

    _send_mavlink_msg(&msg);
}

bool CameraControl::camera_ready()
{
    int r;
    if (!_is_camera_ready) {
        r = _cam_service->open_camera();
        if (r == 0) {
            _is_camera_ready = true;
        } else {
            ALOGE("camera not ready, open return %d!", r);
        }
    }
    return _is_camera_ready;
}

void CameraControl::_heartbeat_timeout(union sigval v)
{
    CameraControl* cam_control = (CameraControl*)v.sival_ptr;
    if (cam_control->camera_ready()) {
        cam_control->broadcast_heartbeat();
    }
}

bool CameraControl::_start_heartbeat()
{
    struct sigevent evp;
    memset(&evp, 0, sizeof(struct sigevent));

    evp.sigev_value.sival_ptr = (void*)this;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = _heartbeat_timeout;

    if (timer_create(CLOCK_MONOTONIC, &evp, &_timer_id) == -1)
    {
        ALOGE("fail to timer_create");
        return false;
    }

    struct itimerspec it;
    it.it_interval.tv_sec = 1;
    it.it_interval.tv_nsec = 0;
    it.it_value.tv_sec = 1;
    it.it_value.tv_nsec = 0;

    if (timer_settime(_timer_id, 0, &it, NULL) == -1)
    {
        ALOGE("fail to timer_settime");
        return false;
    }
    return true;
}

bool CameraControl::_process_data(int fd, uint8_t* buf, int len,
                                  struct sockaddr* src_addr, int addrlen)
{
    mavlink_message_t msg;

    if(fd != _router_fd) {
        return false;
    }
    if (!_parse_mavlink_pack(buf, len, &msg)) {
        return false;
    }
    if (msg.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
        mavlink_command_long_t cmd;
        mavlink_msg_command_long_decode(&msg, &cmd);
        switch (cmd.command) {
        case MAV_CMD_REQUEST_CAMERA_INFORMATION:
            ALOGD("command received: REQUEST_CAMERA_INFORMATION");
            _handle_camera_info_request();
            break;
        case MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION:
            ALOGD("command received: REQUEST_VIDEO_STREAM_INFORMATION");
            _handle_camera_video_stream_request();
            break;
        case MAV_CMD_REQUEST_CAMERA_SETTINGS:
            ALOGD("command received: REQUEST_CAMERA_SETTINGS");
            _handle_camera_settings_request();
            break;
        case MAV_CMD_SET_CAMERA_MODE:
            ALOGD("command received: SET_CAMERA_MODE %d", (int)cmd.param2);
            _handle_set_camera_mode((int)cmd.param2);
            break;
        case MAV_CMD_REQUEST_STORAGE_INFORMATION:
            ALOGD("command received: REQUEST_STORAGE_INFORMATION");
            _handle_storage_info_request();
            break;
        case MAV_CMD_VIDEO_START_STREAMING:
            ALOGD("command received: START_STREAMING %d", (int)cmd.param1);
            _handle_video_start_streaming((int)cmd.param1);
            break;
        case MAV_CMD_VIDEO_STOP_STREAMING:
            ALOGD("command received: STOP_STREAMING");
            _handle_video_stop_streaming();
            break;
        case MAV_CMD_VIDEO_START_CAPTURE:
            ALOGD("command received: START_CAPTURE");
            _handle_video_start_recording();
            break;
        case MAV_CMD_VIDEO_STOP_CAPTURE:
            ALOGD("command received: STOP_CAPTURE");
            _handle_video_stop_recording();
            break;
        case MAV_CMD_IMAGE_START_CAPTURE:
            ALOGD("command received: IMAGE_START_CAPTURE");
            _handle_capture_photo_image();
            break;
        case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
            ALOGD("command received: REQUEST_CAMERA_CAPTURE_STATUS");
            _handle_request_capture_status();
            break;
        default:
            ALOGD("Command %d unhandled. Discarding.", cmd.command);
        }
    } else if (msg.msgid == MAVLINK_MSG_ID_SET_VIDEO_STREAM_SETTINGS) {
        ALOGD("command received: SET_VIDEO_STREAM_SETTINGS");
        _handle_camera_set_video_stream_settings(&msg);
    }
    return true;
}

void CameraControl::_send_ack(int cmd, bool success)
{
    mavlink_message_t msg;

    mavlink_msg_command_ack_pack(_system_id, _comp_id, &msg, cmd,
                                 success ? MAV_RESULT_ACCEPTED : MAV_RESULT_FAILED,
                                 0, 0, _src_sys_id, _src_comp_id);

    _send_mavlink_msg(&msg);
}

void CameraControl::_send_mavlink_msg(mavlink_message_t* pMsg)
{
    uint8_t data[MAVLINK_MAX_PACKET_LEN];
    int len = 0;
    len = mavlink_msg_to_send_buffer(data, pMsg);
    _send_message(_router_fd, data, len,
                  Config::get_instance()->get_camera_endpoint_name(),
                  TYPE_DOMAIN_SOCK_ABSTRACT);
}

void CameraControl::_handle_camera_info_request()
{
    mavlink_message_t msg;

    _send_ack(MAV_CMD_REQUEST_CAMERA_INFORMATION, true);
    ALOGD("ack sent: REQUEST_CAMERA_INFORMATION");

    uint32_t flags = CAMERA_CAP_FLAGS_CAPTURE_VIDEO
                    | CAMERA_CAP_FLAGS_CAPTURE_IMAGE
                    | CAMERA_CAP_FLAGS_HAS_MODES;
    // uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
    // uint32_t time_boot_ms, const uint8_t *vendor_name, const uint8_t *model_name,
    // uint32_t firmware_version, float focal_length, float sensor_size_h,
    // float sensor_size_v, uint16_t resolution_h, uint16_t resolution_v, uint8_t lens_id,
    // uint32_t flags, uint16_t cam_definition_version, const char *cam_definition_uri
    mavlink_msg_camera_information_pack(
                _system_id, _comp_id, &msg,
                0, (const uint8_t*)"camera", (const uint8_t*)"camera",
                0, 0, 0,
                0, 0, 0, 0,
                flags, 0, 0);

    _send_mavlink_msg(&msg);
    ALOGD("msg sent: CAMERA_INFORMATION");
}

void CameraControl::_handle_camera_video_stream_request()
{
    _send_ack(MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION, true);
    ALOGD("ack sent: VIDEO_STREAM_INFORMATION");

    mavlink_message_t msg;
    char rtsp_uri[MAX_RTSP_URI_LEN+1];
    char* local_ip = Config::get_instance()->get_video_stream_ip_address();

    _cam_service->get_camera_preview_size(&_preview_width, &_preview_height);

    char prop_value[PROP_VALUE_MAX];
    property_get("persist.sys.camera.hd", prop_value, "0");
    int isHd = atoi(prop_value);
    int state = -1;
    if ((isHd && _preview_width != 1920) || (!isHd && _preview_width != 1280)) {
        _cam_service->get_camera_state(&state);
        if (state < CAM_STATE_ZSL_PREVIEW) {
            ALOGD("camera preview size is not set yet, use property value %d", isHd);
        } else {
            ALOGE("camera preview size is not consistent with property value!");
        }
        if (isHd) {
            _preview_width = 1920;
            _preview_height = 1080;
        } else {
            _preview_width = 1280;
            _preview_height = 720;
        }
    }

    snprintf(rtsp_uri, MAX_RTSP_URI_LEN, "rtsp://%s:8554/H264Video", local_ip);

    mavlink_msg_video_stream_information_pack(
                _system_id, _comp_id, &msg, _camera_id, 0 /* Status */,
                0 /* FPS */, _preview_width, _preview_height, 0 /* bitrate */, 0 /* Rotation */,
                rtsp_uri);
    _send_mavlink_msg(&msg);
}

void CameraControl::_handle_camera_set_video_stream_settings(mavlink_message_t *msg)
{
    mavlink_set_video_stream_settings_t settings;
    int state = -1;
    bool previewStopped = false;

    mavlink_msg_set_video_stream_settings_decode(msg, &settings);

    _cam_service->get_camera_preview_size(&_preview_width, &_preview_height);
    ALOGD("try to set preview size to %d x %d, now is %d x %d",
              settings.resolution_h, settings.resolution_v,
             _preview_width, _preview_height);
    if (_preview_width != settings.resolution_h || _preview_height != settings.resolution_v) {
        _cam_service->get_camera_state(&state);
        if (state == CAM_STATE_ZSL_PREVIEW || state == CAM_STATE_VIDEO_PREVIEW) {
            ALOGD("need to stop preview at first");
            previewStopped = (_stop_preview_waiton_busy() == 0);
            if (!previewStopped) {
                ALOGE("failed to stop preview");
                return;
            }
        } else if (state != CAM_STATE_OPEN) {
            ALOGE("set preview size in wrong state %d", state);
            return;
        }

        ALOGD("set preview size really");
        if(_cam_service->set_camera_preview_size(settings.resolution_h, settings.resolution_v) != 0) {
            ALOGE("set preview size failed %d x %d", settings.resolution_h, settings.resolution_v);
        } else {
            _preview_width = settings.resolution_h;
            _preview_height = settings.resolution_v;
            // set propery for rtsp server to set correct preview size
            if (_preview_width == 1920 && _preview_height == 1080) {
                property_set("persist.sys.camera.hd", "1");
            } else {
                property_set("persist.sys.camera.hd", "0");
            }
            ALOGD("successfully set preview size to %d x %d", _preview_width, _preview_height);
        }
        // restore preview
        if (previewStopped) {
            if(_start_preview_waiton_busy() != 0 ) {
                ALOGE("restart preview failed");
            }
        }
    }
}

void CameraControl::_handle_video_start_streaming(int id)
{
    bool success = false;
    int state = -1;
    bool previewing = false;
    bool opened = false;

    if (id != _camera_id) {
        do {
            _cam_service->get_camera_state(&state);
            ALOGD("camera state is %d before set id", state);
            if (state == CAM_STATE_ZSL_PREVIEW || state == CAM_STATE_VIDEO_PREVIEW) {
                opened = true;
                previewing = true;
            } else if (state == CAM_STATE_OPEN) {
                opened = true;
                previewing = false;
            } else if (state == CAM_STATE_IDLE) {
                opened = false;
                previewing = false;
            } else if (state == CAM_STATE_VIDEO_RECORDING) {
                ALOGE("change id in video recording");
                break;
            }
            if (previewing && _stop_preview_waiton_busy() != 0) {
                ALOGE("failed to stop preview before change id");
                break;
            }
            ALOGD("preview is stopped before set id");
            if (opened && _cam_service->close_camera() != 0) {
                ALOGE("failed to close camera before change id");
                break;
            }
            ALOGD("camera is closed before set id");
            if (_cam_service->set_camera_id(id) != 0) {
                ALOGE("failed to set camera id to %d", id);
                break;
            }
            ALOGD("successfully set camera id to %d", id);
            _camera_id = id;
            success  = true;
            if (opened && _open_camera_waiton_busy() != 0) {
                ALOGE("failed to open camera after change id");
                break;
            }
            ALOGD("camera is opened post set id");
            if(_preview_width > 0 &&_cam_service->set_camera_preview_size(_preview_width, _preview_height) != 0) {
                ALOGE("restore preview size failed %d x %d", _preview_width, _preview_height);
            }
            if (previewing && _start_preview_waiton_busy() != 0) {
                ALOGE("failed to start preview after change id");
                break;
            }
            ALOGD("start stream done for camera id %d", id);
        } while (0);
    } else {
        ALOGD("camera id is already set to %d", id);
        success = true;
    }

    _send_ack(MAV_CMD_VIDEO_START_STREAMING, success);
    ALOGD("ack sent with result %d: START_STREAMING", success);
}

void CameraControl::_handle_video_stop_streaming()
{
    bool success = false;

    success = (_stop_preview_waiton_busy() == 0);
    _send_ack(MAV_CMD_VIDEO_STOP_STREAMING, success);
    ALOGD("ack sent with result %d: STOP_STREAMING", success);
}

void CameraControl::_handle_video_start_recording()
{
    bool success = false;

    success = (_start_video_recording_waiton_busy() == 0);
    _send_ack(MAV_CMD_VIDEO_START_CAPTURE, success);
    ALOGD("ack sent with result %d: VIDEO_START_CAPTURE", success);
}

void CameraControl::_handle_video_stop_recording()
{
    bool success = false;

    success = (_stop_video_recording_waiton_busy() == 0);
    _send_ack(MAV_CMD_VIDEO_STOP_CAPTURE, success);
    ALOGD("ack sent with result %d: VIDEO_STOP_CAPTURE", success);
}

void CameraControl::_handle_capture_photo_image()
{
    bool success = false;
    int result = 0;

    _send_ack(MAV_CMD_IMAGE_START_CAPTURE, true);
    ALOGD("ack sent : IMAGE_START_CAPTURE");
    success = (_cam_service->capture_photo_image() == 0);

    if (success) {
        result = 1;
        g_image_index++;
    }

    mavlink_message_t msg;
    mavlink_msg_camera_image_captured_pack(_system_id, _comp_id, &msg,
            0/*time_boot_ms*/, 0/*time_utc*/, 1 /*camera_id, 1 for first, 2 for second*/,
            0/*Latitude*/, 0/*Longitude*/, 0/*Altitude*/, 0/*Altitude above ground in meters*/,
            NULL/*Quaternion of camera orientation*/, g_image_index/*image count since armed*/,
            result/*capture_result, 1 success, 0 fail*/, NULL/*file_url URL of image taken*/);
    _send_mavlink_msg(&msg);
    ALOGD("sent msg: CAMERA_IMAGE_CAPTURED");
}

void CameraControl::_handle_request_capture_status()
{
    int state = -1;
    int ps, vs;
    _cam_service->get_camera_state(&state);
    if (state == CAM_STATE_ZSL_PREVIEW || state == CAM_STATE_VIDEO_PREVIEW) {
        ps = 0;
        vs = 0;
    } else if (state == CAM_STATE_VIDEO_RECORDING) {
        ps = 0;
        vs = 1;
    } else {
        ALOGE("request camera capture status in wrong state %d", state);
        _send_ack(MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS, false);
        ALOGD("ack sent with failure: REQUEST_CAMERA_CAPTURE_STATUS");
        return;
    }

    ALOGD("camera capture status ps=%d vs=%d", ps, vs);
    _send_ack(MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS, true);
    ALOGD("ack sent: REQUEST_CAMERA_CAPTURE_STATUS");

    mavlink_message_t msg;
    mavlink_msg_camera_capture_status_pack(_system_id, _comp_id, &msg,
              0/*time_boot_ms*/,
              ps/*image_status (0: idle, 1: capture in progress, 2: interval set but idle, 3: interval set and capture in progress)*/,
              vs/*video_status (0: idle, 1: capture in progress)*/,
              0/*image_interval*/, 0/*recording_time_ms*/, 0xffff/*available_capacity in MB*/);

    _send_mavlink_msg(&msg);
}

void CameraControl::_handle_camera_settings_request()
{
    int state = -1;
    int mode = -1;
    _cam_service->get_camera_state(&state);
    if (state == CAM_STATE_ZSL_PREVIEW) {
        mode = PINE_ZSL_MODE;
    } else if (state == CAM_STATE_VIDEO_PREVIEW) {
        mode = PINE_VIDEO_MODE;
    } else {
        ALOGE("request camera settings in wrong state %d", state);
        _send_ack(MAV_CMD_REQUEST_CAMERA_SETTINGS, false);
        ALOGD("ack sent with failure: REQUEST_CAMERA_SETTINGS");
        return;
    }

    _send_ack(MAV_CMD_REQUEST_CAMERA_SETTINGS, true);
    ALOGD("ack sent: REQUEST_CAMERA_SETTINGS");
    if(mode != -1) {
        mavlink_message_t msg;
        mavlink_msg_camera_settings_pack(
                   _system_id, _comp_id, &msg, 0/*Timestamp*/, mode/* Mode */);
        _send_mavlink_msg(&msg);
    }
}

void CameraControl::_handle_storage_info_request()
{
    _send_ack(MAV_CMD_REQUEST_STORAGE_INFORMATION, true);
    ALOGD("ack sent: REQUEST_STORAGE_INFORMATION");
}

void CameraControl::_handle_set_camera_mode(int mode)
{
    bool success = false;
    int state = -1;
    int oldmode = -1;
    bool previewing = false;
    _cam_service->get_camera_state(&state);
    if (state == CAM_STATE_ZSL_PREVIEW) {
        oldmode = PINE_ZSL_MODE;
        previewing = true;
    } else if (state == CAM_STATE_VIDEO_PREVIEW) {
        oldmode = PINE_VIDEO_MODE;
        previewing = true;
    } else if (state != CAM_STATE_OPEN) {
        _send_ack(MAV_CMD_SET_CAMERA_MODE, false);
        ALOGD("ack sent with failure: SET_CAMERA_MODE state wrong %d", state);
        return;
    }

    if (previewing) {
        ALOGD("need to stop preview at first");
        if (_stop_preview_waiton_busy() != 0) {
            _send_ack(MAV_CMD_SET_CAMERA_MODE, false);
            ALOGD("ack sent with failure: SET_CAMERA_MODE can not stop preview");
            return;
        }
    }
    success = (_cam_service->set_camera_mode(mode) == 0);
    ALOGD("set camera mode to %d success:%d", mode, success);
    if (previewing) {
        if(_start_preview_waiton_busy() != 0) {
            ALOGE("restart preview failed");
        }
    }
    _send_ack(MAV_CMD_SET_CAMERA_MODE, success);
    ALOGD("ack sent with result %d: SET_CAMERA_MODE", success);
}

int CameraControl::_open_camera_waiton_busy()
{
    int r = 0;
    int count = 0;
    while(count++ < 30) {
        r = _cam_service->open_camera();
        if (r == 0) {
            break;
        } else if (r == -99) {
            ALOGD("open camera busy");
            usleep(100 * 1000);
            continue;
        }
    }
    return r;
}

int CameraControl::_start_preview_waiton_busy()
{
    int r = 0;
    int count = 0;
    while(count++ < 30) {
        r = _cam_service->start_preview();
        if (r == 0) {
            break;
        } else if (r == -99) {
            ALOGD("start preview busy");
            usleep(100 * 1000);
            continue;
        }
    }
    return r;
}

int CameraControl::_stop_preview_waiton_busy()
{
    int r = 0;
    int count = 0;
    while(count++ < 30) {
        r = _cam_service->stop_preview();
        if (r == 0) {
            break;
        } else if (r == -99) {
            ALOGD("start preview busy");
            usleep(100 * 1000);
            continue;
        }
    }
    return r;
}

int CameraControl::_start_video_recording_waiton_busy()
{
    int r = 0;
    int count = 0;
    while(count++ < 30) {
        r = _cam_service->start_video_recording();
        if (r == 0) {
            break;
        } else if (r == -99) {
            ALOGD("start preview busy");
            usleep(100 * 1000);
            continue;
        }
    }
    return r;
}

int CameraControl::_stop_video_recording_waiton_busy()
{
    int r = 0;
    int count = 0;
    while(count++ < 30) {
        r = _cam_service->stop_video_recording();
        if (r == 0) {
            break;
        } else if (r == -99) {
            ALOGD("start preview busy");
            usleep(100 * 1000);
            continue;
        }
    }
    return r;
}
