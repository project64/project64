#pragma  once

class CModifiedEditBox : 
	public CEdit
{
	bool   m_Changed;
	bool   m_Reset;
	HFONT  m_BoldFont;
	HFONT  m_OriginalFont;

public:
	// Constructors
	CModifiedEditBox(HWND hWnd = NULL);
	~CModifiedEditBox();
	
	void SetReset ( bool Reset );
	void SetChanged (bool Changed);
	stdstr GetWindowText();

	inline bool IsChanged ( void ) const 
	{
		return m_Changed;
	}
	inline bool IsReset ( void ) const 
	{
		return m_Reset;
	}

};

