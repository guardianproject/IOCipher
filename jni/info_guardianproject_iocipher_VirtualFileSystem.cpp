
#define LOG_TAG "VirtualFileSystem"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"

#include "sqlfs.h"

#include <string.h>

char databaseFileName[PATH_MAX] = { NULL };
sqlfs_t *sqlfs = NULL;

static jint VirtualFileSystem_init(JNIEnv *env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    const char *pathstr = path.c_str();
    // TODO check if dir exists and throw exception if not
    if (databaseFileName != NULL || (pathstr != NULL && strcmp(pathstr, databaseFileName) != 0)) {
        strncpy(databaseFileName, pathstr, PATH_MAX);
        strncat(databaseFileName, "/sqlcipherfs.db", PATH_MAX - strlen(pathstr));
        sqlfs_init(databaseFileName);
    } else {
        LOGI("%s already exists, not running sqlfs_init()", pathstr);
    }
}

static jint VirtualFileSystem_mount(JNIEnv *env, jobject) {
    if(sqlfs != 0) {
        LOGI("sqlfs_open: already open");
        return 1;
    }
    sqlfs_open(databaseFileName, &sqlfs);
}

static jint VirtualFileSystem_mount_key(JNIEnv *env, jobject, jstring key) {
    // TODO implement open_key
}

static jint VirtualFileSystem_close(JNIEnv *env, jobject) {
    sqlfs_close(sqlfs);
    sqlfs = 0;
}

static jboolean VirtualFileSystem_isOpen(JNIEnv *env, jobject) {
    if(sqlfs == 0)
        return 0;
    else
        return 1;
}

static JNINativeMethod sMethods[] = {
    {"init", "(Ljava/lang/String;)I", (void *)VirtualFileSystem_init},
    {"mount", "()I", (void *)VirtualFileSystem_mount},
    {"mount", "(Ljava/lang/String;)I", (void *)VirtualFileSystem_mount_key},
    {"unmount", "()I", (void *)VirtualFileSystem_close},
    {"isOpen", "()Z", (void *)VirtualFileSystem_isOpen},
};
int register_info_guardianproject_iocipher_VirtualFileSystem(JNIEnv* env) {
    jclass cls = env->FindClass("info/guardianproject/iocipher/VirtualFileSystem");
    if (cls == NULL) {
        LOGE("Can't find info/guardianproject/iocipher/VirtualFileSystem\n");
        return -1;
    }
    return env->RegisterNatives(cls, sMethods, NELEM(sMethods));
}
