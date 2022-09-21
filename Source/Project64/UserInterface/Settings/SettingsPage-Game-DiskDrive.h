#pragma once

#include "SettingsPage.h"
#include <Project64\UserInterface\WTLControls\ModifiedCheckBox.h>
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
