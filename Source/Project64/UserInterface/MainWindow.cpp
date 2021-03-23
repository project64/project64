#include "stdafx.h"
#include "RomInformationClass.h"
#include <Project64\UserInterface\About.h>

#include <commctrl.h>
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
#include <Project64-core/N64System/Enhancement/Enhancements.h>
#include <Project64-core/N64System/N64DiskClass.h>
#include "DiscordRPC.h"

void EnterLogOptions(HWND hwndOwner);

#pragma comment(lib, "Comctl32.lib")

LRESULT CALLBACK MainGui_Proc(HWND WndHandle, DWORD uMsg, DWORD wParam, DWORD lParam);

CMainGui::CMainGui(bool bMainWindow, const char * WindowTitle) :
    CRomBrowser(m_hMainWindow, m_hStatusWnd),
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
    m_hStatusWnd = 0;
    m_SaveMainWindowPos = false;
    m_SaveMainWindowTop = 0;
    m_SaveMainWindowLeft = 0;

    m_SaveRomBrowserPos = false;
    m_SaveRomBrowserTop = 0;
    m_SaveRomBrowserLeft = 0;

    if (m_bMainWindow)
    {
        g_Settings->RegisterChangeCB((SettingID)RomBrowser_Enabled, this, (CSettings::SettingChangedFunc)RomBowserEnabledChanged);
        g_Settings->RegisterChangeCB((SettingID)RomBrowser_ColoumnsChanged, this, (CSettings::SettingChangedFunc)RomBowserColoumnsChanged);
		g_Settings->RegisterChangeCB((SettingID)Setting_EnableDiscordRPC, this, (CSettings::SettingChangedFunc)DiscordRPCChanged);
        g_Settings->RegisterChangeCB(RomList_GameDirRecursive, this, (CSettings::SettingChangedFunc)RomBrowserListChanged);
        g_Settings->RegisterChangeCB(RomList_ShowFileExtensions, this, (CSettings::SettingChangedFunc)RomBrowserListChanged);
        g_Settings->RegisterChangeCB(GameRunning_LoadingInProgress, this, (CSettings::SettingChangedFunc)LoadingInProgressChanged);
        g_Settings->RegisterChangeCB(GameRunning_CPU_Running, this, (CSettings::SettingChangedFunc)GameCpuRunning);
        g_Settings->RegisterChangeCB(GameRunning_CPU_Paused, this, (CSettings::SettingChangedFunc)GamePaused);
        g_Settings->RegisterChangeCB(Game_File, this, (CSettings::SettingChangedFunc)GameLoaded);
        g_Settings->RegisterChangeCB((SettingID)UserInterface_ShowStatusBar, this, (CSettings::SettingChangedFunc)ShowStatusBarChanged);

		if (UISettingsLoadBool(Setting_EnableDiscordRPC))
		{
			CDiscord::Init();
			CDiscord::Update(false);
		}
    }

    // If this fails then it has already been created
    RegisterWinClass();
    Create(WindowTitle);
}

CMainGui::~CMainGui(void)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (m_bMainWindow)
    {
        g_Settings->UnregisterChangeCB((SettingID)RomBrowser_Enabled, this, (CSettings::SettingChangedFunc)RomBowserEnabledChanged);
        g_Settings->UnregisterChangeCB((SettingID)RomBrowser_ColoumnsChanged, this, (CSettings::SettingChangedFunc)RomBowserColoumnsChanged);
		g_Settings->UnregisterChangeCB((SettingID)Setting_EnableDiscordRPC, this, (CSettings::SettingChangedFunc)DiscordRPCChanged);
		g_Settings->UnregisterChangeCB(RomList_GameDirRecursive, this, (CSettings::SettingChangedFunc)RomBrowserListChanged);
        g_Settings->UnregisterChangeCB(RomList_ShowFileExtensions, this, (CSettings::SettingChangedFunc)RomBrowserListChanged);
        g_Settings->UnregisterChangeCB(GameRunning_LoadingInProgress, this, (CSettings::SettingChangedFunc)LoadingInProgressChanged);
        g_Settings->UnregisterChangeCB(GameRunning_CPU_Running, this, (CSettings::SettingChangedFunc)GameCpuRunning);
        g_Settings->UnregisterChangeCB(GameRunning_CPU_Paused, this, (CSettings::SettingChangedFunc)GamePaused);
        g_Settings->UnregisterChangeCB(Game_File, this, (CSettings::SettingChangedFunc)GameLoaded);
        g_Settings->UnregisterChangeCB((SettingID)UserInterface_ShowStatusBar, this, (CSettings::SettingChangedFunc)ShowStatusBarChanged);

		if (UISettingsLoadBool(Setting_EnableDiscordRPC))
		{
			CDiscord::Shutdown();
		}
	}
    if (m_hMainWindow)
    {
        DestroyWindow(m_hMainWindow);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

bool CMainGui::RegisterWinClass(void)
{
    std::wstring VersionDisplay = stdstr_f("Project64 %s", VER_FILE_VERSION_STR).ToUTF16();

    WNDCLASS wcl;

    wcl.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcl.cbClsExtra = 0;
    wcl.cbWndExtra = 0;
    wcl.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PJ64_Icon));
    wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcl.hInstance = GetModuleHandle(NULL);

    wcl.lpfnWndProc = (WNDPROC)MainGui_Proc;
    wcl.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcl.lpszMenuName = NULL;
    wcl.lpszClassName = VersionDisplay.c_str();
    if (RegisterClass(&wcl) == 0) return false;
    return true;
}

void CMainGui::AddRecentRom(const char * ImagePath)
{
    if (HIWORD(ImagePath) == NULL) { return; }

    // Get information about the stored ROM list
    size_t MaxRememberedFiles = UISettingsLoadDword(File_RecentGameFileCount);
    strlist RecentGames;
    size_t i;
    for (i = 0; i < MaxRememberedFiles; i++)
    {
        stdstr RecentGame = UISettingsLoadStringIndex(File_RecentGameFileIndex, i);
        if (RecentGame.empty())
        {
            break;
        }
        RecentGames.push_back(RecentGame);
    }

    // See if the directory is already in the list, if so then move it to the top of the list
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
        UISettingsSaveStringIndex(File_RecentGameFileIndex, i, *iter);
    }
}

void CMainGui::SetWindowCaption(const wchar_t * title)
{
    static const size_t TITLE_SIZE = 256;
    wchar_t WinTitle[TITLE_SIZE];

    _snwprintf(WinTitle, TITLE_SIZE, L"%s - %s", title, stdstr(g_Settings->LoadStringVal(Setting_ApplicationName)).ToUTF16().c_str());
    WinTitle[TITLE_SIZE - 1] = 0;
    Caption(WinTitle);
}

void CMainGui::ShowRomBrowser(void)
{
    if (UISettingsLoadBool(RomBrowser_Enabled))
    {
        ShowRomList();
        HighLightLastRom();
    }
}

void RomBowserEnabledChanged(CMainGui * Gui)
{
    if (Gui && UISettingsLoadBool(RomBrowser_Enabled))
    {
        if (!Gui->RomBrowserVisible())
        {
            Gui->ShowRomList();
        }
    }
    else
    {
        if (Gui->RomBrowserVisible())
        {
            Gui->HideRomList();
        }
    }
}

void CMainGui::LoadingInProgressChanged(CMainGui * Gui)
{
    Gui->RefreshMenu();
    if (!g_Settings->LoadBool(GameRunning_LoadingInProgress) && g_Settings->LoadStringVal(Game_File).length() == 0)
    {
        Notify().WindowMode();
        if (UISettingsLoadBool(RomBrowser_Enabled))
        {
            Gui->ShowRomBrowser();
        }
        Gui->MakeWindowOnTop(false);
    }
}

void CMainGui::GameLoaded(CMainGui * Gui)
{
    stdstr FileLoc = g_Settings->LoadStringVal(Game_File);
    if (FileLoc.length() > 0)
    {
        WriteTrace(TraceUserInterface, TraceDebug, "Add Recent ROM");
        Gui->AddRecentRom(FileLoc.c_str());
        Gui->SetWindowCaption(stdstr(g_Settings->LoadStringVal(Rdb_GoodName)).ToUTF16().c_str());
		
		if (UISettingsLoadBool(Setting_EnableDiscordRPC))
		{
			CDiscord::Update();
		}
	}
}

void CMainGui::GamePaused(CMainGui * Gui)
{
    Gui->RefreshMenu();
}

void CMainGui::GameCpuRunning(CMainGui * Gui)
{
    if (g_Settings->LoadBool(GameRunning_CPU_Running))
    {
        Gui->MakeWindowOnTop(UISettingsLoadBool(UserInterface_AlwaysOnTop));
        Gui->HideRomList();
        if (UISettingsLoadBool(Setting_AutoFullscreen))
        {
            WriteTrace(TraceUserInterface, TraceDebug, "15");
            CIniFile RomIniFile(g_Settings->LoadStringVal(SupportFile_RomDatabase).c_str());
            std::string Status = UISettingsLoadStringVal(Rdb_Status);

            char String[100];
            RomIniFile.GetString("ROM Status", stdstr_f("%s.AutoFullScreen", Status.c_str()).c_str(), "true", String, sizeof(String));
            if (_stricmp(String, "true") == 0)
            {
                g_Notify->ChangeFullScreen();
            }
        }
        Gui->RefreshMenu();
        Gui->BringToTop();
    }
    else
    {
        if (Gui->m_CheatsUI.m_hWnd != NULL)
        {
            Gui->m_CheatsUI.SendMessage(WM_COMMAND, MAKELONG(IDCANCEL, 0));
        }
        PostMessage(Gui->m_hMainWindow, WM_GAME_CLOSED, 0, 0);
    }
}

void CMainGui::ShowStatusBarChanged(CMainGui * Gui)
{
    if (!Gui->bCPURunning())
    {
        ShowWindow(Gui->m_hStatusWnd, g_Settings->LoadBool((SettingID)UserInterface_ShowStatusBar) ? SW_SHOW : SW_HIDE);
        
        RECT rc;
        GetClientRect(Gui->m_hMainWindow, &rc);
        Gui->ResizeRomList((WORD)(rc.right - rc.left), (WORD)(rc.bottom - rc.top));
    }
}

void RomBowserColoumnsChanged(CMainGui * Gui)
{
    Gui->ResetRomBrowserColomuns();
}

void RomBrowserListChanged(CMainGui * Gui)
{
    Gui->RefreshRomList();
    Gui->HighLightLastRom();
}

void DiscordRPCChanged(CMainGui*)
{
	if (UISettingsLoadBool(Setting_EnableDiscordRPC))
	{
		CDiscord::Init();
		CDiscord::Update();
	}
	else
	{
		CDiscord::Shutdown();
	}
}

void CMainGui::ChangeWinSize(long width, long height)
{
    CGuard Guard(m_CS);
    WINDOWPLACEMENT wndpl;
    RECT rc1, swrect;

    wndpl.length = sizeof(wndpl);
    GetWindowPlacement(m_hMainWindow, &wndpl);

    if (m_hStatusWnd != nullptr && IsWindowVisible(m_hStatusWnd))
    {
        GetClientRect(m_hStatusWnd, &swrect);
        SetRect(&rc1, 0, 0, width, height + swrect.bottom);
    }
    else
    {
        SetRect(&rc1, 0, 0, width, height);
    }

    AdjustWindowRectEx(&rc1, GetWindowLong(m_hMainWindow, GWL_STYLE), GetMenu(m_hMainWindow) != NULL, GetWindowLong(m_hMainWindow, GWL_EXSTYLE));

    MoveWindow(m_hMainWindow, wndpl.rcNormalPosition.left, wndpl.rcNormalPosition.top, rc1.right - rc1.left, rc1.bottom - rc1.top, TRUE);
}

void * CMainGui::GetStatusBar(void) const
{
    return g_Settings->LoadBool((SettingID)UserInterface_ShowStatusBar) ? m_hStatusWnd : nullptr;
}

void * CMainGui::GetModuleInstance(void) const
{
    return GetModuleHandle(NULL);
}

bool CMainGui::ResetPluginsInUiThread(CPlugins * plugins, CN64System * System)
{
    RESET_PLUGIN info;
    info.system = System;
    info.plugins = plugins;
    info.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    bool bRes = true;
    if (info.hEvent)
    {
        PostMessage(m_hMainWindow, WM_RESET_PLUGIN, (WPARAM)&bRes, (LPARAM)&info);
#ifdef _DEBUG
        DWORD dwRes = WaitForSingleObject(info.hEvent, INFINITE);
#else
        DWORD dwRes = WaitForSingleObject(info.hEvent, 5000);
#endif
        dwRes = dwRes;
        CloseHandle(info.hEvent);
    }
    else
    {
        WriteTrace(TraceUserInterface, TraceError, "Failed to create event");
        bRes = false;
    }
    Notify().RefreshMenu();
    return bRes;
}

void CMainGui::DisplayCheatsUI(bool BlockExecution)
{
    m_CheatsUI.Display(m_hMainWindow, BlockExecution);
}

void CMainGui::DisplayEnhancements(bool BlockExecution)
{
    m_EnhancementUI.Display(m_hMainWindow, BlockExecution);
}

void CMainGui::BringToTop(void)
{
    CGuard Guard(m_CS);
    SetForegroundWindow(m_hMainWindow);
    SetFocus(GetDesktopWindow());
    Sleep(100);
    SetFocus(m_hMainWindow);
}

void CMainGui::MakeWindowOnTop(bool OnTop)
{
    CGuard Guard(m_CS);
    SetWindowPos(m_hMainWindow, OnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
}

void CMainGui::Caption(LPCWSTR Caption)
{
    CGuard Guard(m_CS);
    SetWindowText(m_hMainWindow, Caption);
}

void CMainGui::Create(const char * WindowTitle)
{
    stdstr_f VersionDisplay("Project64 %s", VER_FILE_VERSION_STR);
    m_hMainWindow = CreateWindowEx(WS_EX_ACCEPTFILES, VersionDisplay.ToUTF16().c_str(), stdstr(WindowTitle).ToUTF16().c_str(), WS_OVERLAPPED | WS_CLIPCHILDREN |
        WS_CLIPSIBLINGS | WS_SYSMENU | WS_MINIMIZEBOX, 5, 5, 640, 480,
        NULL, NULL, GetModuleHandle(NULL), this);
    m_Created = m_hMainWindow != NULL;
}

void CMainGui::CreateStatusBar(void)
{
    m_hStatusWnd = (HWND)CreateStatusWindow(WS_CHILD | WS_VISIBLE, L"", m_hMainWindow, StatusBarID);
    SendMessage((HWND)m_hStatusWnd, SB_SETTEXT, 0, (LPARAM)"");
    ShowWindow(m_hStatusWnd, g_Settings->LoadBool((SettingID)UserInterface_ShowStatusBar) ? SW_SHOW : SW_HIDE);
}

WPARAM CMainGui::ProcessAllMessages(void)
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (m_ResetPlugins)
        {
            m_ResetPlugins = false;
            m_ResetInfo->res = m_ResetInfo->plugins->Reset(m_ResetInfo->system);
            SetEvent(m_ResetInfo->hEvent);
            m_ResetInfo = NULL;
        }
        if ((m_CheatsUI.m_hWnd != NULL && IsDialogMessage(m_CheatsUI.m_hWnd, &msg)) ||
            (m_EnhancementUI.m_hWnd != NULL && IsDialogMessage(m_EnhancementUI.m_hWnd, &msg)))
        {
            continue;
        }
        if (m_Menu->ProcessAccelerator(m_hMainWindow, &msg)) { continue; }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

bool CMainGui::ProcessGuiMessages(void)
{
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
    {
        if (m_ResetPlugins)
        {
            m_ResetPlugins = false;
        }
        if (msg.message == WM_QUIT)
        {
            return true;
        }
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (m_Menu->ProcessAccelerator(m_hMainWindow, &msg)) { continue; }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return false;
}

void CMainGui::Resize(DWORD /*fwSizeType*/, WORD nWidth, WORD nHeight)
{
    RECT clrect, swrect;
    GetClientRect(m_hMainWindow, &clrect);
    GetClientRect(m_hStatusWnd, &swrect);

    int Parts[2];
    Parts[0] = (int) (nWidth - 135 * DPIScale(m_hStatusWnd));
    Parts[1] = nWidth;

    SendMessage(m_hStatusWnd, SB_SETPARTS, 2, (LPARAM)&Parts[0]);
    MoveWindow(m_hStatusWnd, 0, clrect.bottom - swrect.bottom, nWidth, nHeight, TRUE);
}

void CMainGui::Show(bool Visible)
{
    m_MakingVisible = true;

    CGuard Guard(m_CS);
    if (m_hMainWindow)
    {
        ShowWindow(m_hMainWindow, Visible ? SW_SHOW : SW_HIDE);
        if (Visible && RomBrowserVisible())
        {
            RomBrowserToTop();
        }
    }

    m_MakingVisible = false;
}

void CMainGui::EnterLogOptions(void)
{
    ::EnterLogOptions(m_hMainWindow);
}

int CMainGui::Height(void)
{
    if (!m_hMainWindow) { return 0; }

    RECT rect;
    GetWindowRect(m_hMainWindow, &rect);
    return rect.bottom - rect.top;
}

int CMainGui::Width(void)
{
    if (!m_hMainWindow) { return 0; }

    RECT rect;
    GetWindowRect(m_hMainWindow, &rect);
    return rect.right - rect.left;
}

float CMainGui::DPIScale(HWND hWnd) 
{
    return CClientDC(hWnd).GetDeviceCaps(LOGPIXELSX) / 96.0f;
}

void CMainGui::SetPos(int X, int Y)
{
    SetWindowPos(m_hMainWindow, NULL, X, Y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void CMainGui::SetWindowMenu(CBaseMenu * Menu)
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
        SetMenu(m_hMainWindow, hMenu);
    }

    m_AttachingMenu = false;
}

void CMainGui::RefreshMenu(void)
{
    if (!m_Menu) { return; }
    m_Menu->ResetMenu();
}

void CMainGui::SetStatusText(int Panel, const wchar_t * Text)
{
    static wchar_t Message[2][500];
    if (Panel >= 2)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    wchar_t * Msg = Message[Panel];

    memset(Msg, 0, sizeof(Message[0]));
    _snwprintf(Msg, sizeof(Message[0]) / sizeof(Message[0][0]), L"%s", Text);
    Msg[(sizeof(Message[0]) / sizeof(Message[0][0])) - 1] = 0;
    if (GetCurrentThreadId() == m_ThreadId)
    {
        SendMessage(m_hStatusWnd, SB_SETTEXT, Panel, (LPARAM)Msg);
    }
    else 
    {
        PostMessage(m_hStatusWnd, SB_SETTEXT, Panel, (LPARAM)Msg);
    }
}

void CMainGui::ShowStatusBar(bool ShowBar)
{
    ShowWindow(m_hStatusWnd, ShowBar ? SW_SHOW : SW_HIDE);
}

void CMainGui::SaveWindowLoc(void)
{
    bool flush = false;
    if (m_SaveMainWindowPos)
    {
        m_SaveMainWindowPos = false;
        UISettingsSaveDword(UserInterface_MainWindowTop, m_SaveMainWindowTop);
        UISettingsSaveDword(UserInterface_MainWindowLeft, m_SaveMainWindowLeft);
        flush = true;
    }

    if (m_SaveRomBrowserPos)
    {
        m_SaveRomBrowserPos = false;
        UISettingsSaveDword(RomBrowser_Top, m_SaveRomBrowserTop);
        UISettingsSaveDword(RomBrowser_Left, m_SaveRomBrowserLeft);
        flush = true;
    }

    if (flush)
    {
        CSettingTypeApplication::Flush();
    }
}

LRESULT CALLBACK CMainGui::MainGui_Proc(HWND hWnd, DWORD uMsg, DWORD wParam, DWORD lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        {
            // Record class for future usage
            LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
            CMainGui * _this = (CMainGui *)lpcs->lpCreateParams;
            SetProp(hWnd, L"Class", _this);

            _this->m_hMainWindow = hWnd;
            _this->CreateStatusBar();

            // Move the main window to the location last executed from or center the window
            int X = (GetSystemMetrics(SM_CXSCREEN) - _this->Width()) / 2;
            int	Y = (GetSystemMetrics(SM_CYSCREEN) - _this->Height()) / 2;

            UISettingsLoadDword(UserInterface_MainWindowTop, (uint32_t &)Y);
            UISettingsLoadDword(UserInterface_MainWindowLeft, (uint32_t &)X);

            _this->SetPos(X, Y);

            _this->ChangeWinSize(int (640 * _this->DPIScale(hWnd)), int (480 * _this->DPIScale(hWnd)));
        }
        break;
    case WM_SYSCOMMAND:
        switch (wParam) {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            {
                CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
                if (_this &&
                    _this->bCPURunning() &&
                    !g_Settings->LoadBool(GameRunning_CPU_Paused) &&
                    UISettingsLoadBool(Setting_DisableScrSaver))
                {
                    return 0;
                }
            }
            break;
        case SC_MAXIMIZE:
            {
                CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
                if (_this)
                {
                    if (_this->RomBrowserVisible())
                    {
                        _this->RomBrowserMaximize(true);
                    }
                }
            }
            break;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;
    case WM_MOVE:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");

            if (!_this->m_bMainWindow ||
                !_this->m_Created ||
                _this->m_AttachingMenu ||
                _this->m_MakingVisible ||
                IsIconic(hWnd) ||
                _this->ShowingRomBrowser())
            {
                break;
            }

            if (IsZoomed(hWnd))
            {
                if (_this->RomBrowserVisible())
                {
                    // Save that browser is maximized
                }
                break;
            }

            // Get the current position of the window
            RECT WinRect;
            GetWindowRect(hWnd, &WinRect);

            // Save the location of the window
            if (_this->RomBrowserVisible())
            {
                _this->m_SaveRomBrowserPos = true;
                _this->m_SaveRomBrowserTop = WinRect.top;
                _this->m_SaveRomBrowserLeft = WinRect.left;
            }
            else
            {
                _this->m_SaveMainWindowPos = true;
                _this->m_SaveMainWindowTop = WinRect.top;
                _this->m_SaveMainWindowLeft = WinRect.left;
            }
            KillTimer(hWnd, Timer_SetWindowPos);
            SetTimer(hWnd, Timer_SetWindowPos, 1000, NULL);
        }
        if (CGuiSettings::bCPURunning() && g_BaseSystem)
        {
            if (g_Plugins->Gfx() && g_Plugins->Gfx()->MoveScreen)
            {
                WriteTrace(TraceGFXPlugin, TraceDebug, "Starting");
                g_Plugins->Gfx()->MoveScreen((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
                WriteTrace(TraceGFXPlugin, TraceDebug, "Done");
            }
        }
        break;
    case WM_TIMER:
        if (wParam == Timer_SetWindowPos)
        {
            KillTimer(hWnd, Timer_SetWindowPos);
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            _this->SaveWindowLoc();
            break;
        }
        break;
    case WM_SIZE:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            if (_this) { _this->Resize(wParam, LOWORD(lParam), HIWORD(lParam)); }
            if (_this)
            {
                if (wParam == SIZE_MAXIMIZED)
                {
                    if (_this->RomBrowserVisible())
                    {
                        _this->RomBrowserMaximize(true);
                    }
                }
                _this->ResizeRomList(LOWORD(lParam), HIWORD(lParam));
            }
            if (_this)
            {
                if (wParam == SIZE_RESTORED && _this->RomBrowserVisible())
                {
                    _this->RomBrowserMaximize(false);
                }
            }
        }
        break;
    case WM_NOTIFY:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            if (_this == NULL || !_this->RomBrowserVisible() || !_this->RomListNotify(wParam, lParam))
            {
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
        }
        break;
    case WM_DRAWITEM:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            if (_this)
            {
                if (!_this->RomListDrawItem(wParam, lParam))
                {
                    return DefWindowProc(hWnd, uMsg, wParam, lParam);
                }
            }
        }
        break;
    case WM_PAINT:
        {
            //			CMainGui * _this = (CMainGui *)GetProp(hWnd,"Class");
            //			CN64System * System  = _this->m_System;

            //			if (bCPURunning() && Settings->Load(CPU_Paused)) {
            //				CPlugins * Plugins = System->Plugins();
            //				if (Plugins->Gfx()->DrawScreen) {
            //					Plugins->Gfx()->DrawScreen();
            //				}
            //			}
            ValidateRect(hWnd, NULL);
        }
        break;
    case WM_KEYUP:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");

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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");

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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            if (_this->RomBrowserVisible())
            {
                PostMessage(hWnd, WM_BROWSER_TOP, 0, 0);
                break;
            }

            if (_this->m_bMainWindow && bCPURunning() && bAutoSleep())
            {
                if (g_BaseSystem)
                {
                    g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_AppGainedFocus);
                }
            }
        }
        break;
    case WM_KILLFOCUS:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            if (_this->RomBrowserVisible())
            {
                break;
            }

            if (_this->m_bMainWindow && bCPURunning() && bAutoSleep())
            {
                if (g_BaseSystem)
                {
                    g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_AppLostFocus);
                }
            }
        }
        break;
    case WM_ACTIVATEAPP:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            DWORD fActive = (BOOL)wParam;

            if (fActive && _this->RomBrowserVisible())
            {
                PostMessage(hWnd, WM_BROWSER_TOP, 0, 0);
            }
            if (_this->m_bMainWindow && bCPURunning())
            {
                if (!fActive && UISettingsLoadBool(UserInterface_InFullScreen) && UISettingsLoadBool(UserInterface_ExitFullscreenOnLoseFocus))
                {
                    Notify().WindowMode();
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
                        g_BaseSystem->ExternalEvent(fActive ? SysEvent_ResumeCPU_AppGainedActive : SysEvent_PauseCPU_AppLostActive);
                    }
                }
            }
        }
        break;
    case WM_HIDE_CUROSR:
        if (!wParam)
        {
            while (ShowCursor(FALSE) >= 0) { Sleep(0); }
        }
        else
        {
            while (ShowCursor(TRUE) < 0) { Sleep(0); }
        }
        break;
    case WM_MAKE_FOCUS:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            _this->BringToTop();
        }
        break;
    case WM_BROWSER_TOP:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            _this->RomBrowserToTop();
        }
        break;
    case WM_RESET_PLUGIN:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            if (_this->m_ResetInfo != NULL)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            _this->m_ResetInfo = (RESET_PLUGIN *)lParam;
            _this->m_ResetPlugins = true;
        }
        break;
    case WM_GAME_CLOSED:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            Notify().WindowMode();
            if (UISettingsLoadBool(RomBrowser_Enabled))
            {
                _this->ShowRomBrowser();
            }
            _this->RefreshMenu();
            _this->MakeWindowOnTop(false);
            _this->SetStatusText(0, L"");
            _this->SetStatusText(1, L"");
        }
        break;
    case WM_COMMAND:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, L"Class");
            if (_this == NULL) { break; }

            switch (LOWORD(wParam)) {
            case ID_POPUPMENU_PLAYGAME: 
                {
                    if ((CPath(_this->CurrentedSelectedRom()).GetExtension() != "ndd") && (CPath(_this->CurrentedSelectedRom()).GetExtension() != "d64"))
                    {
                        g_BaseSystem->RunFileImage(_this->CurrentedSelectedRom());
                    }
                    else
                    {
                        g_BaseSystem->RunDiskImage(_this->CurrentedSelectedRom());
                    }
                    break;
                }
            case ID_POPUPMENU_PLAYGAMEWITHDISK:
                {
                    CPath FileName;
                    const char * Filter = "N64DD Disk Image (*.ndd, *.d64)\0*.ndd;*.d64\0All files (*.*)\0*.*\0";
                    if (FileName.SelectFile(hWnd, g_Settings->LoadStringVal(RomList_GameDir).c_str(), Filter, true))
                    {
                        g_BaseSystem->RunDiskComboImage(_this->CurrentedSelectedRom(), FileName);
                    }
                }
                break;
            case ID_POPUPMENU_ROMDIRECTORY:   _this->SelectRomDir(); break;
            case ID_POPUPMENU_REFRESHROMLIST: _this->RefreshRomList(); break;
            case ID_POPUPMENU_ROMINFORMATION:
                {
                    RomInformation Info(_this->CurrentedSelectedRom());
                    Info.DisplayInformation(hWnd);
                }
                break;
            case ID_POPUPMENU_EDITSETTINGS:
            case ID_POPUPMENU_EDITCHEATS:
            case ID_POPUPMENU_CHOOSEENHANCEMENT:
                {
                    if ((CPath(_this->CurrentedSelectedRom()).GetExtension() != "ndd") && (CPath(_this->CurrentedSelectedRom()).GetExtension() != "d64"))
                    {
                        CN64Rom Rom;
                        Rom.LoadN64Image(_this->CurrentedSelectedRom(), true);
                        Rom.SaveRomSettingID(true);

                        if (LOWORD(wParam) == ID_POPUPMENU_EDITSETTINGS)
                        {
                            CSettingConfig SettingConfig(true);
                            SettingConfig.Display(hWnd);
                        }
                        else if (LOWORD(wParam) == ID_POPUPMENU_EDITCHEATS)
                        {
                            g_Enhancements->ResetActive(nullptr);
                            g_Enhancements->LoadActive(nullptr, nullptr);
                            _this->m_CheatsUI.Display(hWnd, true);
                        }
                        else if (LOWORD(wParam) == ID_POPUPMENU_CHOOSEENHANCEMENT)
                        {
                            g_Enhancements->LoadActive(nullptr, nullptr);
                            _this->m_EnhancementUI.Display(hWnd, true);
                        }

                        if (g_Rom)
                        {
                            g_Rom->SaveRomSettingID(false);
                        }
                        else
                        {
                            Rom.ClearRomSettingID();
                        }
                    }
                    else
                    {
                        CN64Disk Disk;
                        Disk.LoadDiskImage(_this->CurrentedSelectedRom());
                        Disk.SaveDiskSettingID(true);

                        if (LOWORD(wParam) == ID_POPUPMENU_EDITSETTINGS)
                        {
                            CSettingConfig SettingConfig(true);
                            SettingConfig.Display(hWnd);
                        }
                        else if (LOWORD(wParam) == ID_POPUPMENU_EDITCHEATS)
                        {
                            CCheatsUI().Display(hWnd,true);
                        }

                        if (g_Disk)
                        {
                            g_Disk->SaveDiskSettingID(false);
                        }
                        else
                        {
                            Disk.ClearDiskSettingID();
                        }
                    }
                }
                break;
            default:
                if (_this->m_Menu)
                {
                    if (LOWORD(wParam) > 5000 && LOWORD(wParam) <= 5100)
                    {
                        if (g_Plugins->RSP())
                        {
                            g_Plugins->RSP()->ProcessMenuItem(LOWORD(wParam));
                        }
                    }
                    else if (LOWORD(wParam) > 5100 && LOWORD(wParam) <= 5200)
                    {
                        if (g_Plugins->Gfx())
                        {
                            WriteTrace(TraceGFXPlugin, TraceDebug, "Starting");
                            g_Plugins->Gfx()->ProcessMenuItem(LOWORD(wParam));
                            WriteTrace(TraceGFXPlugin, TraceDebug, "Done");
                        }
                    }
                    else if (LOWORD(wParam) > 5200 && LOWORD(wParam) <= 5300)
                    {
                        if (g_Plugins->Gfx() && g_Plugins->Gfx()->OnRomBrowserMenuItem != NULL)
                        {
                            CN64Rom Rom;
                            if (!Rom.LoadN64Image(_this->CurrentedSelectedRom(), true))
                            {
                                break;
                            }
                            Rom.SaveRomSettingID(true);
                            g_Notify->DisplayMessage(0, "");
                            BYTE * RomHeader = Rom.GetRomAddress();
                            WriteTrace(TraceGFXPlugin, TraceDebug, "OnRomBrowserMenuItem - Starting");
                            g_Plugins->Gfx()->OnRomBrowserMenuItem(LOWORD(wParam), hWnd, RomHeader);
                            WriteTrace(TraceGFXPlugin, TraceDebug, "OnRomBrowserMenuItem - Done");
                            if (g_Rom)
                            {
                                g_Rom->SaveRomSettingID(false);
                            }
                            else
                            {
                                g_Settings->SaveString(Game_IniKey, "");
                            }
                        }
                    }
                    else if (_this->m_Menu->ProcessMessage(hWnd, HIWORD(wParam), LOWORD(wParam)))
                    {
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
            DragQueryFileA(hDrop, 0, filename, sizeof(filename));
            DragFinish(hDrop);

            stdstr ext = CPath(filename).GetExtension();
            if ((!(_stricmp(ext.c_str(), "ndd") == 0)) && (!(_stricmp(ext.c_str(), "d64") == 0)))
            {
                CN64System::RunFileImage(filename);
            }
            else
            {
                CN64System::RunDiskImage(filename);
            }
        }
        break;
    case WM_DESTROY:
        WriteTrace(TraceUserInterface, TraceDebug, "WM_DESTROY - start");
        {
            CMainGui   * _this = (CMainGui *)GetProp(hWnd, L"Class");
            if (_this->m_bMainWindow)
            {
                Notify().WindowMode();
            }
            _this->m_hMainWindow = NULL;
            WriteTrace(TraceUserInterface, TraceDebug, "WM_DESTROY - 1");
            if (_this->m_bMainWindow)
            {
                _this->SaveRomListColoumnInfo();
                WriteTrace(TraceUserInterface, TraceDebug, "WM_DESTROY - 2");
                _this->SaveWindowLoc();
            }
        }
        WriteTrace(TraceUserInterface, TraceDebug, "WM_DESTROY - 3");
        RemoveProp(hWnd, L"Class");
        WriteTrace(TraceUserInterface, TraceDebug, "WM_DESTROY - 4");
        PostQuitMessage(0);
        WriteTrace(TraceUserInterface, TraceDebug, "WM_DESTROY - Done");
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return TRUE;
}
