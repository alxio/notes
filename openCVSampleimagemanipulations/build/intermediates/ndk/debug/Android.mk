LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := myNativeLib
LOCAL_LDFLAGS := -Wl,--build-id
LOCAL_SRC_FILES := \
	/home/a.kedziersk2/image-manipulations/openCVSampleimagemanipulations/src/main/jni/main.cpp \
	/home/a.kedziersk2/image-manipulations/openCVSampleimagemanipulations/src/main/jni/empty.cpp \

LOCAL_C_INCLUDES += /home/a.kedziersk2/image-manipulations/openCVSampleimagemanipulations/src/main/jni
LOCAL_C_INCLUDES += /home/a.kedziersk2/image-manipulations/openCVSampleimagemanipulations/src/debug/jni

include $(BUILD_SHARED_LIBRARY)
