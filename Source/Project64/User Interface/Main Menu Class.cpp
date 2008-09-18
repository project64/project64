#include "..\\User Interface.h"
#include "..\\N64 System.h"
#include "..\\Plugin.h"
#include <windows.h>
#include "..\\3rd Party\\HTML Help\\HTMLHELP.H"
#include <common/CriticalSection.h>

CMainMenu::CMainMenu ( CMainGui * hMainWindow, CN64System * N64System ):
	CBaseMenu(),
    m_ResetAccelerators(true)
{
	_Gui      = hMainWindow; //Make a copy of the attatched window
	_System   = N64System;   //Make a copy of the n64 system that is being interacted with
	ResetMenu();

	hMainWindow->SetWindowMenu(this);
}

CMainMenu::~CMainMenu()
{
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

bool CMainMenu::ProcessMessage(WND_HANDLE hWnd, DWORD FromAccelerator, DWORD MenuID) {
	switch (MenuID) {
	case ID_FILE_OPEN_ROM: 
		{
			stdstr File = _System->ChooseFileToOpen(hWnd);
			if (File.length() > 0) {
				_System->RunFileImage(File.c_str());
			}
		}
		break;
	case ID_FILE_ROM_INFO:
		{
			_System->DisplayRomInfo(hWnd);
		}
		break;
	case ID_FILE_STARTEMULATION:
		_Gui->SaveWindowLoc();
		_System->StartEmulation(true);
		break;
	case ID_FILE_ENDEMULATION: 
		WriteTrace(TraceDebug,"ID_FILE_ENDEMULATION");
		_System->CloseCpu(); 
		_Gui->SaveWindowLoc();
		break;
	case ID_FILE_ROMDIRECTORY:   
		WriteTrace(TraceDebug,"ID_FILE_ROMDIRECTORY 1");
		_Gui->SelectRomDir(_Gui->GetNotifyClass()); 
		WriteTrace(TraceDebug,"ID_FILE_ROMDIRECTORY 2");
		_Gui->RefreshMenu();
		WriteTrace(TraceDebug,"ID_FILE_ROMDIRECTORY 3");
		break;
	case ID_FILE_REFRESHROMLIST: _Gui->RefreshRomBrowser(); break;
	case ID_FILE_EXIT:           DestroyWindow((HWND)hWnd); break;
	case ID_SYSTEM_RESET_SOFT:
		WriteTrace(TraceDebug,"ID_SYSTEM_RESET_SOFT"); 
		_System->ExternalEvent(ResetCPU_Soft); 
		break;
	case ID_SYSTEM_RESET_HARD:
		WriteTrace(TraceDebug,"ID_SYSTEM_RESET_HARD"); 
		_System->ExternalEvent(ResetCPU_Hard); 
		break;
	case ID_SYSTEM_PAUSE:        
		_Gui->SaveWindowLoc();
		WriteTrace(TraceDebug,"ID_SYSTEM_PAUSE");
		if (_Settings->LoadDword(CPU_Paused))
		{
			_System->ExternalEvent(ResumeCPU_FromMenu); 
		} else {
			_System->ExternalEvent(PauseCPU_FromMenu); 
		}
		WriteTrace(TraceDebug,"ID_SYSTEM_PAUSE 1");
		break;
	case ID_SYSTEM_BITMAP:
		{
			stdstr Dir(_Settings->LoadString(SnapShotDir));
			WriteTraceF(TraceGfxPlugin,"CaptureScreen(%s): Starting",Dir.c_str());
			_System->Plugins()->Gfx()->CaptureScreen(Dir.c_str());
			WriteTrace(TraceGfxPlugin,"CaptureScreen: Done");
		}
		break;
	case ID_SYSTEM_LIMITFPS:
		WriteTrace(TraceDebug,"ID_SYSTEM_LIMITFPS");
		_Settings->SaveDword(LimitFPS,!_Settings->LoadDword(LimitFPS));
		WriteTrace(TraceDebug,"ID_SYSTEM_LIMITFPS 1");
		ResetMenu();
		break;
	case ID_SYSTEM_SAVE:       WriteTrace(TraceDebug,"ID_SYSTEM_SAVE"); _System->ExternalEvent(SaveMachineState); break;
	case ID_SYSTEM_SAVEAS:
		{
			char drive[_MAX_DRIVE] ,dir[_MAX_DIR], fname[_MAX_FNAME],ext[_MAX_EXT];
			char Directory[255], SaveFile[255];
			OPENFILENAME openfilename;

			memset(&SaveFile, 0, sizeof(SaveFile));
			memset(&openfilename, 0, sizeof(openfilename));

			_Settings->LoadString(LastSaveDir, Directory,sizeof(Directory));

			openfilename.lStructSize  = sizeof( openfilename );
			openfilename.hwndOwner    = (HWND)hWnd;
			openfilename.lpstrFilter  = "PJ64 Saves (*.zip, *.pj)\0*.pj?;*.pj;*.zip;";
			openfilename.lpstrFile    = SaveFile;
			openfilename.lpstrInitialDir    = Directory;
			openfilename.nMaxFile     = MAX_PATH;
			openfilename.Flags        = OFN_HIDEREADONLY;

			_System->ExternalEvent(PauseCPU_SaveGame); 

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
				_Settings->SaveString(InstantSaveFile,SaveFile);

				char SaveDir[MAX_PATH];
				_makepath( SaveDir, drive, dir, NULL, NULL );
				_Settings->SaveString(LastSaveDir,SaveDir);

				_System->ExternalEvent(SaveMachineState);
			}

			_System->ExternalEvent(ResumeCPU_SaveGame);
		}
		break;
	case ID_SYSTEM_RESTORE:   WriteTrace(TraceDebug,"ID_SYSTEM_RESTORE");   _System->ExternalEvent(LoadMachineState); break;
	case ID_SYSTEM_LOAD:
		{
			char Directory[255], SaveFile[255];
			OPENFILENAME openfilename;

			memset(&SaveFile, 0, sizeof(SaveFile));
			memset(&openfilename, 0, sizeof(openfilename));

			_Settings->LoadString(LastSaveDir, Directory,sizeof(Directory));

			openfilename.lStructSize  = sizeof( openfilename );
			openfilename.hwndOwner    = (HWND)hWnd;
			openfilename.lpstrFilter  = "PJ64 Saves (*.zip, *.pj)\0*.pj?;*.pj;*.zip;";
			openfilename.lpstrFile    = SaveFile;
			openfilename.lpstrInitialDir    = Directory;
			openfilename.nMaxFile     = MAX_PATH;
			openfilename.Flags        = OFN_HIDEREADONLY;

			_System->ExternalEvent(PauseCPU_LoadGame); 

			if (GetOpenFileName (&openfilename)) {
				_Settings->SaveString(InstantSaveFile,SaveFile);

				char SaveDir[MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR], fname[_MAX_FNAME],ext[_MAX_EXT];
				_splitpath( SaveFile, drive, dir, fname, ext );
				_makepath( SaveDir, drive, dir, NULL, NULL );
				_Settings->SaveString(LastSaveDir,SaveDir);

				_System->ExternalEvent(LoadMachineState);
			}
			_System->ExternalEvent(ResumeCPU_LoadGame);
		}
		break;
	case ID_SYSTEM_CHEAT:
		{
			_System->SelectCheats(hWnd);
		}
		break;
	case ID_SYSTEM_GSBUTTON:
		_System->ExternalEvent(GSButtonPressed);
		break;
	case ID_OPTIONS_DISPLAY_FR:
		_Settings->SaveDword(DisplayFrameRate,!_Settings->LoadDword(DisplayFrameRate));
		break;
	case ID_OPTIONS_CHANGE_FR:
		switch (_Settings->LoadDword(FrameDisplayType))
		{
		case FR_VIs:
			_Settings->SaveDword(FrameDisplayType,FR_DLs);
			break;
		case FR_DLs:
			_Settings->SaveDword(FrameDisplayType,FR_PERCENT);
			break;
		default:
			_Settings->SaveDword(FrameDisplayType,FR_VIs);
		}
		break;
	case ID_OPTIONS_INCREASE_SPEED:
		_System->IncreaseSpeed();
		break;
	case ID_OPTIONS_DECREASE_SPEED:
		_System->DecreaeSpeed();
		break;
	case ID_OPTIONS_FULLSCREEN:
		_System->ExternalEvent(ChangingFullScreen);		
		break;
	case ID_OPTIONS_FULLSCREEN2:  
		if (_Settings->LoadDword(InFullScreen))
		{
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN a");
			_Gui->MakeWindowOnTop(false);
			Notify().SetGfxPlugin(NULL);
			WriteTrace(TraceGfxPlugin,"ChangeWindow: Starting");
			_System->Plugins()->Gfx()->ChangeWindow(); 
			WriteTrace(TraceGfxPlugin,"ChangeWindow: Done");
			ShowCursor(true);
			_Gui->ShowStatusBar(true);
			_Gui->MakeWindowOnTop(_Settings->LoadDword(AlwaysOnTop) != 0);
			_Settings->SaveDword(InFullScreen,(DWORD)false);
			ResetMenu();
		} else {
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b");
			ShowCursor(false);
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b 1");
			_Gui->ShowStatusBar(false);
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b 2");
			_Settings->SaveDword(InFullScreen,true);
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b 3");
			try {
				WriteTrace(TraceGfxPlugin,"ChangeWindow: Starting");
				_System->Plugins()->Gfx()->ChangeWindow(); 
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
			Notify().SetGfxPlugin(_System->Plugins()->Gfx());
			WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN b 6");
			ResetMenu();
		}
		WriteTrace(TraceDebug,"ID_OPTIONS_FULLSCREEN 1");
		break;
	case ID_OPTIONS_ALWAYSONTOP:
		if (_Settings->LoadDword(AlwaysOnTop)) {
			_Settings->SaveDword(AlwaysOnTop,(DWORD)false);
			_Gui->MakeWindowOnTop(false);
		} else {
			_Settings->SaveDword(AlwaysOnTop,true);
			_Gui->MakeWindowOnTop(_Settings->LoadDword(CPU_Running) != 0);
		}
		ResetMenu();
		break;
	case ID_OPTIONS_CONFIG_RSP:  WriteTrace(TraceDebug,"ID_OPTIONS_CONFIG_RSP"); _System->Plugins()->ConfigPlugin((DWORD)hWnd,PLUGIN_TYPE_RSP); break;
	case ID_OPTIONS_CONFIG_GFX:  WriteTrace(TraceDebug,"ID_OPTIONS_CONFIG_GFX"); _System->Plugins()->ConfigPlugin((DWORD)hWnd,PLUGIN_TYPE_GFX); break;
	case ID_OPTIONS_CONFIG_AUDIO:WriteTrace(TraceDebug,"ID_OPTIONS_CONFIG_AUDIO"); _System->Plugins()->ConfigPlugin((DWORD)hWnd,PLUGIN_TYPE_AUDIO); break;
	case ID_OPTIONS_CONFIG_CONT: WriteTrace(TraceDebug,"ID_OPTIONS_CONFIG_CONT"); _System->Plugins()->ConfigPlugin((DWORD)hWnd,PLUGIN_TYPE_CONTROLLER); break;
	case ID_OPTIONS_CPU_USAGE:
		WriteTrace(TraceDebug,"ID_OPTIONS_CPU_USAGE");
		if (_Settings->LoadDword(ShowCPUPer)) {
			_Settings->SaveDword(ShowCPUPer,(DWORD)false);
			_Gui->GetNotifyClass()->DisplayMessage(0,"");
		} else {
			_Settings->SaveDword(ShowCPUPer,true);
		}
		_System->ExternalEvent(CPUUsageTimerChanged);
		ResetMenu();
		break;
	//case ID_OPTIONS_SETTINGS: _Settings->Config((void *)hWnd,_System,_Gui); break;
	case ID_PROFILE_PROFILE:
		_Settings->SaveDword(ProfileCode,!_Settings->LoadDword(ProfileCode));
		_System->ExternalEvent(Profile_StartStop);
		ResetMenu();
		break;
	case ID_PROFILE_RESETCOUNTER: _System->ExternalEvent(Profile_ResetLogs); break;
	case ID_PROFILE_GENERATELOG: _System->ExternalEvent(Profile_GenerateLogs); break;
	case ID_DEBUG_SHOW_UNHANDLED_MEM: 
		_Settings->SaveDword(ShowUnhandledMemory,!_Settings->LoadDword(ShowUnhandledMemory));
		ResetMenu();
		break;
	case ID_DEBUG_SHOW_PIF_ERRORS: 
		_Settings->SaveDword(ShowPifErrors,!_Settings->LoadDword(ShowPifErrors));
		ResetMenu();
		break;
	case ID_DEBUG_SHOW_DLIST_COUNT:
		_Gui->GetNotifyClass()->DisplayMessage(0,"");
		_Settings->SaveDword(ShowDListAListCount,!_Settings->LoadDword(ShowDListAListCount));
		ResetMenu();
		break;
	case ID_DEBUG_SHOW_RECOMP_MEM_SIZE:
		_Gui->GetNotifyClass()->DisplayMessage(0,"");
		_Settings->SaveDword(ShowRecompMemSize,!_Settings->LoadDword(ShowRecompMemSize));
		ResetMenu();
		break;
	case ID_DEBUG_SHOW_CHECK_OPUSAGE:
		_Settings->SaveDword(ShowCheckOpUsageErrors,!_Settings->LoadDword(ShowCheckOpUsageErrors));
		ResetMenu();
		break;
	case ID_DEBUG_GENERATE_LOG_FILES:
		_Settings->SaveDword(GenerateLogFiles,!_Settings->LoadDword(GenerateLogFiles));
		ResetMenu();
		break;
	case ID_DEBUG_DISABLE_GAMEFIX:
		_Settings->SaveDword(DisableGameFixes,!_Settings->LoadDword(DisableGameFixes));
		ResetMenu();
		break;
	case ID_DEBUGGER_APPLOG_ERRORS:
		{
			DWORD LogLevel = _Settings->LoadDword(AppLogLevel);
			if ((LogLevel & TraceError) != 0)
			{
				LogLevel &= ~TraceError;
			} else {

				LogLevel |= TraceError;
			}
			_Settings->SaveDword(AppLogLevel, LogLevel );
		}
		ResetMenu();
		break;
	case ID_DEBUGGER_APPLOG_SETTINGS:
		{
			DWORD LogLevel = _Settings->LoadDword(AppLogLevel);
			if ((LogLevel & TraceSettings) != 0)
			{
				LogLevel &= ~TraceSettings;
			} else {

				LogLevel |= TraceSettings;
			}
			_Settings->SaveDword(AppLogLevel, LogLevel );
		}
		ResetMenu();
		break;
	case ID_DEBUGGER_APPLOG_RECOMPILER:
		{
			DWORD LogLevel = _Settings->LoadDword(AppLogLevel);
			if ((LogLevel & TraceRecompiler) != 0)
			{
				LogLevel &= ~TraceRecompiler;
			} else {

				LogLevel |= TraceRecompiler;
			}
			_Settings->SaveDword(AppLogLevel, LogLevel );
		}
		ResetMenu();
		break;
	case ID_DEBUGGER_APPLOG_RSP:
		{
			DWORD LogLevel = _Settings->LoadDword(AppLogLevel);
			if ((LogLevel & TraceRSP) != 0)
			{
				LogLevel &= ~TraceRSP;
			} else {

				LogLevel |= TraceRSP;
			}
			_Settings->SaveDword(AppLogLevel, LogLevel );
		}
		ResetMenu();
		break;
	case ID_DEBUGGER_APPLOG_TLB:
		{
			DWORD LogLevel = _Settings->LoadDword(AppLogLevel);
			if ((LogLevel & TraceTLB) != 0)
			{
				LogLevel &= ~TraceTLB;
			} else {

				LogLevel |= TraceTLB;
			}
			_Settings->SaveDword(AppLogLevel, LogLevel );
		}
		ResetMenu();
		break;
	case ID_DEBUGGER_APPLOG_GFX_PLUGIN:
		{
			DWORD LogLevel = _Settings->LoadDword(AppLogLevel);
			if ((LogLevel & TraceGfxPlugin) != 0)
			{
				LogLevel &= ~TraceGfxPlugin;
			} else {

				LogLevel |= TraceGfxPlugin;
			}
			_Settings->SaveDword(AppLogLevel, LogLevel );
		}
		ResetMenu();
		break;
	case ID_DEBUGGER_APPLOG_DEBUG:
		{
			DWORD LogLevel = _Settings->LoadDword(AppLogLevel);
			if ((LogLevel & TraceDebug) != 0)
			{
				LogLevel &= ~TraceDebug;
			} else {

				LogLevel |= TraceDebug;
			}
			_Settings->SaveDword(AppLogLevel, LogLevel );
		}
		ResetMenu();
		break;
	case ID_DEBUGGER_APPLOG_FLUSH:
		_Settings->SaveDword(AppLogFlush,!_Settings->LoadDword(AppLogFlush));
		ResetMenu();
		break;
	case ID_DEBUGGER_LOGOPTIONS: _Gui->EnterLogOptions(); break;
	case ID_DEBUGGER_GENERATELOG:
		_Settings->SaveDword(GenerateDebugLog,!_Settings->LoadDword(GenerateDebugLog));
		ResetMenu();
		break;
	case ID_DEBUGGER_DUMPMEMORY: 
		_System->Debug_ShowMemoryDump();
		break;
	case ID_DEBUGGER_SEARCHMEMORY: _System->Debug_ShowMemorySearch(); break;
	case ID_DEBUGGER_MEMORY: _System->Debug_ShowMemoryWindow(); break;
	case ID_DEBUGGER_TLBENTRIES: _System->Debug_ShowTLBWindow(); break;
	case ID_DEBUGGER_INTERRUPT_SP: _System->ExternalEvent(Interrupt_SP); break;
	case ID_DEBUGGER_INTERRUPT_SI: _System->ExternalEvent(Interrupt_SI); break;
	case ID_DEBUGGER_INTERRUPT_AI: _System->ExternalEvent(Interrupt_AI); break;
	case ID_DEBUGGER_INTERRUPT_VI: _System->ExternalEvent(Interrupt_VI); break;
	case ID_DEBUGGER_INTERRUPT_PI: _System->ExternalEvent(Interrupt_PI); break;
	case ID_DEBUGGER_INTERRUPT_DP: _System->ExternalEvent(Interrupt_DP); break;
	case ID_CURRENT_SAVE_DEFAULT: 
		Notify().DisplayMessage(3,"Save Slot (%s) selected",GetSaveSlotString(MenuID - ID_CURRENT_SAVE_DEFAULT).c_str());
		_Settings->SaveDword(CurrentSaveState,(DWORD)(MenuID - ID_CURRENT_SAVE_DEFAULT)); 
		ResetMenu();
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
		_Settings->SaveDword(CurrentSaveState,(DWORD)((MenuID - ID_CURRENT_SAVE_1) + 1)); 
		ResetMenu();
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
	case ID_HELP_SUPPORTFORUM: ShellExecute(NULL, "open", "http://www.emutalk.net/forumdisplay.php?f=6", NULL, NULL, SW_SHOWMAXIMIZED); break;
	case ID_HELP_HOMEPAGE: ShellExecute(NULL, "open", "http://www.pj64.net", NULL, NULL, SW_SHOWMAXIMIZED); break;
	case ID_HELP_ABOUT: _Gui->AboutBox(); break;
	case ID_HELP_ABOUTSETTINGFILES: _Gui->AboutIniBox(); break;
	default: 
		if (MenuID >= ID_RECENT_ROM_START && MenuID < ID_RECENT_ROM_END) {
			int Offset = MenuID - ID_RECENT_ROM_START;
			Notify().BreakPoint(__FILE__,__LINE__); 
			/*stdstr File = _Settings->LoadString((SettingID)(FirstRecentRom + Offset));
			if (File.length() > 0) {
				_System->RunFileImage(File.c_str());
			}*/
		}
		if (MenuID >= ID_RECENT_DIR_START && MenuID < ID_RECENT_DIR_END) {
			int Offset = MenuID - ID_RECENT_DIR_START;
			stdstr Dir = _Settings->LoadStringIndex(RecentRomDirIndex,Offset);
			if (Dir.length() > 0) {
				_Settings->SaveDword(UseRomDirSelected,true);
				_Settings->SaveString(SelectedRomDir,Dir.c_str());
				_Settings->SaveString(RomDirectory,Dir.c_str());
				_Gui->GetNotifyClass()->AddRecentDir(Dir.c_str());
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
			ResetMenu();
			_Gui->ResetRomBrowserColomuns();
			break;
		}
		return false;
	}
	return true;
}

stdstr CMainMenu::ShortCutString(MSC_MAP & ShortCuts, int  MenuID, MENU_SHORT_CUT_KEY::ACCESS_MODE AccessLevel) {
	MSC_MAP::iterator MenuItem = ShortCuts.find(MenuID);
	if (MenuItem == ShortCuts.end()) { return ""; }

	const SHORTCUT_KEY_LIST & ShortCutList = MenuItem->second.GetAccelItems();
	for (SHORTCUT_KEY_LIST::const_iterator item = ShortCutList.begin(); item != ShortCutList.end(); item++) 
	{					
		MENU_SHORT_CUT_KEY::ACCESS_MODE ItemMode = item->AccessMode();
		if ((ItemMode & AccessLevel) != AccessLevel )
		{
			continue;
		}
		return item->Name();
	}
	return "";
}

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

	if (!_Settings->LoadBool(CPU_Running)) { return SlotName; }

	stdstr LastSaveTime;

	//check first save name
	stdstr & _GoodName = _Settings->LoadString(ROM_GoodName);
	stdstr & _InstantSaveDirectory = _Settings->LoadString(InstantSaveDirectory);
	stdstr CurrentSaveName;
	if (Slot != 0) { 
		CurrentSaveName.Format("%s.pj%d",_GoodName.c_str(), Slot);
	} else {
		CurrentSaveName.Format("%s.pj",_GoodName.c_str());
	}
	stdstr_f FileName("%s%s",_InstantSaveDirectory.c_str(),CurrentSaveName.c_str());
	
	if (_Settings->LoadDword(AutoZip)) 
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
		stdstr & _RomName = _Settings->LoadString(ROM_NAME);
		if (Slot > 0) { 
			FileName.Format("%s%s.pj%d", _InstantSaveDirectory.c_str(), _RomName.c_str(),Slot);
		} else {
			FileName.Format("%s%s.pj",_InstantSaveDirectory.c_str(),_RomName.c_str());		
		}
		
		if (_Settings->LoadDword(AutoZip)) 
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
	MENU_ITEM Item;

	//Get all flags
	bool inBasicMode = _Settings->LoadBool(BasicMode);
	bool CPURunning  = _Settings->LoadBool(CPU_Running);
	bool RomLoading  = _Settings->LoadBool(LoadingRom);
	bool RomLoaded   = _Settings->LoadString(ROM_NAME).length() > 0;
	bool RomList     = _Settings->LoadBool(RomBrowser) && !CPURunning;
	
	//Get Short Cut Info
	MSC_MAP & ShortCut = GetShortCutInfo(false);

	MENU_SHORT_CUT_KEY::ACCESS_MODE AccessLevel = MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING;
	if (_Settings->LoadBool(CPU_Running))
	{
		AccessLevel = _Settings->LoadBool(InFullScreen)  ? 
			MENU_SHORT_CUT_KEY::GAME_RUNNING_FULLSCREEN : 
			MENU_SHORT_CUT_KEY::GAME_RUNNING_WINDOW;
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
	DWORD count, RomsToRemember = _Settings->LoadDword(RememberedRomFilesCount);

	for (count = 0; count < RomsToRemember; count++) {
		stdstr LastRom = _Settings->LoadStringIndex(RecentRomFileIndex,count);
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
	DWORD DirsToRemember = _Settings->LoadDword(RememberedRomDirCount);
	
	for (count = 0; count < DirsToRemember; count++) 
	{
		stdstr LastDir = _Settings->LoadStringIndex(RecentRomDirIndex,count);
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
	Item.Reset(ID_FILE_OPEN_ROM,      MENU_OPEN,   ShortCutString(ShortCut,ID_FILE_OPEN_ROM,AccessLevel));
	FileMenu.push_back(Item);
	if (!inBasicMode) {
		Item.Reset(ID_FILE_ROM_INFO,      MENU_ROM_INFO,ShortCutString(ShortCut,ID_FILE_ROM_INFO,AccessLevel));
		Item.ItemEnabled = RomLoaded;
		FileMenu.push_back(Item);
		FileMenu.push_back(MENU_ITEM(SPLITER                    ));
		Item.Reset(ID_FILE_STARTEMULATION,MENU_START,   ShortCutString(ShortCut,ID_FILE_STARTEMULATION,AccessLevel)   );
		Item.ItemEnabled = RomLoaded && !CPURunning;
		FileMenu.push_back(Item);
	}
	Item.Reset(ID_FILE_ENDEMULATION,  MENU_END,     ShortCutString(ShortCut,ID_FILE_ENDEMULATION,AccessLevel)   );
	Item.ItemEnabled = CPURunning;
	FileMenu.push_back(Item);
	FileMenu.push_back(MENU_ITEM(SPLITER                    ));
	Item.Reset(SUB_MENU,              MENU_LANGUAGE, EMPTY_STDSTR,  &LangMenu );
	FileMenu.push_back(Item);
	if (RomList) {
		FileMenu.push_back(MENU_ITEM(SPLITER                    ));
		Item.Reset(ID_FILE_ROMDIRECTORY,  MENU_CHOOSE_ROM,ShortCutString(ShortCut,ID_FILE_ROMDIRECTORY,AccessLevel)       );
		FileMenu.push_back(Item);
		Item.Reset(ID_FILE_REFRESHROMLIST,MENU_REFRESH,ShortCutString(ShortCut,ID_FILE_REFRESHROMLIST,AccessLevel)          );
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
			for (MenuItemList::iterator MenuItem = RecentRomMenu.begin(); MenuItem != RecentRomMenu.end(); MenuItem++) {
				MENU_ITEM * RomItem = &(*MenuItem);
				FileMenu.push_back(*MenuItem);
			}
		}
	}
	FileMenu.push_back(MENU_ITEM(SPLITER                                      ));
	FileMenu.push_back(MENU_ITEM(ID_FILE_EXIT,          MENU_EXIT,ShortCutString(ShortCut,ID_FILE_EXIT,AccessLevel)             ));

	/* Current Save
	****************/
	MenuItemList CurrentSaveMenu;
	DWORD _CurrentSaveState = _Settings->LoadDword(CurrentSaveState);
	Item.Reset(ID_CURRENT_SAVE_DEFAULT, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_DEFAULT,AccessLevel),NULL,GetSaveSlotString(0));
	if (_CurrentSaveState == 0) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	CurrentSaveMenu.push_back(MENU_ITEM(SPLITER));
	Item.Reset(ID_CURRENT_SAVE_1, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_1,AccessLevel),NULL,GetSaveSlotString(1));
	if (_CurrentSaveState == 1) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_2, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_2,AccessLevel),NULL,GetSaveSlotString(2));
	if (_CurrentSaveState == 2) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_3, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_3,AccessLevel),NULL,GetSaveSlotString(3));
	if (_CurrentSaveState == 3) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_4, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_4,AccessLevel),NULL,GetSaveSlotString(4));
	if (_CurrentSaveState == 4) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_5, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_5,AccessLevel),NULL,GetSaveSlotString(5));
	if (_CurrentSaveState == 5) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_6, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_6,AccessLevel),NULL,GetSaveSlotString(6));
	if (_CurrentSaveState == 6) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_7, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_7,AccessLevel),NULL,GetSaveSlotString(7));
	if (_CurrentSaveState == 7) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_8, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_8,AccessLevel),NULL,GetSaveSlotString(8));
	if (_CurrentSaveState == 8) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_9, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_9,AccessLevel),NULL,GetSaveSlotString(9));
	if (_CurrentSaveState == 9) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);
	Item.Reset(ID_CURRENT_SAVE_10, EMPTY_STRING,ShortCutString(ShortCut,ID_CURRENT_SAVE_10,AccessLevel),NULL,GetSaveSlotString(10));
	if (_CurrentSaveState == 10) {  Item.ItemTicked = true; }
	CurrentSaveMenu.push_back(Item);

	/* System Menu
	****************/
	MenuItemList SystemMenu;
	MenuItemList ResetMenu;
	if (inBasicMode) 
	{
		SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_RESET_SOFT, MENU_RESET, ShortCutString(ShortCut,ID_SYSTEM_RESET_SOFT,AccessLevel)     ));
	} else {
		ResetMenu.push_back(MENU_ITEM(ID_SYSTEM_RESET_SOFT, MENU_RESET_SOFT, ShortCutString(ShortCut,ID_SYSTEM_RESET_SOFT,AccessLevel)     ));
		ResetMenu.push_back(MENU_ITEM(ID_SYSTEM_RESET_HARD, MENU_RESET_HARD, ShortCutString(ShortCut,ID_SYSTEM_RESET_HARD,AccessLevel)));
		SystemMenu.push_back(MENU_ITEM(SUB_MENU,MENU_RESET,EMPTY_STDSTR,&ResetMenu));
	}
	if (_Settings->LoadBool(CPU_Paused)) {
		SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_PAUSE, MENU_RESUME,    ShortCutString(ShortCut,ID_SYSTEM_PAUSE,AccessLevel)));
	} else {
		SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_PAUSE, MENU_PAUSE,    ShortCutString(ShortCut,ID_SYSTEM_PAUSE,AccessLevel)));
	}
	SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_BITMAP, MENU_BITMAP,   ShortCutString(ShortCut,ID_SYSTEM_BITMAP,AccessLevel)));
	SystemMenu.push_back(MENU_ITEM(SPLITER                            ));
	if (!inBasicMode) {
		Item.Reset(ID_SYSTEM_LIMITFPS, MENU_LIMIT_FPS,ShortCutString(ShortCut,ID_SYSTEM_LIMITFPS,AccessLevel) );
		if (_Settings->LoadDword(LimitFPS)) { Item.ItemTicked = true; }
		SystemMenu.push_back(Item);
		SystemMenu.push_back(MENU_ITEM(SPLITER                            ));
	}
	SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_SAVE, MENU_SAVE,     ShortCutString(ShortCut,ID_SYSTEM_SAVE,AccessLevel)));
	if (!inBasicMode) {
		SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_SAVEAS, MENU_SAVE_AS,  ShortCutString(ShortCut,ID_SYSTEM_SAVEAS,AccessLevel)));
	}
	SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_RESTORE, MENU_RESTORE,  ShortCutString(ShortCut,ID_SYSTEM_RESTORE,AccessLevel)));
	if (!inBasicMode) {
		SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_LOAD, MENU_LOAD,     ShortCutString(ShortCut,ID_SYSTEM_LOAD,AccessLevel)));
	}
	SystemMenu.push_back(MENU_ITEM(SPLITER                            ));
	SystemMenu.push_back(MENU_ITEM(SUB_MENU, MENU_CURRENT_SAVE,  EMPTY_STDSTR, &CurrentSaveMenu ));
	SystemMenu.push_back(MENU_ITEM(SPLITER                            ));
	SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_CHEAT, MENU_CHEAT,    ShortCutString(ShortCut,ID_SYSTEM_CHEAT,AccessLevel)));
	SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_GSBUTTON, MENU_GS_BUTTON,  ShortCutString(ShortCut,ID_SYSTEM_GSBUTTON,AccessLevel)    ));
	
	/* Option Menu
	****************/
	MenuItemList OptionMenu;
	Item.Reset(ID_OPTIONS_FULLSCREEN, MENU_FULL_SCREEN,ShortCutString(ShortCut,ID_OPTIONS_FULLSCREEN,AccessLevel) );
	Item.ItemEnabled = CPURunning;
	if (_System->Plugins()->Gfx() && _System->Plugins()->Gfx()->ChangeWindow == NULL) {
		Item.ItemEnabled = false;
	}
	OptionMenu.push_back(Item);
	if (!inBasicMode) {
		Item.Reset(ID_OPTIONS_ALWAYSONTOP, MENU_ON_TOP,ShortCutString(ShortCut,ID_OPTIONS_ALWAYSONTOP,AccessLevel) );
		if (_Settings->LoadDword(AlwaysOnTop)) { Item.ItemTicked = true; }
		Item.ItemEnabled = CPURunning;
		OptionMenu.push_back(Item);
	}
	OptionMenu.push_back(MENU_ITEM(SPLITER                   ));

	Item.Reset(ID_OPTIONS_CONFIG_GFX, MENU_CONFG_GFX,ShortCutString(ShortCut,ID_OPTIONS_CONFIG_GFX,AccessLevel));
	if (_System->Plugins()->Gfx() == NULL || _System->Plugins()->Gfx()->Config == NULL) { 
		Item.ItemEnabled = false; 
	}
	OptionMenu.push_back(Item);
	Item.Reset(ID_OPTIONS_CONFIG_AUDIO, MENU_CONFG_AUDIO,ShortCutString(ShortCut,ID_OPTIONS_CONFIG_AUDIO,AccessLevel));
	if (_System->Plugins()->Audio() == NULL || _System->Plugins()->Audio()->Config == NULL) { 
		Item.ItemEnabled = false; 
	}
	OptionMenu.push_back(Item);
	if (!inBasicMode) {
		Item.Reset(ID_OPTIONS_CONFIG_RSP, MENU_CONFG_RSP,ShortCutString(ShortCut,ID_OPTIONS_CONFIG_RSP,AccessLevel));
		if (_System->Plugins()->RSP() == NULL || _System->Plugins()->RSP()->Config == NULL) { 
			Item.ItemEnabled = false; 
		}
		OptionMenu.push_back(Item);
	}
	Item.Reset(ID_OPTIONS_CONFIG_CONT, MENU_CONFG_CTRL,ShortCutString(ShortCut,ID_OPTIONS_CONFIG_CONT,AccessLevel));
	if (_System->Plugins()->Control() == NULL || _System->Plugins()->Control()->Config == NULL) { 
		Item.ItemEnabled = false; 
	}
	OptionMenu.push_back(Item);

	OptionMenu.push_back(MENU_ITEM(SPLITER                   ));
	if (!inBasicMode) {
		Item.Reset(ID_OPTIONS_CPU_USAGE, MENU_SHOW_CPU,ShortCutString(ShortCut,ID_OPTIONS_CPU_USAGE,AccessLevel) );
		if (_Settings->LoadDword(ShowCPUPer)) { Item.ItemTicked = true; }
		OptionMenu.push_back(Item);
	}
	OptionMenu.push_back(MENU_ITEM(ID_OPTIONS_SETTINGS, MENU_SETTINGS,ShortCutString(ShortCut,ID_OPTIONS_SETTINGS,AccessLevel) ));

	/* Profile Menu
	****************/
	MenuItemList DebugProfileMenu;
	if (_Settings->LoadDword(Debugger)) 
	{
		Item.Reset(ID_PROFILE_PROFILE,EMPTY_STRING,EMPTY_STDSTR,NULL,"Profile Code" );
		if (_Settings->LoadDword(ProfileCode)) { Item.ItemTicked = true; }
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
	if (_Settings->LoadDword(Debugger)) {		
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
		if (_Settings->LoadDword(DisableGameFixes)) { 
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
			DWORD LogLevel = _Settings->LoadDword(AppLogLevel);
			
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

			Item.Reset(ID_DEBUGGER_APPLOG_DEBUG,EMPTY_STRING,EMPTY_STDSTR,NULL,"Debug Messages" );
			if ((LogLevel & TraceDebug) != 0) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);

			DebugAppLoggingMenu.push_back(MENU_ITEM(SPLITER                   ));

			Item.Reset(ID_DEBUGGER_APPLOG_FLUSH,EMPTY_STRING,EMPTY_STDSTR,NULL,"Auto flush file" );
			if (_Settings->LoadDword(AppLogFlush) != 0) { Item.ItemTicked = true; }
			DebugAppLoggingMenu.push_back(Item);
		}


		/* Debug - Logging
		*******************/
		Item.Reset(ID_DEBUGGER_LOGOPTIONS,EMPTY_STRING,EMPTY_STDSTR,NULL,"Log Options..." );
		DebugLoggingMenu.push_back(Item);
		
		
		Item.Reset(ID_DEBUGGER_GENERATELOG,EMPTY_STRING,EMPTY_STDSTR,NULL,"Generate Log" );
		if (_Settings->LoadDword(GenerateDebugLog)) { Item.ItemTicked = true; }
		DebugLoggingMenu.push_back(Item);

		/* Debugger Main Menu
		****************/
		Item.Reset(ID_DEBUGGER_BREAKPOINTS, EMPTY_STRING,EMPTY_STDSTR, NULL,"Breakpoint...");
		Item.ItemEnabled = CPURunning;
		DebugMenu.push_back(Item);
		DebugMenu.push_back(MENU_ITEM(SPLITER));
		
		/* Debug - RSP
		*******************/
		if (_System->Plugins()->RSP() != NULL && IsMenu((HMENU)_System->Plugins()->RSP()->GetDebugMenu())) 
		{ 
			Item.Reset(ID_PLUGIN_MENU,EMPTY_STRING,NULL,_System->Plugins()->RSP()->GetDebugMenu(),"&RSP" );
			DebugMenu.push_back(Item);
		}

		/* Debug - RDP
		*******************/
		if (_System->Plugins()->Gfx() != NULL && IsMenu((HMENU)_System->Plugins()->Gfx()->GetDebugMenu())) 
		{ 
			Item.Reset(ID_PLUGIN_MENU,EMPTY_STRING,NULL,_System->Plugins()->Gfx()->GetDebugMenu(),"&RDP" );
			DebugMenu.push_back(Item);
		}

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
		DebugMenu.push_back(MENU_ITEM(SPLITER));
		Item.Reset(ID_DEBUG_SHOW_UNHANDLED_MEM,EMPTY_STRING,EMPTY_STDSTR,NULL,"Show Unhandled Memory Actions" );
		if (_Settings->LoadDword(ShowUnhandledMemory)) { 
			Item.ItemTicked = true;
		}
		DebugMenu.push_back(Item);
		Item.Reset(ID_DEBUG_SHOW_PIF_ERRORS,EMPTY_STRING,EMPTY_STDSTR,NULL,"Show PIF Errors" );
		if (_Settings->LoadDword(ShowPifErrors)) { 
			Item.ItemTicked = true;
		}
		DebugMenu.push_back(Item);
		Item.Reset(ID_DEBUG_SHOW_DLIST_COUNT,EMPTY_STRING,EMPTY_STDSTR,NULL,"Show Alist/Dlist Counters" );
		if (_Settings->LoadDword(ShowDListAListCount)) { 
			Item.ItemTicked = true;
		}
		DebugMenu.push_back(Item);
		Item.Reset(ID_DEBUG_SHOW_RECOMP_MEM_SIZE,EMPTY_STRING,EMPTY_STDSTR,NULL,"Show Recompile Memory Buffer size" );
		if (_Settings->LoadDword(ShowRecompMemSize)) { 
			Item.ItemTicked = true;
		}
		DebugMenu.push_back(Item);
		Item.Reset(ID_DEBUG_SHOW_CHECK_OPUSAGE,EMPTY_STRING,EMPTY_STDSTR,NULL,"Show Check Opcode Usage Errors" );
		if (_Settings->LoadDword(ShowCheckOpUsageErrors)) { 
			Item.ItemTicked = true;
		}
		DebugMenu.push_back(Item);
		DebugMenu.push_back(MENU_ITEM(SPLITER));
		Item.Reset(ID_DEBUG_GENERATE_LOG_FILES,EMPTY_STRING,EMPTY_STDSTR,NULL,"Generate Log Files" );
		if (_Settings->LoadDword(GenerateLogFiles)) { 
			Item.ItemTicked = true;
		}
		DebugMenu.push_back(Item);
	}

	/* Help Menu
	****************/
	MenuItemList HelpMenu;
	if (_Settings->LoadBool(IsBetaVersion))
	{
		stdstr_f User("Beta For: %s",_Settings->LoadString(BetaUserName).c_str());
		stdstr_f Email("Email: %s",_Settings->LoadString(BetaEmailAddress).c_str());
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
		if (_Settings->LoadDword(Debugger)) {
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
	//Delete the old accel list
	WriteTrace(TraceDebug,"CMainMenu::ResetMenu 6");
	if (m_AccelTable) { 
		DestroyAcceleratorTable((HACCEL)m_AccelTable);
		m_AccelTable = NULL; 
	}
	WriteTrace(TraceDebug,"CMainMenu::ResetMenu 7");
	//Generate a ACCEL list
	MENU_SHORT_CUT_KEY::ACCESS_MODE AccessLevel = MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING;
	if (_Settings->LoadBool(CPU_Running))
	{
		AccessLevel = _Settings->LoadDword(InFullScreen) != 0 ? 
			MENU_SHORT_CUT_KEY::GAME_RUNNING_FULLSCREEN : 
			MENU_SHORT_CUT_KEY::GAME_RUNNING_WINDOW;
	}
	MSC_MAP & ShortCuts = GetShortCutInfo(false);

	int size = 0, MaxSize = ShortCuts.size() * 5;
	ACCEL * AccelList = new ACCEL[MaxSize];
	
	WriteTrace(TraceDebug,"CMainMenu::ResetMenu 8");
	for (MSC_MAP::iterator Item = ShortCuts.begin(); Item != ShortCuts.end(); Item++)
	{
		MENU_SHORT_CUT & short_cut = Item->second;

		MENU_SHORT_CUT_KEY::ACCESS_MODE ItemMode = short_cut.AccessMode();
		if ((ItemMode & AccessLevel) != AccessLevel )
		{
			continue;
		}

		SHORTCUT_KEY_LIST ShortCutAccelList = short_cut.GetAccelItems();
		for (SHORTCUT_KEY_LIST::iterator AccelIter = ShortCutAccelList.begin(); AccelIter != ShortCutAccelList.end(); AccelIter++) 
		{
			if (size >= MaxSize) { break; }
			AccelList[size].cmd = Item->first;
			AccelList[size].key = AccelIter->Key();
			AccelList[size].fVirt = FVIRTKEY;
			if (AccelIter->Alt())   { AccelList[size].fVirt |= FALT;     }
			if (AccelIter->Ctrl())  { AccelList[size].fVirt |= FCONTROL; }
			if (AccelIter->Shift()) { AccelList[size].fVirt |= FSHIFT;   }
			size += 1;
		}
	}

	WriteTrace(TraceDebug,"CMainMenu::ResetMenu 9");
	m_AccelTable = CreateAcceleratorTable(AccelList,size);
	WriteTrace(TraceDebug,"CMainMenu::ResetMenu 10");
	
	delete [] AccelList;
}

void CMainMenu::ResetMenu(void) {
	WriteTrace(TraceDebug,"CMainMenu::ResetMenu starting");
	
	if (!_Settings->LoadBool(InFullScreen))
	{
		//Create a new window with all the items
		WriteTrace(TraceDebug,"CMainMenu::ResetMenu 1");
		MENU_HANDLE hMenu = (MENU_HANDLE)CreateMenu();
		FillOutMenu(hMenu);
		WriteTrace(TraceDebug,"CMainMenu::ResetMenu 2");

		//save old menu to destroy latter
		MENU_HANDLE OldMenuHandle = m_MenuHandle;

		//save handle and re-attach to a window
		WriteTrace(TraceDebug,"CMainMenu::ResetMenu 3");
		m_MenuHandle = hMenu;
		_Gui->SetWindowMenu(this);

		//Destroy the old menu
		WriteTrace(TraceDebug,"CMainMenu::ResetMenu 4");
		if (_System->Plugins()->Gfx() != NULL && IsMenu((HMENU)_System->Plugins()->Gfx()->GetDebugMenu()))
		{
			RemoveMenu((HMENU)OldMenuHandle,(DWORD)_System->Plugins()->Gfx()->GetDebugMenu(), MF_BYCOMMAND); 
		}
		if (_System->Plugins()->RSP() != NULL && IsMenu((HMENU)_System->Plugins()->RSP()->GetDebugMenu()))
		{
			RemoveMenu((HMENU)OldMenuHandle,(DWORD)_System->Plugins()->RSP()->GetDebugMenu(), MF_BYCOMMAND); 
		}
		WriteTrace(TraceDebug,"CMainMenu::ResetMenu 5");
		DestroyMenu((HMENU)OldMenuHandle); 
	}

	ResetAccelerators();

	WriteTrace(TraceDebug,"CMainMenu::ResetMenu Done");
}

MSC_MAP CMainMenu::GetShortCutInfo(bool InitialSettings ) {
#define DEF_SCUT(ID,Section, LangID,AccessMode) ShortCuts.insert(MSC_MAP::value_type(ID,MENU_SHORT_CUT(Section,LangID,AccessMode)))
	MSC_MAP ShortCuts;
	
	ShortCuts.clear();
	DEF_SCUT(ID_FILE_OPEN_ROM,       STR_SHORTCUT_FILEMENU, MENU_OPEN,        MENU_SHORT_CUT_KEY::NOT_IN_FULLSCREEN );
	DEF_SCUT(ID_FILE_ROM_INFO,       STR_SHORTCUT_FILEMENU, MENU_ROM_INFO,    MENU_SHORT_CUT_KEY::NOT_IN_FULLSCREEN );
	DEF_SCUT(ID_FILE_STARTEMULATION, STR_SHORTCUT_FILEMENU, MENU_START,       MENU_SHORT_CUT_KEY::NOT_IN_FULLSCREEN );
	DEF_SCUT(ID_FILE_ENDEMULATION,   STR_SHORTCUT_FILEMENU, MENU_END,         MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_FILE_ROMDIRECTORY,   STR_SHORTCUT_FILEMENU, MENU_CHOOSE_ROM,  MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING );
	DEF_SCUT(ID_FILE_REFRESHROMLIST, STR_SHORTCUT_FILEMENU, MENU_REFRESH,     MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING );
	DEF_SCUT(ID_FILE_EXIT,           STR_SHORTCUT_FILEMENU, MENU_EXIT,        MENU_SHORT_CUT_KEY::ANYTIME );

	DEF_SCUT(ID_SYSTEM_RESET_SOFT,   STR_SHORTCUT_SYSTEMMENU, MENU_RESET_SOFT,  MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_SYSTEM_RESET_HARD,   STR_SHORTCUT_SYSTEMMENU, MENU_RESET_HARD,  MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_SYSTEM_PAUSE,        STR_SHORTCUT_SYSTEMMENU, MENU_PAUSE,       MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_SYSTEM_BITMAP,       STR_SHORTCUT_SYSTEMMENU, MENU_BITMAP,      MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_SYSTEM_LIMITFPS,     STR_SHORTCUT_SYSTEMMENU, MENU_LIMIT_FPS,   MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_SYSTEM_SAVE,         STR_SHORTCUT_SYSTEMMENU, MENU_SAVE,        MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_SYSTEM_SAVEAS,       STR_SHORTCUT_SYSTEMMENU, MENU_SAVE_AS,     MENU_SHORT_CUT_KEY::GAME_RUNNING_WINDOW );
	DEF_SCUT(ID_SYSTEM_RESTORE,      STR_SHORTCUT_SYSTEMMENU, MENU_RESTORE,     MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_SYSTEM_LOAD,         STR_SHORTCUT_SYSTEMMENU, MENU_LOAD,        MENU_SHORT_CUT_KEY::GAME_RUNNING_WINDOW );
	DEF_SCUT(ID_SYSTEM_CHEAT,        STR_SHORTCUT_SYSTEMMENU, MENU_CHEAT,       MENU_SHORT_CUT_KEY::NOT_IN_FULLSCREEN );
	DEF_SCUT(ID_SYSTEM_GSBUTTON,     STR_SHORTCUT_SYSTEMMENU, MENU_GS_BUTTON,   MENU_SHORT_CUT_KEY::GAME_RUNNING );
	
	DEF_SCUT(ID_OPTIONS_DISPLAY_FR,    STR_SHORTCUT_OPTIONS, OPTION_DISPLAY_FR,MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_OPTIONS_CHANGE_FR,     STR_SHORTCUT_OPTIONS, OPTION_CHANGE_FR, MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_OPTIONS_INCREASE_SPEED,STR_SHORTCUT_OPTIONS, STR_INSREASE_SPEED, MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_OPTIONS_DECREASE_SPEED,STR_SHORTCUT_OPTIONS, STR_DECREASE_SPEED, MENU_SHORT_CUT_KEY::GAME_RUNNING );

	DEF_SCUT(ID_CURRENT_SAVE_DEFAULT,STR_SHORTCUT_SAVESLOT, SAVE_SLOT_DEFAULT,MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_CURRENT_SAVE_1,      STR_SHORTCUT_SAVESLOT, SAVE_SLOT_1,      MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_CURRENT_SAVE_2,      STR_SHORTCUT_SAVESLOT, SAVE_SLOT_2,      MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_CURRENT_SAVE_3,      STR_SHORTCUT_SAVESLOT, SAVE_SLOT_3,      MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_CURRENT_SAVE_4,      STR_SHORTCUT_SAVESLOT, SAVE_SLOT_4,      MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_CURRENT_SAVE_5,      STR_SHORTCUT_SAVESLOT, SAVE_SLOT_5,      MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_CURRENT_SAVE_6,      STR_SHORTCUT_SAVESLOT, SAVE_SLOT_6,      MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_CURRENT_SAVE_7,      STR_SHORTCUT_SAVESLOT, SAVE_SLOT_7,      MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_CURRENT_SAVE_8,      STR_SHORTCUT_SAVESLOT, SAVE_SLOT_8,      MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_CURRENT_SAVE_9,      STR_SHORTCUT_SAVESLOT, SAVE_SLOT_9,      MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_CURRENT_SAVE_10,     STR_SHORTCUT_SAVESLOT, SAVE_SLOT_10,     MENU_SHORT_CUT_KEY::GAME_RUNNING );

	//Option Menu
	DEF_SCUT(ID_OPTIONS_FULLSCREEN,  STR_SHORTCUT_OPTIONS, MENU_FULL_SCREEN, MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_OPTIONS_ALWAYSONTOP, STR_SHORTCUT_OPTIONS, MENU_ON_TOP,      MENU_SHORT_CUT_KEY::NOT_IN_FULLSCREEN );
	DEF_SCUT(ID_OPTIONS_CONFIG_GFX,  STR_SHORTCUT_OPTIONS, MENU_CONFG_GFX,   MENU_SHORT_CUT_KEY::NOT_IN_FULLSCREEN );
	DEF_SCUT(ID_OPTIONS_CONFIG_AUDIO,STR_SHORTCUT_OPTIONS, MENU_CONFG_AUDIO, MENU_SHORT_CUT_KEY::NOT_IN_FULLSCREEN );
	DEF_SCUT(ID_OPTIONS_CONFIG_CONT, STR_SHORTCUT_OPTIONS, MENU_CONFG_CTRL,  MENU_SHORT_CUT_KEY::NOT_IN_FULLSCREEN );
	DEF_SCUT(ID_OPTIONS_CONFIG_RSP,  STR_SHORTCUT_OPTIONS, MENU_CONFG_RSP,   MENU_SHORT_CUT_KEY::NOT_IN_FULLSCREEN );
	DEF_SCUT(ID_OPTIONS_CPU_USAGE,   STR_SHORTCUT_OPTIONS, MENU_SHOW_CPU,    MENU_SHORT_CUT_KEY::GAME_RUNNING );
	DEF_SCUT(ID_OPTIONS_SETTINGS,    STR_SHORTCUT_OPTIONS, MENU_SETTINGS,    MENU_SHORT_CUT_KEY::NOT_IN_FULLSCREEN );

	stdstr FileName = _Settings->LoadString(ShortCutFile);
	FILE *file = fopen(FileName.c_str(),"r");
	if (file == NULL || InitialSettings) {
		ShortCuts.find(ID_FILE_OPEN_ROM)->second.AddShortCut('O',TRUE,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_FILE_OPEN_ROM)->second.AddShortCut('O',TRUE,false,false,MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING);
		ShortCuts.find(ID_FILE_STARTEMULATION)->second.AddShortCut(VK_F11,false,false,false,MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING);
		ShortCuts.find(ID_FILE_ENDEMULATION)->second.AddShortCut(VK_F12,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_FILE_REFRESHROMLIST)->second.AddShortCut(VK_F5,false,false,false,MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING);
		ShortCuts.find(ID_FILE_EXIT)->second.AddShortCut(VK_F4,false,true,false,MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING);
		ShortCuts.find(ID_FILE_EXIT)->second.AddShortCut(VK_F4,false,true,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_DEFAULT)->second.AddShortCut(0xC0,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_1)->second.AddShortCut('1',false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_2)->second.AddShortCut('2',false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_3)->second.AddShortCut('3',false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_4)->second.AddShortCut('4',false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_5)->second.AddShortCut('5',false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_6)->second.AddShortCut('6',false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_7)->second.AddShortCut('7',false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_8)->second.AddShortCut('8',false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_9)->second.AddShortCut('9',false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_CURRENT_SAVE_10)->second.AddShortCut('0',false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_OPTIONS_FULLSCREEN)->second.AddShortCut(VK_RETURN,false,true,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_OPTIONS_FULLSCREEN)->second.AddShortCut(VK_ESCAPE,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_OPTIONS_ALWAYSONTOP)->second.AddShortCut('A',true,false,false,MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING);
		ShortCuts.find(ID_OPTIONS_ALWAYSONTOP)->second.AddShortCut('A',true,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING_WINDOW);
		ShortCuts.find(ID_OPTIONS_SETTINGS)->second.AddShortCut('T',true,false,false,MENU_SHORT_CUT_KEY::GAME_NOT_RUNNING);
		ShortCuts.find(ID_OPTIONS_SETTINGS)->second.AddShortCut('T',true,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING_WINDOW);
		ShortCuts.find(ID_SYSTEM_RESET_SOFT)->second.AddShortCut(VK_F1,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_SYSTEM_RESET_HARD)->second.AddShortCut(VK_F1,false,false,true,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_SYSTEM_PAUSE)->second.AddShortCut(VK_F2,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_SYSTEM_PAUSE)->second.AddShortCut(VK_PAUSE,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_SYSTEM_BITMAP)->second.AddShortCut(VK_F3,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_SYSTEM_LIMITFPS)->second.AddShortCut(VK_F4,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_SYSTEM_SAVE)->second.AddShortCut(VK_F5,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_SYSTEM_RESTORE)->second.AddShortCut(VK_F7,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_SYSTEM_LOAD)->second.AddShortCut('L',true,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING_WINDOW);
		ShortCuts.find(ID_SYSTEM_SAVEAS)->second.AddShortCut('S',true,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING_WINDOW);
		ShortCuts.find(ID_SYSTEM_CHEAT)->second.AddShortCut('C',true,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING_WINDOW);
		ShortCuts.find(ID_SYSTEM_GSBUTTON)->second.AddShortCut(VK_F9,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_OPTIONS_INCREASE_SPEED)->second.AddShortCut(VK_OEM_PLUS,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);
		ShortCuts.find(ID_OPTIONS_DECREASE_SPEED)->second.AddShortCut(VK_OEM_MINUS,false,false,false,MENU_SHORT_CUT_KEY::GAME_RUNNING);

	} else if (file) {
		MENU_SHORT_CUT_KEY::ACCESS_MODE AccessMode;
		int ID, key, bCtrl, bAlt, bShift;

		do {
			char Line[300];
			if (fgets(Line,sizeof(Line),file) != NULL) {
				sscanf(Line,"%d,%d,%d,%d,%d,%d", &ID,&key,&bCtrl,&bAlt,&bShift,&AccessMode);

				MENU_SHORT_CUT_MAP::iterator item = ShortCuts.find(ID);
				if (item == ShortCuts.end()) { continue; }
				item->second.AddShortCut(key,bCtrl == 1,bAlt == 1,bShift == 1,AccessMode);
			}
		} while (feof(file) == 0);

	}
	return ShortCuts;
}

LanguageStringID CMainMenu::GetShortCutMenuItemName(MSC_MAP * ShortCuts, WORD key, bool bCtrl, bool bAlt, bool bShift, MENU_SHORT_CUT_KEY::ACCESS_MODE Access) {
	for (MSC_MAP::iterator Item = ShortCuts->begin(); Item != ShortCuts->end(); Item++) {
		MENU_SHORT_CUT & short_cut = Item->second;
		
		for (SHORTCUT_KEY_LIST::const_iterator AccelItem = short_cut.GetAccelItems().begin(); AccelItem != short_cut.GetAccelItems().end(); AccelItem++) {
			if (!AccelItem->Same(key,bCtrl,bAlt,bShift,Access)) { continue; }
			return short_cut.Title();
		}	
	}
	return EMPTY_STRING;
}

void CMainMenu::SaveShortCuts(MSC_MAP * ShortCuts) {
	stdstr FileName = _Settings->LoadString(ShortCutFile);
	FILE *file = fopen(FileName.c_str(),"w");
	for (MSC_MAP::iterator Item = ShortCuts->begin(); Item != ShortCuts->end(); Item++) {
		for (SHORTCUT_KEY_LIST::const_iterator ShortCut = Item->second.GetAccelItems().begin(); ShortCut != Item->second.GetAccelItems().end(); ShortCut++) {
			fprintf(file,"%d,%d,%d,%d,%d,%d\n",Item->first,ShortCut->Key(),ShortCut->Ctrl(),
				ShortCut->Alt(),ShortCut->Shift(),ShortCut->AccessMode());
		}
	}
	fclose(file);
}
