#pragma once

class CModifiedButton : 
	public CButton
{
	bool   m_Changed;
	bool   m_Reset;
	HFONT  m_BoldFont;
	HFONT  m_OriginalFont;


public:
	// Constructors
	CModifiedButton(HWND hWnd = NULL) : 
	    CButton(hWnd),
		m_Changed(false),
		m_Reset(false),
		m_BoldFont(NULL),
		m_OriginalFont(NULL)
	{ 		
	}
	
	~CModifiedButton()
	{
		if (m_BoldFont)
		{
			DeleteObject(m_BoldFont);
		}
	}

	void SetReset (bool Reset)
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
		} else {
			if (m_OriginalFont)
			{
				SendMessage(WM_SETFONT,(WPARAM)m_OriginalFont);
				InvalidateRect(NULL);
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
