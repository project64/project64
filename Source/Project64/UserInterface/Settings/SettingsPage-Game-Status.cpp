#include "stdafx.h"

#include "SettingsPage.h"
#include "SettingsPage-Game-Status.h"

CGameStatusPage::CGameStatusPage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    CIniFile RomIniFile(g_Settings->LoadStringVal(SupportFile_RomDatabase).c_str());
	CIniFile::strlist Keys;
    RomIniFile.GetKeyList("ROM Status", Keys);
    stdstr Status = UISettingsLoadStringVal(Rdb_Status);

    CModifiedComboBoxTxt * ComboBox;
    ComboBox = AddModComboBoxTxt(GetDlgItem(IDC_STATUS_TYPE), (SettingID)Rdb_Status);
    if (ComboBox)
    {
        for (CIniFile::strlist::iterator item = Keys.begin(); item != Keys.end(); item++)
        {
            if (strstr(item->c_str(), ".Sel") != nullptr) { continue; }
            if (strstr(item->c_str(), ".Auto") != nullptr) { continue; }
            ComboBox->AddItem(stdstr(*item).ToUTF16().c_str(), item->c_str());
        }
        ComboBox->SetTextField(GetDlgItem(IDC_STATUS_TEXT));
    }
    CModifiedEditBox * TxtBox;
    TxtBox = AddModTextBox(GetDlgItem(IDC_NOTES_CORE), (SettingID)Rdb_NotesCore, true);
    TxtBox->SetTextField(GetDlgItem(IDC_NOTES_CORE_TEXT));
    TxtBox = AddModTextBox(GetDlgItem(IDC_NOTES_PLUGIN), (SettingID)Rdb_NotesPlugin, true);
    TxtBox->SetTextField(GetDlgItem(IDC_NOTES_PLUGIN_TEXT));

    UpdatePageSettings();
}

void CGameStatusPage::ShowPage()
{
    ShowWindow(SW_SHOW);
}

void CGameStatusPage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void CGameStatusPage::ApplySettings(bool UpdateScreen)
{
    CSettingsPageImpl<CGameStatusPage>::ApplySettings(UpdateScreen);
}

bool CGameStatusPage::EnableReset(void)
{
    if (CSettingsPageImpl<CGameStatusPage>::EnableReset()) { return true; }
    return false;
}

void CGameStatusPage::ResetPage()
{
    CSettingsPageImpl<CGameStatusPage>::ResetPage();
}
