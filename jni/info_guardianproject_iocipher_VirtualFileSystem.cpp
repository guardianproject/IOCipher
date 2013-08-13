
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

static void VirtualFileSystem_mount(JNIEnv *env, jobject, jstring javaKey) {
    char buf[256];
    snprintf(buf, 255, "Could not mount filesystem in %s, bad key given?", databaseFileName);

    sqlfs_t *sqlfs = 0;
    char const * key = env->GetStringUTFChars(javaKey, 0);
    jsize keyLen = env->GetStringUTFLength(javaKey);

    /* 
     * attempt to open the database with the key
     * if it fails, then the key is likely wrong
     */
    if(!sqlfs_open_key(databaseFileName, key, &sqlfs)) {
        LOGI("sqlfs_open_key FAILED");
        jniThrowException(env, "java/lang/IllegalArgumentException", buf);
    }
    sqlfs_close(sqlfs);

    /*
     * init the vfs by storing the key in memory
     * sqlfs_init_key returns 0 for success in the unix fashion
     */
    if(sqlfs_init_key(databaseFileName, key) != 0) {
        LOGI("sqlfs_init_key FAILED");
        jniThrowException(env, "java/lang/IllegalArgumentException", "Initializing VFS failed.");
    }
    env->ReleaseStringUTFChars(javaKey, key);
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
    {"mount", "(Ljava/lang/String;)V", (void *)VirtualFileSystem_mount},
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
