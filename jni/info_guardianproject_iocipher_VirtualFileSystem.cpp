
#define LOG_TAG "VirtualFileSystem"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"

#include "sqlfs.h"

#include <string.h>

char databaseFileName[PATH_MAX] = { NULL };
sqlfs_t *sqlfs = NULL;

static jint info_guardianproject_iocipher_VirtualFileSystem_init(JNIEnv *env, jobject, jstring javaPath) {
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

static jint info_guardianproject_iocipher_VirtualFileSystem_open(JNIEnv *env, jobject) {
    if(sqlfs != 0) {
        LOGI("sqlfs_open: already open");
        return 1;
    }
    sqlfs_open(databaseFileName, &sqlfs);
}

static jint info_guardianproject_iocipher_VirtualFileSystem_open_key(JNIEnv *env, jobject, jstring key) {
    // TODO implement open_key
}

static jint info_guardianproject_iocipher_VirtualFileSystem_close(JNIEnv *env, jobject) {
    sqlfs_close(sqlfs);
    sqlfs = 0;
}

static jboolean info_guardianproject_iocipher_VirtualFileSystem_isOpen(JNIEnv *env, jobject) {
    if(sqlfs == 0)
        return 0;
    else
        return 1;
}

static JNINativeMethod sMethods[] = {
    {"init", "(Ljava/lang/String;)I", (void *)info_guardianproject_iocipher_VirtualFileSystem_init},
    {"open", "()I", (void *)info_guardianproject_iocipher_VirtualFileSystem_open},
    {"open", "(Ljava/lang/String;)I", (void *)info_guardianproject_iocipher_VirtualFileSystem_open_key},
    {"close", "()I", (void *)info_guardianproject_iocipher_VirtualFileSystem_close},
    {"isOpen", "()Z", (void *)info_guardianproject_iocipher_VirtualFileSystem_isOpen},
};
int register_info_guardianproject_iocipher_VirtualFileSystem(JNIEnv* env) {
    jclass cls;

    cls = env->FindClass("info/guardianproject/iocipher/VirtualFileSystem");
    if (cls == NULL) {
        LOGE("Can't find info/guardianproject/iocipher/VirtualFileSystem\n");
        return -1;
    }
    return env->RegisterNatives(cls, sMethods, NELEM(sMethods));
}
