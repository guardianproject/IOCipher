APP_ABI := armeabi armeabi-v7a x86
APP_PROJECT_PATH := $(shell pwd)
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/jni/Android.mk
APP_STL := stlport_shared
#
EXTERNAL_PATH := $(APP_PROJECT_PATH)/external
