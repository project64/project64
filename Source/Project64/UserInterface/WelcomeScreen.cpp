#include "stdafx.h"

#include "WelcomeScreen.h"
#include "resource.h"
#include <commctrl.h>

namespace
{
LRESULT CALLBACK LanguageComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
    if (msg == WM_CTLCOLORLISTBOX || msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORSTATIC)
    {
        HDC dc = (HDC)wParam;
        SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(dc, GetSysColor(COLOR_WINDOW));
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void MakeLanguageComboReadable(HWND combo)
{
    HMODULE themeLib = LoadLibraryW(L"uxtheme.dll");
    if (themeLib != nullptr)
    {
        typedef HRESULT(WINAPI * SetWindowThemeFn)(HWND, LPCWSTR, LPCWSTR);
        auto setWindowTheme = (SetWindowThemeFn)GetProcAddress(themeLib, "SetWindowTheme");
        if (setWindowTheme != nullptr)
        {
            setWindowTheme(combo, L"", L"");
        }
        FreeLibrary(themeLib);
    }
    SetWindowSubclass(combo, LanguageComboProc, 1, 0);
    SendMessage(combo, CB_SETMINVISIBLE, 10, 0);
    SendMessage(combo, CB_SETTOPINDEX, 0, 0);
}
} // namespace

WelcomeScreen::WelcomeScreen()
{
}

void WelcomeScreen::SelectGameDir(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    wchar_t Buffer[MAX_PATH], Directory[MAX_PATH];
    BROWSEINFO bi = {};

    // Keep this alive until SHBrowseForFolder returns. The callback receives the
    // pointer as LPARAM; a 32-bit DWORD truncates it on x64 and ARM64.
    std::wstring InitialDir = stdstr(g_Settings->LoadStringVal(RomList_GameDir)).ToUTF16();
    std::wstring wTitle = L"Select Game Directory";
    bi.hwndOwner = m_hWnd;
    bi.pidlRoot = nullptr;
    bi.pszDisplayName = Buffer;
    bi.lpszTitle = wTitle.c_str();
    bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    bi.lpfn = SelectDirCallBack;
    bi.lParam = (LPARAM)InitialDir.c_str();
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != nullptr)
    {
        if (SHGetPathFromIDList(pidl, Directory))
        {
            stdstr path;
            CPath SelectedDir(path.FromUTF16(Directory), "");
            if (SelectedDir.DirectoryExists())
            {
                GetDlgItem(IDC_GAME_DIR).SetWindowText(Directory);
            }
        }
        CoTaskMemFree(pidl);
    }
}

LRESULT WelcomeScreen::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    m_Logo.SubclassWindow(GetDlgItem(IDC_BMP_LOGO));
    m_Logo.SetBitmap(MAKEINTRESOURCE(IDB_ABOUT_LOGO));

    LanguageList LangList = g_Lang->GetLangList();
    CComboBox LangCB(GetDlgItem(IDC_LANG_SEL));
    MakeLanguageComboReadable(LangCB);
    for (LanguageList::iterator Language = LangList.begin(); Language != LangList.end(); Language++)
    {
        LangCB.AddString(stdstr(Language->LanguageName).ToUTF16().c_str());
    }
    // CBS_SORT moves items as they are inserted, so choose English after the list is complete.
    int english = LangCB.FindStringExact(-1, L"English");
    LangCB.SetCurSel(english >= 0 ? english : 0);
    LangCB.SetTopIndex(0);
    CButton(GetDlgItem(IDC_RADIO_GLIDEN64)).SetCheck(BST_CHECKED);
    return TRUE;
}

LRESULT WelcomeScreen::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    HDC hdcStatic = (HDC)wParam;
    SetTextColor(hdcStatic, RGB(0, 0, 0));
    SetBkMode(hdcStatic, TRANSPARENT);
    return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
}

BOOL WelcomeScreen::OnEraseBackground(CDCHandle dc)
{
    static HPEN outline = CreatePen(PS_SOLID, 1, 0x00FFFFFF);
    static HBRUSH fill = CreateSolidBrush(0x00FFFFFF);
    dc.SelectPen(outline);
    dc.SelectBrush(fill);

    RECT rect;
    GetClientRect(&rect);
    dc.Rectangle(&rect);
    return TRUE;
}

HBRUSH WelcomeScreen::OnCtlColorListBox(CDCHandle dc, CListBox /*listBox*/)
{
    dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
    dc.SetBkColor(GetSysColor(COLOR_WINDOW));
    return GetSysColorBrush(COLOR_WINDOW);
}

HBRUSH WelcomeScreen::OnCtlColorStatic(CDCHandle dc, CStatic wndStatic)
{
    dc.SetTextColor(RGB(0, 0, 0));
    dc.SetBkColor(RGB(255, 255, 255));
    // DC_BRUSH leaves the language combo's list and selection text unpainted on
    // 64-bit Windows with per-monitor DPI. A real white brush keeps the names visible.
    if (wndStatic.GetDlgCtrlID() == IDC_LANG_SEL)
    {
        return (HBRUSH)GetStockObject(WHITE_BRUSH);
    }
    dc.SetDCBrushColor(RGB(255, 255, 255));
    return (HBRUSH)GetStockObject(DC_BRUSH);
}

LRESULT WelcomeScreen::OnOkCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    CComboBox LangCB(GetDlgItem(IDC_LANG_SEL));
    int Index = LangCB.GetCurSel();
    if (Index >= 0)
    {
        wchar_t String[255];
        LangCB.GetLBText(Index, String);
        g_Lang->SetLanguage(stdstr().FromUTF16(String).c_str());
    }

    CPath GameDir(GetCWindowText(GetDlgItem(IDC_GAME_DIR)).c_str(), "");
    if (GameDir.DirectoryExists())
    {
        g_Settings->SaveString(RomList_GameDir, GameDir.GetDriveDirectory().c_str());
        Notify().AddRecentDir(GameDir);
    }

    string Project64VideoPluginPath = g_Settings->LoadStringVal(Plugin_GFX_Default);
    if (Project64VideoPluginPath.find("Project64-Video") == string::npos)
    {
        Project64VideoPluginPath = "GFX\\Project64-Video.dll";
    }
    g_Settings->SaveString(Plugin_GFX_Default, CButton(GetDlgItem(IDC_RADIO_GLIDEN64)).GetCheck() == BST_CHECKED ? "GFX\\GLideN64\\GLideN64.dll" : Project64VideoPluginPath);
    g_Settings->SaveString(Plugin_GFX_Current, g_Settings->LoadStringVal(Plugin_GFX_Default));
    EndDialog(0);
    return TRUE;
}

int CALLBACK WelcomeScreen::SelectDirCallBack(HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        // wParam is TRUE because lpData is a wide path, not a PIDL.
        if (lpData != 0)
        {
            SendMessage(hwnd, BFFM_SETSELECTIONW, TRUE, lpData);
        }
        break;
    }
    return 0;
}
