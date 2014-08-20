/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include "JniConstants.h"
#include "ScopedLocalRef.h"

#include <stdlib.h>

#define LOG_TAG "JniConstants"

jclass JniConstants::bigDecimalClass;
jclass JniConstants::booleanClass;
jclass JniConstants::byteArrayClass;
jclass JniConstants::byteClass;
jclass JniConstants::constructorClass;
jclass JniConstants::deflaterClass;
jclass JniConstants::doubleClass;
jclass JniConstants::errnoExceptionClass;
jclass JniConstants::fieldClass;
jclass JniConstants::fileDescriptorClass;
jclass JniConstants::inflaterClass;
jclass JniConstants::integerClass;
jclass JniConstants::longClass;
jclass JniConstants::methodClass;
jclass JniConstants::parsePositionClass;
jclass JniConstants::patternSyntaxExceptionClass;
jclass JniConstants::stringArrayClass;
jclass JniConstants::stringClass;
jclass JniConstants::structAddrinfoClass;
jclass JniConstants::structFlockClass;
jclass JniConstants::structGroupReqClass;
jclass JniConstants::structLingerClass;
jclass JniConstants::structPasswdClass;
jclass JniConstants::structPollfdClass;
jclass JniConstants::structStatClass;
jclass JniConstants::structStatFsClass;
jclass JniConstants::structTimevalClass;
jclass JniConstants::structUtsnameClass;

static jclass findClass(JNIEnv* env, const char* name) {
    ScopedLocalRef<jclass> localClass(env, env->FindClass(name));
    jclass result = reinterpret_cast<jclass>(env->NewGlobalRef(localClass.get()));
    if (result == NULL) {
        LOGE("failed to find class '%s'", name);
        abort();
    }
    return result;
}

void JniConstants::init(JNIEnv* env) {
    bigDecimalClass = findClass(env, "java/math/BigDecimal");
    booleanClass = findClass(env, "java/lang/Boolean");
    byteClass = findClass(env, "java/lang/Byte");
    byteArrayClass = findClass(env, "[B");
    constructorClass = findClass(env, "java/lang/reflect/Constructor");
    deflaterClass = findClass(env, "java/util/zip/Deflater");
    doubleClass = findClass(env, "java/lang/Double");
    errnoExceptionClass = findClass(env, "info/guardianproject/libcore/io/ErrnoException");
    fieldClass = findClass(env, "java/lang/reflect/Field");
    fileDescriptorClass = findClass(env, "info/guardianproject/iocipher/FileDescriptor");
    inflaterClass = findClass(env, "java/util/zip/Inflater");
    integerClass = findClass(env, "java/lang/Integer");
    longClass = findClass(env, "java/lang/Long");
    methodClass = findClass(env, "java/lang/reflect/Method");
    parsePositionClass = findClass(env, "java/text/ParsePosition");
    patternSyntaxExceptionClass = findClass(env, "java/util/regex/PatternSyntaxException");
    stringArrayClass = findClass(env, "[Ljava/lang/String;");
    stringClass = findClass(env, "java/lang/String");
    structAddrinfoClass = findClass(env, "info/guardianproject/libcore/io/StructAddrinfo");
    structFlockClass = findClass(env, "info/guardianproject/libcore/io/StructFlock");
    structGroupReqClass = findClass(env, "info/guardianproject/libcore/io/StructGroupReq");
    structLingerClass = findClass(env, "info/guardianproject/libcore/io/StructLinger");
    structPasswdClass = findClass(env, "info/guardianproject/libcore/io/StructPasswd");
    structPollfdClass = findClass(env, "info/guardianproject/libcore/io/StructPollfd");
    structStatClass = findClass(env, "info/guardianproject/libcore/io/StructStat");
    structStatFsClass = findClass(env, "info/guardianproject/libcore/io/StructStatFs");
    structTimevalClass = findClass(env, "info/guardianproject/libcore/io/StructTimeval");
    structUtsnameClass = findClass(env, "info/guardianproject/libcore/io/StructUtsname");
}
