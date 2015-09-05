LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := myNativeLib
LOCAL_LDFLAGS := -Wl,--build-id
LOCAL_SRC_FILES := \
	C:\NUTY\image-manipulations\openCVSampleimagemanipulations\src\main\jni\empty.cpp \
	C:\NUTY\image-manipulations\openCVSampleimagemanipulations\src\main\jni\main.cpp \

LOCAL_C_INCLUDES += C:\NUTY\image-manipulations\openCVSampleimagemanipulations\src\main\jni
LOCAL_C_INCLUDES += C:\NUTY\image-manipulations\openCVSampleimagemanipulations\src\debug\jni

include $(BUILD_SHARED_LIBRARY)
