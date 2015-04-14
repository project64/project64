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
#include "Settings Page - Game - General.h"

CGameGeneralPage::CGameGeneralPage (HWND hParent, const RECT & rcDispay )
{
	if (!Create(hParent,rcDispay))
	{
		return;
	}

	//Set the text for all gui Items
	SetDlgItemTextW(m_hWnd, IDC_GOOD_NAME_TEXT, GS(RB_GOODNAME));

	SetDlgItemTextW(m_hWnd, IDC_MEMORY_SIZE_TEXT, GS(ROM_MEM_SIZE));
	SetDlgItemTextW(m_hWnd, IDC_SAVE_TYPE_TEXT, GS(ROM_SAVE_TYPE));
	SetDlgItemTextW(m_hWnd, IDC_COUNTFACT_TEXT, GS(ROM_COUNTER_FACTOR));
	SetDlgItemTextW(m_hWnd, IDC_VIREFESH_TEXT, GS(ROM_VIREFRESH));
	SetDlgItemTextW(m_hWnd, IDC_COUNTPERBYTE_TEXT, GS(ROM_COUNTPERBYTE));

	SetDlgItemTextW(m_hWnd, IDC_ROM_32BIT, GS(ROM_32BIT));
	SetDlgItemTextW(m_hWnd, IDC_ROM_FIXEDAUDIO, GS(ROM_FIXED_AUDIO));
	SetDlgItemTextW(m_hWnd, IDC_DELAY_DP, GS(ROM_DELAY_DP));
	SetDlgItemTextW(m_hWnd, IDC_SYNC_AUDIO, GS(ROM_SYNC_AUDIO));
	SetDlgItemTextW(m_hWnd, IDC_USE_TLB, GS(ROM_USE_TLB));
	SetDlgItemTextW(m_hWnd, IDC_DELAY_SI, GS(ROM_DELAY_SI)); 
	SetDlgItemTextW(m_hWnd,	IDC_AUDIO_SIGNAL, GS(ROM_AUDIO_SIGNAL));

	AddModCheckBox(GetDlgItem(IDC_ROM_32BIT),Game_32Bit);
	AddModCheckBox(GetDlgItem(IDC_SYNC_AUDIO),Game_SyncViaAudio);
	AddModCheckBox(GetDlgItem(IDC_ROM_FIXEDAUDIO),Game_FixedAudio);
	AddModCheckBox(GetDlgItem(IDC_USE_TLB),Game_UseTlb);
	AddModCheckBox(GetDlgItem(IDC_DELAY_DP),Game_DelayDP);
	AddModCheckBox(GetDlgItem(IDC_DELAY_SI),Game_DelaySI);
	AddModCheckBox(GetDlgItem(IDC_AUDIO_SIGNAL),Game_RspAudioSignal);

	CModifiedComboBox * ComboBox;
	ComboBox = AddModComboBox(GetDlgItem(IDC_RDRAM_SIZE),Game_RDRamSize);
	if (ComboBox)
	{
		ComboBox->SetTextField(GetDlgItem(IDC_MEMORY_SIZE_TEXT));
		ComboBox->AddItemW(GS(RDRAM_4MB), 0x400000 );
		ComboBox->AddItemW(GS(RDRAM_8MB), 0x800000 );
	}

	ComboBox = AddModComboBox(GetDlgItem(IDC_SAVE_TYPE),Game_SaveChip);
	if (ComboBox)
	{
		ComboBox->SetTextField(GetDlgItem(IDC_SAVE_TYPE_TEXT));
		ComboBox->AddItemW(GS(SAVE_FIRST_USED), (WPARAM)SaveChip_Auto );
		ComboBox->AddItemW(GS(SAVE_4K_EEPROM),  SaveChip_Eeprom_4K );
		ComboBox->AddItemW(GS(SAVE_16K_EEPROM), SaveChip_Eeprom_16K );
		ComboBox->AddItemW(GS(SAVE_SRAM),       SaveChip_Sram );
		ComboBox->AddItemW(GS(SAVE_FLASHRAM),   SaveChip_FlashRam );
	}

	ComboBox = AddModComboBox(GetDlgItem(IDC_COUNTFACT),Game_CounterFactor);
	if (ComboBox)
	{
		ComboBox->SetTextField(GetDlgItem(IDC_COUNTFACT_TEXT));
		ComboBox->AddItemW(GS(NUMBER_1), 1 );
		ComboBox->AddItemW(GS(NUMBER_2), 2 );
		ComboBox->AddItemW(GS(NUMBER_3), 3 );
		ComboBox->AddItemW(GS(NUMBER_4), 4 );
		ComboBox->AddItemW(GS(NUMBER_5), 5 );
		ComboBox->AddItemW(GS(NUMBER_6), 6 );
	}

	SetDlgItemText(IDC_GOOD_NAME,g_Settings->LoadString(Game_GoodName).c_str());

	CModifiedEditBox * TxtBox = AddModTextBox(GetDlgItem(IDC_VIREFRESH),Game_ViRefreshRate, false);
	TxtBox->SetTextField(GetDlgItem(IDC_VIREFESH_TEXT));

	TxtBox = AddModTextBox(GetDlgItem(IDC_COUNTPERBYTE),Game_AiCountPerBytes, false);
	TxtBox->SetTextField(GetDlgItem(IDC_COUNTPERBYTE_TEXT));

	UpdatePageSettings();
}

void CGameGeneralPage::ShowPage()
{
	ShowWindow(SW_SHOW);
}

void CGameGeneralPage::HidePage()
{
	ShowWindow(SW_HIDE);
}

void CGameGeneralPage::ApplySettings( bool UpdateScreen )
{
	CSettingsPageImpl<CGameGeneralPage>::ApplySettings(UpdateScreen);
}

bool CGameGeneralPage::EnableReset ( void )
{
	if (CSettingsPageImpl<CGameGeneralPage>::EnableReset()) { return true; }
	return false;
}


void CGameGeneralPage::ResetPage()
{
	CSettingsPageImpl<CGameGeneralPage>::ResetPage();
}
