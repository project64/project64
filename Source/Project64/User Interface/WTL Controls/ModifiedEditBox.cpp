#include <User Interface.h>

CModifiedEditBox::CModifiedEditBox(HWND hWnd) : 
	CEdit(hWnd),
	m_Changed(false),
	m_Reset(false),
	m_BoldFont(NULL),
	m_OriginalFont(NULL)
{ 		
}

CModifiedEditBox::~CModifiedEditBox()
{
	if (m_BoldFont)
	{
		DeleteObject(m_BoldFont);
	}
}

void CModifiedEditBox::SetReset ( bool Reset )
{
	m_Reset = Reset;
	if (m_Reset)
	{
		SetChanged(false);
	}
}

void CModifiedEditBox::SetChanged (bool Changed)
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

stdstr CModifiedEditBox::GetWindowText( void )
{
	stdstr Result;
	ATLASSERT(::IsWindow(m_hWnd));

	int nLen = ::GetWindowTextLength(m_hWnd);
	if(nLen == 0)
	{
		return Result;
	}
	Result.resize(nLen+1);
	::GetWindowText(m_hWnd,(char *)Result.c_str(),nLen+1);
	return Result;
}
