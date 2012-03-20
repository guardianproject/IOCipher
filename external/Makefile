
LOCAL_PATH := $(shell pwd)

PROJECT_ROOT = $(LOCAL_PATH)/..
EXTERNAL_ROOT = $(PROJECT_ROOT)/external

# Android NDK setup
NDK_BASE ?=  /usr/local/android-ndk
NDK_PLATFORM_LEVEL ?= 5
NDK_SYSROOT=$(NDK_BASE)/platforms/android-$(NDK_PLATFORM_LEVEL)/arch-arm
NDK_UNAME := `uname -s | tr '[A-Z]' '[a-z]'`
NDK_TOOLCHAIN=$(NDK_BASE)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/$(NDK_UNAME)-x86

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
