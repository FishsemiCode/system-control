/*
** Copyright (C) 2019 FishSemi Inc. All rights reserved.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**       http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef UAV_CAMERA_SERVICE_H
#define UAV_CAMERA_SERVICE_H

#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/Parcel.h>
#include <utils/Log.h>
#include <utils/Singleton.h>
#include <utils/String8.h>
#include <camera/CameraParameters.h>
#include "IUAVMainCamService.h"
#include "IUAVCameraCallBack.h"
#include "UAVCameraCommon.h"

using namespace android;

namespace android
{
    class UAVCamera: virtual public RefBase
    {
    public:
        UAVCamera();
        ~UAVCamera();
        void sayHello();
        int openCamera();
        String8 getParameters();
        int setParameters(const String8& params);
        int startPreview();
        int stopPreview();
        int startRecording();
        int stopRecording();
        int takePicture();
        int closeCamera();
        void pineGetCameraState(int32_t *cam_state);
        int pineSetBitRate(int32_t snr_tp_value);
        int pineRequestIdr();
        int pineSetFPVCameraID(int32_t cam_id);
        void pineGetFPVCameraID(int32_t *cam_id, int32_t *num_of_cam);
        int setNotifyCallback(const sp<IUAVCameraCallBack>& uavCamCB);
        int setDataCallback(const sp<IUAVCameraCallBack>& uavCamCB);
    private:
        static const sp<IUAVMainCamService> getService();
        class DeathNotifier: public IBinder::DeathRecipient
        {
        public:
            DeathNotifier() {}
            virtual ~DeathNotifier();
            virtual void binderDied(const wp<IBinder>& who);
        };

        static sp<DeathNotifier>                  sDeathNotifier;
        static Mutex                              sServiceLock;
        static sp<IUAVMainCamService>             sService;
        Mutex                                     mLock;

        static sp <IUAVCameraCallBack>            pNotifyCB;
        static sp <IUAVCameraCallBack>            pDataCB;
        static Mutex                              mListenLock;
    };
}
#endif

