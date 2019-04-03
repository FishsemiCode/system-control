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

#include "UAVCamera.h"
#include "camera_callback.h"

using namespace android;

class CameraService {
public:
    static CameraService* get_instance();
    int open_camera();
    int close_camera();
    int set_camera_preview_size(unsigned int width, unsigned int height);
    int get_camera_preview_size(int* width, int* height);
    int start_preview();
    int stop_preview();
    int start_video_recording();
    int stop_video_recording();
    int capture_photo_image();
    int set_camera_mode(unsigned int mode);
    int set_bitrate(int snr);
    int request_idr();
    int get_camera_state(int* state);
    int get_camera_id(int* pId, int* pCount);
    int set_camera_id(int id);
    static void camera_notify_callback(int32_t msgType, int32_t ext1, int32_t ext2);
    static void camera_data_callback(int64_t timestamp, int32_t width, int32_t height,
                                     unsigned char *buf);
    void cond_signal_photo_capture();
    int cond_wait_photo_capture();

private:
    CameraService();

    static CameraService* _instance;
    sp<UAVCamera> _camera;
    sp<CameraCallBack> _camera_cb;
    pthread_mutex_t _lock_photo_capture;
    pthread_cond_t _cond_photo_capture;
};
