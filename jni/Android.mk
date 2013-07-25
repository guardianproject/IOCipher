#
# run "make -C external" before running "ndk-build"
#

LOCAL_PATH:= $(call my-dir)

sqlfs_DEFS := -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DFUSE_USE_VERSION=25 -DHAVE_LIBSQLCIPHER -DSQLITE_HAS_CODEC=1 -D_GNU_SOURCE=1

include $(CLEAR_VARS)

LOCAL_MODULE     := libsqlfs
LOCAL_CFLAGS     := $(sqlfs_DEFS) -Wall -Werror -Wno-error=maybe-uninitialized
LOCAL_C_INCLUDES := external/libsqlfs external jni
LOCAL_LDLIBS     := -llog
LOCAL_SRC_FILES  := ../external/libsqlfs/sqlfs.c

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

# NOTE the following flags,
#   SQLITE_TEMP_STORE=3 causes all TEMP files to go into RAM. and thats the behavior we want
#   SQLITE_ENABLE_FTS3   enables usage of FTS3 - NOT FTS1 or 2.
#   SQLITE_DEFAULT_AUTOVACUUM=1  causes the databases to be subject to auto-vacuum
android_sqlite_cflags :=  -DHAVE_USLEEP=1 -DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576 -DSQLITE_THREADSAFE=1 -DNDEBUG=1 -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 -DSQLITE_DEFAULT_AUTOVACUUM=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_FTS3_BACKWARDS -DSQLITE_ENABLE_LOAD_EXTENSION
sqlcipher_cflags := -DSQLITE_HAS_CODEC -DHAVE_FDATASYNC=0 -Dfdatasync=fsync
libsqlfs_cflags := -DHAVE_LIBSQLCIPHER

LOCAL_MODULE    := libiocipher
LOCAL_STATIC_LIBRARIES := libsqlfs
LOCAL_CFLAGS += $(android_sqlite_cflags) $(sqlcipher_cflags) $(libsqlfs_cflags)
LOCAL_C_INCLUDES:= \
	external/libsqlfs \
	external/openssl/include \
	external
LOCAL_LDFLAGS   += \
	-L$(LOCAL_PATH)/../external/openssl/obj/local/armeabi/
LOCAL_LDLIBS    += -lcrypto -llog
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
	info_guardianproject_libcore_io_Posix.cpp \
	../external/sqlcipher/sqlite3.c

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE     := sqlfscat
LOCAL_CFLAGS     := $(sqlfs_DEFS) -Wall -Werror
LOCAL_C_INCLUDES := external/libsqlfs external
LOCAL_SHARED_LIBRARIES := libiocipher
LOCAL_LDFLAGS   += \
	-L$(LOCAL_PATH)/../external/openssl/obj/local/armeabi/
LOCAL_LDLIBS     := -lcrypto -llog
LOCAL_SRC_FILES  := ../external/libsqlfs/sqlfscat.c

include $(BUILD_EXECUTABLE)


TAGS:
	etags *.h *.cpp \
		$(shell pwd)/../external/libsqlfs/*.[ch] \
		$(shell pwd)/../external/openssl/include/openssl/*.h \
		$(shell pwd)/../external/sqlcipher/*.[ch] \
		$(NDK_BASE)/platforms/android-8/arch-arm/usr/include/*.h \
		$(NDK_BASE)/platforms/android-8/arch-arm/usr/include/*/*.h

