#include "../User Interface.h"
#include "Settings Config.h"
#include "Settings/Settings Page.h"


CSettingConfig::CSettingConfig(bool bJustGameSetting /* = false */)	:
	m_CurrentPage(NULL)
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
	stdstr_f ConfigRomTitle("Config: %s",_Settings->LoadString(ROM_GoodName).c_str());
	
	RECT rcSettingInfo;
	::GetWindowRect(GetDlgItem(IDC_SETTING_INFO),&rcSettingInfo);
	::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rcSettingInfo,2);

	
	//Game Settings
	CConfigSettingSection * GameSettings = new CConfigSettingSection(ConfigRomTitle.c_str());
	m_Sections.push_back(GameSettings);
	GameSettings->AddPage(new CGameGeneralPage(this->m_hWnd,rcSettingInfo ));
	GameSettings->AddPage(new CGameRecompilePage(this->m_hWnd,rcSettingInfo ));
	GameSettings->AddPage(new CGamePluginPage(this->m_hWnd,rcSettingInfo ));
	GameSettings->AddPage(new CGameStatusPage(this->m_hWnd,rcSettingInfo ));

	
	m_PagesTreeList.Attach(GetDlgItem(IDC_PAGELIST));

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
			}
			if (hSectionItem == NULL)
			{
				continue;
			}
			m_PagesTreeList.InsertItem(TVIF_TEXT | TVIF_PARAM,GS(Page->PageTitle()),0,0,0,0,(ULONG)Page,hSectionItem,TVI_LAST);
		}
	}
	return TRUE;
}

LRESULT CSettingConfig::OnClicked (WORD wNotifyCode, WORD wID, HWND , BOOL& bHandled)
{
	switch(wID)
	{
	case IDCANCEL:
		EndDialog(0);
		break;
	}
	return FALSE;
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
	}
	//hItem = TreeView_GetSelection(hCheatTree);
	//if (TreeView_GetChild(hCheatTree,hItem) == NULL) { 

	//int nSelItem = m_wndList.GetSelectedIndex();
	//CString sMsg;

	// If no item is selected, show "none". Otherwise, show its index.

	//if ( -1 == nSelItem )
	//	sMsg = _T("(none)");
	//else
	//	sMsg.Format ( _T("%d"), nSelItem );

	//SetDlgItemText ( IDC_SEL_ITEM, sMsg );
	return 0;   // retval ignored
}
