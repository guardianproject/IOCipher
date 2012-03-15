
all: openssl/obj/local/armeabi/libcrypto.so

# TODO include this Android.mk to integrate this into the build
openssl/obj/local/armeabi/libcrypto.so:
	cd openssl && ndk-build -j4
