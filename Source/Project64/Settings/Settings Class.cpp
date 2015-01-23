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
#include "stdafx.h"

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

void CSettings::AddHandler ( SettingID TypeID, CSettingType * Handler )
{
	SETTING_MAP::_Pairib res = m_SettingInfo.insert(SETTING_MAP::value_type(TypeID,Handler));
	if (!res.second)
	{
		delete Handler;
	}
}

void CSettings::AddHowToHandleSetting ()
{
	//information - temp keys
	AddHandler(Info_ShortCutsChanged,   new CSettingTypeTempBool(false));


	//Support Files
	AddHandler(SupportFile_SettingsDefault,    new CSettingTypeRelativePath("Config","Project64.cfg"));
	AddHandler(SupportFile_Settings,           new CSettingTypeApplicationPath("","ConfigFile",SupportFile_SettingsDefault));
	AddHandler(SupportFile_SettingsDefault,    new CSettingTypeRelativePath("Config","Project64.cfg"));
	AddHandler(SupportFile_RomDatabase,        new CSettingTypeApplicationPath("","RomDatabase",SupportFile_RomDatabaseDefault));
	AddHandler(SupportFile_RomDatabaseDefault, new CSettingTypeRelativePath("Config","Project64.rdb"));
	AddHandler(SupportFile_Cheats,             new CSettingTypeApplicationPath("","Cheats",SupportFile_CheatsDefault));
	AddHandler(SupportFile_CheatsDefault,      new CSettingTypeRelativePath("Config","Project64.cht"));
	AddHandler(SupportFile_Notes,              new CSettingTypeApplicationPath("","Notes",SupportFile_NotesDefault));
	AddHandler(SupportFile_NotesDefault,       new CSettingTypeRelativePath("Config","Project64.rdn"));
	AddHandler(SupportFile_ExtInfo,            new CSettingTypeApplicationPath("","ExtInfo",SupportFile_ExtInfoDefault));
	AddHandler(SupportFile_ExtInfoDefault,     new CSettingTypeRelativePath("Config","Project64.rdx"));
	AddHandler(SupportFile_ShortCuts,          new CSettingTypeApplicationPath("","ShortCuts",SupportFile_ShortCutsDefault));
	AddHandler(SupportFile_ShortCutsDefault,   new CSettingTypeRelativePath("Config","Project64.sc3"));
	AddHandler(SupportFile_RomListCache,       new CSettingTypeApplicationPath("","RomListCache",SupportFile_RomListCacheDefault));
	AddHandler(SupportFile_RomListCacheDefault,new CSettingTypeRelativePath("Config","Project64.cache3"));
	AddHandler(SupportFile_7zipCache,          new CSettingTypeApplicationPath("","7zipCache",SupportFile_7zipCacheDefault));
	AddHandler(SupportFile_7zipCacheDefault,   new CSettingTypeRelativePath("Config","Project64.zcache"));
	
	//AddHandler(SyncPluginDir,   new CSettingTypeRelativePath("SyncPlugin",""));

	//Settings location
	AddHandler(Setting_ApplicationName, new CSettingTypeTempString(""));
	AddHandler(Setting_UseFromRegistry, new CSettingTypeApplication("Settings","Use Registry",(DWORD)false));
	AddHandler(Setting_RdbEditor,       new CSettingTypeApplication("","Rdb Editor",          false));
	AddHandler(Setting_PluginPageFirst, new CSettingTypeApplication("","Plugin Page First",   false));
	AddHandler(Setting_DisableScrSaver, new CSettingTypeApplication("","Disable Screen Saver",(DWORD)true));
	AddHandler(Setting_AutoSleep,       new CSettingTypeApplication("","Auto Sleep",          (DWORD)true));
	AddHandler(Setting_AutoStart,       new CSettingTypeApplication("","Auto Start",          (DWORD)true));
	AddHandler(Setting_AutoFullscreen,  new CSettingTypeApplication("","Auto Full Screen",    (DWORD)false));
	AddHandler(Setting_AutoZipInstantSave,new CSettingTypeApplication("","Auto Zip Saves",    (DWORD)true));
	AddHandler(Setting_EraseGameDefaults, new CSettingTypeApplication("","Erase on default",  (DWORD)true));
	AddHandler(Setting_CheckEmuRunning, new CSettingTypeApplication("","Check Running",       (DWORD)true));

	AddHandler(Setting_RememberCheats,  new CSettingTypeApplication("","Remember Cheats",     (DWORD)false));
	AddHandler(Setting_CurrentLanguage, new CSettingTypeApplication("","Current Language",""));
	AddHandler(Setting_LanguageDirDefault, new CSettingTypeRelativePath("Lang",""));
	AddHandler(Setting_LanguageDir,     new CSettingTypeApplicationPath("Directory","Lang",Setting_LanguageDirDefault));

	AddHandler(Rdb_GoodName,            new CSettingTypeRomDatabase("Good Name",Game_GameName));
	AddHandler(Rdb_SaveChip,            new CSettingTypeRDBSaveChip("Save Type",SaveChip_Auto));
#ifdef _DEBUG
	AddHandler(Rdb_CpuType,             new CSettingTypeRDBCpuType("CPU Type",CPU_SyncCores));
#else
	AddHandler(Rdb_CpuType,             new CSettingTypeRDBCpuType("CPU Type",CPU_Recompiler));
#endif
	AddHandler(Rdb_RDRamSize,           new CSettingTypeRDBRDRamSize("RDRAM Size",0x400000));
	AddHandler(Rdb_CounterFactor,       new CSettingTypeRomDatabase("Counter Factor",2));
	AddHandler(Rdb_UseTlb,              new CSettingTypeRDBYesNo("Use TLB",true));
	AddHandler(Rdb_DelayDP,             new CSettingTypeRDBYesNo("Delay DP",true));
	AddHandler(Rdb_DelaySi,             new CSettingTypeRDBYesNo("Delay SI",false));
	AddHandler(Rdb_32Bit,               new CSettingTypeRDBYesNo("32bit",true));
	AddHandler(Rdb_FastSP,              new CSettingTypeRDBYesNo("Fast SP",true));
	AddHandler(Rdb_Status,              new CSettingTypeRomDatabase("Status","Unknown"));
	AddHandler(Rdb_NotesCore,           new CSettingTypeRomDatabase("Core Note",""));
	AddHandler(Rdb_NotesPlugin,         new CSettingTypeRomDatabase("Plugin Note",""));
	AddHandler(Rdb_FixedAudio,          new CSettingTypeRomDatabase("Fixed Audio",false));
	AddHandler(Rdb_SyncViaAudio,        new CSettingTypeRomDatabase("Sync Audio",true));
	AddHandler(Rdb_RspAudioSignal,      new CSettingTypeRDBYesNo("Audio Signal",false));
	AddHandler(Rdb_TLB_VAddrStart,      new CSettingTypeRomDatabase("TLB: Vaddr Start",0));
	AddHandler(Rdb_TLB_VAddrLen,        new CSettingTypeRomDatabase("TLB: Vaddr Len",0));
	AddHandler(Rdb_TLB_PAddrStart,      new CSettingTypeRomDatabase("TLB: PAddr Start",0));
	AddHandler(Rdb_UseHleGfx,           new CSettingTypeRomDatabase("HLE GFX",Plugin_UseHleGfx));
	AddHandler(Rdb_UseHleAudio,         new CSettingTypeRomDatabase("HLE Audio",Plugin_UseHleAudio));
	AddHandler(Rdb_LoadRomToMemory,     new CSettingTypeRomDatabase("Rom In Memory",false));	
	AddHandler(Rdb_ScreenHertz,         new CSettingTypeRomDatabase("ScreenHertz",60));	
	AddHandler(Rdb_FuncLookupMode,      new CSettingTypeRomDatabase("FuncFind",FuncFind_PhysicalLookup));	
	AddHandler(Rdb_RegCache,            new CSettingTypeRDBYesNo("Reg Cache",true));	
	AddHandler(Rdb_BlockLinking,        new CSettingTypeRDBOnOff("Linking",true));	
	AddHandler(Rdb_SMM_Cache,           new CSettingTypeRomDatabase("SMM-Cache",true));
	AddHandler(Rdb_SMM_StoreInstruc,    new CSettingTypeRomDatabase("SMM-StoreInstr",false));
	AddHandler(Rdb_SMM_PIDMA,           new CSettingTypeRomDatabase("SMM-PI DMA",true));
	AddHandler(Rdb_SMM_TLB,             new CSettingTypeRomDatabase("SMM-TLB",true));
	AddHandler(Rdb_SMM_Protect,         new CSettingTypeRomDatabase("SMM-Protect",false));
	AddHandler(Rdb_SMM_ValidFunc,       new CSettingTypeRomDatabase("SMM-FUNC",true));
	AddHandler(Rdb_GameCheatFix,        new CSettingTypeRomDatabaseIndex("Cheat","",""));
	AddHandler(Rdb_ViRefreshRate,       new CSettingTypeRomDatabase("ViRefresh",2200));
	AddHandler(Rdb_AiCountPerBytes,     new CSettingTypeRomDatabase("AiCountPerBytes",500));
	
	AddHandler(Game_IniKey,             new CSettingTypeTempString(""));
	AddHandler(Game_GameName,           new CSettingTypeTempString(""));
	AddHandler(Game_GoodName,           new CSettingTypeGame("Good Name",Rdb_GoodName));
	AddHandler(Game_EditPlugin_Gfx,     new CSettingTypeGame("Plugin-Gfx",Default_None));
	AddHandler(Game_EditPlugin_Audio,   new CSettingTypeGame("Plugin-Audio",Default_None));
	AddHandler(Game_EditPlugin_Contr,   new CSettingTypeGame("Plugin-Controller",Default_None));
	AddHandler(Game_EditPlugin_RSP,     new CSettingTypeGame("Plugin-RSP",Default_None));
	AddHandler(Game_Plugin_Gfx,         new CSettingTypeGame("Plugin-Gfx",Plugin_GFX_Current));
	AddHandler(Game_Plugin_Audio,       new CSettingTypeGame("Plugin-Audio",Plugin_AUDIO_Current));
	AddHandler(Game_Plugin_Controller,  new CSettingTypeGame("Plugin-Controller",Plugin_CONT_Current));
	AddHandler(Game_Plugin_RSP,         new CSettingTypeGame("Plugin-RSP",Plugin_RSP_Current));
	AddHandler(Game_SaveChip,           new CSettingTypeGame("SaveChip",Rdb_SaveChip));
	AddHandler(Game_CpuType,            new CSettingTypeGame("CpuType",Rdb_CpuType));
	AddHandler(Game_LastSaveSlot,       new CSettingTypeGame("Last Used Save Slot",(DWORD)0));
	AddHandler(Game_FixedAudio,         new CSettingTypeGame("Fixed Audio",Rdb_FixedAudio));
	AddHandler(Game_RDRamSize,          new CSettingTypeGame("RDRamSize",Rdb_RDRamSize));
	AddHandler(Game_CounterFactor,      new CSettingTypeGame("Counter Factor",Rdb_CounterFactor));
	AddHandler(Game_UseTlb,             new CSettingTypeGame("Use TLB",Rdb_UseTlb));
	AddHandler(Game_DelayDP,            new CSettingTypeGame("Delay DP",Rdb_DelayDP));
	AddHandler(Game_DelaySI,            new CSettingTypeGame("Delay SI",Rdb_DelaySi));
	AddHandler(Game_RspAudioSignal,     new CSettingTypeGame("Audio Signal",Rdb_RspAudioSignal));
	AddHandler(Game_32Bit,              new CSettingTypeGame("32bit",Rdb_32Bit));
	AddHandler(Game_FastSP,             new CSettingTypeGame("SP Hack",Rdb_FastSP));
	AddHandler(Game_CurrentSaveState,   new CSettingTypeTempNumber(0));
	AddHandler(Game_SyncViaAudio,       new CSettingTypeGame("Sync Audio",Rdb_SyncViaAudio));
	AddHandler(Game_UseHleGfx,          new CSettingTypeGame("HLE GFX",Rdb_UseHleGfx));
	AddHandler(Game_UseHleAudio,        new CSettingTypeGame("HLE Audio",Rdb_UseHleAudio));
	AddHandler(Game_LoadRomToMemory,    new CSettingTypeGame("Rom In Memory",Rdb_LoadRomToMemory));
	AddHandler(Game_ScreenHertz,        new CSettingTypeGame("ScreenHertz",Rdb_ScreenHertz));
	AddHandler(Game_FuncLookupMode,     new CSettingTypeGame("FuncFind",Rdb_FuncLookupMode));
	AddHandler(Game_RegCache,           new CSettingTypeGame("Reg Cache",Rdb_RegCache));
	AddHandler(Game_BlockLinking,       new CSettingTypeGame("Linking",Rdb_BlockLinking));	
	AddHandler(Game_SMM_StoreInstruc,   new CSettingTypeGame("SMM-StoreInst",Rdb_SMM_StoreInstruc));
	AddHandler(Game_SMM_Cache,          new CSettingTypeGame("SMM-Cache",Rdb_SMM_Cache));
	AddHandler(Game_SMM_PIDMA,          new CSettingTypeGame("SMM-PI DMA",Rdb_SMM_PIDMA));
	AddHandler(Game_SMM_TLB,            new CSettingTypeGame("SMM-TLB",Rdb_SMM_TLB));
	AddHandler(Game_SMM_Protect,        new CSettingTypeGame("SMM-Protect",Rdb_SMM_Protect));
	AddHandler(Game_SMM_ValidFunc,      new CSettingTypeGame("SMM-FUNC",Rdb_SMM_ValidFunc));
	AddHandler(Game_ViRefreshRate,      new CSettingTypeGame("ViRefresh",Rdb_ViRefreshRate));
	AddHandler(Game_AiCountPerBytes,    new CSettingTypeGame("AiCountPerBytes",Rdb_AiCountPerBytes));

	//User Interface
	AddHandler(UserInterface_BasicMode,        new CSettingTypeApplication("","Basic Mode",          (DWORD)true));
	AddHandler(UserInterface_ShowCPUPer,       new CSettingTypeApplication("","Display CPU Usage",   (DWORD)false));
	AddHandler(UserInterface_DisplayFrameRate, new CSettingTypeApplication("","Display Frame Rate",  (DWORD)true));
	AddHandler(UserInterface_InFullScreen,     new CSettingTypeTempBool(false));
	AddHandler(UserInterface_FrameDisplayType, new CSettingTypeApplication("","Frame Rate Display Type", (DWORD)FR_VIs));
	AddHandler(UserInterface_MainWindowTop,    new CSettingTypeApplication("Main Window","Top"        ,Default_None));
	AddHandler(UserInterface_MainWindowLeft,   new CSettingTypeApplication("Main Window","Left"       ,Default_None));
	AddHandler(UserInterface_AlwaysOnTop,      new CSettingTypeApplication("","Always on top",       (DWORD)false));

	AddHandler(RomBrowser_Enabled,             new CSettingTypeApplication("Rom Browser","Rom Browser",true));
	AddHandler(RomBrowser_ColoumnsChanged,     new CSettingTypeTempBool(false));
	AddHandler(RomBrowser_Top,                 new CSettingTypeApplication("Rom Browser","Top"        ,Default_None));
	AddHandler(RomBrowser_Left,                new CSettingTypeApplication("Rom Browser","Left"       ,Default_None));
	AddHandler(RomBrowser_Width,               new CSettingTypeApplication("Rom Browser","Width",     (DWORD)640));
	AddHandler(RomBrowser_Height,              new CSettingTypeApplication("Rom Browser","Height",    (DWORD)480));
	AddHandler(RomBrowser_PosIndex,            new CSettingTypeApplicationIndex("Rom Browser\\Field Pos","Field",Default_None));
	AddHandler(RomBrowser_WidthIndex,          new CSettingTypeApplicationIndex("Rom Browser\\Field Width","Field",Default_None));
	AddHandler(RomBrowser_SortFieldIndex,      new CSettingTypeApplicationIndex("Rom Browser", "Sort Field",  Default_None));
	AddHandler(RomBrowser_SortAscendingIndex,  new CSettingTypeApplicationIndex("Rom Browser", "Sort Ascending",  Default_None));
	AddHandler(RomBrowser_Recursive,           new CSettingTypeApplication("Rom Browser","Recursive", false));
	AddHandler(RomBrowser_Maximized,           new CSettingTypeApplication("Rom Browser","Maximized", false));

	AddHandler(Directory_RecentGameDirCount,   new CSettingTypeApplication("","Remembered Rom Dirs",(DWORD)10));
	AddHandler(Directory_RecentGameDirIndex,   new CSettingTypeApplicationIndex("Recent Dir","Recent Dir",Default_None));

	//Directory_Game,
	AddHandler(Directory_Game,                 new CSettingTypeSelectedDirectory("Dir:Game",Directory_GameInitial,Directory_GameSelected,Directory_GameUseSelected));
	AddHandler(Directory_GameInitial,          new CSettingTypeRelativePath("Game Directory",""));
	AddHandler(Directory_GameSelected,         new CSettingTypeApplication("Directory","Game",Directory_GameInitial));
	AddHandler(Directory_GameUseSelected,      new CSettingTypeApplication("Directory","Game - Use Selected",false));

	AddHandler(Directory_Plugin,               new CSettingTypeSelectedDirectory("Dir:Plugin",Directory_PluginInitial,Directory_PluginSelected,Directory_PluginUseSelected));
	AddHandler(Directory_PluginInitial,        new CSettingTypeRelativePath("Plugin",""));
	AddHandler(Directory_PluginSelected,       new CSettingTypeApplicationPath("Directory","Plugin",Directory_PluginInitial));
	AddHandler(Directory_PluginUseSelected,    new CSettingTypeApplication("Directory","Plugin - Use Selected",false));
	AddHandler(Directory_PluginSync,           new CSettingTypeRelativePath("SyncPlugin",""));
	
	AddHandler(Directory_SnapShot,             new CSettingTypeSelectedDirectory("Dir:Snapshot",Directory_SnapShotInitial,Directory_SnapShotSelected,Directory_SnapShotUseSelected));
	AddHandler(Directory_SnapShotInitial,      new CSettingTypeRelativePath("Screenshots",""));
	AddHandler(Directory_SnapShotSelected,     new CSettingTypeApplicationPath("Directory","Snap Shot",Directory_SnapShotInitial));
	AddHandler(Directory_SnapShotUseSelected,  new CSettingTypeApplication("Directory","Snap Shot - Use Selected",false));

	AddHandler(Directory_NativeSave,           new CSettingTypeSelectedDirectory("Dir:NativeSave",Directory_NativeSaveInitial,Directory_NativeSaveSelected,Directory_NativeSaveUseSelected));
	AddHandler(Directory_NativeSaveInitial,    new CSettingTypeRelativePath("Save",""));
	AddHandler(Directory_NativeSaveSelected,   new CSettingTypeApplicationPath("Directory","Save",Directory_NativeSaveInitial));
	AddHandler(Directory_NativeSaveUseSelected,new CSettingTypeApplication("Directory","Save - Use Selected",false));

	AddHandler(Directory_InstantSave,           new CSettingTypeSelectedDirectory("Dir:InstantSave",Directory_InstantSaveInitial,Directory_InstantSaveSelected,Directory_InstantSaveUseSelected));
	AddHandler(Directory_InstantSaveInitial,    new CSettingTypeRelativePath("Save",""));
	AddHandler(Directory_InstantSaveSelected,   new CSettingTypeApplicationPath("Directory","Instant Save",Directory_InstantSaveInitial));
	AddHandler(Directory_InstantSaveUseSelected,new CSettingTypeApplication("Directory","Instant Save - Use Selected",false));

	AddHandler(Directory_Texture,               new CSettingTypeSelectedDirectory("Dir:Texture",Directory_TextureInitial,Directory_TextureSelected,Directory_TextureUseSelected));
	AddHandler(Directory_TextureInitial,        new CSettingTypeRelativePath("Textures",""));
	AddHandler(Directory_TextureSelected,       new CSettingTypeApplicationPath("Directory","Texture Dir",Directory_InstantSaveInitial));
	AddHandler(Directory_TextureUseSelected,    new CSettingTypeApplication("Directory","Texture Dir - Use Selected",false));

	AddHandler(Directory_LastSave,              new CSettingTypeApplication("Directory","Last Save Directory",    Directory_InstantSave));

	AddHandler(GameRunning_LoadingInProgress,  new CSettingTypeTempBool(false));
	AddHandler(GameRunning_CPU_Running,        new CSettingTypeTempBool(false));
	AddHandler(GameRunning_CPU_Paused,         new CSettingTypeTempBool(false));
	AddHandler(GameRunning_CPU_PausedType,     new CSettingTypeTempNumber(Default_None));
	AddHandler(GameRunning_InstantSaveFile,    new CSettingTypeTempString(""));
	AddHandler(GameRunning_LimitFPS,           new CSettingTypeTempBool(true));
	AddHandler(GameRunning_ScreenHertz,        new CSettingTypeTempNumber(60));

	AddHandler(File_RecentGameFileCount,       new CSettingTypeApplication("","Remembered Rom Files",(DWORD)10));
	AddHandler(File_RecentGameFileIndex,       new CSettingTypeApplicationIndex("Recent File","Recent Rom",Default_None));

	AddHandler(Debugger_Enabled,                new CSettingTypeApplication("Debugger","Debugger",false));
	AddHandler(Debugger_ShowTLBMisses,          new CSettingTypeApplication("Debugger","Show TLB Misses",false));
	AddHandler(Debugger_ShowUnhandledMemory,    new CSettingTypeApplication("Debugger","Show Unhandled Memory",false));
	AddHandler(Debugger_ShowPifErrors,          new CSettingTypeApplication("Debugger","Show Pif Errors",false));
	AddHandler(Debugger_DisableGameFixes,       new CSettingTypeApplication("Debugger","Disable Game Fixes",false));
	AddHandler(Debugger_ShowDListAListCount,    new CSettingTypeApplication("Debugger","Show Dlist Alist Count",false));
	AddHandler(Debugger_ShowRecompMemSize,      new CSettingTypeApplication("Debugger","Show Recompiler Memory size",false));
	AddHandler(Debugger_ShowDivByZero,          new CSettingTypeApplication("Debugger","Show Div by zero",false));
	AddHandler(Debugger_GenerateDebugLog,       new CSettingTypeApplication("Debugger","Generate Debug Code",false));
	AddHandler(Debugger_ProfileCode,            new CSettingTypeApplication("Debugger","Profile Code",        (DWORD)false));
	AddHandler(Debugger_AppLogLevel,            new CSettingTypeApplication("Logging","Log Level",(DWORD)TraceError));
	AddHandler(Debugger_AppLogFlush,            new CSettingTypeApplication("Logging","Log Auto Flush",(DWORD)false));
	AddHandler(Debugger_GenerateLogFiles,       new CSettingTypeApplication("Debugger","Generate Log Files", false));


#ifdef BETA_RELEASE
	AddHandler(Beta_IsBetaVersion,      new CSettingTypeTempBool(true));
	AddHandler(Beta_UserName,           new CSettingTypeTempString("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
	AddHandler(Beta_EmailAddress,       new CSettingTypeTempString("????????????????????????????????????????????????????????????????????????????????"));
	AddHandler(Beta_UserNameMD5,        new CSettingTypeTempString("CBBABA8D2262FF1F7A47CEAD87FC4304"));
	AddHandler(Beta_EmailAddressMD5,    new CSettingTypeTempString("47A3D7CBF1DA291D5EB30DCAAF21B9F8"));
	AddHandler(Beta_IsValidExe,         new CSettingTypeTempBool(true));
#endif

	//Plugin
	AddHandler(Plugin_RSP_Current,   new CSettingTypeApplication("Plugin","RSP Dll",       "RSP\\RSP 1.7.dll"));
	AddHandler(Plugin_GFX_Current,   new CSettingTypeApplication("Plugin","Graphics Dll",  "GFX\\Jabo_Direct3D8.dll"));
	AddHandler(Plugin_AUDIO_Current, new CSettingTypeApplication("Plugin","Audio Dll",     "Audio\\Jabo_Dsound.dll"));
	AddHandler(Plugin_CONT_Current,  new CSettingTypeApplication("Plugin","Controller Dll","Input\\Jabo_DInput.dll"));

	AddHandler(Plugin_RSP_CurVer,    new CSettingTypeApplication("Plugin","RSP Dll Ver",        ""));
	AddHandler(Plugin_GFX_CurVer,    new CSettingTypeApplication("Plugin","Graphics Dll Ver",   ""));
	AddHandler(Plugin_AUDIO_CurVer,  new CSettingTypeApplication("Plugin","Audio Dll Ver",      ""));
	AddHandler(Plugin_CONT_CurVer,   new CSettingTypeApplication("Plugin","Controller Dll Ver", ""));

	AddHandler(Plugin_RSP_Changed,   new CSettingTypeTempBool(true));
	AddHandler(Plugin_GFX_Changed,   new CSettingTypeTempBool(true));
	AddHandler(Plugin_AUDIO_Changed, new CSettingTypeTempBool(true));
	AddHandler(Plugin_CONT_Changed,  new CSettingTypeTempBool(true));

	AddHandler(Plugin_UseHleGfx,     new CSettingTypeApplication("RSP","HLE GFX",true));
	AddHandler(Plugin_UseHleAudio,   new CSettingTypeApplication("RSP","HLE Audio",false));

	// cheats
	AddHandler(Cheat_Entry,          new CSettingTypeCheats(""));
	AddHandler(Cheat_Active,         new CSettingTypeGameIndex("Cheat","",(DWORD)false));	
	AddHandler(Cheat_Extension,      new CSettingTypeGameIndex("Cheat",".exten","??? - Not Set"));	
	AddHandler(Cheat_Notes,          new CSettingTypeCheats("_N"));	
	AddHandler(Cheat_Options,        new CSettingTypeCheats("_O"));	
	AddHandler(Cheat_Range,          new CSettingTypeCheats("_R"));	
	AddHandler(Cheat_RangeNotes,     new CSettingTypeCheats("_RN"));	
}

DWORD CSettings::FindSetting ( CSettings * _this, char * Name )
{
	for (SETTING_MAP::iterator iter = _this->m_SettingInfo.begin(); iter != _this->m_SettingInfo.end(); iter++)
	{
		CSettingType * Setting = iter->second;
		if (Setting->GetSettingType() == SettingType_GameSetting)
		{
			CSettingTypeGame * GameSetting = (CSettingTypeGame *)Setting;
			if (_stricmp(GameSetting->GetKeyName(),Name) != 0)
			{
				continue;
			}
			return iter->first;
		}
		if (Setting->GetSettingType() == SettingType_CfgFile)
		{
			CSettingTypeApplication * CfgSetting = (CSettingTypeApplication *)Setting;
			if (_stricmp(CfgSetting->GetKeyName(),Name) != 0)
			{
				continue;
			}
			return iter->first;
		}
		if (Setting->GetSettingType() == SettingType_SelectedDirectory)
		{
			CSettingTypeSelectedDirectory * SelectedDirectory = (CSettingTypeSelectedDirectory *)Setting;
			if (_stricmp(SelectedDirectory->GetName(),Name) != 0)
			{
				continue;
			}
			return iter->first;
		}
	}
	return 0;
}

void CSettings::FlushSettings ( CSettings * /*_this*/ )
{
	CSettingTypeCheats::FlushChanges();
	CSettingTypeApplication::Flush();
}

DWORD CSettings::GetSetting ( CSettings * _this, SettingID Type )
{
	return _this->LoadDword(Type);
}

const char * CSettings::GetSettingSz ( CSettings * _this, SettingID Type, char * Buffer, int BufferSize )
{
	if (Buffer && BufferSize > 0)
	{
		Buffer[0] = 0;
		_this->LoadString(Type, Buffer,BufferSize);
	}
	return Buffer;
}

void CSettings::SetSetting ( CSettings * _this, SettingID ID, unsigned int Value )
{
	_this->SaveDword(ID,Value);
}

void CSettings::SetSettingSz ( CSettings * _this, SettingID ID, const char * Value )
{
	_this->SaveString(ID,Value);
}

void CSettings::RegisterSetting ( CSettings * _this, SettingID ID, SettingID DefaultID, SettingDataType DataType, 
                                      SettingType Type, const char * Category, const char * DefaultStr, 
									  DWORD Value )
{
	switch (Type)
	{
	case SettingType_ConstValue:
		if (DataType != Data_DWORD) 
		{
			g_Notify->BreakPoint(__FILE__,__LINE__); 
			return;
		}
		_this->AddHandler(ID,new CSettingTypeTempNumber(Value));
		break;
	case SettingType_ConstString:
		if (DataType != Data_String) 
		{
			g_Notify->BreakPoint(__FILE__,__LINE__); 
			return;
		}
		_this->AddHandler(ID,new CSettingTypeTempString(DefaultStr));
		break;
	case SettingType_CfgFile:
	case SettingType_Registry:
		switch (DataType)
		{
		case Data_DWORD:
			if (DefaultID == Default_None)
			{
				_this->AddHandler(ID,new CSettingTypeApplication(Category,DefaultStr,Value));
			} else {
				_this->AddHandler(ID,new CSettingTypeApplication(Category,DefaultStr,DefaultID));
			}
			break;
		case Data_String:
			if (DefaultID == Default_None)
			{
				_this->AddHandler(ID,new CSettingTypeApplication(Category,DefaultStr,""));
			} else {
				_this->AddHandler(ID,new CSettingTypeApplication(Category,DefaultStr,DefaultID));
			}
			break;
		default:
			g_Notify->BreakPoint(__FILE__,__LINE__); 
		}
		break;
	case SettingType_GameSetting:
		{
			stdstr_f Name("%s-%s",Category,DefaultStr);
			switch (DataType)
			{
			case Data_DWORD:
				if (DefaultID == Default_None)
				{
					SettingID RdbSetting = (SettingID)_this->m_NextAutoSettingId;
					_this->m_NextAutoSettingId += 1;
					_this->AddHandler(RdbSetting,new CSettingTypeRomDatabase(Name.c_str(),(int)Value));
					_this->AddHandler(ID,new CSettingTypeGame(Name.c_str(),RdbSetting));
				} else {
					_this->AddHandler(ID,new CSettingTypeGame(Name.c_str(),DefaultID));
				}
				break;
			case Data_String:
				if (DefaultID == Default_None)
				{
					_this->AddHandler(ID,new CSettingTypeGame(Name.c_str(),""));
				} else {
					_this->AddHandler(ID,new CSettingTypeGame(Name.c_str(),DefaultID));
				}
				break;
			default:
				g_Notify->BreakPoint(__FILE__,__LINE__); 
			}
		}
		break;
	case SettingType_RomDatabase:
		switch (DataType)
		{
		case Data_DWORD:
			if (DefaultID == Default_None)
			{
				_this->AddHandler(ID,new CSettingTypeRomDatabase(DefaultStr,(int)Value,true));
			} else {
				_this->AddHandler(ID,new CSettingTypeRomDatabase(DefaultStr,(SettingID)Value,true));
			}
			break;
		case Data_String:
			if (DefaultID == Default_None)
			{
				_this->AddHandler(ID,new CSettingTypeRomDatabase(DefaultStr,"",true));
			} else {
				_this->AddHandler(ID,new CSettingTypeRomDatabase(DefaultStr,DefaultID,true));
			}
			break;
		default:
			g_Notify->BreakPoint(__FILE__,__LINE__); 
		}
		break;
	case SettingType_RdbSetting:
		switch (DataType)
		{
		case Data_DWORD:
			if (DefaultID == Default_None)
			{
				_this->AddHandler(ID,new CSettingTypeRomDatabaseSetting(Category, DefaultStr,(int)Value,true));
			} else {
				SettingID RdbSetting = (SettingID)_this->m_NextAutoSettingId;
				_this->m_NextAutoSettingId += 1;
				_this->AddHandler(RdbSetting,new CSettingTypeRomDatabaseSetting(Category, DefaultStr,DefaultID,true));
				_this->AddHandler(ID,new CSettingTypeApplication(Category,DefaultStr,RdbSetting));
			}
			break;
		default:
			g_Notify->BreakPoint(__FILE__,__LINE__); 
		}
		break;
	default:
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	}
}

bool CSettings::Initilize( const char * AppName )
{
	AddHowToHandleSetting();
	CSettingTypeApplication::Initilize(AppName);
	CSettingTypeRomDatabase::Initilize();
	CSettingTypeGame::Initilize();
	CSettingTypeCheats::Initilize();

	g_Settings->SaveString(Setting_ApplicationName,AppName);
	return true;
}

bool CSettings::LoadBool ( SettingID Type )
{
	bool Value = false;
	LoadBool(Type,Value);
	return Value;
}

bool CSettings::LoadBool ( SettingID Type, bool & Value )
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
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	} else {
		return FindInfo->second->Load(0,Value);
	}
	return false;
}

bool CSettings::LoadBoolIndex( SettingID Type, int index )
{
	bool Value = false;
	LoadBoolIndex(Type,index,Value);
	return Value;
}

bool CSettings::LoadBoolIndex( SettingID Type, int index , bool & Value )
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
		return FindInfo->second->Load(index,Value);
	} else {
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	}
	return false;
}

DWORD CSettings::LoadDword ( SettingID Type )
{
	DWORD Value = 0;
	LoadDword(Type,Value);
	return Value;
}

bool CSettings::LoadDword ( SettingID Type, DWORD & Value)
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
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	} else {
		return FindInfo->second->Load(0,Value);
	}
	return false;
}

DWORD CSettings::LoadDwordIndex( SettingID Type, int index)
{
	DWORD Value;
	LoadDwordIndex(Type,index,Value);
	return Value;
}

bool CSettings::LoadDwordIndex( SettingID Type, int index, DWORD & Value)
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
		return FindInfo->second->Load(index,Value);
	} else {
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	}
	return false;
}

stdstr CSettings::LoadString ( SettingID Type )
{
	stdstr Value;
	LoadString(Type,Value);
	return Value;
}

bool CSettings::LoadString ( SettingID Type, stdstr & Value )
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
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	} else {
		return FindInfo->second->Load(0,Value);
	}
	return false;
}

bool CSettings::LoadString ( SettingID Type, char * Buffer, int BufferSize )
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
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	} else {
		stdstr Value;
		bRes = FindInfo->second->Load(0,Value);
		int len = BufferSize;
		if ((Value.length() + 1) < (size_t)len)
		{
			len = Value.length() + 1;
		}
		strncpy(Buffer,Value.c_str(),len);
	}
	return bRes;
}

stdstr CSettings::LoadStringIndex ( SettingID Type, int index )
{
	stdstr Value;
	LoadStringIndex(Type,index,Value);
	return Value;
}

bool CSettings::LoadStringIndex ( SettingID Type, int index, stdstr & Value )
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
		return FindInfo->second->Load(index,Value);
	} else {
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	}
	return false;
}

bool CSettings::LoadStringIndex ( SettingID /*Type*/, int /*index*/, char * /*Buffer*/, int /*BufferSize*/ )
{
	g_Notify->BreakPoint(__FILE__,__LINE__); 
	return false;
}

//Load the default value for the setting
bool CSettings::LoadDefaultBool ( SettingID Type )
{
	bool Value = false;
	LoadDefaultBool(Type,Value);
	return Value;
}

void CSettings::LoadDefaultBool ( SettingID Type, bool & Value )
{
	SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
	if (FindInfo == m_SettingInfo.end()) 
	{  
		//if not found do nothing
		UnknownSetting(Type);
	} else {
		if (FindInfo->second->IndexBasedSetting())
		{
			g_Notify->BreakPoint(__FILE__,__LINE__); 
		} else {
			FindInfo->second->LoadDefault(0,Value);
		}
	}
}

bool CSettings::LoadDefaultBoolIndex ( SettingID /*Type*/, int /*index*/  )
{
	g_Notify->BreakPoint(__FILE__,__LINE__); 
	return false;
}

void CSettings::LoadDefaultBoolIndex ( SettingID /*Type*/, int /*index*/, bool & /*Value*/ )
{
	g_Notify->BreakPoint(__FILE__,__LINE__); 
}

DWORD  CSettings::LoadDefaultDword ( SettingID Type )
{
	DWORD Value = 0;
	LoadDefaultDword(Type,Value);
	return Value;
}

void CSettings::LoadDefaultDword ( SettingID Type, DWORD & Value)
{
	SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
	if (FindInfo == m_SettingInfo.end()) 
	{  
		//if not found do nothing
		UnknownSetting(Type);
	} else {
		if (FindInfo->second->IndexBasedSetting())
		{
			g_Notify->BreakPoint(__FILE__,__LINE__); 
		} else {
			FindInfo->second->LoadDefault(0,Value);
		}
	}
}

DWORD  CSettings::LoadDefaultDwordIndex ( SettingID /*Type*/, int /*index*/ )
{
	g_Notify->BreakPoint(__FILE__,__LINE__); 
	return false;
}

void CSettings::LoadDefaultDwordIndex ( SettingID /*Type*/, int /*index*/, DWORD & /*Value*/)
{
	g_Notify->BreakPoint(__FILE__,__LINE__); 
}

stdstr CSettings::LoadDefaultString ( SettingID Type )
{
	stdstr Value;
	LoadDefaultString(Type,Value);
	return Value;
}

void CSettings::LoadDefaultString ( SettingID Type, stdstr & Value )
{
	SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
	if (FindInfo == m_SettingInfo.end()) 
	{  
		//if not found do nothing
		UnknownSetting(Type);
	} else {
		if (FindInfo->second->IndexBasedSetting())
		{
			g_Notify->BreakPoint(__FILE__,__LINE__); 
		} else {
			FindInfo->second->LoadDefault(0,Value);
		}
	}
}

void CSettings::LoadDefaultString ( SettingID /*Type*/, char * /*Buffer*/, int /*BufferSize*/ )
{
	g_Notify->BreakPoint(__FILE__,__LINE__); 
}

stdstr CSettings::LoadDefaultStringIndex ( SettingID /*Type*/, int /*index*/ )
{
	g_Notify->BreakPoint(__FILE__,__LINE__); 
	return false;
}

void CSettings::LoadDefaultStringIndex ( SettingID /*Type*/, int /*index*/, stdstr & /*Value*/ )
{
	g_Notify->BreakPoint(__FILE__,__LINE__); 
}

void CSettings::LoadDefaultStringIndex ( SettingID /*Type*/, int /*index*/, char * /*Buffer*/, int /*BufferSize*/ )
{
	g_Notify->BreakPoint(__FILE__,__LINE__); 
}

void CSettings::SaveBool ( SettingID Type, bool Value )
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
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	} else {
		FindInfo->second->Save(0,Value);
	}
	NotifyCallBacks(Type);
}

void CSettings::SaveBoolIndex( SettingID Type, int index, bool Value )
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
		FindInfo->second->Save(index,Value);
	} else {
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	}
	NotifyCallBacks(Type);
}

void CSettings::SaveDword ( SettingID Type, DWORD Value )
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
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	} else {
		FindInfo->second->Save(0,Value);
	}
	NotifyCallBacks(Type);
}

void CSettings::SaveDwordIndex ( SettingID Type, int index, DWORD Value )
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
		FindInfo->second->Save(index,Value);
	} else {
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	}
	NotifyCallBacks(Type);
}

void CSettings::SaveString ( SettingID Type, const stdstr & Value )
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
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	} else {
		FindInfo->second->Save(0,Value);
	}
	NotifyCallBacks(Type);
}

void CSettings::SaveString ( SettingID Type, const char * Buffer )
{
	SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
	if (FindInfo == m_SettingInfo.end()) 
	{  
		//if not found do nothing
		UnknownSetting(Type);
	}
	if (FindInfo->second->IndexBasedSetting())
	{
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	} else {
		FindInfo->second->Save(0,Buffer);
	}
	NotifyCallBacks(Type);
}

void CSettings::SaveStringIndex( SettingID Type, int index, const char * Buffer )
{
	SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
	if (FindInfo == m_SettingInfo.end()) 
	{  
		//if not found do nothing
		UnknownSetting(Type);
	}
	if (FindInfo->second->IndexBasedSetting())
	{
		FindInfo->second->Save(index,Buffer);
	} else {
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	}
	NotifyCallBacks(Type);
}

void CSettings::SaveStringIndex( SettingID Type, int index, const stdstr & Value )
{
	SaveStringIndex(Type,index,Value.c_str());
}

void CSettings::DeleteSetting( SettingID Type )
{
	SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
	if (FindInfo == m_SettingInfo.end()) 
	{  
		//if not found do nothing
		UnknownSetting(Type);
	}
	if (FindInfo->second->IndexBasedSetting())
	{
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	} else {
		FindInfo->second->Delete(0);
	}
	NotifyCallBacks(Type);
}

void CSettings::DeleteSettingIndex( SettingID Type, int index  )
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
	} else {
		g_Notify->BreakPoint(__FILE__,__LINE__); 
	}
	NotifyCallBacks(Type);
}

SettingType CSettings::GetSettingType ( SettingID Type )
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

bool CSettings::IndexBasedSetting ( SettingID Type )
{

	SETTING_HANDLER FindInfo = m_SettingInfo.find(Type);
	if (FindInfo == m_SettingInfo.end()) 
	{  
		return false;
	}
	return FindInfo->second->IndexBasedSetting();
}


void CSettings::SettingTypeChanged( SettingType Type )
{
	for (SETTING_MAP::iterator iter = m_SettingInfo.begin(); iter != m_SettingInfo.end(); iter++)
	{
		if (iter->second->GetSettingType() == Type)
		{
			NotifyCallBacks(iter->first);
		}
	}
}
void CSettings::UnknownSetting (SettingID /*Type*/)
{
#ifdef _DEBUG
	g_Notify->BreakPoint(__FILE__,__LINE__); 
#endif
}

void CSettings::NotifyCallBacks( SettingID Type )
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

void CSettings::RegisterChangeCB(SettingID Type,void * Data, SettingChangedFunc Func)
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
	} else {
		m_Callback.insert(SETTING_CALLBACK::value_type(Type,new_item));
	}
}

void CSettings::UnregisterChangeCB(SettingID Type,void * Data, SettingChangedFunc Func)
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
						SettingID Type = Callback->first;
						_SETTING_CHANGED_CB * Next = item->Next;
						m_Callback.erase(Callback);
						m_Callback.insert(SETTING_CALLBACK::value_type(Type,Next));
					} else {
						m_Callback.erase(Callback);
					}
				} else {
					PrevItem->Next = item->Next;
				}
				delete item;
				item = NULL;
				break;
			}
			PrevItem = item;
			item = item->Next;
		}
	} else {
		UnknownSetting(Type);
		return; 
	}

	if (!bRemoved)
	{
		g_Notify->BreakPoint(__FILE__,__LINE__);
	}
}
