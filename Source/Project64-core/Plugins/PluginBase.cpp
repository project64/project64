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
#include "PluginBase.h"
#include <Windows.h>

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
m_hDll(NULL),
m_Initialized(false),
m_RomOpen(false)
{
    memset(&m_PluginInfo, 0, sizeof(m_PluginInfo));
}

CPlugin::~CPlugin()
{
    UnloadPlugin();
}

bool CPlugin::Load(const char * FileName)
{
    // Already loaded, so unload first.
    if (m_hDll != NULL)
    {
        UnloadPlugin();
    }

    // Try to load the plugin DLL
    //Try to load the DLL library
    if (bHaveDebugger())
    {
        m_hDll = LoadLibrary(FileName);
    }
    else
    {
        UINT LastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        m_hDll = LoadLibrary(FileName);
        SetErrorMode(LastErrorMode);
    }

    if (m_hDll == NULL)
    {
        return false;
    }

    // Get DLL information
    void(__cdecl *GetDllInfo) (PLUGIN_INFO * PluginInfo);
    LoadFunction(GetDllInfo);
    if (GetDllInfo == NULL) { return false; }

    GetDllInfo(&m_PluginInfo);
    if (!ValidPluginVersion(m_PluginInfo)) { return false; }
    if (m_PluginInfo.Type != type()) { return false; }

    CloseDLL = (void(__cdecl *)(void)) GetProcAddress((HMODULE)m_hDll, "CloseDLL");
    RomOpen = (void(__cdecl *)(void)) GetProcAddress((HMODULE)m_hDll, "RomOpen");
    RomClosed = (void(__cdecl *)(void)) GetProcAddress((HMODULE)m_hDll, "RomClosed");
    PluginOpened = (void(__cdecl *)(void)) GetProcAddress((HMODULE)m_hDll, "PluginLoaded");
    DllConfig = (void(__cdecl *)(void *)) GetProcAddress((HMODULE)m_hDll, "DllConfig");
    DllAbout = (void(__cdecl *)(void *)) GetProcAddress((HMODULE)m_hDll, "DllAbout");

    SetSettingInfo3 = (void(__cdecl *)(PLUGIN_SETTINGS3 *))GetProcAddress((HMODULE)m_hDll, "SetSettingInfo3");
    if (SetSettingInfo3)
    {
        PLUGIN_SETTINGS3 info;
        info.FlushSettings = (void(*)(void * handle))CSettings::FlushSettings;
        SetSettingInfo3(&info);
    }

    SetSettingInfo2 = (void(__cdecl *)(PLUGIN_SETTINGS2 *))GetProcAddress((HMODULE)m_hDll, "SetSettingInfo2");
    if (SetSettingInfo2)
    {
        PLUGIN_SETTINGS2 info;
        info.FindSystemSettingId = (uint32_t(*)(void * handle, const char *))CSettings::FindSetting;
        SetSettingInfo2(&info);
    }

    SetSettingInfo = (void(__cdecl *)(PLUGIN_SETTINGS *))GetProcAddress((HMODULE)m_hDll, "SetSettingInfo");
    if (SetSettingInfo)
    {
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
        return false;

    if (!LoadFunctions())
    {
        return false;
    }
    WriteTrace(PluginTraceType(), TraceDebug, "Functions loaded");

    if (PluginOpened)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Before Plugin Opened");
        PluginOpened();
        WriteTrace(PluginTraceType(), TraceDebug, "After Plugin Opened");
    }
    return true;
}

void CPlugin::RomOpened()
{
    if (m_RomOpen)
    {
        return;
    }

    if (RomOpen != NULL)
    {
        WriteTrace(PluginTraceType(), TraceDebug, "Before Rom Open");
        RomOpen();
        WriteTrace(PluginTraceType(), TraceDebug, "After Rom Open");
    }
    m_RomOpen = true;
}

void CPlugin::RomClose()
{
    if (!m_RomOpen)
        return;

    WriteTrace(PluginTraceType(), TraceDebug, "Before Rom Close");
    RomClosed();
    m_RomOpen = false;
    WriteTrace(PluginTraceType(), TraceDebug, "After Rom Close");
}

void CPlugin::GameReset()
{
    if (m_RomOpen)
    {
        RomClose();
        if (RomOpen)
        {
            RomOpen();
        }
    }
}

void CPlugin::Close()
{
    WriteTrace(PluginTraceType(), TraceDebug, "(%s): Start", PluginType());
    RomClose();
    if (m_Initialized)
    {
        CloseDLL();
        m_Initialized = false;
    }
    WriteTrace(PluginTraceType(), TraceDebug, "(%s): Done", PluginType());
}

void CPlugin::UnloadPlugin()
{
    WriteTrace(PluginTraceType(), TraceDebug, "(%s): unloading", PluginType());
    memset(&m_PluginInfo, 0, sizeof(m_PluginInfo));
    if (m_hDll != NULL)
    {
        UnloadPluginDetails();
        FreeLibrary((HMODULE)m_hDll);
        m_hDll = NULL;
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
    return TraceUnknown;
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
    return FALSE;
}