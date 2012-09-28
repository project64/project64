#include "stdafx.h"
#include <time.h>

CNotification & Notify ( void )
{
	static CNotification _Notify;
	return _Notify;
}

CNotification::CNotification  ( ) :
	m_NextMsg(0), _gfxPlugin(NULL)
{
	_hWnd     = NULL;
	 _tzset();
}

void CNotification::SetMainWindow  ( CMainGui * Gui ) {
	_hWnd = Gui;
}

void CNotification::WindowMode ( void ) const
{
	static bool InsideFunc = false;
	if (InsideFunc)
	{
		return;
	}
	InsideFunc = true;
	if (InFullScreen())
	{
		ChangeFullScreen();
		for (int i = 0; i < 5; i++)
		{
			Sleep(50);
			if (ProcessGuiMessages())
			{
				break;
			}
		}
	}
	InsideFunc = false;
}

void CNotification::DisplayError  ( const char * Message, ... ) const {
	va_list ap;
	va_start( ap, Message );
	DisplayError (Message,ap);
}

void CNotification::DisplayError  (  const char * Message, va_list ap ) const {
	if (this == NULL) { return; }
	char Msg[1000];

	_vsnprintf( Msg,sizeof(Msg) - 1,Message, ap );
	va_end( ap );

	WriteTrace(TraceError,Msg);
	WindowMode();

	HWND Parent = NULL;
	if (_hWnd) { Parent = reinterpret_cast<HWND>(_hWnd->GetHandle()); }
	MessageBox(Parent,Msg,GS(MSG_MSGBOX_TITLE),MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
}

void CNotification::DisplayMessage  ( int DisplayTime, const char * Message, ... ) const {
	va_list ap;
	va_start( ap, Message );
	DisplayMessage (DisplayTime, Message,ap);
}

void CNotification::DisplayMessage  ( int DisplayTime, const char * Message, va_list ap ) const {
	if (!_hWnd) { return; }

	if (m_NextMsg > 0 || DisplayTime > 0)
	{
		time_t Now = time(NULL);
		if (DisplayTime == 0 && Now < m_NextMsg)
		{
			return;
		}
		if (DisplayTime > 0)
		{
			m_NextMsg = Now + DisplayTime;
		}
		if (m_NextMsg == 0)
		{
			m_NextMsg = 0;
		}
	}
	
	char Msg[1000];

	_vsnprintf( Msg,sizeof(Msg) - 1,Message, ap );
	va_end( ap );
	
	
	if (InFullScreen())
	{
		if (_gfxPlugin && _gfxPlugin->DrawStatus)
		{
			WriteTrace(TraceGfxPlugin,"DrawStatus: Starting");
			_gfxPlugin->DrawStatus(Msg,FALSE);
			WriteTrace(TraceGfxPlugin,"DrawStatus: Done");
		}
	} else {
		_hWnd->SetStatusText(0,Msg);
	}
}

void CNotification::DisplayMessage2  ( const char * Message, ... ) const {
	va_list ap;
	va_start( ap, Message );
	DisplayMessage2 (Message,ap);
}

void CNotification::DisplayMessage2  (  const char * Message, va_list ap ) const {
	if (!_hWnd) { return; }

	char Msg[1000];
	_vsnprintf( Msg,sizeof(Msg) - 1 ,Message, ap );
	va_end( ap );
	
	_hWnd->SetStatusText(1,Msg);
}

void CNotification::SetGfxPlugin( CGfxPlugin * Plugin )
{
	_gfxPlugin = Plugin;
}

void CNotification::SetWindowCaption (const char * Caption) {
	char WinTitle[256];
	_snprintf( WinTitle, sizeof(WinTitle), "%s - %s", Caption, _Settings->LoadString(Setting_ApplicationName).c_str());
	WinTitle[sizeof(WinTitle) - 1] = 0;
	_hWnd->Caption(WinTitle);
}

void CNotification::FatalError  ( const char * Message, ... ) const {
	char Msg[1000];
	va_list ap;

	WindowMode();

	va_start( ap, Message );
	_vsnprintf( Msg,sizeof(Msg) - 1,Message, ap );
	va_end( ap );
	HWND Parent = NULL;
	if (_hWnd) { Parent = reinterpret_cast<HWND>(_hWnd->GetHandle()); }
	MessageBox(Parent,Msg,"Error",MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
	ExitThread(0);
}

void CNotification::AddRecentDir   ( const char * RomDir ) {
	//Validate the passed string
	if (HIWORD(RomDir) == NULL) { return; }

	//Get Information about the stored rom list
	size_t MaxRememberedDirs = _Settings->LoadDword(Directory_RecentGameDirCount);
	strlist RecentDirs;
	size_t i;
	for (i = 0; i < MaxRememberedDirs; i ++ ) 
	{
		stdstr RecentDir = _Settings->LoadStringIndex(Directory_RecentGameDirIndex,i);
		if (RecentDir.empty()) 
		{
			break;
		}
		RecentDirs.push_back(RecentDir);
	}

	//See if the dir is already in the list if so then move it to the top of the list
	strlist::iterator iter;
	for (iter = RecentDirs.begin(); iter != RecentDirs.end(); iter++)
	{
		if (_stricmp(RomDir,iter->c_str()) != 0)
		{
			continue;
		}
		RecentDirs.erase(iter);
		break;
	}
	RecentDirs.push_front(RomDir);
	if (RecentDirs.size() > MaxRememberedDirs)
	{
		RecentDirs.pop_back();
	}
	
	for (i = 0, iter = RecentDirs.begin(); iter != RecentDirs.end(); iter++, i++)
	{
		_Settings->SaveStringIndex(Directory_RecentGameDirIndex,i,*iter);
	}
}

void CNotification::AddRecentRom   ( const char * ImagePath ) {
	if (HIWORD(ImagePath) == NULL) { return; }

	//Get Information about the stored rom list
	size_t MaxRememberedFiles = _Settings->LoadDword(File_RecentGameFileCount);
	strlist RecentGames;
	size_t i;
	for (i = 0; i < MaxRememberedFiles; i ++ ) 
	{
		stdstr RecentGame = _Settings->LoadStringIndex(File_RecentGameFileIndex,i);
		if (RecentGame.empty()) 
		{
			break;
		}
		RecentGames.push_back(RecentGame);
	}

	//See if the dir is already in the list if so then move it to the top of the list
	strlist::iterator iter;
	for (iter = RecentGames.begin(); iter != RecentGames.end(); iter++)
	{
		if (_stricmp(ImagePath,iter->c_str()) != 0)
		{
			continue;
		}
		RecentGames.erase(iter);
		break;
	}
	RecentGames.push_front(ImagePath);
	if (RecentGames.size() > MaxRememberedFiles)
	{
		RecentGames.pop_back();
	}
	
	for (i = 0, iter = RecentGames.begin(); iter != RecentGames.end(); iter++, i++)
	{
		_Settings->SaveStringIndex(File_RecentGameFileIndex,i,*iter);
	}
}

void CNotification::RefreshMenu ( void ) {
	if (_hWnd == NULL) { return; }
	_hWnd->RefreshMenu();
}

void CNotification::HideRomBrowser ( void ) {
	if (_hWnd == NULL) { return; }
	_hWnd->HideRomList();
}

void CNotification::ShowRomBrowser ( void ) {
	if (_hWnd == NULL) { return; }
	if (_Settings->LoadDword(RomBrowser_Enabled)) { 
		//Display the rom browser
		_hWnd->ShowRomList();
		_hWnd->HighLightLastRom();
	}
}

void CNotification::BringToTop ( void ) {
	if (_hWnd == NULL) { return; }
	_hWnd->BringToTop();
}

void CNotification::MakeWindowOnTop ( bool OnTop ) {
	if (_hWnd == NULL) { return; }
	_hWnd->MakeWindowOnTop(OnTop);
}

void CNotification::ChangeFullScreen ( void ) const
{
	if (_hWnd == NULL) { return; }
	SendMessage((HWND)(_hWnd->GetHandle()),WM_COMMAND,MAKELPARAM(ID_OPTIONS_FULLSCREEN2,false),0);
}

bool CNotification::ProcessGuiMessages ( void ) const
{
	if (_hWnd == NULL) { return false; }
	return _hWnd->ProcessGuiMessages();
}

void CNotification::BreakPoint ( const char * File, const int LineNumber ) {
	DisplayError("Break point found at\n%s\n%d",File, LineNumber);

	_asm int 3
}