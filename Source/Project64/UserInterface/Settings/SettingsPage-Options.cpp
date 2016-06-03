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

#include "SettingsPage.h"

CGeneralOptionsPage::CGeneralOptionsPage(CSettingConfig * SettingsConfig, HWND hParent, const RECT & rcDispay) :
m_SettingsConfig(SettingsConfig)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    //Set the text for all gui Items
    SetDlgItemTextW(m_hWnd, IDC_AUTOSLEEP, wGS(OPTION_AUTO_SLEEP).c_str());
    SetDlgItemTextW(m_hWnd, IDC_LOAD_FULLSCREEN, wGS(OPTION_AUTO_FULLSCREEN).c_str());
    SetDlgItemTextW(m_hWnd, IDC_SCREEN_SAVER, wGS(OPTION_DISABLE_SS).c_str());
    SetDlgItemTextW(m_hWnd, IDC_BASIC_MODE, wGS(OPTION_BASIC_MODE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_MAXROMS_TXT, wGS(RB_MAX_ROMS).c_str());
    SetDlgItemTextW(m_hWnd, IDC_ROMSEL_TEXT2, wGS(RB_ROMS).c_str());
    SetDlgItemTextW(m_hWnd, IDC_MAXROMDIR_TXT, wGS(RB_MAX_DIRS).c_str());
    SetDlgItemTextW(m_hWnd, IDC_ROMSEL_TEXT4, wGS(RB_DIRS).c_str());
    SetDlgItemTextW(m_hWnd, IDC_IPLDIR_TXT, wGS(OPTION_IPL_ROM_PATH).c_str());

    AddModCheckBox(GetDlgItem(IDC_AUTOSLEEP), Setting_AutoSleep);
    AddModCheckBox(GetDlgItem(IDC_LOAD_FULLSCREEN), Setting_AutoFullscreen);
    AddModCheckBox(GetDlgItem(IDC_SCREEN_SAVER), Setting_DisableScrSaver);
    AddModCheckBox(GetDlgItem(IDC_BASIC_MODE), UserInterface_BasicMode);

    CModifiedEditBox * TxtBox = AddModTextBox(GetDlgItem(IDC_REMEMBER), File_RecentGameFileCount, false);
    TxtBox->SetTextField(GetDlgItem(IDC_MAXROMS_TXT));

    TxtBox = AddModTextBox(GetDlgItem(IDC_REMEMBERDIR), Directory_RecentGameDirCount, false);
    TxtBox->SetTextField(GetDlgItem(IDC_MAXROMDIR_TXT));

    m_IplDir.Attach(GetDlgItem(IDC_IPL_DIR));

    UpdatePageSettings();
}

void CGeneralOptionsPage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void CGeneralOptionsPage::ShowPage()
{
    ShowWindow(SW_SHOW);
}

void CGeneralOptionsPage::ApplySettings(bool UpdateScreen)
{
    if (m_IplDir.IsChanged())
    {
        stdstr file = m_IplDir.GetWindowText();
        g_Settings->SaveString(File_DiskIPLPath, file.c_str());
    }
    if (m_IplDir.IsReset())
    {
        g_Settings->DeleteSetting(File_DiskIPLPath);
    }

    CSettingsPageImpl<CGeneralOptionsPage>::ApplySettings(UpdateScreen);
}

bool CGeneralOptionsPage::EnableReset(void)
{
    if (CSettingsPageImpl<CGeneralOptionsPage>::EnableReset()) { return true; }
    return false;
}

void CGeneralOptionsPage::ResetPage()
{
    CSettingsPageImpl<CGeneralOptionsPage>::ResetPage();
    m_SettingsConfig->UpdateAdvanced((int)::SendMessage(GetDlgItem(IDC_BASIC_MODE), BM_GETCHECK, 0, 0) == 0);
}

void CGeneralOptionsPage::OnBasicMode(UINT Code, int id, HWND ctl)
{
    CheckBoxChanged(Code, id, ctl);
    m_SettingsConfig->UpdateAdvanced((int)::SendMessage(ctl, BM_GETCHECK, 0, 0) == 0);
}

void CGeneralOptionsPage::SelectIplDir(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    SelectFile(DIR_SELECT_PLUGIN, m_IplDir);
}

void CGeneralOptionsPage::IplDirChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    if (m_InUpdateSettings)  { return; }
    m_IplDir.SetChanged(true);
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void CGeneralOptionsPage::UpdatePageSettings(void)
{
    m_InUpdateSettings = true;
    CSettingsPageImpl<CGeneralOptionsPage>::UpdatePageSettings();

    stdstr File;
    g_Settings->LoadStringVal(File_DiskIPLPath, File);
    m_IplDir.SetWindowText(File.c_str());

    m_InUpdateSettings = false;
}

void CGeneralOptionsPage::SelectFile(LanguageStringID Title, CModifiedEditBox & EditBox)
{
    // Open DDROM
    OPENFILENAME openfilename;
    char FileName[_MAX_PATH], Directory[_MAX_PATH];
    memset(&FileName, 0, sizeof(FileName));
    memset(&openfilename, 0, sizeof(openfilename));

    strcpy(Directory, g_Settings->LoadStringVal(RomList_GameDir).c_str());
    openfilename.lStructSize = sizeof(openfilename);
    openfilename.hwndOwner = m_hWnd;
    openfilename.lpstrFilter = "64DD IPL ROM Image (*.zip, *.7z, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin)\0*.?64;*.zip;*.7z;*.bin;*.rom;*.usa;*.jap;*.pal\0All files (*.*)\0*.*\0";
    openfilename.lpstrFile = FileName;
    openfilename.lpstrInitialDir = Directory;
    openfilename.nMaxFile = MAX_PATH;
    openfilename.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileName(&openfilename))
    {
        EditBox.SetChanged(true);
        EditBox.SetWindowText(FileName);
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }
}