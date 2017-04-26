/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#ifdef ANDROID
#include <jni.h>
#include "ScreenResolution.h"

#define EXPORT      extern "C"  __attribute__((visibility("default")))
#define CALL

EXPORT jint CALL Java_emu_project64_jni_NativeVideo_getResolutionCount(JNIEnv* env, jclass cls)
{
    return GetScreenResolutionCount();
}

EXPORT jstring CALL Java_emu_project64_jni_NativeVideo_getResolutionName(JNIEnv* env, jclass cls, int index)
{
    return env->NewStringUTF(GetScreenResolutionName(index));
}

EXPORT jint CALL Java_emu_project64_jni_NativeVideo_GetScreenResWidth(JNIEnv* env, jclass cls, int index)
{
    return GetScreenResWidth(index);
}

EXPORT jint CALL Java_emu_project64_jni_NativeVideo_GetScreenResHeight(JNIEnv* env, jclass cls, int index)
{
    return GetScreenResHeight(index);
}
#endif