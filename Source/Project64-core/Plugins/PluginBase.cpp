/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include <Project64-core/Plugins/PluginBase.h>
#include <Common/path.h>

CPlugin::CPlugin() :
DllAbout(NULL),
DllConfig(NULL),
CloseDLL(NULL),
RomOpen(NULL),
RomClosed(NULL),
PluginOpened(NULL),
SetSettingInfo(NULL),
SetSettingInfo2(NULL),
SetSettingInfo3(NULL),
m_LibHandle(NULL),
m_Initialized(false),
m_RomOpen(false)
{
    memset(&m_PluginInfo, 0, sizeof(m_PluginInfo));
}

CPlugin::~CPlugin()
{
    WriteTrace(PluginTraceType(), TraceDebug, "Start");
    UnloadPlugin();
    WriteTrace(PluginTraceType(), TraceDebug, "Done");
}

bool CPlugin::Load(const char * FileName)
{
    WriteTrace(PluginTraceType(), TraceDebug, "Loading: %s", FileName);

    // Already loaded, so unload first.
    if (m_LibHandle != NULL)
    {
        UnloadPlugin();
    }

    // Try to load the plugin DLL
    //Try to load the DLL library
    m_LibHandle = pjutil::DynLibOpen(FileName, bHaveDebugger());
    WriteTrace(PluginTraceType(), TraceDebug, "Loaded: %s LibHandle: %X", FileName, m_LibHandle);

    if (m_LibHandle == NULL)
    {
        return false;
    }

    // Get DLL information
    void(CALL *GetDllInfo) (PLUGIN_INFO * PluginInfo);
    LoadFunction(GetDllInfo);
    if (GetDllInfo == NULL) { return false; }

    GetDllInfo(&m_PluginInfo);
    if (!ValidPluginVersion(m_PluginInfo)) { return false; }
    if (m_PluginInfo.Type != type()) { return false; }

    LoadFunction(CloseDLL);
    LoadFunction(RomOpen);
    LoadFunction(RomClosed);
    _LoadFunction("PluginLoaded", PluginOpened);
    LoadFunction(DllConfig);
    LoadFunction(DllAbout);

    LoadFunction(SetPluginNotification);
    if (SetPluginNotification)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Found SetPluginNotification");
        PLUGIN_NOTIFICATION info;
        info.DisplayError = DisplayError;
        info.FatalError = FatalError;
        info.DisplayMessage = DisplayMessage;
        info.DisplayMessage2 = DisplayMessage2;
        info.BreakPoint = BreakPoint;
        SetPluginNotification(&info);
    }

    LoadFunction(SetSettingNotificationInfo);
    if (SetSettingNotificationInfo)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Found SetSettingNotificationInfo");
        PLUGIN_SETTINGS_NOTIFICATION info;
        info.RegisterChangeCB = (void(*)(void *, int ID, void * Data, PLUGIN_SETTINGS_NOTIFICATION::SettingChangedFunc Func))CSettings::sRegisterChangeCB;
        info.UnregisterChangeCB = (void(*)(void *, int ID, void * Data, PLUGIN_SETTINGS_NOTIFICATION::SettingChangedFunc Func))CSettings::sUnregisterChangeCB;
        SetSettingNotificationInfo(&info);
    }

    LoadFunction(SetSettingInfo3);
    if (SetSettingInfo3)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Found SetSettingInfo3");
        PLUGIN_SETTINGS3 info;
        info.FlushSettings = (void(*)(void * handle))CSettings::FlushSettings;
        SetSettingInfo3(&info);
    }

    LoadFunction(SetSettingInfo2);
    if (SetSettingInfo2)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Found SetSettingInfo2");
        PLUGIN_SETTINGS2 info;
        info.FindSystemSettingId = (uint32_t(*)(void * handle, const char *))CSettings::FindSetting;
        SetSettingInfo2(&info);
    }

    LoadFunction(SetSettingInfo);
    if (SetSettingInfo)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Found SetSettingInfo");
        PLUGIN_SETTINGS info;
        info.dwSize = sizeof(PLUGIN_SETTINGS);
        info.DefaultStartRange = GetDefaultSettingStartRange();
        info.SettingStartRange = GetSettingStartRange();
        info.MaximumSettings = MaxPluginSetting;
        info.NoDefault = Default_None;
        info.DefaultLocation = g_Settings->LoadDword(Setting_UseFromRegistry) ? SettingType_Registry : SettingType_CfgFile;
        info.handle = g_Settings;
        info.RegisterSetting = (void(*)(void *, int, int, SettingDataType, SettingType, const char *, const char *, uint32_t))&CSettings::RegisterSetting;
        info.GetSetting = (uint32_t(*)(void *, int))&CSettings::GetSetting;
        info.GetSettingSz = (const char * (*)(void *, int, char *, int))&CSettings::GetSettingSz;
        info.SetSetting = (void(*)(void *, int, uint32_t))&CSettings::SetSetting;
        info.SetSettingSz = (void(*)(void *, int, const char *))&CSettings::SetSettingSz;
        info.UseUnregisteredSetting = NULL;

        SetSettingInfo(&info);
    }

    if (RomClosed == NULL)
    {
        return false;
    }

    if (!LoadFunctions())
    {
        WriteTrace(PluginTraceType(), TraceWarning, "Failed to load functions");
        return false;
    }
    WriteTrace(PluginTraceType(), TraceDebug, "Functions loaded");

    if (PluginOpened)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Before Plugin Opened");
        PluginOpened();
        WriteTrace(PluginTraceType(), TraceDebug, "After Plugin Opened");
    }
    WriteTrace(PluginTraceType(), TraceDebug, "Loaded");
    return true;
}

void CPlugin::RomOpened(RenderWindow * Render)
{
    if (m_RomOpen)
    {
        return;
    }

#ifdef ANDROID
    if (m_PluginInfo.Type == PLUGIN_TYPE_GFX)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Render = %p", Render);
        if (Render != NULL)
        {
            WriteTrace(PluginTraceType(), TraceDebug, "Calling GfxThreadInit");
            Render->GfxThreadInit();
            WriteTrace(PluginTraceType(), TraceDebug, "GfxThreadInit Done");
        }
    }
#else
    Render = Render; // used just for andoid
#endif

    if (RomOpen != NULL)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Before Rom Open");
        RomOpen();
        WriteTrace(PluginTraceType(), TraceDebug, "After Rom Open");
    }

    m_RomOpen = true;
}

void CPlugin::RomClose(RenderWindow * Render)
{
    if (!m_RomOpen)
    {
        return;
    }

#ifdef ANDROID
    if (m_PluginInfo.Type == PLUGIN_TYPE_GFX)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Render = %p", Render);
        if (Render != NULL)
        {
            WriteTrace(PluginTraceType(), TraceDebug, "Calling GfxThreadDone");
            Render->GfxThreadDone();
            WriteTrace(PluginTraceType(), TraceDebug, "GfxThreadDone Done");
        }
    }
#else
    Render = Render; // used just for andoid
#endif

    WriteTrace(PluginTraceType(), TraceDebug, "Before Rom Close");
    RomClosed();
    m_RomOpen = false;
    WriteTrace(PluginTraceType(), TraceDebug, "After Rom Close");
}

void CPlugin::GameReset(RenderWindow * Render)
{
    if (m_RomOpen)
    {
        RomClose(Render);
        RomOpened(Render);
    }
}

void CPlugin::Close(RenderWindow * Render)
{
    WriteTrace(PluginTraceType(), TraceDebug, "(%s): Start", PluginType());
    RomClose(Render);
    if (m_Initialized)
    {
        CloseDLL();
        m_Initialized = false;
    }
    WriteTrace(PluginTraceType(), TraceDebug, "(%s): Done", PluginType());
}

void CPlugin::UnloadPlugin()
{
    WriteTrace(PluginTraceType(), TraceDebug, "(%s): Start", PluginType());
    memset(&m_PluginInfo, 0, sizeof(m_PluginInfo));
    if (m_LibHandle != NULL)
    {
        UnloadPluginDetails();
    }
    if (m_LibHandle != NULL)
    {
        pjutil::DynLibClose(m_LibHandle);
        m_LibHandle = NULL;
    }

    DllAbout = NULL;
    CloseDLL = NULL;
    RomOpen = NULL;
    RomClosed = NULL;
    PluginOpened = NULL;
    DllConfig = NULL;
    SetSettingInfo = NULL;
    SetSettingInfo2 = NULL;
    SetSettingInfo3 = NULL;
    WriteTrace(PluginTraceType(), TraceDebug, "(%s): Done", PluginType());
}

const char * CPlugin::PluginType() const
{
    switch (m_PluginInfo.Type)
    {
    case PLUGIN_TYPE_RSP: return "RSP";
    case PLUGIN_TYPE_GFX: return "GFX";
    case PLUGIN_TYPE_AUDIO: return "Audio";
    case PLUGIN_TYPE_CONTROLLER: return "Control";
    }
    return "Unknown";
}

TraceModuleProject64 CPlugin::PluginTraceType() const
{
    switch (m_PluginInfo.Type)
    {
    case PLUGIN_TYPE_RSP: return TraceRSPPlugin;
    case PLUGIN_TYPE_GFX: return TraceGFXPlugin;
    case PLUGIN_TYPE_AUDIO: return TraceAudioPlugin;
    case PLUGIN_TYPE_CONTROLLER: return TraceControllerPlugin;
    }
    return TracePlugins;
}

bool CPlugin::ValidPluginVersion(PLUGIN_INFO & PluginInfo)
{
    switch (PluginInfo.Type)
    {
    case PLUGIN_TYPE_RSP:
        if (!PluginInfo.MemoryBswaped)	  { return false; }
        if (PluginInfo.Version == 0x0001) { return true; }
        if (PluginInfo.Version == 0x0100) { return true; }
        if (PluginInfo.Version == 0x0101) { return true; }
        if (PluginInfo.Version == 0x0102) { return true; }
        if (PluginInfo.Version == 0x0103) { return true; }
        break;
    case PLUGIN_TYPE_GFX:
        if (!PluginInfo.MemoryBswaped)	  { return false; }
        if (PluginInfo.Version == 0x0102) { return true; }
        if (PluginInfo.Version == 0x0103) { return true; }
        if (PluginInfo.Version == 0x0104) { return true; }
        break;
    case PLUGIN_TYPE_AUDIO:
        if (!PluginInfo.MemoryBswaped)	  { return false; }
        if (PluginInfo.Version == 0x0101) { return true; }
        if (PluginInfo.Version == 0x0102) { return true; }
        break;
    case PLUGIN_TYPE_CONTROLLER:
        if (PluginInfo.Version == 0x0100) { return true; }
        if (PluginInfo.Version == 0x0101) { return true; }
        if (PluginInfo.Version == 0x0102) { return true; }
        break;
    }
    return false;
}

void CPlugin::DisplayError(const char * Message)
{
    g_Notify->DisplayError(Message);
}

void CPlugin::FatalError(const char * Message)
{
    g_Notify->FatalError(Message);
}

void CPlugin::DisplayMessage(int DisplayTime, const char * Message)
{
    g_Notify->DisplayMessage(DisplayTime, Message);
}

void CPlugin::DisplayMessage2(const char * Message)
{
    g_Notify->DisplayMessage2(Message);
}

void CPlugin::BreakPoint(const char * FileName, int32_t LineNumber)
{
    g_Notify->BreakPoint(FileName, LineNumber);
}
