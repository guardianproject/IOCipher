APP_ABI := armeabi-v7a arm64-v8a x86 x86_64
APP_PROJECT_PATH := $(shell pwd)
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/jni/Android.mk
APP_STL := stlport_static
#
EXTERNAL_PATH := $(APP_PROJECT_PATH)/external
