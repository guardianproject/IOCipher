
LOCAL_PATH := $(shell pwd)

all: openssl/libs/armeabi/libcrypto.so sqlcipher/sqlite3.c

# TODO include this Android.mk to integrate this into the build
openssl/libs/armeabi/libcrypto.so:
	cd openssl && ndk-build -j4

sqlcipher/sqlite3.c:
	cd sqlcipher && ./configure --enable-tempstore=yes \
		CFLAGS="-DSQL_HAS_CODEC" LDFLAGS="-lcrypto"
	make -C sqlcipher sqlite3.c

clean:
	-make -C sqlcipher clean
	-rm sqlcipher/sqlite3.c
	-cd openssl && ndk-build clean

.PHONY: clean
