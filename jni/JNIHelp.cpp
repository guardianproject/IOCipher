/*
 * Copyright (C) 2006 The Android Open Source Project
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

#define LOG_TAG "JNIHelp"

#include "JNIHelp.h"

#include <android/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * Equivalent to ScopedLocalRef, but for C_JNIEnv instead. (And slightly more powerful.)
 */
template<typename T>
class scoped_local_ref {
public:
    scoped_local_ref(C_JNIEnv* env, T localRef = NULL)
    : mEnv(env), mLocalRef(localRef)
    {
    }

    ~scoped_local_ref() {
        reset();
    }

    void reset(T localRef = NULL) {
        if (mLocalRef != NULL) {
            (*mEnv)->DeleteLocalRef(reinterpret_cast<JNIEnv*>(mEnv), mLocalRef);
            mLocalRef = localRef;
        }
    }

    T get() const {
        return mLocalRef;
    }

private:
    C_JNIEnv* mEnv;
    T mLocalRef;

    // Disallow copy and assignment.
    scoped_local_ref(const scoped_local_ref&);
    void operator=(const scoped_local_ref&);
};

static jclass findClass(C_JNIEnv* env, const char* className) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    return (*env)->FindClass(e, className);
}

/*
 * Returns a human-readable summary of an exception object.  The buffer will
 * be populated with the "binary" class name and, if present, the
 * exception message.
 */
static char* getExceptionSummary0(C_JNIEnv* env, jthrowable exception) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);

    /* get the name of the exception's class */
    scoped_local_ref<jclass> exceptionClass(env, (*env)->GetObjectClass(e, exception)); // can't fail
    scoped_local_ref<jclass> classClass(env,
            (*env)->GetObjectClass(e, exceptionClass.get())); // java.lang.Class, can't fail
    jmethodID classGetNameMethod =
            (*env)->GetMethodID(e, classClass.get(), "getName", "()Ljava/lang/String;");
    scoped_local_ref<jstring> classNameStr(env,
            (jstring) (*env)->CallObjectMethod(e, exceptionClass.get(), classGetNameMethod));
    if (classNameStr.get() == NULL) {
        return NULL;
    }

    /* get printable string */
    const char* classNameChars = (*env)->GetStringUTFChars(e, classNameStr.get(), NULL);
    if (classNameChars == NULL) {
        return NULL;
    }

    /* if the exception has a detail message, get that */
    jmethodID getMessage =
            (*env)->GetMethodID(e, exceptionClass.get(), "getMessage", "()Ljava/lang/String;");
    scoped_local_ref<jstring> messageStr(env,
            (jstring) (*env)->CallObjectMethod(e, exception, getMessage));
    if (messageStr.get() == NULL) {
        return strdup(classNameChars);
    }

    char* result = NULL;
    const char* messageChars = (*env)->GetStringUTFChars(e, messageStr.get(), NULL);
    if (messageChars != NULL) {
        asprintf(&result, "%s: %s", classNameChars, messageChars);
        (*env)->ReleaseStringUTFChars(e, messageStr.get(), messageChars);
    } else {
        (*env)->ExceptionClear(e); // clear OOM
        asprintf(&result, "%s: <error getting message>", classNameChars);
    }

    (*env)->ReleaseStringUTFChars(e, classNameStr.get(), classNameChars);
    return result;
}

static char* getExceptionSummary(C_JNIEnv* env, jthrowable exception) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    char* result = getExceptionSummary0(env, exception);
    if (result == NULL) {
        (*env)->ExceptionClear(e);
        result = strdup("<error getting class name>");
    }
    return result;
}

/*
 * Returns an exception (with stack trace) as a string.
 */
static char* getStackTrace(C_JNIEnv* env, jthrowable exception) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);

    scoped_local_ref<jclass> stringWriterClass(env, findClass(env, "java/io/StringWriter"));
    if (stringWriterClass.get() == NULL) {
        return NULL;
    }

    jmethodID stringWriterCtor = (*env)->GetMethodID(e, stringWriterClass.get(), "<init>", "()V");
    jmethodID stringWriterToStringMethod =
            (*env)->GetMethodID(e, stringWriterClass.get(), "toString", "()Ljava/lang/String;");

    scoped_local_ref<jclass> printWriterClass(env, findClass(env, "java/io/PrintWriter"));
    if (printWriterClass.get() == NULL) {
        return NULL;
    }

    jmethodID printWriterCtor =
            (*env)->GetMethodID(e, printWriterClass.get(), "<init>", "(Ljava/io/Writer;)V");

    scoped_local_ref<jobject> stringWriter(env,
            (*env)->NewObject(e, stringWriterClass.get(), stringWriterCtor));
    if (stringWriter.get() == NULL) {
        return NULL;
    }

    jobject printWriter =
            (*env)->NewObject(e, printWriterClass.get(), printWriterCtor, stringWriter.get());
    if (printWriter == NULL) {
        return NULL;
    }

    scoped_local_ref<jclass> exceptionClass(env, (*env)->GetObjectClass(e, exception)); // can't fail
    jmethodID printStackTraceMethod =
            (*env)->GetMethodID(e, exceptionClass.get(), "printStackTrace", "(Ljava/io/PrintWriter;)V");
    (*env)->CallVoidMethod(e, exception, printStackTraceMethod, printWriter);

    if ((*env)->ExceptionCheck(e)) {
        return NULL;
    }

    scoped_local_ref<jstring> messageStr(env,
            (jstring) (*env)->CallObjectMethod(e, stringWriter.get(), stringWriterToStringMethod));
    if (messageStr.get() == NULL) {
        return NULL;
    }

    const char* utfChars = (*env)->GetStringUTFChars(e, messageStr.get(), NULL);
    if (utfChars == NULL) {
        return NULL;
    }

    char* result = strdup(utfChars);
    (*env)->ReleaseStringUTFChars(e, messageStr.get(), utfChars);
    return result;
}

extern "C" int jniThrowException(C_JNIEnv* env, const char* className, const char* msg) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);

    if ((*env)->ExceptionCheck(e)) {
        /* TODO: consider creating the new exception with this as "cause" */
        scoped_local_ref<jthrowable> exception(env, (*env)->ExceptionOccurred(e));
        (*env)->ExceptionClear(e);

        if (exception.get() != NULL) {
            char* text = getExceptionSummary(env, exception.get());
            LOGW("Discarding pending exception (%s) to throw %s", text, className);
            free(text);
        }
    }

    scoped_local_ref<jclass> exceptionClass(env, findClass(env, className));
    if (exceptionClass.get() == NULL) {
        LOGE("Unable to find exception class %s", className);
        /* ClassNotFoundException now pending */
        return -1;
    }

    if ((*env)->ThrowNew(e, exceptionClass.get(), msg) != JNI_OK) {
        LOGE("Failed throwing '%s' '%s'", className, msg);
        /* an exception, most likely OOM, will now be pending */
        return -1;
    }

    return 0;
}

int jniThrowExceptionFmt(C_JNIEnv* env, const char* className, const char* fmt, va_list args) {
    char msgBuf[512];
    vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
    return jniThrowException(env, className, msgBuf);
}

int jniThrowNullPointerException(C_JNIEnv* env, const char* msg) {
    return jniThrowException(env, "java/lang/NullPointerException", msg);
}

int jniThrowRuntimeException(C_JNIEnv* env, const char* msg) {
    return jniThrowException(env, "java/lang/RuntimeException", msg);
}

int jniThrowIOException(C_JNIEnv* env, int errnum) {
    char buffer[80];
    const char* message = jniStrError(errnum, buffer, sizeof(buffer));
    return jniThrowException(env, "java/io/IOException", message);
}

void jniLogException(C_JNIEnv* env, int priority, const char* tag, jthrowable exception) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);

    scoped_local_ref<jthrowable> currentException(env);
    if (exception == NULL) {
        exception = (*env)->ExceptionOccurred(e);
        if (exception == NULL) {
            return;
        }

        (*env)->ExceptionClear(e);
        currentException.reset(exception);
    }

    char* buffer = getStackTrace(env, exception);
    if (buffer == NULL) {
        (*env)->ExceptionClear(e);
        buffer = getExceptionSummary(env, exception);
    }

    __android_log_write(priority, tag, buffer);
    free(buffer);

    if (currentException.get() != NULL) {
        (*env)->Throw(e, exception); // rethrow
    }
}

const char* jniStrError(int errnum, char* buf, size_t buflen) {
#if __GLIBC__
    // Note: glibc has a nonstandard strerror_r that returns char* rather than POSIX's int.
    // char *strerror_r(int errnum, char *buf, size_t n);
    return strerror_r(errnum, buf, buflen);
#else
    int rc = strerror_r(errnum, buf, buflen);
    if (rc != 0) {
        // (POSIX only guarantees a value other than 0. The safest
        // way to implement this function is to use C++ and overload on the
        // type of strerror_r to accurately distinguish GNU from POSIX.)
        snprintf(buf, buflen, "errno %d", errnum);
    }
    return buf;
#endif
}

static struct CachedFields {
    jclass fileDescriptorClass;
    jmethodID fileDescriptorCtor;
    jfieldID pathField;
    jfieldID invalidField;
} gCachedFields;

int registerJniHelp(JNIEnv* env) {
    gCachedFields.fileDescriptorClass =
            reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("info/guardianproject/iocipher/FileDescriptor")));
    if (gCachedFields.fileDescriptorClass == NULL) {
        return -1;
    }

    gCachedFields.fileDescriptorCtor =
            env->GetMethodID(gCachedFields.fileDescriptorClass, "<init>", "()V");
    if (gCachedFields.fileDescriptorCtor == NULL) {
        return -1;
    }

    gCachedFields.pathField =
        env->GetFieldID(gCachedFields.fileDescriptorClass,
                        "path", "Ljava/lang/String;");
    if (gCachedFields.pathField == NULL) {
        return -1;
    }

    gCachedFields.invalidField =
        env->GetFieldID(gCachedFields.fileDescriptorClass,
                        "invalid", "Ljava/lang/String;");
    if (gCachedFields.invalidField == NULL) {
        return -1;
    }

    return 0;
}

/* in sqlfs, the full path is used as the file descriptor */
jobject jniCreateFileDescriptor(C_JNIEnv* env, jstring javaPath) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    jobject fileDescriptor = (*env)->NewObject(e,
            gCachedFields.fileDescriptorClass, gCachedFields.fileDescriptorCtor);
    jniSetFileDescriptorWithPath(env, fileDescriptor, javaPath);
    return fileDescriptor;
}

jstring jniGetPathFromFileDescriptor(C_JNIEnv* env, jobject fileDescriptor) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    return static_cast<jstring>((*env)->GetObjectField(e, fileDescriptor, gCachedFields.pathField));
}

void jniSetFileDescriptorWithPath(C_JNIEnv* env, jobject fileDescriptor, jstring javaPath) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    (*env)->SetObjectField(e, fileDescriptor, gCachedFields.pathField, javaPath);
}

void jniSetFileDescriptorInvalid(C_JNIEnv* env, jobject fileDescriptor) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    jstring javaInvalid = static_cast<jstring>((*env)->GetObjectField(e, fileDescriptor,
                                                                      gCachedFields.invalidField));
    (*env)->SetObjectField(e, fileDescriptor, gCachedFields.pathField, javaInvalid);
}
