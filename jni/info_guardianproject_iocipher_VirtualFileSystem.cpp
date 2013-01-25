
#define LOG_TAG "VirtualFileSystem.cpp"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"

#include "sqlfs.h"

#include <string.h>
#include <stdio.h>

// yes, databaseFileName is a duplicate of default_db_file in sqlfs.c
// TODO get dbFile from VirtualFileSystem.java instance
char databaseFileName[PATH_MAX] = { 0 };

static void VirtualFileSystem_init(JNIEnv *env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    const char *pathstr = path.c_str();
    if (databaseFileName != 0 || (pathstr != 0 && strcmp(pathstr, databaseFileName) != 0)) {
        strncpy(databaseFileName, pathstr, PATH_MAX);
    } else {
        LOGI("%s already inited, not running sqlfs_init()", pathstr);
    }
}

static void VirtualFileSystem_mount_unencrypted(JNIEnv *env, jobject) {
    char buf[256];
    snprintf(buf, 255, "Could not mount filesystem in %s", databaseFileName);
    if (sqlfs_init(databaseFileName) != 0)
        jniThrowException(env, "java/lang/IllegalArgumentException", buf);
}

static void VirtualFileSystem_mount_key(JNIEnv *env, jobject, jstring javaKey) {
    ScopedUtfChars key(env, javaKey);
    char buf[256];
    snprintf(buf, 255, "Could not mount filesystem in %s, bad key given?", databaseFileName);

    sqlfs_t *sqlfs = 0;
    if(!sqlfs_open_key(databaseFileName, key.c_str(), &sqlfs)) {
        jniThrowException(env, "java/lang/IllegalArgumentException", buf);
    }
    sqlfs_close(sqlfs);
    sqlfs_init_key(databaseFileName, key.c_str());
}

static void VirtualFileSystem_unmount(JNIEnv *env, jobject) {
    // NOP
    return;
}

static jboolean VirtualFileSystem_isMounted(JNIEnv *env, jobject) {
    return databaseFileName != 0;
}

static void VirtualFileSystem_beginTransaction(JNIEnv *env, jobject) {
    sqlfs_begin_transaction(0);
    return;
}

static void VirtualFileSystem_completeTransaction(JNIEnv *env, jobject) {
    sqlfs_complete_transaction(0,1);
    return;
}

static JNINativeMethod sMethods[] = {
    {"init", "(Ljava/lang/String;)V", (void *)VirtualFileSystem_init},
    {"mount_unencrypted", "()V", (void *)VirtualFileSystem_mount_unencrypted},
    {"mount", "(Ljava/lang/String;)V", (void *)VirtualFileSystem_mount_key},
    {"unmount", "()V", (void *)VirtualFileSystem_unmount},
    {"isMounted", "()Z", (void *)VirtualFileSystem_isMounted},
    {"beginTransaction", "()V", (void *)VirtualFileSystem_beginTransaction},
    {"completeTransaction", "()V", (void *)VirtualFileSystem_completeTransaction},
};
int register_info_guardianproject_iocipher_VirtualFileSystem(JNIEnv* env) {
    jclass cls = env->FindClass("info/guardianproject/iocipher/VirtualFileSystem");
    if (cls == NULL) {
        LOGE("Can't find info/guardianproject/iocipher/VirtualFileSystem\n");
        return -1;
    }
    return env->RegisterNatives(cls, sMethods, NELEM(sMethods));
}
