# IOCipher changelog

0.5

- ported to 64-bit architectures (arm64-v8a and x86_64)
- updated to latest SQLCipher (v4.2.0)
- set min SDK to 14, since SQLCipher v4.2.0 requires that
- fix bug in VirtualFileSystem directory check
- publish javadoc to https://guardianproject.github.io/IOCipher/
- build with NDK r17c

0.4

- fixed Android 7.x compatibility
- added method to finalize sqlfs on extra threads
- port test suite to Android Studio/gradle
- updated to android-database-sqlcipher 3.5.4
- delete every container file even if a prior ones failed to delete
- setup build and tests on gitlab-ci
- official releases use Java source/target 1.6 instead of 1.5

