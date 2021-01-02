#include "stdafx.h"
#include <time.h>

CNotificationImp & Notify(void)
{
    static CNotificationImp Notify;
    return Notify;
}

CNotificationImp::CNotificationImp() :
    m_hWnd(NULL),
    m_gfxPlugin(NULL),
    m_NextMsg(0)
{
    _tzset();
}

void CNotificationImp::AppInitDone(void)
{
    RegisterUISettings();
    CNotificationSettings::RegisterNotifications();
}

void CNotificationImp::SetMainWindow(CMainGui * Gui)
{
    m_hWnd = Gui;
}

void CNotificationImp::WindowMode(void) const
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

void CNotificationImp::DisplayWarning(const char * Message) const
{
    HWND Parent = NULL;
    if (m_hWnd)
    {
        Parent = reinterpret_cast<HWND>(m_hWnd->GetWindowHandle());
    }
    MessageBox(Parent, stdstr(Message).ToUTF16().c_str(), wGS(MSG_MSGBOX_WARNING_TITLE).c_str(), MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
}

void CNotificationImp::DisplayWarning(LanguageStringID StringID) const
{
    DisplayWarning(g_Lang->GetString(StringID).c_str());
}

void CNotificationImp::DisplayError(LanguageStringID StringID) const
{
    DisplayError(g_Lang->GetString(StringID).c_str());
}

void CNotificationImp::DisplayError(const char * Message) const
{
    WriteTrace(TraceUserInterface, TraceError, Message);
    WindowMode();

    HWND Parent = NULL;
    if (m_hWnd)
    {
        Parent = reinterpret_cast<HWND>(m_hWnd->GetWindowHandle());
    }
    MessageBox(Parent, stdstr(Message).ToUTF16().c_str(), wGS(MSG_MSGBOX_ERROR_TITLE).c_str(), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

void CNotificationImp::DisplayMessage(int DisplayTime, LanguageStringID StringID) const
{
    DisplayMessage(DisplayTime, g_Lang->GetString(StringID).c_str());
}

void CNotificationImp::DisplayMessage(int DisplayTime, const char * Message) const
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
            WriteTrace(TraceGFXPlugin, TraceDebug, "DrawStatus - Starting");
            m_gfxPlugin->DrawStatus(Message, FALSE);
            WriteTrace(TraceGFXPlugin, TraceDebug, "DrawStatus - Done");
        }
    }
    else
    {
        m_hWnd->SetStatusText(0, stdstr(Message).ToUTF16().c_str());
    }
}

void CNotificationImp::DisplayMessage2(const char * Message) const
{
    if (!m_hWnd) { return; }

    m_hWnd->SetStatusText(1, stdstr(Message).ToUTF16().c_str());
}

bool CNotificationImp::AskYesNoQuestion(const char * Question) const
{
    WriteTrace(TraceUserInterface, TraceError, Question);
    WindowMode();

    HWND Parent = NULL;
    if (m_hWnd)
    {
        Parent = reinterpret_cast<HWND>(m_hWnd->GetWindowHandle());
    }
    int result = MessageBox(Parent, stdstr(Question).ToUTF16().c_str(), wGS(MSG_MSGBOX_WARNING_TITLE).c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_SETFOREGROUND);
    return result == IDYES;
}

void CNotificationImp::SetGfxPlugin(CGfxPlugin * Plugin)
{
    m_gfxPlugin = Plugin;
}

void CNotificationImp::FatalError(LanguageStringID StringID) const
{
    FatalError(g_Lang->GetString(StringID).c_str());
}

void CNotificationImp::FatalError(const char  * Message) const
{
    WriteTrace(TraceUserInterface, TraceError, Message);
    WindowMode();

    HWND Parent = NULL;
    if (m_hWnd) { Parent = reinterpret_cast<HWND>(m_hWnd->GetWindowHandle()); }
    MessageBox(Parent, stdstr(Message).ToUTF16().c_str(), L"Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    ExitThread(0);
}

void CNotificationImp::AddRecentDir(const char * RomDir)
{
    //Validate the passed string
    if (HIWORD(RomDir) == NULL) { return; }

    //Get Information about the stored rom list
    size_t MaxRememberedDirs = UISettingsLoadDword(Directory_RecentGameDirCount);
    strlist RecentDirs;
    size_t i;
    for (i = 0; i < MaxRememberedDirs; i++)
    {
        stdstr RecentDir = UISettingsLoadStringIndex(Directory_RecentGameDirIndex, i);
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
        UISettingsSaveStringIndex(Directory_RecentGameDirIndex, i, *iter);
    }
}

void CNotificationImp::RefreshMenu(void)
{
    if (m_hWnd == NULL) { return; }

    m_hWnd->RefreshMenu();
}

void CNotificationImp::HideRomBrowser(void)
{
    if (m_hWnd == NULL) { return; }
    m_hWnd->HideRomList();
}

void CNotificationImp::ShowRomBrowser(void)
{
    if (m_hWnd == NULL) { return; }
    if (UISettingsLoadBool(RomBrowser_Enabled))
    {
        //Display the rom browser
        m_hWnd->ShowRomList();
        m_hWnd->HighLightLastRom();
    }
}

void CNotificationImp::BringToTop(void)
{
    if (m_hWnd == NULL) { return; }

    m_hWnd->BringToTop();
}

void CNotificationImp::ChangeFullScreen(void) const
{
    if (m_hWnd == NULL) { return; }
    SendMessage((HWND)(m_hWnd->GetWindowHandle()), WM_COMMAND, MAKELPARAM(ID_OPTIONS_FULLSCREEN2, false), 0);
}

bool CNotificationImp::ProcessGuiMessages(void) const
{
    if (m_hWnd == NULL) { return false; }

    return m_hWnd->ProcessGuiMessages();
}

void CNotificationImp::BreakPoint(const char * FileName, int LineNumber)
{
    if (g_Settings->LoadBool(Debugger_Enabled))
    {
        DisplayError(stdstr_f("Break point found at\n%s\n%d", FileName, LineNumber).c_str());
        if (IsDebuggerPresent() != 0)
        {
            DebugBreak();
        }
        else
        {
            if (g_BaseSystem)
            {
                g_BaseSystem->CloseCpu();
            }
        }
    }
    else
    {
        DisplayError("Fatal Error: Stopping emulation");
        if (g_BaseSystem)
        {
            g_BaseSystem->CloseCpu();
        }
    }
}