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
#include "SettingsPage-Game-DiskDrive.h"

CGameDiskDrivePage::CGameDiskDrivePage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    //Set the text for all gui Items
    SetDlgItemText(IDC_DISKSEEKTIMING_TEXT2, wGS(ROM_DISK_SEEK_TIMING).c_str());

    CModifiedComboBox* ComboBox;
    ComboBox = AddModComboBox(GetDlgItem(IDC_DISKSEEKTIMING2), Game_DiskSeekTiming);
    if (ComboBox)
    {
        //ComboBox->SetTextField(GetDlgItem(IDC_MEMORY_SIZE_TEXT));
        ComboBox->AddItem(wGS(ROM_DISK_SEEK_TIMING_TURBO).c_str(), DiskSeek_Turbo);
        ComboBox->AddItem(wGS(ROM_DISK_SEEK_TIMING_SLOW).c_str(), DiskSeek_Slow);
    }

    UpdatePageSettings();
}

void CGameDiskDrivePage::ShowPage()
{
    ShowWindow(SW_SHOW);
}

void CGameDiskDrivePage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void CGameDiskDrivePage::ApplySettings(bool UpdateScreen)
{
    CSettingsPageImpl<CGameDiskDrivePage>::ApplySettings(UpdateScreen);
}

bool CGameDiskDrivePage::EnableReset(void)
{
    if (CSettingsPageImpl<CGameDiskDrivePage>::EnableReset()) { return true; }
    return false;
}

void CGameDiskDrivePage::ResetPage()
{
    CSettingsPageImpl<CGameDiskDrivePage>::ResetPage();
}
