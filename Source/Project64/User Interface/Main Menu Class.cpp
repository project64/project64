#include "stdafx.h"
#include "..\\3rd Party\\HTML Help\\HTMLHELP.H"

CMainMenu::CMainMenu ( CMainGui * hMainWindow ):
	CBaseMenu(),
    m_ResetAccelerators(true)
{
	_Gui      = hMainWindow; //Make a copy of the attatched window
	ResetMenu();

	hMainWindow->SetWindowMenu(this);

	m_ChangeSettingList.push_back(Info_ShortCutsChanged);
	m_ChangeSettingList.push_back(GameRunning_LimitFPS);
	m_ChangeSettingList.push_back(UserInterface_InFullScreen);
	m_ChangeSettingList.push_back(UserInterface_AlwaysOnTop);
	m_ChangeSettingList.push_back(UserInterface_ShowCPUPer);
	m_ChangeSettingList.push_back(Debugger_ProfileCode);
	m_ChangeSettingList.push_back(Debugger_ShowTLBMisses);
	m_ChangeSettingList.push_back(Debugger_ShowUnhandledMemory);
	m_ChangeSettingList.push_back(Debugger_ShowPifErrors);
	m_ChangeSettingList.push_back(Debugger_ShowDListAListCount);
	m_ChangeSettingList.push_back(Debugger_ShowRecompMemSize);
	m_ChangeSettingList.push_back(Debugger_ShowDivByZero);
	m_ChangeSettingList.push_back(Debugger_GenerateLogFiles);
	m_ChangeSettingList.push_back(Debugger_DisableGameFixes);
	m_ChangeSettingList.push_back(Debugger_AppLogLevel);
	m_ChangeSettingList.push_back(Debugger_AppLogFlush);
	m_ChangeSettingList.push_back(Debugger_GenerateDebugLog);
	m_ChangeSettingList.push_back(Game_CurrentSaveState);
	m_ChangeSettingList.push_back(Setting_CurrentLanguage);

	for (SettingList::const_iterator iter = m_ChangeSettingList.begin(); iter != m_ChangeSettingList.end(); iter++)
	{
		g_Settings->RegisterChangeCB(*iter,this,(CSettings::SettingChangedFunc)SettingsChanged);
	}
}

CMainMenu::~CMainMenu()
{
	for (SettingList::const_iterator iter = m_ChangeSettingList.begin(); iter != m_ChangeSettingList.end(); iter++)
	{
		g_Settings->UnregisterChangeCB(*iter,this,(CSettings::SettingChangedFunc)SettingsChanged);
	}
}


void CMainMenu::SettingsChanged (CMainMenu * _this )
{
	_this->ResetMenu();
}

int CMainMenu::ProcessAccelerator ( WND_HANDLE hWnd, void * lpMsg ) {
	if (m_ResetAccelerators)
	{
		m_ResetAccelerators = false;
		RebuildAccelerators();
	}
	if (!m_AccelTable) { return false; }
	return TranslateAccelerator((HWND)hWnd,(HACCEL)m_AccelTable,(LPMSG)lpMsg);
}

bool CMainMenu::ProcessMessage(WND_HANDLE hWnd, DWORD /*FromAccelerator*/, DWORD MenuID) {
	switch (MenuID) {
	case ID_FILE_OPEN_ROM: 
		{
			stdstr File = g_BaseSystem->ChooseFileToOpen(hWnd);
			if (File.length() > 0) {
				g_BaseSystem->RunFileImage(File.c_str());
			}
		}
		break;
	case ID_FILE_ROM_INFO:
		{
			g_BaseSystem->DisplayRomInfo(hWnd);
		}
		break;
	case ID_FILE_STARTEMULATION:
		_Gui->SaveWindowLoc();
		g_BaseSystem->StartEmulation(true);
		break;
	case ID_FILE_ENDEMULATION: 
		WriteTrace(TraceDebug,"ID_FILE_ENDEMULATION");
		g_BaseSystem->CloseCpu(); 
		_Gui->SaveWindowLoc();
		break;
	case ID_FILE_ROMDIRECTORY:   
		WriteTrace(TraceDebug,"ID_FILE_ROMDIRECTORY 1");
		_Gui->SelectRomDir(); 
		WriteTrace(TraceDebug,"ID_FILE_ROMDIRECTORY 2");
		_Gui->RefreshMenu();
		WriteTrace(TraceDebug,"ID_FILE_ROMDIRECTORY 3");
		break;
	case ID_FILE_REFRESHROMLIST: _Gui->RefreshRomBrowser(); break;
	case ID_FILE_EXIT:           DestroyWindow((HWND)hWnd); break;
	case ID_SYSTEM_RESET_SOFT:
		WriteTrace(TraceDebug,"ID_SYSTEM_RESET_SOFT"); 
		g_BaseSystem->ExternalEvent(SysEvent_ResetCPU_Soft); 
		break;
	case ID_SYSTEM_RESET_HARD:
		WriteTrace(TraceDebug,"ID_SYSTEM_RESET_HARD"); 
		g_BaseSystem->ExternalEvent(SysEvent_ResetCPU_Hard); 
		break;
	case ID_SYSTEM_PAUSE:        
		_Gui->SaveWindowLoc();
		WriteTrace(TraceDebug,"ID_SYSTEM_PAUSE");
		if (g_Settings->LoadBool(GameRunning_CPU_Paused))
		{
			g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_FromMenu); 
		} else {
			g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_FromMenu); 
		}
		WriteTrace(TraceDebug,"ID_SYSTEM_PAUSE 1");
		break;
	case ID_SYSTEM_BITMAP:
		{
			stdstr Dir(g_Settings->LoadString(Directory_SnapShot));
			WriteTraceF(TraceGfxPlugin,"CaptureScreen(%s): Starting",Dir.c_str());
			_Plugins->Gfx()->CaptureScreen(Dir.c_str());
			WriteTrace(TraceGfxPlugin,"CaptureScreen: Done");
		}
		break;
	case ID_SYSTEM_LIMITFPS:
		WriteTrace(TraceDebug,"ID_SYSTEM_LIMITFPS");
		g_Settings->SaveBool(GameRunning_LimitFPS,!g_Settings->LoadBool(GameRunning_LimitFPS));
		WriteTrace(TraceDebug,"ID_SYSTEM_LIMITFPS 1");
		break;
	case ID_SYSTEM_SAVE:
		WriteTrace(TraceDebug,"ID_SYSTEM_SAVE"); 
		g_BaseSystem->ExternalEvent(SysEvent_SaveMachineState); 
		break;
	case ID_SYSTEM_SAVEAS:
		{
			char drive[_MAX_DRIVE] ,dir[_MAX_DIR], fname[_MAX_FNAME],ext[_MAX_EXT];
			char Directory[255], SaveFile[255];
			OPENFILENAME openfilename;

			memset(&SaveFile, 0, sizeof(SaveFile));
			memset(&openfilename, 0, sizeof(openfilename));

			g_Settings->LoadString(Directory_LastSave, Directory,sizeof(Directory));

			openfilename.lStructSize  = sizeof( openfilename );
			openfilename.hwndOwner    = (HWND)hWnd;
			openfilename.lpstrFilter  = "PJ64 Saves (*.zip, *.pj)\0*.pj?;*.pj;*.zip;";
			openfilename.lpstrFile    = SaveFile;
			openfilename.lpstrInitialDir    = Directory;
			openfilename.nMaxFile     = MAX_PATH;
			openfilename.Flags        = OFN_HIDEREADONLY;

			g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_SaveGame);
			if (GetSaveFileName (&openfilename)) 
			{

				_splitpath( SaveFile, drive, dir, fname, ext );
				if (_stricmp(ext, ".pj") == 0 || _stricmp(ext, ".zip") == 0) 
				{
					_makepath( SaveFile, drive, dir, fname, NULL );
					_splitpath( SaveFile, drive, dir, fname, ext );
					if (_stricmp(ext, ".pj") == 0) 
					{
						_makepath( SaveFile, drive, dir, fname, NULL );
					}
				}
				g_Settings->SaveString(GameRunning_InstantSaveFile,SaveFile);

				char SaveDir[MAX_PATH];
				_makepath( SaveDir, drive, dir, NULL, NULL );
				g_Settings->SaveString(Directory_LastSave,SaveDir);
				g_System->SaveState();
			}
			g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_SaveGame);
		}
		break;
	case ID_SYSTEM_RESTORE:   WriteTrace(TraceDebug,"ID_SYSTEM_RESTORE");   g_BaseSystem->ExternalEvent(SysEvent_LoadMachineState); break;
	case ID_SYSTEM_LOAD:
		{
			char Directory[255], SaveFile[255];
			OPENFILENAME openfilename;

			memset(&SaveFile, 0, sizeof(SaveFile));
			memset(&openfilename, 0, sizeof(openfilename));

			g_Settings->LoadString(Directory_LastSave, Directory,sizeof(Directory));

			openfilename.lStructSize  = sizeof( openfilename );
			openfilename.hwndOwner    = (HWND)hWnd;
			openfilename.lpstrFilter  = "PJ64 Saves (*.zip, *.pj)\0*.pj?;*.pj;*.zip;";
			openfilename.lpstrFile    = SaveFile;
			openfilename.lpstrInitialDir    = Directory;
			openfilename.nMaxFile     = MAX_PATH;
			openfilename.Flags        = OFN_HIDEREADONLY;

			g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_LoadGame);
			if (GetOpenFileName (&openfilename)) {
				g_Settings->SaveString(GameRunning_InstantSaveFile,SaveFile);
				char SaveDir[MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR], fname[_MAX_FNAME],ext[_MAX_EXT];
				_splitpath( SaveFile, drive, dir, fname, ext );
				_makepath( SaveDir, drive, dir, NULL, NULL );
				g_Settings->SaveString(Directory_LastSave,SaveDir);
				g_System->LoadState();
			}
			g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_LoadGame);
		}
		break;
	case ID_SYSTEM_CHEAT:
		{
			g_BaseSystem->SelectCheats(hWnd);
		}
		break;
	case ID_SYSTEM_GSBUTTON:
		g_BaseSystem->ExternalEvent(SysEvent_GSButtonPressed);
		break;
	case ID_OPTIONS_DISPLAY_FR:
		g_Settings->SaveBool(UserInterface_DisplayFrameRate,!g_Settings->LoadBool(UserInterface_DisplayFrameRate));
		break;
	case ID_OPTIONS_CHANGE_FR:
		switch (g_Settings->LoadDword(UserInterface_FrameDisplayType))
		{
		case FR_VIs:
			g_Settings->SaveDword(UserInterface_FrameDisplayType,FR_DLs);
			break;
		case FR_DLs:
			g_Settings->SaveDword(UserInterface_FrameDisplayType,FR_PERCENT);
			break;
		default:
			g_Settings->SaveDword(UserInterface_FrameDisplayType,FR_VIs);
		}
		break;
	case ID_OPTIONS_INCREASE_SPEED:
		g_BaseSystem->IncreaseSpeed();
		break;
	case ID_OPTIONS_DECREASE_SPEED:
		g_BaseSystem->DecreaeSpeed();
		break;
	case ID_OPTIONS_FULLSCREEN:
		g_BaseSystem->ExternalEvent(SysEvent_ChangingFullScreen);		
		break;
	case ID_OPTIONS_FULLSCREEN2:  
		if (g_Settings->LoadBool(UserInterface_InFullScreen))
		{
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN a");
			_Gui->MakeWindowOnTop(false);
			Notify().SetGfxPlugin(NULL);
			WriteTrace(TraceGfxPlugin,"ChangeWindow: Starting");
			_Plugins->Gfx()->ChangeWindow(); 
			WriteTrace(TraceGfxPlugin,"ChangeWindow: Done");
			ShowCursor(true);
			_Gui->ShowStatusBar(true);
			_Gui->MakeWindowOnTop(g_Settings->LoadBool(UserInterface_AlwaysOnTop));
			g_Settings->SaveBool(UserInterface_InFullScreen,(DWORD)false);
		} else {
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b");
			ShowCursor(false);
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b 1");
			_Gui->ShowStatusBar(false);
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b 2");
			try {
				WriteTrace(TraceGfxPlugin,"ChangeWindow: Starting");
				_Plugins->Gfx()->ChangeWindow(); 
				WriteTrace(TraceGfxPlugin,"ChangeWindow: Done");
			} 
			catch (...)
			{
				WriteTrace(TraceError,"Exception when going to full screen");
				char Message[600];
				sprintf(Message,"Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
				MessageBox(NULL,Message,"Exception",MB_OK);
			}
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b 4");
			_Gui->MakeWindowOnTop(false);
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b 5");
			Notify().SetGfxPlugin(_Plugins->Gfx());
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b 3");
			g_Settings->SaveBool(UserInterface_InFullScreen,true);
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b 6");
		}
		WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN 1");
		break;
	case ID_OPTIONS_ALWAYSONTOP:
		if (g_Settings->LoadDword(UserInterface_AlwaysOnTop)) {
			g_Settings->SaveBool(UserInterface_AlwaysOnTop,false);
			_Gui->MakeWindowOnTop(false);
		} else {
			g_Settings->SaveBool(UserInterface_AlwaysOnTop,true);
			_Gui->MakeWindowOnTop(g_Settings->LoadBool(GameRunning_CPU_Running));
		}
		break;
	case ID_OPTIONS_CONFIG_RSP:  WriteTrace(TraceDebug,"ID_OPTIONS_CONFIG_RSP"); _Plugins->ConfigPlugin((DWORD)hWnd,PLUGIN_TYPE_RSP); break;
	case ID_OPTIONS_CONFIG_GFX:  WriteTrace(TraceDebug,"ID_OPTIONS_CONFIG_GFX"); _Plugins->ConfigPlugin((DWORD)hWnd,PLUGIN_TYPE_GFX); break;
	case ID_OPTIONS_CONFIG_AUDIO:WriteTrace(TraceDebug,"ID_OPTIONS_CONFIG_AUDIO"); _Plugins->ConfigPlugin((DWORD)hWnd,PLUGIN_TYPE_AUDIO); break;
	case ID_OPTIONS_CONFIG_CONT: WriteTrace(TraceDebug,"ID_OPTIONS_CONFIG_CONT"); _Plugins->ConfigPlugin((DWORD)hWnd,PLUGIN_TYPE_CONTROLLER); break;
	case ID_OPTIONS_CPU_USAGE:
		WriteTrace(TraceDebug,"ID_OPTIONS_CPU_USAGE");
		if (g_Settings->LoadBool(UserInterface_ShowCPUPer)) 
		{
			g_Settings->SaveBool(UserInterface_ShowCPUPer,false);
			g_Notify->DisplayMessage(0,"");
		} else {
			g_Settings->SaveBool(UserInterface_ShowCPUPer,true);
		}
		break;
	case ID_OPTIONS_SETTINGS:
		{
			CSettingConfig SettingConfig;
			SettingConfig.Display(hWnd);
		}
		break;
	case ID_PROFILE_PROFILE:
		g_Settings->SaveBool(Debugger_ProfileCode,!g_Settings->LoadBool(Debugger_ProfileCode));
		g_BaseSystem->ExternalEvent(SysEvent_Profile_StartStop);
		break;
	case ID_PROFILE_RESETCOUNTER: g_BaseSystem->ExternalEvent(SysEvent_Profile_ResetLogs); break;
	case ID_PROFILE_GENERATELOG: g_BaseSystem->ExternalEvent(SysEvent_Profile_GenerateLogs); break;
	case ID_DEBUG_SHOW_TLB_MISSES: 
		g_Settings->SaveBool(Debugger_ShowTLBMisses,!g_Settings->LoadBool(Debugger_ShowTLBMisses));
		break;
	case ID_DEBUG_SHOW_UNHANDLED_MEM: 
		g_Settings->SaveBool(Debugger_ShowUnhandledMemory,!g_Settings->LoadBool(Debugger_ShowUnhandledMemory));
		break;
	case ID_DEBUG_SHOW_PIF_ERRORS: 
		g_Settings->SaveBool(Debugger_ShowPifErrors,!g_Settings->LoadBool(Debugger_ShowPifErrors));
		break;
	case ID_DEBUG_SHOW_DLIST_COUNT:
		g_Notify->DisplayMessage(0,"");
		g_Settings->SaveBool(Debugger_ShowDListAListCount,!g_Settings->LoadBool(Debugger_ShowDListAListCount));
		break;
	case ID_DEBUG_SHOW_RECOMP_MEM_SIZE:
		g_Notify->DisplayMessage(0,"");
		g_Settings->SaveBool(Debugger_ShowRecompMemSize,!g_Settings->LoadBool(Debugger_ShowRecompMemSize));
		break;
	case ID_DEBUG_SHOW_DIV_BY_ZERO:
		g_Settings->SaveBool(Debugger_ShowDivByZero,!g_Settings->LoadBool(Debugger_ShowDivByZero));
		break;
	case ID_DEBUG_GENERATE_LOG_FILES:
		g_Settings->SaveBool(Debugger_GenerateLogFiles,!g_Settings->LoadBool(Debugger_GenerateLogFiles));
		break;
	case ID_DEBUG_DISABLE_GAMEFIX:
		g_Settings->SaveBool(Debugger_DisableGameFixes,!g_Settings->LoadBool(Debugger_DisableGameFixes));
		break;
	case ID_DEBUGGER_APPLOG_ERRORS:
		{
			DWORD LogLevel = g_Settings->LoadDword(Debugger_AppLogLevel);
			if ((LogLevel & TraceError) != 0)
			{
				LogLevel &= ~TraceError;
			} else {
				LogLevel |= TraceError;
			}
			g_Settings->SaveDword(Debugger_AppLogLevel, LogLevel );
		}
		break;
	case ID_DEBUGGER_APPLOG_SETTINGS:
		{
			DWORD LogLevel = g_Settings->LoadDword(Debugger_AppLogLevel);
			if ((LogLevel & TraceSettings) != 0)
			{
				LogLevel &= ~TraceSettings;
			} else {

				LogLevel |= TraceSettings;
			}
			g_Settings->SaveDword(Debugger_AppLogLevel, LogLevel );
		}
		break;
	case ID_DEBUGGER_APPLOG_RECOMPILER:
		{
			DWORD LogLevel = g_Settings->LoadDword(Debugger_AppLogLevel);
			if ((LogLevel & TraceRecompiler) != 0)
			{
				LogLevel &= ~TraceRecompiler;
			} else {

				LogLevel |= TraceRecompiler;
			}
			g_Settings->SaveDword(Debugger_AppLogLevel, LogLevel );
		}
		break;
	case ID_DEBUGGER_APPLOG_RSP:
		{
			DWORD LogLevel = g_Settings->LoadDword(Debugger_AppLogLevel);
			if ((LogLevel & TraceRSP) != 0)
			{
				LogLevel &= ~TraceRSP;
			} else {

				LogLevel |= TraceRSP;
			}
			g_Settings->SaveDword(Debugger_AppLogLevel, LogLevel );
		}
		break;
	case ID_DEBUGGER_APPLOG_TLB:
		{
			DWORD LogLevel = g_Settings->LoadDword(Debugger_AppLogLevel);
			if ((LogLevel & TraceTLB) != 0)
			{
				LogLevel &= ~TraceTLB;
			} else {

				LogLevel |= TraceTLB;
			}
			g_Settings->SaveDword(Debugger_AppLogLevel, LogLevel );
		}
		break;
	case ID_DEBUGGER_APPLOG_GFX_PLUGIN:
		{
			DWORD LogLevel = g_Settings->LoadDword(Debugger_AppLogLevel);
			if ((LogLevel & TraceGfxPlugin) != 0)
			{
				LogLevel &= ~TraceGfxPlugin;
			} else {

				LogLevel |= TraceGfxPlugin;
			}
			g_Settings->SaveDword(Debugger_AppLogLevel, LogLevel );
		}
		break;
	case ID_DEBUGGER_APPLOG_DEBUG:
		{
			DWORD LogLevel = g_Settings->LoadDword(Debugger_AppLogLevel);
			if ((LogLevel & TraceDebug) != 0)
			{
				LogLevel &= ~TraceDebug;
			} else {

				LogLevel |= TraceDebug;
			}
			g_Settings->SaveDword(Debugger_AppLogLevel, LogLevel );
		}
		break;
	case ID_DEBUGGER_APPLOG_AUDIO_EMU:
		{
			DWORD LogLevel = g_Settings->LoadDword(Debugger_AppLogLevel);
			if ((LogLevel & TraceAudio) != 0)
			{
				LogLevel &= ~TraceAudio;
			} else {

				LogLevel |= TraceAudio;
			}
			g_Settings->SaveDword(Debugger_AppLogLevel, LogLevel );
		}
		break;
	case ID_DEBUGGER_APPLOG_FLUSH:
		g_Settings->SaveBool(Debugger_AppLogFlush,!g_Settings->LoadBool(Debugger_AppLogFlush));
		break;
	case ID_DEBUGGER_LOGOPTIONS: _Gui->EnterLogOptions(); break;
	case ID_DEBUGGER_GENERATELOG:
		g_Settings->SaveBool(Debugger_GenerateDebugLog,!g_Settings->LoadBool(Debugger_GenerateDebugLog));
		break;
	case ID_DEBUGGER_DUMPMEMORY: 
		g_BaseSystem->Debug_ShowMemoryDump();
		break;
	case ID_DEBUGGER_SEARCHMEMORY: g_BaseSystem->Debug_ShowMemorySearch(); break;
	case ID_DEBUGGER_MEMORY: g_BaseSystem->Debug_ShowMemoryWindow(); break;
	case ID_DEBUGGER_TLBENTRIES: g_BaseSystem->Debug_ShowTLBWindow(); break;
	case ID_DEBUGGER_INTERRUPT_SP: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_SP); break;
	case ID_DEBUGGER_INTERRUPT_SI: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_SI); break;
	case ID_DEBUGGER_INTERRUPT_AI: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_AI); break;
	case ID_DEBUGGER_INTERRUPT_VI: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_VI); break;
	case ID_DEBUGGER_INTERRUPT_PI: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_PI); break;
	case ID_DEBUGGER_INTERRUPT_DP: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_DP); break;
	case ID_CURRENT_SAVE_DEFAULT: 
		Notify().DisplayMessage(3,"Save Slot (%s) selected",GetSaveSlotString(MenuID - ID_CURRENT_SAVE_DEFAULT).c_str());
		g_Settings->SaveDword(Game_CurrentSaveState,(DWORD)(MenuID - ID_CURRENT_SAVE_DEFAULT)); 
		break;
	case ID_CURRENT_SAVE_1: 
	case ID_CURRENT_SAVE_2: 
	case ID_CURRENT_SAVE_3: 
	case ID_CURRENT_SAVE_4: 
	case ID_CURRENT_SAVE_5: 
	case ID_CURRENT_SAVE_6: 
	case ID_CURRENT_SAVE_7: 
	case ID_CURRENT_SAVE_8: 
	case ID_CURRENT_SAVE_9: 
	case ID_CURRENT_SAVE_10: 
		Notify().DisplayMessage(3,"Save Slot (%s) selected",GetSaveSlotString((MenuID - ID_CURRENT_SAVE_1) + 1).c_str());
		g_Settings->SaveDword(Game_CurrentSaveState,(DWORD)((MenuID - ID_CURRENT_SAVE_1) + 1)); 
		break;
	case ID_HELP_CONTENTS:
		{
			char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
			char fname[_MAX_FNAME],ext[_MAX_EXT], HelpFileName[_MAX_PATH];

			GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
			_splitpath(path_buffer, drive, dir, fname, ext);
   			_makepath(HelpFileName, drive, dir, "Project64", "chm");

			if (HtmlHelp((HWND)hWnd, HelpFileName, HH_DISPLAY_TOPIC, 0) == NULL) {
				ShellExecute((HWND)hWnd, "open", HelpFileName, NULL, NULL, SW_SHOW);
			}
		}
		break;
	case ID_HELP_GAMEFAQ:
		{
			char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
			char fname[_MAX_FNAME],ext[_MAX_EXT], HelpFileName[_MAX_PATH], HelpFileName2[_MAX_PATH];

			GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
			_splitpath( path_buffer, drive, dir, fname, ext );
   			_makepath( HelpFileName, drive, dir, "PJgameFAQ", "chm" );
			strcpy(HelpFileName2, HelpFileName);
			strcat(HelpFileName, "::/html/8 FAQ/games/_index.htm");

			if (HtmlHelp((HWND)hWnd, HelpFileName, HH_DISPLAY_TOPIC, 0) == NULL) {
				ShellExecute((HWND)hWnd, "open", HelpFileName2, NULL, NULL, SW_SHOW);
			}
		}
		break;
	case ID_HELP_SUPPORTFORUM: ShellExecute(NULL, "open", "http://forum.pj64-emu.com/", NULL, NULL, SW_SHOWMAXIMIZED); break;
	case ID_HELP_HOMEPAGE: ShellExecute(NULL, "open", "http://www.pj64-emu.com", NULL, NULL, SW_SHOWMAXIMIZED); break;
	case ID_HELP_ABOUT: _Gui->AboutBox(); break;
	case ID_HELP_ABOUTSETTINGFILES: _Gui->AboutIniBox(); break;
	default: 
		if (MenuID >= ID_RECENT_ROM_START && MenuID < ID_RECENT_ROM_END) {
			stdstr FileName;
			if (g_Settings->LoadStringIndex(File_RecentGameFileIndex,MenuID - ID_RECENT_ROM_START,FileName) && 
				FileName.length() > 0) 
			{
				g_BaseSystem->RunFileImage(FileName.c_str());
			}
		}
		if (MenuID >= ID_RECENT_DIR_START && MenuID < ID_RECENT_DIR_END) {
			int Offset = MenuID - ID_RECENT_DIR_START;
			stdstr Dir = g_Settings->LoadStringIndex(Directory_RecentGameDirIndex,Offset);
			if (Dir.length() > 0) {
				g_Settings->SaveString(Directory_Game,Dir.c_str());
				g_Notify->AddRecentDir(Dir.c_str());
				_Gui->RefreshMenu();
				if (_Gui->RomBrowserVisible()) {
					_Gui->RefreshRomBrowser();
				}
			}
		}
		if (MenuID >= ID_LANG_START && MenuID < ID_LANG_END) {
			MENUITEMINFO menuinfo;
			char String[300];

			menuinfo.cbSize = sizeof(MENUITEMINFO);
			menuinfo.fMask = MIIM_TYPE;
			menuinfo.fType = MFT_STRING;
			menuinfo.dwTypeData = String;
			menuinfo.cch = sizeof(String);
			GetMenuItemInfo((HMENU)m_MenuHandle,MenuID,FALSE,&menuinfo);
			
			//See if the language has changed, if not do nothing
			//Set the language
			_Lang->SetLanguage(String);
			_Gui->ResetRomBrowserColomuns();
			break;
		}
		return false;
	}
	return true;
}

/*stdstr CMainMenu::ShortCutString(MSC_MAP & ShortCuts, int  MenuID, CMenuShortCutKey::ACCESS_MODE AccessLevel) {
	Notify().BreakPoint(__FILE__,__LINE__);
	MSC_MAP::iterator MenuItem = ShortCuts.find(MenuID);
	if (MenuItem == ShortCuts.end()) { return ""; }

	const SHORTCUT_KEY_LIST & ShortCutList = MenuItem->second.GetAccelItems();
	for (SHORTCUT_KEY_LIST::const_iterator item = ShortCutList.begin(); item != ShortCutList.end(); item++) 
	{					
		CMenuShortCutKey::ACCESS_MODE ItemMode = item->AccessMode();
		if ((ItemMode & AccessLevel) != AccessLevel )
		{
			continue;
		}
		return item->Name();
	}
	return "";
}*/

stdstr CMainMenu::GetFileLastMod (stdstr FileName)
{
	HANDLE hFile = CreateFile(FileName.c_str(), GENERIC_READ, FILE_SHARE_READ,NULL,
		OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return "";
	}
	FILETIME CreationTime, LastAccessTime, LastWriteTime;
	stdstr LastMod;
	if (GetFileTime(hFile,&CreationTime,&LastAccessTime,&LastWriteTime))
	{
		SYSTEMTIME stUTC, stLocal;

		 // Convert the last-write time to local time.
		FileTimeToSystemTime(&LastWriteTime, &stUTC);
		SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
		
		LastMod.Format(" [%d/%02d/%02d %02d:%02d]",
			stLocal.wYear, stLocal.wMonth, stLocal.wDay,stLocal.wHour, stLocal.wMinute);
	}
	CloseHandle(hFile);

	return LastMod;
}

stdstr CMainMenu::GetSaveSlotString (int Slot)
{
	stdstr SlotName;
	switch (Slot)
	{
	case 0: SlotName = GS(MENU_SLOT_DEFAULT); break;
	case 1: SlotName = GS(MENU_SLOT_1); break;
	case 2: SlotName = GS(MENU_SLOT_2); break;
	case 3: SlotName = GS(MENU_SLOT_3); break;
	case 4: SlotName = GS(MENU_SLOT_4); break;
	case 5: SlotName = GS(MENU_SLOT_5); break;
	case 6: SlotName = GS(MENU_SLOT_6); break;
	case 7: SlotName = GS(MENU_SLOT_7); break;
	case 8: SlotName = GS(MENU_SLOT_8); break;
	case 9: SlotName = GS(MENU_SLOT_9); break;
	case 10: SlotName = GS(MENU_SLOT_10); break;
	}

	if (!g_Settings->LoadBool(GameRunning_CPU_Running)) { return SlotName; }

	stdstr LastSaveTime;

	//check first save name
	stdstr _GoodName = g_Settings->LoadString(Game_GoodName);
	stdstr _InstantSaveDirectory = g_Settings->LoadString(Directory_InstantSave);
	stdstr CurrentSaveName;
	if (Slot != 0) { 
		CurrentSaveName.Format("%s.pj%d",_GoodName.c_str(), Slot);
	} else {
		CurrentSaveName.Format("%s.pj",_GoodName.c_str());
	}
	stdstr_f FileName("%s%s",_InstantSaveDirectory.c_str(),CurrentSaveName.c_str());
	
	if (g_Settings->LoadDword(Setting_AutoZipInstantSave)) 
	{
		stdstr_f ZipFileName("%s.zip",FileName.c_str());
		LastSaveTime = GetFileLastMod(ZipFileName);
	}
	if (LastSaveTime.empty())
	{
		LastSaveTime = GetFileLastMod(FileName);
		
	}

	// Check old file name 
	if (LastSaveTime.empty())
	{
		stdstr _RomName = g_Settings->LoadString(Game_GameName);
		if (Slot > 0) { 
			FileName.Format("%s%s.pj%d", _InstantSaveDirectory.c_str(), _RomName.c_str(),Slot);
		} else {
			FileName.Format("%s%s.pj",_InstantSaveDirectory.c_str(),_RomName.c_str());		
		}
		
		if (g_Settings->LoadBool(Setting_AutoZipInstantSave)) 
		{
			stdstr_f ZipFileName("%s.zip",FileName.c_str());
			LastSaveTime = GetFileLastMod(ZipFileName);
		}
		if (LastSaveTime.empty())
		{
			LastSaveTime = GetFileLastMod(FileName);			
		}
	}

	return stdstr_f("%s%s",SlotName.c_str(),LastSaveTime.c_str()) ;
}

void CMainMenu::FillOutMenu ( MENU_HANDLE hMenu ) {
	CGuard Guard(m_CS);

	MENU_ITEM Item;

	//Get all flags
	bool inBasicMode = g_Settings->LoadBool(UserInterface_BasicMode);
	bool CPURunning  = g_Settings->LoadBool(GameRunning_CPU_Running);
	bool RomLoading  = g_Settings->LoadBool(GameRunning_LoadingInProgress);
	bool RomLoaded   = g_Settings->LoadString(Game_GameName).length() > 0;
	bool RomList     = g_Settings->LoadBool(RomBrowser_Enabled) && !CPURunning;
	
	CMenuShortCutKey::ACCESS_MODE AccessLevel = CMenuShortCutKey::GAME_NOT_RUNNING;
	if (g_Settings->LoadBool(GameRunning_CPU_Running))
	{
		AccessLevel = g_Settings->LoadBool(UserInterface_InFullScreen)  ? 
			CMenuShortCutKey::GAME_RUNNING_FULLSCREEN : 
			CMenuShortCutKey::GAME_RUNNING_WINDOW;
	}

	
	//Get the system information to make the menu
	LanguageList LangList          = _Lang->GetLangList();
	
	MenuItemList LangMenu;
	int Offset = 0;
	for (LanguageList::iterator Language = LangList.begin(); Language != LangList.end(); Language++) {
		Item.Reset(ID_LANG_START + Offset++,EMPTY_STRING,NULL,NULL,Language->LanguageName.c_str());
		if (_Lang->IsCurrentLang(*Language))
		{ 
			Item.ItemTicked = true;
		}
		LangMenu.push_back(Item);
	}

	//Go through the settings to create a list of Recent Roms
	MenuItemList RecentRomMenu;
	DWORD count, RomsToRemember = g_Settings->LoadDword(File_RecentGameFileCount);

	for (count = 0; count < RomsToRemember; count++) {
		stdstr LastRom = g_Settings->LoadStringIndex(File_RecentGameFileIndex,count);
		if (LastRom.empty())
		{
			break;
		}
		stdstr_f MenuString("&%d %s",(count + 1) % 10,LastRom.c_str());
		RecentRomMenu.push_back(MENU_ITEM(ID_RECENT_ROM_START + count,EMPTY_STRING,EMPTY_STDSTR,NULL,MenuString.c_str()));
	}

	
	/* Recent Dir
	****************/
	MenuItemList RecentDirMenu;
	DWORD DirsToRemember = g_Settings->LoadDword(Directory_RecentGameDirCount);
	
	for (count = 0; count < DirsToRemember; count++) 
	{
		stdstr LastDir = g_Settings->LoadStringIndex(Directory_RecentGameDirIndex,count);
		if (LastDir.empty())
		{
			break;
		}
		
		stdstr_f MenuString("&%d %s",(count + 1) % 10,LastDir.c_str());
		RecentDirMenu.push_back(MENU_ITEM(ID_RECENT_DIR_START + count,EMPTY_STRING,EMPTY_STDSTR,NULL,MenuString.c_str()));
	}

	/* File Menu
	****************/
 	MenuItemList FileMenu;
	Item.Reset(ID_FILE_OPEN_ROM,      MENU_OPEN,   m_ShortCuts.ShortCutString(ID_FILE_OPEN_ROM,AccessLevel));
	FileMenu.push_back(Item);
	if (!inBasicMode) {
		Item.Reset(ID_FILE_ROM_INFO,      MENU_ROM_INFO,m_ShortCuts.ShortCutString(ID_FILE_ROM_INFO,AccessLevel));
		Item.ItemEnabled = RomLoaded;
		FileMenu.push_back(Item);
		FileMenu.push_back(MENU_ITEM(SPLITER                    ));
		Item.Reset(ID_FILE_STARTEMULATION,MENU_START,   m_ShortCuts.ShortCutString(ID_FILE_STARTEMULATION,AccessLevel)   );
		Item.ItemEnabled = RomLoaded && !CPURunning;
		FileMenu.push_back(Item);
	}
	Item.Reset(ID_FILE_ENDEMULATION,  MENU_END,     m_ShortCuts.ShortCutString(ID_FILE_ENDEMULATION,AccessLevel)   );
	Item.ItemEnabled = CPURunning;
	FileMenu.push_back(Item);
	FileMenu.push_back(MENU_ITEM(SPLITER                    ));
	Item.Reset(SUB_MENU,              MENU_LANGUAGE, EMPTY_STDSTR,  &LangMenu );
	FileMenu.push_back(Item);
	if (RomList) {
		FileMenu.push_back(MENU_ITEM(SPLITER                    ));
		Item.Reset(ID_FILE_ROMDIRECTORY,  MENU_CHOOSE_ROM,m_ShortCuts.ShortCutString(ID_FILE_ROMDIRECTORY,AccessLevel)       );
		FileMenu.push_back(Item);
		Item.Reset(ID_FILE_REFRESHROMLIST,MENU_REFRESH,m_ShortCuts.ShortCutString(ID_FILE_REFRESHROMLIST,AccessLevel)          );
		FileMenu.push_back(Item);
	}
	
	if (!inBasicMode && RomList) {
		FileMenu.push_back(MENU_ITEM(SPLITER                    ));
		Item.Reset(SUB_MENU,              MENU_RECENT_ROM,EMPTY_STDSTR, &RecentRomMenu);
		if (RecentRomMenu.size() == 0) {
			RecentRomMenu.push_back(MENU_ITEM(SPLITER));
			Item.ItemEnabled = false;
		}
		FileMenu.push_back(Item);
		Item.Reset(SUB_MENU,              MENU_RECENT_DIR,EMPTY_STDSTR, &RecentDirMenu);
		if (RecentDirMenu.size() == 0) {
			RecentDirMenu.push_back(MENU_ITEM(SPLITER));
			Item.ItemEnabled = false;
		}
		FileMenu.push_back(Item);
	} else {
		if (RecentRomMenu.size() != 0) {
			FileMenu.push_back(MENU_ITEM(SPLITER                    ));
			for (MenuItemList::iterator MenuItem = RecentRomMenu.begin(); MenuItem != RecentRomMenu.end(); MenuItem++) 
			{
				FileMenu.push_back(*MenuItem);
			}
		}
	}
	FileMenu.push_back(MENU_ITEM(SPLITER                                      ));
	FileMenu.push_back(MENU_ITEM(ID_FILE_EXIT,          MENU_EXIT,m_ShortCuts.ShortCutString(ID_FILE_EXIT,AccessLevel)             ));

	/* Current Save
	****************/
	MenuItemList CurrentSaveMenu;
	DWORD _CurrentSaveState = g_Settings->LoadDword(Game_CurrentSaveState);
	Item.Reset(ID_CURRENT_SAVE_DEFAULT, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_DEFAULT,AccessLevel),NULL,GetSaveSlotString(0));
	if (_CurrentSaveState == 0) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	CurrentSaveMenu.push_back(MENU_ITEM(SPLITER));
	Item.Reset(ID_CURRENT_SAVE_1, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_1,AccessLevel),NULL,GetSaveSlotString(1));
	if (_CurrentSaveState == 1) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_2, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_2,AccessLevel),NULL,GetSaveSlotString(2));
	if (_CurrentSaveState == 2) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_3, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_3,AccessLevel),NULL,GetSaveSlotString(3));
	if (_CurrentSaveState == 3) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_4, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_4,AccessLevel),NULL,GetSaveSlotString(4));
	if (_CurrentSaveState == 4) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_5, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_5,AccessLevel),NULL,GetSaveSlotString(5));
	if (_CurrentSaveState == 5) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_6, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_6,AccessLevel),NULL,GetSaveSlotString(6));
	if (_CurrentSaveState == 6) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_7, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_7,AccessLevel),NULL,GetSaveSlotString(7));
	if (_CurrentSaveState == 7) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_8, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_8,AccessLevel),NULL,GetSaveSlotString(8));
	if (_CurrentSaveState == 8) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_9, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_9,AccessLevel),NULL,GetSaveSlotString(9));
	if (_CurrentSaveState == 9) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_10, EMPTY_STRING,m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_10,AccessLevel),NULL,GetSaveSlotString(10));
	if (_CurrentSaveState == 10) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);

	/* System Menu
	****************/
	MenuItemList SystemMenu;
	MenuItemList ResetMenu;
	if (inBasicMode) 
	{
		SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_RESET_SOFT, MENU_RESET, m_ShortCuts.ShortCutString(ID_SYSTEM_RESET_SOFT,AccessLevel)     ));
	} else {
		ResetMenu.push_back(MENU_ITEM(ID_SYSTEM_RESET_SOFT, MENU_RESET_SOFT, m_ShortCuts.ShortCutString(ID_SYSTEM_RESET_SOFT,AccessLevel)     ));
		ResetMenu.push_back(MENU_ITEM(ID_SYSTEM_RESET_HARD, MENU_RESET_HARD, m_ShortCuts.ShortCutString(ID_SYSTEM_RESET_HARD,AccessLevel)));
		SystemMenu.push_back(MENU_ITEM(SUB_MENU,MENU_RESET,EMPTY_STDSTR,&ResetMenu));
	}
	if (g_Settings->LoadBool(GameRunning_CPU_Paused)) {
		SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_PAUSE, MENU_RESUME,    m_ShortCuts.ShortCutString(ID_SYSTEM_PAUSE,AccessLevel)));
	} else {
		SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_PAUSE, MENU_PAUSE,    m_ShortCuts.ShortCutString(ID_SYSTEM_PAUSE,AccessLevel)));
	}
	SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_BITMAP, MENU_BITMAP,   m_ShortCuts.ShortCutString(ID_SYSTEM_BITMAP,AccessLevel)));
	SystemMenu.push_back(MENU_ITEM(SPLITER                            ));
	if (!inBasicMode) {
		Item.Reset(ID_SYSTEM_LIMITFPS, MENU_LIMIT_FPS,m_ShortCuts.ShortCutString(ID_SYSTEM_LIMITFPS,AccessLevel) );
		if (g_Settings->LoadBool(GameRunning_LimitFPS)) { Item.ItemTicked = true; }
		SystemMenu.push_back(Item);
		SystemMenu.push_back(MENU_ITEM(SPLITER                            ));
	}
	SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_SAVE, MENU_SAVE,     m_ShortCuts.ShortCutString(ID_SYSTEM_SAVE,AccessLevel)));
	if (!inBasicMode) {
		SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_SAVEAS, MENU_SAVE_AS,  m_ShortCuts.ShortCutString(ID_SYSTEM_SAVEAS,AccessLevel)));
	}
	SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_RESTORE, MENU_RESTORE,  m_ShortCuts.ShortCutString(ID_SYSTEM_RESTORE,AccessLevel)));
	if (!inBasicMode) {
		SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_LOAD, MENU_LOAD,     m_ShortCuts.ShortCutString(ID_SYSTEM_LOAD,AccessLevel)));
	}
	SystemMenu.push_back(MENU_ITEM(SPLITER                            ));
	SystemMenu.push_back(MENU_ITEM(SUB_MENU, MENU_CURRENT_SAVE,  EMPTY_STDSTR, &CurrentSaveMenu ));
	SystemMenu.push_back(MENU_ITEM(SPLITER                            ));
	SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_CHEAT, MENU_CHEAT,    m_ShortCuts.ShortCutString(ID_SYSTEM_CHEAT,AccessLevel)));
	SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_GSBUTTON, MENU_GS_BUTTON,  m_ShortCuts.ShortCutString(ID_SYSTEM_GSBUTTON,AccessLevel)    ));
	
	/* Option Menu
	****************/
	MenuItemList OptionMenu;
	Item.Reset(ID_OPTIONS_FULLSCREEN, MENU_FULL_SCREEN,m_ShortCuts.ShortCutString(ID_OPTIONS_FULLSCREEN,AccessLevel) );
	Item.ItemEnabled = CPURunning;
	if (_Plugins->Gfx() && _Plugins->Gfx()->ChangeWindow == NULL) {
		Item.ItemEnabled = false;
	}
	OptionMenu.push_back(Item);
	if (!inBasicMode) {
		Item.Reset(ID_OPTIONS_ALWAYSONTOP, MENU_ON_TOP,m_ShortCuts.ShortCutString(ID_OPTIONS_ALWAYSONTOP,AccessLevel) );
		if (g_Settings->LoadDword(UserInterface_AlwaysOnTop)) { Item.ItemTicked = true; }
		Item.ItemEnabled = CPURunning;
		OptionMenu.push_back(Item);
	}
	OptionMenu.push_back(MENU_ITEM(SPLITER                   ));

	Item.Reset(ID_OPTIONS_CONFIG_GFX, MENU_CONFG_GFX,m_ShortCuts.ShortCutString(ID_OPTIONS_CONFIG_GFX,AccessLevel));
	if (_Plugins->Gfx() == NULL || _Plugins->Gfx()->Config == NULL) { 
		Item.ItemEnabled = false; 
	}
	OptionMenu.push_back(Item);
	Item.Reset(ID_OPTIONS_CONFIG_AUDIO, MENU_CONFG_AUDIO,m_ShortCuts.ShortCutString(ID_OPTIONS_CONFIG_AUDIO,AccessLevel));
	if (_Plugins->Audio() == NULL || _Plugins->Audio()->Config == NULL) { 
		Item.ItemEnabled = false; 
	}
	OptionMenu.push_back(Item);
	if (!inBasicMode) {
		Item.Reset(ID_OPTIONS_CONFIG_RSP, MENU_CONFG_RSP,m_ShortCuts.ShortCutString(ID_OPTIONS_CONFIG_RSP,AccessLevel));
		if (_Plugins->RSP() == NULL || _Plugins->RSP()->Config == NULL) { 
			Item.ItemEnabled = false; 
		}
		OptionMenu.push_back(Item);
	}
	Item.Reset(ID_OPTIONS_CONFIG_CONT, MENU_CONFG_CTRL,m_ShortCuts.ShortCutString(ID_OPTIONS_CONFIG_CONT,AccessLevel));
	if (_Plugins->Control() == NULL || _Plugins->Control()->Config == NULL) { 
		Item.ItemEnabled = false; 
	}
	OptionMenu.push_back(Item);

	OptionMenu.push_back(MENU_ITEM(SPLITER                   ));
	if (!inBasicMode) {
		Item.Reset(ID_OPTIONS_CPU_USAGE, MENU_SHOW_CPU,m_ShortCuts.ShortCutString(ID_OPTIONS_CPU_USAGE,AccessLevel) );
		if (g_Settings->LoadDword(UserInterface_ShowCPUPer)) { Item.ItemTicked = true; }
		OptionMenu.push_back(Item);
	}
	OptionMenu.push_back(MENU_ITEM(ID_OPTIONS_SETTINGS, MENU_SETTINGS,m_ShortCuts.ShortCutString(ID_OPTIONS_SETTINGS,AccessLevel) ));

	/* Profile Menu
	****************/
	MenuItemList DebugProfileMenu;
	if (bHaveDebugger()) 
	{
		Item.Reset(ID_PROFILE_PROFILE,EMPTY_STRING,EMPTY_STDSTR,NULL,"Profile Code" );
		if (g_Settings->LoadBool(Debugger_ProfileCode)) { Item.ItemTicked = true; }
		DebugProfileMenu.push_back(Item);
		Item.Reset(ID_PROFILE_RESETCOUNTER,EMPTY_STRING,EMPTY_STDSTR,NULL,"Reset Counters" );
		if (!CPURunning) { Item.ItemEnabled = false; }
		DebugProfileMenu.push_back(Item);
		Item.Reset(ID_PROFILE_GENERATELOG,EMPTY_STRING,EMPTY_STDSTR,NULL,"Generate Log File" );
		if (!CPURunning) { Item.ItemEnabled = false; }
		DebugProfileMenu.push_back(Item);
	}

	/* Debugger Menu
	****************/
	MenuItemList DebugMenu;
	MenuItemList DebugLoggingMenu;
	MenuItemList DebugAppLoggingMenu;
	MenuItemList DebugR4300Menu;
	MenuItemList DebugMemoryMenu;
	MenuItemList DebugInterrupt;
	MenuItemList DebugNotificationMenu;
	if (bHaveDebugger()) {		
		/* Debug - Interrupt
		*******************/
		Item.Reset(ID_DEBUGGER_INTERRUPT_SP,EMPTY_STRING,EMPTY_STDSTR,NULL,"SP Interrupt" );
		Item.ItemEnabled = CPURunning;
		DebugInterrupt.push_back(Item);
		Item.Reset(ID_DEBUGGER_INTERRUPT_SI,EMPTY_STRING,EMPTY_STDSTR,NULL,"SI Interrupt" );
		Item.ItemEnabled = CPURunning;
		DebugInterrupt.push_back(Item);
		Item.Reset(ID_DEBUGGER_INTERRUPT_AI,EMPTY_STRING,EMPTY_STDSTR,NULL,"AI Interrupt" );
		Item.ItemEnabled = CPURunning;
		DebugInterrupt.push_back(Item);
		Item.Reset(ID_DEBUGGER_INTERRUPT_VI,EMPTY_STRING,EMPTY_STDSTR,NULL,"VI Interrupt" );
		Item.ItemEnabled = CPURunning;
		DebugInterrupt.push_back(Item);
		Item.Reset(ID_DEBUGGER_INTERRUPT_PI,EMPTY_STRING,EMPTY_STDSTR,NULL,"PI Interrupt" );
		Item.ItemEnabled = CPURunning;
		DebugInterrupt.push_back(Item);
		Item.Reset(ID_DEBUGGER_INTERRUPT_DP,EMPTY_STRING,EMPTY_STDSTR,NULL,"DP Interrupt" );
		Item.ItemEnabled = CPURunning;
		DebugInterrupt.push_back(Item);

		/* Debug - R4300i
		*******************/
		Item.Reset(ID_DEBUGGER_LOGOPTIONS,EMPTY_STRING,EMPTY_STDSTR,NULL,"R4300i &Commands..." );
		Item.ItemEnabled = false;
		DebugR4300Menu.push_back(Item);
		Item.Reset(ID_DEBUGGER_R4300REGISTERS,EMPTY_STRING,EMPTY_STDSTR,NULL,"R4300i &Registers..." );
		Item.ItemEnabled = true;
		DebugR4300Menu.push_back(Item);
		Item.Reset(ID_DEBUG_DISABLE_GAMEFIX,EMPTY_STRING,EMPTY_STDSTR,NULL,"Disable Game Fixes" );
		if (g_Settings->LoadBool(Debugger_DisableGameFixes)) { 
			Item.ItemTicked = true;
		}
		DebugR4300Menu.push_back(Item);
		Item.Reset(SUB_MENU, EMPTY_STRING,EMPTY_STDSTR, &DebugInterrupt,"&Generate Interrupt");
		DebugR4300Menu.push_back(Item);

		/* Debug - Memory
		****************/
		Item.Reset(ID_DEBUGGER_MEMORY,EMPTY_STRING,EMPTY_STDSTR,NULL,"View..." );
		DebugMemoryMenu.push_back(Item);
		Item.Reset(ID_DEBUGGER_SEARCHMEMORY,EMPTY_STRING,EMPTY_STDSTR,NULL,"Search..." );
		DebugMemoryMenu.push_back(Item);
		Item.Reset(ID_DEBUGGER_DUMPMEMORY,EMPTY_STRING,EMPTY_STDSTR,NULL,"Dump..." );
		DebugMemoryMenu.push_back(Item);
		Item.Reset(ID_DEBUGGER_TLBENTRIES,EMPTY_STRING,EMPTY_STDSTR,NULL,"TLB Entries..." );
		DebugMemoryMenu.push_back(Item);

		/* Debug - App logging
		*******************/
		{
			DWORD LogLevel = g_Settings->LoadDword(Debugger_AppLogLevel);
			
			Item.Reset(ID_DEBUGGER_APPLOG_ERRORS,EMPTY_STRING,EMPTY_STDSTR,NULL,"Error Messages" );
			if ((LogLevel & TraceError) != 0) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);
			
			Item.Reset(ID_DEBUGGER_APPLOG_SETTINGS,EMPTY_STRING,EMPTY_STDSTR,NULL,"Settings" );
			if ((LogLevel & TraceSettings) != 0) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);
			
			Item.Reset(ID_DEBUGGER_APPLOG_RECOMPILER,EMPTY_STRING,EMPTY_STDSTR,NULL,"Recompiler" );
			if ((LogLevel & TraceRecompiler) != 0) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);

			Item.Reset(ID_DEBUGGER_APPLOG_RSP,EMPTY_STRING,EMPTY_STDSTR,NULL,"RSP" );
			if ((LogLevel & TraceRSP) != 0) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);

			Item.Reset(ID_DEBUGGER_APPLOG_TLB,EMPTY_STRING,EMPTY_STDSTR,NULL,"TLB" );
			if ((LogLevel & TraceTLB) != 0) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);

			Item.Reset(ID_DEBUGGER_APPLOG_GFX_PLUGIN,EMPTY_STRING,EMPTY_STDSTR,NULL,"Gfx Plugin" );
			if ((LogLevel & TraceGfxPlugin) != 0) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);

			Item.Reset(ID_DEBUGGER_APPLOG_AUDIO_EMU,EMPTY_STRING,EMPTY_STDSTR,NULL,"Audio Emulation" );
			if ((LogLevel & TraceAudio) != 0) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);

			Item.Reset(ID_DEBUGGER_APPLOG_DEBUG,EMPTY_STRING,EMPTY_STDSTR,NULL,"Debug Messages" );
			if ((LogLevel & TraceDebug) != 0) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);

			DebugAppLoggingMenu.push_back(MENU_ITEM(SPLITER                   ));

			Item.Reset(ID_DEBUGGER_APPLOG_FLUSH,EMPTY_STRING,EMPTY_STDSTR,NULL,"Auto flush file" );
			if (g_Settings->LoadBool(Debugger_AppLogFlush)) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);
		}


		/* Debug - Logging
		*******************/
		Item.Reset(ID_DEBUGGER_LOGOPTIONS,EMPTY_STRING,EMPTY_STDSTR,NULL,"Log Options..." );
		DebugLoggingMenu.push_back(Item);
		
		
		Item.Reset(ID_DEBUGGER_GENERATELOG,EMPTY_STRING,EMPTY_STDSTR,NULL,"Generate Log" );
		if (g_Settings->LoadBool(Debugger_GenerateDebugLog)) { Item.ItemTicked = true; }
		DebugLoggingMenu.push_back(Item);

		/* Debugger Main Menu
		****************/
		Item.Reset(ID_DEBUGGER_BREAKPOINTS, EMPTY_STRING,EMPTY_STDSTR, NULL,"Breakpoint...");
		Item.ItemEnabled = CPURunning;
		DebugMenu.push_back(Item);
		DebugMenu.push_back(MENU_ITEM(SPLITER));
		
		/* Debug - RSP
		*******************/
		if (_Plugins->RSP() != NULL && IsMenu((HMENU)_Plugins->RSP()->GetDebugMenu())) 
		{ 
			Item.Reset(ID_PLUGIN_MENU,EMPTY_STRING,NULL,_Plugins->RSP()->GetDebugMenu(),"&RSP" );
			DebugMenu.push_back(Item);
		}

		/* Debug - RDP
		*******************/
		if (_Plugins->Gfx() != NULL && IsMenu((HMENU)_Plugins->Gfx()->GetDebugMenu())) 
		{ 
			Item.Reset(ID_PLUGIN_MENU,EMPTY_STRING,NULL,_Plugins->Gfx()->GetDebugMenu(),"&RDP" );
			DebugMenu.push_back(Item);
		}

		/* Notification Menu
		*******************/
		Item.Reset(ID_DEBUG_SHOW_UNHANDLED_MEM,EMPTY_STRING,EMPTY_STDSTR,NULL,"On Unhandled Memory Actions" );
		if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { 
			Item.ItemTicked = true;
		}
		DebugNotificationMenu.push_back(Item);
		Item.Reset(ID_DEBUG_SHOW_PIF_ERRORS,EMPTY_STRING,EMPTY_STDSTR,NULL,"On PIF Errors" );
		if (g_Settings->LoadBool(Debugger_ShowPifErrors)) { 
			Item.ItemTicked = true;
		}
		DebugNotificationMenu.push_back(Item);
		Item.Reset(ID_DEBUG_SHOW_DIV_BY_ZERO,EMPTY_STRING,EMPTY_STDSTR,NULL,"On Div By Zero" );
		if (g_Settings->LoadBool(Debugger_ShowDivByZero)) { 
			Item.ItemTicked = true;
		}
		DebugNotificationMenu.push_back(Item);

		Item.Reset(SUB_MENU, EMPTY_STRING,EMPTY_STDSTR, &DebugR4300Menu,"&R4300i");
		DebugMenu.push_back(Item);
		Item.Reset(SUB_MENU, EMPTY_STRING,EMPTY_STDSTR, &DebugMemoryMenu,"Memory");
		Item.ItemEnabled = CPURunning;
		DebugMenu.push_back(Item);
		DebugMenu.push_back(MENU_ITEM(SPLITER));
		Item.Reset(SUB_MENU, EMPTY_STRING,EMPTY_STDSTR, &DebugProfileMenu,"Profile");
		DebugMenu.push_back(Item);
		Item.Reset(SUB_MENU, EMPTY_STRING,EMPTY_STDSTR, &DebugAppLoggingMenu,"App Logging");
		DebugMenu.push_back(Item);
		Item.Reset(SUB_MENU, EMPTY_STRING,EMPTY_STDSTR, &DebugLoggingMenu,"Logging");
		DebugMenu.push_back(Item);
		Item.Reset(SUB_MENU, EMPTY_STRING,EMPTY_STDSTR, &DebugNotificationMenu,"Notification");
		DebugMenu.push_back(Item);
		DebugMenu.push_back(MENU_ITEM(SPLITER));
		Item.Reset(ID_DEBUG_SHOW_TLB_MISSES,EMPTY_STRING,EMPTY_STDSTR,NULL,"Show TLB Misses" );
		if (g_Settings->LoadBool(Debugger_ShowTLBMisses)) { 
			Item.ItemTicked = true;
		}
		Item.Reset(ID_DEBUG_SHOW_DLIST_COUNT,EMPTY_STRING,EMPTY_STDSTR,NULL,"Display Alist/Dlist Count" );
		if (g_Settings->LoadBool(Debugger_ShowDListAListCount)) { 
			Item.ItemTicked = true;
		}
		DebugMenu.push_back(Item);
		Item.Reset(ID_DEBUG_SHOW_RECOMP_MEM_SIZE,EMPTY_STRING,EMPTY_STDSTR,NULL,"Display Recompiler Code Buffer Size" );
		if (g_Settings->LoadBool(Debugger_ShowRecompMemSize)) { 
			Item.ItemTicked = true;
		}
		DebugMenu.push_back(Item);
		DebugMenu.push_back(MENU_ITEM(SPLITER));
		Item.Reset(ID_DEBUG_GENERATE_LOG_FILES,EMPTY_STRING,EMPTY_STDSTR,NULL,"Generate Log Files" );
		if (g_Settings->LoadBool(Debugger_GenerateLogFiles)) { 
			Item.ItemTicked = true;
		}
		DebugMenu.push_back(Item);
	}

	/* Help Menu
	****************/
	MenuItemList HelpMenu;
	if (g_Settings->LoadBool(Beta_IsBetaVersion))
	{
		stdstr_f User("Beta For: %s",g_Settings->LoadString(Beta_UserName).c_str());
		stdstr_f Email("Email: %s",g_Settings->LoadString(Beta_EmailAddress).c_str());
		HelpMenu.push_back(MENU_ITEM(NO_ID, EMPTY_STRING,EMPTY_STDSTR,NULL,User   ));
		HelpMenu.push_back(MENU_ITEM(NO_ID, EMPTY_STRING,EMPTY_STDSTR,NULL,Email   ));
		HelpMenu.push_back(MENU_ITEM(SPLITER                   ));
	}

	HelpMenu.push_back(MENU_ITEM(ID_HELP_CONTENTS, MENU_USER_MAN   ));
	HelpMenu.push_back(MENU_ITEM(ID_HELP_GAMEFAQ, MENU_GAME_FAQ   ));
	HelpMenu.push_back(MENU_ITEM(SPLITER                   ));
	HelpMenu.push_back(MENU_ITEM(ID_HELP_SUPPORTFORUM, MENU_FORUM      ));
	HelpMenu.push_back(MENU_ITEM(ID_HELP_HOMEPAGE, MENU_HOMEPAGE   ));
	HelpMenu.push_back(MENU_ITEM(SPLITER                   ));
	if (!inBasicMode) {
		HelpMenu.push_back(MENU_ITEM(ID_HELP_ABOUTSETTINGFILES,      MENU_ABOUT_INI  ));
	}
	HelpMenu.push_back(MENU_ITEM(ID_HELP_ABOUT, MENU_ABOUT_PJ64 ));

	/* Main Title bar Menu
	***********************/
	MenuItemList MainTitleMenu;
	Item.Reset(SUB_MENU, MENU_FILE,    EMPTY_STDSTR, &FileMenu);
	if (RomLoading) { Item.ItemEnabled = false; }
	MainTitleMenu.push_back(Item);
	if (CPURunning) {
		Item.Reset(SUB_MENU, MENU_SYSTEM,  EMPTY_STDSTR, &SystemMenu);
		if (RomLoading) { Item.ItemEnabled = false; }
		MainTitleMenu.push_back(Item);
	}
	Item.Reset(SUB_MENU, MENU_OPTIONS,    EMPTY_STDSTR, &OptionMenu);
	if (RomLoading) { Item.ItemEnabled = false; }
	MainTitleMenu.push_back(Item);
	if (!inBasicMode) {
		if (bHaveDebugger()) {
			Item.Reset(SUB_MENU, MENU_DEBUGGER,    EMPTY_STDSTR, &DebugMenu);
			if (RomLoading) { Item.ItemEnabled = false; }
			MainTitleMenu.push_back(Item);
		}
	}
	Item.Reset(SUB_MENU, MENU_HELP,    EMPTY_STDSTR, &HelpMenu);
	if (RomLoading) { Item.ItemEnabled = false; }
	MainTitleMenu.push_back(Item);

	AddMenu(hMenu,MainTitleMenu);
}

void CMainMenu::RebuildAccelerators(void) {
	CGuard Guard(m_CS);

	//Delete the old accel list
	WriteTrace(TraceDebug,"CMainMenu::RebuildAccelerators - Start");

	HACCEL m_OldAccelTable = (HACCEL)m_AccelTable;
	m_AccelTable = m_ShortCuts.GetAcceleratorTable();
	if (m_OldAccelTable) { 
		DestroyAcceleratorTable(m_OldAccelTable);
	}
	WriteTrace(TraceDebug,"CMainMenu::RebuildAccelerators - Done");
}

void CMainMenu::ResetMenu(void) {
	WriteTrace(TraceDebug,"CMainMenu::ResetMenu - Start");
	
	{
		CGuard Guard(m_CS);
		m_ShortCuts.Load();
	}

	if (!g_Settings->LoadBool(UserInterface_InFullScreen))
	{
		//Create a new window with all the items
		WriteTrace(TraceDebug,"CMainMenu::ResetMenu - Create Menu");
		MENU_HANDLE hMenu = (MENU_HANDLE)CreateMenu();
		FillOutMenu(hMenu);
		WriteTrace(TraceDebug,"CMainMenu::ResetMenu - Create Menu Done");

		//save old menu to destroy latter
		MENU_HANDLE OldMenuHandle;
		{
			CGuard Guard(m_CS);
			OldMenuHandle = m_MenuHandle;

			//save handle and re-attach to a window
			WriteTrace(TraceDebug,"CMainMenu::ResetMenu - Attach Menu");
			m_MenuHandle = hMenu;
		}
		_Gui->SetWindowMenu(this);

		WriteTrace(TraceDebug,"CMainMenu::ResetMenu - Remove plugin menu");
		if (_Plugins->Gfx() != NULL && IsMenu((HMENU)_Plugins->Gfx()->GetDebugMenu()))
		{
			RemoveMenu((HMENU)OldMenuHandle,(DWORD)_Plugins->Gfx()->GetDebugMenu(), MF_BYCOMMAND); 
		}
		if (_Plugins->RSP() != NULL && IsMenu((HMENU)_Plugins->RSP()->GetDebugMenu()))
		{
			RemoveMenu((HMENU)OldMenuHandle,(DWORD)_Plugins->RSP()->GetDebugMenu(), MF_BYCOMMAND); 
		}
		WriteTrace(TraceDebug,"CMainMenu::ResetMenu - Destroy Old Menu");

		//Destroy the old menu
		DestroyMenu((HMENU)OldMenuHandle); 
	}

	ResetAccelerators();

	WriteTrace(TraceDebug,"CMainMenu::ResetMenu Done");
}

/*LanguageStringID CMainMenu::GetShortCutMenuItemName(MSC_MAP * ShortCuts, WORD key, bool bCtrl, bool bAlt, bool bShift, CMenuShortCutKey::ACCESS_MODE Access) {
	Notify().BreakPoint(__FILE__,__LINE__);
	/*for (MSC_MAP::iterator Item = ShortCuts->begin(); Item != ShortCuts->end(); Item++) {
		CMenuShortCutKey & short_cut = Item->second;
		
		for (SHORTCUT_KEY_LIST::const_iterator AccelItem = short_cut.GetAccelItems().begin(); AccelItem != short_cut.GetAccelItems().end(); AccelItem++) {
			if (!AccelItem->Same(key,bCtrl,bAlt,bShift,Access)) { continue; }
			return short_cut.Title();
		}	
	}
	return EMPTY_STRING;
}

void CMainMenu::SaveShortCuts(MSC_MAP * ShortCuts) {
	Notify().BreakPoint(__FILE__,__LINE__);
	stdstr FileName = g_Settings->LoadString(SupportFile_ShortCuts);
	FILE *file = fopen(FileName.c_str(),"w");
	for (MSC_MAP::iterator Item = ShortCuts->begin(); Item != ShortCuts->end(); Item++) {
		for (SHORTCUT_KEY_LIST::const_iterator ShortCut = Item->second.GetAccelItems().begin(); ShortCut != Item->second.GetAccelItems().end(); ShortCut++) {
			fprintf(file,"%d,%d,%d,%d,%d,%d\n",Item->first,ShortCut->Key(),ShortCut->Ctrl(),
				ShortCut->Alt(),ShortCut->Shift(),ShortCut->AccessMode());
		}
	}
	fclose(file);
}*/
