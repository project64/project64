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

CAdvancedOptionsPage::CAdvancedOptionsPage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    //Set the text for all gui Items
    SetDlgItemTextW(m_hWnd, IDC_START_ON_ROM_OPEN, wGS(ADVANCE_AUTO_START).c_str());
    SetDlgItemTextW(m_hWnd, IDC_ZIP, wGS(ADVANCE_COMPRESS).c_str());
    SetDlgItemTextW(m_hWnd, IDC_DEBUGGER, wGS(ADVANCE_DEBUGGER).c_str());
    SetDlgItemTextW(m_hWnd, IDC_REMEMBER_CHEAT, wGS(OPTION_REMEMBER_CHEAT).c_str());
    SetDlgItemTextW(m_hWnd, IDC_UNIQUE_SAVE_DIR, wGS(OPTION_UNIQUE_SAVE_DIR).c_str());
    SetDlgItemTextW(m_hWnd, IDC_CHECK_RUNNING, wGS(OPTION_CHECK_RUNNING).c_str());
    SetDlgItemTextW(m_hWnd, IDC_DISPLAY_FRAMERATE, wGS(OPTION_CHANGE_FR).c_str());
    SetDlgItemTextW(m_hWnd, IDC_IPLDIR_TXT, wGS(OPTION_IPL_ROM_PATH).c_str());

    AddModCheckBox(GetDlgItem(IDC_START_ON_ROM_OPEN), Setting_AutoStart);
    AddModCheckBox(GetDlgItem(IDC_ZIP), Setting_AutoZipInstantSave);
    AddModCheckBox(GetDlgItem(IDC_DEBUGGER), Debugger_Enabled);
	AddModCheckBox(GetDlgItem(IDC_INTERPRETER), Setting_ForceInterpreterCPU);
    AddModCheckBox(GetDlgItem(IDC_REMEMBER_CHEAT), Setting_RememberCheats);
    AddModCheckBox(GetDlgItem(IDC_UNIQUE_SAVE_DIR), Setting_UniqueSaveDir);
    AddModCheckBox(GetDlgItem(IDC_CHECK_RUNNING), Setting_CheckEmuRunning);
    AddModCheckBox(GetDlgItem(IDC_DISPLAY_FRAMERATE), UserInterface_DisplayFrameRate);

    CModifiedComboBox * ComboBox;
    ComboBox = AddModComboBox(GetDlgItem(IDC_FRAME_DISPLAY_TYPE), UserInterface_FrameDisplayType);
    if (ComboBox)
    {
        ComboBox->AddItemW(wGS(STR_FR_VIS).c_str(), FR_VIs);
        ComboBox->AddItemW(wGS(STR_FR_DLS).c_str(), FR_DLs);
        ComboBox->AddItemW(wGS(STR_FR_PERCENT).c_str(), FR_PERCENT);
        ComboBox->AddItemW(wGS(STR_FR_DLS_VIS).c_str(), FR_VIs_DLs);
    }

    m_IplDir.Attach(GetDlgItem(IDC_IPL_DIR));

    UpdatePageSettings();
}

void CAdvancedOptionsPage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void CAdvancedOptionsPage::ShowPage()
{
    ShowWindow(SW_SHOW);
}

void CAdvancedOptionsPage::ApplySettings(bool UpdateScreen)
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

    CSettingsPageImpl<CAdvancedOptionsPage>::ApplySettings(UpdateScreen);
}

bool CAdvancedOptionsPage::EnableReset(void)
{
    if (CSettingsPageImpl<CAdvancedOptionsPage>::EnableReset()) { return true; }
    return false;
}

void CAdvancedOptionsPage::ResetPage()
{
    CSettingsPageImpl<CAdvancedOptionsPage>::ResetPage();
}

void CAdvancedOptionsPage::SelectIplDir(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    SelectFile(DIR_SELECT_PLUGIN, m_IplDir);
}

void CAdvancedOptionsPage::IplDirChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    if (m_InUpdateSettings)  { return; }
    m_IplDir.SetChanged(true);
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void CAdvancedOptionsPage::UpdatePageSettings(void)
{
    m_InUpdateSettings = true;
    CSettingsPageImpl<CAdvancedOptionsPage>::UpdatePageSettings();

    stdstr File;
    g_Settings->LoadStringVal(File_DiskIPLPath, File);
    m_IplDir.SetWindowText(File.c_str());

    m_InUpdateSettings = false;
}

void CAdvancedOptionsPage::SelectFile(LanguageStringID /*Title*/, CModifiedEditBox & EditBox)
{
    const char * Filter = "64DD IPL ROM Image (*.zip, *.7z, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin)\0*.?64;*.zip;*.7z;*.bin;*.rom;*.usa;*.jap;*.pal\0All files (*.*)\0*.*\0";

    CPath FileName;
    if (FileName.SelectFile(m_hWnd, g_Settings->LoadStringVal(RomList_GameDir).c_str(), Filter, true))
    {
        EditBox.SetChanged(true);
        EditBox.SetWindowText(FileName);
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }
}