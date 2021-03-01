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
#include "WelcomeScreen.h"
#include "resource.h"

WelcomeScreen::WelcomeScreen()
{
}

void WelcomeScreen::SelectGameDir(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    wchar_t Buffer[MAX_PATH], Directory[MAX_PATH];
    LPITEMIDLIST pidl;
    BROWSEINFO bi;

    stdstr InitialDir = g_Settings->LoadStringVal(RomList_GameDir);
    std::wstring wTitle = L"Select Game Directory";
    bi.hwndOwner = m_hWnd;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = Buffer;
    bi.lpszTitle = wTitle.c_str();
    bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    bi.lpfn = (BFFCALLBACK)SelectDirCallBack;
    bi.lParam = (DWORD)InitialDir.c_str();
    if ((pidl = SHBrowseForFolder(&bi)) != NULL)
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
    }
}

LRESULT WelcomeScreen::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_Logo.SubclassWindow(GetDlgItem(IDC_BMP_LOGO));
    m_Logo.SetBitmap(MAKEINTRESOURCE(IDB_ABOUT_LOGO));
 
    LanguageList LangList = g_Lang->GetLangList();
    CComboBox LangCB(GetDlgItem(IDC_LANG_SEL));
    for (LanguageList::iterator Language = LangList.begin(); Language != LangList.end(); Language++)
    {
        int Index = LangCB.AddString(stdstr(Language->LanguageName).ToUTF16().c_str());
        if (_stricmp(Language->LanguageName.c_str(), "English") == 0)
        {
            LangCB.SetCurSel(Index);
        }
    }
    if (LangCB.GetCurSel() < 0)
    {
        LangCB.SetCurSel(0);
    }
    CButton(GetDlgItem(IDC_RADIO_GLIDEN64)).SetCheck(BST_CHECKED);
    return TRUE;
}

LRESULT WelcomeScreen::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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

HBRUSH WelcomeScreen::OnCtlColorStatic(CDCHandle dc, CStatic /*wndStatic*/)
{
    dc.SetBkColor(RGB(255, 255, 255));
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

    g_Settings->SaveString(Plugin_GFX_Default, CButton(GetDlgItem(IDC_RADIO_GLIDEN64)).GetCheck() == BST_CHECKED ? "GFX\\GLideN64\\GLideN64.dll" : "GFX\\Project64-Video.dll");
    EndDialog(0);
    return TRUE;
}

int CALLBACK WelcomeScreen::SelectDirCallBack(HWND hwnd, DWORD uMsg, DWORD /*lp*/, DWORD lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        // WParam is TRUE since you are passing a path.
        // It would be FALSE if you were passing a pidl.
        if (lpData)
        {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        }
        break;
    }
    return 0;
}
