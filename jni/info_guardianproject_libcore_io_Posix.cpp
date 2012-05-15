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
#include "stubs.h"

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
extern char databaseFileName[PATH_MAX];
extern sqlfs_t *sqlfs;


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

/*
static void throwGaiException(JNIEnv* env, const char* functionName, int error) {
    if (error == EAI_SYSTEM) {
        // EAI_SYSTEM means "look at errno instead", so we want our GaiException to have the
        // relevant ErrnoException as its cause.
        throwErrnoException(env, functionName);
        // Deliberately fall through to throw another exception...
    }
    static jmethodID ctor3 = env->GetMethodID(JniConstants::gaiExceptionClass,
            "<init>", "(Ljava/lang/String;ILjava/lang/Throwable;)V");
    static jmethodID ctor2 = env->GetMethodID(JniConstants::gaiExceptionClass,
            "<init>", "(Ljava/lang/String;I)V");
    throwException(env, JniConstants::gaiExceptionClass, ctor3, ctor2, functionName, error);
}
*/

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
    int rc = TEMP_FAILURE_RETRY(sqlfs_proc_getattr(sqlfs, path.c_str(), &sb));
    if (rc < 0) {
        throwErrnoException(env, isLstat ? "lstat" : "stat", rc);
        return NULL;
    }
    return makeStructStat(env, sb);
}

/*
class Passwd {
public:
    Passwd(JNIEnv* env) : mEnv(env), mResult(NULL) {
        mBufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);
        if (mBufferSize == -1UL) {
            // We're probably on bionic, where 1KiB should be enough for anyone.
            // TODO: fix bionic to return 1024 like glibc.
            mBufferSize = 1024;
        }
        mBuffer.reset(new char[mBufferSize]);
    }

    jobject getpwnam(const char* name) {
        return process("getpwnam_r", getpwnam_r(name, &mPwd, mBuffer.get(), mBufferSize, &mResult));
    }

    jobject getpwuid(uid_t uid) {
        return process("getpwuid_r", getpwuid_r(uid, &mPwd, mBuffer.get(), mBufferSize, &mResult));
    }

    struct passwd* get() {
        return mResult;
    }

private:
    jobject process(const char* syscall, int error) {
        if (mResult == NULL) {
            throwErrnoException(mEnv, syscall, error);
            return NULL;
        }
        return makeStructPasswd(mEnv, *mResult);
    }

    JNIEnv* mEnv;
    UniquePtr<char[]> mBuffer;
    size_t mBufferSize;
    struct passwd mPwd;
    struct passwd* mResult;
};
*/

static jboolean Posix_access(JNIEnv* env, jobject, jstring javaPath, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return JNI_FALSE;
    }
    int rc = TEMP_FAILURE_RETRY(sqlfs_proc_access(sqlfs, path.c_str(), mode));
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
    throwIfNegative(env, "chmod", TEMP_FAILURE_RETRY(sqlfs_proc_chmod(sqlfs, path.c_str(), mode)));
}

static void Posix_close(JNIEnv* env, jobject, jobject javaFd) {
    // Get the FileDescriptor's 'fd' field and clear it.
    // sqlfs doesn't have a close() since files don't really need to be open()ed
    jstring path = jniGetPathFromFileDescriptor(env, javaFd);
    jniSetFileDescriptorInvalid(env, javaFd);
}

/*
static jobject Posix_dup(JNIEnv* env, jobject, jobject javaOldFd) {
    int oldFd = jniGetFDFromFileDescriptor(env, javaOldFd);
    int newFd = throwIfNegative(env, "dup", TEMP_FAILURE_RETRY(dup(oldFd)));
    return (newFd != -1) ? jniCreateFileDescriptor(env, newFd) : NULL;
}

static jobject Posix_dup2(JNIEnv* env, jobject, jobject javaOldFd, jint newFd) {
    int oldFd = jniGetFDFromFileDescriptor(env, javaOldFd);
    int fd = throwIfNegative(env, "dup2", TEMP_FAILURE_RETRY(dup2(oldFd, newFd)));
    return (fd != -1) ? jniCreateFileDescriptor(env, fd) : NULL;
}

static jobjectArray Posix_environ(JNIEnv* env, jobject) {
    extern char** environ; // Standard, but not in any header file.
    return toStringArray(env, environ);
}

static jint Posix_fcntlVoid(JNIEnv* env, jobject, jobject javaFd, jint cmd) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    return throwIfNegative(env, "fcntl", TEMP_FAILURE_RETRY(fcntl(fd, cmd)));
}

static jint Posix_fcntlLong(JNIEnv* env, jobject, jobject javaFd, jint cmd, jlong arg) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    return throwIfNegative(env, "fcntl", TEMP_FAILURE_RETRY(fcntl(fd, cmd, arg)));
}

static jint Posix_fcntlFlock(JNIEnv* env, jobject, jobject javaFd, jint cmd, jobject javaFlock) {
    static jfieldID typeFid = env->GetFieldID(JniConstants::structFlockClass, "l_type", "S");
    static jfieldID whenceFid = env->GetFieldID(JniConstants::structFlockClass, "l_whence", "S");
    static jfieldID startFid = env->GetFieldID(JniConstants::structFlockClass, "l_start", "J");
    static jfieldID lenFid = env->GetFieldID(JniConstants::structFlockClass, "l_len", "J");
    static jfieldID pidFid = env->GetFieldID(JniConstants::structFlockClass, "l_pid", "I");

    struct flock64 lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = env->GetShortField(javaFlock, typeFid);
    lock.l_whence = env->GetShortField(javaFlock, whenceFid);
    lock.l_start = env->GetLongField(javaFlock, startFid);
    lock.l_len = env->GetLongField(javaFlock, lenFid);
    lock.l_pid = env->GetIntField(javaFlock, pidFid);

    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    int rc = throwIfNegative(env, "fcntl", TEMP_FAILURE_RETRY(fcntl(fd, cmd, &lock)));
    if (rc != -1) {
        env->SetShortField(javaFlock, typeFid, lock.l_type);
        env->SetShortField(javaFlock, whenceFid, lock.l_whence);
        env->SetLongField(javaFlock, startFid, lock.l_start);
        env->SetLongField(javaFlock, lenFid, lock.l_len);
        env->SetIntField(javaFlock, pidFid, lock.l_pid);
    }
    return rc;
}

static void Posix_fdatasync(JNIEnv* env, jobject, jobject javaFd) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfNegative(env, "fdatasync", TEMP_FAILURE_RETRY(fdatasync(fd)));
}
*/

static jobject Posix_fstat(JNIEnv* env, jobject, jobject javaFd) {
    jstring javaPath = jniGetPathFromFileDescriptor(env, javaFd);
    return doStat(env, javaPath, false);
}

// TODO if sqlfs_proc_fsync changes to need isfdatasync and *fi, then fix here
static void Posix_fsync(JNIEnv* env, jobject, jobject javaFd) {
    jstring javaPath = jniGetPathFromFileDescriptor(env, javaFd);
    ScopedUtfChars path(env, javaPath);
    throwIfNegative(env, "fsync", TEMP_FAILURE_RETRY(sqlfs_proc_fsync(sqlfs, path.c_str(), 0, NULL)));
}

/* in sqlfs, truncate() and ftruncate() do the same thing since there
 * isn't a difference between and open and a closed file */
static void Posix_ftruncate(JNIEnv* env, jobject, jobject javaFd, jlong length) {
    jstring javaPath = jniGetPathFromFileDescriptor(env, javaFd);
    ScopedUtfChars path(env, javaPath);
    throwIfNegative(env, "ftruncate", TEMP_FAILURE_RETRY(sqlfs_proc_truncate(sqlfs, path.c_str(), length)));
}

/*
static jstring Posix_gai_strerror(JNIEnv* env, jobject, jint error) {
    return env->NewStringUTF(gai_strerror(error));
}

static jint Posix_getegid(JNIEnv*, jobject) {
    return getegid();
}

static jint Posix_geteuid(JNIEnv*, jobject) {
    return geteuid();
}

static jint Posix_getgid(JNIEnv*, jobject) {
    return getgid();
}

static jstring Posix_getenv(JNIEnv* env, jobject, jstring javaName) {
    ScopedUtfChars name(env, javaName);
    if (name.c_str() == NULL) {
        return NULL;
    }
    return env->NewStringUTF(getenv(name.c_str()));
}

static jint Posix_getpid(JNIEnv*, jobject) {
    return getpid();
}

static jint Posix_getppid(JNIEnv*, jobject) {
    return getppid();
}

static jobject Posix_getpwnam(JNIEnv* env, jobject, jstring javaName) {
    ScopedUtfChars name(env, javaName);
    if (name.c_str() == NULL) {
        return NULL;
    }
    return Passwd(env).getpwnam(name.c_str());
}

static jobject Posix_getpwuid(JNIEnv* env, jobject, jint uid) {
    return Passwd(env).getpwuid(uid);
}

static jint Posix_ioctlInt(JNIEnv* env, jobject, jobject javaFd, jint cmd, jobject javaArg) {
    // This is complicated because ioctls may return their result by updating their argument
    // or via their return value, so we need to support both.
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    static jfieldID valueFid = env->GetFieldID(JniConstants::mutableIntClass, "value", "I");
    jint arg = env->GetIntField(javaArg, valueFid);
    int rc = throwIfNegative(env, "ioctl", TEMP_FAILURE_RETRY(ioctl(fd, cmd, &arg)));
    if (!env->ExceptionCheck()) {
        env->SetIntField(javaArg, valueFid, arg);
    }
    return rc;
}

static jboolean Posix_isatty(JNIEnv* env, jobject, jobject javaFd) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    return TEMP_FAILURE_RETRY(isatty(fd)) == 0;
}

static void Posix_kill(JNIEnv* env, jobject, jint pid, jint sig) {
    throwIfNegative(env, "kill", TEMP_FAILURE_RETRY(kill(pid, sig)));
}

static jobject Posix_lstat(JNIEnv* env, jobject, jstring javaPath) {
    return doStat(env, javaPath, true);
}

static void Posix_mincore(JNIEnv* env, jobject, jlong address, jlong byteCount, jbyteArray javaVector) {
    ScopedByteArrayRW vector(env, javaVector);
    if (vector.get() == NULL) {
        return;
    }
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    unsigned char* vec = reinterpret_cast<unsigned char*>(vector.get());
    throwIfNegative(env, "mincore", TEMP_FAILURE_RETRY(mincore(ptr, byteCount, vec)));
}
*/

static void Posix_link(JNIEnv* env, jobject, jstring javaFrom, jstring javaTo) {
    ScopedUtfChars from(env, javaFrom);
    ScopedUtfChars to(env, javaTo);
    if (from.c_str() == NULL || from.c_str() == NULL) {
        return;
    }
    throwIfNegative(env, "link", TEMP_FAILURE_RETRY(sqlfs_proc_link(sqlfs, from.c_str(), from.c_str())));
}

static void Posix_mkdir(JNIEnv* env, jobject, jstring javaPath, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    if (sqlfs == NULL) {
        // TODO throw exception warning that VirtualFileSystem is not open
        LOGE("VirtualFileSystem is not open");
        return;
    }
    throwIfNegative(env, "mkdir", TEMP_FAILURE_RETRY(sqlfs_proc_mkdir(sqlfs, path.c_str(), mode)));
}

/*
static void Posix_mlock(JNIEnv* env, jobject, jlong address, jlong byteCount) {
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    throwIfNegative(env, "mlock", TEMP_FAILURE_RETRY(mlock(ptr, byteCount)));
}

static jlong Posix_mmap(JNIEnv* env, jobject, jlong address, jlong byteCount, jint prot, jint flags, jobject javaFd, jlong offset) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    void* suggestedPtr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    void* ptr = mmap(suggestedPtr, byteCount, prot, flags, fd, offset);
    if (ptr == MAP_FAILED) {
        throwErrnoException(env, "mmap");
    }
    return static_cast<jlong>(reinterpret_cast<uintptr_t>(ptr));
}

static void Posix_msync(JNIEnv* env, jobject, jlong address, jlong byteCount, jint flags) {
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    throwIfNegative(env, "msync", TEMP_FAILURE_RETRY(msync(ptr, byteCount, flags)));
}

static void Posix_munlock(JNIEnv* env, jobject, jlong address, jlong byteCount) {
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    throwIfNegative(env, "munlock", TEMP_FAILURE_RETRY(munlock(ptr, byteCount)));
}

static void Posix_munmap(JNIEnv* env, jobject, jlong address, jlong byteCount) {
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    throwIfNegative(env, "munmap", TEMP_FAILURE_RETRY(munmap(ptr, byteCount)));
}
*/

static jobject Posix_open(JNIEnv* env, jobject, jstring javaPath, jint flags, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return NULL;
    }
    LOGI("path: %s %x %o", path.c_str(), flags, mode);
    struct fuse_file_info ffi;
    ffi.flags = flags;
    ffi.direct_io = 0; // don't use direct_io so this open() call will create a file

    int result = 0;
    if(flags & O_CREAT) {
        LOGI("sqlfs_proc_create");
        char buf = 0;
        result = sqlfs_proc_create(sqlfs, path.c_str(), mode, &ffi);
    } else {
        LOGI("sqlfs_proc_open");
        result = sqlfs_proc_open(sqlfs, path.c_str(), &ffi);
    }
	if (result < 0) {
		throwErrnoException(env, "open", result);
		return NULL;
	} else {
		sqlfs_proc_chmod(sqlfs, path.c_str(), mode);
		return jniCreateFileDescriptor(env, javaPath);
	}
}

/*
static jobjectArray Posix_pipe(JNIEnv* env, jobject) {
    int fds[2];
    throwIfNegative(env, "pipe", TEMP_FAILURE_RETRY(pipe(&fds[0])));
    jobjectArray result = env->NewObjectArray(2, JniConstants::fileDescriptorClass, NULL);
    if (result == NULL) {
        return NULL;
    }
    for (int i = 0; i < 2; ++i) {
        ScopedLocalRef<jobject> fd(env, jniCreateFileDescriptor(env, fds[i]));
        if (fd.get() == NULL) {
            return NULL;
        }
        env->SetObjectArrayElement(result, i, fd.get());
        if (env->ExceptionCheck()) {
            return NULL;
        }
    }
    return result;
}

static jint Posix_poll(JNIEnv* env, jobject, jobjectArray javaStructs, jint timeoutMs) {
    static jfieldID fdFid = env->GetFieldID(JniConstants::structPollfdClass, "fd", "Ljava/io/FileDescriptor;");
    static jfieldID eventsFid = env->GetFieldID(JniConstants::structPollfdClass, "events", "S");
    static jfieldID reventsFid = env->GetFieldID(JniConstants::structPollfdClass, "revents", "S");

    // Turn the Java libcore.io.StructPollfd[] into a C++ struct pollfd[].
    size_t arrayLength = env->GetArrayLength(javaStructs);
    UniquePtr<struct pollfd[]> fds(new struct pollfd[arrayLength]);
    memset(fds.get(), 0, sizeof(struct pollfd) * arrayLength);
    size_t count = 0; // Some trailing array elements may be irrelevant. (See below.)
    for (size_t i = 0; i < arrayLength; ++i) {
        ScopedLocalRef<jobject> javaStruct(env, env->GetObjectArrayElement(javaStructs, i));
        if (javaStruct.get() == NULL) {
            break; // We allow trailing nulls in the array for caller convenience.
        }
        ScopedLocalRef<jobject> javaFd(env, env->GetObjectField(javaStruct.get(), fdFid));
        if (javaFd.get() == NULL) {
            break; // We also allow callers to just clear the fd field (this is what Selector does).
        }
        fds[count].fd = jniGetFDFromFileDescriptor(env, javaFd.get());
        fds[count].events = env->GetShortField(javaStruct.get(), eventsFid);
        ++count;
    }

    int rc = TEMP_FAILURE_RETRY(poll(fds.get(), count, timeoutMs));
    if (rc == -1) {
        throwErrnoException(env, "poll", rc);
        return -1;
    }

    // Update the revents fields in the Java libcore.io.StructPollfd[].
    for (size_t i = 0; i < count; ++i) {
        ScopedLocalRef<jobject> javaStruct(env, env->GetObjectArrayElement(javaStructs, i));
        if (javaStruct.get() == NULL) {
            return -1;
        }
        env->SetShortField(javaStruct.get(), reventsFid, fds[i].revents);
    }
    return rc;
}
*/

static jint Posix_preadBytes(JNIEnv* env, jobject, jobject javaFd, jobject javaBytes, jint byteOffset, jint byteCount, jlong offset) {
    ScopedBytesRW bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    jstring javaPath = jniGetPathFromFileDescriptor(env, javaFd);
    ScopedUtfChars path(env, javaPath);
    int result = sqlfs_proc_read(sqlfs, path.c_str(), reinterpret_cast<char*>(bytes.get() + byteOffset), byteCount, (off_t)offset, NULL);
    if (result < 0) {
        if (result != -EIO) { // sqlfs_proc_open returns EIO on end-of-file
            throwErrnoException(env, "pread", result);
        }
        return -1;
    } else {
        return result;
    }
}

/*
static jint Posix_readv(JNIEnv* env, jobject, jobject javaFd, jobjectArray buffers, jintArray offsets, jintArray byteCounts) {
    IoVec<ScopedBytesRW> ioVec(env, env->GetArrayLength(buffers));
    if (!ioVec.init(buffers, offsets, byteCounts)) {
        return -1;
    }
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    return throwIfNegative(env, "readv", TEMP_FAILURE_RETRY(readv(fd, ioVec.get(), ioVec.size())));
}
*/

static void Posix_remove(JNIEnv* env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    if(sqlfs_is_dir(sqlfs, path.c_str()))
        throwIfNegative(env, "remove", TEMP_FAILURE_RETRY(sqlfs_proc_rmdir(sqlfs, path.c_str())));
    else
        throwIfNegative(env, "remove", TEMP_FAILURE_RETRY(sqlfs_proc_unlink(sqlfs, path.c_str())));
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
    throwIfNegative(env, "rename", TEMP_FAILURE_RETRY(sqlfs_proc_rename(sqlfs, oldPath.c_str(), newPath.c_str())));
}

static void Posix_rmdir(JNIEnv* env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfNegative(env, "rmdir", TEMP_FAILURE_RETRY(sqlfs_proc_rmdir(sqlfs, path.c_str())));
}

/*
static void Posix_setegid(JNIEnv* env, jobject, jint egid) {
    throwIfNegative(env, "setegid", TEMP_FAILURE_RETRY(setegid(egid)));
}

static void Posix_seteuid(JNIEnv* env, jobject, jint euid) {
    throwIfNegative(env, "seteuid", TEMP_FAILURE_RETRY(seteuid(euid)));
}

static void Posix_setgid(JNIEnv* env, jobject, jint gid) {
    throwIfNegative(env, "setgid", TEMP_FAILURE_RETRY(setgid(gid)));
}

static void Posix_setuid(JNIEnv* env, jobject, jint uid) {
    throwIfNegative(env, "setuid", TEMP_FAILURE_RETRY(setuid(uid)));
}

static void Posix_shutdown(JNIEnv* env, jobject, jobject javaFd, jint how) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfNegative(env, "shutdown", TEMP_FAILURE_RETRY(shutdown(fd, how)));
}
*/

static jobject Posix_stat(JNIEnv* env, jobject, jstring javaPath) {
    return doStat(env, javaPath, false);
}

/* we are faking this somewhat by using the data from the underlying
 partition that the database file is stored on.  That means we ignore
 the javaPath passed in and just use the databaseFilename. */
static jobject Posix_statfs(JNIEnv* env, jobject, jstring javaPath) {
    struct statfs sb;
    int rc = TEMP_FAILURE_RETRY(statfs(databaseFileName, &sb));
    if (rc == -1) {
        throwErrnoException(env, "statfs", rc);
        return NULL;
    }
    /* some guesses at how things should be represented */
    sb.f_bsize = 4096; // libsqlfs uses 4k page sizes in sqlite (I think)

    struct stat st;
    stat(databaseFileName, &st);
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
    throwIfNegative(env, "symlink", TEMP_FAILURE_RETRY(sqlfs_proc_symlink(sqlfs, oldPath.c_str(), newPath.c_str())));
}

static void Posix_unlink(JNIEnv* env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfNegative(env, "unlink", TEMP_FAILURE_RETRY(sqlfs_proc_unlink(sqlfs, path.c_str())));
}

/*
static jlong Posix_sysconf(JNIEnv* env, jobject, jint name) {
    // Since -1 is a valid result from sysconf(3), detecting failure is a little more awkward.
    errno = 0;
    long result = sysconf(name);
    if (result == -1L && errno == EINVAL) {
        throwErrnoException(env, "sysconf");
    }
    return result;
}

static jobject Posix_uname(JNIEnv* env, jobject) {
    struct utsname buf;
    if (TEMP_FAILURE_RETRY(uname(&buf)) == -1) {
        return NULL; // Can't happen.
    }
    return makeStructUtsname(env, buf);
}

static jint Posix_waitpid(JNIEnv* env, jobject, jint pid, jobject javaStatus, jint options) {
    int status;
    int rc = throwIfNegative(env, "waitpid", TEMP_FAILURE_RETRY(waitpid(pid, &status, options)));
    if (rc != -1) {
        static jfieldID valueFid = env->GetFieldID(JniConstants::mutableIntClass, "value", "I");
        env->SetIntField(javaStatus, valueFid, status);
    }
    return rc;
}
*/

static jint Posix_writeBytes(JNIEnv* env, jobject, jobject javaFd, jbyteArray javaBytes, jint byteOffset, jint byteCount) {
    ScopedBytesRO bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    jstring javaPath = jniGetPathFromFileDescriptor(env, javaFd);
    ScopedUtfChars path(env, javaPath);
    //return throwIfNegative(env, "write", TEMP_FAILURE_RETRY(write(fd, bytes.get() + byteOffset, byteCount)));
    int result = sqlfs_proc_write(sqlfs, path.c_str(), reinterpret_cast<const char*>(bytes.get()), byteCount, byteOffset, NULL);
    if (result < 0) {
        throwErrnoException(env, "write", result);
        return -1;
    } else {
        // TODO make this stick the values into javaBytes
        return result;
    }
}

/*
static jint Posix_writev(JNIEnv* env, jobject, jobject javaFd, jobjectArray buffers, jintArray offsets, jintArray byteCounts) {
    IoVec<ScopedBytesRO> ioVec(env, env->GetArrayLength(buffers));
    if (!ioVec.init(buffers, offsets, byteCounts)) {
        return -1;
    }
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    return throwIfNegative(env, "writev", TEMP_FAILURE_RETRY(writev(fd, ioVec.get(), ioVec.size())));
}
*/

static JNINativeMethod sMethods[] = {
    {"access", "(Ljava/lang/String;I)Z", (void *)Posix_access},
    {"chmod", "(Ljava/lang/String;I)V", (void *)Posix_chmod},
    {"close", "(Linfo/guardianproject/iocipher/FileDescriptor;)V", (void *)Posix_close},
//    {"dup", "(Linfo/guardianproject/iocipher/FileDescriptor;)Linfo/guardianproject/iocipher/FileDescriptor;", (void *)Posix_dup},
//    {"dup2", "(Linfo/guardianproject/iocipher/FileDescriptor;I)Linfo/guardianproject/iocipher/FileDescriptor;", (void *)Posix_dup2},
//    {"environ", "()[Ljava/lang/String;", (void *)Posix_environ},
//    {"fcntlVoid", "(Linfo/guardianproject/iocipher/FileDescriptor;I)I", (void *)Posix_fcntlVoid},
//    {"fcntlLong", "(Linfo/guardianproject/iocipher/FileDescriptor;IJ)I", (void *)Posix_fcntlLong},
//    {"fcntlFlock", "(Linfo/guardianproject/iocipher/FileDescriptor;ILlibcore/io/StructFlock;)I", (void *)Posix_fcntlFlock},
//    {"fdatasync", "(Linfo/guardianproject/iocipher/FileDescriptor;)V", (void *)Posix_fdatasync},
    {"fstat", "(Linfo/guardianproject/iocipher/FileDescriptor;)Linfo/guardianproject/libcore/io/StructStat;", (void *)Posix_fstat},
//    {"fstatfs", "(Linfo/guardianproject/iocipher/FileDescriptor;)Linfo/guardianproject/libcore/io/StructStatFs;", (void *)Posix_fstatfs},
    {"fsync", "(Linfo/guardianproject/iocipher/FileDescriptor;)V", (void *)Posix_fsync},
    {"ftruncate", "(Linfo/guardianproject/iocipher/FileDescriptor;J)V", (void *)Posix_ftruncate},
//    {"gai_strerror", "(I)Ljava/lang/String;", (void *)Posix_gai_strerror},
//    {"getegid", "()I", (void *)Posix_getegid},
//    {"geteuid", "()I", (void *)Posix_geteuid},
//    {"getgid", "()I", (void *)Posix_getgid},
//    {"getenv", "(Ljava/lang/String;)Ljava/lang/String;", (void *)Posix_getenv},
//    {"getpid", "()I", (void *)Posix_getpid},
//    {"getppid", "()I", (void *)Posix_getppid},
//    {"getpwnam", "(Ljava/lang/String;)Llibcore/io/StructPasswd;", (void *)Posix_getpwnam},
//    {"getpwuid", "(I)Llibcore/io/StructPasswd;", (void *)Posix_getpwuid},
//    {"getuid", "()I", (void *)Posix_getuid},
//    {"if_indextoname", "(I)Ljava/lang/String;", (void *)Posix_if_indextoname},
//    {"ioctlInt", "(Linfo/guardianproject/iocipher/FileDescriptor;ILlibcore/util/MutableInt;)I", (void *)Posix_ioctlInt},
//    {"isatty", "(Linfo/guardianproject/iocipher/FileDescriptor;)Z", (void *)Posix_isatty},
//    {"kill", "(II)V", (void *)Posix_kill},
//    {"listen", "(Linfo/guardianproject/iocipher/FileDescriptor;I)V", (void *)Posix_listen},
//    {"lstat", "(Ljava/lang/String;)Linfo/guardianproject/libcore/io/StructStat;", (void *)Posix_lstat},
//    {"mincore", "(JJ[B)V", (void *)Posix_mincore},
    {"link", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)Posix_link},
    {"mkdir", "(Ljava/lang/String;I)V", (void *)Posix_mkdir},
//    {"mlock", "(JJ)V", (void *)Posix_mlock},
//    {"mmap", "(JJIILinfo/guardianproject/iocipher/FileDescriptor;J)J", (void *)Posix_mmap},
//    {"msync", "(JJI)V", (void *)Posix_msync},
//    {"munlock", "(JJ)V", (void *)Posix_munlock},
//    {"munmap", "(JJ)V", (void *)Posix_munmap},
    {"open", "(Ljava/lang/String;II)Linfo/guardianproject/iocipher/FileDescriptor;", (void *)Posix_open},
//    {"pipe", "()[Linfo/guardianproject/iocipher/FileDescriptor;", (void *)Posix_pipe},
//    {"poll", "([Llibcore/io/StructPollfd;I)I", (void *)Posix_poll},
    {"preadBytes", "(Linfo/guardianproject/iocipher/FileDescriptor;Ljava/lang/Object;IIJ)I", (void *)Posix_preadBytes},
//    {"pwriteBytes", "(Linfo/guardianproject/iocipher/FileDescriptor;Ljava/lang/Object;IIJ)I", (void *)Posix_pwriteBytes},
//    {"readBytes", "(Linfo/guardianproject/iocipher/FileDescriptor;Ljava/lang/Object;II)I", (void *)Posix_readBytes},
//    {"readv", "(Linfo/guardianproject/iocipher/FileDescriptor;[Ljava/lang/Object;[I[I)I", (void *)Posix_readv},
    {"remove", "(Ljava/lang/String;)V", (void *)Posix_remove},
    {"rename", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)Posix_rename},
    {"rmdir", "(Ljava/lang/String;)V", (void *)Posix_rmdir},
//    {"sendfile", "(Linfo/guardianproject/iocipher/FileDescriptor;Linfo/guardianproject/iocipher/FileDescriptor;Llibcore/util/MutableLong;J)J", (void *)Posix_sendfile},
//    {"setegid", "(I)V", (void *)Posix_setegid},
//    {"seteuid", "(I)V", (void *)Posix_seteuid},
//    {"setgid", "(I)V", (void *)Posix_setgid},
//    {"setuid", "(I)V", (void *)Posix_setuid},
//    {"shutdown", "(Linfo/guardianproject/iocipher/FileDescriptor;I)V", (void *)Posix_shutdown},
    {"stat", "(Ljava/lang/String;)Linfo/guardianproject/libcore/io/StructStat;", (void *)Posix_stat},
    {"statfs", "(Ljava/lang/String;)Linfo/guardianproject/libcore/io/StructStatFs;", (void *)Posix_statfs},
    {"strerror", "(I)Ljava/lang/String;", (void *)Posix_strerror},
    {"symlink", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)Posix_symlink},
    {"unlink", "(Ljava/lang/String;)V", (void *)Posix_unlink},
//    {"sysconf", "(I)J", (void *)Posix_sysconf},
//    {"uname", "()Llibcore/io/StructUtsname;", (void *)Posix_uname},
//    {"waitpid", "(ILlibcore/util/MutableInt;I)I", (void *)Posix_waitpid},
    {"writeBytes", "(Linfo/guardianproject/iocipher/FileDescriptor;Ljava/lang/Object;II)I", (void *)Posix_writeBytes},
//    {"writev", "(Linfo/guardianproject/iocipher/FileDescriptor;[Ljava/lang/Object;[I[I)I", (void *)Posix_writev},
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
