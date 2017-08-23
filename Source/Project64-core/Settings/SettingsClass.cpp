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

#include <Common/Platform.h>
#include "SettingType/SettingsType-Application.h"
#include "SettingType/SettingsType-ApplicationPath.h"
#include "SettingType/SettingsType-ApplicationIndex.h"
#include "SettingType/SettingsType-Cheats.h"
#include "SettingType/SettingsType-GameSetting.h"
#include "SettingType/SettingsType-GameSettingIndex.h"
#include "SettingType/SettingsType-RelativePath.h"
#include "SettingType/SettingsType-RomDatabase.h"
#include "SettingType/SettingsType-RomDatabaseSetting.h"
#include "SettingType/SettingsType-RomDatabaseIndex.h"
#include "SettingType/SettingsType-RDBCpuType.h"
#include "SettingType/SettingsType-RDBRamSize.h"
#include "SettingType/SettingsType-RDBSaveChip.h"
#include "SettingType/SettingsType-RDBYesNo.h"
#include "SettingType/SettingsType-RDBOnOff.h"
#include "SettingType/SettingsType-SelectedDirectory.h"
#include "SettingType/SettingsType-TempString.h"
#include "SettingType/SettingsType-TempNumber.h"
#include "SettingType/SettingsType-TempBool.h"
#include <Project64-core/Settings/SettingsClass.h>
#include <Project64-core/N64System/N64Types.h>
#include <Common/Trace.h>

CSettings * g_Settings = NULL;

CSettings::CSettings() :
    m_NextAutoSettingId(0x200000)
{
}

CSettings::~CSettings()
{
    CSettingTypeApplication::CleanUp();
    CSettingTypeRomDatabase::CleanUp();
    CSettingTypeGame::CleanUp();
    CSettingTypeCheats::CleanUp();

    for (SETTING_MAP::iterator iter = m_SettingInfo.begin(); iter != m_SettingInfo.end(); iter++)
    {
        delete iter->second;
    }

    for (SETTING_CALLBACK::iterator cb_iter = m_Callback.begin(); cb_iter != m_Callback.end(); cb_iter++)
    {
        SETTING_CHANGED_CB * item = cb_iter->second;
        while (item != NULL)
        {
            SETTING_CHANGED_CB * current_item = item;
            item = item->Next;
            delete current_item;
        }
    }
}

void CSettings::AddHandler(SettingID TypeID, CSettingType * Handler)
{
    std::pair<SETTING_MAP::iterator, bool> res = m_SettingInfo.insert(SETTING_MAP::value_type(TypeID, Handler));
    if (!res.second)
    {
        delete res.first->second;
        m_SettingInfo.erase(res.first);
        res = m_SettingInfo.insert(SETTING_MAP::value_type(TypeID, Handler));
        if (!res.second)
        {
            delete Handler;
        }
    }
}

void CSettings::AddHowToHandleSetting(const char * BaseDirectory)
{
    WriteTrace(TraceAppInit, TraceDebug, "Start");

    //Command Settings
    AddHandler(Cmd_BaseDirectory, new CSettingTypeTempString(BaseDirectory));
    AddHandler(Cmd_ShowHelp, new CSettingTypeTempBool(false));
    AddHandler(Cmd_RomFile, new CSettingTypeTempString(""));

    //Support Files
    AddHandler(SupportFile_Settings, new CSettingTypeApplicationPath("", "ConfigFile", SupportFile_SettingsDefault));
    AddHandler(SupportFile_SettingsDefault, new CSettingTypeRelativePath("Config", "Project64.cfg"));
    AddHandler(SupportFile_RomDatabase, new CSettingTypeApplicationPath("", "RomDatabase", SupportFile_RomDatabaseDefault));
    AddHandler(SupportFile_RomDatabaseDefault, new CSettingTypeRelativePath("Config", "Project64.rdb"));
    AddHandler(SupportFile_Project64VideoRDB, new CSettingTypeApplicationPath("", "Project64VideoRDB", SupportFile_Project64VideoRDBDefault));
    AddHandler(SupportFile_Project64VideoRDBDefault, new CSettingTypeRelativePath("Config", "Project64Video.rdb"));
    AddHandler(SupportFile_Cheats, new CSettingTypeApplicationPath("", "Cheats", SupportFile_CheatsDefault));
    AddHandler(SupportFile_CheatsDefault, new CSettingTypeRelativePath("Config", "Project64.cht"));
    AddHandler(SupportFile_Notes, new CSettingTypeApplicationPath("", "Notes", SupportFile_NotesDefault));
    AddHandler(SupportFile_NotesDefault, new CSettingTypeRelativePath("Config", "Project64.rdn"));
    AddHandler(SupportFile_ExtInfo, new CSettingTypeApplicationPath("", "ExtInfo", SupportFile_ExtInfoDefault));
    AddHandler(SupportFile_ExtInfoDefault, new CSettingTypeRelativePath("Config", "Project64.rdx"));

    //Settings location
    AddHandler(Setting_ApplicationName, new CSettingTypeTempString(""));
    AddHandler(Setting_UseFromRegistry, new CSettingTypeApplication("Settings", "Use Registry", (uint32_t)false));
    AddHandler(Setting_RdbEditor, new CSettingTypeApplication("", "Rdb Editor", false));
    AddHandler(Setting_CN64TimeCritical, new CSettingTypeApplication("", "CN64TimeCritical", false));
    AddHandler(Setting_AutoStart, new CSettingTypeApplication("", "Auto Start", (uint32_t)true));
    AddHandler(Setting_AutoZipInstantSave, new CSettingTypeApplication("", "Auto Zip Saves", (uint32_t)true));
    AddHandler(Setting_EraseGameDefaults, new CSettingTypeApplication("", "Erase on default", (uint32_t)true));
    AddHandler(Setting_CheckEmuRunning, new CSettingTypeApplication("", "Check Running", (uint32_t)true));
    AddHandler(Setting_ForceInterpreterCPU, new CSettingTypeApplication("", "Force Interpreter CPU", false));
    AddHandler(Setting_FixedRdramAddress, new CSettingTypeApplication("", "Fixed Rdram Address", (uint32_t)0));

    AddHandler(Setting_RememberCheats, new CSettingTypeApplication("", "Remember Cheats", (uint32_t)false));
#ifdef ANDROID
    AddHandler(Setting_UniqueSaveDir, new CSettingTypeTempBool(true));
#else
    AddHandler(Setting_UniqueSaveDir, new CSettingTypeApplication("", "Unique Game Dir", (uint32_t)false));
#endif
    AddHandler(Setting_CurrentLanguage, new CSettingTypeApplication("", "Current Language", ""));
    AddHandler(Setting_EnableDisk, new CSettingTypeTempBool(false));
    AddHandler(Setting_LanguageDirDefault, new CSettingTypeRelativePath("Lang", ""));
    AddHandler(Setting_LanguageDir, new CSettingTypeApplicationPath("Lang Directory", "Directory", Setting_LanguageDirDefault));

    AddHandler(Rdb_GoodName, new CSettingTypeRomDatabase("Good Name", Game_GameName));
    AddHandler(Rdb_SaveChip, new CSettingTypeRDBSaveChip("Save Type", SaveChip_Auto));
#ifdef _DEBUG
    AddHandler(Rdb_CpuType, new CSettingTypeRDBCpuType("CPU Type", CPU_SyncCores));
#else
    AddHandler(Rdb_CpuType, new CSettingTypeRDBCpuType("CPU Type", CPU_Recompiler));
#endif
    AddHandler(Rdb_RDRamSize, new CSettingTypeRDBRDRamSize("RDRAM Size", 0x400000));
    AddHandler(Rdb_CounterFactor, new CSettingTypeRomDatabase("Counter Factor", 2));
    AddHandler(Rdb_UseTlb, new CSettingTypeRDBYesNo("Use TLB", true));
    AddHandler(Rdb_DelayDP, new CSettingTypeRDBYesNo("Delay DP", true));
    AddHandler(Rdb_DelaySi, new CSettingTypeRDBYesNo("Delay SI", false));
    AddHandler(Rdb_32Bit, new CSettingTypeRDBYesNo("32bit", true));
    AddHandler(Rdb_FastSP, new CSettingTypeRDBYesNo("Fast SP", true));
    AddHandler(Rdb_FixedAudio, new CSettingTypeRomDatabase("Fixed Audio", true));
    AddHandler(Rdb_SyncViaAudio, new CSettingTypeRomDatabase("Sync Audio", false));
    AddHandler(Rdb_RspAudioSignal, new CSettingTypeRDBYesNo("Audio Signal", false));
    AddHandler(Rdb_TLB_VAddrStart, new CSettingTypeRomDatabase("TLB: Vaddr Start", 0));
    AddHandler(Rdb_TLB_VAddrLen, new CSettingTypeRomDatabase("TLB: Vaddr Len", 0));
    AddHandler(Rdb_TLB_PAddrStart, new CSettingTypeRomDatabase("TLB: PAddr Start", 0));
    AddHandler(Rdb_UseHleGfx, new CSettingTypeRomDatabase("HLE GFX", Plugin_UseHleGfx));
    AddHandler(Rdb_UseHleAudio, new CSettingTypeRomDatabase("HLE Audio", Plugin_UseHleAudio));
    AddHandler(Rdb_LoadRomToMemory, new CSettingTypeRomDatabase("Rom In Memory", false));
    AddHandler(Rdb_ScreenHertz, new CSettingTypeRomDatabase("ScreenHertz", 0));
    AddHandler(Rdb_FuncLookupMode, new CSettingTypeRomDatabase("FuncFind", FuncFind_PhysicalLookup));
    AddHandler(Rdb_RegCache, new CSettingTypeRDBYesNo("Reg Cache", true));
    AddHandler(Rdb_BlockLinking, new CSettingTypeRDBOnOff("Linking", true));
    AddHandler(Rdb_SMM_Cache, new CSettingTypeRomDatabase("SMM-Cache", true));
    AddHandler(Rdb_SMM_StoreInstruc, new CSettingTypeRomDatabase("SMM-StoreInstr", false));
    AddHandler(Rdb_SMM_PIDMA, new CSettingTypeRomDatabase("SMM-PI DMA", true));
    AddHandler(Rdb_SMM_TLB, new CSettingTypeRomDatabase("SMM-TLB", true));
    AddHandler(Rdb_SMM_Protect, new CSettingTypeRomDatabase("SMM-Protect", false));
    AddHandler(Rdb_SMM_ValidFunc, new CSettingTypeRomDatabase("SMM-FUNC", true));
    AddHandler(Rdb_GameCheatFix, new CSettingTypeRomDatabaseIndex("Cheat", "", ""));
    AddHandler(Rdb_GameCheatFixPlugin, new CSettingTypeRomDatabaseIndex("CheatPlugin", "", ""));
    AddHandler(Rdb_ViRefreshRate, new CSettingTypeRomDatabase("ViRefresh", 1500));
    AddHandler(Rdb_AiCountPerBytes, new CSettingTypeRomDatabase("AiCountPerBytes", 0));
    AddHandler(Rdb_AudioResetOnLoad, new CSettingTypeRDBYesNo("AudioResetOnLoad", false));
    AddHandler(Rdb_AllowROMWrites, new CSettingTypeRDBYesNo("AllowROMWrites", false));
    AddHandler(Rdb_CRC_Recalc, new CSettingTypeRDBYesNo("CRC-Recalc", false));
    AddHandler(Rdb_OverClockModifier, new CSettingTypeRomDatabase("OverClockModifier", 1));

    AddHandler(Game_IniKey, new CSettingTypeTempString(""));
    AddHandler(Game_File, new CSettingTypeTempString(""));
    AddHandler(Game_UniqueSaveDir, new CSettingTypeTempString(""));
    AddHandler(Game_GameName, new CSettingTypeTempString(""));
    AddHandler(Cfg_GoodName, new CSettingTypeGame("Good Name", ""));
    AddHandler(Game_TempLoaded, new CSettingTypeTempBool(false));
    AddHandler(Game_SystemType, new CSettingTypeTempNumber(SYSTEM_NTSC));
    AddHandler(Game_EditPlugin_Gfx, new CSettingTypeGame("Plugin-Gfx", Default_None));
    AddHandler(Game_EditPlugin_Audio, new CSettingTypeGame("Plugin-Audio", Default_None));
    AddHandler(Game_EditPlugin_Contr, new CSettingTypeGame("Plugin-Controller", Default_None));
    AddHandler(Game_EditPlugin_RSP, new CSettingTypeGame("Plugin-RSP", Default_None));
    AddHandler(Game_Plugin_Gfx, new CSettingTypeGame("Plugin-Gfx", Plugin_GFX_Current));
    AddHandler(Game_Plugin_Audio, new CSettingTypeGame("Plugin-Audio", Plugin_AUDIO_Current));
    AddHandler(Game_Plugin_Controller, new CSettingTypeGame("Plugin-Controller", Plugin_CONT_Current));
    AddHandler(Game_Plugin_RSP, new CSettingTypeGame("Plugin-RSP", Plugin_RSP_Current));
    AddHandler(Game_SaveChip, new CSettingTypeGame("SaveChip", Rdb_SaveChip));
    AddHandler(Game_CpuType, new CSettingTypeGame("CpuType", Rdb_CpuType));
    AddHandler(Game_LastSaveSlot, new CSettingTypeGame("Last Used Save Slot", (uint32_t)0));
    AddHandler(Game_FixedAudio, new CSettingTypeGame("Fixed Audio", Rdb_FixedAudio));
    AddHandler(Game_RDRamSize, new CSettingTypeGame("RDRamSize", Rdb_RDRamSize));
    AddHandler(Game_CounterFactor, new CSettingTypeGame("Counter Factor", Rdb_CounterFactor));
    AddHandler(Game_UseTlb, new CSettingTypeGame("Use TLB", Rdb_UseTlb));
    AddHandler(Game_DelayDP, new CSettingTypeGame("Delay DP", Rdb_DelayDP));
    AddHandler(Game_DelaySI, new CSettingTypeGame("Delay SI", Rdb_DelaySi));
    AddHandler(Game_RspAudioSignal, new CSettingTypeGame("Audio Signal", Rdb_RspAudioSignal));
    AddHandler(Game_32Bit, new CSettingTypeGame("32bit", Rdb_32Bit));
    AddHandler(Game_FastSP, new CSettingTypeGame("Fast SP", Rdb_FastSP));
#ifdef ANDROID
    AddHandler(Game_CurrentSaveState, new CSettingTypeTempNumber(1));
#else
    AddHandler(Game_CurrentSaveState, new CSettingTypeTempNumber(0));
#endif
    AddHandler(Game_LastSaveTime, new CSettingTypeTempNumber(0));
    AddHandler(Game_SyncViaAudio, new CSettingTypeGame("Sync Audio", Rdb_SyncViaAudio));
    AddHandler(Game_UseHleGfx, new CSettingTypeGame("HLE GFX", Rdb_UseHleGfx));
    AddHandler(Game_UseHleAudio, new CSettingTypeGame("HLE Audio", Rdb_UseHleAudio));
    AddHandler(Game_LoadRomToMemory, new CSettingTypeGame("Rom In Memory", Rdb_LoadRomToMemory));
    AddHandler(Game_ScreenHertz, new CSettingTypeGame("ScreenHertz", Rdb_ScreenHertz));
    AddHandler(Game_FuncLookupMode, new CSettingTypeGame("FuncFind", Rdb_FuncLookupMode));
    AddHandler(Game_RegCache, new CSettingTypeGame("Reg Cache", Rdb_RegCache));
    AddHandler(Game_BlockLinking, new CSettingTypeGame("Linking", Rdb_BlockLinking));
    AddHandler(Game_SMM_StoreInstruc, new CSettingTypeGame("SMM-StoreInst", Rdb_SMM_StoreInstruc));
    AddHandler(Game_SMM_Cache, new CSettingTypeGame("SMM-Cache", Rdb_SMM_Cache));
    AddHandler(Game_SMM_PIDMA, new CSettingTypeGame("SMM-PI DMA", Rdb_SMM_PIDMA));
    AddHandler(Game_SMM_TLB, new CSettingTypeGame("SMM-TLB", Rdb_SMM_TLB));
    AddHandler(Game_SMM_Protect, new CSettingTypeGame("SMM-Protect", Rdb_SMM_Protect));
    AddHandler(Game_SMM_ValidFunc, new CSettingTypeGame("SMM-FUNC", Rdb_SMM_ValidFunc));
    AddHandler(Game_ViRefreshRate, new CSettingTypeGame("ViRefresh", Rdb_ViRefreshRate));
    AddHandler(Game_AiCountPerBytes, new CSettingTypeGame("AiCountPerBytes", Rdb_AiCountPerBytes));
    AddHandler(Game_AudioResetOnLoad, new CSettingTypeGame("AudioResetOnLoad", Rdb_AudioResetOnLoad));
    AddHandler(Game_AllowROMWrites, new CSettingTypeGame("AllowROMWrites", Rdb_AllowROMWrites));
    AddHandler(Game_CRC_Recalc, new CSettingTypeGame("CRC-Recalc", Rdb_CRC_Recalc));
    AddHandler(Game_Transferpak_ROM, new CSettingTypeGame("Tpak-ROM-dir", Default_None));
    AddHandler(Game_Transferpak_Sav, new CSettingTypeGame("Tpak-Sav-dir", Default_None));
    AddHandler(Game_LoadSaveAtStart, new CSettingTypeTempBool(false));
    AddHandler(Game_OverClockModifier, new CSettingTypeGame("OverClockModifier", Rdb_OverClockModifier));

    //User Interface
    AddHandler(UserInterface_ShowCPUPer, new CSettingTypeApplication("", "Display CPU Usage", (uint32_t)false));
#ifdef ANDROID
    AddHandler(UserInterface_DisplayFrameRate, new CSettingTypeApplication("", "Display Frame Rate", (uint32_t)false));
#else
    AddHandler(UserInterface_DisplayFrameRate, new CSettingTypeApplication("", "Display Frame Rate", (uint32_t)true));
#endif
    AddHandler(UserInterface_FrameDisplayType, new CSettingTypeApplication("", "Frame Rate Display Type", (uint32_t)FR_VIs));
    AddHandler(Directory_Plugin, new CSettingTypeSelectedDirectory("Dir:Plugin", Directory_PluginInitial, Directory_PluginSelected, Directory_PluginUseSelected, Directory_Plugin));
#ifndef _M_X64
    AddHandler(Directory_PluginInitial, new CSettingTypeRelativePath("Plugin", ""));
    AddHandler(Directory_PluginSelected, new CSettingTypeApplicationPath("Plugin Directory", "Directory", Directory_PluginInitial));
    AddHandler(Directory_PluginUseSelected, new CSettingTypeApplication("Plugin Directory", "Use Selected", false));

    AddHandler(Directory_PluginSyncInitial, new CSettingTypeRelativePath("SyncPlugin", ""));
    AddHandler(Directory_PluginSyncSelected, new CSettingTypeApplicationPath("Sync Plugin Directory", "Directory", Directory_PluginInitial));
    AddHandler(Directory_PluginSyncUseSelected, new CSettingTypeApplication("Sync Plugin Directory", "Use Selected", false));

#else
    AddHandler(Directory_PluginInitial, new CSettingTypeRelativePath("Plugin64", ""));
    AddHandler(Directory_PluginSelected, new CSettingTypeApplicationPath("Plugin64 Directory", "Directory", Directory_PluginInitial));
    AddHandler(Directory_PluginUseSelected, new CSettingTypeApplication("Plugin64 Directory", "Use Selected", false));
    AddHandler(Directory_PluginSync, new CSettingTypeRelativePath("SyncPlugin64", ""));

    AddHandler(Directory_PluginSyncInitial, new CSettingTypeRelativePath("SyncPlugin64", ""));
    AddHandler(Directory_PluginSyncSelected, new CSettingTypeApplicationPath("Sync Plugin Directory64", "Directory", Directory_PluginInitial));
    AddHandler(Directory_PluginSyncUseSelected, new CSettingTypeApplication("Sync Plugin Directory64", "Use Selected", false));

#endif
    AddHandler(Directory_PluginSync, new CSettingTypeSelectedDirectory("Dir:SyncPlugin", Directory_PluginSyncInitial, Directory_PluginSyncSelected, Directory_PluginSyncUseSelected, Directory_PluginSync));

    AddHandler(Directory_SnapShot, new CSettingTypeSelectedDirectory("Dir:Snapshot", Directory_SnapShotInitial, Directory_SnapShotSelected, Directory_SnapShotUseSelected, Directory_SnapShot));
    AddHandler(Directory_SnapShotInitial, new CSettingTypeRelativePath("Screenshots", ""));
    AddHandler(Directory_SnapShotSelected, new CSettingTypeApplicationPath("Snap Shot Directory", "Directory", Directory_SnapShotInitial));
    AddHandler(Directory_SnapShotUseSelected, new CSettingTypeApplication("Snap Shot Directory", "Use Selected", false));

    AddHandler(Directory_NativeSave, new CSettingTypeSelectedDirectory("Dir:NativeSave", Directory_NativeSaveInitial, Directory_NativeSaveSelected, Directory_NativeSaveUseSelected, Directory_NativeSave));
    AddHandler(Directory_NativeSaveInitial, new CSettingTypeRelativePath("Save", ""));
    AddHandler(Directory_NativeSaveSelected, new CSettingTypeApplicationPath("Native Save Directory", "Directory", Directory_NativeSaveInitial));
    AddHandler(Directory_NativeSaveUseSelected, new CSettingTypeApplication("Native Save Directory", "Use Selected", false));

    AddHandler(Directory_InstantSave, new CSettingTypeSelectedDirectory("Dir:InstantSave", Directory_InstantSaveInitial, Directory_InstantSaveSelected, Directory_InstantSaveUseSelected, Directory_InstantSave));
    AddHandler(Directory_InstantSaveInitial, new CSettingTypeRelativePath("Save", ""));
    AddHandler(Directory_InstantSaveSelected, new CSettingTypeApplicationPath("Instant Save Directory", "Directory", Directory_InstantSaveInitial));
    AddHandler(Directory_InstantSaveUseSelected, new CSettingTypeApplication("Instant Save Directory", "Use Selected", false));

    AddHandler(Directory_Texture, new CSettingTypeSelectedDirectory("Dir:Texture", Directory_TextureInitial, Directory_TextureSelected, Directory_TextureUseSelected, Directory_Texture));
    AddHandler(Directory_TextureInitial, new CSettingTypeRelativePath("Textures", ""));
    AddHandler(Directory_TextureSelected, new CSettingTypeApplicationPath("Texture Directory", "Directory", Directory_InstantSaveInitial));
    AddHandler(Directory_TextureUseSelected, new CSettingTypeApplication("Texture Directory", "Use Selected", false));

    AddHandler(Directory_Log, new CSettingTypeSelectedDirectory("Dir:Log", Directory_LogInitial, Directory_LogSelected, Directory_LogUseSelected, Directory_Log));
    AddHandler(Directory_LogInitial, new CSettingTypeRelativePath("Logs", ""));
    AddHandler(Directory_LogSelected, new CSettingTypeApplicationPath("Log Directory", "Directory", Directory_InstantSaveInitial));
    AddHandler(Directory_LogUseSelected, new CSettingTypeApplication("Log Directory", "Use Selected", false));

    AddHandler(RomList_RomListCacheDefault, new CSettingTypeRelativePath("Config", "Project64.cache3"));
    AddHandler(RomList_RomListCache, new CSettingTypeApplicationPath("", "RomListCache", RomList_RomListCacheDefault));
    AddHandler(RomList_GameDir, new CSettingTypeSelectedDirectory("Dir:Game", RomList_GameDirInitial, RomList_GameDirSelected, RomList_GameDirUseSelected, RomList_GameDir));
    AddHandler(RomList_GameDirInitial, new CSettingTypeRelativePath("Game Directory", ""));
    AddHandler(RomList_GameDirSelected, new CSettingTypeApplication("Game Directory", "Directory", RomList_GameDirInitial));
    AddHandler(RomList_GameDirUseSelected, new CSettingTypeApplication("Game Directory", "Use Selected", false));
    AddHandler(RomList_GameDirRecursive, new CSettingTypeApplication("Game Directory", "Recursive", false));
    AddHandler(RomList_7zipCache, new CSettingTypeApplicationPath("", "7zipCache", RomList_7zipCacheDefault));
    AddHandler(RomList_7zipCacheDefault, new CSettingTypeRelativePath("Config", "Project64.zcache"));

    AddHandler(GameRunning_LoadingInProgress, new CSettingTypeTempBool(false));
    AddHandler(GameRunning_CPU_Running, new CSettingTypeTempBool(false));
    AddHandler(GameRunning_CPU_Paused, new CSettingTypeTempBool(false));
    AddHandler(GameRunning_CPU_PausedType, new CSettingTypeTempNumber(Default_None));
    AddHandler(GameRunning_InstantSaveFile, new CSettingTypeTempString(""));
    AddHandler(GameRunning_LimitFPS, new CSettingTypeTempBool(true));
    AddHandler(GameRunning_ScreenHertz, new CSettingTypeTempNumber(60));
    AddHandler(GameRunning_InReset, new CSettingTypeTempBool(false));

    AddHandler(UserInterface_BasicMode, new CSettingTypeApplication("", "Basic Mode", (uint32_t)true));
    AddHandler(File_DiskIPLPath, new CSettingTypeApplicationPath("", "Disk IPL ROM Path", Default_None));

    AddHandler(Debugger_Enabled, new CSettingTypeApplication("Debugger", "Debugger", false));
    AddHandler(Debugger_ShowTLBMisses, new CSettingTypeApplication("Debugger", "Show TLB Misses", false));
    AddHandler(Debugger_ShowUnhandledMemory, new CSettingTypeApplication("Debugger", "Show Unhandled Memory", false));
    AddHandler(Debugger_ShowPifErrors, new CSettingTypeApplication("Debugger", "Show Pif Errors", false));
    AddHandler(Debugger_DisableGameFixes, new CSettingTypeApplication("Debugger", "Disable Game Fixes", false));
    AddHandler(Debugger_ShowDListAListCount, new CSettingTypeApplication("Debugger", "Show Dlist Alist Count", false));
    AddHandler(Debugger_ShowRecompMemSize, new CSettingTypeApplication("Debugger", "Show Recompiler Memory size", false));
    AddHandler(Debugger_RecordExecutionTimes, new CSettingTypeApplication("Debugger", "Record Execution Times", false));
    AddHandler(Debugger_DebugLanguage, new CSettingTypeApplication("Debugger", "Debug Language", false));
    AddHandler(Debugger_ShowDivByZero, new CSettingTypeApplication("Debugger", "Show Div by zero", false));
    AddHandler(Debugger_AppLogFlush, new CSettingTypeApplication("Logging", "Log Auto Flush", (uint32_t)false));
    AddHandler(Debugger_RecordRecompilerAsm, new CSettingTypeApplication("Debugger", "Record Recompiler Asm", false));

    //Logging
    AddHandler(Debugger_TraceMD5, new CSettingTypeApplication("Logging", "MD5", (uint32_t)g_ModuleLogLevel[TraceMD5]));
    AddHandler(Debugger_TraceThread, new CSettingTypeApplication("Logging", "Thread", (uint32_t)g_ModuleLogLevel[TraceThread]));
    AddHandler(Debugger_TracePath, new CSettingTypeApplication("Logging", "Path", (uint32_t)g_ModuleLogLevel[TracePath]));
    AddHandler(Debugger_TraceSettings, new CSettingTypeApplication("Logging", "Settings", (uint32_t)g_ModuleLogLevel[TraceSettings]));
    AddHandler(Debugger_TraceUnknown, new CSettingTypeApplication("Logging", "Unknown", (uint32_t)g_ModuleLogLevel[TraceUnknown]));
    AddHandler(Debugger_TraceAppInit, new CSettingTypeApplication("Logging", "App Init", (uint32_t)g_ModuleLogLevel[TraceAppInit]));
    AddHandler(Debugger_TraceAppCleanup, new CSettingTypeApplication("Logging", "App Cleanup", (uint32_t)g_ModuleLogLevel[TraceAppCleanup]));
    AddHandler(Debugger_TraceN64System, new CSettingTypeApplication("Logging", "N64 System", (uint32_t)g_ModuleLogLevel[TraceN64System]));
    AddHandler(Debugger_TracePlugins, new CSettingTypeApplication("Logging", "Plugins", (uint32_t)g_ModuleLogLevel[TracePlugins]));
    AddHandler(Debugger_TraceGFXPlugin, new CSettingTypeApplication("Logging", "GFX Plugin", (uint32_t)g_ModuleLogLevel[TraceGFXPlugin]));
    AddHandler(Debugger_TraceAudioPlugin, new CSettingTypeApplication("Logging", "Audio Plugin", (uint32_t)g_ModuleLogLevel[TraceAudioPlugin]));
    AddHandler(Debugger_TraceControllerPlugin, new CSettingTypeApplication("Logging", "Controller Plugin", (uint32_t)g_ModuleLogLevel[TraceControllerPlugin]));
    AddHandler(Debugger_TraceRSPPlugin, new CSettingTypeApplication("Logging", "RSP Plugin", (uint32_t)g_ModuleLogLevel[TraceRSPPlugin]));
    AddHandler(Debugger_TraceRSP, new CSettingTypeApplication("Logging", "RSP", (uint32_t)g_ModuleLogLevel[TraceRSP]));
    AddHandler(Debugger_TraceAudio, new CSettingTypeApplication("Logging", "Audio", (uint32_t)g_ModuleLogLevel[TraceAudio]));
    AddHandler(Debugger_TraceRegisterCache, new CSettingTypeApplication("Logging", "Register Cache", (uint32_t)g_ModuleLogLevel[TraceRegisterCache]));
    AddHandler(Debugger_TraceRecompiler, new CSettingTypeApplication("Logging", "Recompiler", (uint32_t)g_ModuleLogLevel[TraceRecompiler]));
    AddHandler(Debugger_TraceTLB, new CSettingTypeApplication("Logging", "TLB", (uint32_t)g_ModuleLogLevel[TraceTLB]));
    AddHandler(Debugger_TraceProtectedMEM, new CSettingTypeApplication("Logging", "Protected MEM", (uint32_t)g_ModuleLogLevel[TraceProtectedMem]));
    AddHandler(Debugger_TraceUserInterface, new CSettingTypeApplication("Logging", "User Interface", (uint32_t)g_ModuleLogLevel[TraceUserInterface]));
    AddHandler(Debugger_TraceRomList, new CSettingTypeApplication("Logging", "Rom List", (uint32_t)g_ModuleLogLevel[TraceRomList]));
    AddHandler(Debugger_TraceExceptionHandler, new CSettingTypeApplication("Logging", "Exception Handler", (uint32_t)g_ModuleLogLevel[TraceExceptionHandler]));

    //Plugin
#ifdef _WIN32
    AddHandler(Plugin_RSP_Current, new CSettingTypeApplication("Plugin", "RSP Dll", "RSP\\RSP 1.7.dll"));
    AddHandler(Plugin_GFX_Current, new CSettingTypeApplication("Plugin", "Graphics Dll", "GFX\\Jabo_Direct3D8.dll"));
    AddHandler(Plugin_AUDIO_Current, new CSettingTypeApplication("Plugin", "Audio Dll", "Audio\\Jabo_Dsound.dll"));
    AddHandler(Plugin_CONT_Current, new CSettingTypeApplication("Plugin", "Controller Dll", "Input\\PJ64_NRage.dll"));
#else
    AddHandler(Plugin_RSP_Current, new CSettingTypeApplication("Plugin", "RSP Dll", "libProject64-rsp-hle.so"));
    AddHandler(Plugin_GFX_Current, new CSettingTypeApplication("Plugin", "Graphics Dll", "libProject64-gfx.so"));
    AddHandler(Plugin_AUDIO_Current, new CSettingTypeApplication("Plugin", "Audio Dll", "libProject64-audio-android.so"));
    AddHandler(Plugin_CONT_Current, new CSettingTypeApplication("Plugin", "Controller Dll", "libProject64-input-android.so"));
#endif
    AddHandler(Plugin_RSP_CurVer, new CSettingTypeApplication("Plugin", "RSP Dll Ver", ""));
    AddHandler(Plugin_GFX_CurVer, new CSettingTypeApplication("Plugin", "Graphics Dll Ver", ""));
    AddHandler(Plugin_AUDIO_CurVer, new CSettingTypeApplication("Plugin", "Audio Dll Ver", ""));
    AddHandler(Plugin_CONT_CurVer, new CSettingTypeApplication("Plugin", "Controller Dll Ver", ""));

    AddHandler(Plugin_UseHleGfx, new CSettingTypeApplication("RSP", "HLE GFX", true));
    AddHandler(Plugin_UseHleAudio, new CSettingTypeApplication("RSP", "HLE Audio", false));
    AddHandler(Plugin_EnableAudio, new CSettingTypeApplication("Audio", "Enable Audio", true));

    //Logging
    AddHandler(Logging_GenerateLog, new CSettingTypeApplication("Logging", "Generate Log Files", false));
    AddHandler(Logging_LogRDRamRegisters, new CSettingTypeApplication("Logging", "Log RDRam Registers", false));
    AddHandler(Logging_LogSPRegisters, new CSettingTypeApplication("Logging", "Log SP Registers", false));
    AddHandler(Logging_LogDPCRegisters, new CSettingTypeApplication("Logging", "Log DPC Registers", false));
    AddHandler(Logging_LogDPSRegisters, new CSettingTypeApplication("Logging", "Log DPS Registers", false));
    AddHandler(Logging_LogMIPSInterface, new CSettingTypeApplication("Logging", "Log MIPS Interface", false));
    AddHandler(Logging_LogVideoInterface, new CSettingTypeApplication("Logging", "Log Video Interface", false));
    AddHandler(Logging_LogAudioInterface, new CSettingTypeApplication("Logging", "Log Audio Interface", false));
    AddHandler(Logging_LogPerInterface, new CSettingTypeApplication("Logging", "Log Per Interface", false));
    AddHandler(Logging_LogRDRAMInterface, new CSettingTypeApplication("Logging", "Log RDRAM Interface", false));
    AddHandler(Logging_LogSerialInterface, new CSettingTypeApplication("Logging", "Log Serial Interface", false));
    AddHandler(Logging_LogPRDMAOperations, new CSettingTypeApplication("Logging", "Log PR DMA Operations", false));
    AddHandler(Logging_LogPRDirectMemLoads, new CSettingTypeApplication("Logging", "Log PR Direct Mem Loads", false));
    AddHandler(Logging_LogPRDMAMemLoads, new CSettingTypeApplication("Logging", "Log PR DMA Mem Loads", false));
    AddHandler(Logging_LogPRDirectMemStores, new CSettingTypeApplication("Logging", "Log PR Direct Mem Stores", false));
    AddHandler(Logging_LogPRDMAMemStores, new CSettingTypeApplication("Logging", "Log PRDMA Mem Stores", false));
    AddHandler(Logging_LogControllerPak, new CSettingTypeApplication("Logging", "Log Controller Pak", false));
    AddHandler(Logging_LogCP0changes, new CSettingTypeApplication("Logging", "Log CP0 changes", false));
    AddHandler(Logging_LogCP0reads, new CSettingTypeApplication("Logging", "Log CP0 reads", false));
    AddHandler(Logging_LogTLB, new CSettingTypeApplication("Logging", "Log TLB", false));
    AddHandler(Logging_LogExceptions, new CSettingTypeApplication("Logging", "Log Exceptions", false));
    AddHandler(Logging_NoInterrupts, new CSettingTypeApplication("Logging", "No Interrupts", false));
    AddHandler(Logging_LogCache, new CSettingTypeApplication("Logging", "Log Cache", false));
    AddHandler(Logging_LogRomHeader, new CSettingTypeApplication("Logging", "Generate Log Files", false));
    AddHandler(Logging_LogUnknown, new CSettingTypeApplication("Logging", "Log Rom Header", false));

    // cheats
    AddHandler(Cheat_Entry, new CSettingTypeCheats(""));
    AddHandler(Cheat_Active, new CSettingTypeGameIndex("Cheat", "", (uint32_t)false));
    AddHandler(Cheat_Extension, new CSettingTypeGameIndex("Cheat", ".exten", "??? - Not Set"));
    AddHandler(Cheat_Notes, new CSettingTypeCheats("_N"));
    AddHandler(Cheat_Options, new CSettingTypeCheats("_O"));
    AddHandler(Cheat_Range, new CSettingTypeCheats("_R"));
    AddHandler(Cheat_RangeNotes, new CSettingTypeCheats("_RN"));

    WriteTrace(TraceAppInit, TraceDebug, "Done");
}

uint32_t CSettings::FindSetting(CSettings * _this, const char * Name)
{
    for (SETTING_MAP::iterator iter = _this->m_SettingInfo.begin(); iter != _this->m_SettingInfo.end(); iter++)
    {
        CSettingType * Setting = iter->second;
        if (Setting->GetSettingType() == SettingType_GameSetting)
        {
            CSettingTypeGame * GameSetting = (CSettingTypeGame *)Setting;
            if (_stricmp(GameSetting->GetKeyName(), Name) != 0)
            {
                continue;
            }
            return iter->first;
        }
        if (Setting->GetSettingType() == SettingType_CfgFile)
        {
            CSettingTypeApplication * CfgSetting = (CSettingTypeApplication *)Setting;
            if (_stricmp(CfgSetting->GetKeyName(), Name) != 0)
            {
                continue;
            }
            return iter->first;
        }
        if (Setting->GetSettingType() == SettingType_SelectedDirectory)
        {
            CSettingTypeSelectedDirectory * SelectedDirectory = (CSettingTypeSelectedDirectory *)Setting;
            if (_stricmp(SelectedDirectory->GetName(), Name) != 0)
            {
                continue;
            }
            return iter->first;
        }
    }
    return 0;
}

void CSettings::FlushSettings(CSettings * /*_this*/)
{
    CSettingTypeCheats::FlushChanges();
    CSettingTypeApplication::Flush();
}

void CSettings::sRegisterChangeCB(CSettings * _this, SettingID Type, void * Data, SettingChangedFunc Func)
{
    _this->RegisterChangeCB(Type, Data, Func);
}

void CSettings::sUnregisterChangeCB(CSettings * _this, SettingID Type, void * Data, SettingChangedFunc Func)
{
    _this->UnregisterChangeCB(Type, Data, Func);
}

uint32_t CSettings::GetSetting(CSettings * _this, SettingID Type)
{
    return _this->LoadDword(Type);
}

const char * CSettings::GetSettingSz(CSettings * _this, SettingID Type, char * Buffer, int BufferSize)
{
    if (Buffer && BufferSize > 0)
    {
        Buffer[0] = 0;
        _this->LoadStringVal(Type, Buffer, BufferSize);
    }
    return Buffer;
}

void CSettings::SetSetting(CSettings * _this, SettingID ID, uint32_t Value)
{
    _this->SaveDword(ID, Value);
}

void CSettings::SetSettingSz(CSettings * _this, SettingID ID, const char * Value)
{
    _this->SaveString(ID, Value);
}

void CSettings::RegisterSetting(CSettings * _this, SettingID ID, SettingID DefaultID, SettingDataType DataType,
    SettingType Type, const char * Category, const char * DefaultStr,
    uint32_t Value)
{
    SettingID RdbSetting;
    stdstr Name;

    switch (Type)
    {
    case SettingType_ConstValue:
        if (DataType != Data_DWORD)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        _this->AddHandler(ID, new CSettingTypeTempNumber(Value));
        break;
    case SettingType_ConstString:
        if (DataType != Data_String)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        _this->AddHandler(ID, new CSettingTypeTempString(DefaultStr));
        break;
    case SettingType_CfgFile:
    case SettingType_Registry:
        switch (DataType)
        {
        case Data_DWORD:
            if (DefaultID == Default_None)
            {
                _this->AddHandler(ID, new CSettingTypeApplication(Category, DefaultStr, Value));
            }
            else
            {
                _this->AddHandler(ID, new CSettingTypeApplication(Category, DefaultStr, DefaultID));
            }
            break;
        case Data_String:
            if (DefaultID == Default_None)
            {
                _this->AddHandler(ID, new CSettingTypeApplication(Category, DefaultStr, ""));
            }
            else
            {
                _this->AddHandler(ID, new CSettingTypeApplication(Category, DefaultStr, DefaultID));
            }
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    case SettingType_GameSetting:
        Name.Format("%s-%s", Category, DefaultStr);
        switch (DataType)
        {
        case Data_DWORD:
            RdbSetting = (SettingID)_this->m_NextAutoSettingId;
            _this->m_NextAutoSettingId += 1;
            if (DefaultID == Default_None)
            {
                _this->AddHandler(RdbSetting, new CSettingTypeRomDatabase(Name.c_str(), (int)Value));
                _this->AddHandler(ID, new CSettingTypeGame(Name.c_str(), RdbSetting));
            }
            else
            {
                _this->AddHandler(RdbSetting, new CSettingTypeRomDatabase(Name.c_str(), DefaultID));
                _this->AddHandler(ID, new CSettingTypeGame(Name.c_str(), RdbSetting));
            }
            break;
        case Data_String:
            RdbSetting = (SettingID)_this->m_NextAutoSettingId;
            _this->m_NextAutoSettingId += 1;
            if (DefaultID == Default_None)
            {
                _this->AddHandler(RdbSetting, new CSettingTypeRomDatabase(Name.c_str(), ""));
                _this->AddHandler(ID, new CSettingTypeGame(Name.c_str(), RdbSetting));
            }
            else
            {
                _this->AddHandler(RdbSetting, new CSettingTypeRomDatabase(Name.c_str(), DefaultID));
                _this->AddHandler(ID, new CSettingTypeGame(Name.c_str(), RdbSetting));
            }
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    case SettingType_RomDatabase:
        switch (DataType)
        {
        case Data_DWORD:
            if (DefaultID == Default_None)
            {
                _this->AddHandler(ID, new CSettingTypeRomDatabase(DefaultStr, (int)Value, true));
            }
            else
            {
                _this->AddHandler(ID, new CSettingTypeRomDatabase(DefaultStr, (SettingID)Value, true));
            }
            break;
        case Data_String:
            if (DefaultID == Default_None)
            {
                _this->AddHandler(ID, new CSettingTypeRomDatabase(DefaultStr, "", true));
            }
            else
            {
                _this->AddHandler(ID, new CSettingTypeRomDatabase(DefaultStr, DefaultID, true));
            }
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    case SettingType_RdbSetting:
        switch (DataType)
        {
        case Data_DWORD:
            if (DefaultID == Default_None)
            {
                _this->AddHandler(ID, new CSettingTypeRomDatabaseSetting(Category, DefaultStr, (int)Value, true));
            }
            else
            {
                SettingID AutoRdbSetting = (SettingID)_this->m_NextAutoSettingId;
                _this->m_NextAutoSettingId += 1;
                _this->AddHandler(AutoRdbSetting, new CSettingTypeRomDatabaseSetting(Category, DefaultStr, DefaultID, true));
                _this->AddHandler(ID, new CSettingTypeApplication(Category, DefaultStr, AutoRdbSetting));
            }
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

bool CSettings::Initialize(const char * BaseDirectory, const char * AppName)
{
    WriteTrace(TraceAppInit, TraceDebug, "Start");
    AddHowToHandleSetting(BaseDirectory);
    CSettingTypeApplication::Initialize(AppName);
    CSettingTypeRomDatabase::Initialize();
    CSettingTypeGame::Initialize();
    CSettingTypeCheats::Initialize();

    g_Settings->SaveString(Setting_ApplicationName, AppName);
    WriteTrace(TraceAppInit, TraceDebug, "Done");
    return true;
}

bool CSettings::LoadBool(SettingID Type)
{
    bool Value = false;
    LoadBool(Type, Value);
    return Value;
}

bool CSettings::LoadBool(SettingID Type, bool & Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return 0;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        return FindInfo->second->Load(0, Value);
    }
    return false;
}

bool CSettings::LoadBoolIndex(SettingID Type, int index)
{
    bool Value = false;
    LoadBoolIndex(Type, index, Value);
    return Value;
}

bool CSettings::LoadBoolIndex(SettingID Type, int index, bool & Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return false;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        return FindInfo->second->Load(index, Value);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return false;
}

uint32_t CSettings::LoadDword(SettingID Type)
{
    uint32_t Value = 0;
    LoadDword(Type, Value);
    return Value;
}

bool CSettings::LoadDword(SettingID Type, uint32_t & Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return false;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        return FindInfo->second->Load(0, Value);
    }
    return false;
}

uint32_t CSettings::LoadDwordIndex(SettingID Type, int index)
{
    uint32_t Value;
    LoadDwordIndex(Type, index, Value);
    return Value;
}

bool CSettings::LoadDwordIndex(SettingID Type, int index, uint32_t & Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return 0;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        return FindInfo->second->Load(index, Value);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return false;
}

stdstr CSettings::LoadStringVal(SettingID Type)
{
    stdstr Value;
    LoadStringVal(Type, Value);
    return Value;
}

bool CSettings::LoadStringVal(SettingID Type, stdstr & Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return 0;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        return FindInfo->second->Load(0, Value);
    }
    return false;
}

bool CSettings::LoadStringVal(SettingID Type, char * Buffer, int BufferSize)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return 0;
    }
    bool bRes = false;
    if (FindInfo->second->IndexBasedSetting())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        stdstr Value;
        bRes = FindInfo->second->Load(0, Value);
        int len = BufferSize;
        if ((Value.length() + 1) < (size_t)len)
        {
            len = Value.length() + 1;
        }
        strncpy(Buffer, Value.c_str(), len);
    }
    return bRes;
}

stdstr CSettings::LoadStringIndex(SettingID Type, int index)
{
    stdstr Value;
    LoadStringIndex(Type, index, Value);
    return Value;
}

bool CSettings::LoadStringIndex(SettingID Type, int index, stdstr & Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return 0;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        return FindInfo->second->Load(index, Value);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return false;
}

bool CSettings::LoadStringIndex(SettingID /*Type*/, int /*index*/, char * /*Buffer*/, int /*BufferSize*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

//Load the default value for the setting
bool CSettings::LoadDefaultBool(SettingID Type)
{
    bool Value = false;
    LoadDefaultBool(Type, Value);
    return Value;
}

void CSettings::LoadDefaultBool(SettingID Type, bool & Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
    }
    else
    {
        if (FindInfo->second->IndexBasedSetting())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            FindInfo->second->LoadDefault(0, Value);
        }
    }
}

bool CSettings::LoadDefaultBoolIndex(SettingID /*Type*/, int /*index*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

void CSettings::LoadDefaultBoolIndex(SettingID /*Type*/, int /*index*/, bool & /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

uint32_t  CSettings::LoadDefaultDword(SettingID Type)
{
    uint32_t Value = 0;
    LoadDefaultDword(Type, Value);
    return Value;
}

void CSettings::LoadDefaultDword(SettingID Type, uint32_t & Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
    }
    else
    {
        if (FindInfo->second->IndexBasedSetting())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            FindInfo->second->LoadDefault(0, Value);
        }
    }
}

uint32_t  CSettings::LoadDefaultDwordIndex(SettingID /*Type*/, int /*index*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

void CSettings::LoadDefaultDwordIndex(SettingID /*Type*/, int /*index*/, uint32_t & /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

stdstr CSettings::LoadDefaultString(SettingID Type)
{
    stdstr Value;
    LoadDefaultString(Type, Value);
    return Value;
}

void CSettings::LoadDefaultString(SettingID Type, stdstr & Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
    }
    else
    {
        if (FindInfo->second->IndexBasedSetting())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            FindInfo->second->LoadDefault(0, Value);
        }
    }
}

void CSettings::LoadDefaultString(SettingID /*Type*/, char * /*Buffer*/, int /*BufferSize*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

stdstr CSettings::LoadDefaultStringIndex(SettingID /*Type*/, int /*index*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return "";
}

void CSettings::LoadDefaultStringIndex(SettingID /*Type*/, int /*index*/, stdstr & /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettings::LoadDefaultStringIndex(SettingID /*Type*/, int /*index*/, char * /*Buffer*/, int /*BufferSize*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettings::SaveBool(SettingID Type, bool Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        FindInfo->second->Save(0, Value);
    }
    NotifyCallBacks(Type);
}

void CSettings::SaveBoolIndex(SettingID Type, int index, bool Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        FindInfo->second->Save(index, Value);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    NotifyCallBacks(Type);
}

void CSettings::SaveDword(SettingID Type, uint32_t Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        FindInfo->second->Save(0, Value);
    }
    NotifyCallBacks(Type);
}

void CSettings::SaveDwordIndex(SettingID Type, int index, uint32_t Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        FindInfo->second->Save(index, Value);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    NotifyCallBacks(Type);
}

void CSettings::SaveString(SettingID Type, const stdstr & Value)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
        return;
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        FindInfo->second->Save(0, Value);
    }
    NotifyCallBacks(Type);
}

void CSettings::SaveString(SettingID Type, const char * Buffer)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
    }
    else if (FindInfo->second->IndexBasedSetting())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        FindInfo->second->Save(0, Buffer);
    }
    NotifyCallBacks(Type);
}

void CSettings::SaveStringIndex(SettingID Type, int index, const char * Buffer)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        FindInfo->second->Save(index, Buffer);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    NotifyCallBacks(Type);
}

void CSettings::SaveStringIndex(SettingID Type, int index, const stdstr & Value)
{
    SaveStringIndex(Type, index, Value.c_str());
}

void CSettings::DeleteSetting(SettingID Type)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        FindInfo->second->Delete(0);
    }
    NotifyCallBacks(Type);
}

void CSettings::DeleteSettingIndex(SettingID Type, int index)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        //if not found do nothing
        UnknownSetting(Type);
    }
    if (FindInfo->second->IndexBasedSetting())
    {
        FindInfo->second->Delete(index);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    NotifyCallBacks(Type);
}

SettingType CSettings::GetSettingType(SettingID Type)
{
    if (Type == Default_None || Type == Default_Constant)
    {
        return SettingType_Unknown;
    }

    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        return SettingType_Unknown;
    }
    return FindInfo->second->GetSettingType();
}

bool CSettings::IndexBasedSetting(SettingID Type)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        return false;
    }
    return FindInfo->second->IndexBasedSetting();
}

void CSettings::SettingTypeChanged(SettingType Type)
{
    for (SETTING_MAP::iterator iter = m_SettingInfo.begin(); iter != m_SettingInfo.end(); iter++)
    {
        if (iter->second->GetSettingType() == Type)
        {
            NotifyCallBacks(iter->first);
        }
    }
}

bool CSettings::IsSettingSet(SettingID Type)
{
    SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
    if (FindInfo == m_SettingInfo.end())
    {
        return false;
    }
    return FindInfo->second->IsSettingSet();
}

void CSettings::UnknownSetting(SettingID /*Type*/)
{
#ifdef _DEBUG
    g_Notify->BreakPoint(__FILE__, __LINE__);
#endif
}

void CSettings::NotifyCallBacks(SettingID Type)
{
    SETTING_CALLBACK::iterator Callback = m_Callback.find(Type);
    if (Callback == m_Callback.end())
    {
        return;
    }

    for (SETTING_CHANGED_CB * item = Callback->second; item != NULL; item = item->Next)
    {
        item->Func(item->Data);
    }
}

void CSettings::RegisterChangeCB(SettingID Type, void * Data, SettingChangedFunc Func)
{
    SETTING_CHANGED_CB * new_item = new SETTING_CHANGED_CB;
    new_item->Data = Data;
    new_item->Func = Func;
    new_item->Next = NULL;

    SETTING_CALLBACK::iterator Callback = m_Callback.find(Type);
    if (Callback != m_Callback.end())
    {
        SETTING_CHANGED_CB * item = Callback->second;
        while (item->Next)
        {
            item = item->Next;
        }
        item->Next = new_item;
    }
    else
    {
        m_Callback.insert(SETTING_CALLBACK::value_type(Type, new_item));
    }
}

void CSettings::UnregisterChangeCB(SettingID Type, void * Data, SettingChangedFunc Func)
{
    bool bRemoved = false;

    //Find out the information for handling the setting type from the list
    SETTING_CALLBACK::iterator Callback = m_Callback.find(Type);
    if (Callback != m_Callback.end())
    {
        SETTING_CHANGED_CB * PrevItem = NULL;
        SETTING_CHANGED_CB * item = Callback->second;

        while (item)
        {
            if (Callback->first == Type && item->Data == Data && item->Func == Func)
            {
                bRemoved = true;
                if (PrevItem == NULL)
                {
                    if (item->Next)
                    {
                        SettingID CallbackType = Callback->first;
                        SETTING_CHANGED_CB * Next = item->Next;
                        m_Callback.erase(Callback);
                        m_Callback.insert(SETTING_CALLBACK::value_type(CallbackType, Next));
                    }
                    else
                    {
                        m_Callback.erase(Callback);
                    }
                }
                else
                {
                    PrevItem->Next = item->Next;
                }
                delete item;
                item = NULL;
                break;
            }
            PrevItem = item;
            item = item->Next;
        }
    }
    else
    {
        UnknownSetting(Type);
        return;
    }

    if (!bRemoved)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}