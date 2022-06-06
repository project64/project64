#pragma once

class COptionsDirectoriesPage :
    public CDialogImpl<COptionsDirectoriesPage>,
    public CSettingsPage
{
    BEGIN_MSG_MAP_EX(COptionsDirectoriesPage)
        COMMAND_ID_HANDLER_EX(IDC_SELECT_PLUGIN_DIR, SelectPluginDir)
        COMMAND_ID_HANDLER_EX(IDC_SELECT_AUTO_DIR, SelectAutoDir)
        COMMAND_ID_HANDLER_EX(IDC_SELECT_INSTANT_DIR, SelectInstantDir)
        COMMAND_ID_HANDLER_EX(IDC_SELECT_SNAP_DIR, SelectSnapShotDir)
        COMMAND_ID_HANDLER_EX(IDC_SELECT_TEXTURE_DIR, SelectTextureDir)
        COMMAND_HANDLER_EX(IDC_PLUGIN_DIR, EN_UPDATE, PluginDirChanged)
        COMMAND_HANDLER_EX(IDC_AUTO_DIR, EN_UPDATE, AutoSaveDirChanged)
        COMMAND_HANDLER_EX(IDC_INSTANT_DIR, EN_UPDATE, InstantSaveDirChanged)
        COMMAND_HANDLER_EX(IDC_SNAP_DIR, EN_UPDATE, SnapShotDirChanged)
        COMMAND_HANDLER_EX(IDC_TEXTURE_DIR, EN_UPDATE, TextureDirChanged)

        COMMAND_HANDLER_EX(IDC_PLUGIN_DEFAULT, BN_CLICKED, UseSelectedClicked)
        COMMAND_HANDLER_EX(IDC_PLUGIN_OTHER, BN_CLICKED, UseSelectedClicked)
        COMMAND_HANDLER_EX(IDC_AUTO_DEFAULT, BN_CLICKED, UseSelectedClicked)
        COMMAND_HANDLER_EX(IDC_AUTO_OTHER, BN_CLICKED, UseSelectedClicked)
        COMMAND_HANDLER_EX(IDC_INSTANT_DEFAULT, BN_CLICKED, UseSelectedClicked)
        COMMAND_HANDLER_EX(IDC_INSTANT_OTHER, BN_CLICKED, UseSelectedClicked)
        COMMAND_HANDLER_EX(IDC_SNAP_DEFAULT, BN_CLICKED, UseSelectedClicked)
        COMMAND_HANDLER_EX(IDC_SNAP_OTHER, BN_CLICKED, UseSelectedClicked)
        COMMAND_HANDLER_EX(IDC_TEXTURE_DEFAULT, BN_CLICKED, UseSelectedClicked)
        COMMAND_HANDLER_EX(IDC_TEXTURE_OTHER, BN_CLICKED, UseSelectedClicked)
    END_MSG_MAP()

    enum { IDD = IDD_Settings_Directory };

public:
    COptionsDirectoriesPage(HWND hParent, const RECT & rcDispay);

    LanguageStringID PageTitle(void) { return TAB_DIRECTORY; }
    void             HidePage(void);
    void             ShowPage(void);
    void             ApplySettings(bool UpdateScreen);
    bool             EnableReset(void);
    void             ResetPage(void);

private:
    void  SelectPluginDir(UINT Code, int id, HWND ctl);
    void  SelectAutoDir(UINT Code, int id, HWND ctl);
    void  SelectInstantDir(UINT Code, int id, HWND ctl);
    void  SelectSnapShotDir(UINT Code, int id, HWND ctl);
    void  SelectTextureDir(UINT Code, int id, HWND ctl);
    void  PluginDirChanged(UINT Code, int id, HWND ctl);
    void  AutoSaveDirChanged(UINT Code, int id, HWND ctl);
    void  InstantSaveDirChanged(UINT Code, int id, HWND ctl);
    void  SnapShotDirChanged(UINT Code, int id, HWND ctl);
    void  TextureDirChanged(UINT Code, int id, HWND ctl);
    void  UseSelectedClicked(UINT Code, int id, HWND ctl);
    void  UpdatePageSettings(void);
    void  SelectDirectory(LanguageStringID Title, CModifiedEditBox & EditBox, CModifiedButton & Default, CModifiedButton & selected);

    void  UpdateDirectory(CModifiedEditBox & EditBox, SettingID Type);
    void  UpdateDefaultSelected(CModifiedButton & Button, SettingID Type);

    void  ResetDirectory(CModifiedEditBox & EditBox, SettingID Type);
    void  ResetDefaultSelected(CModifiedButton & ButtonDefault, CModifiedButton & ButtonSelected, SettingID Type);

    static int CALLBACK SelectDirCallBack(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM lpData);

    CPartialGroupBox m_PluginGroup, m_AutoSaveGroup, m_InstantSaveGroup,
        m_ScreenShotGroup, m_TextureGroup;
    CModifiedEditBox m_PluginDir, m_AutoSaveDir, m_InstantSaveDir,
        m_ScreenShotDir, m_TextureDir;

    CModifiedButton  m_PluginDefault, m_PluginSelected, m_AutoSaveDefault, m_AutoSaveSelected,
        m_InstantDefault, m_InstantSelected, m_ScreenShotDefault, m_ScreenShotSelected,
        m_TextureDefault, m_TextureSelected;

    bool m_InUpdateSettings;
};
