#include "stdafx.h"

#include <Common/path.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Plugins/Plugin.h>

CPlugins::CPlugins(SettingID PluginDirSetting, bool SyncPlugins) :
    m_MainWindow(nullptr),
    m_SyncWindow(nullptr),
    m_PluginDirSetting(PluginDirSetting),
    m_PluginDir(g_Settings->LoadStringVal(PluginDirSetting)),
    m_Gfx(nullptr),
    m_Audio(nullptr),
    m_RSP(nullptr),
    m_Control(nullptr),
    m_initilized(false),
    m_SyncPlugins(SyncPlugins)
{
    m_PluginDir = (const std::string &)((CPath(m_PluginDir, "").NormalizePath(CPath(CPath::MODULE_DIRECTORY))));
    CreatePlugins();
    g_Settings->RegisterChangeCB(Plugin_RSP_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_GFX_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_AUDIO_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_CONT_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_RspMultiThreaded, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_UseHleGfx, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Plugin_UseHleAudio, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Game_EditPlugin_Gfx, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Game_EditPlugin_Audio, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Game_EditPlugin_Contr, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(Game_EditPlugin_RSP, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->RegisterChangeCB(m_PluginDirSetting, this, (CSettings::SettingChangedFunc)PluginChanged);
}

CPlugins::~CPlugins(void)
{
    g_Settings->UnregisterChangeCB(Plugin_RSP_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_GFX_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_AUDIO_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_CONT_Current, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_RspMultiThreaded, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_UseHleGfx, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Plugin_UseHleAudio, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Game_EditPlugin_Gfx, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Game_EditPlugin_Audio, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Game_EditPlugin_Contr, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(Game_EditPlugin_RSP, this, (CSettings::SettingChangedFunc)PluginChanged);
    g_Settings->UnregisterChangeCB(m_PluginDirSetting, this, (CSettings::SettingChangedFunc)PluginChanged);

    DestroyGfxPlugin();
    DestroyAudioPlugin();
    DestroyRspPlugin();
    DestroyControlPlugin();
}

void CPlugins::PluginChanged(CPlugins * _this)
{
    WriteTrace(TracePlugins, TraceDebug, "Start");
    if (g_Settings->LoadBool(Game_TempLoaded) == true)
    {
        WriteTrace(TracePlugins, TraceDebug, "Game is temporary loaded, not changing plugins");
        return;
    }

    bool bGfxChange = _stricmp(_this->m_GfxFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Gfx).c_str()) != 0;
    bool bAudioChange = _stricmp(_this->m_AudioFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Audio).c_str()) != 0;
    bool bRspChange = _stricmp(_this->m_RSPFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_RSP).c_str()) != 0;
    bool bContChange = _stricmp(_this->m_ControlFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Controller).c_str()) != 0;
    bool bDirChange = _stricmp(_this->m_PluginDir.c_str(), g_Settings->LoadStringVal(_this->m_PluginDirSetting).c_str()) != 0;
    WriteTrace(TracePlugins, TraceVerbose, "m_GfxFile: \"%s\" Game_Plugin_Gfx: \"%s\" changed: %s", _this->m_GfxFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Gfx).c_str(), bGfxChange ? "true" : "false");
    WriteTrace(TracePlugins, TraceVerbose, "m_AudioFile: \"%s\" Game_Plugin_Audio: \"%s\" changed: %s", _this->m_AudioFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Audio).c_str(), bAudioChange ? "true" : "false");
    WriteTrace(TracePlugins, TraceVerbose, "m_RSPFile: \"%s\" Game_Plugin_RSP: \"%s\" changed: %s", _this->m_RSPFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_RSP).c_str(), bRspChange ? "true" : "false");
    WriteTrace(TracePlugins, TraceVerbose, "m_ControlFile: \"%s\" Game_Plugin_Controller: \"%s\" changed: %s", _this->m_ControlFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Controller).c_str(), bContChange ? "true" : "false");
    WriteTrace(TracePlugins, TraceVerbose, "m_PluginDir: \"%s\" m_PluginDirSetting: \"%s\" changed: %s", _this->m_PluginDir.c_str(), g_Settings->LoadStringVal(_this->m_PluginDirSetting).c_str(), bDirChange ? "true" : "false");
    if (bDirChange)
    {
        WriteTrace(TracePlugins, TraceDebug, "Plugin directory changed");
        bGfxChange = true;
        bAudioChange = true;
        bRspChange = true;
        bContChange = true;
        _this->m_PluginDir = g_Settings->LoadStringVal(_this->m_PluginDirSetting);
    }

    if (bGfxChange || bAudioChange || bRspChange || bContChange)
    {
        if (bGfxChange)
        {
            WriteTrace(TracePlugins, TraceDebug, "GFX plugin changed");
        }
        if (bAudioChange)
        {
            WriteTrace(TracePlugins, TraceDebug, "Audio plugin changed");
        }
        if (bRspChange)
        {
            WriteTrace(TracePlugins, TraceDebug, "RSP plugin changed");
        }
        if (bContChange)
        {
            WriteTrace(TracePlugins, TraceDebug, "Controller plugin changed");
        }
        if (g_Settings->LoadBool(GameRunning_CPU_Running))
        {
            // Ensure that base system actually exists before we go triggering the event
            if (g_BaseSystem)
            {
                g_BaseSystem->ExternalEvent(SysEvent_ChangePlugins);
            }
        }
        else
        {
            _this->Reset(nullptr);
        }
    }
    WriteTrace(TracePlugins, TraceDebug, "Done");
}

template <typename plugin_type>
static void LoadPlugin(SettingID PluginSettingID, SettingID PluginVerSettingID, plugin_type *& plugin, const char * PluginDir, stdstr & FileName, TraceModuleProject64 TraceLevel, const char * type, bool IsCopy)
{
    if (plugin != nullptr)
    {
        return;
    }
    FileName = g_Settings->LoadStringVal(PluginSettingID);
    CPath PluginFileName(PluginDir, FileName.c_str());
    if (IsCopy)
    {
        PluginFileName.SetName(stdstr_f("%s-copy", PluginFileName.GetName().c_str()).c_str());
    }
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
            WriteTrace(TraceLevel, TraceError, "Failed to load %s", (const char *)PluginFileName);
            delete plugin;
            plugin = nullptr;
        }
        WriteTrace(TraceLevel, TraceDebug, "%s Loading done", type);
    }
    else
    {
        WriteTrace(TraceLevel, TraceError, "Failed to allocate %s plugin", type);
    }
}

void CPlugins::CreatePlugins(void)
{
    WriteTrace(TracePlugins, TraceInfo, "Start");

    LoadPlugin(Game_Plugin_Gfx, Plugin_GFX_CurVer, m_Gfx, m_PluginDir.c_str(), m_GfxFile, TraceVideoPlugin, "GFX", m_SyncPlugins);
    LoadPlugin(Game_Plugin_Audio, Plugin_AUDIO_CurVer, m_Audio, m_PluginDir.c_str(), m_AudioFile, TraceAudioPlugin, "Audio", m_SyncPlugins);
    LoadPlugin(Game_Plugin_RSP, Plugin_RSP_CurVer, m_RSP, m_PluginDir.c_str(), m_RSPFile, TraceRSPPlugin, "RSP", m_SyncPlugins);
    LoadPlugin(Game_Plugin_Controller, Plugin_CONT_CurVer, m_Control, m_PluginDir.c_str(), m_ControlFile, TraceControllerPlugin, "Control", m_SyncPlugins);

    if (m_RSP != nullptr)
    {
        m_RSP->EnableDebugging(HaveDebugger());
    }
    WriteTrace(TracePlugins, TraceInfo, "Done");
}

void CPlugins::GameReset(void)
{
    if (m_Gfx)
    {
        m_Gfx->GameReset(m_MainWindow);
    }
    if (m_Audio)
    {
        m_Audio->GameReset(m_MainWindow);
    }
    if (m_RSP)
    {
        m_RSP->GameReset(m_MainWindow);
    }
    if (m_Control)
    {
        m_Control->GameReset(m_MainWindow);
    }
}

void CPlugins::DestroyGfxPlugin(void)
{
    if (m_Gfx == nullptr)
    {
        return;
    }
    WriteTrace(TraceVideoPlugin, TraceDebug, "Before close");
    m_Gfx->Close(m_MainWindow);
    WriteTrace(TraceVideoPlugin, TraceInfo, "Deleting");
    delete m_Gfx;
    WriteTrace(TraceVideoPlugin, TraceInfo, "m_Gfx deleted");
    m_Gfx = nullptr;
    //		g_Settings->UnknownSetting_GFX = nullptr;
    DestroyRspPlugin();
    WriteTrace(TraceVideoPlugin, TraceInfo, "Done");
}

void CPlugins::DestroyAudioPlugin(void)
{
    if (m_Audio == nullptr)
    {
        return;
    }
    WriteTrace(TraceAudioPlugin, TraceDebug, "Before close");
    m_Audio->Close(m_MainWindow);
    WriteTrace(TraceAudioPlugin, TraceDebug, "Before delete");
    delete m_Audio;
    WriteTrace(TraceAudioPlugin, TraceDebug, "After delete");
    m_Audio = nullptr;
    WriteTrace(TraceAudioPlugin, TraceDebug, "Before DestroyRspPlugin");
    //		g_Settings->UnknownSetting_AUDIO = nullptr;
    DestroyRspPlugin();
    WriteTrace(TraceAudioPlugin, TraceDebug, "After DestroyRspPlugin");
}

void CPlugins::DestroyRspPlugin(void)
{
    if (m_RSP == nullptr)
    {
        return;
    }
    WriteTrace(TraceRSPPlugin, TraceDebug, "Before close");
    m_RSP->Close(m_MainWindow);
    WriteTrace(TraceRSPPlugin, TraceDebug, "Before delete");
    delete m_RSP;
    m_RSP = nullptr;
    WriteTrace(TraceRSPPlugin, TraceDebug, "After delete");
    //		g_Settings->UnknownSetting_RSP = nullptr;
}

void CPlugins::DestroyControlPlugin(void)
{
    if (m_Control == nullptr)
    {
        return;
    }
    WriteTrace(TraceControllerPlugin, TraceDebug, "Before close");
    m_Control->Close(m_MainWindow);
    WriteTrace(TraceControllerPlugin, TraceDebug, "Before delete");
    delete m_Control;
    m_Control = nullptr;
    WriteTrace(TraceControllerPlugin, TraceDebug, "After delete");
    //		g_Settings->UnknownSetting_CTRL = nullptr;
}

void CPlugins::SetRenderWindows(RenderWindow * MainWindow, RenderWindow * SyncWindow)
{
    WriteTrace(TracePlugins, TraceDebug, "MainWindow = %p SyncWindow = %p", MainWindow, SyncWindow);
    m_MainWindow = MainWindow;
    m_SyncWindow = SyncWindow;
}

void CPlugins::RomOpened(void)
{
    WriteTrace(TracePlugins, TraceDebug, "Start");

    m_Gfx->RomOpened(m_MainWindow);
    m_RSP->RomOpened(m_MainWindow);
    m_Audio->RomOpened(m_MainWindow);
    m_Control->RomOpened(m_MainWindow);

    WriteTrace(TracePlugins, TraceDebug, "Done");
}

void CPlugins::RomClosed(void)
{
    WriteTrace(TracePlugins, TraceDebug, "Start");

    m_Gfx->RomClose(m_MainWindow);
    m_RSP->RomClose(m_MainWindow);
    m_Audio->RomClose(m_MainWindow);
    m_Control->RomClose(m_MainWindow);

    WriteTrace(TracePlugins, TraceDebug, "Done");
}

bool CPlugins::Initiate(CN64System * System)
{
    WriteTrace(TracePlugins, TraceDebug, "Start");
    // Check to make sure we have the plugin available to be used
    if (m_Gfx == nullptr)
    {
        return false;
    }
    if (m_Audio == nullptr)
    {
        return false;
    }
    if (m_RSP == nullptr)
    {
        return false;
    }
    if (m_Control == nullptr)
    {
        return false;
    }

    WriteTrace(TraceVideoPlugin, TraceDebug, "GFX initiate starting");
    if (!m_Gfx->Initiate(System, m_MainWindow))
    {
        return false;
    }
    WriteTrace(TraceVideoPlugin, TraceDebug, "GFX initiate done");
    WriteTrace(TraceAudioPlugin, TraceDebug, "Audio initiate starting");
    if (!m_Audio->Initiate(System, m_MainWindow))
    {
        return false;
    }
    WriteTrace(TraceAudioPlugin, TraceDebug, "Audio initiate done");
    WriteTrace(TraceControllerPlugin, TraceDebug, "Control initiate starting");
    if (!m_Control->Initiate(System, m_MainWindow))
    {
        return false;
    }
    WriteTrace(TraceControllerPlugin, TraceDebug, "Control initiate done");
    WriteTrace(TraceRSPPlugin, TraceDebug, "RSP initiate starting");
    if (!m_RSP->Initiate(this, System))
    {
        return false;
    }
    WriteTrace(TraceRSPPlugin, TraceDebug, "RSP initiate done");
    WriteTrace(TracePlugins, TraceDebug, "Done");
    m_initilized = true;
    return true;
}

bool CPlugins::ResetInUiThread(CN64System * System)
{
#ifdef _WIN32
    return m_MainWindow->ResetPluginsInUiThread(this, System);
#else
    return false;
#endif
}

bool CPlugins::Reset(CN64System * System)
{
    WriteTrace(TracePlugins, TraceDebug, "Start");

    bool bGfxChange = _stricmp(m_GfxFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Gfx).c_str()) != 0;
    bool bAudioChange = _stricmp(m_AudioFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Audio).c_str()) != 0;
    bool bRspChange = _stricmp(m_RSPFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_RSP).c_str()) != 0;
    bool bContChange = _stricmp(m_ControlFile.c_str(), g_Settings->LoadStringVal(Game_Plugin_Controller).c_str()) != 0;

    // If GFX and audio has changed we also need to force reset of RSP
    if (bGfxChange || bAudioChange)
    {
        bRspChange = true;
    }

    if (bGfxChange)
    {
        DestroyGfxPlugin();
    }
    if (bAudioChange)
    {
        DestroyAudioPlugin();
    }
    if (bRspChange)
    {
        DestroyRspPlugin();
    }
    if (bContChange)
    {
        DestroyControlPlugin();
    }

    CreatePlugins();

    if (m_Gfx && bGfxChange)
    {
        WriteTrace(TraceVideoPlugin, TraceDebug, "GFX initiate starting");
        if (!m_Gfx->Initiate(System, m_MainWindow))
        {
            return false;
        }
        WriteTrace(TraceVideoPlugin, TraceDebug, "GFX initiate done");
    }
    if (m_Audio && bAudioChange)
    {
        WriteTrace(TraceAudioPlugin, TraceDebug, "Audio initiate starting");
        if (!m_Audio->Initiate(System, m_MainWindow))
        {
            return false;
        }
        WriteTrace(TraceAudioPlugin, TraceDebug, "Audio initiate done");
    }
    if (m_Control && bContChange)
    {
        WriteTrace(TraceControllerPlugin, TraceDebug, "Control initiate starting");
        if (!m_Control->Initiate(System, m_MainWindow))
        {
            return false;
        }
        WriteTrace(TraceControllerPlugin, TraceDebug, "Control initiate done");
    }
    if (m_RSP && bRspChange)
    {
        WriteTrace(TraceRSPPlugin, TraceDebug, "RSP initiate starting");
        if (!m_RSP->Initiate(this, System))
        {
            return false;
        }
        WriteTrace(TraceRSPPlugin, TraceDebug, "RSP initiate done");
    }

    if (System)
    {
        System->RefreshSyncToAudio();
    }
    WriteTrace(TracePlugins, TraceDebug, "Done");
    return true;
}

void CPlugins::ConfigPlugin(void * hParent, PLUGIN_TYPE Type)
{
    if (g_BaseSystem)
    {
        g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_Settings);
    }

    switch (Type)
    {
    case PLUGIN_TYPE_RSP:
        if (m_RSP == nullptr || m_RSP->DllConfig == nullptr)
        {
            break;
        }
        if (!m_RSP->Initialized())
        {
            if (!m_RSP->Initiate(this, nullptr))
            {
                break;
            }
        }
        m_RSP->DllConfig(hParent);
        break;
    case PLUGIN_TYPE_VIDEO:
        if (m_Gfx == nullptr || m_Gfx->DllConfig == nullptr)
        {
            break;
        }
        if (!m_Gfx->Initialized())
        {
            if (!m_Gfx->Initiate(nullptr, m_MainWindow))
            {
                break;
            }
        }
        m_Gfx->DllConfig(hParent);
        break;
    case PLUGIN_TYPE_AUDIO:
        if (m_Audio == nullptr || m_Audio->DllConfig == nullptr)
        {
            break;
        }
        if (!m_Audio->Initialized())
        {
            if (!m_Audio->Initiate(nullptr, m_MainWindow))
            {
                break;
            }
        }
        m_Audio->DllConfig(hParent);
        if (g_BaseSystem)
        {
            g_BaseSystem->RefreshSyncToAudio();
        }
        break;
    case PLUGIN_TYPE_CONTROLLER:
        if (m_Control == nullptr || m_Control->DllConfig == nullptr)
        {
            break;
        }
        if (!m_Control->Initialized())
        {
            if (!m_Control->Initiate(nullptr, m_MainWindow))
            {
                break;
            }
        }
        m_Control->DllConfig(hParent);
        break;
    case PLUGIN_TYPE_NONE:
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (g_BaseSystem)
    {
        g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_Settings);
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
    // Copy GFX plugin
    CPath srcGfxPlugin(m_PluginDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Gfx).c_str());
    CPath dstGfxPlugin(DstDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Gfx).c_str());
    dstGfxPlugin.SetName(stdstr_f("%s-copy", dstGfxPlugin.GetName().c_str()).c_str());

    if (!dstGfxPlugin.DirectoryExists())
    {
        dstGfxPlugin.DirectoryCreate();
    }
    if (!srcGfxPlugin.CopyTo(dstGfxPlugin))
    {
        WriteTrace(TracePlugins, TraceError, "Failed to copy %s to %s", (const char *)srcGfxPlugin, (const char *)dstGfxPlugin);
        return false;
    }

    // Copy m_Audio plugin
    CPath srcAudioPlugin(m_PluginDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Audio).c_str());
    CPath dstAudioPlugin(DstDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Audio).c_str());
    dstAudioPlugin.SetName(stdstr_f("%s-copy", dstAudioPlugin.GetName().c_str()).c_str());
    if (!dstAudioPlugin.DirectoryExists())
    {
        dstAudioPlugin.DirectoryCreate();
    }
    if (!srcAudioPlugin.CopyTo(dstAudioPlugin))
    {
        return false;
    }
    // Copy RSP plugin
    CPath srcRSPPlugin(m_PluginDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_RSP).c_str());
    CPath dstRSPPlugin(DstDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_RSP).c_str());
    dstRSPPlugin.SetName(stdstr_f("%s-copy", dstRSPPlugin.GetName().c_str()).c_str());
    if (!dstRSPPlugin.DirectoryExists())
    {
        dstRSPPlugin.DirectoryCreate();
    }
    if (!srcRSPPlugin.CopyTo(dstRSPPlugin))
    {
        return false;
    }

    // Copy Controller plugin
    CPath srcContPlugin(m_PluginDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Controller).c_str());
    CPath dstContPlugin(DstDir.c_str(), g_Settings->LoadStringVal(Game_Plugin_Controller).c_str());
    dstContPlugin.SetName(stdstr_f("%s-copy", dstContPlugin.GetName().c_str()).c_str());
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