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
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/Plugins/PluginClass.h>
#include <Common/path.h>

CPlugins::CPlugins(const stdstr & PluginDir) :
    m_MainWindow(NULL),
    m_SyncWindow(NULL),
    m_PluginDir(PluginDir),
    m_Gfx(NULL),
    m_Audio(NULL),
    m_RSP(NULL),
    m_Control(NULL)
{
    CreatePlugins();
    g_Settings->RegisterChangeCB(Plugin_RSP_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_GFX_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_AUDIO_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_CONT_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_UseHleGfx, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_UseHleAudio, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Game_EditPlugin_Gfx, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Game_EditPlugin_Audio, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Game_EditPlugin_Contr, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Game_EditPlugin_RSP, this, (CSettings::SettingChangedFunc)PluginChanged);
}

CPlugins::~CPlugins(void)
{
    g_Settings->UnregisterChangeCB(Plugin_RSP_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_GFX_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_AUDIO_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_CONT_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_UseHleGfx, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_UseHleAudio, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Game_EditPlugin_Gfx, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Game_EditPlugin_Audio, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Game_EditPlugin_Contr, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Game_EditPlugin_RSP, this, (CSettings::SettingChangedFunc)PluginChanged);

    DestroyGfxPlugin();
    DestroyAudioPlugin();
    DestroyRspPlugin();
    DestroyControlPlugin();
}

void CPlugins::PluginChanged(CPlugins * _this)
{
    if (g_Settings->LoadBool(Game_TempLoaded) == true)
    {
        return;
    }
    bool bGfxChange = _stricmp(_this->m_GfxFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Gfx).c_str()) != 0;
    bool bAudioChange = _stricmp(_this->m_AudioFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Audio).c_str()) != 0;
    bool bRspChange = _stricmp(_this->m_RSPFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_RSP).c_str()) != 0;
    bool bContChange = _stricmp(_this->m_ControlFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Controller).c_str()) != 0;

    if (bGfxChange || bAudioChange || bRspChange || bContChange)
    {
        if (g_Settings->LoadBool(GameRunning_CPU_Running))
        {
            //Ensure that base system actually exists before we go triggering the event
            if (g_BaseSystem)
            {
                g_BaseSystem->ExternalEvent(SysEvent_ChangePlugins);
            }
        }
        else
        {
            _this->Reset(NULL);
        }
    }
}

template <typename plugin_type>
static void LoadPlugin(SettingID PluginSettingID, SettingID PluginVerSettingID, plugin_type * & plugin, const char * PluginDir, stdstr & FileName, TraceModuleProject64 TraceLevel, const char * type)
{
    if (plugin != NULL)
    {
        return;
    }
    FileName = g_Settings->LoadStringVal(PluginSettingID);
    CPath PluginFileName(PluginDir, FileName.c_str());
    plugin = new plugin_type();
    if (plugin)
    {
        WriteTrace(TraceLevel, TraceDebug, "%s Loading (%s): Starting", type, (const char *)PluginFileName);
        if (plugin->Load(PluginFileName))
        {
            WriteTrace(TraceLevel, TraceDebug, "%s Current Ver: %s", type, plugin->PluginName());
            g_Settings->SaveString(PluginVerSettingID, plugin->PluginName());
        }
        else
        {
            WriteTrace(TraceError, TraceDebug, "Failed to load %s", (const char *)PluginFileName);
            delete plugin;
            plugin = NULL;
        }
        WriteTrace(TraceLevel, TraceDebug, "%s Loading Done", type);
    }
    else
    {
        WriteTrace(TraceError, TraceDebug, "Failed to allocate %s plugin", type);
    }
}

void CPlugins::CreatePlugins(void)
{
    LoadPlugin(Game_Plugin_Gfx, Plugin_GFX_CurVer, m_Gfx, m_PluginDir.c_str(), m_GfxFile, TraceGFXPlugin, "GFX");
    LoadPlugin(Game_Plugin_Audio, Plugin_AUDIO_CurVer, m_Audio, m_PluginDir.c_str(), m_AudioFile, TraceAudioPlugin, "Audio");
    LoadPlugin(Game_Plugin_RSP, Plugin_RSP_CurVer, m_RSP, m_PluginDir.c_str(), m_RSPFile, TraceRSPPlugin, "RSP");
    LoadPlugin(Game_Plugin_Controller, Plugin_CONT_CurVer, m_Control, m_PluginDir.c_str(), m_ControlFile, TraceControllerPlugin, "Control");

    //Enable debugger
    if (m_RSP != NULL && m_RSP->EnableDebugging)
    {
        WriteTrace(TraceRSPPlugin, TraceInfo, "EnableDebugging starting");
        m_RSP->EnableDebugging(bHaveDebugger());
        WriteTrace(TraceRSPPlugin, TraceInfo, "EnableDebugging done");
    }
}

void CPlugins::GameReset(void)
{
    if (m_Gfx)
    {
        m_Gfx->GameReset();
    }
    if (m_Audio)
    {
        m_Audio->GameReset();
    }
    if (m_RSP)
    {
        m_RSP->GameReset();
    }
    if (m_Control)
    {
        m_Control->GameReset();
    }
}

void CPlugins::DestroyGfxPlugin(void)
{
    if (m_Gfx == NULL)
    {
        return;
    }
    WriteTrace(TraceGFXPlugin, TraceInfo, "before delete m_Gfx");
    delete m_Gfx;
    WriteTrace(TraceGFXPlugin, TraceInfo, "after delete m_Gfx");
    m_Gfx = NULL;
    //		g_Settings->UnknownSetting_GFX = NULL;
    DestroyRspPlugin();
}

void CPlugins::DestroyAudioPlugin(void)
{
    if (m_Audio == NULL)
    {
        return;
    }
    WriteTrace(TraceAudioPlugin, TraceDebug, "before close");
    m_Audio->Close();
    WriteTrace(TraceAudioPlugin, TraceDebug, "before delete");
    delete m_Audio;
    WriteTrace(TraceAudioPlugin, TraceDebug, "after delete");
    m_Audio = NULL;
    WriteTrace(TraceAudioPlugin, TraceDebug, "before DestroyRspPlugin");
    //		g_Settings->UnknownSetting_AUDIO = NULL;
    DestroyRspPlugin();
    WriteTrace(TraceAudioPlugin, TraceDebug, "after DestroyRspPlugin");
}

void CPlugins::DestroyRspPlugin(void)
{
    if (m_RSP == NULL)
    {
        return;
    }
    WriteTrace(TraceRSPPlugin, TraceDebug, "before close");
    m_RSP->Close();
    WriteTrace(TraceRSPPlugin, TraceDebug, "before delete");
    delete m_RSP;
    m_RSP = NULL;
    WriteTrace(TraceRSPPlugin, TraceDebug, "after delete");
    //		g_Settings->UnknownSetting_RSP = NULL;
}

void CPlugins::DestroyControlPlugin(void)
{
    if (m_Control == NULL)
    {
        return;
    }
    WriteTrace(TraceControllerPlugin, TraceDebug, "before close");
    m_Control->Close();
    WriteTrace(TraceControllerPlugin, TraceDebug, "before delete");
    delete m_Control;
    m_Control = NULL;
    WriteTrace(TraceControllerPlugin, TraceDebug, "after delete");
    //		g_Settings->UnknownSetting_CTRL = NULL;
}

void CPlugins::SetRenderWindows(RenderWindow * MainWindow, RenderWindow * SyncWindow)
{
    m_MainWindow = MainWindow;
    m_SyncWindow = SyncWindow;
}

void CPlugins::RomOpened(void)
{
    m_Gfx->RomOpened();
    m_RSP->RomOpened();
    m_Audio->RomOpened();
    m_Control->RomOpened();
}

void CPlugins::RomClosed(void)
{
    m_Gfx->RomClose();
    m_RSP->RomClose();
    m_Audio->RomClose();
    m_Control->RomClose();
}

bool CPlugins::Initiate(CN64System * System)
{
    WriteTrace(TracePlugins, TraceDebug, "Start");
    //Check to make sure we have the plugin available to be used
    if (m_Gfx == NULL) { return false; }
    if (m_Audio == NULL) { return false; }
    if (m_RSP == NULL) { return false; }
    if (m_Control == NULL) { return false; }

    WriteTrace(TraceGFXPlugin, TraceDebug, "Gfx Initiate Starting");
    if (!m_Gfx->Initiate(System, m_MainWindow))   { return false; }
    WriteTrace(TraceGFXPlugin, TraceDebug, "Gfx Initiate Done");
    WriteTrace(TraceAudioPlugin, TraceDebug, "Audio Initiate Starting");
    if (!m_Audio->Initiate(System, m_MainWindow)) { return false; }
    WriteTrace(TraceAudioPlugin, TraceDebug, "Audio Initiate Done");
    WriteTrace(TraceControllerPlugin, TraceDebug, "Control Initiate Starting");
    if (!m_Control->Initiate(System, m_MainWindow)) { return false; }
    WriteTrace(TraceControllerPlugin, TraceDebug, "Control Initiate Done");
    WriteTrace(TraceRSPPlugin, TraceDebug, "RSP Initiate Starting");
    if (!m_RSP->Initiate(this, System))   { return false; }
    WriteTrace(TraceRSPPlugin, TraceDebug, "RSP Initiate Done");
    WriteTrace(TracePlugins, TraceDebug, "Done");
    return true;
}

bool CPlugins::ResetInUiThread(CN64System * System)
{
    return m_MainWindow->ResetPluginsInUiThread(this, System);
}

bool CPlugins::Reset(CN64System * System)
{
    WriteTrace(TracePlugins, TraceDebug, "Start");

    bool bGfxChange = _stricmp(m_GfxFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Gfx).c_str()) != 0;
    bool bAudioChange = _stricmp(m_AudioFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Audio).c_str()) != 0;
    bool bRspChange = _stricmp(m_RSPFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_RSP).c_str()) != 0;
    bool bContChange = _stricmp(m_ControlFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Controller).c_str()) != 0;

    //if GFX and Audio has changed we also need to force reset of RSP
    if (bGfxChange || bAudioChange)
        bRspChange = true;

    if (bGfxChange) { DestroyGfxPlugin(); }
    if (bAudioChange) { DestroyAudioPlugin(); }
    if (bRspChange) { DestroyRspPlugin(); }
    if (bContChange) { DestroyControlPlugin(); }

    CreatePlugins();

    if (m_Gfx && bGfxChange)
    {
        WriteTrace(TraceGFXPlugin, TraceDebug, "Gfx Initiate Starting");
        if (!m_Gfx->Initiate(System, m_MainWindow)) { return false; }
        WriteTrace(TraceGFXPlugin, TraceDebug, "Gfx Initiate Done");
    }
    if (m_Audio && bAudioChange)
    {
        WriteTrace(TraceAudioPlugin, TraceDebug, "Audio Initiate Starting");
        if (!m_Audio->Initiate(System, m_MainWindow)) { return false; }
        WriteTrace(TraceAudioPlugin, TraceDebug, "Audio Initiate Done");
    }
    if (m_Control && bContChange)
    {
        WriteTrace(TraceControllerPlugin, TraceDebug, "Control Initiate Starting");
        if (!m_Control->Initiate(System, m_MainWindow)) { return false; }
        WriteTrace(TraceControllerPlugin, TraceDebug, "Control Initiate Done");
    }
    if (m_RSP && bRspChange)
    {
        WriteTrace(TraceRSPPlugin, TraceDebug, "RSP Initiate Starting");
        if (!m_RSP->Initiate(this, System)) { return false; }
        WriteTrace(TraceRSPPlugin, TraceDebug, "RSP Initiate Done");
    }
    WriteTrace(TracePlugins, TraceDebug, "Done");
    return true;
}

void CPlugins::ConfigPlugin(void* hParent, PLUGIN_TYPE Type)
{
    switch (Type)
    {
    case PLUGIN_TYPE_RSP:
        if (m_RSP == NULL || m_RSP->DllConfig == NULL) { break; }
        if (!m_RSP->Initialized())
        {
            if (!m_RSP->Initiate(this, NULL))
            {
                break;
            }
        }
        m_RSP->DllConfig(hParent);
        break;
    case PLUGIN_TYPE_GFX:
        if (m_Gfx == NULL || m_Gfx->DllConfig == NULL) { break; }
        if (!m_Gfx->Initialized())
        {
            if (!m_Gfx->Initiate(NULL, m_MainWindow))
            {
                break;
            }
        }
        m_Gfx->DllConfig(hParent);
        break;
    case PLUGIN_TYPE_AUDIO:
        if (m_Audio == NULL || m_Audio->DllConfig == NULL) { break; }
        if (!m_Audio->Initialized())
        {
            if (!m_Audio->Initiate(NULL, m_MainWindow))
            {
                break;
            }
        }
        m_Audio->DllConfig(hParent);
        break;
    case PLUGIN_TYPE_CONTROLLER:
        if (m_Control == NULL || m_Control->DllConfig == NULL) { break; }
        if (!m_Control->Initialized())
        {
            if (!m_Control->Initiate(NULL, m_MainWindow))
            {
                break;
            }
        }
        m_Control->DllConfig(hParent);
        break;
    }
}

void DummyCheckInterrupts(void)
{
}

void DummyFunction(void)
{
}

bool CPlugins::CopyPlugins(const stdstr & DstDir) const
{
    //Copy GFX Plugin
    CPath srcGfxPlugin(m_PluginDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Gfx).c_str());
    CPath dstGfxPlugin(DstDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Gfx).c_str());

    if (!dstGfxPlugin.DirectoryExists())
    {
        dstGfxPlugin.DirectoryCreate();
    }
    if (!srcGfxPlugin.CopyTo(dstGfxPlugin))
    {
        return false;
    }

    //Copy m_Audio Plugin
    CPath srcAudioPlugin(m_PluginDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Audio).c_str());
    CPath dstAudioPlugin(DstDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Audio).c_str());
    if (!dstAudioPlugin.DirectoryExists())
    {
        dstAudioPlugin.DirectoryCreate();
    }
    if (!srcAudioPlugin.CopyTo(dstAudioPlugin))
    {
        return false;
    }

    //Copy RSP Plugin
    CPath srcRSPPlugin(m_PluginDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_RSP).c_str());
    CPath dstRSPPlugin(DstDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_RSP).c_str());
    if (!dstRSPPlugin.DirectoryExists())
    {
        dstRSPPlugin.DirectoryCreate();
    }
    if (!srcRSPPlugin.CopyTo(dstRSPPlugin))
    {
        return false;
    }

    //Copy Controller Plugin
    CPath srcContPlugin(m_PluginDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Controller).c_str());
    CPath dstContPlugin(DstDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Controller).c_str());
    if (!dstContPlugin.DirectoryExists())
    {
        dstContPlugin.DirectoryCreate();
    }
    if (!srcContPlugin.CopyTo(dstContPlugin))
    {
        return false;
    }
    return true;
}