#pragma once

class CConfigSettingSection;
class CSettingsPage;

class CSettingConfig :
	private CDialogImpl<CSettingConfig>
{

	BEGIN_MSG_MAP_EX(CSettingConfig)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_CODE_HANDLER(BN_CLICKED,OnClicked)
		NOTIFY_HANDLER_EX(IDC_PAGELIST, TVN_SELCHANGED, OnPageListItemChanged)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	enum { IDD = IDD_Settings_Config };

	LRESULT	OnInitDialog ( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT	OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);
	LRESULT OnPageListItemChanged(NMHDR* phdr);
	
public:
	 CSettingConfig ( bool bJustGameSetting = false );
	~CSettingConfig ( void );
	
	void Display ( void * ParentWindow );

private:
	typedef std::list<CConfigSettingSection *> SETTING_SECTIONS;

	CTreeViewCtrl    m_PagesTreeList;
	SETTING_SECTIONS m_Sections;
	CSettingsPage *  m_CurrentPage;
};