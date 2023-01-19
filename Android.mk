LOCAL_PATH := $(call my-dir)

# build RemoteCaptury
include $(CLEAR_VARS)
LOCAL_MODULE    := RemoteCaptury
LOCAL_SRC_FILES := ${CMAKE_CURRENT_SOURCE_DIR}/RemoteCaptury.cpp
LOCAL_C_INCLUDES := ${CMAKE_SOURCE_DIR}/lib/include
include $(BUILD_SHARED_LIBRARY)
