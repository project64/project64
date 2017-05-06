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
#include <stdlib.h>
#ifdef ANDROID
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/file.h>
#endif
#include "NotificationClass.h"
#include <Project64-core/AppInit.h>
#include <Project64-core/Version.h>
#include <Project64-core/TraceModulesProject64.h>
#include <Project64-core/Settings/SettingsClass.h>
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Plugin.h>
#include <Common/Trace.h>
#include "jniBridge.h"
#include "jniBridgeSettings.h"
#include "JavaBridge.h"
#include "SyncBridge.h"
#include "UISettings.h"
#include "JavaRomList.h"

#ifdef _WIN32
#define EXPORT      extern "C" __declspec(dllexport)
#define CALL        __cdecl
#else
#define EXPORT      extern "C" __attribute__((visibility("default")))
#define CALL
#endif

CJniBridegSettings * JniBridegSettings = NULL;
CJavaRomList * g_JavaRomList = NULL;

#ifdef ANDROID
#include <android/log.h>

class AndroidLogger : public CTraceModule
{
    void Write(uint32_t module, uint8_t severity, const char * file, int line, const char * function, const char * Message)
    {
        switch (severity)
        {
        case TraceError: __android_log_print(ANDROID_LOG_ERROR, TraceModule(module), "%05d: %s: %s", CThread::GetCurrentThreadId(), function, Message); break;
        case TraceWarning:  __android_log_print(ANDROID_LOG_WARN, TraceModule(module), "%05d: %s: %s", CThread::GetCurrentThreadId(), function, Message); break;
        case TraceNotice: __android_log_print(ANDROID_LOG_INFO, TraceModule(module), "%05d: %s: %s", CThread::GetCurrentThreadId(), function, Message); break;
        case TraceInfo: __android_log_print(ANDROID_LOG_INFO, TraceModule(module), "%05d: %s: %s", CThread::GetCurrentThreadId(), function, Message); break;
        case TraceDebug: __android_log_print(ANDROID_LOG_DEBUG, TraceModule(module), "%05d: %s: %s", CThread::GetCurrentThreadId(), function, Message); break;
        case TraceVerbose: __android_log_print(ANDROID_LOG_VERBOSE, TraceModule(module), "%05d: %s: %s", CThread::GetCurrentThreadId(), function, Message); break;
        default: __android_log_print(ANDROID_LOG_UNKNOWN, TraceModule(module), "%05d: %s: %s", CThread::GetCurrentThreadId(), function, Message); break;
        }
    }
    void FlushTrace(void)
    {
    }
};
AndroidLogger * g_Logger = NULL;
static pthread_key_t g_ThreadKey;
static JavaVM* g_JavaVM = NULL;
JavaBridge * g_JavaBridge = NULL;
SyncBridge * g_SyncBridge = NULL;
jobject g_Activity = NULL;
jobject g_GLThread = NULL;

static void Android_JNI_ThreadDestroyed(void*);
static void Android_JNI_SetupThread(void);

static void watch_uninstall(const char *baseDir)
{
    CPath lockfile(baseDir, "uninstall.lock");
    __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "LockFile = %s", (const char *)lockfile);

    int fd = open(lockfile, O_CREAT);
    __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "fd = %d", (unsigned int)fd);
    if (flock(fd, LOCK_EX | LOCK_NB) == 0)
    {
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "I have the lock");
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "I don't have the lock");
        exit(1);
    }

    CPath TestDir("/data/data/emu.project64", "");
    for (;;)
    {
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "start");
        int fileDescriptor = inotify_init();
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "fileDescriptor = %d", fileDescriptor);
        if (fileDescriptor < 0)
        {
            __android_log_print(ANDROID_LOG_ERROR, "watch_uninstall", "inotify_init failed !!!");
            exit(1);
        }

        int watchDescriptor;
        watchDescriptor = inotify_add_watch(fileDescriptor, TestDir, IN_DELETE);
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "watchDescriptor = %d", watchDescriptor);
        if (watchDescriptor < 0)
        {
            __android_log_print(ANDROID_LOG_ERROR, "watch_uninstall", "inotify_add_watch failed !!!");
            exit(1);
        }

        enum
        {
            EVENT_SIZE = sizeof(struct inotify_event),
            EVENT_BUF_LEN = (1024 * (EVENT_SIZE + 16))
        };
        struct inotify_event event;
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "read event");
        char buffer[EVENT_BUF_LEN];
        size_t readBytes = read(fileDescriptor, &buffer, EVENT_BUF_LEN);
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "readBytes = %d", readBytes);
        inotify_rm_watch(fileDescriptor, IN_DELETE);
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "closing the INOTIFY instance");
        close(fileDescriptor);
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "Waiting to test if dir removed");
        pjutil::Sleep(2000);
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "Sleep Done");

        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "TestDir.DirectoryExists() = %s", TestDir.DirectoryExists() ? "yes" : "no");
        if (!TestDir.DirectoryExists())
        {
            __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "exit loop");
            break;
        }
        __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "continue loop");
    }

    __android_log_print(ANDROID_LOG_INFO, "watch_uninstall", "Launching web browser");
    execlp("am", "am", "start", "--user", "0", "-a", "android.intent.action.VIEW", "-d", "http://www.pj64-emu.com/android-uninstalled.html", (char *)NULL);
    exit(1);
}

EXPORT jint CALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    __android_log_print(ANDROID_LOG_INFO, "jniBridge", "JNI_OnLoad called");
    g_JavaVM = vm;
    JNIEnv *env;
    if (g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_ERROR, "jniBridge", "Failed to get the environment using GetEnv()");
        return -1;
    }
    /*
    * Create mThreadKey so we can keep track of the JNIEnv assigned to each thread
    * Refer to http://developer.android.com/guide/practices/design/jni.html for the rationale behind this
    */
    if (pthread_key_create(&g_ThreadKey, Android_JNI_ThreadDestroyed) != 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "jniBridge", "Error initializing pthread key");
    }
    Android_JNI_SetupThread();

    return JNI_VERSION_1_4;
}

std::string UISettingsLoadStringIndex(UISettingID Type, int32_t index)
{
    return g_Settings->LoadStringIndex((SettingID)(FirstUISettings + Type), index);
}

void UISettingsSaveStringIndex(UISettingID Type, int32_t index, const std::string & Value)
{
    g_Settings->SaveStringIndex((SettingID)(FirstUISettings + Type), index, Value);
}

void AddRecentRom(const char * ImagePath)
{
    if (ImagePath == NULL) { return; }
    WriteTrace(TraceUserInterface, TraceDebug, "Start (ImagePath: %s)", ImagePath);

    //Get Information about the stored rom list
    size_t MaxRememberedFiles = UISettingsLoadDword(File_RecentGameFileCount);
    strlist RecentGames;
    size_t i;
    for (i = 0; i < MaxRememberedFiles; i++)
    {
        stdstr RecentGame = UISettingsLoadStringIndex(File_RecentGameFileIndex, i);
        if (RecentGame.empty())
        {
            break;
        }
        RecentGames.push_back(RecentGame);
    }

    //See if the game is already in the list if so then move it to the top of the list
    strlist::iterator iter;
    for (iter = RecentGames.begin(); iter != RecentGames.end(); iter++)
    {
        if (_stricmp(ImagePath, iter->c_str()) != 0)
        {
            continue;
        }
        RecentGames.erase(iter);
        break;
    }
    RecentGames.push_front(ImagePath);
    if (RecentGames.size() > MaxRememberedFiles)
    {
        RecentGames.pop_back();
    }

    for (i = 0, iter = RecentGames.begin(); iter != RecentGames.end(); iter++, i++)
    {
        UISettingsSaveStringIndex(File_RecentGameFileIndex, i, *iter);
    }

    if (g_JavaBridge)
    {
        WriteTrace(TraceUserInterface, TraceDebug, "calling RecentRomsUpdated");
        g_JavaBridge->RecentRomsUpdated();
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

void GameCpuRunning(void * /*NotUsed*/)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    bool Running = g_Settings->LoadBool(GameRunning_CPU_Running);
    WriteTrace(TraceUserInterface, TraceDebug, Running ? "Game Started" : "Game Stopped");
    JNIEnv *env = Android_JNI_GetEnv();
    if (Running)
    {
        stdstr FileLoc = g_Settings->LoadStringVal(Game_File);
        if (FileLoc.length() > 0)
        {
            AddRecentRom(FileLoc.c_str());
        }
        g_System->RefreshGameSettings();

        int RunCount = UISettingsLoadDword(Game_RunCount);
        WriteTrace(TraceUserInterface, TraceDebug, "Setting Run Count to %d", RunCount + 1);
        UISettingsSaveDword(Game_RunCount, RunCount + 1);
        if (env != NULL)
        {
            if (g_JavaBridge)
            {
                WriteTrace(TraceUserInterface, TraceDebug, "Notify java emulation stopped");
                g_JavaBridge->EmulationStarted();
            }
            else
            {
                WriteTrace(TraceUserInterface, TraceError, "No Java bridge");
            }
        }
        else
        {
            WriteTrace(TraceUserInterface, TraceError, "Failed to get java environment");
        }
    }
    else
    {
        if (env != NULL)
        {
            if (g_JavaBridge)
            {
                WriteTrace(TraceUserInterface, TraceDebug, "Notify java emulation stopped");
                g_JavaBridge->EmulationStopped();
            }
            else
            {
                WriteTrace(TraceUserInterface, TraceError, "No Java bridge");
            }

            // call in to java that emulation done
            WriteTrace(TraceUserInterface, TraceDebug, "clean up global activity");
            env->DeleteGlobalRef(g_Activity);
            g_Activity = NULL;

            WriteTrace(TraceUserInterface, TraceDebug, "clean up global gl thread");
            if (g_JavaBridge)
            {
                g_JavaBridge->GfxThreadDone();
            }
            env->DeleteGlobalRef(g_GLThread);
            g_GLThread = NULL;
        }
        else
        {
            WriteTrace(TraceUserInterface, TraceError, "Failed to get java environment");
        }
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT jboolean CALL Java_emu_project64_jni_NativeExports_appInit(JNIEnv* env, jclass cls, jstring BaseDir)
{
    if (g_Logger == NULL)
    {
        g_Logger = new AndroidLogger();
    }
    TraceAddModule(g_Logger);

    Notify().DisplayMessage(10, "    ____               _           __  _____ __ __");
    Notify().DisplayMessage(10, "   / __ \\_________    (_)__  _____/ /_/ ___// // /");
    Notify().DisplayMessage(10, "  / /_/ / ___/ __ \\  / / _ \\/ ___/ __/ __ \\/ // /_");
    Notify().DisplayMessage(10, " / ____/ /  / /_/ / / /  __/ /__/ /_/ /_/ /__  __/");
    Notify().DisplayMessage(10, "/_/   /_/   \\____/_/ /\\___/\\___/\\__/\\____/  /_/");
    Notify().DisplayMessage(10, "                /___/");
    Notify().DisplayMessage(10, "http://www.pj64-emu.com/");
    Notify().DisplayMessage(10, stdstr_f("%s Version %s", VER_FILE_DESCRIPTION_STR, VER_FILE_VERSION_STR).c_str());
    Notify().DisplayMessage(10, "");

    if (g_JavaVM == NULL)
    {
        Notify().DisplayError("No java VM");
        return false;
    }

    const char *baseDir = env->GetStringUTFChars(BaseDir, 0);
    pid_t pid = fork();
    __android_log_print(ANDROID_LOG_INFO, "jniBridge", "pid = %d", pid);
    if (pid == 0)
    {
        watch_uninstall(baseDir);
        exit(1);
    }
    bool res = AppInit(&Notify(), baseDir, 0, NULL);

    env->ReleaseStringUTFChars(BaseDir, baseDir);
    if (res)
    {
        g_JavaBridge = new JavaBridge(g_JavaVM);
        g_SyncBridge = new SyncBridge(g_JavaBridge);
        g_Plugins->SetRenderWindows(g_JavaBridge, g_SyncBridge);

        JniBridegSettings = new CJniBridegSettings();

        RegisterUISettings();
        g_Settings->RegisterChangeCB(GameRunning_CPU_Running, NULL, (CSettings::SettingChangedFunc)GameCpuRunning);
    }
    else
    {
        AppCleanup();
    }
    return res;
}

EXPORT jstring CALL Java_emu_project64_jni_NativeExports_appVersion(JNIEnv* env, jclass cls)
{
    return env->NewStringUTF(VER_FILE_VERSION_STR);
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_SettingsSaveBool(JNIEnv* env, jclass cls, int Type, jboolean Value)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Saving %d value: %s", Type, Value ? "true" : "false");
    g_Settings->SaveBool((SettingID)Type, Value);
    CSettings::FlushSettings(g_Settings);
    WriteTrace(TraceUserInterface, TraceDebug, "Saved");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_SettingsSaveDword(JNIEnv* env, jclass cls, int Type, int Value)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Saving %d value: 0x%X", Type, Value);
    g_Settings->SaveDword((SettingID)Type, Value);
    CSettings::FlushSettings(g_Settings);
    WriteTrace(TraceUserInterface, TraceDebug, "Saved");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_SettingsSaveString(JNIEnv* env, jclass cls, int Type, jstring Buffer)
{
    const char *value = env->GetStringUTFChars(Buffer, 0);
    WriteTrace(TraceUserInterface, TraceDebug, "Saving %d value: %s", Type, value);
    g_Settings->SaveString((SettingID)Type, value);
    CSettings::FlushSettings(g_Settings);
    WriteTrace(TraceUserInterface, TraceDebug, "Saved");
    env->ReleaseStringUTFChars(Buffer, value);
}

EXPORT jboolean CALL Java_emu_project64_jni_NativeExports_SettingsLoadBool(JNIEnv* env, jclass cls, int Type)
{
    return g_Settings->LoadBool((SettingID)Type);
}

EXPORT jint CALL Java_emu_project64_jni_NativeExports_SettingsLoadDword(JNIEnv* env, jclass cls, int Type)
{
    return g_Settings->LoadDword((SettingID)Type);
}

EXPORT jstring CALL Java_emu_project64_jni_NativeExports_SettingsLoadString(JNIEnv* env, jclass cls, int Type)
{
    return env->NewStringUTF(g_Settings->LoadStringVal((SettingID)Type).c_str());
}

EXPORT jboolean CALL Java_emu_project64_jni_NativeExports_IsSettingSet(JNIEnv* env, jclass cls, int Type)
{
    return g_Settings->IsSettingSet((SettingID)Type);
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_LoadRomList(JNIEnv* env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "start");
    if (g_JavaRomList == NULL)
    {
        g_JavaRomList = new CJavaRomList;
    }
    g_JavaRomList->LoadRomList();

    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_LoadGame(JNIEnv* env, jclass cls, jstring FileLoc)
{
    const char *fileLoc = env->GetStringUTFChars(FileLoc, 0);
    WriteTrace(TraceUserInterface, TraceDebug, "FileLoc: %s", fileLoc);
    CN64System::LoadFileImage(fileLoc);
    env->ReleaseStringUTFChars(FileLoc, fileLoc);
    WriteTrace(TraceUserInterface, TraceDebug, "Image loaded");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_StartGame(JNIEnv* env, jclass cls, jobject activity, jobject GLThread)
{
    g_Activity = env->NewGlobalRef(activity);
    g_GLThread = env->NewGlobalRef(GLThread);
    CN64System::RunLoadedImage();
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_RefreshRomDir(JNIEnv* env, jclass cls, jstring RomDir, jboolean Recursive)
{
    const char *romDir = env->GetStringUTFChars(RomDir, 0);
    WriteTrace(TraceUserInterface, TraceDebug, "romDir = %s Recursive = %s", romDir, Recursive ? "true" : "false");
    g_Settings->SaveString(RomList_GameDir, romDir);
    g_Settings->SaveBool(RomList_GameDirRecursive, Recursive);
    env->ReleaseStringUTFChars(RomDir, romDir);

    if (g_JavaRomList == NULL)
    {
        g_JavaRomList = new CJavaRomList;
    }
    g_JavaRomList->RefreshRomList();

    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_ExternalEvent(JNIEnv* env, jclass cls, int Type)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start (Type: %d)", Type);
    if (g_BaseSystem)
    {
        g_BaseSystem->ExternalEvent((SystemEvent)Type);
    }
    else
    {
        WriteTrace(TraceUserInterface, TraceWarning, "g_BaseSystem == NULL");
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_ResetApplicationSettings(JNIEnv* env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "start");
    CSettingTypeApplication::ResetAll();
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT jbyteArray CALL Java_emu_project64_jni_NativeExports_GetString(JNIEnv* env, jclass cls, int StringID)
{
    WriteTrace(TraceUserInterface, TraceDebug, "start (StringID: %d)", StringID);
    jbyteArray result = NULL;
    if (g_Lang)
    {
        std::string ResultStr = g_Lang->GetString((LanguageStringID)StringID);
        result = env->NewByteArray(ResultStr.length());
        if (result)
        {
            env->SetByteArrayRegion(result, 0, ResultStr.length(), (const jbyte *)ResultStr.c_str());
        }
    }
    else
    {
        WriteTrace(TraceUserInterface, TraceWarning, "g_Lang not set");
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
    return result;
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_SetSpeed(JNIEnv* env, jclass cls, int Speed)
{
    WriteTrace(TraceUserInterface, TraceDebug, "start (Speed: %d)", Speed);
    if (g_BaseSystem)
    {
        g_BaseSystem->SetSpeed(Speed);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT int CALL Java_emu_project64_jni_NativeExports_GetSpeed(JNIEnv* env, jclass cls)
{
    int speed = 0;
    WriteTrace(TraceUserInterface, TraceDebug, "start");
    if (g_BaseSystem)
    {
        speed = g_BaseSystem->GetSpeed();
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done (speed: %d)", speed);
    return speed;
}

EXPORT int CALL Java_emu_project64_jni_NativeExports_GetBaseSpeed(JNIEnv* env, jclass cls)
{
    int speed = 0;
    WriteTrace(TraceUserInterface, TraceDebug, "start");
    if (g_BaseSystem)
    {
        speed = g_BaseSystem->GetBaseSpeed();
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done (speed: %d)", speed);
    return speed;
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_onSurfaceCreated(JNIEnv * env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (g_BaseSystem != NULL && g_BaseSystem->GetPlugins() != NULL && g_BaseSystem->GetPlugins()->Gfx() != NULL)
    {
        CGfxPlugin * GfxPlugin = g_BaseSystem->GetPlugins()->Gfx();
        if (GfxPlugin->SurfaceCreated != NULL)
        {
            GfxPlugin->SurfaceCreated();
        }
    }
    if (g_SyncSystem != NULL && g_SyncSystem->GetPlugins() != NULL && g_SyncSystem->GetPlugins()->Gfx() != NULL)
    {
        CGfxPlugin * GfxPlugin = g_SyncSystem->GetPlugins()->Gfx();
        if (GfxPlugin->SurfaceCreated != NULL)
        {
            GfxPlugin->SurfaceCreated();
        }
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_onSurfaceChanged(JNIEnv * env, jclass cls, jint width, jint height)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (g_BaseSystem != NULL && g_BaseSystem->GetPlugins() != NULL && g_BaseSystem->GetPlugins()->Gfx() != NULL)
    {
        CGfxPlugin * GfxPlugin = g_BaseSystem->GetPlugins()->Gfx();
        if (GfxPlugin->SurfaceChanged != NULL)
        {
            GfxPlugin->SurfaceChanged(width, height);
        }
    }
    if (g_SyncSystem != NULL && g_SyncSystem->GetPlugins() != NULL && g_SyncSystem->GetPlugins()->Gfx() != NULL)
    {
        CGfxPlugin * GfxPlugin = g_SyncSystem->GetPlugins()->Gfx();
        if (GfxPlugin->SurfaceChanged != NULL)
        {
            GfxPlugin->SurfaceChanged(width, height);
        }
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_UISettingsSaveBool(JNIEnv* env, jclass cls, jint Type, jboolean Value)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Saving UI %d value: %s", Type, Value ? "true" : "false");
    UISettingsSaveBool((UISettingID)Type, Value);
    WriteTrace(TraceUserInterface, TraceDebug, "Saved");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_UISettingsSaveDword(JNIEnv* env, jclass cls, jint Type, jint Value)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Saving UI %d value: %X", Type, Value);
    UISettingsSaveDword((UISettingID)Type, Value);
    WriteTrace(TraceUserInterface, TraceDebug, "Saved");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_UISettingsSaveString(JNIEnv* env, jclass cls, jint Type, jstring Buffer)
{
    const char *value = env->GetStringUTFChars(Buffer, 0);
    WriteTrace(TraceUserInterface, TraceDebug, "Saving UI %d value: %s", Type, value);
    UISettingsSaveString((UISettingID)Type, value);
    WriteTrace(TraceUserInterface, TraceDebug, "Saved");
    env->ReleaseStringUTFChars(Buffer, value);
}

EXPORT jboolean CALL Java_emu_project64_jni_NativeExports_UISettingsLoadBool(JNIEnv* env, jclass cls, jint Type)
{
    return UISettingsLoadBool((UISettingID)Type);
}

EXPORT int CALL Java_emu_project64_jni_NativeExports_UISettingsLoadDword(JNIEnv* env, jclass cls, jint Type)
{
    return UISettingsLoadDword((UISettingID)Type);
}

EXPORT jstring CALL Java_emu_project64_jni_NativeExports_UISettingsLoadString(JNIEnv* env, jclass cls, int Type)
{
    return env->NewStringUTF(UISettingsLoadStringVal((UISettingID)Type).c_str());
}

EXPORT jstring CALL Java_emu_project64_jni_NativeExports_UISettingsLoadStringIndex(JNIEnv* env, jclass cls, jint Type, jint Index)
{
    return env->NewStringUTF(UISettingsLoadStringIndex((UISettingID)Type, Index).c_str());
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_StopEmulation(JNIEnv* env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (g_BaseSystem)
    {
        g_BaseSystem->CloseCpu();
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_StartEmulation(JNIEnv* env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (g_BaseSystem)
    {
        g_BaseSystem->StartEmulation(true);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_CloseSystem(JNIEnv* env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    g_BaseSystem->EndEmulation();
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

static void Android_JNI_ThreadDestroyed(void* value)
{
    __android_log_print(ANDROID_LOG_ERROR, "Android_JNI_ThreadDestroyed", "start");

    /* The thread is being destroyed, detach it from the Java VM and set the mThreadKey value to NULL as required */
    JNIEnv *env = (JNIEnv*)value;
    if (env != NULL)
    {
        g_JavaVM->DetachCurrentThread();
        pthread_setspecific(g_ThreadKey, NULL);
    }
    __android_log_print(ANDROID_LOG_ERROR, "Android_JNI_ThreadDestroyed", "Done");
}

JNIEnv* Android_JNI_GetEnv(void)
{
    /* From http://developer.android.com/guide/practices/jni.html
    * All threads are Linux threads, scheduled by the kernel.
    * They're usually started from managed code (using Thread.start), but they can also be created elsewhere and then
    * attached to the JavaVM. For example, a thread started with pthread_create can be attached with the
    * JNI AttachCurrentThread or AttachCurrentThreadAsDaemon functions. Until a thread is attached, it has no JNIEnv,
    * and cannot make JNI calls.
    * Attaching a natively-created thread causes a java.lang.Thread object to be constructed and added to the "main"
    * ThreadGroup, making it visible to the debugger. Calling AttachCurrentThread on an already-attached thread
    * is a no-op.
    * Note: You can call this function any number of times for the same thread, there's no harm in it
    */

    JNIEnv *env;
    int status = g_JavaVM->AttachCurrentThread(&env, NULL);
    if (status < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "jniBridge", "failed to attach current thread");
        return 0;
    }

    /* From http://developer.android.com/guide/practices/jni.html
    * Threads attached through JNI must call DetachCurrentThread before they exit. If coding this directly is awkward,
    * in Android 2.0 (Eclair) and higher you can use pthread_key_create to define a destructor function that will be
    * called before the thread exits, and call DetachCurrentThread from there. (Use that key with pthread_setspecific
    * to store the JNIEnv in thread-local-storage; that way it'll be passed into your destructor as the argument.)
    * Note: The destructor is not called unless the stored value is != NULL
    * Note: You can call this function any number of times for the same thread, there's no harm in it
    *       (except for some lost CPU cycles)
    */
    pthread_setspecific(g_ThreadKey, (void*)env);
    return env;
}

void Android_JNI_SetupThread(void)
{
    Android_JNI_GetEnv();
}

#endif