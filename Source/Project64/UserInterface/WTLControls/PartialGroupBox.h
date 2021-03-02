#pragma once

class CPartialGroupBox : 
	public CWindowImpl<CPartialGroupBox, CButton>
{
	void Draw3dLine(CPaintDC & dc, LPCRECT lpRect, COLORREF clrTopLeft, COLORREF clrBottomRight);

public:
	BEGIN_MSG_MAP_EX(CPartialGroupBox)
		MSG_WM_PAINT(OnPaint)
	END_MSG_MAP()

	// Constructors
	CPartialGroupBox()
	{ 		
	}
	
	virtual ~CPartialGroupBox()
	{
	}

	BOOL Attach(HWND hWndNew);
	BOOL AttachToDlgItem(HWND parent, UINT dlgID);
	void OnPaint(HDC hDC);
};
