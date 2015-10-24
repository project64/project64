/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

template <class TParam>
class CModifiedComboBoxT : 
	public CComboBox
{
	typedef std::list<TParam *> TParamList;
	
public:
	// Constructors
	CModifiedComboBoxT(TParam defaultValue, HWND hWnd = NULL, bool AllwaysSelected = true) : 
		CComboBox(hWnd),
		m_Changed(false),
		m_Reset(false),
		m_defaultValue(defaultValue),
		m_BoldFont(NULL),
		m_OriginalFont(NULL),
		m_TextField(NULL),
		m_AllwaysSelected(AllwaysSelected)
	{ 		
	}
	
	~CModifiedComboBoxT()
	{
		if (m_BoldFont)
		{
			DeleteObject(m_BoldFont);
		}
		for (TParamList::iterator iter = m_ParamList.begin(); iter != m_ParamList.end(); iter ++)
		{
			TParam * Item = (TParam *)*iter;
			if (Item)
			{
				delete Item;
			}
		}
	}

	int AddItem (LPCSTR strItem, const TParam & lParam) 
	{
		int indx = AddString(strItem);
		TParam * Value = new TParam(lParam);
		SetItemData(indx,(DWORD_PTR)(Value));
		m_ParamList.push_back(Value);
		if ((m_AllwaysSelected && GetCount() == 1) || m_defaultValue == lParam) 
		{ 
			SetCurSel(indx);
		}
		return indx;
	}

	int AddItemW (LPCWSTR strItem, const TParam & lParam) 
	{
		int indx = AddStringW(strItem);
		TParam * Value = new TParam(lParam);
		SetItemData(indx,(DWORD_PTR)(Value));
		m_ParamList.push_back(Value);
		if ((m_AllwaysSelected && GetCount() == 1) || m_defaultValue == lParam) 
		{ 
			SetCurSel(indx);
		}
		return indx;
	}

    void SetReset ( bool Reset )
	{
		m_Reset = Reset;
		if (m_Reset)
		{
			SetChanged(false);
		}
	}
	
	void SetChanged (bool Changed)
	{
		m_Changed = Changed;
		if (m_Changed)
		{
			SetReset(false);
			if (m_BoldFont == NULL)
			{
				m_OriginalFont = (HFONT)SendMessage(WM_GETFONT); 

				LOGFONT lfSystemVariableFont;
				GetObject ( m_OriginalFont, sizeof(LOGFONT), &lfSystemVariableFont );
				lfSystemVariableFont.lfWeight = FW_BOLD;

				m_BoldFont = CreateFontIndirect ( &lfSystemVariableFont );
			}
			SendMessage(WM_SETFONT,(WPARAM)m_BoldFont);
			InvalidateRect(NULL);
			if (m_TextField)
			{
				::SendMessage(m_TextField,WM_SETFONT,(WPARAM)m_BoldFont,0);
				::InvalidateRect(m_TextField, NULL, true);
					
			}
		} else {
			if (m_OriginalFont)
			{
				SendMessage(WM_SETFONT,(WPARAM)m_OriginalFont);
				InvalidateRect(NULL);
				if (m_TextField)
				{
					::SendMessage(m_TextField,WM_SETFONT,(WPARAM)m_OriginalFont,0);
					::InvalidateRect(m_TextField, NULL, true);
				}
			}
		}
	}

	void SetTextField (HWND hWnd)
	{
		if (m_TextField && m_OriginalFont)
		{
			::SendMessage(m_TextField,WM_SETFONT,(WPARAM)m_OriginalFont,0);
		}
		m_TextField = hWnd;
		if (m_Changed && m_BoldFont)
		{
			::SendMessage(m_TextField,WM_SETFONT,(WPARAM)m_BoldFont,0);
		}
	}

	inline void SetDefault (const TParam & defaultValue)
	{
		m_defaultValue = defaultValue;
		for (int i = 0, n = GetCount(); i < n; i++)
		{
			if (*((TParam *)GetItemData(i)) == m_defaultValue)
			{
				SetCurSel(i);
				break;
			}
		}
	}

	inline bool IsChanged ( void ) const 
	{
		return m_Changed;
	}
	inline bool IsReset ( void ) const 
	{
		return m_Reset;
	}

private:
	CModifiedComboBoxT(void);									// Disable default constructor
	CModifiedComboBoxT(const CModifiedComboBoxT&);				// Disable copy constructor
	CModifiedComboBoxT& operator=(const CModifiedComboBoxT&);	// Disable assignment

	bool   m_Changed;
	bool   m_Reset;
	TParam m_defaultValue;
	HFONT  m_BoldFont;
	HFONT  m_OriginalFont;
	HWND   m_TextField;
	bool   m_AllwaysSelected;

	TParamList m_ParamList;


};

typedef CModifiedComboBoxT<WPARAM>   CModifiedComboBox;
typedef CModifiedComboBoxT<stdstr>   CModifiedComboBoxTxt;
