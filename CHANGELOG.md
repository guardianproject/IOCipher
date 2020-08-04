# IOCipher changelog

0.4.2

e24ed74 remove useSDK / minSDK from manifest!
ca59b92 remove Android Emulator tests for library and add pages building for javadocs
da53534 add uses and min SDK to manifest
da237da add parenthesis around "!" operator to ensure scope is clear
5e03487 ensure sqlcipher is in the includes path
77a59f5 add the SDK 29 license hash
d2b191e update gitlab ci script for enabling licenses
a20fa55 use absolute path for calling cmdline-tools
1a76360 disable ant for now (android update no longer generates build.xml)
b200ca8 remove ant setup in make release build
56dd7a4 update sdkmanager parent folder to cmdline-tools due to this bug https://stackoverflow.com/questions/60440509/android-command-line-tools-sdkmanager-always-shows-warning-could-not-create-se
0f6e13c more gitlab ci updates to support SDK 29
36f1d79 update gradle ci build tools to 29.0.3
27f2b26 update tooling to SDK 29 and gradle 4.0.1
fe2b742 remove incorrect variable from gitlab ci runner script
81e8045 set relative local path for uploadArchives task
8f62681 update gitlab to have aar.deployPath variable
5b247ed update NDK version in gitlab runner to 21d
a3fd96b update build.gradle to support local gradle archive building
0de4d14 (tag: 4.1.2, 0.4.1) update minimum platform to 21
185adb1 update manifest to SDK 21
2c7c943 (tag: 0.4.1, origin/0.4.1) remove old armeabi sqlcipher depend
0dc1a13 update the gradle pickFirst for SQLCipher 64-bit
2c8ea9e (dev_4.0_alt, 4.1) set long alignment mask to 0 for 64 bit platforms
e6ab677 add 64 bit archs for gradle build
1dc4bdc modify (int) cast to add (size_t) for 64-bit
c7b1e76 update minSDK to 16
f672804 update jni build files for 64-bit
f696678 update libs to iocipher 3.5.9 with 64-bit9


0.4

- fixed Android 7.x compatibility
- added method to finalize sqlfs on extra threads
- port test suite to Android Studio/gradle
- updated to android-database-sqlcipher 3.5.4
- delete every container file even if a prior ones failed to delete
- setup build and tests on gitlab-ci
- official releases use Java source/target 1.6 instead of 1.5

