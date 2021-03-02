#ifdef ANDROID
#include <jni.h>
#include "ScreenResolution.h"

#define EXPORT      extern "C"  __attribute__((visibility("default")))
#define CALL

EXPORT void CALL Java_emu_project64_jni_NativeVideo_UpdateScreenRes(JNIEnv* env, jclass cls, int ScreenWidth, int ScreenHeight)
{
    UpdateScreenResolution(ScreenWidth, ScreenHeight);
}

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