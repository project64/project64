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
#include "SettingsPage-Game-General.h"

CGameGeneralPage::CGameGeneralPage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    //Set the text for all gui Items
    SetDlgItemTextW(m_hWnd, IDC_GOOD_NAME_TEXT, wGS(RB_GOODNAME).c_str());

    SetDlgItemTextW(m_hWnd, IDC_MEMORY_SIZE_TEXT, wGS(ROM_MEM_SIZE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_SAVE_TYPE_TEXT, wGS(ROM_SAVE_TYPE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_COUNTFACT_TEXT, wGS(ROM_COUNTER_FACTOR).c_str());
    SetDlgItemTextW(m_hWnd, IDC_VIREFESH_TEXT, wGS(ROM_VIREFRESH).c_str());
    SetDlgItemTextW(m_hWnd, IDC_COUNTPERBYTE_TEXT, wGS(ROM_COUNTPERBYTE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_OVER_CLOCK_MODIFIER_TEXT, wGS(ROM_OVER_CLOCK_MODIFIER).c_str());

    SetDlgItemTextW(m_hWnd, IDC_ROM_32BIT, wGS(ROM_32BIT).c_str());
    SetDlgItemTextW(m_hWnd, IDC_ROM_FIXEDAUDIO, wGS(ROM_FIXED_AUDIO).c_str());
    SetDlgItemTextW(m_hWnd, IDC_DELAY_DP, wGS(ROM_DELAY_DP).c_str());
    SetDlgItemTextW(m_hWnd, IDC_SYNC_AUDIO, wGS(ROM_SYNC_AUDIO).c_str());
    SetDlgItemTextW(m_hWnd, IDC_USE_TLB, wGS(ROM_USE_TLB).c_str());
    SetDlgItemTextW(m_hWnd, IDC_DELAY_SI, wGS(ROM_DELAY_SI).c_str());
    SetDlgItemTextW(m_hWnd, IDC_AUDIO_SIGNAL, wGS(ROM_AUDIO_SIGNAL).c_str());

    AddModCheckBox(GetDlgItem(IDC_ROM_32BIT), Game_32Bit);
    AddModCheckBox(GetDlgItem(IDC_SYNC_AUDIO), Game_SyncViaAudio);
    AddModCheckBox(GetDlgItem(IDC_ROM_FIXEDAUDIO), Game_FixedAudio);
    AddModCheckBox(GetDlgItem(IDC_USE_TLB), Game_UseTlb);
    AddModCheckBox(GetDlgItem(IDC_DELAY_DP), Game_DelayDP);
    AddModCheckBox(GetDlgItem(IDC_DELAY_SI), Game_DelaySI);
    AddModCheckBox(GetDlgItem(IDC_AUDIO_SIGNAL), Game_RspAudioSignal);

    CModifiedComboBox * ComboBox;
    ComboBox = AddModComboBox(GetDlgItem(IDC_RDRAM_SIZE), Game_RDRamSize);
    if (ComboBox)
    {
        ComboBox->SetTextField(GetDlgItem(IDC_MEMORY_SIZE_TEXT));
        ComboBox->AddItemW(wGS(RDRAM_4MB).c_str(), 0x400000);
        ComboBox->AddItemW(wGS(RDRAM_8MB).c_str(), 0x800000);
    }

    ComboBox = AddModComboBox(GetDlgItem(IDC_SAVE_TYPE), Game_SaveChip);
    if (ComboBox)
    {
        ComboBox->SetTextField(GetDlgItem(IDC_SAVE_TYPE_TEXT));
        ComboBox->AddItemW(wGS(SAVE_FIRST_USED).c_str(), (uint32_t)(int64_t)SaveChip_Auto);
        ComboBox->AddItemW(wGS(SAVE_4K_EEPROM).c_str(), SaveChip_Eeprom_4K);
        ComboBox->AddItemW(wGS(SAVE_16K_EEPROM).c_str(), SaveChip_Eeprom_16K);
        ComboBox->AddItemW(wGS(SAVE_SRAM).c_str(), SaveChip_Sram);
        ComboBox->AddItemW(wGS(SAVE_FLASHRAM).c_str(), SaveChip_FlashRam);
    }

    ComboBox = AddModComboBox(GetDlgItem(IDC_COUNTFACT), Game_CounterFactor);
    if (ComboBox)
    {
        ComboBox->SetTextField(GetDlgItem(IDC_COUNTFACT_TEXT));
        ComboBox->AddItemW(wGS(NUMBER_1).c_str(), 1);
        ComboBox->AddItemW(wGS(NUMBER_2).c_str(), 2);
        ComboBox->AddItemW(wGS(NUMBER_3).c_str(), 3);
        ComboBox->AddItemW(wGS(NUMBER_4).c_str(), 4);
        ComboBox->AddItemW(wGS(NUMBER_5).c_str(), 5);
        ComboBox->AddItemW(wGS(NUMBER_6).c_str(), 6);
    }

    SetDlgItemText(IDC_GOOD_NAME, g_Settings->LoadStringVal(Rdb_GoodName).c_str());

    CModifiedEditBox * TxtBox = AddModTextBox(GetDlgItem(IDC_VIREFRESH), Game_ViRefreshRate, false);
    TxtBox->SetTextField(GetDlgItem(IDC_VIREFESH_TEXT));

    TxtBox = AddModTextBox(GetDlgItem(IDC_COUNTPERBYTE), Game_AiCountPerBytes, false);
    TxtBox->SetTextField(GetDlgItem(IDC_COUNTPERBYTE_TEXT));

    TxtBox = AddModTextBox(GetDlgItem(IDC_OVER_CLOCK_MODIFIER), Game_OverClockModifier, false);
    TxtBox->SetTextField(GetDlgItem(IDC_OVER_CLOCK_MODIFIER_TEXT));

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

void CGameGeneralPage::ApplySettings(bool UpdateScreen)
{
    CSettingsPageImpl<CGameGeneralPage>::ApplySettings(UpdateScreen);
}

bool CGameGeneralPage::EnableReset(void)
{
    if (CSettingsPageImpl<CGameGeneralPage>::EnableReset()) { return true; }
    return false;
}

void CGameGeneralPage::ResetPage()
{
    CSettingsPageImpl<CGameGeneralPage>::ResetPage();
}
