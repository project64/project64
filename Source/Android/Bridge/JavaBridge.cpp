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
#include <Project64-core/TraceModulesProject64.h>
#include <Common/Trace.h>
#include "JavaBridge.h"
#include "jniBridge.h"

#ifdef ANDROID
JavaBridge::JavaBridge(JavaVM* vm) :
m_vm(vm)
{
    JNIEnv *env = Android_JNI_GetEnv();
    jclass GalleryActivityClass = env->FindClass("emu/project64/GalleryActivity");
    if (GalleryActivityClass != NULL)
    {
        m_GalleryActivityClass = (jclass)env->NewGlobalRef(GalleryActivityClass);
    }
    jclass NotifierClass = env->FindClass("emu/project64/util/Notifier");
    if (NotifierClass != NULL)
    {
        m_NotifierClass = (jclass)env->NewGlobalRef(NotifierClass);
    }
}

void JavaBridge::GfxThreadInit()
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    JNIEnv *env = Android_JNI_GetEnv();
    if (g_GLThread != NULL && env != NULL)
    {
        jclass GLThreadClass = env->GetObjectClass(g_GLThread);
        jmethodID midThreadStarting = env->GetMethodID(GLThreadClass, "ThreadStarting", "()V");
        env->CallVoidMethod(g_GLThread, midThreadStarting);
        env->DeleteLocalRef(GLThreadClass);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

void JavaBridge::GfxThreadDone()
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    JNIEnv *env = Android_JNI_GetEnv();
    if (g_GLThread != NULL && env != NULL)
    {
        WriteTrace(TraceUserInterface, TraceDebug, "calling java GLThread::ThreadExiting");
        jclass GLThreadClass = env->GetObjectClass(g_GLThread);
        jmethodID midThreadExiting = env->GetMethodID(GLThreadClass, "ThreadExiting", "()V");
        env->CallVoidMethod(g_GLThread, midThreadExiting);
        env->DeleteLocalRef(GLThreadClass);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

void JavaBridge::SwapWindow()
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (g_GLThread != NULL && env != NULL)
    {
        jclass GLThreadClass = env->GetObjectClass(g_GLThread);
        jmethodID midSwapBuffers = env->GetMethodID(GLThreadClass, "SwapBuffers", "()V");
        env->CallVoidMethod(g_GLThread, midSwapBuffers);
        env->DeleteLocalRef(GLThreadClass);
    }
}

void JavaBridge::RomListReset(void)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (env)
    {
        jmethodID midRomListReset = env->GetStaticMethodID(m_GalleryActivityClass, "RomListReset", "()V");
        env->CallStaticVoidMethod(m_GalleryActivityClass, midRomListReset);
    }
}

void JavaBridge::RomListAddItem(const char * FullFileName, const char * FileName, const char * GoodName, uint32_t TextColor)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (env)
    {
        jmethodID midRomListAddItem = env->GetStaticMethodID(m_GalleryActivityClass, "RomListAddItem", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
        if (midRomListAddItem != NULL)
        {
            jstring j_FullFileName = env->NewStringUTF(FullFileName);
            jstring j_FileName = env->NewStringUTF(FileName);
            jstring j_GoodName = env->NewStringUTF(GoodName);
            env->CallStaticVoidMethod(m_GalleryActivityClass, midRomListAddItem, j_FullFileName, j_FileName, j_GoodName, TextColor);
            env->DeleteLocalRef(j_FullFileName);
            env->DeleteLocalRef(j_FileName);
            env->DeleteLocalRef(j_GoodName);
        }
    }
}

void JavaBridge::RomListLoaded(void)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (env)
    {
        jmethodID midRomListLoadDone = env->GetStaticMethodID(m_GalleryActivityClass, "RomListLoadDone", "()V");
        env->CallStaticVoidMethod(m_GalleryActivityClass, midRomListLoadDone);
    }
}

void JavaBridge::DisplayError(const char * Message)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (env)
    {
        jstring j_Message = env->NewStringUTF(Message);
        jmethodID midDisplayError = env->GetStaticMethodID(m_NotifierClass, "DisplayError", "(Landroid/app/Activity;Ljava/lang/String;)V");
        env->CallStaticVoidMethod(m_NotifierClass, midDisplayError,g_Activity,j_Message);
        env->DeleteLocalRef(j_Message);
    }
}

void JavaBridge::DisplayMessage(const char * Message, int DisplayTime)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (env)
    {
        jstring j_Message = env->NewStringUTF(Message);
        jmethodID midshowMessage = env->GetStaticMethodID(m_NotifierClass, "showMessage", "(Landroid/app/Activity;Ljava/lang/String;I)V");
        env->CallStaticVoidMethod(m_NotifierClass, midshowMessage,g_Activity,j_Message,DisplayTime);
        env->DeleteLocalRef(j_Message);
    }
}

void JavaBridge::DisplayMessage2(const char * Message)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (env)
    {
        jstring j_Message = env->NewStringUTF(Message);
        jmethodID midshowMessage2 = env->GetStaticMethodID(m_NotifierClass, "showMessage2", "(Landroid/app/Activity;Ljava/lang/String;)V");
        env->CallStaticVoidMethod(m_NotifierClass, midshowMessage2,g_Activity,j_Message);
        env->DeleteLocalRef(j_Message);
    }
}

void JavaBridge::EmulationStopped(void)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (g_Activity != NULL && env != NULL)
    {
        jmethodID midEmulationStopped = env->GetStaticMethodID(m_NotifierClass, "EmulationStopped", "(Landroid/app/Activity;)V");
        env->CallStaticVoidMethod(m_NotifierClass, midEmulationStopped, g_Activity);
    }
}

void JavaBridge::EmulationStarted(void)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (g_Activity != NULL && env != NULL)
    {
        jmethodID midEmulationStarted = env->GetStaticMethodID(m_NotifierClass, "EmulationStarted", "(Landroid/app/Activity;)V");
        env->CallStaticVoidMethod(m_NotifierClass, midEmulationStarted, g_Activity);
    }
}

void JavaBridge::RecentRomsUpdated(void)
{
    JNIEnv *env = Android_JNI_GetEnv();
    if (env)
    {
        jmethodID midRRecentRomsUpdated = env->GetStaticMethodID(m_GalleryActivityClass, "RecentRomsUpdated", "()V");
        env->CallStaticVoidMethod(m_GalleryActivityClass, midRRecentRomsUpdated);
    }
}

#endif