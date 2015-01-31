#pragma once

class CConfigSettingSection;
class CSettingsPage;

class CSettingConfig :
	public CDialogImpl<CSettingConfig>
{
public:
	BEGIN_MSG_MAP_EX(CSettingConfig)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_CODE_HANDLER(BN_CLICKED,OnClicked)		
		NOTIFY_HANDLER_EX(IDC_PAGELIST, TVN_SELCHANGED, OnPageListItemChanged)
		MESSAGE_HANDLER_EX(PSM_CHANGED,OnSettingPageChanged)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	enum { IDD = IDD_Settings_Config };

	LRESULT	OnInitDialog ( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT	OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);
	LRESULT OnPageListItemChanged(NMHDR* phdr);
	LRESULT	OnSettingPageChanged ( UINT /*uMsg*/, WPARAM wPage, LPARAM /*lParam*/);

public:
	CSettingConfig ( bool bJustGameSetting = false );
	~CSettingConfig ( void );

	void Display ( void * ParentWindow );
	void UpdateAdvanced ( bool AdvancedMode );

private:
	bool UpdateAdvanced   ( bool AdvancedMode, HTREEITEM hItem );
	void ApplySettings    ( bool UpdateScreen );
	void BoldChangedPages ( HTREEITEM hItem );

	typedef std::list<CConfigSettingSection *> SETTING_SECTIONS;

	CTreeViewCtrl    m_PagesTreeList;
	SETTING_SECTIONS m_Sections;
	CSettingsPage *  m_CurrentPage, * m_GeneralOptionsPage, * m_AdvancedPage;
	bool             m_GameConfig;
};
