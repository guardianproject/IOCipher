
Building Native Bits
--------------------

This app relies on OpenSSL libcrypto, sqlcipher, and libsqlfs, which
are all "native" C code that needs to be built before working with the
Java. First, make sure you have the prerequisites:

  apt-get install tcl libtool automake autoconf gawk

Now build everything:

  cd /path/to/guardianproject/iocipher
  git submodule init
  git submodule update
  make -C external
  ndk-build

The shared library .so files are in libs/armeabi and the libsqlfs.a
static library is in external/libsqlfs/.libs/libsqlfs.a.

