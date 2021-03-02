#pragma once

class COptionsGameBrowserPage :
    public CSettingsPageImpl<COptionsGameBrowserPage>,
    public CSettingsPage
{
    BEGIN_MSG_MAP_EX(COptionsGameBrowserPage)
        COMMAND_HANDLER_EX(IDC_ADD, BN_CLICKED, AddFieldClicked)
        COMMAND_HANDLER_EX(IDC_REMOVE, BN_CLICKED, RemoveFieldClicked)
        COMMAND_HANDLER_EX(IDC_UP, BN_CLICKED, MoveFieldUpClicked)
        COMMAND_HANDLER_EX(IDC_DOWN, BN_CLICKED, MoveFieldDownClicked)
        COMMAND_ID_HANDLER_EX(IDC_USE_ROMBROWSER, UseRomBrowserChanged)
        COMMAND_ID_HANDLER_EX(IDC_RECURSION, CheckBoxChanged)
        COMMAND_ID_HANDLER_EX(IDC_SHOW_FILE_EXTENSIONS, CheckBoxChanged)
    END_MSG_MAP()

    enum { IDD = IDD_Settings_RomBrowser };

public:
    COptionsGameBrowserPage(HWND hParent, const RECT & rcDispay);

    LanguageStringID PageTitle(void) { return TAB_ROMSELECTION; }
    void             HidePage(void);
    void             ShowPage(void);
    void             ApplySettings(bool UpdateScreen);
    bool             EnableReset(void);
    void             ResetPage(void);

private:
    void  UpdatePageSettings(void);
    void  UpdateFieldList(const ROMBROWSER_FIELDS_LIST & Fields);
    void  AddFieldClicked(UINT Code, int id, HWND ctl);
    void  RemoveFieldClicked(UINT Code, int id, HWND ctl);
    void  MoveFieldUpClicked(UINT Code, int id, HWND ctl);
    void  MoveFieldDownClicked(UINT Code, int id, HWND ctl);
    void  UseRomBrowserChanged(UINT Code, int id, HWND ctl);
    void  FixCtrlState(void);

    ROMBROWSER_FIELDS_LIST m_Fields;
    CListBox               m_Avaliable, m_Using;
    bool                   m_OrderChanged, m_OrderReset;
};
