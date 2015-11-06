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
#pragma once

enum
{
	MaxPluginSetting = 65535
};

enum SettingID
{
	//Default values
	Default_None,
	Default_Constant,

	//information - temp keys
	Info_ShortCutsChanged,

	//Support Files
	SupportFile_Settings,
	SupportFile_SettingsDefault,
	SupportFile_RomDatabase,
	SupportFile_RomDatabaseDefault,
	SupportFile_Glide64RDB,
	SupportFile_Glide64RDBDefault,
	SupportFile_Cheats,
	SupportFile_CheatsDefault,
	SupportFile_Notes,
	SupportFile_NotesDefault,
	SupportFile_ExtInfo,
	SupportFile_ExtInfoDefault,
	SupportFile_ShortCuts,
	SupportFile_ShortCutsDefault,
	SupportFile_RomListCache,
	SupportFile_RomListCacheDefault,
	SupportFile_7zipCache,
	SupportFile_7zipCacheDefault,

	//Settings
	Setting_ApplicationName,
	Setting_UseFromRegistry,
	Setting_RdbEditor,
	Setting_PluginPageFirst,
	Setting_DisableScrSaver,
	Setting_AutoSleep,
	Setting_AutoStart,
	Setting_AutoFullscreen,
	Setting_CheckEmuRunning,
	Setting_EraseGameDefaults,

	Setting_AutoZipInstantSave,
	Setting_RememberCheats,
	Setting_LanguageDir,
	Setting_LanguageDirDefault,
	Setting_CurrentLanguage,

	//RDB TLB Settings
	Rdb_GoodName,
	Rdb_SaveChip,
	Rdb_CpuType,
	Rdb_RDRamSize,
	Rdb_CounterFactor,
	Rdb_UseTlb,
	Rdb_DelayDP,
	Rdb_DelaySi,
	Rdb_32Bit,
	Rdb_FastSP,
	Rdb_Status,
	Rdb_NotesCore,
	Rdb_NotesPlugin,
	Rdb_FixedAudio,
	Rdb_SyncViaAudio,
	Rdb_RspAudioSignal,
	Rdb_TLB_VAddrStart,
	Rdb_TLB_VAddrLen,
	Rdb_TLB_PAddrStart,
	Rdb_UseHleGfx,
	Rdb_UseHleAudio,
	Rdb_LoadRomToMemory,
	Rdb_ScreenHertz,
	Rdb_FuncLookupMode,
	Rdb_RegCache,
	Rdb_BlockLinking,
	Rdb_SMM_StoreInstruc,
	Rdb_SMM_Cache,
	Rdb_SMM_PIDMA,
	Rdb_SMM_TLB,
	Rdb_SMM_Protect,
	Rdb_SMM_ValidFunc,
	Rdb_GameCheatFix,
	Rdb_GameCheatFixPlugin,
	Rdb_ViRefreshRate,
	Rdb_AiCountPerBytes,
	Rdb_AudioResetOnLoad,
	Rdb_AllowROMWrites,
	Rdb_CRC_Recalc,

	//Individual Game Settings
	Game_IniKey,
	Game_GameName,
	Game_GoodName,
	Game_TempLoaded,
	Game_SystemType,
	Game_EditPlugin_Gfx,
	Game_EditPlugin_Audio,
	Game_EditPlugin_Contr,
	Game_EditPlugin_RSP,
	Game_Plugin_Gfx,
	Game_Plugin_Audio,
	Game_Plugin_Controller,
	Game_Plugin_RSP,
	Game_SaveChip,
	Game_CpuType,
	Game_LastSaveSlot,
	Game_FixedAudio,
	Game_SyncViaAudio,
	Game_32Bit,
	Game_SMM_Cache,
	Game_SMM_Protect,
	Game_SMM_ValidFunc,
	Game_SMM_PIDMA,
	Game_SMM_TLB,
	Game_SMM_StoreInstruc,
	Game_CurrentSaveState,
	Game_RDRamSize,
	Game_CounterFactor,
	Game_UseTlb,
	Game_DelayDP,
	Game_DelaySI,
	Game_FastSP,
	Game_FuncLookupMode,
	Game_RegCache,
	Game_BlockLinking,
	Game_ScreenHertz,
	Game_RspAudioSignal,
	Game_UseHleGfx,
	Game_UseHleAudio,
	Game_LoadRomToMemory,
	Game_ViRefreshRate,
	Game_AiCountPerBytes,
	Game_AudioResetOnLoad,
	Game_AllowROMWrites,
	Game_CRC_Recalc,

	// General Game running info
	GameRunning_LoadingInProgress,
	GameRunning_CPU_Running,
	GameRunning_CPU_Paused,
	GameRunning_CPU_PausedType,
	GameRunning_InstantSaveFile,
	GameRunning_LimitFPS,
	GameRunning_ScreenHertz,
	GameRunning_InReset,

	//User Interface
	UserInterface_BasicMode,
	UserInterface_ShowCPUPer,
	UserInterface_DisplayFrameRate,
	UserInterface_InFullScreen,
	UserInterface_FrameDisplayType,
	UserInterface_MainWindowTop,
	UserInterface_MainWindowLeft,
	UserInterface_AlwaysOnTop,

	RomBrowser_Enabled,
	RomBrowser_ColoumnsChanged,
	RomBrowser_Top,
	RomBrowser_Left,
	RomBrowser_Width,
	RomBrowser_Height,
	RomBrowser_PosIndex,
	RomBrowser_WidthIndex,
	RomBrowser_SortFieldIndex,
	RomBrowser_SortAscendingIndex,
	RomBrowser_Recursive,
	RomBrowser_Maximized,

	//Directory Info
	Directory_LastSave,
	Directory_RecentGameDirCount,
	Directory_RecentGameDirIndex,
	Directory_Game,
	Directory_GameInitial,
	Directory_GameSelected,
	Directory_GameUseSelected,
	Directory_Plugin,
	Directory_PluginInitial,
	Directory_PluginSelected,
	Directory_PluginUseSelected,
	Directory_PluginSync,
	Directory_SnapShot,
	Directory_SnapShotInitial,
	Directory_SnapShotSelected,
	Directory_SnapShotUseSelected,
	Directory_NativeSave,
	Directory_NativeSaveInitial,
	Directory_NativeSaveSelected,
	Directory_NativeSaveUseSelected,
	Directory_InstantSave,
	Directory_InstantSaveInitial,
	Directory_InstantSaveSelected,
	Directory_InstantSaveUseSelected,
	Directory_Texture,
	Directory_TextureInitial,
	Directory_TextureSelected,
	Directory_TextureUseSelected,

	//File Info
	File_RecentGameFileCount,
	File_RecentGameFileIndex,

	//Debugger
	Debugger_Enabled,
	Debugger_ShowTLBMisses,
	Debugger_ShowUnhandledMemory,
	Debugger_ShowPifErrors,
	Debugger_ShowDivByZero,
	Debugger_GenerateLogFiles,
	Debugger_ProfileCode,
	Debugger_DisableGameFixes,
	Debugger_AppLogLevel,
	Debugger_AppLogFlush,
	Debugger_GenerateDebugLog,
	Debugger_ShowDListAListCount,
	Debugger_ShowRecompMemSize,

	//Plugins
	Plugin_RSP_Current,
	Plugin_RSP_CurVer,
	Plugin_GFX_Current,
	Plugin_GFX_CurVer,
	Plugin_AUDIO_Current,
	Plugin_AUDIO_CurVer,
	Plugin_CONT_Current,
	Plugin_CONT_CurVer,
	Plugin_UseHleGfx,
	Plugin_UseHleAudio,

	//Cheats
	Cheat_Entry,
	Cheat_Active,
	Cheat_Extension,
	Cheat_Notes,
	Cheat_Options,
	Cheat_Range,
	Cheat_RangeNotes,

	FirstRSPDefaultSet, LastRSPDefaultSet = FirstRSPDefaultSet + MaxPluginSetting,
	FirstRSPSettings, LastRSPSettings = FirstRSPSettings + MaxPluginSetting,
	FirstGfxDefaultSet, LastGfxDefaultSet = FirstGfxDefaultSet + MaxPluginSetting,
	FirstGfxSettings, LastGfxSettings = FirstGfxSettings + MaxPluginSetting,
	FirstAudioDefaultSet, LastAudioDefaultSet = FirstAudioDefaultSet + MaxPluginSetting,
	FirstAudioSettings, LastAudioSettings = FirstAudioSettings + MaxPluginSetting,
	FirstCtrlDefaultSet, LastCtrlDefaultSet = FirstCtrlDefaultSet + MaxPluginSetting,
	FirstCtrlSettings, LastCtrlSettings = FirstCtrlSettings + MaxPluginSetting,
};

#include "Support.h"
#include "./Settings/Settings Class.h"
#include "./Settings/Debug Settings.h"
#include "./Settings/Game Settings.h"
#include "./Settings/Recompiler Settings.h"
#include "./Settings/N64System Settings.h"
#include "./Settings/Gui Settings.h"
