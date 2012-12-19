/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include "Settings Page.h"

CAdvancedOptionsPage::CAdvancedOptionsPage (HWND hParent, const RECT & rcDispay )
{
	if (!Create(hParent,rcDispay))
	{
		return;
	}
	AddModCheckBox(GetDlgItem(IDC_START_ON_ROM_OPEN),Setting_AutoStart);
	AddModCheckBox(GetDlgItem(IDC_ZIP),Setting_AutoZipInstantSave);
	AddModCheckBox(GetDlgItem(IDC_DEBUGGER),Debugger_Enabled);
	AddModCheckBox(GetDlgItem(IDC_REMEMBER_CHEAT),Setting_RememberCheats);
	AddModCheckBox(GetDlgItem(IDC_DISPLAY_FRAMERATE),UserInterface_DisplayFrameRate);

	CModifiedComboBox * ComboBox;
	ComboBox = AddModComboBox(GetDlgItem(IDC_FRAME_DISPLAY_TYPE),UserInterface_FrameDisplayType);
	if (ComboBox)
	{
		ComboBox->AddItem(GS(STR_FR_VIS), FR_VIs );
		ComboBox->AddItem(GS(STR_FR_DLS), FR_DLs );
		ComboBox->AddItem(GS(STR_FR_PERCENT), FR_PERCENT );
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

void CAdvancedOptionsPage::ApplySettings( bool UpdateScreen )
{
	CSettingsPageImpl<CAdvancedOptionsPage>::ApplySettings(UpdateScreen);
}

bool CAdvancedOptionsPage::EnableReset ( void )
{
	if (CSettingsPageImpl<CAdvancedOptionsPage>::EnableReset()) { return true; }
	return false;
}

void CAdvancedOptionsPage::ResetPage()
{
	CSettingsPageImpl<CAdvancedOptionsPage>::ResetPage();
}
