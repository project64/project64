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

CDiskDrivePage::CDiskDrivePage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    //Set the text for all gui Items
    SetDlgItemText(IDC_IPLDIR_JP_TXT, wGS(OPTION_IPL_ROM_PATH).c_str());
    SetDlgItemText(IDC_IPLDIR_US_TXT, wGS(OPTION_IPL_ROM_USA_PATH).c_str());
    SetDlgItemText(IDC_IPLDIR_TL_TXT, wGS(OPTION_IPL_ROM_TOOL_PATH).c_str());
    SetDlgItemText(IDC_DISKSAVETYPE_TXT, wGS(OPTION_DISKSAVETYPE).c_str());

    m_IplDirJp.Attach(GetDlgItem(IDC_IPL_JP_DIR));
    m_IplDirUs.Attach(GetDlgItem(IDC_IPL_US_DIR));
    m_IplDirTl.Attach(GetDlgItem(IDC_IPL_TL_DIR));

    CModifiedComboBox * ComboBox;
    ComboBox = AddModComboBox(GetDlgItem(IDC_DISKSAVETYPE), Setting_DiskSaveType);
    if (ComboBox)
    {
        ComboBox->AddItem(wGS(DISKSAVE_SHADOW).c_str(), SaveDisk_ShadowFile);
        ComboBox->AddItem(wGS(DISKSAVE_RAM).c_str(), SaveDisk_RAMFile);
    }

    UpdatePageSettings();
}

void CDiskDrivePage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void CDiskDrivePage::ShowPage()
{
    ShowWindow(SW_SHOW);
}

void CDiskDrivePage::ApplySettings(bool UpdateScreen)
{
    if (m_IplDirJp.IsChanged())
    {
        stdstr file = m_IplDirJp.GetWindowText();
        g_Settings->SaveString(File_DiskIPLPath, file.c_str());
    }
    if (m_IplDirJp.IsReset())
    {
        g_Settings->DeleteSetting(File_DiskIPLPath);
    }

    if (m_IplDirUs.IsChanged())
    {
        stdstr file = m_IplDirUs.GetWindowText();
        g_Settings->SaveString(File_DiskIPLUSAPath, file.c_str());
    }
    if (m_IplDirUs.IsReset())
    {
        g_Settings->DeleteSetting(File_DiskIPLUSAPath);
    }

    if (m_IplDirTl.IsChanged())
    {
        stdstr file = m_IplDirTl.GetWindowText();
        g_Settings->SaveString(File_DiskIPLTOOLPath, file.c_str());
    }
    if (m_IplDirTl.IsReset())
    {
        g_Settings->DeleteSetting(File_DiskIPLTOOLPath);
    }

    CSettingsPageImpl<CDiskDrivePage>::ApplySettings(UpdateScreen);
}

bool CDiskDrivePage::EnableReset(void)
{
    if (CSettingsPageImpl<CDiskDrivePage>::EnableReset()) { return true; }
    return false;
}

void CDiskDrivePage::ResetPage()
{
    CSettingsPageImpl<CDiskDrivePage>::ResetPage();
}

void CDiskDrivePage::SelectIplDirJp(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    SelectFile(DIR_SELECT_PLUGIN, m_IplDirJp);
}

void CDiskDrivePage::SelectIplDirUs(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    SelectFile(DIR_SELECT_PLUGIN, m_IplDirUs);
}

void CDiskDrivePage::SelectIplDirTl(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    SelectFile(DIR_SELECT_PLUGIN, m_IplDirTl);
}

void CDiskDrivePage::IplDirJpChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    if (m_InUpdateSettings) { return; }
    m_IplDirJp.SetChanged(true);
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void CDiskDrivePage::IplDirUsChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    if (m_InUpdateSettings) { return; }
    m_IplDirUs.SetChanged(true);
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void CDiskDrivePage::IplDirTlChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    if (m_InUpdateSettings)  { return; }
    m_IplDirTl.SetChanged(true);
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void CDiskDrivePage::UpdatePageSettings(void)
{
    m_InUpdateSettings = true;
    CSettingsPageImpl<CDiskDrivePage>::UpdatePageSettings();

    stdstr File;
    g_Settings->LoadStringVal(File_DiskIPLPath, File);
    m_IplDirJp.SetWindowText(File.ToUTF16().c_str());
    g_Settings->LoadStringVal(File_DiskIPLUSAPath, File);
    m_IplDirUs.SetWindowText(File.ToUTF16().c_str());
    g_Settings->LoadStringVal(File_DiskIPLTOOLPath, File);
    m_IplDirTl.SetWindowText(File.ToUTF16().c_str());

    m_InUpdateSettings = false;
}

void CDiskDrivePage::SelectFile(LanguageStringID /*Title*/, CModifiedEditBox & EditBox)
{
    const char * Filter = "64DD IPL ROM Image (*.zip, *.7z, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin)\0*.?64;*.zip;*.7z;*.bin;*.rom;*.usa;*.jap;*.pal\0All files (*.*)\0*.*\0";

    CPath FileName;
    if (FileName.SelectFile(m_hWnd, g_Settings->LoadStringVal(RomList_GameDir).c_str(), Filter, true))
    {
        EditBox.SetChanged(true);
        EditBox.SetWindowText(stdstr((const char *)FileName).ToUTF16().c_str());
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }
}