
LOCAL_PATH := $(shell pwd)

PROJECT_ROOT = $(LOCAL_PATH)/..
EXTERNAL_ROOT = $(PROJECT_ROOT)/external

# Android now has 64-bit and 32-bit versions of the NDK for GNU/Linux.  We
# assume that the build platform uses the appropriate version, otherwise the
# user building this will have to manually set NDK_PROCESSOR or NDK_TOOLCHAIN.
CPU := $(shell uname -m)
ifeq ($(CPU),x86_64)
 NDK_PROCESSOR=x86_64
else
 NDK_PROCESSOR=x86
endif

# Android NDK setup
NDK_BASE ?= /opt/android-ndk
NDK_PLATFORM_LEVEL ?= 5
NDK_ABI=arm
NDK_COMPILER_VERSION=4.6
NDK_SYSROOT=$(NDK_BASE)/platforms/android-$(NDK_PLATFORM_LEVEL)/arch-$(NDK_ABI)
NDK_UNAME:=`uname -s | tr '[A-Z]' '[a-z]'`
NDK_TOOLCHAIN=$(NDK_BASE)/toolchains/$(NDK_ABI)-linux-androideabi-$(NDK_COMPILER_VERSION)/prebuilt/$(NDK_UNAME)-$(NDK_PROCESSOR)

# to use the real HOST tag, you need the latest libtool files:
# http://stackoverflow.com/questions/4594736/configure-does-not-recognize-androideabi
#HOST := arm-none-linux-gnueabi
HOST := arm-linux-androideabi

all: openssl/libs/armeabi/libcrypto.so sqlcipher/sqlite3.c

# TODO include this Android.mk to integrate this into the build
openssl/libs/armeabi/libcrypto.so:
	cd openssl && ndk-build -j4

openssl-clean:
	-cd openssl && ndk-build clean


# this generates the sqlite3.c source file, which is built by jni/Android.mk
sqlcipher/sqlite3.c:
	cd sqlcipher && ./configure --enable-tempstore=yes \
		CFLAGS="-DSQL_HAS_CODEC"
	make -C sqlcipher sqlite3.c

sqlcipher-clean:
	-make -C sqlcipher clean
	-rm sqlcipher/sqlite3.c


clean: openssl-clean sqlcipher-clean

.PHONY: clean openssl-clean sqlcipher-clean
