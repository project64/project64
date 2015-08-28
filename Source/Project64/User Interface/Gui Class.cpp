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

#include <commctrl.h>
#include "Settings/SettingType/SettingsType-Application.h"

extern "C" 
{
void EnterLogOptions(HWND hwndOwner);
}

#pragma comment(lib, "Comctl32.lib") 

DWORD CALLBACK AboutBoxProc (HWND WndHandle, DWORD uMsg, DWORD wParam, DWORD lParam);
LRESULT CALLBACK MainGui_Proc (HWND WndHandle, DWORD uMsg, DWORD wParam, DWORD lParam);

extern BOOL set_about_field(
    HWND hDlg,
    int nIDDlgItem,
    const wchar_t * config_string,
    const wchar_t * language_string
);

CMainGui::CMainGui (bool bMainWindow, const char * WindowTitle ) :
	CRomBrowser(m_hMainWindow,m_hStatusWnd),
	m_ThreadId(GetCurrentThreadId()),
	m_bMainWindow(bMainWindow),
	m_Created(false),
	m_AttachingMenu(false),
	m_MakingVisible(false),
	m_ResetPlugins(false),
	m_ResetInfo(NULL)
{
	m_Menu = NULL;
	
	m_hMainWindow = 0;
	m_hStatusWnd  = 0;
	m_SaveMainWindowPos  = false;
	m_SaveMainWindowTop  = 0;
	m_SaveMainWindowLeft = 0;
	
	m_SaveRomBrowserPos  = false;
	m_SaveRomBrowserTop  = 0;
	m_SaveRomBrowserLeft = 0;

	if (m_bMainWindow)
	{
		g_Settings->RegisterChangeCB(RomBrowser_Enabled,this,(CSettings::SettingChangedFunc)RomBowserEnabledChanged);
		g_Settings->RegisterChangeCB(RomBrowser_ColoumnsChanged,this,(CSettings::SettingChangedFunc)RomBowserColoumnsChanged);
		g_Settings->RegisterChangeCB(RomBrowser_Recursive,this,(CSettings::SettingChangedFunc)RomBrowserRecursiveChanged);
	}

	//if this fails then it has already been created
	RegisterWinClass();
	Create(WindowTitle);

}

CMainGui::~CMainGui (void) 
{
	WriteTrace(TraceDebug,__FUNCTION__ ": Start");
	if (m_bMainWindow)
	{
		g_Settings->UnregisterChangeCB(RomBrowser_Enabled,this,(CSettings::SettingChangedFunc)RomBowserEnabledChanged);
		g_Settings->UnregisterChangeCB(RomBrowser_ColoumnsChanged,this,(CSettings::SettingChangedFunc)RomBowserColoumnsChanged);
		g_Settings->UnregisterChangeCB(RomBrowser_Recursive,this,(CSettings::SettingChangedFunc)RomBrowserRecursiveChanged);
	}
	if (m_hMainWindow)
	{
		DestroyWindow(m_hMainWindow);
	}
	WriteTrace(TraceDebug,__FUNCTION__ ": Done");
}

bool CMainGui::RegisterWinClass ( void ) 
{
	stdstr_f VersionDisplay("Project64 %s", VER_FILE_VERSION_STR);

	WNDCLASS wcl;

	wcl.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wcl.cbClsExtra		= 0;
	wcl.cbWndExtra		= 0;
	wcl.hIcon			= LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_PJ64_Icon));
	wcl.hCursor			= LoadCursor(NULL,IDC_ARROW);	
	wcl.hInstance		= GetModuleHandle(NULL);

	wcl.lpfnWndProc		= (WNDPROC)MainGui_Proc;
	wcl.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcl.lpszMenuName	= NULL;
	wcl.lpszClassName	= VersionDisplay.c_str();
	if (RegisterClass(&wcl)  == 0) return false;
	return true;
}


void RomBowserEnabledChanged (CMainGui * Gui)
{
	if (Gui && g_Settings->LoadBool(RomBrowser_Enabled)) 
	{
		if (!Gui->RomBrowserVisible()) 
		{
			Gui->ShowRomList();
		}
	} else {
		if (Gui->RomBrowserVisible())
		{
			Gui->HideRomList();
		}
	}
}

void RomBowserColoumnsChanged (CMainGui * Gui)
{
	Gui->ResetRomBrowserColomuns();   
}

void RomBrowserRecursiveChanged (CMainGui * Gui)
{
	Gui->RefreshRomBrowser(); 
	Gui->HighLightLastRom();
}

void CMainGui::ChangeWinSize (long width, long height) 
{
	CGuard Guard(m_CS);
	WINDOWPLACEMENT wndpl;
    RECT rc1, swrect;

	wndpl.length = sizeof(wndpl);
	GetWindowPlacement( m_hMainWindow, &wndpl);

	if ( (HWND)m_hStatusWnd != NULL ) 
    {
		GetClientRect( (HWND)m_hStatusWnd, &swrect );
	    SetRect( &rc1, 0, 0, width, height + swrect.bottom );
	} 
    else 
    {
	    SetRect( &rc1, 0, 0, width, height );
	}


    AdjustWindowRectEx( &rc1,GetWindowLong( m_hMainWindow, GWL_STYLE ),
		GetMenu( m_hMainWindow ) != NULL, GetWindowLong( m_hMainWindow, GWL_EXSTYLE ) ); 

    MoveWindow( m_hMainWindow, wndpl.rcNormalPosition.left, wndpl.rcNormalPosition.top, 
		rc1.right - rc1.left, rc1.bottom - rc1.top, TRUE );
}

void CMainGui::AboutBox (void) 
{
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_About), m_hMainWindow, (DLGPROC)AboutBoxProc,(LPARAM)this);
}

void CMainGui::AboutIniBox (void) 
{
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_About_Ini), m_hMainWindow, (DLGPROC)AboutIniBoxProc,(LPARAM)this);
}

DWORD CALLBACK AboutIniBoxProc (HWND hDlg, DWORD uMsg, DWORD wParam, DWORD /*lParam*/) 
{
	static wchar_t RDBHomePage[300], CHTHomePage[300], RDXHomePage[300];
	
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			wchar_t String[200];
			
			//Title
			LONG_PTR originalWndProc = GetWindowLongPtrW(hDlg, GWLP_WNDPROC);
			SetWindowLongPtrW(hDlg, GWLP_WNDPROC, (LONG_PTR) DefWindowProcW);
			SetWindowTextW(hDlg, GS(INI_TITLE));
			SetWindowLongPtrW(hDlg, GWLP_WNDPROC, originalWndProc);

			//Language
			SetDlgItemTextW(hDlg,IDC_LAN,GS(INI_CURRENT_LANG));
			set_about_field(hDlg, IDC_LAN_AUTHOR, GS(INI_AUTHOR), GS(LANGUAGE_AUTHOR));
			set_about_field(hDlg, IDC_LAN_VERSION, GS(INI_VERSION), GS(LANGUAGE_VERSION));
			set_about_field(hDlg, IDC_LAN_DATE, GS(INI_DATE), GS(LANGUAGE_DATE));
			if (wcslen(GS(LANGUAGE_NAME)) == 0) 
            {
				EnableWindow(GetDlgItem(hDlg,IDC_LAN),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_LAN_AUTHOR),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_LAN_VERSION),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_LAN_DATE),FALSE);
			}
			
			//RDB
			CIniFile RdbIniFile(g_Settings->LoadString(SupportFile_RomDatabase).c_str());
			wcsncpy(String, RdbIniFile.GetString("Meta","Author","").ToUTF16().c_str(),sizeof(String) / sizeof(String[0]));
			if (wcslen(String) == 0) 
            {
				EnableWindow(GetDlgItem(hDlg,IDC_RDB),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDB_AUTHOR),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDB_VERSION),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDB_DATE),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDB_HOME),FALSE);
			}

			set_about_field(hDlg, IDC_RDB_AUTHOR, GS(INI_AUTHOR), String);
			
			wcsncpy(String, RdbIniFile.GetString("Meta","Version","").ToUTF16().c_str(),sizeof(String) / sizeof(String[0]));
			set_about_field(hDlg, IDC_RDB_VERSION, GS(INI_VERSION), String);
			wcsncpy(String, RdbIniFile.GetString("Meta","Date","").ToUTF16().c_str(),sizeof(String) / sizeof(String[0]));
			set_about_field(hDlg, IDC_RDB_DATE, GS(INI_DATE), String);
			wcsncpy(RDBHomePage, RdbIniFile.GetString("Meta","Homepage","").ToUTF16().c_str(),sizeof(RDBHomePage) / sizeof(RDBHomePage[0]));
			SetDlgItemTextW(hDlg, IDC_RDB_HOME, GS(INI_HOMEPAGE));
			if (wcslen(RDBHomePage) == 0) 
			{
				EnableWindow(GetDlgItem(hDlg,IDC_RDB_HOME),FALSE);
			}
			
			//Cheat
			SetDlgItemTextW(hDlg,IDC_CHT,GS(INI_CURRENT_CHT));
			CIniFile CheatIniFile(g_Settings->LoadString(SupportFile_Cheats).c_str());
			wcsncpy(String, CheatIniFile.GetString("Meta","Author","").ToUTF16().c_str(),sizeof(String) / sizeof(String[0]));
			if (wcslen(String) == 0) 
			{
				EnableWindow(GetDlgItem(hDlg,IDC_CHT),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_CHT_AUTHOR),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_CHT_VERSION),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_CHT_DATE),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_CHT_HOME),FALSE);
			}
			set_about_field(hDlg, IDC_CHT_AUTHOR, GS(INI_AUTHOR), String);
			wcsncpy(String, CheatIniFile.GetString("Meta","Version","").ToUTF16().c_str(),sizeof(String) / sizeof(String[0]));
			set_about_field(hDlg, IDC_CHT_VERSION, GS(INI_VERSION), String);
			wcsncpy(String, CheatIniFile.GetString("Meta","Date","").ToUTF16().c_str(),sizeof(String) / sizeof(String[0]));
			set_about_field(hDlg, IDC_CHT_DATE, GS(INI_DATE), String);
			wcsncpy(CHTHomePage, CheatIniFile.GetString("Meta","Homepage","").ToUTF16().c_str(),sizeof(CHTHomePage) / sizeof(CHTHomePage[0]));
			SetDlgItemTextW(hDlg, IDC_CHT_HOME, GS(INI_HOMEPAGE));
			if (wcslen(CHTHomePage) == 0)
            {
				EnableWindow(GetDlgItem(hDlg,IDC_CHT_HOME),FALSE);
			}
			
			//Extended Info
			SetDlgItemTextW(hDlg, IDC_RDX, GS(INI_CURRENT_RDX));
			CIniFile RdxIniFile(g_Settings->LoadString(SupportFile_ExtInfo).c_str());
			wcsncpy(String, RdxIniFile.GetString("Meta","Author","").ToUTF16().c_str(),sizeof(String) / sizeof(String[0]));
			if (wcslen(String) == 0) 
			{
				EnableWindow(GetDlgItem(hDlg,IDC_RDX),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDX_AUTHOR),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDX_VERSION),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDX_DATE),FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_RDX_HOME),FALSE);
			}
			set_about_field(hDlg, IDC_RDX_AUTHOR, GS(INI_AUTHOR), String);
			wcsncpy(String, RdxIniFile.GetString("Meta","Version","").ToUTF16().c_str(),sizeof(String) / sizeof(String[0]));
			set_about_field(hDlg, IDC_RDX_VERSION, GS(INI_VERSION), String);
			wcsncpy(String, RdxIniFile.GetString("Meta","Date","").ToUTF16().c_str(),sizeof(String) / sizeof(String[0]));
			set_about_field(hDlg, IDC_RDX_DATE, GS(INI_DATE), String);
			wcsncpy(RDXHomePage, RdxIniFile.GetString("Meta","Homepage","").ToUTF16().c_str(),sizeof(RDXHomePage) / sizeof(RDXHomePage[0]));
			SetDlgItemTextW(hDlg, IDC_RDX_HOME, GS(INI_HOMEPAGE));
			if (wcslen(RDXHomePage) == 0) 
            {
				EnableWindow(GetDlgItem(hDlg,IDC_RDX_HOME),FALSE);
			}
			SetDlgItemTextW(hDlg, IDOK, GS(CHEAT_OK));
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
        {
		case IDC_RDB_HOME: ShellExecuteW(NULL,L"open",RDBHomePage,NULL,NULL,SW_SHOWNORMAL); break;
		case IDC_CHT_HOME: ShellExecuteW(NULL,L"open",CHTHomePage,NULL,NULL,SW_SHOWNORMAL); break;
		case IDC_RDX_HOME: ShellExecuteW(NULL,L"open",RDXHomePage,NULL,NULL,SW_SHOWNORMAL); break;
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg,0);
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

bool CMainGui::ResetPlugins (CPlugins * plugins, CN64System * System)
{
	RESET_PLUGIN info;
	info.system = System;
	info.plugins = plugins;
	info.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	bool bRes = true;
	if (info.hEvent)
	{
		PostMessage(m_hMainWindow,WM_RESET_PLUGIN,(WPARAM)&bRes,(LPARAM)&info);
#ifdef _DEBUG
		DWORD dwRes = WaitForSingleObject(info.hEvent,INFINITE);
#else
		DWORD dwRes = WaitForSingleObject(info.hEvent,5000);
#endif
		dwRes = dwRes;
		CloseHandle(info.hEvent);
	}
    else 
    {
		WriteTrace(TraceError,__FUNCTION__ ": Failed to create event");
		bRes = false;
	}
	return bRes;
}

void CMainGui::BringToTop (void) 
{
	CGuard Guard(m_CS);
	SetForegroundWindow(m_hMainWindow);
	SetFocus(GetDesktopWindow());
	Sleep(100);
	SetFocus(m_hMainWindow);
}

void CMainGui::MakeWindowOnTop (bool OnTop) 
{
	CGuard Guard(m_CS);
	SetWindowPos(m_hMainWindow,OnTop ? HWND_TOPMOST : HWND_NOTOPMOST,0,0,0,0,
		SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW);
}

void CMainGui::Caption (LPCWSTR Caption)
{
	CGuard Guard(m_CS);
	SetWindowTextW(m_hMainWindow,Caption);
}

void CMainGui::Create (const char * WindowTitle)
{
	stdstr_f VersionDisplay("Project64 %s", VER_FILE_VERSION_STR);
	m_hMainWindow = (HWND)CreateWindowEx(WS_EX_ACCEPTFILES, VersionDisplay.c_str(), WindowTitle, WS_OVERLAPPED | WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS | WS_SYSMENU | WS_MINIMIZEBOX,5,5,640,480,
		NULL,NULL,GetModuleHandle(NULL),this );
	m_Created = m_hMainWindow != NULL;
}

void CMainGui::CreateStatusBar (void)
{
	m_hStatusWnd = (HWND)CreateStatusWindow( WS_CHILD | WS_VISIBLE, "", m_hMainWindow, StatusBarID );
	SendMessage( (HWND)m_hStatusWnd, SB_SETTEXT, 0, (LPARAM)"" );
}

WPARAM CMainGui::ProcessAllMessages (void)
{
	MSG msg;

	while (GetMessage(&msg,NULL,0,0))
	{
		if (g_BaseSystem && g_BaseSystem->IsDialogMsg(&msg))
		{
			continue;
		}
		if (m_ResetPlugins)
		{
			m_ResetPlugins = false;
			m_ResetInfo->res = m_ResetInfo->plugins->Reset(m_ResetInfo->system);
			SetEvent(m_ResetInfo->hEvent);
			m_ResetInfo = NULL;
		}
		//if (IsDialogMessage( hManageWindow,&msg)) { continue; }
		if (m_Menu->ProcessAccelerator(m_hMainWindow,&msg)) { continue; }
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

bool CMainGui::ProcessGuiMessages (void) {
	MSG msg;

	while (PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)) 
	{
		if (m_ResetPlugins)
		{
			m_ResetPlugins = false;
		}
		if (msg.message == WM_QUIT) {
			return true;
		}
		PeekMessage(&msg,NULL,0,0,PM_REMOVE);
		if (m_Menu->ProcessAccelerator(m_hMainWindow,&msg)) { continue; }
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return false;
}

void CMainGui::Resize (DWORD /*fwSizeType*/, WORD nWidth, WORD nHeight) {
	RECT clrect, swrect;
	GetClientRect( m_hMainWindow, &clrect );
	GetClientRect( (HWND)m_hStatusWnd, &swrect );

	int Parts[2];
	Parts[0] = (nWidth - (int)( clrect.right * 0.25 ));
	Parts[1] = nWidth;

	SendMessage( (HWND)m_hStatusWnd, SB_SETPARTS, 2, (LPARAM)&Parts[0] );
	MoveWindow ( (HWND)m_hStatusWnd, 0, clrect.bottom - swrect.bottom,nWidth, nHeight, TRUE );
}

void CMainGui::Show (bool Visible) 
{
	m_MakingVisible = true;

	CGuard Guard(m_CS);
	if (m_hMainWindow)
	{
		ShowWindow(m_hMainWindow,Visible?SW_SHOW:SW_HIDE);
		if (Visible && RomBrowserVisible()) 
		{
			RomBrowserToTop();
		}
	}

	m_MakingVisible = false;
}

void CMainGui::EnterLogOptions (void) 
{
	::EnterLogOptions(m_hMainWindow);
}

int CMainGui::Height (void) {
	if (!m_hMainWindow) { return 0; }
	
	RECT rect;
	GetWindowRect(m_hMainWindow,&rect);
	return rect.bottom - rect.top;
}

int CMainGui::Width (void) 
{
	if (!m_hMainWindow) { return 0; }

	RECT rect;
	GetWindowRect(m_hMainWindow,&rect);
	return rect.right - rect.left;
}

void CMainGui::SetPos (int X, int Y) 
{
	SetWindowPos(m_hMainWindow,NULL,X,Y,0,0,SWP_NOZORDER|SWP_NOSIZE);
}

void CMainGui::SetWindowMenu (CBaseMenu * Menu) 
{
	m_AttachingMenu = true;

	HMENU hMenu = NULL;
	{
		CGuard Guard(m_CS);
		m_Menu = Menu;
		hMenu = (HMENU)Menu->GetHandle();
	}

	if (hMenu)
	{
		SetMenu(m_hMainWindow,hMenu);
	}

	m_AttachingMenu = false;
}

void CMainGui::RefreshMenu (void)
{
	if (!m_Menu) { return; }
	m_Menu->ResetMenu();
}

void CMainGui::SetStatusText (int Panel,const wchar_t * Text)
{
	static wchar_t Message[2][500];
	if (Panel >= 2)
	{
		Notify().BreakPoint(__FILEW__,__LINE__);
		return;
	}
	wchar_t * Msg = Message[Panel];

	memset(Msg,0,sizeof(Message[0]));
	_snwprintf(Msg, sizeof(Message[0]) / sizeof(Message[0][0]), L"%s", Text);
	Msg[(sizeof(Message[0]) / sizeof(Message[0][0])) - 1] = 0;
	if (GetCurrentThreadId() == m_ThreadId)
	{
		SendMessageW( (HWND)m_hStatusWnd, SB_SETTEXTW, Panel, (LPARAM)Msg );
	} else {
		PostMessageW( (HWND)m_hStatusWnd, SB_SETTEXTW, Panel, (LPARAM)Msg );		
	}
}

void CMainGui::ShowStatusBar ( bool ShowBar )
{
	ShowWindow((HWND)m_hStatusWnd,ShowBar ? SW_SHOW : SW_HIDE );
}

void CMainGui::SaveWindowLoc ( void )
{
	bool flush = false;
	if (m_SaveMainWindowPos)
	{
		m_SaveMainWindowPos = false;
		g_Settings->SaveDword(UserInterface_MainWindowTop,m_SaveMainWindowTop);
		g_Settings->SaveDword(UserInterface_MainWindowLeft,m_SaveMainWindowLeft);
		flush = true;
	}

	if (m_SaveRomBrowserPos)
	{
		m_SaveRomBrowserPos = false;
		g_Settings->SaveDword(RomBrowser_Top,m_SaveRomBrowserTop);
		g_Settings->SaveDword(RomBrowser_Left,m_SaveRomBrowserLeft);
		flush = true;
	}

	if (flush)
	{
		CSettingTypeApplication::Flush();
	}
}

LRESULT CALLBACK CMainGui::MainGui_Proc (HWND hWnd, DWORD uMsg, DWORD wParam, DWORD lParam)
{
	switch (uMsg)
	{	
	case WM_CREATE:
		{
			//record class for future usage	
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			CMainGui * _this = (CMainGui *)lpcs->lpCreateParams;
			SetProp((HWND)hWnd,"Class",_this);
			
			_this->m_hMainWindow = hWnd;
			_this->CreateStatusBar();
			
			//Move the Main window to the location last executed from or center the window
			int X = (GetSystemMetrics( SM_CXSCREEN ) - _this->Width()) / 2;
			int	Y = (GetSystemMetrics( SM_CYSCREEN ) - _this->Height()) / 2;

			g_Settings->LoadDword(UserInterface_MainWindowTop,(DWORD &)Y);
			g_Settings->LoadDword(UserInterface_MainWindowLeft,(DWORD &)X);

			_this->SetPos(X,Y);
			
			_this->ChangeWinSize(640,480);

		}
		break;	
	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			{
				CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
				if (_this && 
					_this->bCPURunning() &&
					!g_Settings->LoadBool(GameRunning_CPU_Paused) &&
					g_Settings->LoadDword(Setting_DisableScrSaver))
				{
					return 0;
				}
			}
			break;
		case SC_MAXIMIZE:
			{
				CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
				if (_this) { 
					if (_this->RomBrowserVisible()) {
						_this->RomBrowserMaximize(true);
					}
				}
			}
			break;
		}
		return DefWindowProc((HWND)hWnd,uMsg,wParam,lParam);
		break;
	case WM_MOVE:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");

			if (!_this->m_bMainWindow || 
				!_this->m_Created || 
				_this->m_AttachingMenu ||
				_this->m_MakingVisible ||
				IsIconic((HWND)hWnd) || 
				_this->ShowingRomBrowser())
			{
				break;
			}

			if (IsZoomed((HWND)hWnd))
			{
				if (_this->RomBrowserVisible())
				{
					// save that browser is maximized
				}
				break;
			}

			//get the current position of the window
			RECT WinRect;
			GetWindowRect((HWND)hWnd, &WinRect );
			
			//save the location of the window
			if (_this->RomBrowserVisible()) 
			{
				_this->m_SaveRomBrowserPos = true;
				_this->m_SaveRomBrowserTop = WinRect.top;
				_this->m_SaveRomBrowserLeft = WinRect.left;
			} else {
				_this->m_SaveMainWindowPos = true;
				_this->m_SaveMainWindowTop = WinRect.top;
				_this->m_SaveMainWindowLeft = WinRect.left;
			}
			KillTimer(hWnd,Timer_SetWindowPos);
			SetTimer(hWnd,Timer_SetWindowPos,1000,NULL);
		}
		if (CGuiSettings::bCPURunning() && g_BaseSystem) 
		{
			if (g_Plugins->Gfx() && g_Plugins->Gfx()->MoveScreen)
			{
				WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Starting");
				g_Plugins->Gfx()->MoveScreen((int)(short) LOWORD(lParam), (int)(short) HIWORD(lParam));
				WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Done");
			}
		}
		break;
	case WM_TIMER:
		if (wParam == Timer_SetWindowPos)
		{
			KillTimer(hWnd,Timer_SetWindowPos);
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			_this->SaveWindowLoc();
			break;
		}
		break;
	case WM_SIZE: 
		{   
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			if (_this) { _this->Resize(wParam,LOWORD(lParam), HIWORD(lParam)); } 
			if (_this) 
			{ 
				if (wParam == SIZE_MAXIMIZED)
				{
					if (_this->RomBrowserVisible()) {
						_this->RomBrowserMaximize(true);
					}
				}
				_this->ResizeRomList(LOWORD(lParam), HIWORD(lParam)); 
			}
			if (_this) {
				if (wParam == SIZE_RESTORED && _this->RomBrowserVisible()) {
					_this->RomBrowserMaximize(false);
				}
			}
		}
		break;
	case WM_NOTIFY:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			if (_this) { 
				if (_this->RomBrowserVisible() && !_this->RomListNotify(wParam,lParam)) {
					return DefWindowProc((HWND)hWnd,uMsg,wParam,lParam);
				}
			}
		}
		break;
	case WM_DRAWITEM:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			if (_this) { 
				if (!_this->RomListDrawItem(wParam,lParam)) {
					return DefWindowProc((HWND)hWnd,uMsg,wParam,lParam);
				}
			}
		}
		break;
	case WM_PAINT:
		{
//			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
//			CN64System * System  = _this->m_System;

//			if (bCPURunning() && Settings->Load(CPU_Paused)) {
//				CPlugins * Plugins = System->Plugins();
//				if (Plugins->Gfx()->DrawScreen) {
//					Plugins->Gfx()->DrawScreen();
//				}
//			}
			ValidateRect((HWND)hWnd,NULL);
		}
		break;
	case WM_KEYUP:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");

			if (_this->m_bMainWindow && bCPURunning()) 
			{
				if (g_BaseSystem)
				{
					if (g_Plugins && g_Plugins->Control()->WM_KeyUp) {
						g_Plugins->Control()->WM_KeyUp(wParam, lParam);
					}
				}
			}
		}
		break;
	case WM_KEYDOWN:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");

			if (_this->m_bMainWindow && bCPURunning()) 
			{
				if (g_BaseSystem)
				{
					if (g_Plugins && g_Plugins->Control()->WM_KeyDown)
					{
						g_Plugins->Control()->WM_KeyDown(wParam, lParam);
					}
				}
			}
		}
		break;
	case WM_SETFOCUS: 
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			if (_this->RomBrowserVisible()) 
			{
				PostMessage((HWND)hWnd,WM_BORWSER_TOP,0,0);
				break;
			}

			if (_this->m_bMainWindow && bCPURunning() && bAutoSleep())
			{
				if (g_BaseSystem)
				{
					g_BaseSystem->ExternalEvent( SysEvent_ResumeCPU_AppGainedFocus );
				}
			}
		}
		break;
	case WM_KILLFOCUS:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			if (_this->RomBrowserVisible()) 
			{
				break;
			}

			if (_this->m_bMainWindow && bCPURunning() && bAutoSleep())
			{ 
				if (g_BaseSystem)
				{
					g_BaseSystem->ExternalEvent( SysEvent_PauseCPU_AppLostFocus );
				}
			}
		}
		break;
	case WM_ACTIVATEAPP:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			DWORD fActive = (BOOL) wParam;

			if (fActive && _this->RomBrowserVisible()) {
				PostMessage((HWND)hWnd,WM_BORWSER_TOP,0,0);
			}
			if (_this->m_bMainWindow && bCPURunning())
			{
				if (!fActive && g_Settings->LoadBool(UserInterface_InFullScreen))
				{
					g_Notify->WindowMode();
					if (bAutoSleep() && g_BaseSystem)
					{
						//System->ExternalEvent(PauseCPU_AppLostActiveDelayed );
					}
					break;
				}
				if (bAutoSleep() || fActive)
				{ 
					if (g_BaseSystem)
					{
						g_BaseSystem->ExternalEvent(fActive ? SysEvent_ResumeCPU_AppGainedActive : SysEvent_PauseCPU_AppLostActive );
					}
				}
			}

		}
		break;
	case WM_HIDE_CUROSR: 
		if (!wParam) {
			while (ShowCursor(FALSE) >= 0) { Sleep(0); }
		} else {
			while (ShowCursor(TRUE) < 0) { Sleep(0); }
		}
		break;
	case WM_MAKE_FOCUS:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			_this->BringToTop();
		}
		break;
	case WM_BORWSER_TOP:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			_this->RomBrowserToTop();
		}
		break;
	case WM_RESET_PLUGIN:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd, "Class");
			if (_this->m_ResetInfo != NULL)
			{
				Notify().BreakPoint(__FILEW__, __LINE__);
			}
			_this->m_ResetInfo = (RESET_PLUGIN *)lParam;
			_this->m_ResetPlugins = true;
		}
		break;
	case WM_COMMAND:
		{
			CMainGui * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			if (_this == NULL) { break; }

			switch (LOWORD(wParam)) {			
			case ID_POPUPMENU_PLAYGAME: g_BaseSystem->RunFileImage(_this->CurrentedSelectedRom()); break;
			case ID_POPUPMENU_ROMDIRECTORY:   _this->SelectRomDir(); break;
			case ID_POPUPMENU_REFRESHROMLIST: _this->RefreshRomBrowser(); break;
			case ID_POPUPMENU_ROMINFORMATION: 
				{
					RomInformation Info(_this->CurrentedSelectedRom());
					Info.DisplayInformation(hWnd);
				}
				break;
			case ID_POPUPMENU_EDITSETTINGS: 
			case ID_POPUPMENU_EDITCHEATS: 
				{
					CN64Rom Rom;
					Rom.LoadN64Image(_this->CurrentedSelectedRom(),true);
					Rom.SaveRomSettingID(true);
					/*if (g_Settings->LoadString(ROM_MD5).length() == 0) {
						Rom.LoadN64Image(_this->CurrentedSelectedRom(),false);
						g_Settings->SaveString(ROM_MD5,Rom.GetRomMD5().c_str());
						g_Settings->SaveString(ROM_InternalName,Rom.GetRomName().c_str());
					}*/
					
					if (LOWORD(wParam) == ID_POPUPMENU_EDITSETTINGS)
					{
						CSettingConfig SettingConfig(true);
						SettingConfig.Display(hWnd);
					}

					if (LOWORD(wParam) == ID_POPUPMENU_EDITCHEATS) {
						CCheats RomCheats(&Rom);
						RomCheats.SelectCheats(hWnd,true);
					}

					if (g_Rom) {
						g_Rom->SaveRomSettingID(false);
					} else {
						Rom.ClearRomSettingID();
					}
				}
				break;
			default:
				if (_this->m_Menu) {
					if (LOWORD(wParam) > 5000 && LOWORD(wParam) <= 5100 ) { 
						if (g_Plugins->RSP())
						{
							g_Plugins->RSP()->ProcessMenuItem(LOWORD(wParam));
						}
					} else if (LOWORD(wParam) > 5100 && LOWORD(wParam) <= 5200 ) { 
						if (g_Plugins->Gfx())
						{
							WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Starting");
							g_Plugins->Gfx()->ProcessMenuItem(LOWORD(wParam));
							WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Done");
						}
					} else if (LOWORD(wParam) > 5200 && LOWORD(wParam) <= 5300 ) { 
						if (g_Plugins->Gfx() && g_Plugins->Gfx()->OnRomBrowserMenuItem != NULL) 
						{
							CN64Rom Rom;
							if (!Rom.LoadN64Image(_this->CurrentedSelectedRom(),true))
							{
								break;
							}
							Rom.SaveRomSettingID(true);
							g_Notify->DisplayMessage(0,L"");
							BYTE * RomHeader = Rom.GetRomAddress();
							WriteTrace(TraceGfxPlugin,__FUNCTION__ ": OnRomBrowserMenuItem - Starting");
							g_Plugins->Gfx()->OnRomBrowserMenuItem(LOWORD(wParam),hWnd,RomHeader);
							WriteTrace(TraceGfxPlugin,__FUNCTION__ ": OnRomBrowserMenuItem - Done");
							if (g_Rom) {
								g_Rom->SaveRomSettingID(false);
							} else {
								g_Settings->SaveString(Game_IniKey,"");
							}
						}
					} else if (_this->m_Menu->ProcessMessage(hWnd,HIWORD(wParam), LOWORD(wParam))) {
						return true;
					}
				}
			}
		}
		break;
	case WM_DROPFILES:
		{
			char filename[MAX_PATH];

			HDROP hDrop = (HDROP)wParam;
			DragQueryFile(hDrop, 0, filename, sizeof(filename));
			DragFinish(hDrop);

			CN64System::RunFileImage(filename);
		}
		break;
	case WM_DESTROY:
		WriteTrace(TraceDebug,__FUNCTION__ ": WM_DESTROY - start");
		{
			CMainGui   * _this = (CMainGui *)GetProp((HWND)hWnd,"Class");
			if (_this->m_bMainWindow)
			{
				g_Notify->WindowMode();
			}
			_this->m_hMainWindow = NULL;
			WriteTrace(TraceDebug,__FUNCTION__ ": WM_DESTROY - 1");
			if (_this->m_bMainWindow)
			{
				_this->SaveRomListColoumnInfo();
				WriteTrace(TraceDebug,__FUNCTION__ ": WM_DESTROY - 2");
				_this->SaveWindowLoc();
			}
		}
		WriteTrace(TraceDebug,__FUNCTION__ ": WM_DESTROY - 3");
		RemoveProp((HWND)hWnd,"Class");
		WriteTrace(TraceDebug,__FUNCTION__ ": WM_DESTROY - 4");
		PostQuitMessage(0);
		WriteTrace(TraceDebug,__FUNCTION__ ": WM_DESTROY - Done");
		break;
	default:
		return DefWindowProc((HWND)hWnd,uMsg,wParam,lParam);
	}
	return TRUE;
}

WNDPROC pfnWndAboutBoxCancelProc = NULL;
HBITMAP hCloseButton = NULL;

DWORD CALLBACK AboutBoxCancelProc (HWND hWnd, DWORD uMsg, DWORD wParam, DWORD lParam) 
{		
	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			if (BeginPaint(hWnd,&ps))
			{
				if (hCloseButton)
				{
					RECT rcClient;
					GetClientRect(hWnd, &rcClient);

					BITMAP bmTL1;
					GetObject(hCloseButton, sizeof(BITMAP), &bmTL1);
					HDC     memdc	= CreateCompatibleDC(ps.hdc);
					HGDIOBJ save	= SelectObject(memdc, hCloseButton);
					BitBlt(ps.hdc, 0, 0, bmTL1.bmWidth, bmTL1.bmHeight, memdc, 0, 0, SRCCOPY);
					SelectObject(memdc, save);
					DeleteDC(memdc);
				}
				EndPaint(hWnd,&ps);
			}
		}
		break;
	}
			
	return CallWindowProc(pfnWndAboutBoxCancelProc, hWnd, uMsg, wParam, lParam);
}

DWORD CALLBACK AboutBoxProc (HWND hWnd, DWORD uMsg, DWORD wParam, DWORD lParam) 
{
	static HBITMAP hbmpBackgroundTop = NULL;
	static HBITMAP hbmpBackgroundBottom = NULL;
	static HBITMAP hbmpBackgroundMiddle = NULL;
	static HFONT   hPageHeadingFont = NULL;
	static HFONT   hTextFont = NULL;
	static HFONT   hAuthorFont = NULL;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			enum { ROUND_EDGE = 15 };

			DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
			dwStyle &= ~(WS_CAPTION|WS_SIZEBOX);
			SetWindowLong(hWnd, GWL_STYLE, dwStyle);

			// Use the size of the image
			hbmpBackgroundTop    = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT_TOP)); 
			hbmpBackgroundBottom = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT_BOTTOM)); 
			hbmpBackgroundMiddle = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT_MIDDLE)); 
			
			BITMAP bmTL;
			GetObject(hbmpBackgroundTop, sizeof(BITMAP), &bmTL);

			hCloseButton               = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_CLOSE_NORMAL)); 
			pfnWndAboutBoxCancelProc   = (WNDPROC)::GetWindowLongPtr(GetDlgItem(hWnd,IDCANCEL), GWLP_WNDPROC);
			::SetWindowLongPtr(GetDlgItem(hWnd,IDCANCEL), GWLP_WNDPROC,(LONG_PTR)AboutBoxCancelProc);


			if (hbmpBackgroundTop)
			{
//				int iHeight = bmTL.bmHeight;
				int iWidth  = bmTL.bmWidth;

				RECT rect;
				GetWindowRect(hWnd, &rect);
				rect.left -= rect.left;
				rect.bottom -= rect.top;
				rect.top -= rect.top;

				// Tweaked
				HRGN hWindowRegion= CreateRoundRectRgn
				(
					rect.left,
					rect.top,
					rect.left+iWidth+GetSystemMetrics(SM_CXEDGE)-1,
					rect.bottom+GetSystemMetrics(SM_CYEDGE)-1,
					ROUND_EDGE,
					ROUND_EDGE
				);

				if (hWindowRegion)
				{
					SetWindowRgn(hWnd, hWindowRegion, TRUE);
					DeleteObject(hWindowRegion);
				}
			}

			hTextFont = ::CreateFont
			(
				18, 
				0,
				0, 
				0, 
				FW_NORMAL,
				0,
				0,
				0,
				DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				PROOF_QUALITY,
				DEFAULT_PITCH|FF_DONTCARE,
				_T("Arial")
			);

			hAuthorFont = ::CreateFont
			(
				18, 
				0,
				0, 
				0, 
				FW_BOLD,
				0,
				0,
				0,
				DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				PROOF_QUALITY,
				DEFAULT_PITCH|FF_DONTCARE,
				_T("Arial")
			);

			hPageHeadingFont = ::CreateFont
			(
				24, 
				0,
				0, 
				0, 
				FW_BOLD,
				0,
				FALSE, //Show underlined?
				0,
				DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				PROOF_QUALITY,
				DEFAULT_PITCH|FF_DONTCARE,
				_T("Arial Bold")
			);

			SendDlgItemMessage(hWnd,IDC_VERSION,WM_SETFONT,(WPARAM)hTextFont,TRUE);
			SendDlgItemMessage(hWnd,IDC_TEAM,WM_SETFONT,(WPARAM)hPageHeadingFont,TRUE);
			SendDlgItemMessage(hWnd,IDC_THANKS,WM_SETFONT,(WPARAM)hPageHeadingFont,TRUE);

			SendDlgItemMessage(hWnd,IDC_ZILMAR,WM_SETFONT,(WPARAM)hAuthorFont,TRUE);
			SendDlgItemMessage(hWnd,IDC_JABO,WM_SETFONT,(WPARAM)hAuthorFont,TRUE);
			SendDlgItemMessage(hWnd,IDC_SMIFF,WM_SETFONT,(WPARAM)hAuthorFont,TRUE);
			SendDlgItemMessage(hWnd,IDC_GENT,WM_SETFONT,(WPARAM)hAuthorFont,TRUE);

			SendDlgItemMessage(hWnd,IDC_ZILMAR_DETAILS,WM_SETFONT,(WPARAM)hTextFont,TRUE);
			SendDlgItemMessage(hWnd,IDC_JABO_DETAILS,WM_SETFONT,(WPARAM)hTextFont,TRUE);
			SendDlgItemMessage(hWnd,IDC_SMIFF_DETAILS,WM_SETFONT,(WPARAM)hTextFont,TRUE);
			SendDlgItemMessage(hWnd,IDC_GENT_DETAILS,WM_SETFONT,(WPARAM)hTextFont,TRUE);
			
			SendDlgItemMessage(hWnd,IDC_THANK_LIST,WM_SETFONT,(WPARAM)hTextFont,TRUE);

			//SetCapture(hWnd);
			stdstr_f VersionDisplay("Version: %s", VER_FILE_VERSION_STR);
			SetWindowText(GetDlgItem(hWnd,IDC_VERSION),VersionDisplay.c_str());
		}
		break;
	case WM_NCHITTEST:
		{
			int xPos = LOWORD(lParam); 
			int yPos = HIWORD(lParam); 
			RECT client, a;
			GetClientRect(hWnd,&a);
			GetClientRect(hWnd,&client);
			ClientToScreen(hWnd,(LPPOINT)&client);
			client.right += client.left;
			client.bottom += client.top;


			int nCaption = GetSystemMetrics(SM_CYCAPTION)*4;

			LRESULT lResult = HTCLIENT;

			//check caption
			if (xPos <= client.right && xPos >= client.left && 
				(yPos >= client.top+ 0)&& (yPos <= client.top + 0+nCaption))
			{
				lResult = HTCAPTION;
			}
			SetWindowLong(hWnd, DWLP_MSGRESULT, lResult);
			return TRUE;
		}
		break;
	case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			if (BeginPaint(hWnd,&ps))
			{
				RECT rcClient;
				GetClientRect(hWnd, &rcClient);

				BITMAP bmTL_top, bmTL_bottom, bmTL_Middle;
				GetObject(hbmpBackgroundTop, sizeof(BITMAP), &bmTL_top);
				GetObject(hbmpBackgroundBottom, sizeof(BITMAP), &bmTL_bottom);
				GetObject(hbmpBackgroundMiddle, sizeof(BITMAP), &bmTL_Middle);

				HDC     memdc	= CreateCompatibleDC(ps.hdc);
				HGDIOBJ save	= SelectObject(memdc, hbmpBackgroundTop);
				BitBlt(ps.hdc, 0, 0, bmTL_top.bmWidth, bmTL_top.bmHeight, memdc, 0, 0, SRCCOPY);
				SelectObject(memdc, save);
				DeleteDC(memdc);

				
				memdc	= CreateCompatibleDC(ps.hdc);
				save	= SelectObject(memdc, hbmpBackgroundMiddle);
				for (int x = bmTL_top.bmHeight; x < rcClient.bottom; x += bmTL_Middle.bmHeight)
				{
					//BitBlt(ps.hdc, 0, bmTL_top.bmHeight, bmTL_Middle.bmWidth, rcClient.bottom - (bmTL_bottom.bmHeight + bmTL_top.bmHeight), memdc, 0, 0, SRCCOPY);
					BitBlt(ps.hdc, 0, x, bmTL_Middle.bmWidth, bmTL_Middle.bmHeight, memdc, 0, 0, SRCCOPY);
				}
				SelectObject(memdc, save);
				DeleteDC(memdc);

				BITMAP ;
				memdc	= CreateCompatibleDC(ps.hdc);
				save	= SelectObject(memdc, hbmpBackgroundBottom);
				BitBlt(ps.hdc, 0, rcClient.bottom - bmTL_bottom.bmHeight, bmTL_bottom.bmWidth, bmTL_bottom.bmHeight, memdc, 0, 0, SRCCOPY);
				SelectObject(memdc, save);
				DeleteDC(memdc);

				BITMAP ;

				EndPaint(hWnd,&ps);
			}
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			if (hbmpBackgroundTop)
			{
				DeleteObject(hbmpBackgroundTop);
			}
			if (hbmpBackgroundBottom)
			{
				DeleteObject(hbmpBackgroundBottom);
			}
			if (hbmpBackgroundMiddle)
			{
				DeleteObject(hbmpBackgroundMiddle);
			}
			if (hTextFont)
				::DeleteObject(hTextFont);
			if (hPageHeadingFont)
				::DeleteObject(hPageHeadingFont);
			if (hAuthorFont)
				::DeleteObject(hAuthorFont);


			//ReleaseCapture();
			EndDialog(hWnd,0);
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL set_about_field(
    HWND hDlg,
    int nIDDlgItem,
    const wchar_t * config_string,
    const wchar_t * language_string
)
{
    wchar_t temp_string[200];

    swprintf(
        temp_string,
        sizeof(temp_string) / sizeof(temp_string[0]),
        L"%s: %s",
        config_string,
        language_string
    );
    return SetDlgItemTextW(
        hDlg,
        nIDDlgItem,
        temp_string
    );
}
