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

#define LOG_TAG "Console"

#include "JNIHelp.h"
#include "JniConstants.h"

#include <errno.h>
#include <termios.h>
#include <unistd.h>

static jint info_guardianproject_iocipher_Console_setEchoImpl(JNIEnv* env, jclass, jboolean on, jint previousState) {
    termios state;
    if (TEMP_FAILURE_RETRY(tcgetattr(STDIN_FILENO, &state)) == -1) {
        jniThrowIOException(env, errno);
        return 0;
    }
    if (on) {
        state.c_lflag = previousState;
    } else {
        previousState = state.c_lflag;
        state.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    }
    if (TEMP_FAILURE_RETRY(tcsetattr(STDIN_FILENO, TCSAFLUSH, &state)) == -1){
        jniThrowIOException(env, errno);
        return 0;
    }
    return previousState;
}

static JNINativeMethod sMethods[] = {
    {"setEchoImpl", "(ZI)I", (void *)info_guardianproject_iocipher_Console_setEchoImpl},
};
int register_info_guardianproject_iocipher_Console(JNIEnv* env) {
    jclass cls;

    cls = env->FindClass("info/guardianproject/iocipher/Console");
    if (cls == NULL) {
        LOGE("Can't find info/guardianproject/iocipher/Console\n");
        return -1;
    }
    return env->RegisterNatives(cls, sMethods, NELEM(sMethods));
}
