
IOCipher: Encrypted Virtual Disk
--------------------------------

IOCipher is a virtual encrypted disk for apps without requiring the device to
be rooted. It uses a clone of the standard java.io API for working with
files. Just password handling & opening the virtual disk are what stand
between developers and fully encrypted file storage. It is based on libsqlfs
and SQLCipher.

If you are using this in your app, we'd love to hear about it! Please send us
an email at root@guardianproject.info


Adding to your project
----------------------

If you are using gradle, then add this to your project:

    compile 'info.guardianproject.iocipher:IOCipherStandalone:0.3',

Apps that are also using [SQLCipher-for-Android] should use the version that
only includes IOCipher itself.  The standlone version includes
*libstlport_shared.so* and *libsqlcipher_android.so*, and they will conflict
with SQLCipher-for-Android.  Then include this in your gradle:

    compile 'info.guardianproject.iocipher:IOCipher:0.3'



Getting Supporting Libraries
----------------------------

IOCipher is built upon SQLCipher, which is required in order to use this
library.  You can get SQLCipher-for-Android here:

http://sqlcipher.net/open-source

For example, SQLCipher for Android v3.1.0 binary is available here:
https://s3.amazonaws.com/sqlcipher/SQLCipher+for+Android+v3.1.0.zip

And the signature is here:
https://s3.amazonaws.com/sqlcipher/SQLCipher+for+Android+v3.1.0.zip.sig

The releases should be signed by this key:

```
$ gpg --recv-keys D1FA3A2A97ED25C2
$ gpg --fingerprint support@zetetic.net
pub   4096R/97ED25C2 2014-04-22 [expires: 2017-04-21]
      Key fingerprint = D83F 5F9E B811 D6E6 B4A0  D9C5 D1FA 3A2A 97ED 25C2
uid                  Zetetic LLC <support@zetetic.net>
sub   3072R/67FD0322 2014-04-22 [expires: 2015-04-22]
sub   3072R/D4DFEDA7 2014-04-22 [expires: 2015-04-22]
sub   3072R/B1C49DF6 2014-04-22 [expires: 2015-04-22]
```


Building
--------

This app relies on OpenSSL libcrypto, sqlcipher, and libsqlfs, which
are all "native" C code that needs to be built before working with the
Java. First, make sure you have the build prerequisites:

  apt-get install tcl libtool automake autoconf gawk libssl-dev

Point `ant` to where your Android NDK is installed, either by setting
`ndk.dir` in your local.properties or setting the environment variable:

  export ANDROID_NDK=/opt/android-ndk

Now build everything:

  git clone https://github.com/guardianproject/IOCipher
  git submodule update --init --recursive
  android update lib-project --path .
  ant clean debug

The iocipher.jar is in bin/.  The shared library .so files are in libs/armeabi
and the libsqlfs.a static library is in external/libsqlfs/.libs/libsqlfs.a.


Building Native Bits for Eclipse
--------------------------------

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
