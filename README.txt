
Building Native Bits
--------------------

This app relies on OpenSSL libcrypto, sqlcipher, and libsqlfs, which
are all "native" C code that needs to be built before working with the
Java.  Here's how:

  cd /path/to/guardianproject/iocipher
  make -C external
  ndk-build

The shared library .so files are in libs/armeabi and the libsqlfs.a
static library is in external/libsqlfs/.libs/libsqlfs.a.
