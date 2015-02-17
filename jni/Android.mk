#
# run "make -C external" before running "ndk-build"
#

LOCAL_PATH:= $(call my-dir)

sqlfs_DEFS := -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DFUSE_USE_VERSION=25 -DHAVE_LIBSQLCIPHER -DSQLITE_HAS_CODEC=1 -D_GNU_SOURCE=1

include $(CLEAR_VARS)
LOCAL_MODULE     := libsqlfs
LOCAL_SHARED_LIBRARIES := libsqlcipher_android
LOCAL_CFLAGS     := $(sqlfs_DEFS) -Wall -Werror -Wno-error=maybe-uninitialized
LOCAL_C_INCLUDES := external/libsqlfs jni
LOCAL_EXPORT_C_INCLUDES:= external/libsqlfs
LOCAL_SRC_FILES  := ../external/libsqlfs/sqlfs.c
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libiocipher
LOCAL_STATIC_LIBRARIES := libsqlfs
LOCAL_SHARED_LIBRARIES := libsqlcipher_android
LOCAL_CFLAGS += -DHAVE_LIBSQLCIPHER
# Google's gold linker has bugs, so use good ol' binutils
# https://code.google.com/p/android/issues/detail?id=109071
# http://osdir.com/ml/android-ndk/2013-02/msg00107.html
LOCAL_LDFLAGS   += -fuse-ld=bfd
LOCAL_LDLIBS    += -llog
LOCAL_SRC_FILES := \
	JniConstants.cpp \
	JNI_OnLoad.cpp \
	JNIHelp.cpp \
	readlink.cpp \
	realpath.cpp \
	toStringArray.cpp \
	info_guardianproject_iocipher_File.cpp \
	info_guardianproject_iocipher_VirtualFileSystem.cpp \
	info_guardianproject_libcore_io_Memory.cpp \
	info_guardianproject_libcore_io_OsConstants.cpp \
	info_guardianproject_libcore_io_Posix.cpp
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE     := sqlfscat
LOCAL_CFLAGS     := $(sqlfs_DEFS) -Wall -Werror
LOCAL_C_INCLUDES := external/libsqlfs
LOCAL_SHARED_LIBRARIES := libiocipher libsqlcipher_android
LOCAL_LDLIBS     := -llog
LOCAL_SRC_FILES  := ../external/libsqlfs/sqlfscat.c
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := libsqlcipher_android
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/libs/$(TARGET_ARCH_ABI)/libsqlcipher_android.so
LOCAL_EXPORT_C_INCLUDES := external external/openssl/include
LOCAL_EXPORT_LDLIBS := -lcrypto
LOCAL_EXPORT_LDFLAGS := \
	-L$(LOCAL_PATH)/../external/libs/$(TARGET_ARCH_ABI)
include $(PREBUILT_SHARED_LIBRARY)
