#include "stdafx.h"

#include "SettingsPage.h"

CDefaultsOptionsPage::CDefaultsOptionsPage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }
    UpdatePageSettings();

    SetDlgItemText(IDC_MEMORY_SIZE_KNOWN_TEXT, wGS(ROM_MEM_SIZE_KNOWN).c_str());
    SetDlgItemText(IDC_MEMORY_SIZE_UNKOWN_TEXT, wGS(ROM_MEM_SIZE_UNKNOWN).c_str());
    SetDlgItemText(IDC_HLE_GFX, wGS(PLUG_HLE_GFX).c_str());
    SetDlgItemText(IDC_VIREFESH_TEXT, wGS(ROM_VIREFRESH).c_str());
    SetDlgItemText(IDC_COUNTPERBYTE_TEXT, wGS(ROM_COUNTPERBYTE).c_str());
    SetDlgItemText(IDC_COUNTFACT_TEXT, wGS(ROM_COUNTER_FACTOR).c_str());
    SetDlgItemText(IDC_ROM_FIXEDAUDIO, wGS(ROM_FIXED_AUDIO).c_str());
    SetDlgItemText(IDC_SYNC_AUDIO, wGS(ROM_SYNC_AUDIO).c_str());
    SetDlgItemText(IDC_RANDOMIZE_SIPI_INTERRUPTS, wGS(ROM_RANDOMIZE_SIPI_INTERRUPTS).c_str());
    SetDlgItemText(IDC_PROTECT_MEMORY, wGS(ADVANCE_SMM_PROTECT).c_str());
    SetDlgItemText(IDC_DISKSEEKTIMING_TEXT1, wGS(ROM_DISK_SEEK_TIMING).c_str());

    CModifiedComboBox * ComboBox;
    ComboBox = AddModComboBox(GetDlgItem(IDC_RDRAM_SIZE_KNOWN), Default_RDRamSizeUnknown);
    if (ComboBox)
    {
        ComboBox->SetTextField(GetDlgItem(IDC_MEMORY_SIZE_TEXT));
        ComboBox->AddItem(wGS(RDRAM_4MB).c_str(), 0x400000);
        ComboBox->AddItem(wGS(RDRAM_8MB).c_str(), 0x800000);
    }

    ComboBox = AddModComboBox(GetDlgItem(IDC_RDRAM_SIZE_UNKNOWN), Default_RDRamSizeKnown);
    if (ComboBox)
    {
        ComboBox->SetTextField(GetDlgItem(IDC_MEMORY_SIZE_TEXT));
        ComboBox->AddItem(wGS(RDRAM_4MB).c_str(), 0x400000);
        ComboBox->AddItem(wGS(RDRAM_8MB).c_str(), 0x800000);
    }

    ComboBox = AddModComboBox(GetDlgItem(IDC_COUNTFACT), Default_CounterFactor);
    if (ComboBox)
    {
        ComboBox->SetTextField(GetDlgItem(IDC_COUNTFACT_TEXT));
        ComboBox->AddItem(wGS(NUMBER_1).c_str(), 1);
        ComboBox->AddItem(wGS(NUMBER_2).c_str(), 2);
        ComboBox->AddItem(wGS(NUMBER_3).c_str(), 3);
        ComboBox->AddItem(wGS(NUMBER_4).c_str(), 4);
        ComboBox->AddItem(wGS(NUMBER_5).c_str(), 5);
        ComboBox->AddItem(wGS(NUMBER_6).c_str(), 6);
    }

    CModifiedEditBox * TxtBox = AddModTextBox(GetDlgItem(IDC_VIREFRESH), Default_ViRefreshRate, false);
    TxtBox->SetTextField(GetDlgItem(IDC_VIREFESH_TEXT));
    TxtBox = AddModTextBox(GetDlgItem(IDC_COUNTPERBYTE), Default_AiCountPerBytes, false);
    TxtBox->SetTextField(GetDlgItem(IDC_COUNTPERBYTE_TEXT));

    AddModCheckBox(GetDlgItem(IDC_HLE_GFX), Default_UseHleGfx);
    AddModCheckBox(GetDlgItem(IDC_SYNC_AUDIO), Default_SyncViaAudio);
    AddModCheckBox(GetDlgItem(IDC_ROM_FIXEDAUDIO), Default_FixedAudio);
    AddModCheckBox(GetDlgItem(IDC_RANDOMIZE_SIPI_INTERRUPTS), Default_RandomizeSIPIInterrupts);
    AddModCheckBox(GetDlgItem(IDC_PROTECT_MEMORY), Default_SMM_Protect_Memory);
    AddModCheckBox(GetDlgItem(IDC_DISKSEEKTIMING), Default_DiskSeekTiming);

    ComboBox = AddModComboBox(GetDlgItem(IDC_DISKSEEKTIMING), Default_DiskSeekTiming);
    if (ComboBox)
    {
        //ComboBox->SetTextField(GetDlgItem(IDC_COUNTFACT_TEXT));
        ComboBox->AddItem(wGS(ROM_DISK_SEEK_TIMING_TURBO).c_str(), DiskSeek_Turbo);
        ComboBox->AddItem(wGS(ROM_DISK_SEEK_TIMING_SLOW).c_str(), DiskSeek_Slow);
    }

    if (!g_Settings->LoadBool(Setting_SyncViaAudioEnabled))
    {
        GetDlgItem(IDC_SYNC_AUDIO).EnableWindow(false);
    }
    UpdatePageSettings();
}

void CDefaultsOptionsPage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void CDefaultsOptionsPage::ShowPage()
{
    ShowWindow(SW_SHOW);
}

void CDefaultsOptionsPage::ApplySettings(bool UpdateScreen)
{
    CSettingsPageImpl<CDefaultsOptionsPage>::ApplySettings(UpdateScreen);
}

bool CDefaultsOptionsPage::EnableReset(void)
{
    if (CSettingsPageImpl<CDefaultsOptionsPage>::EnableReset())
    {
        return true;
    }
    return false;
}

void CDefaultsOptionsPage::ResetPage()
{
    CSettingsPageImpl<CDefaultsOptionsPage>::ResetPage();
}

void CDefaultsOptionsPage::UpdatePageSettings(void)
{
    m_InUpdateSettings = true;
    CSettingsPageImpl<CDefaultsOptionsPage>::UpdatePageSettings();
    m_InUpdateSettings = false;
}
