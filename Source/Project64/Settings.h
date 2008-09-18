#ifndef __SETTINGS__H__
#define __SETTINGS__H__

#define MaxRomBrowserFields	100
#define MaxMD5_Per_Rom		10
#define MaxCheats			500
#define MaxPluginSetting	65535

enum SettingID {
	//Default values
	No_Default, Default_False, Default_True, Default_Language, Default_RdramSize,
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

	//Idvidual Game Settings
	Game_LastSaveSlot,

	//Rom Settings
	Default_CPUType, 
	Default_SaveChip,
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
	ROM_SaveChip, 
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
	SaveChipType,	 //DWORD - Current Save Type
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
};

#include "Support.h"
#include "./Settings/Settings Class.h"
#include "./Settings/Recompiler Settings.h"
#include "./Settings/N64System Settings.h"
#include "./Settings/Gui Settings.h"

#endif
