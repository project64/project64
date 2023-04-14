#pragma once

#include "SettingsPage.h"
#include <Project64-core/N64System/N64Types.h>
#include <Project64\UserInterface\WTLControls\ModifiedCheckBox.h>

class CGameGeneralPage :
    public CSettingsPageImpl<CGameGeneralPage>,
    public CSettingsPage
{
    BEGIN_MSG_MAP_EX(CGameGeneralPage)
    {
        COMMAND_HANDLER_EX(IDC_RDRAM_SIZE, LBN_SELCHANGE, ComboBoxChanged);
        COMMAND_HANDLER_EX(IDC_SAVE_TYPE, LBN_SELCHANGE, ComboBoxChanged);
        COMMAND_HANDLER_EX(IDC_COUNTFACT, LBN_SELCHANGE, ComboBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_SYNC_AUDIO, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_ROM_FIXEDAUDIO, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_DELAY_DP, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_DELAY_SI, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_AUDIO_SIGNAL, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_UNALIGNED_DMA, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_RANDOMIZE_SIPI_INTERRUPTS, CheckBoxChanged);
        COMMAND_HANDLER_EX(IDC_VIREFRESH, EN_UPDATE, EditBoxChanged);
        COMMAND_HANDLER_EX(IDC_COUNTPERBYTE, EN_UPDATE, EditBoxChanged);
        COMMAND_HANDLER_EX(IDC_OVER_CLOCK_MODIFIER, EN_UPDATE, EditBoxChanged);
    }
    END_MSG_MAP()

    enum
    {
        IDD = IDD_Settings_GameGeneral
    };

public:
    CGameGeneralPage(HWND hParent, const RECT & rcDispay);

    LanguageStringID PageTitle(void)
    {
        return TAB_ROMSETTINGS;
    }
    void HidePage(void);
    void ShowPage(void);
    void ApplySettings(bool UpdateScreen);
    bool EnableReset(void);
    void ResetPage(void);
};
