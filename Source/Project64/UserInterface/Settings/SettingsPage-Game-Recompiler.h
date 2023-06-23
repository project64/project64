#pragma once
#include "SettingsPage.h"

class CGameRecompilePage :
    public CSettingsPageImpl<CGameRecompilePage>,
    public CSettingsPage
{
    BEGIN_MSG_MAP_EX(CGameRecompilePage)
    {
        COMMAND_HANDLER_EX(IDC_CPU_TYPE, LBN_SELCHANGE, ComboBoxChanged);
        COMMAND_HANDLER_EX(IDC_FUNCFIND, LBN_SELCHANGE, ComboBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_ROM_REGCACHE, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_ROM_FASTSP, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_BLOCK_LINKING, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_ROM_32BIT, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_SMM_CACHE, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_SMM_DMA, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_SMM_VALIDATE, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_SMM_TLB, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_SMM_PROTECT, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_SMM_STORE, CheckBoxChanged);
    }
    END_MSG_MAP()

    enum
    {
        IDD = IDD_Settings_GameRecompiler
    };

public:
    CGameRecompilePage(HWND hParent, const RECT & rcDispay);

    LanguageStringID PageTitle(void)
    {
        return TAB_RECOMPILER;
    }
    void HidePage(void);
    void ShowPage(void);
    void ApplySettings(bool UpdateScreen);
    bool EnableReset(void);
    void ResetPage(void);
    bool PageAccessible(bool AdvancedMode)
    {
        return CSettingsPageImpl<CGameRecompilePage>::PageAccessible(AdvancedMode);
    }

private:
    CPartialGroupBox m_SelfModGroup;
};
