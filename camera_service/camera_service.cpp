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
#include <unistd.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <hardware/camera.h>
#include "camera_callback.h"
#include "camera_service.h"

using namespace android;

#define SERVICE_NOT_READY (-1)
//#define SERVICE_NOT_READY (0) // set to (0) for test


CameraService* CameraService::_instance = NULL;
CameraService* CameraService::get_instance() {
    if (_instance == NULL) {
        _instance = new CameraService();
    }
    return _instance;
}

CameraService::CameraService()
{
    if (SERVICE_NOT_READY == 0)
        return;
    _camera = new UAVCamera();
    _camera_cb= new CameraCallBack();
    _camera->setNotifyCallback(_camera_cb);
}

int CameraService::open_camera() {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    return _camera->openCamera();
}

int CameraService::close_camera() {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    return _camera->closeCamera();
}

int CameraService::set_camera_preview_size(unsigned int width, unsigned int height) {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    String8 params;
    CameraParameters cam_param;

    params = _camera->getParameters();
    cam_param.unflatten(params);
    cam_param.setPreviewSize(width, height);
    params = cam_param.flatten();

    return _camera->setParameters(params);
}

int CameraService::get_camera_preview_size(int* width, int* height) {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    String8 params;
    CameraParameters cam_param;

    params = _camera->getParameters();
    cam_param.unflatten(params);
    cam_param.getPreviewSize(width, height);
    return 0;
}

int CameraService::start_preview() {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    return _camera->startPreview();
}

int CameraService::stop_preview() {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    return _camera->stopPreview();
}

int CameraService::start_video_recording() {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    return _camera->startRecording();
}

int CameraService::stop_video_recording() {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    return _camera->stopRecording();
}

int CameraService::capture_photo_image() {
    int rc;
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    rc = _camera->takePicture();
    if (rc != 0) {
        return rc;
    }
    rc = cond_wait_photo_capture();
    if (rc == 0) {
        return 0;
    } else {
        return -1;
    }
}

int CameraService::set_camera_mode(unsigned int mode) {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    String8 params;
    CameraParameters cam_param;

    params = _camera->getParameters();
    cam_param.unflatten(params);
    if (mode == 1)
        cam_param.set(CameraParameters::KEY_RECORDING_HINT, "true");
    else
        cam_param.set(CameraParameters::KEY_RECORDING_HINT, "false");
    params = cam_param.flatten();

    return _camera->setParameters(params);
}

int CameraService::set_bitrate(int snr) {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    return _camera->pineSetBitRate(snr);
}

int CameraService::request_idr() {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    return _camera->pineRequestIdr();
}

int CameraService::get_camera_state(int* pState) {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    _camera->pineGetCameraState(pState);
    return 0;
}

int CameraService::get_camera_id(int* pId, int* pCount) {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    _camera->pineGetFPVCameraID(pId, pCount);
    return 0;
}

int CameraService::set_camera_id(int id) {
    if (_camera == NULL) {
        return SERVICE_NOT_READY;
    }
    return _camera->pineSetFPVCameraID(id);
}

void CameraService::camera_notify_callback(int32_t msgType, int32_t ext1, int32_t ext2)
{
    (void) ext1;
    (void) ext2;

    switch(msgType) {
        case CAMERA_MSG_COMPRESSED_IMAGE:
            get_instance()->cond_signal_photo_capture();
            break;
        case CAMERA_MSG_SHUTTER:
            break;
        case CAMERA_MSG_RAW_IMAGE_NOTIFY:
            break;
        default:
            break;
    }
}

void CameraService::camera_data_callback(int64_t timestamp, int32_t width, int32_t height, unsigned char *buf)
{
    (void) timestamp;
    (void) width;
    (void) height;
    (void) buf;
}

void CameraService::cond_signal_photo_capture()
{
    pthread_mutex_lock(&_lock_photo_capture);
    pthread_cond_signal(&_cond_photo_capture);
    pthread_mutex_unlock(&_lock_photo_capture);
}

int CameraService::cond_wait_photo_capture()
{
    int rc;
    timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5;
    pthread_mutex_lock(&_lock_photo_capture);
    rc = pthread_cond_timedwait(&_cond_photo_capture, &_lock_photo_capture, &ts);
    pthread_mutex_unlock(&_lock_photo_capture);
    if (rc != 0) {
        ALOGE("take photo timedwait return %d", rc);
    }
    return rc;
}
