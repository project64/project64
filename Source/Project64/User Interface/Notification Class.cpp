#include "stdafx.h"
#include <time.h>

CNotification & Notify ( void )
{
	static CNotification g_Notify;
	return g_Notify;
}

CNotification::CNotification  ( ) :
	m_NextMsg(0), 
	m_gfxPlugin(NULL),
	m_hWnd(NULL)
{
	 _tzset();
}

void CNotification::SetMainWindow  ( CMainGui * Gui ) 
{
	m_hWnd = Gui;
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

void CNotification::DisplayError ( const wchar_t * Message, ... ) const 
{
	va_list ap;
	va_start( ap, Message );
	DisplayError (Message,ap);
}

void CNotification::DisplayError ( const wchar_t * Message, va_list ap ) const 
{
	if (this == NULL) { return; }
	wchar_t Msg[1000];

	_vsnwprintf( Msg,sizeof(Msg) - 1,Message, ap );
	va_end( ap );

    stdstr TraceMessage;
    TraceMessage.FromUTF16(Msg);
	WriteTrace(TraceError,TraceMessage.c_str());
	WindowMode();

	HWND Parent = NULL;
	if (m_hWnd)
    { 
        Parent = m_hWnd->GetHandle(); 
    }
	MessageBoxW(Parent,Msg,GS(MSG_MSGBOX_TITLE),MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
}

void CNotification::DisplayMessage  ( int DisplayTime, const wchar_t * Message, ... ) const 
{
	va_list ap;
	va_start( ap, Message );
	DisplayMessage (DisplayTime, Message,ap);
}

void CNotification::DisplayMessage  ( int DisplayTime, const wchar_t * Message, va_list ap ) const 
{
	if (!m_hWnd) { return; }

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
	
	wchar_t Msg[1000];

	_vsnwprintf( Msg,sizeof(Msg) - 1,Message, ap );
	va_end( ap );
	
	
	if (InFullScreen())
	{
		if (m_gfxPlugin && m_gfxPlugin->DrawStatus)
		{
			WriteTrace(TraceGfxPlugin,__FUNCTION__ ": DrawStatus - Starting");
			stdstr PluginMessage;
			PluginMessage.FromUTF16(Msg);
			m_gfxPlugin->DrawStatus(PluginMessage.c_str(), FALSE);
			WriteTrace(TraceGfxPlugin,__FUNCTION__ ": DrawStatus - Done");
		}
	} 
    else 
    {
		m_hWnd->SetStatusText(0, Msg);
	}
}

void CNotification::DisplayMessage2 ( const wchar_t * Message, ... ) const 
{
	va_list ap;
	va_start( ap, Message );
	DisplayMessage2 (Message,ap);
}

void CNotification::DisplayMessage2 (  const wchar_t * Message, va_list ap ) const 
{
	if (!m_hWnd) { return; }

	wchar_t Msg[1000];
	_vsnwprintf( Msg,sizeof(Msg) - 1 ,Message, ap );
	va_end( ap );
	
    m_hWnd->SetStatusText(1,Msg);
}

void CNotification::SetGfxPlugin( CGfxPlugin * Plugin )
{
	m_gfxPlugin = Plugin;
}

void CNotification::SetWindowCaption (const wchar_t * Caption)
{
	wchar_t WinTitle[256];
    _snwprintf( WinTitle, sizeof(WinTitle), L"%s - %s", Caption, g_Settings->LoadString(Setting_ApplicationName).ToUTF16().c_str());
	WinTitle[sizeof(WinTitle) - 1] = 0;
	m_hWnd->Caption(WinTitle);
}

void CNotification::FatalError  ( const wchar_t * Message, ... ) const 
{
	wchar_t Msg[1000];
	va_list ap;

	WindowMode();

	va_start( ap, Message );
	_vsnwprintf( Msg,(sizeof(Msg) / sizeof(Msg[0])) - 1, Message, ap );
	va_end( ap );
	HWND Parent = NULL;
	if (m_hWnd) { Parent = reinterpret_cast<HWND>(m_hWnd->GetHandle()); }
	MessageBoxW(Parent,Msg,L"Error",MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
	ExitThread(0);
}

void CNotification::AddRecentDir ( const char * RomDir ) 
{
	//Validate the passed string
	if (HIWORD(RomDir) == NULL) { return; }

	//Get Information about the stored rom list
	size_t MaxRememberedDirs = g_Settings->LoadDword(Directory_RecentGameDirCount);
	strlist RecentDirs;
	size_t i;
	for (i = 0; i < MaxRememberedDirs; i ++ ) 
	{
		stdstr RecentDir = g_Settings->LoadStringIndex(Directory_RecentGameDirIndex,i);
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
		g_Settings->SaveStringIndex(Directory_RecentGameDirIndex,i,*iter);
	}
}

void CNotification::AddRecentRom ( const char * ImagePath ) 
{
	if (HIWORD(ImagePath) == NULL) { return; }

	//Get Information about the stored rom list
	size_t MaxRememberedFiles = g_Settings->LoadDword(File_RecentGameFileCount);
	strlist RecentGames;
	size_t i;
	for (i = 0; i < MaxRememberedFiles; i ++ ) 
	{
		stdstr RecentGame = g_Settings->LoadStringIndex(File_RecentGameFileIndex,i);
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
		g_Settings->SaveStringIndex(File_RecentGameFileIndex,i,*iter);
	}
}

void CNotification::RefreshMenu ( void )
{
	if (m_hWnd == NULL) { return; }
	m_hWnd->RefreshMenu();
}

void CNotification::HideRomBrowser ( void )
{
	if (m_hWnd == NULL) { return; }
	m_hWnd->HideRomList();
}

void CNotification::ShowRomBrowser ( void )
{
	if (m_hWnd == NULL) { return; }
	if (g_Settings->LoadDword(RomBrowser_Enabled))
    {
		//Display the rom browser
		m_hWnd->ShowRomList();
		m_hWnd->HighLightLastRom();
	}
}

void CNotification::BringToTop ( void )
{
	if (m_hWnd == NULL) { return; }
	m_hWnd->BringToTop();
}

void CNotification::MakeWindowOnTop ( bool OnTop )
{
	if (m_hWnd == NULL) { return; }
	m_hWnd->MakeWindowOnTop(OnTop);
}

void CNotification::ChangeFullScreen ( void ) const
{
	if (m_hWnd == NULL) { return; }
	SendMessage((HWND)(m_hWnd->GetHandle()),WM_COMMAND,MAKELPARAM(ID_OPTIONS_FULLSCREEN2,false),0);
}

bool CNotification::ProcessGuiMessages ( void ) const
{
	if (m_hWnd == NULL) { return false; }
	return m_hWnd->ProcessGuiMessages();
}

void CNotification::BreakPoint ( const wchar_t * FileName, const int LineNumber )
{
	if (g_Settings->LoadBool(Debugger_Enabled))
	{
		DisplayError(L"Break point found at\n%s\n%d",FileName, LineNumber);
		if (IsDebuggerPresent() != 0)
		{
			DebugBreak();
		}
        else 
        {
			g_BaseSystem->CloseCpu();
		}
	}
    else 
    {
		DisplayError(L"Fatal Error: Stopping emulation");
		g_BaseSystem->CloseCpu();
	}
}
