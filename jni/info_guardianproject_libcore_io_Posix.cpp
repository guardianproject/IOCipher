/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Posix"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "JniException.h"
#include "ScopedBytes.h"
#include "ScopedLocalRef.h"
#include "ScopedPrimitiveArray.h"
#include "ScopedUtfChars.h"
#include "StaticAssert.h"
#include "toStringArray.h"
#include "sqlfs.h"

// our replacements for missing things

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/vfs.h> // Bionic doesn't have <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>

/* right now, we use a single global virtual file system so we don't
 * have to map the structs sqlfs_t and sqlite3 to Java code */
extern char dbFileName[PATH_MAX];

#define TO_JAVA_STRING(NAME, EXP) \
        jstring NAME = env->NewStringUTF(EXP); \
        if (NAME == NULL) return NULL;

static void throwException(JNIEnv* env, jclass exceptionClass, jmethodID ctor3, jmethodID ctor2,
                           const char* functionName, int error) {
    jthrowable cause = NULL;
    if (env->ExceptionCheck()) {
        cause = env->ExceptionOccurred();
        env->ExceptionClear();
    }

    ScopedLocalRef<jstring> detailMessage(env, env->NewStringUTF(functionName));
    if (detailMessage.get() == NULL) {
        // Not really much we can do here. We're probably dead in the water,
        // but let's try to stumble on...
        env->ExceptionClear();
    }

    jobject exception;
    if (cause != NULL) {
        exception = env->NewObject(exceptionClass, ctor3, detailMessage.get(), error, cause);
    } else {
        exception = env->NewObject(exceptionClass, ctor2, detailMessage.get(), error);
    }
    env->Throw(reinterpret_cast<jthrowable>(exception));
}

static void throwErrnoException(JNIEnv* env, const char* functionName, int error) {
    static jmethodID ctor3 = env->GetMethodID(JniConstants::errnoExceptionClass,
                             "<init>", "(Ljava/lang/String;ILjava/lang/Throwable;)V");
    static jmethodID ctor2 = env->GetMethodID(JniConstants::errnoExceptionClass,
                             "<init>", "(Ljava/lang/String;I)V");
    /* FUSE/sqlfs returns negative errno values, but throwException wants the positive ones */
    throwException(env, JniConstants::errnoExceptionClass, ctor3, ctor2, functionName, -error);
}

// sqlfs/FUSE returns errno-style errors as negative values
template <typename rc_t>
static rc_t throwIfNegative(JNIEnv* env, const char* name, rc_t rc) {
    if (rc < rc_t(0)) {
        throwErrnoException(env, name, rc);
    }
    return rc;
}

template <typename ScopedT>
class IoVec {
public:
    IoVec(JNIEnv* env, size_t bufferCount) : mEnv(env), mBufferCount(bufferCount) {
    }

    bool init(jobjectArray javaBuffers, jintArray javaOffsets, jintArray javaByteCounts) {
        // We can't delete our local references until after the I/O, so make sure we have room.
        if (mEnv->PushLocalFrame(mBufferCount + 16) < 0) {
            return false;
        }
        ScopedIntArrayRO offsets(mEnv, javaOffsets);
        if (offsets.get() == NULL) {
            return false;
        }
        ScopedIntArrayRO byteCounts(mEnv, javaByteCounts);
        if (byteCounts.get() == NULL) {
            return false;
        }
        // TODO: Linux actually has a 1024 buffer limit. glibc works around this, and we should too.
        // TODO: you can query the limit at runtime with sysconf(_SC_IOV_MAX).
        for (size_t i = 0; i < mBufferCount; ++i) {
            jobject buffer = mEnv->GetObjectArrayElement(javaBuffers, i); // We keep this local ref.
            mScopedBuffers.push_back(new ScopedT(mEnv, buffer));
            jbyte* ptr = const_cast<jbyte*>(mScopedBuffers.back()->get());
            if (ptr == NULL) {
                return false;
            }
            struct iovec iov;
            iov.iov_base = reinterpret_cast<void*>(ptr + offsets[i]);
            iov.iov_len = byteCounts[i];
            mIoVec.push_back(iov);
        }
        return true;
    }

    ~IoVec() {
        for (size_t i = 0; i < mScopedBuffers.size(); ++i) {
            delete mScopedBuffers[i];
        }
        mEnv->PopLocalFrame(NULL);
    }

    iovec* get() {
        return &mIoVec[0];
    }

    size_t size() {
        return mBufferCount;
    }

private:
    JNIEnv* mEnv;
    size_t mBufferCount;
    std::vector<iovec> mIoVec;
    std::vector<ScopedT*> mScopedBuffers;
};

static jobject makeStructPasswd(JNIEnv* env, const struct passwd& pw) {
    TO_JAVA_STRING(pw_name, pw.pw_name);
    TO_JAVA_STRING(pw_dir, pw.pw_dir);
    TO_JAVA_STRING(pw_shell, pw.pw_shell);
    static jmethodID ctor = env->GetMethodID(JniConstants::structPasswdClass, "<init>",
                            "(Ljava/lang/String;IILjava/lang/String;Ljava/lang/String;)V");
    return env->NewObject(JniConstants::structPasswdClass, ctor,
                          pw_name, static_cast<jint>(pw.pw_uid), static_cast<jint>(pw.pw_gid), pw_dir, pw_shell);
}

static jobject makeStructStat(JNIEnv* env, const struct stat& sb) {
    static jmethodID ctor = env->GetMethodID(JniConstants::structStatClass, "<init>",
                            "(JJIJIIJJJJJJJ)V");
    return env->NewObject(JniConstants::structStatClass, ctor,
                          static_cast<jlong>(sb.st_dev), static_cast<jlong>(sb.st_ino),
                          static_cast<jint>(sb.st_mode), static_cast<jlong>(sb.st_nlink),
                          static_cast<jint>(sb.st_uid), static_cast<jint>(sb.st_gid),
                          static_cast<jlong>(sb.st_rdev), static_cast<jlong>(sb.st_size),
                          static_cast<jlong>(sb.st_atime), static_cast<jlong>(sb.st_mtime),
                          static_cast<jlong>(sb.st_ctime), static_cast<jlong>(sb.st_blksize),
                          static_cast<jlong>(sb.st_blocks));
}

static jobject makeStructStatFs(JNIEnv* env, const struct statfs& sb) {
    STATIC_ASSERT(sizeof(sb.f_bavail) == sizeof(jlong), statfs_not_64_bit);
    STATIC_ASSERT(sizeof(sb.f_bfree) == sizeof(jlong), statfs_not_64_bit);
    STATIC_ASSERT(sizeof(sb.f_blocks) == sizeof(jlong), statfs_not_64_bit);

    static jmethodID ctor = env->GetMethodID(JniConstants::structStatFsClass, "<init>",
                            "(JJJJJJJJ)V");
    return env->NewObject(JniConstants::structStatFsClass, ctor, static_cast<jlong>(sb.f_bsize),
                          static_cast<jlong>(sb.f_blocks), static_cast<jlong>(sb.f_bfree),
                          static_cast<jlong>(sb.f_bavail), static_cast<jlong>(sb.f_files),
                          static_cast<jlong>(sb.f_ffree), static_cast<jlong>(sb.f_namelen),
                          static_cast<jlong>(sb.f_frsize));
}

static jobject makeStructTimeval(JNIEnv* env, const struct timeval& tv) {
    static jmethodID ctor = env->GetMethodID(JniConstants::structTimevalClass, "<init>", "(JJ)V");
    return env->NewObject(JniConstants::structTimevalClass, ctor,
                          static_cast<jlong>(tv.tv_sec), static_cast<jlong>(tv.tv_usec));
}

static jobject makeStructUtsname(JNIEnv* env, const struct utsname& buf) {
    TO_JAVA_STRING(sysname, buf.sysname);
    TO_JAVA_STRING(nodename, buf.nodename);
    TO_JAVA_STRING(release, buf.release);
    TO_JAVA_STRING(version, buf.version);
    TO_JAVA_STRING(machine, buf.machine);
    static jmethodID ctor = env->GetMethodID(JniConstants::structUtsnameClass, "<init>",
                            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    return env->NewObject(JniConstants::structUtsnameClass, ctor,
                          sysname, nodename, release, version, machine);
};

static jobject doStat(JNIEnv* env, jstring javaPath, bool isLstat) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return NULL;
    }
    struct stat sb;
    // TODO implement lstat() once symlink support is added
    if (isLstat)
        jniThrowRuntimeException(env, "lstat() is not implemented");
    int rc = TEMP_FAILURE_RETRY(sqlfs_proc_getattr(0, path.c_str(), &sb));
    if (rc < 0) {
        throwErrnoException(env, isLstat ? "lstat" : "stat", rc);
        return NULL;
    }
    return makeStructStat(env, sb);
}

static jboolean Posix_access(JNIEnv* env, jobject, jstring javaPath, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return JNI_FALSE;
    }
    int rc = TEMP_FAILURE_RETRY(sqlfs_proc_access(0, path.c_str(), mode));
    if (rc == -1) {
        throwErrnoException(env, "access", rc);
    }
    return (rc == 0);
}

static void Posix_chmod(JNIEnv* env, jobject, jstring javaPath, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfNegative(env, "chmod", TEMP_FAILURE_RETRY(sqlfs_proc_chmod(0, path.c_str(), mode)));
}

static void Posix_close(JNIEnv* env, jobject, jobject javaFd) {
    // Get the FileDescriptor's 'fd' field and clear it.
    // sqlfs doesn't have a close() since files don't really need to be open()ed
    jstring path = jniGetPathFromFileDescriptor(env, javaFd);
    jniSetFileDescriptorInvalid(env, javaFd);
}

static jobject Posix_fstat(JNIEnv* env, jobject, jobject javaFd) {
    jstring javaPath = jniGetPathFromFileDescriptor(env, javaFd);
    return doStat(env, javaPath, false);
}

// TODO if sqlfs_proc_fsync changes to need isfdatasync and *fi, then fix here
static void Posix_fsync(JNIEnv* env, jobject, jobject javaFd) {
    jstring javaPath = jniGetPathFromFileDescriptor(env, javaFd);
    ScopedUtfChars path(env, javaPath);
    throwIfNegative(env, "fsync", TEMP_FAILURE_RETRY(sqlfs_proc_fsync(0, path.c_str(), 0, NULL)));
}

/* in sqlfs, truncate() and ftruncate() do the same thing since there
 * isn't a difference between and open and a closed file */
static void Posix_ftruncate(JNIEnv* env, jobject, jobject javaFd, jlong length) {
    jstring javaPath = jniGetPathFromFileDescriptor(env, javaFd);
    ScopedUtfChars path(env, javaPath);
    throwIfNegative(env, "ftruncate", TEMP_FAILURE_RETRY(sqlfs_proc_truncate(0, path.c_str(), length)));
}

static void Posix_link(JNIEnv* env, jobject, jstring javaFrom, jstring javaTo) {
    ScopedUtfChars from(env, javaFrom);
    ScopedUtfChars to(env, javaTo);
    if (from.c_str() == NULL || from.c_str() == NULL) {
        return;
    }
    throwIfNegative(env, "link", TEMP_FAILURE_RETRY(sqlfs_proc_link(0, from.c_str(), from.c_str())));
}

static void Posix_mkdir(JNIEnv* env, jobject, jstring javaPath, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    // TODO throw exception warning that VirtualFileSystem is not open
    throwIfNegative(env, "mkdir", TEMP_FAILURE_RETRY(sqlfs_proc_mkdir(0, path.c_str(), mode)));
}

static jobject Posix_open(JNIEnv* env, jobject, jstring javaPath, jint flags, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return NULL;
    }
    LOGI("path: %s %x %o", path.c_str(), flags, mode);
    struct fuse_file_info ffi;
    ffi.flags = flags;
    ffi.direct_io = 0; // don't use direct_io so this open() call will create a file


    int do_create = 0;
    // libsqfs' open() doesn't create.
    if( (flags & O_CREAT) && (flags & O_EXCL) ) {
        // we must attempt a create
        do_create = 1;
    } else if ( (flags & O_CREAT) ) {
        int rc = TEMP_FAILURE_RETRY(sqlfs_proc_access(0, path.c_str(), F_OK));
        if (rc != 0) {
            // file does not exist
            do_create = 1;
        }
    }

    int result = 0;
    if( do_create ) {
        LOGI("sqlfs_proc_create");
        char buf = 0;
        result = sqlfs_proc_create(0, path.c_str(), mode, &ffi);
    } else {
        LOGI("sqlfs_proc_open");
        result = sqlfs_proc_open(0, path.c_str(), &ffi);
    }
    if (result < 0) {
        throwErrnoException(env, "open", result);
        return NULL;
    } else {
        sqlfs_proc_chmod(0, path.c_str(), mode);
        return jniCreateFileDescriptor(env, javaPath);
    }
}

static jint Posix_preadBytes(JNIEnv* env, jobject, jobject javaFd, jobject javaBytes, jint byteOffset, jint byteCount, jlong offset) {
    ScopedBytesRW bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    jstring javaPath = jniGetPathFromFileDescriptor(env, javaFd);
    ScopedUtfChars path(env, javaPath);
    int result = sqlfs_proc_read(0, path.c_str(), reinterpret_cast<char*>(bytes.get() + byteOffset), byteCount, (off_t)offset, NULL);
    if (result < 0) {
        if (result != -EIO) { // sqlfs_proc_open returns EIO on end-of-file
            throwErrnoException(env, "pread", result);
        }
        return -1;
    } else {
        return result;
    }
}

static jint Posix_pwriteBytes(JNIEnv* env, jobject, jobject javaFd, jbyteArray javaBytes, jint byteOffset, jint byteCount, jlong offset, jint flags) {
    ScopedBytesRO bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    jstring javaPath = jniGetPathFromFileDescriptor(env, javaFd);
    ScopedUtfChars path(env, javaPath);
    struct fuse_file_info ffi;
    ffi.flags = flags;
    int result = sqlfs_proc_write(0,
                                  path.c_str(),
                                  reinterpret_cast<const char*>(bytes.get() + byteOffset),
                                  byteCount,
                                  offset,
                                  &ffi);
    if (result < 0) {
        throwErrnoException(env, "pwrite", result);
        return -1;
    } else {
        // TODO make this stick the values into javaBytes
        return result;
    }
}

static void Posix_remove(JNIEnv* env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    if(sqlfs_is_dir(0, path.c_str()))
        throwIfNegative(env, "remove", TEMP_FAILURE_RETRY(sqlfs_proc_rmdir(0, path.c_str())));
    else
        throwIfNegative(env, "remove", TEMP_FAILURE_RETRY(sqlfs_proc_unlink(0, path.c_str())));
}

static void Posix_rename(JNIEnv* env, jobject, jstring javaOldPath, jstring javaNewPath) {
    ScopedUtfChars oldPath(env, javaOldPath);
    if (oldPath.c_str() == NULL) {
        return;
    }
    ScopedUtfChars newPath(env, javaNewPath);
    if (newPath.c_str() == NULL) {
        return;
    }
    throwIfNegative(env, "rename", TEMP_FAILURE_RETRY(sqlfs_proc_rename(0, oldPath.c_str(), newPath.c_str())));
}

static void Posix_rmdir(JNIEnv* env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfNegative(env, "rmdir", TEMP_FAILURE_RETRY(sqlfs_proc_rmdir(0, path.c_str())));
}

static jobject Posix_stat(JNIEnv* env, jobject, jstring javaPath) {
    return doStat(env, javaPath, false);
}

/* we are faking this somewhat by using the data from the underlying
 partition that the database file is stored on.  That means we ignore
 the javaPath passed in and just use the dbFilename. */
static jobject Posix_statfs(JNIEnv* env, jobject, jstring javaPath) {
    struct statfs sb;
    int rc = TEMP_FAILURE_RETRY(statfs(dbFileName, &sb));
    if (rc == -1) {
        throwErrnoException(env, "statfs", rc);
        return NULL;
    }
    /* some guesses at how things should be represented */
    sb.f_bsize = 4096; // libsqlfs uses 4k page sizes in sqlite (I think)

    struct stat st;
    stat(dbFileName, &st);
    sb.f_blocks = st.st_blocks;
    return makeStructStatFs(env, sb);
}

static jstring Posix_strerror(JNIEnv* env, jobject, jint errnum) {
    char buffer[BUFSIZ];
    const char* message = jniStrError(errnum, buffer, sizeof(buffer));
    return env->NewStringUTF(message);
}

static void Posix_symlink(JNIEnv* env, jobject, jstring javaOldPath, jstring javaNewPath) {
    ScopedUtfChars oldPath(env, javaOldPath);
    if (oldPath.c_str() == NULL) {
        return;
    }
    ScopedUtfChars newPath(env, javaNewPath);
    if (newPath.c_str() == NULL) {
        return;
    }
    throwIfNegative(env, "symlink", TEMP_FAILURE_RETRY(sqlfs_proc_symlink(0, oldPath.c_str(), newPath.c_str())));
}

static void Posix_unlink(JNIEnv* env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfNegative(env, "unlink", TEMP_FAILURE_RETRY(sqlfs_proc_unlink(0, path.c_str())));
}

static JNINativeMethod sMethods[] = {
    {"access", "(Ljava/lang/String;I)Z", (void *)Posix_access},
    {"chmod", "(Ljava/lang/String;I)V", (void *)Posix_chmod},
    {"close", "(Linfo/guardianproject/iocipher/FileDescriptor;)V", (void *)Posix_close},
    {"fstat", "(Linfo/guardianproject/iocipher/FileDescriptor;)Linfo/guardianproject/libcore/io/StructStat;", (void *)Posix_fstat},
    {"fsync", "(Linfo/guardianproject/iocipher/FileDescriptor;)V", (void *)Posix_fsync},
    {"ftruncate", "(Linfo/guardianproject/iocipher/FileDescriptor;J)V", (void *)Posix_ftruncate},
    {"link", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)Posix_link},
    {"mkdir", "(Ljava/lang/String;I)V", (void *)Posix_mkdir},
    {"open", "(Ljava/lang/String;II)Linfo/guardianproject/iocipher/FileDescriptor;", (void *)Posix_open},
    {"preadBytes", "(Linfo/guardianproject/iocipher/FileDescriptor;Ljava/lang/Object;IIJ)I", (void *)Posix_preadBytes},
    {"pwriteBytes", "(Linfo/guardianproject/iocipher/FileDescriptor;Ljava/lang/Object;IIJI)I", (void *)Posix_pwriteBytes},
    {"remove", "(Ljava/lang/String;)V", (void *)Posix_remove},
    {"rename", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)Posix_rename},
    {"rmdir", "(Ljava/lang/String;)V", (void *)Posix_rmdir},
    {"stat", "(Ljava/lang/String;)Linfo/guardianproject/libcore/io/StructStat;", (void *)Posix_stat},
    {"statfs", "(Ljava/lang/String;)Linfo/guardianproject/libcore/io/StructStatFs;", (void *)Posix_statfs},
    {"strerror", "(I)Ljava/lang/String;", (void *)Posix_strerror},
    {"symlink", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)Posix_symlink},
    {"unlink", "(Ljava/lang/String;)V", (void *)Posix_unlink},
};
int register_info_guardianproject_libcore_io_Posix(JNIEnv* env) {
    jclass cls;

    cls = env->FindClass("info/guardianproject/libcore/io/Posix");
    if (cls == NULL) {
        LOGE("Can't find info/guardianproject/libcore/io/Posix\n");
        return -1;
    }
    return env->RegisterNatives(cls, sMethods, NELEM(sMethods));
}
