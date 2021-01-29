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
#pragma once

#include "../WTLControls/ModifiedCheckBox.h"
#include <Project64-core/N64System/N64Types.h>

class CGameDiskDrivePage :
    public CSettingsPageImpl<CGameDiskDrivePage>,
    public CSettingsPage
{
    BEGIN_MSG_MAP_EX(CGameDiskDrivePage)
        COMMAND_HANDLER_EX(IDC_DISKSEEKTIMING2, LBN_SELCHANGE, ComboBoxChanged)
    END_MSG_MAP()

    enum { IDD = IDD_Settings_GameDiskDrive };

public:
    CGameDiskDrivePage(HWND hParent, const RECT & rcDispay);

    LanguageStringID PageTitle(void) { return TAB_DISKSETTINGS; }
    void             HidePage(void);
    void             ShowPage(void);
    void             ApplySettings(bool UpdateScreen);
    bool             EnableReset(void);
    void             ResetPage(void);
};
