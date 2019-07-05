
IOCipher: Encrypted Virtual Disk
--------------------------------

IOCipher is a virtual encrypted disk for apps without requiring the device to
be rooted. It uses a clone of the standard java.io API for working with
files. Just password handling & opening the virtual disk are what stand
between developers and fully encrypted file storage. It is based on libsqlfs
and SQLCipher.

If you are using this in your app, we'd love to hear about it! Please send us
an email at support@guardianproject.info


Adding to your project
----------------------

See https://guardianproject.info/code/iocipher


Building
--------

This app relies on OpenSSL libcrypto, sqlcipher, and libsqlfs, which
are all "native" C code that needs to be built before working with the
Java. First, make sure you have the build prerequisites:

  apt-get install tcl libtool automake autoconf gawk libssl-dev

Point the build to where your Android SDK and NDK are installed,
either by setting `sdk.dir` and `ndk.dir` in your _local.properties_ or
setting the environment variables:

  export ANDROID_HOME=/opt/android-sdk
  export ANDROID_NDK_HOME=/opt/android-ndk

With _gradle_, just run `gradle build` and it will run all the steps.

Using _ant_, build everything like this:

  git clone https://github.com/guardianproject/IOCipher
  git submodule update --init --recursive
  ./setup-ant
  make -C external/
  $ANDROID_NDK_HOME/ndk-build
  ant clean debug

The official releases are built using _ant_ using a script that fully resets
the git repo, then runs the whole build:

  ./make-release-build


Building Native Bits
--------------------

If you are using Eclipse with this library, you can have the NDK parts built
as part of the Eclipse build process.  You just need to set ANDROID_NDK in the
"String Substitution" section of your Eclipse's workspace preferences.

Otherwise, you can build the native bits from the Terminal using:

  make -C external
  ndk-build


License
-------

When taken as a whole, this project is under the the LGPLv3 license
since it is the only license that is compatible with the licenses of
all the components.  The source code for this comes from a few
different places, so there are a number of licenses for different
chunks.

* Apache 2.0 (Android Internals): Much of the code here is taken from
  the Android internals, so it has an Apache 2.0 license.

* OpenSSL License: It is linked to the OpenSSL that is provided with
  Android, so it should be covered under Android's handling of the
  advertisement clause.

* LGPL 2.1 (libsqlfs)

* BSD-style (sqlcipher)

We believe the LGPLv3 is compatible with all reasonable uses, including
proprietary software, but let us know if it provides difficulties for you.
For more info on how that works with Java, see:

https://www.gnu.org/licenses/lgpl-java.en.html


Included shared library files
-----------------------------

In `external/libs` are some binary .so files, these are all binaries pulled
from other sources so that the C code can have something link against.

`libcrypto.so` comes from Android emulators.  They are included here
so that the C code can link against openssl's libcrypto, which Android
includes but does not expose in the NDK.  If you want to build this library
from source, then do this:

```
git clone https://github.com/guardianproject/openssl-android
cd openssl-android
ndk-build -j4
```

These shared libraries _must_ not be included in any real app. Android
provides `/system/lib/libcrypto.so` and you should get SQLCipher directly from
the source, listed above.




[SQLCipher-for-Android]: https://www.zetetic.net/sqlcipher/open-source
