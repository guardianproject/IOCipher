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

#define LOG_TAG "ObjectStreamClass"

#include "JNIHelp.h"
#include "JniConstants.h"

static jobject getSignature(JNIEnv* env, jclass c, jobject object) {
    jmethodID mid = env->GetMethodID(c, "getSignature", "()Ljava/lang/String;");
    if (!mid) {
        return NULL;
    }
    jclass objectClass = env->GetObjectClass(object);
    return env->CallNonvirtualObjectMethod(object, objectClass, mid);
}

static jobject info_guardianproject_iocipher_ObjectStreamClass_getFieldSignature(JNIEnv* env, jclass, jobject field) {
    return getSignature(env, JniConstants::fieldClass, field);
}

static jobject info_guardianproject_iocipher_ObjectStreamClass_getMethodSignature(JNIEnv* env, jclass, jobject method) {
    return getSignature(env, JniConstants::methodClass, method);
}

static jobject info_guardianproject_iocipher_ObjectStreamClass_getConstructorSignature(JNIEnv* env, jclass, jobject constructor) {
    return getSignature(env, JniConstants::constructorClass, constructor);
}

static jboolean info_guardianproject_iocipher_ObjectStreamClass_hasClinit(JNIEnv * env, jclass, jclass targetClass) {
    jmethodID mid = env->GetStaticMethodID(targetClass, "<clinit>", "()V");
    env->ExceptionClear();
    return (mid != 0);
}

static jint info_guardianproject_iocipher_ObjectStreamClass_getConstructorId(JNIEnv* env, jclass, jclass constructorClass) {
    return reinterpret_cast<jint>(env->GetMethodID(constructorClass, "<init>", "()V"));
}

static jobject info_guardianproject_iocipher_ObjectStreamClass_newInstance(JNIEnv* env, jclass, jclass instantiationClass, jint methodId) {
    return env->NewObject(instantiationClass, reinterpret_cast<jmethodID>(methodId));
}

static JNINativeMethod sMethods[] = {
    {"getConstructorId", "(Ljava/lang/Class;)I", (void *)info_guardianproject_iocipher_ObjectStreamClass_getConstructorId},
    {"getConstructorSignature", "(Ljava/lang/reflect/Constructor;)Ljava/lang/String;", (void *)info_guardianproject_iocipher_ObjectStreamClass_getConstructorSignature},
    {"getFieldSignature", "(Ljava/lang/reflect/Field;)Ljava/lang/String;", (void *)info_guardianproject_iocipher_ObjectStreamClass_getFieldSignature},
    {"getMethodSignature", "(Ljava/lang/reflect/Method;)Ljava/lang/String;", (void *)info_guardianproject_iocipher_ObjectStreamClass_getMethodSignature},
    {"hasClinit", "(Ljava/lang/Class;)Z", (void *)info_guardianproject_iocipher_ObjectStreamClass_hasClinit},
    {"newInstance", "(Ljava/lang/Class;I)Ljava/lang/Object;", (void *)info_guardianproject_iocipher_ObjectStreamClass_newInstance},
};
int register_info_guardianproject_iocipher_ObjectStreamClass(JNIEnv* env) {
    jclass cls;

    cls = env->FindClass("info/guardianproject/iocipher/ObjectStreamClass");
    if (cls == NULL) {
        LOGE("Can't find info/guardianproject/iocipher/ObjectStreamClass\n");
        return -1;
    }
    return env->RegisterNatives(cls, sMethods, NELEM(sMethods));
}
