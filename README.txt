
Building Native Bits
--------------------

This app relies on OpenSSL libcrypto, sqlcipher, and libsqlfs, which
are all "native" C code that needs to be built before working with the
Java.  Here's how:

  cd /path/to/guardianproject/iocipher
  make -C external
  ndk-build
