#pragma once

#ifdef ANDROID
#include <jni.h>

JNIEnv * Android_JNI_GetEnv(void);

extern jobject g_Activity;
extern jobject g_GLThread;

#endif
