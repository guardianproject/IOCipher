
#define LOG_TAG "VirtualFileSystem.cpp"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"

#include "sqlfs.h"

#include <string.h>
#include <stdio.h>

// yes, dbFileName is a duplicate of default_db_file in sqlfs.c
char dbFileName[PATH_MAX] = { 0 };

void setDatabaseFileName(JNIEnv *env, jobject obj) {
    jclass cls = env->GetObjectClass(obj);
    jfieldID fid = env->GetStaticFieldID(cls, "dbFileName", "Ljava/lang/String;");
    jstring javaDbFileName = (jstring) env->GetStaticObjectField(cls, fid);
    const char *name = env->GetStringUTFChars(javaDbFileName, 0);
    memset(dbFileName, 0, PATH_MAX);
    if (name == NULL)
        return;
    strncpy(dbFileName, name, PATH_MAX-2);
    dbFileName[PATH_MAX-1] = '\0';
    env->ReleaseStringUTFChars(javaDbFileName, name);
}

static void VirtualFileSystem_mount_unencrypted(JNIEnv *env, jobject obj) {
    char buf[256];
    setDatabaseFileName(env, obj);
    snprintf(buf, 255, "Could not mount filesystem in %s", dbFileName);
    if (sqlfs_init(dbFileName) != 0)
        jniThrowException(env, "java/lang/IllegalArgumentException", buf);
}

static void VirtualFileSystem_mount(JNIEnv *env, jobject obj, jstring javaKey) {
    char buf[256];
    setDatabaseFileName(env, obj);
    snprintf(buf, 255, "Could not mount filesystem in %s, bad key given?", dbFileName);

    sqlfs_t *sqlfs = 0;
    char const * key = env->GetStringUTFChars(javaKey, 0);
    jsize keyLen = env->GetStringUTFLength(javaKey);

    /* 
     * attempt to open the database with the key
     * if it fails, then the key is likely wrong
     */
    if(!sqlfs_open_key(dbFileName, key, &sqlfs)) {
        LOGI("sqlfs_open_key FAILED");
        jniThrowException(env, "java/lang/IllegalArgumentException", buf);
    }
    sqlfs_close(sqlfs);

    /*
     * init the vfs by storing the key in memory
     * sqlfs_init_key returns 0 for success in the unix fashion
     */
    if(sqlfs_init_key(dbFileName, key) != 0) {
        LOGI("sqlfs_init_key FAILED");
        jniThrowException(env, "java/lang/IllegalArgumentException", "Initializing VFS failed.");
    }
    env->ReleaseStringUTFChars(javaKey, key);
}

static void VirtualFileSystem_unmount(JNIEnv *env, jobject) {
    dbFileName[0] = '\0';
}

static jboolean VirtualFileSystem_isMounted(JNIEnv *env, jobject obj) {
    return strlen(dbFileName) > 0;
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
