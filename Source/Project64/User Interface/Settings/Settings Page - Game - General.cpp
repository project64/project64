#include "../../User Interface.h"
#include "Settings Page.h"
#include "Settings Page - Game - General.h"

CGameGeneralPage::CGameGeneralPage (HWND hParent, const RECT & rcDispay )
{
	if (!Create(hParent,rcDispay))
	{
		return;
	}

	AddModCheckBox(GetDlgItem(IDC_SYNC_AUDIO),Game_SyncViaAudio);
	AddModCheckBox(GetDlgItem(IDC_ROM_SPHACK),Game_SPHack);
	AddModCheckBox(GetDlgItem(IDC_ROM_FIXEDAUDIO),Game_FixedAudio);
	AddModCheckBox(GetDlgItem(IDC_USE_TLB),Game_UseTlb);
	AddModCheckBox(GetDlgItem(IDC_DELAY_SI),Game_DelaySI);
	AddModCheckBox(GetDlgItem(IDC_AUDIO_SIGNAL),Game_RspAudioSignal);

	CModifiedComboBox * ComboBox;
	ComboBox = AddModComboBox(GetDlgItem(IDC_RDRAM_SIZE),Game_RDRamSize);
	if (ComboBox)
	{
		ComboBox->SetTextField(GetDlgItem(IDC_MEMORY_SIZE_TEXT));
		/*if (_Settings->LoadBool(Setting_RdbEditor))
		{
			ComboBox->AddItem(GS(RDRAM_4MB), 4 );
			ComboBox->AddItem(GS(RDRAM_8MB), 8 );
		} else {*/
			ComboBox->AddItem(GS(RDRAM_4MB), 0x400000 );
			ComboBox->AddItem(GS(RDRAM_8MB), 0x800000 );
		//}
	}

	ComboBox = AddModComboBox(GetDlgItem(IDC_SAVE_TYPE),Game_SaveChip);
	if (ComboBox)
	{
		ComboBox->SetTextField(GetDlgItem(IDC_SAVE_TYPE_TEXT));
		ComboBox->AddItem(GS(SAVE_FIRST_USED), SaveChip_Auto );
		ComboBox->AddItem(GS(SAVE_4K_EEPROM),  SaveChip_Eeprom_4K );
		ComboBox->AddItem(GS(SAVE_16K_EEPROM), SaveChip_Eeprom_16K );
		ComboBox->AddItem(GS(SAVE_SRAM),       SaveChip_Sram );
		ComboBox->AddItem(GS(SAVE_FLASHRAM),   SaveChip_FlashRam );
	}

	ComboBox = AddModComboBox(GetDlgItem(IDC_COUNTFACT),Game_CounterFactor);
	if (ComboBox)
	{
		ComboBox->SetTextField(GetDlgItem(IDC_COUNTFACT_TEXT));
		ComboBox->AddItem(GS(NUMBER_1), 1 );
		ComboBox->AddItem(GS(NUMBER_2), 2 );
		ComboBox->AddItem(GS(NUMBER_3), 3 );
		ComboBox->AddItem(GS(NUMBER_4), 4 );
		ComboBox->AddItem(GS(NUMBER_5), 5 );
		ComboBox->AddItem(GS(NUMBER_6), 6 );
	}

	SetDlgItemText(IDC_GOOD_NAME,_Settings->LoadString(Game_GoodName).c_str());

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