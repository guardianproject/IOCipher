
#define LOG_TAG "VirtualFileSystem.cpp"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"

#include "sqlfs.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

// yes, dbFileName is a duplicate of default_db_file in sqlfs.c
char dbFileName[PATH_MAX] = { 0 };
// store first sqlfs instance as marker for mounted state
static sqlfs_t *sqlfs = NULL;
// memory blob for error messages
static char msg[256];
#define MAX_MSG_LEN 255

static jboolean VirtualFileSystem_isMounted(JNIEnv *env, jobject obj);

bool throwContainerReadWriteError(JNIEnv *env) {
    bool error = false;
    if (access(dbFileName, R_OK) != 0) {
        error = true;
        snprintf(msg, MAX_MSG_LEN,
                 "Could not mount %s does not exist or is not readable (%d)!",
                 dbFileName, errno);
    } else if (access(dbFileName, W_OK) != 0) {
        error = true;
        snprintf(msg, MAX_MSG_LEN,
                 "Could not mount %s is not writable (%d)!",
                 dbFileName, errno);
    }
    if (error)
        jniThrowException(env, "java/lang/IllegalArgumentException", msg);
    return error;
}

void handleCreateError(JNIEnv *env) {
    if (!throwContainerReadWriteError(env)) {
        snprintf(msg, MAX_MSG_LEN, "Unknown error creating %s", dbFileName);
        jniThrowException(env, "java/lang/IllegalStateException", msg);
    }
}

void handleMountError(JNIEnv *env) {
    if (!throwContainerReadWriteError(env)) {
        snprintf(msg, MAX_MSG_LEN,
                 "Could not mount filesystem in %s, bad password given?", dbFileName);
        jniThrowException(env, "java/lang/IllegalArgumentException", msg);
    }
}

bool throwKeyLengthException(JNIEnv *env, jsize keyLen) {
    if (keyLen != REQUIRED_KEY_LENGTH) {
        snprintf(msg, MAX_MSG_LEN, "Key length is not %i bytes (%i bytes)!",
                 REQUIRED_KEY_LENGTH, keyLen);
        jniThrowException(env, "java/lang/IllegalArgumentException", msg);
        return true;
    } else {
        return false;
    }
}

bool throwMountedException(JNIEnv *env, jobject obj) {
    if (VirtualFileSystem_isMounted(env, obj)) {
        snprintf(msg, MAX_MSG_LEN, "Filesystem in '%s' already mounted!", dbFileName);
        jniThrowException(env, "java/lang/IllegalStateException", msg);
        return true;
    } else {
        return false;
    }
}

static jstring VirtualFileSystem_getContainerPath(JNIEnv *env, jobject obj) {
    return env->NewStringUTF(dbFileName);
}

static void VirtualFileSystem_setContainerPath(JNIEnv *env, jobject obj, jstring javaFileName) {
    char const *name = env->GetStringUTFChars(javaFileName, 0);
    jsize nameLen = env->GetStringUTFLength(javaFileName);
    memset(dbFileName, 0, PATH_MAX);
    if (name == NULL || nameLen < 1) {
        jniThrowException(env, "java/lang/IllegalArgumentException",
                          "blank file name not allowed!");
        env->ReleaseStringUTFChars(javaFileName, name);
        return;
    }

    int validFileName = 1;
    struct stat sb;
    const char *dir = dirname(name);

    if (access(dir, R_OK) != 0) {
        validFileName = 0;
        snprintf(msg, MAX_MSG_LEN,
                 "Base directory %s, does not exist or is not readable (%d)!",
                 dir, errno);
    } else if (access(dir, W_OK) != 0) {
        validFileName = 0;
        snprintf(msg, MAX_MSG_LEN, "Could not write to base directory %s (%d)!",
                 dir, errno);
    } else if (stat(dir, &sb) == -1) {
        validFileName = 0;
        snprintf(msg, MAX_MSG_LEN, "Cannot stat %s (%d)!", dir, errno);
    } else if (!sb.st_mode & S_IFDIR) {
        validFileName = 0;
        snprintf(msg, MAX_MSG_LEN, "Base path %s is not a directory!", dir);
    }

    if (validFileName) {
        strncpy(dbFileName, name, PATH_MAX-2);
        dbFileName[PATH_MAX-1] = '\0';
    } else {
        jniThrowException(env, "java/lang/IllegalArgumentException", msg);
    }
    env->ReleaseStringUTFChars(javaFileName, name);
}

static jboolean VirtualFileSystem_isMounted(JNIEnv *env, jobject obj) {
    return sqlfs != NULL || sqlfs_instance_count() > 0;
}

static void VirtualFileSystem_createNewContainer(JNIEnv *env, jobject obj, jstring javaPassword) {
    if (throwMountedException(env, obj))
        return;

    char const *password = env->GetStringUTFChars(javaPassword, 0);
    jsize passwordLen = env->GetStringUTFLength(javaPassword);

    /* Attempt to open the database with the password, then immediately close
     * it. If it fails, then the password is likely wrong. */
    if (sqlfs_open_password(dbFileName, password, &sqlfs)) {
        sqlfs_close(sqlfs);
        sqlfs = NULL;
    } else {
        handleCreateError(env);
    }
    env->ReleaseStringUTFChars(javaPassword, password);
}

static void VirtualFileSystem_createNewContainer_byte(JNIEnv *env, jobject obj, jbyteArray javaKey) {
    if (throwMountedException(env, obj))
        return;

    jsize keyLen = env->GetArrayLength(javaKey);
    if (throwKeyLengthException(env, keyLen))
        return;

    jbyte *key = env->GetByteArrayElements(javaKey, NULL); //direct mem ref

    /* attempt to open the database with the key if it fails, most likely the
     * db file does not exist or the key is wrong */
    if (sqlfs_open_key(dbFileName, (uint8_t*)key, keyLen, &sqlfs)) {
        sqlfs_close(sqlfs);
        sqlfs = NULL;
    } else {
        handleMountError(env);
    }

    env->ReleaseByteArrayElements(javaKey, key, 0);
}

static void VirtualFileSystem_mount(JNIEnv *env, jobject obj, jstring javaPassword) {
    if (throwMountedException(env, obj))
        return;

    if (throwContainerReadWriteError(env))
        return;

    char const *password = env->GetStringUTFChars(javaPassword, 0);
    jsize passwordLen = env->GetStringUTFLength(javaPassword);

    /* Attempt to open the database with the password, then immediately close
     * it. If it fails, then the password is likely wrong. */
    if (!sqlfs_open_password(dbFileName, password, &sqlfs)) {
        handleMountError(env);
    }
    env->ReleaseStringUTFChars(javaPassword, password);
}

static void VirtualFileSystem_mount_byte(JNIEnv *env, jobject obj, jbyteArray javaKey) {
    if (throwMountedException(env, obj))
        return;

    if (throwContainerReadWriteError(env))
        return;

    jsize keyLen = env->GetArrayLength(javaKey);
    if (throwKeyLengthException(env, keyLen))
        return;

    jbyte *key = env->GetByteArrayElements(javaKey, NULL); //direct mem ref

    /* attempt to open the database with the key if it fails, most likely the
     * db file does not exist or the key is wrong */
    if (!sqlfs_open_key(dbFileName, (uint8_t*)key, keyLen, &sqlfs)) {
        handleMountError(env);
    }

    env->ReleaseByteArrayElements(javaKey, key, 0);
}

static void VirtualFileSystem_unmount(JNIEnv *env, jobject obj) {
    if (!VirtualFileSystem_isMounted(env, obj)) {
        snprintf(msg, MAX_MSG_LEN, "Filesystem in '%s' not mounted!", dbFileName);
        jniThrowException(env, "java/lang/IllegalStateException", msg);
        return;
    }
    /* VFS holds a sqlfs instance open purely as a marker of the filesystem
     * being mounted. If there is more than one sqlfs instance, that means
     * that threads are still active. Closing the final sqlfs instances causes
     * libsqlfs to close the database and zero out the key/password. */
    if (sqlfs_instance_count() > 1) {
        snprintf(msg, 255,
                 "Cannot unmount when threads are still active! (%i threads)",
                 sqlfs_instance_count() - 1);
        jniThrowException(env, "java/lang/IllegalStateException", msg);
        return;
    }
    sqlfs_close(sqlfs);
    sqlfs = NULL;
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
    {"getContainerPath", "()Ljava/lang/String;", (void *)VirtualFileSystem_getContainerPath},
    {"setContainerPath", "(Ljava/lang/String;)V", (void *)VirtualFileSystem_setContainerPath},
    {"createNewContainer", "(Ljava/lang/String;)V", (void *)VirtualFileSystem_createNewContainer},
    {"createNewContainer", "([B)V", (void *)VirtualFileSystem_createNewContainer_byte},
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
