LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        main.cpp \
        config.cpp \
        thread_base.cpp \
        module_thread.cpp \
        board_control.cpp \
        d2d_tracker.cpp \
        camera_control.cpp \
        camera_service/camera_service.cpp \
        camera_service/camera_callback.cpp \

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog \
        libutils \
        libbinder \
        libuavmaincamera_client \
        libcamera_client

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/camera_service \
        $(LOCAL_PATH)/../common/include/mavlink \
        $(LOCAL_PATH)/../common/include/mavlink/ardupilotmega \
        $(LOCAL_PATH)/../common/include/camera \

LOCAL_MODULE:= system-control

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
