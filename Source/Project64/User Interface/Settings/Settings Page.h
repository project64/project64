#pragma once

class CSettingsPage 
{
public:
	virtual ~CSettingsPage ( void ) = 0 {};
	
	virtual LanguageStringID PageTitle     ( void ) = 0;
	virtual void             HidePage      ( void ) = 0;
	virtual void             ShowPage      ( void ) = 0;
	virtual void             ApplySettings ( bool UpdateScreen ) = 0;
	virtual bool             EnableReset   ( void ) = 0;
	virtual void             ResetPage     ( void ) = 0;
};

template <class T>
class CSettingsPageImpl :
	public CDialogImpl<T>
{
protected:
	typedef std::map<SettingID,CModifiedButton *>   ButtonList;
	typedef std::map<SettingID,CModifiedComboBox *> ComboBoxList;

	virtual ~CSettingsPageImpl()
	{
		for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
		{
			delete iter->second;
		}
		for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
		{
			delete cb_iter->second;
		}
		
	}
	bool Create(HWND hParent, const RECT & rcDispay)
	{
		CDialogImpl<T>::Create(hParent);
		if (m_hWnd == NULL)
		{
			return false;
		}
		SetWindowPos(HWND_TOP,&rcDispay,SWP_HIDEWINDOW);
		return true;
	}

	void CheckBoxChanged ( UINT Code, int id, HWND ctl )
	{
		for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
		{
			CModifiedButton * Button = iter->second;
			if ((int)Button->GetMenu() != id)
			{
				continue;
			}			
			Button->SetChanged(true);
			SendMessage(GetParent(),PSM_CHANGED,(WPARAM)m_hWnd,0);
			break;
		}
	}
	
	void UpdateModCheckBox ( CModifiedButton & CheckBox, SettingID Type )
	{
		if (CheckBox.IsChanged())
		{
			bool bValue = CheckBox.GetCheck() == BST_CHECKED;
			if (bValue != _Settings->LoadBool(Type))
			{
				_Settings->SaveBool(Type,bValue);
			}
		}
		if (CheckBox.IsReset())
		{
			_Settings->DeleteSetting(Type);
			CheckBox.SetReset(false);
		}
	}

	bool ResetCheckBox ( CModifiedButton & CheckBox, SettingID Type )
	{
		if (!CheckBox.IsChanged())
		{
			return false;
		}

		bool Value = _Settings->LoadDefaultBool(Type);
		CheckBox.SetReset(true);
		CheckBox.SetCheck(Value ? BST_CHECKED : BST_UNCHECKED);
		return true;
	}

	void AddModCheckBox ( HWND hWnd, SettingID Type )
	{
		ButtonList::iterator item = m_ButtonList.find(Type);
		if (item == m_ButtonList.end())
		{
			CModifiedButton * Button = new CModifiedButton;
			if (Button == NULL)
			{
				return;
			}
			Button->Attach(hWnd);

			m_ButtonList.insert(ButtonList::value_type(Type,Button));
		} 
	}

	CModifiedComboBox * AddModComboBox ( HWND hWnd, SettingID Type )
	{
		ComboBoxList::iterator item = m_ComboBoxList.find(Type);
		if (item != m_ComboBoxList.end())
		{
			return item->second;
		}

		CModifiedComboBox * ComboBox = new CModifiedComboBox;
		if (ComboBox == NULL)
		{
			return NULL;
		}
		ComboBox->Attach(hWnd);
		m_ComboBoxList.insert(ComboBoxList::value_type(Type,ComboBox));
		return ComboBox;
	}

	void UpdateCheckBoxes ( void )
	{
		for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
		{
			CModifiedButton * Button = iter->second;
			bool SettingSelected;
			
			Button->SetChanged(_Settings->LoadBool(iter->first,SettingSelected));
			Button->SetCheck(SettingSelected ? BST_CHECKED : BST_UNCHECKED);
		}

	}

	void UpdateComboBoxes ( void)
	{
		for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
		{
			CModifiedComboBox * ComboBox = cb_iter->second;
			DWORD SelectedValue;
			
			ComboBox->SetChanged(_Settings->LoadDword(cb_iter->first,SelectedValue));
			ComboBox->SetDefault(SelectedValue);
		}
	}

	virtual void UpdatePageSettings ( void )
	{
		UpdateCheckBoxes();
		UpdateComboBoxes();
	}

	void ComboBoxChanged ( UINT Code, int id, HWND ctl )
	{
		for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
		{
			CModifiedComboBox * ComboBox = cb_iter->second;
			if ((int)ComboBox->GetMenu() != id)
			{
				continue;
			}			
			ComboBox->SetChanged(true);
			SendMessage(GetParent(),PSM_CHANGED,(WPARAM)m_hWnd,0);
			break;
		}
	}

	virtual bool ResetComboBox ( CModifiedComboBox & ComboBox, SettingID Type )
	{
		if (!ComboBox.IsChanged())
		{
			return false;
		}

		ComboBox.SetReset(true);
		DWORD Value = _Settings->LoadDefaultDword(Type);
		for (int i = 0, n = ComboBox.GetCount(); i < n; i++)
		{
			if (ComboBox.GetItemData(i) != Value)
			{
				continue;
			}
			ComboBox.SetCurSel(i);
			return true;
		}
		return false;
	}

	virtual void UpdateModComboBox ( CModifiedComboBox & ComboBox, SettingID Type )
	{
		if (ComboBox.IsChanged())
		{
			int index = ComboBox.GetCurSel();
			if (index == CB_ERR) 
			{
				return; 
			}
			_Settings->SaveDword(Type,(DWORD)ComboBox.GetItemData(index));
		}
		if (ComboBox.IsReset())
		{
			_Settings->DeleteSetting(Type);
			ComboBox.SetReset(false);
		}
	}

public:
	virtual void ApplyCheckBoxes ( void )
	{
		for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
		{
			CModifiedButton * Button = iter->second;
			UpdateModCheckBox(*Button,iter->first);
		}
	}
	virtual void ApplyComboBoxes ( void )
	{
		for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
		{
			CModifiedComboBox * ComboBox = cb_iter->second;
			UpdateModComboBox(*ComboBox,cb_iter->first);
		}
	}
	void ApplySettings( bool UpdateScreen )
	{
		ApplyCheckBoxes();
		ApplyComboBoxes();

		if (UpdateScreen)
		{
			UpdatePageSettings();
		}
	}

	bool EnableReset ( void )
	{
		for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
		{
			CModifiedButton * Button = iter->second;
			if (Button->IsChanged())
			{
				return true;
			}
		}
		for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
		{
			CModifiedComboBox * ComboBox = cb_iter->second;
			if (ComboBox->IsChanged())
			{
				return true;
			}
		}
		return false;
	}
	
	void ResetPage ( void )
	{
		bool Changed = false;
		for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
		{
			CModifiedButton * Button = iter->second;
			if (ResetCheckBox(*Button,iter->first))
			{
				Changed = true;
			}
		}
		for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
		{
			CModifiedComboBox * ComboBox = cb_iter->second;
			if (ResetComboBox(*ComboBox,cb_iter->first))
			{
				Changed = true;
			}
		}
		if (Changed)
		{
			SendMessage(GetParent(),PSM_CHANGED,(WPARAM)m_hWnd,0);
		}
	}
protected:
	ButtonList   m_ButtonList;
	ComboBoxList m_ComboBoxList;
};


typedef std::vector<CSettingsPage *> SETTING_PAGES;

class CConfigSettingSection
{
	SETTING_PAGES m_Pages;
	stdstr        m_PageTitle;

public:
	CConfigSettingSection ( LPCSTR PageTitle );
	~CConfigSettingSection();

	LPCTSTR GetPageTitle    ( void ) const { return m_PageTitle.c_str(); }
	void    AddPage         ( CSettingsPage * Page );
	int     GetPageCount    ( void ) const { return m_Pages.size(); }
	CSettingsPage * GetPage ( int PageNo );
};

#include "Settings Page - Advanced Options.h"
#include "Settings Page - Directories.h"
#include "Settings Page - Game - General.h"
#include "Settings Page - Game - Plugin.h"
#include "Settings Page - Game - Recompiler.h"
#include "Settings Page - Game - Status.h"
#include "Settings Page - Game Browser.h"
#include "Settings Page - Keyboard Shortcuts.h"
#include "Settings Page - Options.h"
#include "Settings Page - Plugin.h"
