/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include "RomInformationClass.h"

#include <commctrl.h>
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
#include <Project64-core/N64System/N64DiskClass.h>

void EnterLogOptions(HWND hwndOwner);

#pragma comment(lib, "Comctl32.lib")

DWORD CALLBACK AboutBoxProc(HWND WndHandle, DWORD uMsg, DWORD wParam, DWORD lParam);
LRESULT CALLBACK MainGui_Proc(HWND WndHandle, DWORD uMsg, DWORD wParam, DWORD lParam);

extern BOOL set_about_field(HWND hDlg, int nIDDlgItem, const wchar_t * config_string, const wchar_t * language_string);

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
        g_Settings->RegisterChangeCB((SettingID)(FirstUISettings + RomBrowser_Enabled), this, (CSettings::SettingChangedFunc)RomBowserEnabledChanged);
        g_Settings->RegisterChangeCB((SettingID)(FirstUISettings + RomBrowser_ColoumnsChanged), this, (CSettings::SettingChangedFunc)RomBowserColoumnsChanged);
        g_Settings->RegisterChangeCB(RomList_GameDirRecursive, this, (CSettings::SettingChangedFunc)RomBrowserListChanged);
        g_Settings->RegisterChangeCB(RomList_ShowFileExtensions, this, (CSettings::SettingChangedFunc)RomBrowserListChanged);
        g_Settings->RegisterChangeCB(GameRunning_LoadingInProgress, this, (CSettings::SettingChangedFunc)LoadingInProgressChanged);
        g_Settings->RegisterChangeCB(GameRunning_CPU_Running, this, (CSettings::SettingChangedFunc)GameCpuRunning);
        g_Settings->RegisterChangeCB(GameRunning_CPU_Paused, this, (CSettings::SettingChangedFunc)GamePaused);
        g_Settings->RegisterChangeCB(Game_File, this, (CSettings::SettingChangedFunc)GameLoaded);
    }

    //if this fails then it has already been created
    RegisterWinClass();
    Create(WindowTitle);
}

CMainGui::~CMainGui(void)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (m_bMainWindow)
    {
        g_Settings->UnregisterChangeCB((SettingID)(FirstUISettings + RomBrowser_Enabled), this, (CSettings::SettingChangedFunc)RomBowserEnabledChanged);
        g_Settings->UnregisterChangeCB((SettingID)(FirstUISettings + RomBrowser_ColoumnsChanged), this, (CSettings::SettingChangedFunc)RomBowserColoumnsChanged);
        g_Settings->UnregisterChangeCB(RomList_GameDirRecursive, this, (CSettings::SettingChangedFunc)RomBrowserListChanged);
        g_Settings->UnregisterChangeCB(RomList_ShowFileExtensions, this, (CSettings::SettingChangedFunc)RomBrowserListChanged);
        g_Settings->UnregisterChangeCB(GameRunning_LoadingInProgress, this, (CSettings::SettingChangedFunc)LoadingInProgressChanged);
        g_Settings->UnregisterChangeCB(GameRunning_CPU_Running, this, (CSettings::SettingChangedFunc)GameCpuRunning);
        g_Settings->UnregisterChangeCB(GameRunning_CPU_Paused, this, (CSettings::SettingChangedFunc)GamePaused);
        g_Settings->UnregisterChangeCB(Game_File, this, (CSettings::SettingChangedFunc)GameLoaded);
    }
    if (m_hMainWindow)
    {
        DestroyWindow(m_hMainWindow);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

bool CMainGui::RegisterWinClass(void)
{
    stdstr_f VersionDisplay("Project64 %s", VER_FILE_VERSION_STR);

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

    //Get Information about the stored rom list
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
        WriteTrace(TraceUserInterface, TraceDebug, "Add Recent Rom");
        Gui->AddRecentRom(FileLoc.c_str());
        Gui->SetWindowCaption(stdstr(g_Settings->LoadStringVal(Rdb_GoodName)).ToUTF16().c_str());
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
            RomIniFile.GetString("Rom Status", stdstr_f("%s.AutoFullScreen", Status.c_str()).c_str(), "true", String, sizeof(String));
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
        PostMessage(Gui->m_hMainWindow, WM_GAME_CLOSED, 0, 0);
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

void CMainGui::ChangeWinSize(long width, long height)
{
    CGuard Guard(m_CS);
    WINDOWPLACEMENT wndpl;
    RECT rc1, swrect;

    wndpl.length = sizeof(wndpl);
    GetWindowPlacement(m_hMainWindow, &wndpl);

    if ((HWND)m_hStatusWnd != NULL)
    {
        GetClientRect((HWND)m_hStatusWnd, &swrect);
        SetRect(&rc1, 0, 0, width, height + swrect.bottom);
    }
    else
    {
        SetRect(&rc1, 0, 0, width, height);
    }

    AdjustWindowRectEx(&rc1, GetWindowLong(m_hMainWindow, GWL_STYLE), GetMenu(m_hMainWindow) != NULL, GetWindowLong(m_hMainWindow, GWL_EXSTYLE));

    MoveWindow(m_hMainWindow, wndpl.rcNormalPosition.left, wndpl.rcNormalPosition.top, rc1.right - rc1.left, rc1.bottom - rc1.top, TRUE);
}

void * CMainGui::GetModuleInstance(void) const
{
    return GetModuleHandle(NULL);
}

void CMainGui::AboutBox(void)
{
    DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_About), m_hMainWindow, (DLGPROC)AboutBoxProc, (LPARAM)this);
}

void CMainGui::AboutIniBox(void)
{
    DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_About_Ini), m_hMainWindow, (DLGPROC)AboutIniBoxProc, (LPARAM)this);
}

DWORD CALLBACK AboutIniBoxProc(HWND hDlg, DWORD uMsg, DWORD wParam, DWORD /*lParam*/)
{
    static wchar_t RDBHomePage[300], CHTHomePage[300], RDXHomePage[300];

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            wchar_t String[200];

            //Title
            SetWindowTextW(hDlg, wGS(INI_TITLE).c_str());

            //Language
            SetDlgItemTextW(hDlg, IDC_LAN, wGS(INI_CURRENT_LANG).c_str());
            set_about_field(hDlg, IDC_LAN_AUTHOR, wGS(INI_AUTHOR).c_str(), wGS(LANGUAGE_AUTHOR).c_str());
            set_about_field(hDlg, IDC_LAN_VERSION, wGS(INI_VERSION).c_str(), wGS(LANGUAGE_VERSION).c_str());
            set_about_field(hDlg, IDC_LAN_DATE, wGS(INI_DATE).c_str(), wGS(LANGUAGE_DATE).c_str());
            if (wcslen(wGS(LANGUAGE_NAME).c_str()) == 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_LAN), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_LAN_AUTHOR), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_LAN_VERSION), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_LAN_DATE), FALSE);
            }
            //RDB
            CIniFile RdbIniFile(g_Settings->LoadStringVal(SupportFile_RomDatabase).c_str());
            wcsncpy(String, stdstr(RdbIniFile.GetString("Meta", "Author", "")).ToUTF16().c_str(), sizeof(String) / sizeof(String[0]));
            if (wcslen(String) == 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_RDB), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RDB_AUTHOR), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RDB_VERSION), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RDB_DATE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RDB_HOME), FALSE);
            }

            set_about_field(hDlg, IDC_RDB_AUTHOR, wGS(INI_AUTHOR).c_str(), String);

            wcsncpy(String, stdstr(RdbIniFile.GetString("Meta", "Version", "")).ToUTF16().c_str(), sizeof(String) / sizeof(String[0]));
            set_about_field(hDlg, IDC_RDB_VERSION, wGS(INI_VERSION).c_str(), String);
            wcsncpy(String, stdstr(RdbIniFile.GetString("Meta", "Date", "")).ToUTF16().c_str(), sizeof(String) / sizeof(String[0]));
            set_about_field(hDlg, IDC_RDB_DATE, wGS(INI_DATE).c_str(), String);
            wcsncpy(RDBHomePage, stdstr(RdbIniFile.GetString("Meta", "Homepage", "")).ToUTF16().c_str(), sizeof(RDBHomePage) / sizeof(RDBHomePage[0]));
            SetDlgItemTextW(hDlg, IDC_RDB_HOME, wGS(INI_HOMEPAGE).c_str());
            if (wcslen(RDBHomePage) == 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_RDB_HOME), FALSE);
            }

            //Cheat
            SetDlgItemTextW(hDlg, IDC_CHT, wGS(INI_CURRENT_CHT).c_str());
            CIniFile CheatIniFile(g_Settings->LoadStringVal(SupportFile_Cheats).c_str());
            wcsncpy(String, stdstr(CheatIniFile.GetString("Meta", "Author", "")).ToUTF16().c_str(), sizeof(String) / sizeof(String[0]));
            if (wcslen(String) == 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_CHT), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHT_AUTHOR), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHT_VERSION), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHT_DATE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_CHT_HOME), FALSE);
            }
            set_about_field(hDlg, IDC_CHT_AUTHOR, wGS(INI_AUTHOR).c_str(), String);
            wcsncpy(String, stdstr(CheatIniFile.GetString("Meta", "Version", "")).ToUTF16().c_str(), sizeof(String) / sizeof(String[0]));
            set_about_field(hDlg, IDC_CHT_VERSION, wGS(INI_VERSION).c_str(), String);
            wcsncpy(String, stdstr(CheatIniFile.GetString("Meta", "Date", "")).ToUTF16().c_str(), sizeof(String) / sizeof(String[0]));
            set_about_field(hDlg, IDC_CHT_DATE, wGS(INI_DATE).c_str(), String);
            wcsncpy(CHTHomePage, stdstr(CheatIniFile.GetString("Meta", "Homepage", "")).ToUTF16().c_str(), sizeof(CHTHomePage) / sizeof(CHTHomePage[0]));
            SetDlgItemTextW(hDlg, IDC_CHT_HOME, wGS(INI_HOMEPAGE).c_str());
            if (wcslen(CHTHomePage) == 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_CHT_HOME), FALSE);
            }

            //Extended Info
            SetDlgItemTextW(hDlg, IDC_RDX, wGS(INI_CURRENT_RDX).c_str());
            CIniFile RdxIniFile(g_Settings->LoadStringVal(SupportFile_ExtInfo).c_str());
            wcsncpy(String, stdstr(RdxIniFile.GetString("Meta", "Author", "")).ToUTF16().c_str(), sizeof(String) / sizeof(String[0]));
            if (wcslen(String) == 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_RDX), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RDX_AUTHOR), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RDX_VERSION), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RDX_DATE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_RDX_HOME), FALSE);
            }
            set_about_field(hDlg, IDC_RDX_AUTHOR, wGS(INI_AUTHOR).c_str(), String);
            wcsncpy(String, stdstr(RdxIniFile.GetString("Meta", "Version", "")).ToUTF16().c_str(), sizeof(String) / sizeof(String[0]));
            set_about_field(hDlg, IDC_RDX_VERSION, wGS(INI_VERSION).c_str(), String);
            wcsncpy(String, stdstr(RdxIniFile.GetString("Meta", "Date", "")).ToUTF16().c_str(), sizeof(String) / sizeof(String[0]));
            set_about_field(hDlg, IDC_RDX_DATE, wGS(INI_DATE).c_str(), String);
            wcsncpy(RDXHomePage, stdstr(RdxIniFile.GetString("Meta", "Homepage", "")).ToUTF16().c_str(), sizeof(RDXHomePage) / sizeof(RDXHomePage[0]));
            SetDlgItemTextW(hDlg, IDC_RDX_HOME, wGS(INI_HOMEPAGE).c_str());
            if (wcslen(RDXHomePage) == 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_RDX_HOME), FALSE);
            }
            SetDlgItemTextW(hDlg, IDOK, wGS(CHEAT_OK).c_str());
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_RDB_HOME: ShellExecuteW(NULL, L"open", RDBHomePage, NULL, NULL, SW_SHOWNORMAL); break;
        case IDC_CHT_HOME: ShellExecuteW(NULL, L"open", CHTHomePage, NULL, NULL, SW_SHOWNORMAL); break;
        case IDC_RDX_HOME: ShellExecuteW(NULL, L"open", RDXHomePage, NULL, NULL, SW_SHOWNORMAL); break;
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;
        }
    default:
        return FALSE;
    }
    return TRUE;
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
    SetWindowTextW(m_hMainWindow, Caption);
}

void CMainGui::Create(const char * WindowTitle)
{
    stdstr_f VersionDisplay("Project64 %s", VER_FILE_VERSION_STR);
    m_hMainWindow = CreateWindowExW(WS_EX_ACCEPTFILES, VersionDisplay.ToUTF16().c_str(), stdstr(WindowTitle).ToUTF16().c_str(), WS_OVERLAPPED | WS_CLIPCHILDREN |
        WS_CLIPSIBLINGS | WS_SYSMENU | WS_MINIMIZEBOX, 5, 5, 640, 480,
        NULL, NULL, GetModuleHandle(NULL), this);
    m_Created = m_hMainWindow != NULL;
}

void CMainGui::CreateStatusBar(void)
{
    m_hStatusWnd = (HWND)CreateStatusWindow(WS_CHILD | WS_VISIBLE, "", m_hMainWindow, StatusBarID);
    SendMessage((HWND)m_hStatusWnd, SB_SETTEXT, 0, (LPARAM)"");
}

WPARAM CMainGui::ProcessAllMessages(void)
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (g_cheatUI != NULL && g_cheatUI->IsCheatMessage(&msg))
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
        if (g_cheatUI && g_cheatUI->IsCheatMessage(&msg)) { continue; }
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
    GetClientRect((HWND)m_hStatusWnd, &swrect);

    int Parts[2];
    Parts[0] = (nWidth - (int)(clrect.right * 0.25));
    Parts[1] = nWidth;

    SendMessage((HWND)m_hStatusWnd, SB_SETPARTS, 2, (LPARAM)&Parts[0]);
    MoveWindow((HWND)m_hStatusWnd, 0, clrect.bottom - swrect.bottom, nWidth, nHeight, TRUE);
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
        SendMessageW((HWND)m_hStatusWnd, SB_SETTEXTW, Panel, (LPARAM)Msg);
    }
    else {
        PostMessageW((HWND)m_hStatusWnd, SB_SETTEXTW, Panel, (LPARAM)Msg);
    }
}

void CMainGui::ShowStatusBar(bool ShowBar)
{
    ShowWindow((HWND)m_hStatusWnd, ShowBar ? SW_SHOW : SW_HIDE);
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
            //record class for future usage
            LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
            CMainGui * _this = (CMainGui *)lpcs->lpCreateParams;
            SetProp(hWnd, "Class", _this);

            _this->m_hMainWindow = hWnd;
            _this->CreateStatusBar();

            //Move the Main window to the location last executed from or center the window
            int X = (GetSystemMetrics(SM_CXSCREEN) - _this->Width()) / 2;
            int	Y = (GetSystemMetrics(SM_CYSCREEN) - _this->Height()) / 2;

            UISettingsLoadDword(UserInterface_MainWindowTop, (uint32_t &)Y);
            UISettingsLoadDword(UserInterface_MainWindowLeft, (uint32_t &)X);

            _this->SetPos(X, Y);

            _this->ChangeWinSize(640, 480);
        }
        break;
    case WM_SYSCOMMAND:
        switch (wParam) {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            {
                CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
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
                CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");

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
                    // save that browser is maximized
                }
                break;
            }

            //get the current position of the window
            RECT WinRect;
            GetWindowRect(hWnd, &WinRect);

            //save the location of the window
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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
            _this->SaveWindowLoc();
            break;
        }
        break;
    case WM_SIZE:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
            if (_this == NULL || !_this->RomBrowserVisible() || !_this->RomListNotify(wParam, lParam))
            {
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
        }
        break;
    case WM_DRAWITEM:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");

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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");

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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
            DWORD fActive = (BOOL)wParam;

            if (fActive && _this->RomBrowserVisible())
            {
                PostMessage(hWnd, WM_BROWSER_TOP, 0, 0);
            }
            if (_this->m_bMainWindow && bCPURunning())
            {
                if (!fActive && UISettingsLoadBool(UserInterface_InFullScreen))
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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
            _this->BringToTop();
        }
        break;
    case WM_BROWSER_TOP:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
            _this->RomBrowserToTop();
        }
        break;
    case WM_RESET_PLUGIN:
        {
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
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
            CMainGui * _this = (CMainGui *)GetProp(hWnd, "Class");
            if (_this == NULL) { break; }

            switch (LOWORD(wParam)) {
            case ID_POPUPMENU_PLAYGAME: 
				{
					if (CPath(_this->CurrentedSelectedRom()).GetExtension() != "ndd")
					{
						g_BaseSystem->RunFileImage(_this->CurrentedSelectedRom());
					}
					else
					{
						if (!CPath(g_Settings->LoadStringVal(File_DiskIPLPath)).Exists() || !g_BaseSystem->RunDiskImage(_this->CurrentedSelectedRom()))
						{
							CPath FileName;
							const char * Filter = "64DD IPL ROM Image (*.zip, *.7z, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin)\0*.?64;*.zip;*.7z;*.bin;*.rom;*.usa;*.jap;*.pal\0All files (*.*)\0*.*\0";
							if (FileName.SelectFile(hWnd, g_Settings->LoadStringVal(RomList_GameDir).c_str(), Filter, true))
							{
								g_Settings->SaveString(File_DiskIPLPath, (const char *)FileName);
								g_BaseSystem->RunDiskImage(_this->CurrentedSelectedRom());
							}
						}
					}
					break;
				}
            case ID_POPUPMENU_PLAYGAMEWITHDISK:
                {
					CPath FileName;
					const char * Filter = "N64DD Disk Image (*.ndd)\0*.ndd\0All files (*.*)\0*.*\0";
					if (FileName.SelectFile(hWnd, g_Settings->LoadStringVal(RomList_GameDir).c_str(), Filter, true))
					{
						if (!CPath(g_Settings->LoadStringVal(File_DiskIPLPath)).Exists() || !g_BaseSystem->RunDiskComboImage(_this->CurrentedSelectedRom(), FileName))
						{
							CPath FileNameIPL;
							const char * Filter = "64DD IPL ROM Image (*.zip, *.7z, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin)\0*.?64;*.zip;*.7z;*.bin;*.rom;*.usa;*.jap;*.pal\0All files (*.*)\0*.*\0";
							if (FileNameIPL.SelectFile(hWnd, g_Settings->LoadStringVal(RomList_GameDir).c_str(), Filter, true))
							{
								g_Settings->SaveString(File_DiskIPLPath, (const char *)FileNameIPL);
								g_BaseSystem->RunDiskComboImage(_this->CurrentedSelectedRom(), FileName);
							}
						}
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
					if (CPath(_this->CurrentedSelectedRom()).GetExtension() != "ndd")
					{
						CN64Rom Rom;
						Rom.LoadN64Image(_this->CurrentedSelectedRom(), true);
						Rom.SaveRomSettingID(true);

						if (LOWORD(wParam) == ID_POPUPMENU_EDITSETTINGS)
						{
							CSettingConfig SettingConfig(true);
							SettingConfig.Display(hWnd);
						}
						else if (LOWORD(wParam) == ID_POPUPMENU_CHOOSEENHANCEMENT)
						{
							CEnhancementConfig().Display(hWnd);
						}
						else if (LOWORD(wParam) == ID_POPUPMENU_EDITCHEATS)
						{
							CCheatsUI * cheatUI = new CCheatsUI;
							g_cheatUI = cheatUI;
							cheatUI->SelectCheats(hWnd, true);
							if (g_cheatUI == cheatUI)
							{
								g_cheatUI = NULL;
							}
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
						else if (LOWORD(wParam) == ID_POPUPMENU_CHOOSEENHANCEMENT)
						{
							CEnhancementConfig().Display(hWnd);
						}
						else if (LOWORD(wParam) == ID_POPUPMENU_EDITCHEATS)
						{
							CCheatsUI * cheatUI = new CCheatsUI;
							g_cheatUI = cheatUI;
							cheatUI->SelectCheats(hWnd, true);
							if (g_cheatUI == cheatUI)
							{
								g_cheatUI = NULL;
							}
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
            DragQueryFile(hDrop, 0, filename, sizeof(filename));
            DragFinish(hDrop);

            stdstr ext = CPath(filename).GetExtension();
            if (!(_stricmp(ext.c_str(), "ndd") == 0))
            {
                delete g_DDRom;
                g_DDRom = NULL;
                CN64System::RunFileImage(filename);
            }
            else
            {
                // Open Disk
				if (!CPath(g_Settings->LoadStringVal(File_DiskIPLPath)).Exists() || !g_BaseSystem->RunDiskImage(filename))
				{
					CPath FileName;
					const char * Filter = "64DD IPL ROM Image (*.zip, *.7z, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin)\0*.?64;*.zip;*.7z;*.bin;*.rom;*.usa;*.jap;*.pal\0All files (*.*)\0*.*\0";
					if (FileName.SelectFile(hWnd, g_Settings->LoadStringVal(RomList_GameDir).c_str(), Filter, true))
					{
						g_Settings->SaveString(File_DiskIPLPath, (const char *)FileName);
						g_BaseSystem->RunDiskImage(filename);
					}
				}
            }
        }
        break;
    case WM_DESTROY:
        WriteTrace(TraceUserInterface, TraceDebug, "WM_DESTROY - start");
        {
            CMainGui   * _this = (CMainGui *)GetProp(hWnd, "Class");
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
        RemoveProp(hWnd, "Class");
        WriteTrace(TraceUserInterface, TraceDebug, "WM_DESTROY - 4");
        PostQuitMessage(0);
        WriteTrace(TraceUserInterface, TraceDebug, "WM_DESTROY - Done");
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return TRUE;
}

DWORD CALLBACK AboutBoxProc(HWND hWnd, DWORD uMsg, DWORD wParam, DWORD /*lParam*/)
{
    static HBITMAP hbmpBackgroundTop = NULL;
    static HFONT   hPageHeadingFont = NULL;
    static HFONT   hTextFont = NULL;
    static HFONT   hAuthorFont = NULL;

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            //Title
            SetWindowTextW(hWnd, wGS(PLUG_ABOUT).c_str());

            // Use the size of the image
            hbmpBackgroundTop = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ABOUT_LOGO));

            BITMAP bmTL;
            GetObject(hbmpBackgroundTop, sizeof(BITMAP), &bmTL);

            hTextFont = ::CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
            hAuthorFont = ::CreateFont(18, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");

            hPageHeadingFont = ::CreateFont(24, 0, 0, 0, FW_BOLD, 0, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial Bold");

            SendDlgItemMessage(hWnd, IDC_VERSION, WM_SETFONT, (WPARAM)hTextFont, TRUE);
            SendDlgItemMessage(hWnd, IDC_TEAM, WM_SETFONT, (WPARAM)hPageHeadingFont, TRUE);
            SendDlgItemMessage(hWnd, IDC_THANKS, WM_SETFONT, (WPARAM)hPageHeadingFont, TRUE);

            SendDlgItemMessage(hWnd, IDC_ZILMAR, WM_SETFONT, (WPARAM)hAuthorFont, TRUE);
            SendDlgItemMessage(hWnd, IDC_JABO, WM_SETFONT, (WPARAM)hAuthorFont, TRUE);
            SendDlgItemMessage(hWnd, IDC_SMIFF, WM_SETFONT, (WPARAM)hAuthorFont, TRUE);
            SendDlgItemMessage(hWnd, IDC_GENT, WM_SETFONT, (WPARAM)hAuthorFont, TRUE);

            SendDlgItemMessage(hWnd, IDC_ZILMAR_DETAILS, WM_SETFONT, (WPARAM)hTextFont, TRUE);
            SendDlgItemMessage(hWnd, IDC_JABO_DETAILS, WM_SETFONT, (WPARAM)hTextFont, TRUE);
            SendDlgItemMessage(hWnd, IDC_SMIFF_DETAILS, WM_SETFONT, (WPARAM)hTextFont, TRUE);
            SendDlgItemMessage(hWnd, IDC_GENT_DETAILS, WM_SETFONT, (WPARAM)hTextFont, TRUE);

            SendDlgItemMessage(hWnd, IDC_THANK_LIST, WM_SETFONT, (WPARAM)hTextFont, TRUE);

            stdstr_f VersionDisplay("Version: %s", VER_FILE_VERSION_STR);
            SetWindowText(GetDlgItem(hWnd, IDC_VERSION), VersionDisplay.c_str());
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
    case WM_ERASEBKGND:
        {
            HPEN outline;
            HBRUSH fill;
            RECT rect;

            outline = CreatePen(PS_SOLID, 1, 0x00FFFFFF);
            fill = CreateSolidBrush(0x00FFFFFF);
            SelectObject((HDC)wParam, outline);
            SelectObject((HDC)wParam, fill);

            GetClientRect(hWnd, &rect);

            Rectangle((HDC)wParam, rect.left, rect.top, rect.right, rect.bottom);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;

            if (BeginPaint(hWnd, &ps))
            {
                RECT rcClient;
                GetClientRect(hWnd, &rcClient);

                BITMAP bmTL_top;
                GetObject(hbmpBackgroundTop, sizeof(BITMAP), &bmTL_top);

                HDC     memdc = CreateCompatibleDC(ps.hdc);
                HGDIOBJ save = SelectObject(memdc, hbmpBackgroundTop);
                BitBlt(ps.hdc, 0, 0, bmTL_top.bmWidth, bmTL_top.bmHeight, memdc, 0, 0, SRCCOPY);
                SelectObject(memdc, save);
                DeleteDC(memdc);

                EndPaint(hWnd, &ps);
            }
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            if (hbmpBackgroundTop)
            {
                DeleteObject(hbmpBackgroundTop);
            }
            if (hTextFont)
            {
                ::DeleteObject(hTextFont);
            }
            if (hPageHeadingFont)
            {
                ::DeleteObject(hPageHeadingFont);
            }
            if (hAuthorFont)
            {
                ::DeleteObject(hAuthorFont);
            }
            //ReleaseCapture();
            EndDialog(hWnd, 0);
            break;
        }
    default:
        return FALSE;
    }
    return TRUE;
}

BOOL set_about_field(HWND hDlg, int nIDDlgItem, const wchar_t * config_string, const wchar_t * language_string)
{
    wchar_t temp_string[200];

    swprintf(temp_string, sizeof(temp_string) / sizeof(temp_string[0]), L"%s: %s", config_string, language_string);
    return SetDlgItemTextW(hDlg, nIDDlgItem, temp_string);
}
