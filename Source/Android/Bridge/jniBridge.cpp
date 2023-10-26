#include <stdlib.h>
#ifdef ANDROID
#include <jni.h>
#endif
#include "JavaBridge.h"
#include "JavaRomList.h"
#include "Notification.h"
#include "SyncBridge.h"
#include "UISettings.h"
#include "jniBridge.h"
#include "jniBridgeSettings.h"
#include <Common/StdString.h>
#include <Common/Thread.h>
#include <Common/Trace.h>
#include <Project64-core/AppInit.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Settings.h>
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
#include <Project64-core/TraceModulesProject64.h>
#include <Project64-core/Version.h>

#ifdef _WIN32
#define EXPORT extern "C" __declspec(dllexport)
#define CALL __cdecl
#else
#define EXPORT extern "C" __attribute__((visibility("default")))
#define CALL
#endif

#ifdef ANDROID
#include <android/log.h>

class AndroidLogger : public CTraceModule
{
    void Write(uint32_t module, uint8_t severity, const char * file, int line, const char * function, const char * Message)
    {
        switch (severity)
        {
        case TraceError: __android_log_print(ANDROID_LOG_ERROR, TraceModule(module), "%05d: %s: %s", CThread::GetCurrentThreadId(), function, Message); break;
        case TraceWarning: __android_log_print(ANDROID_LOG_WARN, TraceModule(module), "%05d: %s: %s", CThread::GetCurrentThreadId(), function, Message); break;
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

std::unique_ptr<CJniBridegSettings> JniBridegSettings;
CJavaRomList * g_JavaRomList = NULL;
AndroidLogger * g_Logger = NULL;
static pthread_key_t g_ThreadKey;
static JavaVM * g_JavaVM = NULL;
JavaBridge * g_JavaBridge = NULL;
SyncBridge * g_SyncBridge = NULL;
jobject g_Activity = NULL;
jobject g_GLThread = NULL;

static void Android_JNI_ThreadDestroyed(void *);
static void Android_JNI_SetupThread(void);

EXPORT jint CALL JNI_OnLoad(JavaVM * vm, void * reserved)
{
    __android_log_print(ANDROID_LOG_INFO, "jniBridge", "JNI_OnLoad called");
    g_JavaVM = vm;
    JNIEnv * env;
    if (g_JavaVM->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_ERROR, "jniBridge", "Failed to get the environment using GetEnv()");
        return -1;
    }

    // Create mThreadKey so we can keep track of the JNIEnv assigned to each thread
    // Refer to http://developer.android.com/guide/practices/design/jni.html for the rationale behind this

    if (pthread_key_create(&g_ThreadKey, Android_JNI_ThreadDestroyed) != 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "jniBridge", "Error initializing pthread key");
    }
    Android_JNI_SetupThread();

    return JNI_VERSION_1_4;
}

void AddRecentRom(const char * ImagePath)
{
    if (ImagePath == NULL)
    {
        return;
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Start (ImagePath: %s)", ImagePath);

    // Get information about the stored ROM list
    size_t MaxRememberedFiles = g_Settings->LoadDword((SettingID)FileRecentGameFileCount);
    strlist RecentGames;
    size_t i;
    for (i = 0; i < MaxRememberedFiles; i++)
    {
        stdstr RecentGame = g_Settings->LoadStringIndex((SettingID)FileRecentGameFileIndex, i);
        if (RecentGame.empty())
        {
            break;
        }
        RecentGames.push_back(RecentGame);
    }

    // See if the game is already in the list, and if so then move it to the top of the list
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
        g_Settings->SaveStringIndex((SettingID)FileRecentGameFileIndex, i, *iter);
    }

    if (g_JavaBridge)
    {
        WriteTrace(TraceUserInterface, TraceDebug, "Calling RecentRomsUpdated");
        g_JavaBridge->RecentRomsUpdated();
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

void GameCpuRunning(void * /*NotUsed*/)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    bool Running = g_Settings->LoadBool(GameRunning_CPU_Running);
    WriteTrace(TraceUserInterface, TraceDebug, Running ? "Game started" : "Game stopped");
    JNIEnv * env = Android_JNI_GetEnv();
    if (Running)
    {
        stdstr FileLoc = g_Settings->LoadStringVal(Game_File);
        if (FileLoc.length() > 0)
        {
            AddRecentRom(FileLoc.c_str());
        }
        g_System->RefreshGameSettings();

        //int RunCount = UISettingsLoadDword(Game_RunCount);
        //WriteTrace(TraceUserInterface, TraceDebug, "Setting run count to %d", RunCount + 1);
        //UISettingsSaveDword(Game_RunCount, RunCount + 1);
        if (env != NULL)
        {
            if (g_JavaBridge)
            {
                WriteTrace(TraceUserInterface, TraceDebug, "Notify Java emulation stopped");
                g_JavaBridge->EmulationStarted();
            }
            else
            {
                WriteTrace(TraceUserInterface, TraceError, "No Java bridge");
            }
        }
        else
        {
            WriteTrace(TraceUserInterface, TraceError, "Failed to get Java environment");
        }
    }
    else
    {
        if (env != NULL)
        {
            if (g_JavaBridge)
            {
                WriteTrace(TraceUserInterface, TraceDebug, "Notify Java emulation stopped");
                g_JavaBridge->EmulationStopped();
            }
            else
            {
                WriteTrace(TraceUserInterface, TraceError, "No Java bridge");
            }

            // Call in to java that emulation done
            WriteTrace(TraceUserInterface, TraceDebug, "Clean up global activity");
            env->DeleteGlobalRef(g_Activity);
            g_Activity = NULL;

            WriteTrace(TraceUserInterface, TraceDebug, "Clean up global GL thread");
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

EXPORT jboolean CALL Java_emu_project64_jni_NativeExports_appInit(JNIEnv * env, jclass cls, jstring BaseDir)
{
    __android_log_print(ANDROID_LOG_INFO, "Project64", "    ____               _           __  _____ __ __");
    __android_log_print(ANDROID_LOG_INFO, "Project64", "   / __ \\_________    (_)__  _____/ /_/ ___// // /");
    __android_log_print(ANDROID_LOG_INFO, "Project64", "  / /_/ / ___/ __ \\  / / _ \\/ ___/ __/ __ \\/ // /_");
    __android_log_print(ANDROID_LOG_INFO, "Project64", " / ____/ /  / /_/ / / /  __/ /__/ /_/ /_/ /__  __/");
    __android_log_print(ANDROID_LOG_INFO, "Project64", "/_/   /_/   \\____/_/ /\\___/\\___/\\__/\\____/  /_/");
    __android_log_print(ANDROID_LOG_INFO, "Project64", "                /___/");
    __android_log_print(ANDROID_LOG_INFO, "Project64", "https://www.pj64-emu.com/");
    __android_log_print(ANDROID_LOG_INFO, "Project64", "%s", stdstr_f("%s Version %s", VER_FILE_DESCRIPTION_STR, VER_FILE_VERSION_STR).c_str());

    if (g_Logger == NULL)
    {
        g_Logger = new AndroidLogger();
    }
    TraceAddModule(g_Logger);

    if (g_JavaVM == NULL)
    {
        Notify().DisplayError("No Java VM");
        return false;
    }

    const char * baseDir = env->GetStringUTFChars(BaseDir, 0);
    bool res = AppInit(&Notify(), baseDir, 0, NULL);
    __android_log_print(ANDROID_LOG_INFO, "Project64", "baseDir: %s", baseDir);
    env->ReleaseStringUTFChars(BaseDir, baseDir);
    if (res)
    {
        g_JavaBridge = new JavaBridge(g_JavaVM);
        g_SyncBridge = new SyncBridge(g_JavaBridge);
        g_Plugins->SetRenderWindows(g_JavaBridge, g_SyncBridge);

        JniBridegSettings.reset(new CJniBridegSettings());

        RegisterUISettings();
        g_Settings->RegisterChangeCB(GameRunning_CPU_Running, NULL, (CSettings::SettingChangedFunc)GameCpuRunning);
    }
    else
    {
        AppCleanup();
    }
    __android_log_print(ANDROID_LOG_INFO, "Project64", "");
    return res;
}

EXPORT jstring CALL Java_emu_project64_jni_NativeExports_appVersion(JNIEnv * env, jclass cls)
{
    return env->NewStringUTF(VER_FILE_VERSION_STR);
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_SettingsSaveBool(JNIEnv * env, jclass cls, jstring Type, jboolean Value)
{
    const char * szType = env->GetStringUTFChars(Type, 0);
    WriteTrace(TraceUserInterface, TraceDebug, "Saving %s value: %s", szType, Value ? "true" : "false");
    SettingID Id = JniBridegSettings->TranslateSettingID(szType);
    if (Id != Default_None)
    {
        g_Settings->SaveBool(Id, Value);
        CSettings::FlushSettings(g_Settings);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Saved");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_SettingsSaveDword(JNIEnv * env, jclass cls, jstring Type, int Value)
{
    const char * szType = env->GetStringUTFChars(Type, 0);
    WriteTrace(TraceUserInterface, TraceDebug, "Saving %s value: 0x%X", szType, Value);
    SettingID Id = JniBridegSettings->TranslateSettingID(szType);
    if (Id != Default_None)
    {
        g_Settings->SaveDword(Id, Value);
        CSettings::FlushSettings(g_Settings);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Saved");
    env->ReleaseStringUTFChars(Type, szType);
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_SettingsSaveString(JNIEnv * env, jclass cls, jstring Type, jstring Buffer)
{
    const char * szBuffer = env->GetStringUTFChars(Buffer, 0);
    const char * szType = env->GetStringUTFChars(Type, 0);
    WriteTrace(TraceUserInterface, TraceDebug, "Saving %s value: %s", szType, szBuffer);
    SettingID Id = JniBridegSettings->TranslateSettingID(szType);
    if (Id != Default_None)
    {
        g_Settings->SaveString(Id, szBuffer);
        CSettings::FlushSettings(g_Settings);
    }
    env->ReleaseStringUTFChars(Buffer, szBuffer);
    env->ReleaseStringUTFChars(Type, szType);
    WriteTrace(TraceUserInterface, TraceDebug, "Saved");
}

EXPORT jboolean CALL Java_emu_project64_jni_NativeExports_SettingsLoadBool(JNIEnv * env, jclass cls, jstring Type)
{
    const char * szType = env->GetStringUTFChars(Type, 0);
    SettingID Id = JniBridegSettings->TranslateSettingID(szType);
    env->ReleaseStringUTFChars(Type, szType);
    if (Id != Default_None)
    {
        return g_Settings->LoadBool(Id);
    }
    return false;
}

EXPORT jint CALL Java_emu_project64_jni_NativeExports_SettingsLoadDword(JNIEnv * env, jclass cls, jstring Type)
{
    const char * szType = env->GetStringUTFChars(Type, 0);
    SettingID Id = JniBridegSettings->TranslateSettingID(szType);
    env->ReleaseStringUTFChars(Type, szType);
    if (Id != Default_None)
    {
        return g_Settings->LoadDword(Id);
    }
    return 0;
}

EXPORT jstring CALL Java_emu_project64_jni_NativeExports_SettingsLoadString(JNIEnv * env, jclass cls, jstring Type)
{
    const char * szType = env->GetStringUTFChars(Type, 0);
    SettingID Id = JniBridegSettings->TranslateSettingID(szType);
    env->ReleaseStringUTFChars(Type, szType);
    if (Id != Default_None)
    {
        return env->NewStringUTF(g_Settings->LoadStringVal(Id).c_str());
    }
    return env->NewStringUTF("");
}

EXPORT jstring CALL Java_emu_project64_jni_NativeExports_SettingsLoadStringIndex(JNIEnv * env, jclass cls, jstring Type, int32_t Index)
{
    const char * szType = env->GetStringUTFChars(Type, 0);
    SettingID Id = JniBridegSettings->TranslateSettingID(szType);
    env->ReleaseStringUTFChars(Type, szType);
    if (Id != Default_None)
    {
        return env->NewStringUTF(g_Settings->LoadStringIndex(Id, Index).c_str());
    }
    return env->NewStringUTF("");
}

EXPORT jboolean CALL Java_emu_project64_jni_NativeExports_IsSettingSet(JNIEnv * env, jclass cls, jstring Type)
{
    const char * szType = env->GetStringUTFChars(Type, 0);
    SettingID Id = JniBridegSettings->TranslateSettingID(szType);
    env->ReleaseStringUTFChars(Type, szType);
    if (Id != Default_None)
    {
        return g_Settings->IsSettingSet(Id);
    }
    return false;
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_LoadRomList(JNIEnv * env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (g_JavaRomList == NULL)
    {
        g_JavaRomList = new CJavaRomList;
    }
    g_JavaRomList->LoadRomList();

    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_LoadGame(JNIEnv * env, jclass cls, jstring FileLoc)
{
    const char * fileLoc = env->GetStringUTFChars(FileLoc, 0);
    WriteTrace(TraceUserInterface, TraceDebug, "FileLoc: %s", fileLoc);
    CN64System::LoadFileImage(fileLoc);
    env->ReleaseStringUTFChars(FileLoc, fileLoc);
    WriteTrace(TraceUserInterface, TraceDebug, "Image loaded");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_StartGame(JNIEnv * env, jclass cls, jobject activity, jobject GLThread)
{
    g_Activity = env->NewGlobalRef(activity);
    g_GLThread = env->NewGlobalRef(GLThread);
    CN64System::RunLoadedImage();
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_RefreshRomDir(JNIEnv * env, jclass cls, jstring RomDir, jboolean Recursive)
{
    const char * romDir = env->GetStringUTFChars(RomDir, 0);
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

EXPORT void CALL Java_emu_project64_jni_NativeExports_ExternalEvent(JNIEnv * env, jclass cls, int Type)
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

EXPORT void CALL Java_emu_project64_jni_NativeExports_ResetApplicationSettings(JNIEnv * env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    CSettingTypeApplication::ResetAll();
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT jbyteArray CALL Java_emu_project64_jni_NativeExports_GetString(JNIEnv * env, jclass cls, int StringID)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start (StringID: %d)", StringID);
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

EXPORT void CALL Java_emu_project64_jni_NativeExports_SetSpeed(JNIEnv * env, jclass cls, int Speed)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start (Speed: %d)", Speed);
    if (g_BaseSystem)
    {
        g_BaseSystem->SetSpeed(Speed);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT int CALL Java_emu_project64_jni_NativeExports_GetSpeed(JNIEnv * env, jclass cls)
{
    int speed = 0;
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (g_BaseSystem)
    {
        speed = g_BaseSystem->GetSpeed();
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done (speed: %d)", speed);
    return speed;
}

EXPORT int CALL Java_emu_project64_jni_NativeExports_GetBaseSpeed(JNIEnv * env, jclass cls)
{
    int speed = 0;
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
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

EXPORT void CALL Java_emu_project64_jni_NativeExports_StopEmulation(JNIEnv * env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (g_BaseSystem)
    {
        g_BaseSystem->CloseCpu();
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_StartEmulation(JNIEnv * env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (g_BaseSystem)
    {
        g_BaseSystem->StartEmulation(true);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

EXPORT void CALL Java_emu_project64_jni_NativeExports_CloseSystem(JNIEnv * env, jclass cls)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    g_BaseSystem->EndEmulation();
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

static void Android_JNI_ThreadDestroyed(void * value)
{
    __android_log_print(ANDROID_LOG_ERROR, "Android_JNI_ThreadDestroyed", "start");

    // The thread is being destroyed, detach it from the Java VM and set the mThreadKey value to NULL as required
    JNIEnv * env = (JNIEnv *)value;
    if (env != NULL)
    {
        g_JavaVM->DetachCurrentThread();
        pthread_setspecific(g_ThreadKey, NULL);
    }
    __android_log_print(ANDROID_LOG_ERROR, "Android_JNI_ThreadDestroyed", "Done");
}

JNIEnv * Android_JNI_GetEnv(void)
{
    /*
    * From http://developer.android.com/guide/practices/jni.html
    * All threads are Linux threads, scheduled by the kernel.
    * They're usually started from managed code (using Thread.start), but they can also be created elsewhere and then
    * attached to the Java VM. For example, a thread started with pthread_create can be attached with the
    * JNI AttachCurrentThread or AttachCurrentThreadAsDaemon functions. Until a thread is attached, it has no JNIEnv,
    * and cannot make JNI calls.
    * Attaching a natively-created thread causes a java.lang.Thread object to be constructed and added to the "main"
    * ThreadGroup, making it visible to the debugger. Calling AttachCurrentThread on an already-attached thread
    * is a no-op.
    * Note: You can call this function any number of times for the same thread, there's no harm in it
    */

    JNIEnv * env;
    int status = g_JavaVM->AttachCurrentThread(&env, NULL);
    if (status < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "jniBridge", "Failed to attach current thread");
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
    pthread_setspecific(g_ThreadKey, (void *)env);
    return env;
}

void Android_JNI_SetupThread(void)
{
    Android_JNI_GetEnv();
}

#endif
