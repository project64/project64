#pragma once

template <class TParam>
class CModifiedComboBoxT : 
	public CComboBox
{
	typedef std::list<TParam *> TParamList;
	
public:
	// Constructors
	CModifiedComboBoxT(TParam defaultValue, HWND hWnd = nullptr, bool AllwaysSelected = true) : 
		CComboBox(hWnd),
		m_Changed(false),
		m_Reset(false),
		m_defaultValue(defaultValue),
		m_BoldFont(nullptr),
		m_OriginalFont(nullptr),
		m_TextField(nullptr),
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

	int AddItem (LPCTSTR strItem, const TParam & lParam) 
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
			if (m_BoldFont == nullptr)
			{
				m_OriginalFont = (HFONT)SendMessage(WM_GETFONT); 

				LOGFONT lfSystemVariableFont;
				GetObject ( m_OriginalFont, sizeof(LOGFONT), &lfSystemVariableFont );
				lfSystemVariableFont.lfWeight = FW_BOLD;

				m_BoldFont = CreateFontIndirect ( &lfSystemVariableFont );
			}
			SendMessage(WM_SETFONT,(WPARAM)m_BoldFont);
			InvalidateRect(nullptr);
			if (m_TextField)
			{
				::SendMessage(m_TextField,WM_SETFONT,(WPARAM)m_BoldFont,0);
				::InvalidateRect(m_TextField, nullptr, true);
					
			}
		} else {
			if (m_OriginalFont)
			{
				SendMessage(WM_SETFONT,(WPARAM)m_OriginalFont);
				InvalidateRect(nullptr);
				if (m_TextField)
				{
					::SendMessage(m_TextField,WM_SETFONT,(WPARAM)m_OriginalFont,0);
					::InvalidateRect(m_TextField, nullptr, true);
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
	CModifiedComboBoxT(void);
	CModifiedComboBoxT(const CModifiedComboBoxT&);
	CModifiedComboBoxT& operator=(const CModifiedComboBoxT&);

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
