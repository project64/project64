#include "stdafx.h"

#ifdef WINDOWS_UI
#include "SettingsConfig.h"
#include "Settings/SettingsPage.h"
#include "Settings/SettingType/SettingsType-Application.h"

CSettingConfig::CSettingConfig(bool bJustGameSetting /* = false */)	:
	m_CurrentPage(NULL),
	m_GeneralOptionsPage(NULL),
	m_AdvancedPage(NULL),
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

void CSettingConfig::UpdateAdvanced ( bool AdvancedMode )
{
	UpdateAdvanced(AdvancedMode,m_PagesTreeList.GetRootItem());
	BoldChangedPages(m_PagesTreeList.GetRootItem());
}

bool CSettingConfig::UpdateAdvanced ( bool AdvancedMode, HTREEITEM hItem )
{
	while (hItem)
	{
		CSettingsPage * Page = (CSettingsPage * )m_PagesTreeList.GetItemData(hItem);
		if (!AdvancedMode && Page == m_AdvancedPage)
		{
			m_PagesTreeList.DeleteItem(hItem);
			return true;
		}
		if (AdvancedMode && Page == m_GeneralOptionsPage)
		{
			m_PagesTreeList.InsertItemW(TVIF_TEXT | TVIF_PARAM,GS(m_AdvancedPage->PageTitle()),0,0,0,0,(ULONG)m_AdvancedPage,hItem,TVI_FIRST);
			return true;
		}
		if (UpdateAdvanced(AdvancedMode,m_PagesTreeList.GetChildItem(hItem)))
		{
			return true;
		}
		hItem = m_PagesTreeList.GetNextSiblingItem(hItem);
	}
	return false;
}

LRESULT	CSettingConfig::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	stdstr ConfigRomTitle, GameIni(g_Settings->LoadStringVal(Game_IniKey));

	if (!GameIni.empty())
	{
		ConfigRomTitle.Format("Config: %s",g_Settings->LoadStringVal(Game_GoodName).c_str());
	}

	RECT rcSettingInfo;
	::GetWindowRect(GetDlgItem(IDC_SETTING_INFO),&rcSettingInfo);
	::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rcSettingInfo,2);

	CConfigSettingSection * SettingsSection;

	//Set the text for all gui Items
	SetDlgItemTextW(m_hWnd, IDC_RESET_PAGE, GS(BOTTOM_RESET_PAGE));
	SetDlgItemTextW(m_hWnd, IDC_RESET_ALL, GS(BOTTOM_RESET_ALL));
	SetDlgItemTextW(m_hWnd, IDOK, GS(CHEAT_OK));
	SetDlgItemTextW(m_hWnd, IDCANCEL, GS(CHEAT_CANCEL));
	SetDlgItemTextW(m_hWnd, IDAPPLY, GS(BOTTOM_APPLY));

	if (m_GameConfig)
	{
		if (g_Settings->LoadBool(Setting_RdbEditor))
		{
			SetWindowText(stdstr_f("%s ** RDB Edit Mode **",ConfigRomTitle.c_str()).c_str());
		} 
		else 
		{
			SetWindowText(ConfigRomTitle.c_str());
		}		
	}
	else 
	{
		if (g_Settings->LoadBool(Setting_RdbEditor))
		{
			SetWindowText(stdstr_f("%ws ** RDB Edit Mode **",GS(OPTIONS_TITLE)).c_str());
		} else {
			::SetWindowTextW(m_hWnd, GS(OPTIONS_TITLE));
		}

		if (g_Settings->LoadBool(Setting_PluginPageFirst))
		{
			SettingsSection = new CConfigSettingSection(GS(TAB_PLUGIN));
			SettingsSection->AddPage(new COptionPluginPage(this->m_hWnd,rcSettingInfo ));
			m_Sections.push_back(SettingsSection);
		}

		m_GeneralOptionsPage = new CGeneralOptionsPage(this,this->m_hWnd,rcSettingInfo );
		m_AdvancedPage = new CAdvancedOptionsPage(this->m_hWnd,rcSettingInfo );

		SettingsSection = new CConfigSettingSection(GS(TAB_OPTIONS));
		SettingsSection->AddPage(m_GeneralOptionsPage);
		SettingsSection->AddPage(m_AdvancedPage);
		SettingsSection->AddPage(new COptionsDirectoriesPage(this->m_hWnd,rcSettingInfo ));
		m_Sections.push_back(SettingsSection);

		SettingsSection = new CConfigSettingSection(GS(TAB_ROMSELECTION));
		SettingsSection->AddPage(new COptionsGameBrowserPage(this->m_hWnd,rcSettingInfo ));
		m_Sections.push_back(SettingsSection);

		SettingsSection = new CConfigSettingSection(GS(TAB_SHORTCUTS));
		SettingsSection->AddPage(new COptionsShortCutsPage(this->m_hWnd,rcSettingInfo ));
		m_Sections.push_back(SettingsSection);

		if (!g_Settings->LoadBool(Setting_PluginPageFirst))
		{
			SettingsSection = new CConfigSettingSection(GS(TAB_PLUGIN));
			SettingsSection->AddPage(new COptionPluginPage(this->m_hWnd,rcSettingInfo ));
			m_Sections.push_back(SettingsSection);
		}
	}

	//Game Settings
	if (!GameIni.empty())
	{
        CConfigSettingSection * GameSettings = new CConfigSettingSection(ConfigRomTitle.ToUTF16().c_str());
		GameSettings->AddPage(new CGameGeneralPage(this->m_hWnd,rcSettingInfo ));
		GameSettings->AddPage(new CGameRecompilePage(this->m_hWnd,rcSettingInfo ));
		GameSettings->AddPage(new CGamePluginPage(this->m_hWnd,rcSettingInfo ));
		if (g_Settings->LoadBool(Setting_RdbEditor))
		{
			GameSettings->AddPage(new CGameStatusPage(this->m_hWnd,rcSettingInfo ));
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
			if (HideAdvanced && Page == m_AdvancedPage)
			{
				continue;
			}
			if (i == 0)
			{
				hSectionItem = m_PagesTreeList.InsertItemW(TVIF_TEXT | TVIF_PARAM,Section->GetPageTitle(),0,0,0,0,(ULONG)Page,TVI_ROOT,TVI_LAST);
				continue;
			}
			if (hSectionItem == NULL)
			{
				continue;
			}
			m_PagesTreeList.InsertItemW(TVIF_TEXT | TVIF_PARAM,GS(Page->PageTitle()),0,0,0,0,(ULONG)Page,hSectionItem,TVI_LAST);
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

LRESULT CSettingConfig::OnClicked (WORD /*wNotifyCode*/, WORD wID, HWND , BOOL& /*bHandled*/)
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
			
			for (size_t i = 0; i < Section->GetPageCount(); i++ )
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
	stdstr GameIni(g_Settings->LoadStringVal(Game_IniKey));

	if (!GameIni.empty())
	{
		stdstr GoodName;
		if (!g_Settings->LoadStringVal(Game_GoodName,GoodName))
		{
			g_Settings->SaveString(Game_GoodName,GoodName);
		}
	}

	for (SETTING_SECTIONS::const_iterator iter = m_Sections.begin(); iter != m_Sections.end(); iter++)
	{
		CConfigSettingSection * Section = *iter;
		
		for (size_t i = 0; i < Section->GetPageCount(); i++ )
		{
			CSettingsPage * Page = Section->GetPage(i);
			Page->ApplySettings(UpdateScreen);
		}
	}

	if (UpdateScreen)
	{
		::EnableWindow(GetDlgItem(IDAPPLY),false);
		::EnableWindow(GetDlgItem(IDC_RESET_PAGE), m_CurrentPage->EnableReset());
	}
	

	if (!g_Settings->LoadStringVal(Game_IniKey).empty())
	{
		stdstr GoodName = g_Settings->LoadStringVal(Rdb_GoodName);
		if (GoodName.length() > 0)
		{
			g_Settings->SaveString(Game_GoodName,GoodName);
		}
	}
	CSettingTypeApplication::Flush();
}

LRESULT CSettingConfig::OnPageListItemChanged(NMHDR* /*phdr*/)
{
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
		::EnableWindow(GetDlgItem(IDC_RESET_PAGE), m_CurrentPage->EnableReset());
	}
	return 0;   // retval ignored
}

LRESULT	CSettingConfig::OnSettingPageChanged ( UINT /*uMsg*/, WPARAM /*wPage*/, LPARAM /*lParam*/)
{
	::EnableWindow(GetDlgItem(IDAPPLY),true);
	::EnableWindow(GetDlgItem(IDC_RESET_PAGE), m_CurrentPage->EnableReset());
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
#endif
