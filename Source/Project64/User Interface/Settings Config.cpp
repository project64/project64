#include "../User Interface.h"
#include "Settings Config.h"
#include "Settings/Settings Page.h"


CSettingConfig::CSettingConfig(bool bJustGameSetting /* = false */)	:
	m_CurrentPage(NULL),
	m_GameConfig(bJustGameSetting)
{
}

CSettingConfig::~CSettingConfig ()
{
	for (SETTING_SECTIONS::const_iterator iter = m_Sections.begin(); iter != m_Sections.end(); iter++)
	{
		CConfigSettingSection * Section = *iter;
		delete Section;
	}
}

void CSettingConfig::Display(void * ParentWindow)
{
	DoModal((HWND)ParentWindow);
}

LRESULT	CSettingConfig::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	stdstr ConfigRomTitle, GameIni(_Settings->LoadString(Game_IniKey));

	if (!GameIni.empty())
	{
		ConfigRomTitle.Format("Config: %s",_Settings->LoadString(Game_GoodName).c_str());
	}

	RECT rcSettingInfo;
	::GetWindowRect(GetDlgItem(IDC_SETTING_INFO),&rcSettingInfo);
	::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rcSettingInfo,2);

	CConfigSettingSection * SettingsSection;

	if (m_GameConfig)
	{
		SetWindowText(ConfigRomTitle.c_str());
	} else {
		SetWindowText(GS(OPTIONS_TITLE));

		if (_Settings->LoadBool(Setting_PluginPageFirst))
		{
			SettingsSection = new CConfigSettingSection(GS(TAB_PLUGIN));
			SettingsSection->AddPage(new COptionPluginPage(this->m_hWnd,rcSettingInfo ));
			m_Sections.push_back(SettingsSection);
		}

		SettingsSection = new CConfigSettingSection(GS(TAB_OPTIONS));
		SettingsSection->AddPage(new CGeneralOptionsPage(this->m_hWnd,rcSettingInfo ));
		SettingsSection->AddPage(new CAdvancedOptionsPage(this->m_hWnd,rcSettingInfo ));
		SettingsSection->AddPage(new COptionsDirectoriesPage(this->m_hWnd,rcSettingInfo ));
		m_Sections.push_back(SettingsSection);

		SettingsSection = new CConfigSettingSection(GS(TAB_ROMSELECTION));
		SettingsSection->AddPage(new COptionsGameBrowserPage(this->m_hWnd,rcSettingInfo ));
		m_Sections.push_back(SettingsSection);

		SettingsSection = new CConfigSettingSection(GS(TAB_SHORTCUTS));
		SettingsSection->AddPage(new COptionsShortCutsPage(this->m_hWnd,rcSettingInfo ));
		m_Sections.push_back(SettingsSection);

		if (!_Settings->LoadBool(Setting_PluginPageFirst))
		{
			SettingsSection = new CConfigSettingSection(GS(TAB_PLUGIN));
			SettingsSection->AddPage(new COptionPluginPage(this->m_hWnd,rcSettingInfo ));
			m_Sections.push_back(SettingsSection);
		}
	}

	//Game Settings
	if (!GameIni.empty())
	{
		CConfigSettingSection * GameSettings = new CConfigSettingSection(ConfigRomTitle.c_str());
		GameSettings->AddPage(new CGameGeneralPage(this->m_hWnd,rcSettingInfo ));
		GameSettings->AddPage(new CGameRecompilePage(this->m_hWnd,rcSettingInfo ));
		GameSettings->AddPage(new CGamePluginPage(this->m_hWnd,rcSettingInfo ));
		if (_Settings->LoadBool(Setting_RdbEditor))
		{
			GameSettings->AddPage(new CGameStatusPage(this->m_hWnd,rcSettingInfo ));
		}
		m_Sections.push_back(GameSettings);
	}

	
	m_PagesTreeList.Attach(GetDlgItem(IDC_PAGELIST));

	bool bFirstItem = true;
	for (SETTING_SECTIONS::const_iterator iter = m_Sections.begin(); iter != m_Sections.end(); iter++)
	{
		CConfigSettingSection * Section = *iter;
		
		HTREEITEM hSectionItem = NULL;	
		for (int i = 0; i < Section->GetPageCount(); i++ )
		{
			CSettingsPage * Page = Section->GetPage(i);
			if (i == 0)
			{
				hSectionItem = m_PagesTreeList.InsertItem(TVIF_TEXT | TVIF_PARAM,Section->GetPageTitle(),0,0,0,0,(ULONG)Page,TVI_ROOT,TVI_LAST);
				continue;
			}
			if (hSectionItem == NULL)
			{
				continue;
			}
			m_PagesTreeList.InsertItem(TVIF_TEXT | TVIF_PARAM,GS(Page->PageTitle()),0,0,0,0,(ULONG)Page,hSectionItem,TVI_LAST);
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

LRESULT CSettingConfig::OnClicked (WORD wNotifyCode, WORD wID, HWND , BOOL& bHandled)
{
	switch(wID)
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
	case IDC_RESET:
		if (m_CurrentPage)
		{
			m_CurrentPage->ResetPage();
		}
		break;
	case IDC_RESET_ALL:
		for (SETTING_SECTIONS::const_iterator iter = m_Sections.begin(); iter != m_Sections.end(); iter++)
		{
			CConfigSettingSection * Section = *iter;
			
			for (int i = 0; i < Section->GetPageCount(); i++ )
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

void CSettingConfig::ApplySettings( bool UpdateScreen )
{
	for (SETTING_SECTIONS::const_iterator iter = m_Sections.begin(); iter != m_Sections.end(); iter++)
	{
		CConfigSettingSection * Section = *iter;
		
		for (int i = 0; i < Section->GetPageCount(); i++ )
		{
			CSettingsPage * Page = Section->GetPage(i);
			Page->ApplySettings(UpdateScreen);
		}
	}

	if (UpdateScreen)
	{
		::EnableWindow(GetDlgItem(IDAPPLY),false);
		::EnableWindow(GetDlgItem(IDC_RESET), m_CurrentPage->EnableReset());
	}
}

LRESULT CSettingConfig::OnPageListItemChanged(NMHDR* phdr)
{
	NMLISTVIEW* pnmlv = (NMLISTVIEW*) phdr;
	HTREEITEM hItem = m_PagesTreeList.GetSelectedItem();
	CSettingsPage * Page = (CSettingsPage * )m_PagesTreeList.GetItemData(hItem);
	
	if (Page) 
	{
		if (m_CurrentPage)
		{
			m_CurrentPage->HidePage();
		}
		m_CurrentPage = Page;
		m_CurrentPage->ShowPage();
		::EnableWindow(GetDlgItem(IDC_RESET), m_CurrentPage->EnableReset());
	}
	return 0;   // retval ignored
}

LRESULT	CSettingConfig::OnSettingPageChanged ( UINT /*uMsg*/, WPARAM wPage, LPARAM /*lParam*/)
{
	::EnableWindow(GetDlgItem(IDAPPLY),true);
	::EnableWindow(GetDlgItem(IDC_RESET), m_CurrentPage->EnableReset());
	BoldChangedPages(m_PagesTreeList.GetRootItem());
	return 0;
}

void CSettingConfig::BoldChangedPages ( HTREEITEM hItem )
{
	if (hItem == m_PagesTreeList.GetRootItem())
	{
		::EnableWindow(GetDlgItem(IDC_RESET_ALL), false);
	}
	bool bEnableResetAll = false;

	while (hItem)
	{
		CSettingsPage * Page = (CSettingsPage * )m_PagesTreeList.GetItemData(hItem);
		if (Page)
		{
			m_PagesTreeList.SetItemState(hItem,Page->EnableReset() ? TVIS_BOLD : 0,TVIS_BOLD);
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

