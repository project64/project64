#include "stdafx.h"
#include "Settings Page.h"

CGeneralOptionsPage::CGeneralOptionsPage (HWND hParent, const RECT & rcDispay )
{
	if (!Create(hParent,rcDispay))
	{
		return;
	}

	AddModCheckBox(GetDlgItem(IDC_AUTOSLEEP),Setting_AutoSleep);
	AddModCheckBox(GetDlgItem(IDC_LOAD_FULLSCREEN),Setting_AutoFullscreen);
	AddModCheckBox(GetDlgItem(IDC_SCREEN_SAVER),Setting_DisableScrSaver);
	AddModCheckBox(GetDlgItem(IDC_BASIC_MODE),UserInterface_BasicMode);

	CModifiedEditBox * TxtBox = AddModTextBox(GetDlgItem(IDC_REMEMBER),File_RecentGameFileCount, false);
	TxtBox->SetTextField(GetDlgItem(IDC_MAXROMS_TXT));

	TxtBox = AddModTextBox(GetDlgItem(IDC_REMEMBERDIR),Directory_RecentGameDirCount, false);
	TxtBox->SetTextField(GetDlgItem(IDC_MAXROMDIR_TXT));

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

void CGeneralOptionsPage::ApplySettings( bool UpdateScreen )
{
	CSettingsPageImpl<CGeneralOptionsPage>::ApplySettings(UpdateScreen);
}

bool CGeneralOptionsPage::EnableReset ( void )
{
	if (CSettingsPageImpl<CGeneralOptionsPage>::EnableReset()) { return true; }
	return false;
}

void CGeneralOptionsPage::ResetPage()
{
	CSettingsPageImpl<CGeneralOptionsPage>::ResetPage();
}
