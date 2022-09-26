#pragma once

class CGeneralOptionsPage :
    public CSettingsPageImpl<CGeneralOptionsPage>,
    public CSettingsPage
{

    BEGIN_MSG_MAP_EX(CGeneralOptionsPage)
    {
        COMMAND_ID_HANDLER_EX(IDC_AUTOSLEEP, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_LOAD_FULLSCREEN, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_SCREEN_SAVER, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_DISCORD_RPC, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_ENABLE_ENHANCEMENTS, CheckBoxChanged);
        COMMAND_ID_HANDLER_EX(IDC_BASIC_MODE, OnBasicMode);
        COMMAND_HANDLER_EX(IDC_REMEMBER, EN_UPDATE, EditBoxChanged);
        COMMAND_HANDLER_EX(IDC_REMEMBERDIR, EN_UPDATE, EditBoxChanged);
    }
    END_MSG_MAP()

    enum
    {
        IDD = IDD_Settings_General
    };

public:
    CGeneralOptionsPage(CSettingConfig * SettingsConfig, HWND hParent, const RECT & rcDispay);

    LanguageStringID PageTitle(void)
    {
        return TAB_OPTIONS;
    }
    void HidePage(void);
    void ShowPage(void);
    void ApplySettings(bool UpdateScreen);
    bool EnableReset(void);
    void ResetPage(void);

private:
    void OnBasicMode(UINT Code, int id, HWND ctl);
    CSettingConfig * m_SettingsConfig;
};
