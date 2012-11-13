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
	typedef std::map<SettingID,CModifiedEditBox *>     TextBoxList;
	typedef std::map<SettingID,CModifiedButton *>      ButtonList;
	typedef std::map<SettingID,CModifiedComboBox *>    ComboBoxList;
	typedef std::map<SettingID,CModifiedComboBoxTxt *> ComboBoxTxtList;

	virtual ~CSettingsPageImpl()
	{
		for (TextBoxList::iterator eb_iter = m_TxtBoxList.begin(); eb_iter != m_TxtBoxList.end(); eb_iter ++)
		{
			delete eb_iter->second;
		}
		for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
		{
			delete iter->second;
		}
		for (ComboBoxTxtList::iterator cbtxt_iter = m_ComboBoxTxtList.begin(); cbtxt_iter != m_ComboBoxTxtList.end(); cbtxt_iter ++)
		{
			delete cbtxt_iter->second;
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
		m_UpdatingTxt = false;
		return true;
	}

	void CheckBoxChanged ( UINT /*Code*/, int id, HWND /*ctl*/ )
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
	
	void UpdateModEditBox ( CModifiedEditBox & EditBox, SettingID Type )
	{
		if (EditBox.IsChanged())
		{
			stdstr Value = EditBox.GetWindowText();
			if (EditBox.IsbString())
			{
				_Settings->SaveString(Type,Value);
			} else {
				DWORD dwValue = atoi(Value.c_str());
				_Settings->SaveDword(Type,dwValue);
			}
		}
		if (EditBox.IsReset())
		{
			_Settings->DeleteSetting(Type);
			EditBox.SetReset(false);
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

	bool ResetEditBox ( CModifiedEditBox & EditBox, SettingID Type )
	{
		if (!EditBox.IsChanged())
		{
			return false;
		}

		if (EditBox.IsbString())
		{
			stdstr Value = _Settings->LoadDefaultString(Type);
			EditBox.SetWindowText(Value.c_str());
			EditBox.SetReset(true);
		} else {
			DWORD Value = _Settings->LoadDefaultDword(Type);
			EditBox.SetWindowText(stdstr_f("%d",Value).c_str());
			EditBox.SetReset(true);
		}
		return true;
	}

	CModifiedEditBox * AddModTextBox ( HWND hWnd, SettingID Type, bool bString )
	{
		TextBoxList::iterator item = m_TxtBoxList.find(Type);
		if (item == m_TxtBoxList.end())
		{
			CModifiedEditBox * EditBox = new CModifiedEditBox(bString);
			if (EditBox == NULL)
			{
				return NULL;
			}
			EditBox->Attach(hWnd);

			m_TxtBoxList.insert(TextBoxList::value_type(Type,EditBox));
			return EditBox;
		}
		return NULL;
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

		CModifiedComboBox * ComboBox = new CModifiedComboBox(_Settings->LoadDefaultDword(Type),NULL,false);
		if (ComboBox == NULL)
		{
			return NULL;
		}
		ComboBox->Attach(hWnd);
		m_ComboBoxList.insert(ComboBoxList::value_type(Type,ComboBox));
		return ComboBox;
	}

	CModifiedComboBoxTxt * AddModComboBoxTxt ( HWND hWnd, SettingID Type )
	{
		ComboBoxTxtList::iterator item = m_ComboBoxTxtList.find(Type);
		if (item != m_ComboBoxTxtList.end())
		{
			return item->second;
		}

		CModifiedComboBoxTxt * ComboBox = new CModifiedComboBoxTxt(_Settings->LoadDefaultString(Type));
		if (ComboBox == NULL)
		{
			return NULL;
		}
		ComboBox->Attach(hWnd);
		m_ComboBoxTxtList.insert(ComboBoxTxtList::value_type(Type,ComboBox));
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
		for (ComboBoxTxtList::iterator cbtxt_iter = m_ComboBoxTxtList.begin(); cbtxt_iter != m_ComboBoxTxtList.end(); cbtxt_iter ++)
		{
			CModifiedComboBoxTxt * ComboBox = cbtxt_iter->second;
			stdstr SelectedValue;
			
			ComboBox->SetChanged(_Settings->LoadString(cbtxt_iter->first,SelectedValue));
			ComboBox->SetDefault(SelectedValue);
		}

		for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
		{
			CModifiedComboBox * ComboBox = cb_iter->second;
			DWORD SelectedValue;
			
			ComboBox->SetChanged(_Settings->LoadDword(cb_iter->first,SelectedValue));
			ComboBox->SetDefault(SelectedValue);
		}
	}

	void UpdateTextBoxes ( void)
	{
		for (TextBoxList::iterator iter = m_TxtBoxList.begin(); iter != m_TxtBoxList.end(); iter ++)
		{
			CModifiedEditBox * TextBox = iter->second;
			
			m_UpdatingTxt = true;
			if (TextBox->IsbString())
			{
				stdstr SelectedValue;
				TextBox->SetChanged(_Settings->LoadString(iter->first,SelectedValue));
				TextBox->SetWindowText(SelectedValue.c_str());
			} else {
				DWORD SelectedValue;
				TextBox->SetChanged(_Settings->LoadDword(iter->first,SelectedValue));
				TextBox->SetWindowText(stdstr_f("%d",SelectedValue).c_str());
			}
			m_UpdatingTxt = false;
		}
	}

	virtual void UpdatePageSettings ( void )
	{
		UpdateCheckBoxes();
		UpdateComboBoxes();
		UpdateTextBoxes();
	}

	void ComboBoxChanged ( UINT /*Code*/, int id, HWND /*ctl*/ )
	{
		for (ComboBoxTxtList::iterator cbtxt_iter = m_ComboBoxTxtList.begin(); cbtxt_iter != m_ComboBoxTxtList.end(); cbtxt_iter ++)
		{
			CModifiedComboBoxTxt * ComboBox = cbtxt_iter->second;
			if ((int)ComboBox->GetMenu() != id)
			{
				continue;
			}
			ComboBox->SetChanged(true);
			SendMessage(GetParent(),PSM_CHANGED,(WPARAM)m_hWnd,0);
			break;
		}
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

	void EditBoxChanged ( UINT /*Code*/, int id, HWND /*ctl*/ )
	{
		if (m_UpdatingTxt)
		{
			return;
		}

		for (TextBoxList::iterator eb_iter = m_TxtBoxList.begin(); eb_iter != m_TxtBoxList.end(); eb_iter ++)
		{
			CModifiedEditBox * EditBox = eb_iter->second;
			if ((int)EditBox->GetMenu() != id)
			{
				continue;
			}
			EditBox->SetChanged(true);
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
			if (*((WPARAM *)ComboBox.GetItemData(i)) != Value)
			{
				continue;
			}
			ComboBox.SetCurSel(i);
			return true;
		}
		return false;
	}

	virtual bool ResetComboBoxTxt ( CModifiedComboBoxTxt & ComboBox, SettingID Type )
	{
		if (!ComboBox.IsChanged())
		{
			return false;
		}

		ComboBox.SetReset(true);
		stdstr Value = _Settings->LoadDefaultString(Type);
		for (int i = 0, n = ComboBox.GetCount(); i < n; i++)
		{
			if (*((stdstr *)ComboBox.GetItemData(i)) != Value)
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
			_Settings->SaveDword(Type,*(DWORD *)ComboBox.GetItemData(index));
		}
		if (ComboBox.IsReset())
		{
			_Settings->DeleteSetting(Type);
			ComboBox.SetReset(false);
		}
	}

	virtual void UpdateModComboBoxTxt ( CModifiedComboBoxTxt & ComboBox, SettingID Type )
	{
		if (ComboBox.IsChanged())
		{
			int index = ComboBox.GetCurSel();
			if (index == CB_ERR) 
			{
				return; 
			}
			_Settings->SaveString(Type,((stdstr *)ComboBox.GetItemData(index))->c_str());
		}
		if (ComboBox.IsReset())
		{
			_Settings->DeleteSetting(Type);
			ComboBox.SetReset(false);
		}
	}
public:
	virtual void ApplyEditBoxes ( void )
	{
		for (TextBoxList::iterator eb_iter = m_TxtBoxList.begin(); eb_iter != m_TxtBoxList.end(); eb_iter ++)
		{
			CModifiedEditBox * EditBox = eb_iter->second;
			UpdateModEditBox(*EditBox,eb_iter->first);
		}
	}
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
		for (ComboBoxTxtList::iterator cbtxt_iter = m_ComboBoxTxtList.begin(); cbtxt_iter != m_ComboBoxTxtList.end(); cbtxt_iter ++)
		{
			CModifiedComboBoxTxt * ComboBox = cbtxt_iter->second;
			UpdateModComboBoxTxt(*ComboBox,cbtxt_iter->first);
		}
		for (ComboBoxList::iterator cb_iter = m_ComboBoxList.begin(); cb_iter != m_ComboBoxList.end(); cb_iter ++)
		{
			CModifiedComboBox * ComboBox = cb_iter->second;
			UpdateModComboBox(*ComboBox,cb_iter->first);
		}
	}
	void ApplySettings( bool UpdateScreen )
	{
		ApplyEditBoxes();
		ApplyCheckBoxes();
		ApplyComboBoxes();

		if (UpdateScreen)
		{
			UpdatePageSettings();
		}
	}

	bool EnableReset ( void )
	{
		for (TextBoxList::iterator eb_iter = m_TxtBoxList.begin(); eb_iter != m_TxtBoxList.end(); eb_iter ++)
		{
			CModifiedEditBox * EditBox = eb_iter->second;
			if (EditBox->IsChanged())
			{
				return true;
			}
		}
		for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
		{
			CModifiedButton * Button = iter->second;
			if (Button->IsChanged())
			{
				return true;
			}
		}
		for (ComboBoxTxtList::iterator cbtxt_iter = m_ComboBoxTxtList.begin(); cbtxt_iter != m_ComboBoxTxtList.end(); cbtxt_iter ++)
		{
			CModifiedComboBoxTxt * ComboBox = cbtxt_iter->second;
			if (ComboBox->IsChanged())
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
		for (TextBoxList::iterator eb_iter = m_TxtBoxList.begin(); eb_iter != m_TxtBoxList.end(); eb_iter ++)
		{
			CModifiedEditBox * EditBox = eb_iter->second;
			if (ResetEditBox(*EditBox,eb_iter->first))
			{
				Changed = true;
			}
		}
		for (ButtonList::iterator iter = m_ButtonList.begin(); iter != m_ButtonList.end(); iter ++)
		{
			CModifiedButton * Button = iter->second;
			if (ResetCheckBox(*Button,iter->first))
			{
				Changed = true;
			}
		}
		for (ComboBoxTxtList::iterator cbtxt_iter = m_ComboBoxTxtList.begin(); cbtxt_iter != m_ComboBoxTxtList.end(); cbtxt_iter ++)
		{
			CModifiedComboBoxTxt * ComboBox = cbtxt_iter->second;
			if (ResetComboBoxTxt(*ComboBox,cbtxt_iter->first))
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
	TextBoxList     m_TxtBoxList;
	ButtonList      m_ButtonList;
	ComboBoxList    m_ComboBoxList;
	ComboBoxTxtList m_ComboBoxTxtList;
	bool            m_UpdatingTxt;
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
