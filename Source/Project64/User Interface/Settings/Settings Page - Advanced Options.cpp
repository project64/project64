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

	//Set the text for all gui Items
	SetDlgItemTextW(m_hWnd, IDC_START_ON_ROM_OPEN, GS(ADVANCE_AUTO_START));
	SetDlgItemTextW(m_hWnd, IDC_ZIP, GS(ADVANCE_COMPRESS));
	SetDlgItemTextW(m_hWnd, IDC_DEBUGGER, GS(ADVANCE_DEBUGGER));
	SetDlgItemTextW(m_hWnd, IDC_REMEMBER_CHEAT, GS(OPTION_REMEMBER_CHEAT));
	SetDlgItemTextW(m_hWnd, IDC_CHECK_RUNNING, GS(OPTION_CHECK_RUNNING));
	SetDlgItemTextW(m_hWnd, IDC_DISPLAY_FRAMERATE, GS(OPTION_CHANGE_FR));

	AddModCheckBox(GetDlgItem(IDC_START_ON_ROM_OPEN),Setting_AutoStart);
	AddModCheckBox(GetDlgItem(IDC_ZIP),Setting_AutoZipInstantSave);
	AddModCheckBox(GetDlgItem(IDC_DEBUGGER),Debugger_Enabled);
	AddModCheckBox(GetDlgItem(IDC_REMEMBER_CHEAT),Setting_RememberCheats);
	AddModCheckBox(GetDlgItem(IDC_CHECK_RUNNING),Setting_CheckEmuRunning);
	AddModCheckBox(GetDlgItem(IDC_DISPLAY_FRAMERATE),UserInterface_DisplayFrameRate);

	CModifiedComboBox * ComboBox;
	ComboBox = AddModComboBox(GetDlgItem(IDC_FRAME_DISPLAY_TYPE),UserInterface_FrameDisplayType);
	if (ComboBox)
	{
		ComboBox->AddItemW(GS(STR_FR_VIS), FR_VIs );
		ComboBox->AddItemW(GS(STR_FR_DLS), FR_DLs );
		ComboBox->AddItemW(GS(STR_FR_PERCENT), FR_PERCENT );
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
