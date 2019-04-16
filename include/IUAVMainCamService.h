/*
** Copyright (C) 2019 FishSemi Inc. All rights reserved.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**   http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef IUAV_MAIN_CAM_SERVICE_H
#define IUAV_MAIN_CAM_SERVICE_H

#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/Parcel.h>
#include <utils/Log.h>
#include <utils/String8.h>
#include <camera/CameraParameters.h>
#include "IUAVCameraCallBack.h"

using namespace android;

namespace android
{
    class IUAVMainCamService : public IInterface
    {
    public:
        DECLARE_META_INTERFACE(UAVMainCamService);
        virtual void sayHello()=0;
        virtual int openCamera()=0;
        virtual String8 getParameters()=0;
        virtual int setParameters(const String8& params)=0;
        virtual int startPreview()=0;
        virtual int stopPreview()=0;
        virtual int startRecording()=0;
        virtual int stopRecording()=0;
        virtual int takePicture()=0;
        virtual int closeCamera()=0;
        virtual int setNotifyCallback(const sp<IUAVCameraCallBack>& uavCameraCallBack) = 0;
        virtual int setDataCallback(const sp<IUAVCameraCallBack>& uavCameraCallBack) = 0;
        virtual void pineGetCameraState(int32_t *cam_state)=0;
        virtual int pineSetBitRate(int32_t snr_tp_value)=0;
        virtual int pineRequestIdr()=0;
        virtual int pineSetFPVCameraID(int32_t cam_id)=0;
        virtual void pineGetFPVCameraID(int32_t *cam_id, int32_t *num_of_cam)=0;
        virtual void pineYoloDebugInfo(const void* yoloinfo, size_t len)=0;
    };

    enum
    {
        PINE_HELLO = 1,
        OPEN_CAMERA,
        GET_PARAMETERS,
        SET_PARAMETERS,
        START_PREVIEW,
        STOP_PREVIEW,
        START_RECORDING,
        STOP_RECORDING,
        TAKE_PICTURE,
        CLOSE_CAMERA,
        SET_NOTIFY_CALLBACK,
        SET_DATA_CALLBACK,
        PINE_XXX_XXX_REMAIN,
        PINE_GET_CAMERA_STATE,
        PINE_SET_BIT_RATE,
        PINE_REQUEST_IDR_FRAME,
        PINE_SET_FPV_CAMERA_ID,
        PINE_GET_FPV_CAMERA_ID,
        PINE_YOLO_DEBUG_INFO,
    };

    class BpUAVMainCamService: public BpInterface<IUAVMainCamService> {
    public:
        BpUAVMainCamService(const sp<IBinder>& impl);
        virtual void sayHello();
        virtual int openCamera();
        virtual String8 getParameters();
        virtual int setParameters(const String8& params);
        virtual int startPreview();
        virtual int stopPreview();
        virtual int startRecording();
        virtual int stopRecording();
        virtual int takePicture();
        virtual int closeCamera();
        virtual int setNotifyCallback(const sp<IUAVCameraCallBack>& uavCamCB);
        virtual int setDataCallback(const sp<IUAVCameraCallBack>& uavCamCB);
        virtual void pineGetCameraState(int32_t *cam_state);
        virtual int pineSetBitRate(int32_t snr_tp_value);
        virtual int pineRequestIdr();
        virtual int pineSetFPVCameraID(int32_t cam_id);
        virtual void pineGetFPVCameraID(int32_t *cam_id, int32_t *num_of_cam);
        virtual void pineYoloDebugInfo(const void* yoloinfo, size_t len);
    };

    class BnUAVMainCamService: public BnInterface<IUAVMainCamService> {
    public:
        virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
                uint32_t flags = 0);
    };
}

#endif
