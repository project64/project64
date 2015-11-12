#include "stdafx.h"
#include <time.h>

CNotification & Notify(void)
{
    static CNotification g_Notify;
    return g_Notify;
}

CNotification::CNotification() :
m_hWnd(NULL),
m_gfxPlugin(NULL),
m_NextMsg(0)
{
    _tzset();
}

void CNotification::AppInitDone(void)
{
    CNotificationSettings::RegisterNotifications();
}

void CNotification::SetMainWindow(CMainGui * Gui)
{
    m_hWnd = Gui;
}

void CNotification::WindowMode(void) const
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

void CNotification::DisplayError(LanguageStringID StringID) const
{
    DisplayError(g_Lang->GetString(StringID).c_str());
}

void CNotification::DisplayError(const wchar_t * Message) const
{
    if (this == NULL) { return; }

    stdstr TraceMessage;
    TraceMessage.FromUTF16(Message);
    WriteTrace(TraceError, TraceMessage.c_str());
    WindowMode();

    HWND Parent = NULL;
    if (m_hWnd)
    {
        Parent = reinterpret_cast<HWND>(m_hWnd->GetWindowHandle());
    }
    MessageBoxW(Parent, Message, GS(MSG_MSGBOX_TITLE), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

void CNotification::DisplayMessage(int DisplayTime, LanguageStringID StringID) const
{
    DisplayMessage(DisplayTime, g_Lang->GetString(StringID).c_str());
}

void CNotification::DisplayMessage(int DisplayTime, const wchar_t * Message) const
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

    if (InFullScreen())
    {
        if (m_gfxPlugin && m_gfxPlugin->DrawStatus)
        {
            WriteTrace(TraceGfxPlugin, __FUNCTION__ ": DrawStatus - Starting");
            stdstr PluginMessage;
            PluginMessage.FromUTF16(Message);
            m_gfxPlugin->DrawStatus(PluginMessage.c_str(), FALSE);
            WriteTrace(TraceGfxPlugin, __FUNCTION__ ": DrawStatus - Done");
        }
    }
    else
    {
        m_hWnd->SetStatusText(0, Message);
    }
}

void CNotification::DisplayMessage2(const wchar_t * Message) const
{
    if (!m_hWnd) { return; }

    m_hWnd->SetStatusText(1, Message);
}

void CNotification::SetGfxPlugin(CGfxPlugin * Plugin)
{
    m_gfxPlugin = Plugin;
}

void CNotification::SetWindowCaption(const wchar_t * Caption)
{
    static const size_t TITLE_SIZE = 256;
    wchar_t WinTitle[TITLE_SIZE];

    _snwprintf(WinTitle, TITLE_SIZE, L"%s - %s", Caption, g_Settings->LoadStringVal(Setting_ApplicationName).ToUTF16().c_str());
    WinTitle[TITLE_SIZE - 1] = 0;
#if defined(WINDOWS_UI)
    m_hWnd->Caption(WinTitle);
#else
    g_Notify->BreakPoint(__FILEW__, __LINE__);
#endif
}

void CNotification::FatalError(LanguageStringID StringID) const
{
    FatalError(g_Lang->GetString(StringID).c_str());
}

void CNotification::FatalError(const wchar_t * Message) const
{
    WindowMode();

    HWND Parent = NULL;
    if (m_hWnd) { Parent = reinterpret_cast<HWND>(m_hWnd->GetWindowHandle()); }
    MessageBoxW(Parent, Message, L"Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    ExitThread(0);
}

void CNotification::AddRecentDir(const char * RomDir)
{
    //Validate the passed string
    if (HIWORD(RomDir) == NULL) { return; }

    //Get Information about the stored rom list
    size_t MaxRememberedDirs = g_Settings->LoadDword(Directory_RecentGameDirCount);
    strlist RecentDirs;
    size_t i;
    for (i = 0; i < MaxRememberedDirs; i++)
    {
        stdstr RecentDir = g_Settings->LoadStringIndex(Directory_RecentGameDirIndex, i);
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
        if (_stricmp(RomDir, iter->c_str()) != 0)
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
        g_Settings->SaveStringIndex(Directory_RecentGameDirIndex, i, *iter);
    }
}

void CNotification::AddRecentRom(const char * ImagePath)
{
    if (HIWORD(ImagePath) == NULL) { return; }

    //Get Information about the stored rom list
    size_t MaxRememberedFiles = g_Settings->LoadDword(File_RecentGameFileCount);
    strlist RecentGames;
    size_t i;
    for (i = 0; i < MaxRememberedFiles; i++)
    {
        stdstr RecentGame = g_Settings->LoadStringIndex(File_RecentGameFileIndex, i);
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
        if (_stricmp(ImagePath, iter->c_str()) != 0)
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
        g_Settings->SaveStringIndex(File_RecentGameFileIndex, i, *iter);
    }
}

void CNotification::RefreshMenu(void)
{
    if (m_hWnd == NULL) { return; }

#if defined(WINDOWS_UI)
    m_hWnd->RefreshMenu();
#else
    g_Notify->BreakPoint(__FILEW__, __LINE__);
#endif
}

void CNotification::HideRomBrowser(void)
{
    if (m_hWnd == NULL) { return; }
    m_hWnd->HideRomList();
}

void CNotification::ShowRomBrowser(void)
{
    if (m_hWnd == NULL) { return; }
    if (g_Settings->LoadDword(RomBrowser_Enabled))
    {
        //Display the rom browser
        m_hWnd->ShowRomList();
        m_hWnd->HighLightLastRom();
    }
}

void CNotification::BringToTop(void)
{
    if (m_hWnd == NULL) { return; }

#if defined(WINDOWS_UI)
    m_hWnd->BringToTop();
#else
    g_Notify->BreakPoint(__FILEW__, __LINE__);
#endif
}

void CNotification::MakeWindowOnTop(bool OnTop)
{
    if (m_hWnd == NULL) { return; }

#if defined(WINDOWS_UI)
    m_hWnd->MakeWindowOnTop(OnTop);
#else
    g_Notify->BreakPoint(__FILEW__, __LINE__);
#endif
}

void CNotification::ChangeFullScreen(void) const
{
    if (m_hWnd == NULL) { return; }
    SendMessage((HWND)(m_hWnd->GetWindowHandle()), WM_COMMAND, MAKELPARAM(ID_OPTIONS_FULLSCREEN2, false), 0);
}

bool CNotification::ProcessGuiMessages(void) const
{
    if (m_hWnd == NULL) { return false; }

    return m_hWnd->ProcessGuiMessages();
}

void CNotification::BreakPoint(const wchar_t * FileName, const int LineNumber)
{
    if (g_Settings->LoadBool(Debugger_Enabled))
    {
        DisplayError(stdstr_f("Break point found at\n%ws\n%d", FileName, LineNumber).ToUTF16().c_str());
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