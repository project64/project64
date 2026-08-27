#pragma once

#include "SettingsPage.h"
#include <Project64-core/Plugin.h>
#include <Project64/Plugins/PluginList.h>

class CGamePluginPage :
    public CSettingsPageImpl<CGamePluginPage>,
    public CSettingsPage
{
    BEGIN_MSG_MAP_EX(CGamePluginPage)
    {
        COMMAND_HANDLER_EX(GFX_LIST, LBN_SELCHANGE, GfxPluginChanged);
        COMMAND_HANDLER_EX(AUDIO_LIST, LBN_SELCHANGE, AudioPluginChanged);
        COMMAND_HANDLER_EX(CONT_LIST, LBN_SELCHANGE, ControllerPluginChanged);
        COMMAND_HANDLER_EX(RSP_LIST, LBN_SELCHANGE, RspPluginChanged);
        COMMAND_ID_HANDLER_EX(GFX_ABOUT, GfxPluginAbout);
        COMMAND_ID_HANDLER_EX(AUDIO_ABOUT, AudioPluginAbout);
        COMMAND_ID_HANDLER_EX(CONT_ABOUT, ControllerPluginAbout);
        COMMAND_ID_HANDLER_EX(RSP_ABOUT, RspPluginAbout);
        COMMAND_ID_HANDLER_EX(IDC_HLE_GFX, HleGfxChanged);
        COMMAND_ID_HANDLER_EX(IDC_HLE_AUDIO, HleAudioChanged);
    }
    END_MSG_MAP()

    enum
    {
        IDD = IDD_Settings_GamePlugin
    };

public:
    CGamePluginPage(HWND hParent, const RECT & rcDispay);

    LanguageStringID PageTitle(void)
    {
        return TAB_PLUGIN;
    }
    void HidePage(void);
    void ShowPage(void);
    void ApplySettings(bool UpdateScreen);
    bool EnableReset(void);
    void ResetPage(void);
    bool PageAccessible(bool AdvancedMode)
    {
        return CSettingsPageImpl<CGamePluginPage>::PageAccessible(AdvancedMode);
    }

private:
    void GfxPluginAbout(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        ShowAboutButton(GFX_LIST);
    }
    void AudioPluginAbout(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        ShowAboutButton(AUDIO_LIST);
    }
    void ControllerPluginAbout(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        ShowAboutButton(CONT_LIST);
    }
    void RspPluginAbout(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        ShowAboutButton(RSP_LIST);
    }

    void GfxPluginChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        PluginItemChanged(GFX_LIST, GFX_ABOUT);
    }
    void AudioPluginChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        PluginItemChanged(AUDIO_LIST, AUDIO_ABOUT);
    }
    void ControllerPluginChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        PluginItemChanged(CONT_LIST, CONT_ABOUT);
    }
    void RspPluginChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
    {
        PluginItemChanged(RSP_LIST, RSP_ABOUT);
    }

    void HleGfxChanged(UINT Code, int id, HWND ctl);
    void HleAudioChanged(UINT Code, int id, HWND ctl);

    void ShowAboutButton(int id);
    void PluginItemChanged(int id, int AboutID, bool bSetChanged = true);

    void AddPlugins(int ListId, SettingID Type, PLUGIN_TYPE PluginType);
    void UpdatePageSettings(void);
    void ApplyComboBoxes(void);
    bool ResetComboBox(CModifiedComboBox & ComboBox, SettingID Type);

    CPartialGroupBox m_GfxGroup, m_AudioGroup, m_ControlGroup, m_RspGroup;
    CPluginList m_PluginList;
};
