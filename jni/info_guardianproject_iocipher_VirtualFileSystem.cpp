
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

static void VirtualFileSystem_mount(JNIEnv *env, jobject obj, jstring javaPassword) {
    char buf[256];
    setDatabaseFileName(env, obj);
    snprintf(buf, 255, "Could not mount filesystem in %s, bad password given?", dbFileName);

    sqlfs_t *sqlfs = 0;
    char const *password = env->GetStringUTFChars(javaPassword, 0);
    jsize passwordLen = env->GetStringUTFLength(javaPassword);

    /* Attempt to open the database with the password, then immediately close
     * it. If it fails, then the password is likely wrong. */
    if (!sqlfs_open_password(dbFileName, password, &sqlfs)) {
        LOGI("sqlfs_open_password FAILED");
        jniThrowException(env, "java/lang/IllegalArgumentException", buf);
    }
    sqlfs_close(sqlfs);

    /* init the vfs by caching the password in memory.  Then as threads are
     * spawned, they will use that cached password to open a connection to the
     * database.  sqlfs_init_password returns 0 for success in the unix fashion */
    if (sqlfs_init_password(dbFileName, password) != 0) {
        LOGI("sqlfs_init_password FAILED");
        jniThrowException(env, "java/lang/IllegalArgumentException", "Initializing VFS failed.");
    }
    env->ReleaseStringUTFChars(javaPassword, password);
}

static void VirtualFileSystem_mount_byte(JNIEnv *env, jobject obj, jbyteArray javaKey) {
    char buf[256];
    sqlfs_t *sqlfs = 0;
    jsize keyLen = env->GetArrayLength(javaKey);

    if (keyLen != REQUIRED_KEY_LENGTH) {
        snprintf(buf, 255, "Key length is not %i bytes (%i bytes)!",
                 REQUIRED_KEY_LENGTH, keyLen);
        jniThrowException(env, "java/lang/IllegalArgumentException", buf);
        return;
    }

    setDatabaseFileName(env, obj);
    jbyte *key = env->GetByteArrayElements(javaKey, NULL); //direct mem ref

    /* 
     * attempt to open the database with the key
     * if it fails, then the key is likely wrong
     */
    snprintf(buf, 255, "Could not mount filesystem in %s, bad key given?", dbFileName);
    // TODO create sqlfs_open_key() and sqlfs_open_password()
    if (!sqlfs_open_key(dbFileName, (uint8_t*)key, keyLen, &sqlfs)) {
        LOGI("sqlfs_open_key FAILED");
        jniThrowException(env, "java/lang/IllegalArgumentException", buf);
    }
    LOGE("sqlfs_close %p", sqlfs);
    sqlfs_close(sqlfs);

    /*
     * init the vfs by storing the key in memory
     * sqlfs_init_key returns 0 for success in the unix fashion
     */
    if (sqlfs_init_key(dbFileName, (uint8_t*)key, keyLen) != 0) {
        LOGI("sqlfs_init_key FAILED");
        jniThrowException(env, "java/lang/IllegalArgumentException", "Initializing VFS failed.");
    }
    env->ReleaseByteArrayElements(javaKey, key, 0);
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
    {"mount", "([B)V", (void *)VirtualFileSystem_mount_byte},
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
