APP_ABI := armeabi armeabi-v7a x86
APP_PROJECT_PATH := $(shell pwd)
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/jni/Android.mk
APP_STL := stlport_shared
# the NDK platform level, aka APP_PLATFORM, is equivalent to minSdkVersion
APP_PLATFORM := android-$(shell sed -n 's,.*android:minSdkVersion="\([0-9][0-9]*\)".*,\1,p' AndroidManifest.xml)
#
EXTERNAL_PATH := $(APP_PROJECT_PATH)/external
