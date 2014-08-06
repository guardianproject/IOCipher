/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#define LOG_TAG "File"

#include "JNIHelp.h"
#include "JniConstants.h"
//#include "JniException.h"
#include "ScopedPrimitiveArray.h"
#include "ScopedUtfChars.h"
#include "readlink.h"
#include "toStringArray.h"
#include "sqlfs.h"

#include <string>
#include <vector>
#include <algorithm>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

/* right now, we use a single global virtual file system so we don't
 * have to map the structs sqlfs_t and sqlite3 to Java code */
extern sqlfs_t *sqlfs;

// from fuse.h
typedef int(* 	fuse_fill_dir_t )(void *buf, const char *name, const struct stat *stbuf, off_t off);

static jstring File_readlink(JNIEnv* env, jclass, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return NULL;
    }

    std::string result;
    if (!readlink(path.c_str(), result)) {
        jniThrowIOException(env, errno);
        return NULL;
    }
    return env->NewStringUTF(result.c_str());
}

static jstring File_realpath(JNIEnv* env, jclass, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return NULL;
    }

    extern bool realpath(const char* path, std::string& resolved);
    std::string result;
    if (!realpath(path.c_str(), result)) {
        jniThrowIOException(env, errno);
        return NULL;
    }
    return env->NewStringUTF(result.c_str());
}

static jlong File_lastModifiedImpl(JNIEnv* env, jclass, jstring javaPath) {
    jboolean ret = 0;
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return JNI_FALSE;
    }

    key_attr attr;
    sqlfs_get_attr(0, "mtime", &attr);
    return static_cast<jlong>(attr.mtime) * 1000L;
}

static jboolean File_setLastModifiedImpl(JNIEnv* env, jclass, jstring javaPath, jlong ms) {
    jboolean ret = 0;
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return JNI_FALSE;
    }

    // We want to preserve the access time.
    key_attr atime;
    sqlfs_get_attr(0, "atime", &atime);

    // TODO: we could get microsecond resolution with utimes(3), "legacy" though it is.
    utimbuf times;
    key_attr mtime;
    mtime.mtime = static_cast<time_t>(ms / 1000);
    if(!sqlfs_set_attr(0, "mtime", &mtime))
        return 0;
    if(!sqlfs_set_attr(0, "atime", &atime))
        return 0;
    return 1;
}

typedef std::vector<std::string> DirEntries;

/* FUSE filler() function for use with the FUSE style readdir() that
 * libsqlfs provides.  Note: this function only ever expects statp to
 * be NULL and off to be 0.  buf is DirEntries */
static int fill_dir(void *buf, const char *name, const struct stat *statp, off_t off) {
    DirEntries *entries = (DirEntries*) buf;
    if(statp != NULL)
        LOGE("File.listImpl() fill_dir always expects statp to be NULL");
    if(off != 0)
        LOGE("File.listImpl() fill_dir always expects off to be 0");
    entries->push_back(name);
    // TODO implement returning an error (1) if something bad happened
    return 0;
}

static jboolean File_isDirectoryImpl(JNIEnv* env, jclass, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return JNI_FALSE;
    }
    return sqlfs_is_dir(0, path.c_str());
}

static jobjectArray File_listImpl(JNIEnv* env, jclass, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    DirEntries entries;
    // using FUSE readdir in old getdir() style which gives us the whole thing at once
    sqlfs_proc_readdir(0, path.c_str(), (void *)&entries, (fuse_fill_dir_t)fill_dir, 0, NULL);
    // filter "." and ".." from list of entries
    // Translate the intermediate form into a Java String[].
    entries.erase(std::remove(entries.begin(), entries.end(), std::string(".")), entries.end());
    entries.erase(std::remove(entries.begin(), entries.end(), std::string("..")), entries.end());
    return toStringArray(env, entries);
}

static JNINativeMethod sMethods[] = {
    {"isDirectoryImpl", "(Ljava/lang/String;)Z", (void *)File_isDirectoryImpl},
    {"listImpl", "(Ljava/lang/String;)[Ljava/lang/String;", (void *)File_listImpl},
    {"readlink", "(Ljava/lang/String;)Ljava/lang/String;", (void *)File_readlink},
    {"realpath", "(Ljava/lang/String;)Ljava/lang/String;", (void *)File_realpath},
    {"lastModifiedImpl", "(Ljava/lang/String;)J", (void *)File_lastModifiedImpl},
    {"setLastModifiedImpl", "(Ljava/lang/String;J)Z", (void *)File_setLastModifiedImpl},
};
int register_info_guardianproject_iocipher_File(JNIEnv* env) {
    jclass cls;

    cls = env->FindClass("info/guardianproject/iocipher/File");
    if (cls == NULL) {
        LOGE("Can't find info/guardianproject/iocipher/File\n");
        return -1;
    }
    return env->RegisterNatives(cls, sMethods, NELEM(sMethods));
}
