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

class CGameGeneralPage :
    public CSettingsPageImpl<CGameGeneralPage>,
    public CSettingsPage
{
    BEGIN_MSG_MAP_EX(CGameGeneralPage)
        COMMAND_HANDLER_EX(IDC_RDRAM_SIZE, LBN_SELCHANGE, ComboBoxChanged)
        COMMAND_HANDLER_EX(IDC_SAVE_TYPE, LBN_SELCHANGE, ComboBoxChanged)
        COMMAND_HANDLER_EX(IDC_COUNTFACT, LBN_SELCHANGE, ComboBoxChanged)
        COMMAND_ID_HANDLER_EX(IDC_ROM_32BIT, CheckBoxChanged)
        COMMAND_ID_HANDLER_EX(IDC_SYNC_AUDIO, CheckBoxChanged)
        COMMAND_ID_HANDLER_EX(IDC_ROM_FIXEDAUDIO, CheckBoxChanged)
        COMMAND_ID_HANDLER_EX(IDC_USE_TLB, CheckBoxChanged)
        COMMAND_ID_HANDLER_EX(IDC_DELAY_DP, CheckBoxChanged)
        COMMAND_ID_HANDLER_EX(IDC_DELAY_SI, CheckBoxChanged)
        COMMAND_ID_HANDLER_EX(IDC_AUDIO_SIGNAL, CheckBoxChanged)
        COMMAND_HANDLER_EX(IDC_VIREFRESH, EN_UPDATE, EditBoxChanged)
        COMMAND_HANDLER_EX(IDC_COUNTPERBYTE, EN_UPDATE, EditBoxChanged)
        COMMAND_HANDLER_EX(IDC_OVER_CLOCK_MODIFIER, EN_UPDATE, EditBoxChanged)
        END_MSG_MAP()

    enum { IDD = IDD_Settings_GameGeneral };

public:
    CGameGeneralPage(HWND hParent, const RECT & rcDispay);

    LanguageStringID PageTitle(void) { return TAB_ROMSETTINGS; }
    void             HidePage(void);
    void             ShowPage(void);
    void             ApplySettings(bool UpdateScreen);
    bool             EnableReset(void);
    void             ResetPage(void);
};
