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

#ifndef UAV_CAMERA_COMMON_H
#define UAV_CAMERA_COMMON_H

enum UAV_CAMERA_MODE {
    PINE_ZSL_MODE,
    PINE_VIDEO_MODE,
};
enum UAV_CAM_STATE {
    CAM_STATE_IDLE = 0,
    CAM_STATE_OPEN,
    CAM_STATE_ZSL_PREVIEW,
    CAM_STATE_VIDEO_PREVIEW,
    CAM_STATE_VIDEO_RECORDING,
};
enum UAV_ERROR_CODE {
    CAM_SUCCESS = 0,
    CAM_ERROR = -1,
    CAM_NO_DEVICE = -2,
    CAM_BUSY = -99,
};

#ifndef __CAM_LOG__
#define __CAM_LOG__
#define CAM_LOG(fmt, arg...) do { \
        ALOGE("[%s:%d]" fmt, __FUNCTION__, __LINE__, ## arg); \
    } while(0)
#endif

#endif

