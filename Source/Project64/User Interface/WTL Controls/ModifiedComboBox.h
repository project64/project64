#pragma  once

template <class TParam>
class CModifiedComboBoxT : 
	public CComboBox
{
	bool   m_Changed;
	bool   m_Reset;
	TParam m_defaultValue;
	HFONT  m_BoldFont;
	HFONT  m_OriginalFont;
	HWND   m_TextField;

public:
	// Constructors
	CModifiedComboBoxT(HWND hWnd = NULL) : 
	    CComboBox(hWnd),
		m_Changed(false),
		m_Reset(false),
		m_BoldFont(NULL),
		m_OriginalFont(NULL),
		m_TextField(NULL)
	{ 		
	}
	
	CModifiedComboBoxT(TParam defaultValue, HWND hWnd = NULL ) : 
	    CComboBox(hWnd),
		m_Changed(false),
		m_Reset(false),
		m_BoldFont(NULL),
		m_OriginalFont(NULL),
		m_defaultValue(),
		m_TextField(NULL)
	{ 		
	}

	~CModifiedComboBoxT()
	{
		if (m_BoldFont)
		{
			DeleteObject(m_BoldFont);
		}
	}

	int AddItem (LPCSTR strItem, TParam lParam) 
	{
		int indx = AddString(strItem);
		SetItemData(indx,(DWORD_PTR)lParam);
		if (GetCount() == 1 || m_defaultValue == lParam) 
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

	inline void SetDefault (TParam defaultValue)
	{
		m_defaultValue = defaultValue;
		for (int i = 0, n = GetCount(); i < n; i++)
		{
			if (GetItemData(i) == m_defaultValue)
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

};

typedef CModifiedComboBoxT<WPARAM>   CModifiedComboBox;
