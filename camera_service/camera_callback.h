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
    class CameraCallBack : public BnUAVCameraCallBack
    {
    public:
        CameraCallBack() { }
        virtual ~CameraCallBack() { }

        status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
                uint32_t flags = 0);
        virtual void notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2);
        virtual void dataCallback(int64_t timestamp, int32_t width, int32_t height, unsigned char *buf);
    };

}
