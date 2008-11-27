#ifndef __SETTINGS__H__
#define __SETTINGS__H__

#define MaxPluginSetting	65535

enum SettingID {
	//Default values
	Default_None,
	Default_Constant,
	
	//information - temp keys
	Info_ShortCutsChanged,

	//Support Files
	SupportFile_Settings, 
	SupportFile_RomDatabase,
	SupportFile_Cheats,
	SupportFile_Notes,
	SupportFile_ExtInfo,
	SupportFile_ShortCuts,
	SupportFile_RomListCache,
	SupportFile_7zipCache,

	//Settings
	Setting_ApplicationName,
	Setting_UseFromRegistry,
	Setting_RdbEditor,
	Setting_DisableScrSaver,
	Setting_AutoSleep,
	Setting_AutoStart,
	Setting_AutoFullscreen,
		
	Setting_AutoZipInstantSave,
	Setting_RememberCheats,
	Setting_CurrentLanguage,

	//RDB TLB Settings
	Rdb_SaveChip,
	Rdb_CpuType,
	Rdb_RDRamSize,
	Rdb_CounterFactor,
	Rdb_UseTlb,
	Rdb_DelaySi,
	Rdb_SPHack,
	Rdb_Status,
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
	Rdb_SMM_Cache,
	Rdb_SMM_PIDMA,
	Rdb_SMM_TLB,
	Rdb_SMM_Protect,
	Rdb_SMM_ValidFunc,
	Rdb_GameCheatFix,

	//Individual Game Settings
	Game_IniKey,
	Game_GameName,
	Game_GoodName,
	Game_Plugin_Gfx,
	Game_Plugin_Audio,
	Game_Plugin_Controller,
	Game_Plugin_RSP,
	Game_SaveChip,
	Game_CpuType,
	Game_LastSaveSlot,
	Game_FixedAudio,
	Game_SyncViaAudio,
	Game_SMM_Cache,
	Game_SMM_Protect,
	Game_SMM_ValidFunc,
	Game_SMM_PIDMA,
	Game_SMM_TLB,
	Game_CurrentSaveState,
	Game_RDRamSize,
	Game_CounterFactor,
	Game_UseTlb,
	Game_DelaySI,
	Game_SPHack,
	Game_FuncLookupMode,
	Game_RegCache,
	Game_BlockLinking,
	Game_ScreenHertz,
	Game_RspAudioSignal,
	Game_UseHleGfx,
	Game_UseHleAudio,
	Game_LoadRomToMemory,

	// General Game running info
	GameRunning_LoadingInProgress,
	GameRunning_CPU_Running,
	GameRunning_CPU_Paused,
	GameRunning_CPU_PausedType,
	GameRunning_InstantSaveFile,
	GameRunning_LimitFPS,
	GameRunning_ScreenHertz,
	
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
	Debugger_ShowUnhandledMemory,
	Debugger_ShowPifErrors,
	Debugger_ShowCheckOpUsageErrors,
	Debugger_GenerateLogFiles,
	Debugger_ProfileCode,
	Debugger_DisableGameFixes,
	Debugger_AppLogLevel,
	Debugger_AppLogFlush,
	Debugger_GenerateDebugLog,
	Debugger_ShowDListAListCount,
	Debugger_ShowRecompMemSize,

	//Beta Information
	Beta_IsBetaVersion,
	Beta_UserName,
	Beta_UserNameMD5,
	Beta_EmailAddress,
	Beta_EmailAddressMD5,
	Beta_IsValidExe,
	
	//Plugins
	Plugin_RSP_Current,
	Plugin_RSP_CurVer,
	Plugin_RSP_Changed,
	Plugin_GFX_Current,
	Plugin_GFX_CurVer,
	Plugin_GFX_Changed,
	Plugin_AUDIO_Current,
	Plugin_AUDIO_CurVer,
	Plugin_AUDIO_Changed,
	Plugin_CONT_Current,
	Plugin_CONT_CurVer,
	Plugin_CONT_Changed,
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

	FirstRSPDefaultSet,   LastRSPDefaultSet   = FirstRSPDefaultSet   + MaxPluginSetting,
	FirstRSPSettings,     LastRSPSettings     = FirstRSPSettings     + MaxPluginSetting,
	FirstGfxDefaultSet,   LastGfxDefaultSet   = FirstGfxDefaultSet   + MaxPluginSetting,
	FirstGfxSettings,     LastGfxSettings     = FirstGfxSettings     + MaxPluginSetting,
	FirstAudioDefaultSet, LastAudioDefaultSet = FirstAudioDefaultSet + MaxPluginSetting,
	FirstAudioSettings,   LastAudioSettings   = FirstAudioSettings   + MaxPluginSetting,
	FirstCtrlDefaultSet,  LastCtrlDefaultSet  = FirstCtrlDefaultSet  + MaxPluginSetting,
	FirstCtrlSettings,    LastCtrlSettings    = FirstCtrlSettings    + MaxPluginSetting,


/*	No_Default, Default_False, Default_True, Default_Language, Default_RdramSize,
	Default_RomStatus,Default_RomBrowserWidth,Default_RomBrowserHeight,
	Default_RememberedRomFiles, Default_RememberedRomDirs, Default_CheatExt,
	Default_SelfModCheck, Default_BlockLinking, Default_SaveSlot, Default_FunctionLookup,
	Default_LogLevel, Default_FrameDisplayType,

	//Settings
	UseSettingFromRegistry,

	//Language
	CurrentLanguage,

	//Gui Settings
	MainWindowTop, MainWindowLeft, RomBrowser, RomBrowserRecursive, 
	RomBrowserSortFieldIndex, RomBrowserSortAscendingIndex, 
	RomBrowserWidth, RomBrowserHeight,RomBrowserTop, RomBrowserLeft, RomBrowserMaximized, 
	RomBrowserPosIndex, RomBrowserWidthIndex, TLBWindowTop, TLBWindowLeft,

	//Beta Settings
	IsBetaVersion, BetaUserName, BetaUserNameMD5, BetaEmailAddress, BetaEmailAddressMD5, BetaVersion,
	IsValidExe,
	
	//General Settings
	AutoSleep, AutoFullScreen, AutoStart, AutoZip, BasicMode, RememberCheats, 
	DisableScrSaver, ShowCPUPer, LogFunctionCalls, ProfileCode, GenerateLogFiles, 
	LimitFPS, AlwaysOnTop, GenerateDebugLog, UseHighLevelGfx, UseHighLevelAudio,
	DisableGameFixes, AppLogLevel, AppLogFlush, DisplayFrameRate, FrameDisplayType,
	 

	//Debugger
	Debugger, ShowUnhandledMemory, ShowPifErrors, ShowDListAListCount, ShowCheckOpUsageErrors, 
	ShowRecompMemSize, ShowPifRamErrors,

	//Recent Files and Directories
	RememberedRomFilesCount, RecentRomFileIndex, RememberedRomDirCount, RecentRomDirIndex,

	//Directories
	InitialPluginDirectory,      PluginDirectory,      SelectedPluginDirectory,      UsePluginDirSelected,
	InitialRomDirectory,         RomDirectory,         SelectedRomDir,               UseRomDirSelected,
	InitialSaveDirectory,        SaveDirectory,        SelectedSaveDirectory,        UseSaveDirSelected,
	InitialInstantSaveDirectory, InstantSaveDirectory, SelectedInstantSaveDirectory, UseInstantDirSelected,
	InitialSnapShotDir,          SnapShotDir,          SelectedSnapShotDir,          UseSnapShotDirSelected,
	SyncPluginDir, LastSaveDir, InitialTextureDir, TextureDir, 

	//Support Files
	RomDatabaseFile, CheatIniName, SettingsIniName, NotesIniName, ExtIniName, ShortCutFile, 
	RomListCache, ZipCacheIniName,

	//Plugins
	DefaultRSP_Plugin,    DefaultGFX_Plugin, DefaultAUDIO_Plugin, DefaultCONT_Plugin,
	CurrentRSP_Plugin,    CurrentGFX_Plugin, CurrentAUDIO_Plugin, CurrentCONT_Plugin,
	CurVerRSP_Plugin,     CurVerGFX_Plugin,  CurVerAUDIO_Plugin,  CurVerCONT_Plugin,
	RSP_PluginChanged,    GFX_PluginChanged, AUDIO_PluginChanged, CONT_PluginChanged,
	FirstRSPDefaultSet,   LastRSPDefaultSet   = FirstRSPDefaultSet   + MaxPluginSetting,
	FirstRSPSettings,     LastRSPSettings     = FirstRSPSettings     + MaxPluginSetting,
	FirstGfxDefaultSet,   LastGfxDefaultSet   = FirstGfxDefaultSet   + MaxPluginSetting,
	FirstGfxSettings,     LastGfxSettings     = FirstGfxSettings     + MaxPluginSetting,
	FirstAudioDefaultSet, LastAudioDefaultSet = FirstAudioDefaultSet + MaxPluginSetting,
	FirstAudioSettings,   LastAudioSettings   = FirstAudioSettings   + MaxPluginSetting,
	FirstCtrlDefaultSet,  LastCtrlDefaultSet  = FirstCtrlDefaultSet  + MaxPluginSetting,
	FirstCtrlSettings,    LastCtrlSettings    = FirstCtrlSettings    + MaxPluginSetting,

	//Cheats
	CheatEntry,     LastCheatEntry     = CheatEntry     + MaxCheats,
	CheatPermEntry, LastPermCheatEntry = CheatPermEntry + MaxCheats,
	CheatOptions,   LastCheatOptions   = CheatOptions   + MaxCheats,
	CheatRange,     LastCheatRange     = CheatRange     + MaxCheats,
	CheatRangeNotes,LastCheatRangeNotes= CheatRangeNotes+ MaxCheats,
	CheatNotes,     LastCheatNotes     = CheatNotes     + MaxCheats,
	CheatActive,    LastCheatActive    = CheatActive    + MaxCheats,
	CheatExtension, LastCheatExtension = CheatExtension + MaxCheats,

	//Individual Game Settings
	Game_SaveChip,
	Game_LastSaveSlot,

	//RDB Settings
	Rdb_SaveChip,

	//Default Values
	System_SaveChip,

	//Rom Settings
	Default_CPUType, 
	Default_CFactor,
	ROM_IniKey, 
	ROM_NAME, 
	ROM_GoodName,
	ROM_MD5, ROM_LastMD5 = ROM_MD5 + MaxMD5_Per_Rom,
	ROM_InternalName, 
	ROM_Default, 
	ROM_CPUType, 
	ROM_RomInMemory, 
	ROM_FunctionLookup,
	ROM_RamSize, 
	ROM_Status, 
	ROM_CoreNotes, 
	ROM_PluginNotes, 
	ROM_CounterFactor,
	ROM_CustomSMM,
	ROM_SMM_Cache,
	ROM_SMM_PIDMA,
	ROM_SMM_TLB,
	ROM_SMM_Protect,
	ROM_SMM_ValidFunc,
	ROM_SyncAudio,
	ROM_UseTlb,
	ROM_RegCache,
	ROM_BlockLinking,
	ROM_UseJumpTable,
	ROM_DelayDlists,
	ROM_DelaySI,
	ROM_AudioSignal,
	ROM_SPHack,
	ROM_FixedAudio,

	//Rom TLB Settings
	ROM_TLB_VAddrStart,
	ROM_TLB_VAddrLen,
	ROM_TLB_PAddrStart,
	
	
	//System Settings
	SYSTEM_CPUType, 
	SYSTEM_RDRamSize,
	SYSTEM_SelfModMethod,
	SYSTEM_BlockLinking,
	SYSTEM_FunctionLookup,
	SYSTEM_SMM_Cache,       //DWORD - Self mod method (clear code on clearing instruction cache)
	SYSTEM_SMM_PIDMA,       //DWORD - Self mod method (clear code on PI DMA blocks)
	SYSTEM_SMM_ValidFunc,   //DWORD - Self mod method (Compare memory contents of function on finding)
	SYSTEM_SMM_Protect,     //DWORD - Self mod method (Protect Memory from any write to the code pages)
	SYSTEM_SMM_TLB,         //DWORD - Self mod method (clear code on TLB unmapping)

	//Currrent Running Information
	RamSize,		 //DWORD - Size of RDRAM 
	CPUType,        //DWORD - Current CPU  
	CPU_Paused,      //bool  - Is CPU Paused
	CPU_Paused_type, //DWORD - What type of pause is it
	SMM_ChangeMemory, 
	SMM_CheckMemory2, 
	SMM_CheckMemoryCache, 
	BlockLinking,
	FuncLookupMode,
	DelayDlists,     //DWORD - Delay when interrupt is set for DLIST
	DelaySI, 
	CounterFactor,   //DWORD - how many cycles each Opcode takes, 0 for variable
	UseTLB,
	UseJumpTable,
	RomInMemory,
	SyncViaAudio,
	AudioSignal,
	
	//Temporay Keys
	ApplicationName, //String
	LoadingRom,		 //bool
	CPU_Running,     //bool
	FirstDMA,        //bool
	CurrentSaveState,//DWORD
	ScreenHertz,     //DWORD
	InstantSaveFile, //String
	InFullScreen,    //bool
	SMM_Cache,       //bool - Self mod method (clear code on clearing instruction cache)
	SMM_PIDMA,       //bool - Self mod method (clear code on PI DMA blocks)
	SMM_ValidFunc,   //bool - Self mod method (Compare memory contents of function on finding)
	SMM_Protect,     //bool - Self mod method (Protect Memory from any write to the code pages)
	SMM_TLB,         //bool - Self mod method (clear code on TLB unmapping)
*/
};

#include "Support.h"
#include "./Settings/Settings Class.h"
#include "./Settings/Recompiler Settings.h"
#include "./Settings/N64System Settings.h"
#include "./Settings/Gui Settings.h"

#endif
