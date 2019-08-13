LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

CAMERA_EXIST := $(shell test -d $(LOCAL_PATH)/../camera && echo yes)
LAMP_SIGNAL_EXIST := $(shell test -d $(LOCAL_PATH)/../lampsignal && echo yes)

LOCAL_SRC_FILES:= \
        main.cpp \
        config.cpp \
        thread_base.cpp \
        module_thread.cpp \
        board_control.cpp \
        d2d_tracker.cpp \
        wifi_control.cpp \

ifeq ($(CAMERA_EXIST), yes)
LOCAL_SRC_FILES += \
        camera_control.cpp \
        camera_service/camera_service.cpp \
        camera_service/camera_callback.cpp \

endif

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog \
        libutils \
        libbinder \

ifeq ($(CAMERA_EXIST), yes)
LOCAL_SHARED_LIBRARIES += \
        libuavmaincamera_client \
        libcamera_client
endif

ifeq ($(LAMP_SIGNAL_EXIST), yes)

LOCAL_SHARED_LIBRARIES += \
        liblampsignal

LOCAL_CFLAGS += -DLAMP_SIGNAL_EXIST

endif

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/camera_service \
        $(LOCAL_PATH)/../common/include/mavlink \
        $(LOCAL_PATH)/../common/include/mavlink/ardupilotmega \

ifeq ($(CAMERA_EXIST), yes)
LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/../common/include/camera \

LOCAL_CFLAGS += -DCAMERA_EXIST
endif

LOCAL_MODULE:= system-control

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
