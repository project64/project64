#include "stdafx.h"

#include "SettingsPage.h"

CAdvancedOptionsPage::CAdvancedOptionsPage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    // Set the text for all GUI items
    SetDlgItemText(IDC_START_ON_ROM_OPEN, wGS(ADVANCE_AUTO_START).c_str());
    SetDlgItemText(IDC_ZIP, wGS(ADVANCE_COMPRESS).c_str());
    SetDlgItemText(IDC_DEBUGGER, wGS(ADVANCE_DEBUGGER).c_str());
    SetDlgItemText(IDC_REMEMBER_CHEAT, wGS(OPTION_REMEMBER_CHEAT).c_str());
    SetDlgItemText(IDC_UNIQUE_SAVE_DIR, wGS(OPTION_UNIQUE_SAVE_DIR).c_str());
    SetDlgItemText(IDC_CHECK_RUNNING, wGS(OPTION_CHECK_RUNNING).c_str());
    SetDlgItemText(IDC_ENABLE_ENHANCEMENTS, wGS(OPTION_ENABLE_ENHANCEMENTS).c_str());
    SetDlgItemText(IDC_DISPLAY_FRAMERATE, wGS(OPTION_CHANGE_FR).c_str());
    SetDlgItemText(IDC_SHOW_STATUS_BAR, wGS(OPTION_SHOW_STATUS_BAR).c_str());

    AddModCheckBox(GetDlgItem(IDC_START_ON_ROM_OPEN), Setting_AutoStart);
    AddModCheckBox(GetDlgItem(IDC_ZIP), Setting_AutoZipInstantSave);
    AddModCheckBox(GetDlgItem(IDC_DEBUGGER), Debugger_Enabled);
	AddModCheckBox(GetDlgItem(IDC_INTERPRETER), Setting_ForceInterpreterCPU);
    AddModCheckBox(GetDlgItem(IDC_REMEMBER_CHEAT), Setting_RememberCheats);
    AddModCheckBox(GetDlgItem(IDC_UNIQUE_SAVE_DIR), Setting_UniqueSaveDir);
    AddModCheckBox(GetDlgItem(IDC_CHECK_RUNNING), Setting_CheckEmuRunning);
    AddModCheckBox(GetDlgItem(IDC_ENABLE_ENHANCEMENTS), Setting_Enhancement);
    AddModCheckBox(GetDlgItem(IDC_DISPLAY_FRAMERATE), UserInterface_DisplayFrameRate);
    AddModCheckBox(GetDlgItem(IDC_SHOW_STATUS_BAR), (SettingID)UserInterface_ShowStatusBar);

    CModifiedComboBox * ComboBox;
    ComboBox = AddModComboBox(GetDlgItem(IDC_FRAME_DISPLAY_TYPE), UserInterface_FrameDisplayType);
    if (ComboBox)
    {
        ComboBox->AddItem(wGS(STR_FR_VIS).c_str(), FR_VIs);
        ComboBox->AddItem(wGS(STR_FR_DLS).c_str(), FR_DLs);
        ComboBox->AddItem(wGS(STR_FR_PERCENT).c_str(), FR_PERCENT);
        ComboBox->AddItem(wGS(STR_FR_DLS_VIS).c_str(), FR_VIs_DLs);
    }

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

void CAdvancedOptionsPage::UpdatePageSettings(void)
{
    m_InUpdateSettings = true;
    CSettingsPageImpl<CAdvancedOptionsPage>::UpdatePageSettings();
    m_InUpdateSettings = false;
}
