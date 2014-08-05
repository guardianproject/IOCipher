
LOCAL_PATH := $(shell pwd)

PROJECT_ROOT = $(LOCAL_PATH)/..
EXTERNAL_ROOT = $(PROJECT_ROOT)/external

all: sqlcipher/sqlite3.c

# This target only generates the sqlite3.c source file, which is later built
# by jni/Android.mk.  Therefore, we don't need it to have any of the actual
# build flags.  We also rely on the build host system to provide the
# dependencies, like openssl, rather than point it to the included sources
# which are built for Android.
sqlcipher/sqlite3.c:
	cd sqlcipher && ./configure --enable-tempstore=yes \
		CFLAGS="-DSQL_HAS_CODEC"
	make -C sqlcipher sqlite3.c

sqlcipher-clean:
	-make -C sqlcipher clean
	-rm sqlcipher/sqlite3.c


clean: sqlcipher-clean

.PHONY: clean sqlcipher-clean
