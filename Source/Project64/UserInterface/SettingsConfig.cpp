#include "stdafx.h"

#include "SettingsConfig.h"
#include "Settings/SettingsPage.h"
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>

CSettingConfig::CSettingConfig(bool bJustGameSetting /* = false */) :
    m_CurrentPage(NULL),
    m_GeneralOptionsPage(NULL),
    m_AdvancedPage(NULL),
    m_DefaultsPage(NULL),
    m_GameConfig(bJustGameSetting),
    m_bTVNSelChangedSupported(false)
{
}

CSettingConfig::~CSettingConfig()
{
    for (SETTING_SECTIONS::const_iterator iter = m_Sections.begin(); iter != m_Sections.end(); iter++)
    {
        CConfigSettingSection * Section = *iter;
        delete Section;
    }
}

void CSettingConfig::Display(void * ParentWindow)
{
    if (g_BaseSystem)
    {
        g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_Settings);
    }

    BOOL result = m_thunk.Init(NULL, NULL);
    if (result)
    {
        _AtlWinModule.AddCreateWndData(&m_thunk.cd, this);
#ifdef _DEBUG
        m_bModal = true;
#endif //_DEBUG
        ::DialogBoxParamW(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCEW(IDD), (HWND)ParentWindow, StartDialogProc, NULL);
    }

    if (g_BaseSystem)
    {
        g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_Settings);
    }
}

void CSettingConfig::UpdateAdvanced(bool AdvancedMode)
{
    UpdateAdvanced(AdvancedMode, m_PagesTreeList.GetRootItem());
    BoldChangedPages(m_PagesTreeList.GetRootItem());
}

void CSettingConfig::UpdateAdvanced(bool AdvancedMode, HTREEITEM hItem)
{
    while (hItem)
    {
        CSettingsPage * Page = (CSettingsPage *)m_PagesTreeList.GetItemData(hItem);
        if (!AdvancedMode && (Page == m_AdvancedPage || Page == m_DefaultsPage))
        {
			HTREEITEM hPage = hItem;
			hItem = m_PagesTreeList.GetNextSiblingItem(hItem);
			m_PagesTreeList.DeleteItem(hPage);
        }
        else if (AdvancedMode && Page == m_GeneralOptionsPage)
        {
            m_PagesTreeList.InsertItemW(TVIF_TEXT | TVIF_PARAM, wGS(m_AdvancedPage->PageTitle()).c_str(), 0, 0, 0, 0, (ULONG)m_AdvancedPage, hItem, TVI_FIRST);
            m_PagesTreeList.InsertItemW(TVIF_TEXT | TVIF_PARAM, wGS(m_DefaultsPage->PageTitle()).c_str(), 0, 0, 0, 0, (ULONG)m_DefaultsPage, hItem, TVI_FIRST);
            break;
        }
		else
		{
			UpdateAdvanced(AdvancedMode, m_PagesTreeList.GetChildItem(hItem));
			hItem = m_PagesTreeList.GetNextSiblingItem(hItem);
		}
    }
}

LRESULT	CSettingConfig::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    stdstr ConfigRomTitle, GameIni(g_Settings->LoadStringVal(Game_IniKey));

    if (!GameIni.empty())
    {
        ConfigRomTitle.Format("Config: %s", g_Settings->LoadStringVal(Rdb_GoodName).c_str());
    }

    RECT rcSettingInfo;
    ::GetWindowRect(GetDlgItem(IDC_SETTING_INFO), &rcSettingInfo);
    ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rcSettingInfo, 2);

    CConfigSettingSection * SettingsSection;

    //Set the text for all gui Items
    SetDlgItemText(IDC_RESET_PAGE, wGS(BOTTOM_RESET_PAGE).c_str());
    SetDlgItemText(IDC_RESET_ALL, wGS(BOTTOM_RESET_ALL).c_str());
    SetDlgItemText(IDOK, wGS(CHEAT_OK).c_str());
    SetDlgItemText(IDCANCEL, wGS(CHEAT_CANCEL).c_str());
    SetDlgItemText(IDAPPLY, wGS(BOTTOM_APPLY).c_str());

    if (m_GameConfig)
    {
        if (g_Settings->LoadBool(Setting_RdbEditor))
        {
            SetWindowText(stdstr_f("%s ** RDB Edit Mode **", ConfigRomTitle.c_str()).ToUTF16().c_str());
        }
        else
        {
            SetWindowText(ConfigRomTitle.ToUTF16().c_str());
        }
    }
    else
    {
        if (g_Settings->LoadBool(Setting_RdbEditor))
        {
            SetWindowText(stdstr_f("%s ** RDB Edit Mode **", GS(OPTIONS_TITLE)).ToUTF16().c_str());
        }
        else
        {
            ::SetWindowTextW(m_hWnd, wGS(OPTIONS_TITLE).c_str());
        }

        if (UISettingsLoadBool(Setting_PluginPageFirst))
        {
            SettingsSection = new CConfigSettingSection(wGS(TAB_PLUGIN).c_str());
            SettingsSection->AddPage(new COptionPluginPage(this->m_hWnd, rcSettingInfo));
            m_Sections.push_back(SettingsSection);
        }

        m_GeneralOptionsPage = new CGeneralOptionsPage(this, this->m_hWnd, rcSettingInfo);
        m_AdvancedPage = new CAdvancedOptionsPage(this->m_hWnd, rcSettingInfo);
        m_DefaultsPage = new CDefaultsOptionsPage(this->m_hWnd, rcSettingInfo);
        m_DiskDrivePage = new CDiskDrivePage(this->m_hWnd, rcSettingInfo);

        SettingsSection = new CConfigSettingSection(wGS(TAB_OPTIONS).c_str());
        SettingsSection->AddPage(m_GeneralOptionsPage);
        SettingsSection->AddPage(m_AdvancedPage);
        SettingsSection->AddPage(m_DefaultsPage);
        SettingsSection->AddPage(new COptionsDirectoriesPage(this->m_hWnd, rcSettingInfo));
        SettingsSection->AddPage(m_DiskDrivePage);
        m_Sections.push_back(SettingsSection);

        SettingsSection = new CConfigSettingSection(wGS(TAB_ROMSELECTION).c_str());
        SettingsSection->AddPage(new COptionsGameBrowserPage(this->m_hWnd, rcSettingInfo));
        m_Sections.push_back(SettingsSection);

        SettingsSection = new CConfigSettingSection(wGS(TAB_SHORTCUTS).c_str());
        SettingsSection->AddPage(new COptionsShortCutsPage(this->m_hWnd, rcSettingInfo));
        m_Sections.push_back(SettingsSection);

        if (!UISettingsLoadBool(Setting_PluginPageFirst))
        {
            SettingsSection = new CConfigSettingSection(wGS(TAB_PLUGIN).c_str());
            SettingsSection->AddPage(new COptionPluginPage(this->m_hWnd, rcSettingInfo));
            m_Sections.push_back(SettingsSection);
        }
    }

    //Game Settings
    if (!GameIni.empty())
    {
        CConfigSettingSection * GameSettings = new CConfigSettingSection(ConfigRomTitle.ToUTF16().c_str());
        GameSettings->AddPage(new CGameGeneralPage(this->m_hWnd, rcSettingInfo));
        GameSettings->AddPage(new CGameRecompilePage(this->m_hWnd, rcSettingInfo));
        GameSettings->AddPage(new CGamePluginPage(this->m_hWnd, rcSettingInfo));
        if (g_Settings->LoadBool(Setting_RdbEditor))
        {
            GameSettings->AddPage(new CGameStatusPage(this->m_hWnd, rcSettingInfo));
        }
        m_Sections.push_back(GameSettings);
    }

    m_PagesTreeList.Attach(GetDlgItem(IDC_PAGELIST));

    bool bFirstItem = true;
    bool HideAdvanced = g_Settings->LoadBool(UserInterface_BasicMode);
    for (SETTING_SECTIONS::const_iterator iter = m_Sections.begin(); iter != m_Sections.end(); iter++)
    {
        CConfigSettingSection * Section = *iter;

        HTREEITEM hSectionItem = NULL;

        for (size_t i = 0; i < Section->GetPageCount(); i++)
        {
            CSettingsPage * Page = Section->GetPage(i);
            if (HideAdvanced && (Page == m_AdvancedPage || Page == m_DefaultsPage))
            {
                continue;
            }
            if (i == 0)
            {
                hSectionItem = m_PagesTreeList.InsertItemW(TVIF_TEXT | TVIF_PARAM, Section->GetPageTitle(), 0, 0, 0, 0, (ULONG)Page, TVI_ROOT, TVI_LAST);
                continue;
            }
            if (hSectionItem == NULL)
            {
                continue;
            }
            m_PagesTreeList.InsertItemW(TVIF_TEXT | TVIF_PARAM, wGS(Page->PageTitle()).c_str(), 0, 0, 0, 0, (ULONG)Page, hSectionItem, TVI_LAST);
        }
        if (bFirstItem && hSectionItem != NULL)
        {
            bFirstItem = false;
            m_PagesTreeList.Expand(hSectionItem);
            m_PagesTreeList.SelectItem(hSectionItem);
        }
    }

    BoldChangedPages(m_PagesTreeList.GetRootItem());
    return TRUE;
}

LRESULT CSettingConfig::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDAPPLY:
        ApplySettings(true);
        break;
    case IDOK:
        ApplySettings(false);
        EndDialog(1);
        break;
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDC_RESET_PAGE:
        if (m_CurrentPage)
        {
            m_CurrentPage->ResetPage();
        }
        break;
    case IDC_RESET_ALL:
        for (SETTING_SECTIONS::const_iterator iter = m_Sections.begin(); iter != m_Sections.end(); iter++)
        {
            CConfigSettingSection * Section = *iter;

            for (size_t i = 0; i < Section->GetPageCount(); i++)
            {
                CSettingsPage * Page = Section->GetPage(i);
                if (Page->EnableReset())
                {
                    Page->ResetPage();
                }
            }
        }

        break;
    }
    return FALSE;
}

void CSettingConfig::ApplySettings(bool UpdateScreen)
{
    stdstr GameIni(g_Settings->LoadStringVal(Game_IniKey));

    if (!GameIni.empty())
    {
        stdstr GoodName;
        if (g_Settings->LoadStringVal(Rdb_GoodName, GoodName))
        {
            g_Settings->SaveString(Cfg_GoodName, GoodName);
        }
    }

    for (SETTING_SECTIONS::const_iterator iter = m_Sections.begin(); iter != m_Sections.end(); iter++)
    {
        CConfigSettingSection * Section = *iter;

        for (size_t i = 0; i < Section->GetPageCount(); i++)
        {
            CSettingsPage * Page = Section->GetPage(i);
            Page->ApplySettings(UpdateScreen);
        }
    }

    if (UpdateScreen)
    {
        ::EnableWindow(GetDlgItem(IDAPPLY), false);
        ::EnableWindow(GetDlgItem(IDC_RESET_PAGE), m_CurrentPage->EnableReset());
    }

    if (!g_Settings->LoadStringVal(Game_IniKey).empty())
    {
        stdstr GoodName = g_Settings->LoadStringVal(Rdb_GoodName);
        if (GoodName.length() > 0)
        {
            g_Settings->SaveString(Cfg_GoodName, GoodName);
        }
    }
    CSettingTypeApplication::Flush();
}

LRESULT CSettingConfig::OnPageListItemChanged(NMHDR* /*phdr*/)
{
    m_bTVNSelChangedSupported = true;

    HTREEITEM hItem = m_PagesTreeList.GetSelectedItem();
    CSettingsPage * Page = (CSettingsPage *)m_PagesTreeList.GetItemData(hItem);

    if (Page)
    {
        if (m_CurrentPage)
        {
            m_CurrentPage->HidePage();
        }
        m_CurrentPage = Page;
        m_CurrentPage->ShowPage();
        ::EnableWindow(GetDlgItem(IDC_RESET_PAGE), m_CurrentPage->EnableReset());
    }

    return 0;   // retval ignored
}

// fallback to using HitTest if TVN_SELCHANGED isn't working
LRESULT CSettingConfig::OnPageListClicked(NMHDR*)
{
	if (m_bTVNSelChangedSupported)
	{
		return 0;
	}

	DWORD dwClickPos = GetMessagePos();
	CPoint clickPt = CPoint(dwClickPos);
	ScreenToClient(&clickPt);

	CRect treeRect;
	m_PagesTreeList.GetWindowRect(treeRect);
	ScreenToClient(&treeRect);

	clickPt.x -= treeRect.left;
	clickPt.y -= treeRect.top;
	clickPt.y -= 2;

	UINT uFlags;
	HTREEITEM hItem = m_PagesTreeList.HitTest(clickPt, &uFlags);

	CSettingsPage * Page = (CSettingsPage *)m_PagesTreeList.GetItemData(hItem);

	if (Page)
	{
		if (m_CurrentPage)
		{
			m_CurrentPage->HidePage();
		}
		m_CurrentPage = Page;
		m_CurrentPage->ShowPage();
		::EnableWindow(GetDlgItem(IDC_RESET_PAGE), m_CurrentPage->EnableReset());
	}
	return 0;   // retval ignored
}

LRESULT	CSettingConfig::OnSettingPageChanged(UINT /*uMsg*/, WPARAM /*wPage*/, LPARAM /*lParam*/)
{
    ::EnableWindow(GetDlgItem(IDAPPLY), true);
    ::EnableWindow(GetDlgItem(IDC_RESET_PAGE), m_CurrentPage->EnableReset());
    BoldChangedPages(m_PagesTreeList.GetRootItem());
    return 0;
}

void CSettingConfig::BoldChangedPages(HTREEITEM hItem)
{
    if (hItem == m_PagesTreeList.GetRootItem())
    {
        ::EnableWindow(GetDlgItem(IDC_RESET_ALL), false);
    }
    bool bEnableResetAll = false;

    while (hItem)
    {
        CSettingsPage * Page = (CSettingsPage *)m_PagesTreeList.GetItemData(hItem);
        if (Page)
        {
            m_PagesTreeList.SetItemState(hItem, Page->EnableReset() ? TVIS_BOLD : 0, TVIS_BOLD);
            if (Page->EnableReset())
            {
                bEnableResetAll = true;
            }
        }

        BoldChangedPages(m_PagesTreeList.GetChildItem(hItem));
        hItem = m_PagesTreeList.GetNextSiblingItem(hItem);
    }

    if (bEnableResetAll)
    {
        ::EnableWindow(GetDlgItem(IDC_RESET_ALL), true);
    }
}
