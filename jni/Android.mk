#
# run "make -C external" before running "ndk-build"
#

LOCAL_PATH:= $(call my-dir)

sqlfs_DEFS := -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DFUSE_USE_VERSION=25 -DHAVE_LIBSQLCIPHER -DSQLITE_HAS_CODEC=1 -D_GNU_SOURCE=1

include $(CLEAR_VARS)
LOCAL_MODULE     := libsqlfs
LOCAL_SHARED_LIBRARIES := libsqlcipher
LOCAL_CFLAGS     := $(sqlfs_DEFS) -Wall -Werror
LOCAL_C_INCLUDES := external/libsqlfs external/sqlcipher jni
LOCAL_EXPORT_C_INCLUDES:= external/libsqlfs external/sqlcipher
LOCAL_SRC_FILES  := ../external/libsqlfs/sqlfs.c
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsqlcipher
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/libs/$(TARGET_ARCH_ABI)/libsqlcipher.so
LOCAL_EXPORT_C_INCLUDES := external
LOCAL_EXPORT_LDFLAGS := \
	-L$(LOCAL_PATH)/../external/libs/$(TARGET_ARCH_ABI)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libiocipher
LOCAL_STATIC_LIBRARIES := libsqlfs
LOCAL_SHARED_LIBRARIES := libsqlcipher
LOCAL_CFLAGS += -DHAVE_LIBSQLCIPHER
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
LOCAL_C_INCLUDES := external/libsqlfs external/sqlcipher
LOCAL_SHARED_LIBRARIES := libiocipher libsqlcipher
LOCAL_LDLIBS     := -llog
LOCAL_SRC_FILES  := ../external/libsqlfs/sqlfscat.c
include $(BUILD_EXECUTABLE)


