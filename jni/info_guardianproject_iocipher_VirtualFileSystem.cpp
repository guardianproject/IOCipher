
#define LOG_TAG "VirtualFileSystem.cpp"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"

#include "sqlfs.h"

#include <string.h>
#include <stdio.h>

// yes, databaseFileName is a duplicate of default_db_file in sqlfs.c
// TODO get dbFile from VirtualFileSystem.java instance
char databaseFileName[PATH_MAX] = { NULL };
sqlfs_t *sqlfs = NULL;

static void VirtualFileSystem_init(JNIEnv *env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    const char *pathstr = path.c_str();
    if (databaseFileName != NULL || (pathstr != NULL && strcmp(pathstr, databaseFileName) != 0)) {
        strncpy(databaseFileName, pathstr, PATH_MAX);
        sqlfs_init(databaseFileName);
    } else {
        LOGI("%s already inited, not running sqlfs_init()", pathstr);
    }
}

static void VirtualFileSystem_mount(JNIEnv *env, jobject) {
    if(sqlfs != 0) {
        LOGI("Cannot mount %s, already open", databaseFileName);
        return;
    }
    char buf[256];
    snprintf(buf, 255, "Could not mount filesystem in %s", databaseFileName);
    if (!sqlfs_open(databaseFileName, &sqlfs))
        jniThrowException(env, "java/io/IOException", buf);
}

static void VirtualFileSystem_mount_key(JNIEnv *env, jobject, jstring javaKey) {
    if(sqlfs != 0) {
        LOGI("Cannot mount %s, already open", databaseFileName);
        return;
    }
    ScopedUtfChars key(env, javaKey);
    char buf[256];
    snprintf(buf, 255, "Could not mount filesystem in %s", databaseFileName);
    if (!sqlfs_open_key(databaseFileName, key.c_str(), key.size(), &sqlfs))
        jniThrowException(env, "java/io/IOException", buf);
}

static void VirtualFileSystem_unmount(JNIEnv *env, jobject) {
    sqlfs_close(sqlfs);
    sqlfs = 0;
}

static jboolean VirtualFileSystem_isMounted(JNIEnv *env, jobject) {
    if(sqlfs == 0)
        return 0;
    else
        return 1;
}

static JNINativeMethod sMethods[] = {
    {"init", "(Ljava/lang/String;)V", (void *)VirtualFileSystem_init},
    {"mount", "()V", (void *)VirtualFileSystem_mount},
    {"mount", "(Ljava/lang/String;)V", (void *)VirtualFileSystem_mount_key},
    {"unmount", "()V", (void *)VirtualFileSystem_unmount},
    {"isMounted", "()Z", (void *)VirtualFileSystem_isMounted},
};
int register_info_guardianproject_iocipher_VirtualFileSystem(JNIEnv* env) {
    jclass cls = env->FindClass("info/guardianproject/iocipher/VirtualFileSystem");
    if (cls == NULL) {
        LOGE("Can't find info/guardianproject/iocipher/VirtualFileSystem\n");
        return -1;
    }
    return env->RegisterNatives(cls, sMethods, NELEM(sMethods));
}
