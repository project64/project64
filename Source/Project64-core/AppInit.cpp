#include "stdafx.h"
#include <Common/path.h>
#include <Common/trace.h>
#include <Common/Util.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Plugins/PluginClass.h>
#include <Project64-core/N64System/N64RomClass.h>

void FixDirectories(void);

static void IncreaseThreadPriority(void);
static CTraceFileLog * g_LogFile = NULL;

void LogFlushChanged(CTraceFileLog * LogFile)
{
    LogFile->SetFlushFile(g_Settings->LoadDword(Debugger_AppLogFlush) != 0);
}

void InitializeLog(void)
{
#ifdef _DEBUG
    TraceSetMaxModule(MaxTraceModuleProject64, TraceInfo);
#else
    TraceSetMaxModule(MaxTraceModuleProject64, TraceError);
#endif
}

void AddLogModule(void)
{
    CPath LogFilePath(CPath::MODULE_DIRECTORY);
    LogFilePath.AppendDirectory("Logs");
    if (!LogFilePath.DirectoryExists())
    {
        LogFilePath.DirectoryCreate();
    }
    LogFilePath.SetNameExtension("Project64.log");

    g_LogFile = new CTraceFileLog(LogFilePath, g_Settings->LoadDword(Debugger_AppLogFlush) != 0, Log_New, 500);
    TraceAddModule(g_LogFile);
}

void SetTraceModuleNames(void)
{
    TraceSetModuleName(TraceMD5, "MD5");
    TraceSetModuleName(TraceSettings, "Settings");
    TraceSetModuleName(TraceUnknown, "Unknown");
    TraceSetModuleName(TraceAppInit, "App Init");
    TraceSetModuleName(TraceAppCleanup, "App Cleanup");
    TraceSetModuleName(TraceN64System, "N64 System");
    TraceSetModuleName(TracePlugins, "Plugins");
    TraceSetModuleName(TraceGFXPlugin, "GFX Plugin");
    TraceSetModuleName(TraceAudioPlugin, "Audio Plugin");
    TraceSetModuleName(TraceControllerPlugin, "Controller Plugin");
    TraceSetModuleName(TraceRSPPlugin, "RSP Plugin");
    TraceSetModuleName(TraceRSP, "RSP");
    TraceSetModuleName(TraceAudio, "Audio");
    TraceSetModuleName(TraceRegisterCache, "Register Cache");
    TraceSetModuleName(TraceRecompiler, "Recompiler");
    TraceSetModuleName(TraceTLB, "TLB");
    TraceSetModuleName(TraceProtectedMem, "Protected Memory");
    TraceSetModuleName(TraceUserInterface, "User Interface");
}

void UpdateTraceLevel(void * /*NotUsed*/)
{
    g_ModuleLogLevel[TraceMD5] = (uint8_t)g_Settings->LoadDword(Debugger_TraceMD5);
    g_ModuleLogLevel[TraceSettings] = (uint8_t)g_Settings->LoadDword(Debugger_TraceSettings);
    g_ModuleLogLevel[TraceUnknown] = (uint8_t)g_Settings->LoadDword(Debugger_TraceUnknown);
    g_ModuleLogLevel[TraceAppInit] = (uint8_t)g_Settings->LoadDword(Debugger_TraceAppInit);
    g_ModuleLogLevel[TraceAppCleanup] = (uint8_t)g_Settings->LoadDword(Debugger_TraceAppCleanup);
    g_ModuleLogLevel[TraceN64System] = (uint8_t)g_Settings->LoadDword(Debugger_TraceN64System);
    g_ModuleLogLevel[TracePlugins] = (uint8_t)g_Settings->LoadDword(Debugger_TracePlugins);
    g_ModuleLogLevel[TraceGFXPlugin] = (uint8_t)g_Settings->LoadDword(Debugger_TraceGFXPlugin);
    g_ModuleLogLevel[TraceAudioPlugin] = (uint8_t)g_Settings->LoadDword(Debugger_TraceAudioPlugin);
    g_ModuleLogLevel[TraceControllerPlugin] = (uint8_t)g_Settings->LoadDword(Debugger_TraceControllerPlugin);
    g_ModuleLogLevel[TraceRSPPlugin] = (uint8_t)g_Settings->LoadDword(Debugger_TraceRSPPlugin);
    g_ModuleLogLevel[TraceRSP] = (uint8_t)g_Settings->LoadDword(Debugger_TraceRSP);
    g_ModuleLogLevel[TraceAudio] = (uint8_t)g_Settings->LoadDword(Debugger_TraceAudio);
    g_ModuleLogLevel[TraceRegisterCache] = (uint8_t)g_Settings->LoadDword(Debugger_TraceRegisterCache);
    g_ModuleLogLevel[TraceRecompiler] = (uint8_t)g_Settings->LoadDword(Debugger_TraceRecompiler);
    g_ModuleLogLevel[TraceTLB] = (uint8_t)g_Settings->LoadDword(Debugger_TraceTLB);
    g_ModuleLogLevel[TraceProtectedMem] = (uint8_t)g_Settings->LoadDword(Debugger_TraceProtectedMEM);
    g_ModuleLogLevel[TraceUserInterface] = (uint8_t)g_Settings->LoadDword(Debugger_TraceUserInterface);
}

void SetupTrace(void)
{
    SetTraceModuleNames();
    AddLogModule();

    g_Settings->RegisterChangeCB(Debugger_TraceMD5, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceSettings, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceUnknown, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceAppInit, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceAppCleanup, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceN64System, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TracePlugins, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceGFXPlugin, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceAudioPlugin, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceControllerPlugin, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceRSPPlugin, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceRSP, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceAudio, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceRegisterCache, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceRecompiler, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceTLB, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceProtectedMEM, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_TraceUserInterface, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->RegisterChangeCB(Debugger_AppLogFlush, g_LogFile, (CSettings::SettingChangedFunc)LogFlushChanged);
    UpdateTraceLevel(NULL);

    WriteTrace(TraceAppInit, TraceInfo, "Application Starting %s", VER_FILE_VERSION_STR);
}

void CleanupTrace(void)
{
    WriteTrace(TraceAppCleanup, TraceDebug, "Done");

    g_Settings->UnregisterChangeCB(Debugger_TraceMD5, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceSettings, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceUnknown, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceAppInit, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceAppCleanup, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceN64System, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TracePlugins, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceGFXPlugin, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceAudioPlugin, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceControllerPlugin, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceRSPPlugin, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceRSP, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceAudio, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceRegisterCache, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceRecompiler, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceTLB, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceProtectedMEM, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_TraceUserInterface, NULL, (CSettings::SettingChangedFunc)UpdateTraceLevel);
    g_Settings->UnregisterChangeCB(Debugger_AppLogFlush, g_LogFile, (CSettings::SettingChangedFunc)LogFlushChanged);

    CloseTrace();
    if (g_LogFile) { delete g_LogFile; g_LogFile = NULL; }
}

void AppInit(CNotification * Notify)
{
    try
    {
        g_Notify = Notify;
        InitializeLog();

        stdstr_f AppName("Project64 %s", VER_FILE_VERSION_STR);
        IncreaseThreadPriority();

        g_Settings = new CSettings;
        g_Settings->Initialize(AppName.c_str());

        if (g_Settings->LoadBool(Setting_CheckEmuRunning) &&
            pjutil::TerminatedExistingExe())
        {
            delete g_Settings;
            g_Settings = new CSettings;
            g_Settings->Initialize(AppName.c_str());
        }

        SetupTrace();
        CMipsMemoryVM::ReserveMemory();
        FixDirectories();

        //Create the plugin container
        WriteTrace(TraceAppInit, TraceInfo, "Create Plugins");
        g_Plugins = new CPlugins(g_Settings->LoadStringVal(Directory_Plugin));

        g_Lang = new CLanguage();
        g_Lang->LoadCurrentStrings();
        g_Notify->AppInitDone();
    }
    catch (...)
    {
        g_Notify->DisplayError(stdstr_f("Exception caught\nFile: %s\nLine: %d", __FILE__, __LINE__).ToUTF16().c_str());
    }
}

void AppCleanup(void)
{
    WriteTrace(TraceAppCleanup, TraceDebug, "cleaning up global objects");

    if (g_Rom)      { delete g_Rom; g_Rom = NULL; }
    if (g_Plugins)  { delete g_Plugins; g_Plugins = NULL; }
    if (g_Settings) { delete g_Settings; g_Settings = NULL; }
    if (g_Lang)     { delete g_Lang; g_Lang = NULL; }

    CMipsMemoryVM::FreeReservedMemory();
    CleanupTrace();
}

void FixDirectories(void)
{
    CPath Directory(CPath::MODULE_DIRECTORY);
    Directory.AppendDirectory("Config");
    if (!Directory.DirectoryExists()) Directory.DirectoryCreate();

    Directory.UpDirectory();
    Directory.AppendDirectory("Save");
    if (!Directory.DirectoryExists()) Directory.DirectoryCreate();

    Directory.UpDirectory();
    Directory.AppendDirectory("Screenshots");
    if (!Directory.DirectoryExists()) Directory.DirectoryCreate();

    Directory.UpDirectory();
    Directory.AppendDirectory("textures");
    if (!Directory.DirectoryExists()) Directory.DirectoryCreate();
}

#include <windows.h>
void IncreaseThreadPriority(void)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
}