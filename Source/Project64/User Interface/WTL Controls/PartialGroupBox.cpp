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
#include "stdafx.h"

BOOL CPartialGroupBox::Attach(HWND hWndNew)
{
	return SubclassWindow(hWndNew);
}

BOOL CPartialGroupBox::AttachToDlgItem(HWND parent, UINT dlgID)
{
	return SubclassWindow(::GetDlgItem(parent,dlgID));
}

void CPartialGroupBox::Draw3dLine(CPaintDC & dc, LPCRECT lpRect, COLORREF clrTopLeft, COLORREF /*clrBottomRight*/)
{
	int x = lpRect->left;
	int y = lpRect->top;
	int cx = lpRect->right - lpRect->left;
	//int cy = lpRect->bottom - lpRect->top;

	dc.FillSolidRect(x, y, cx - 1, 1, clrTopLeft);
	//dc.FillSolidRect(x, y, 1, cy - 1, clrTopLeft);
	//dc.FillSolidRect(x + cx, y, -1, cy, clrBottomRight);
	//dc.FillSolidRect(x, y + cy, cx, -1, clrBottomRight);
}

void CPartialGroupBox::OnPaint(HDC /*hDC*/)
{
	CPaintDC dc(m_hWnd);

	//paint groupbox manually
	CRect controlrect;
	GetClientRect(controlrect);
	//::MapWindowPoints(HWND_DESKTOP, GetParent(), (LPPOINT)(LPRECT)controlrect, (sizeof(RECT)/sizeof(POINT)));

	CFontHandle font = GetFont();

	dc.SelectFont(font);
	dc.SetMapMode(MM_TEXT);
	dc.SelectBrush(GetSysColorBrush(COLOR_BTNFACE));

	TCHAR grptext[MAX_PATH];
	GetWindowText(grptext,MAX_PATH);

	CRect fontsizerect(0,0,0,0);
	dc.DrawText(grptext,-1,fontsizerect,DT_SINGLELINE|DT_LEFT|DT_CALCRECT);

	CRect framerect(controlrect);
	framerect.top += (fontsizerect.Height())/2;
	long Style =  GetStyle();

	if((Style & 0xF000) == BS_FLAT)
	{
		dc.Draw3dRect(framerect,RGB(0,0,0),RGB(0,0,0));
		framerect.DeflateRect(1,1);
		dc.Draw3dRect(framerect,RGB(255,255,255),RGB(255,255,255));
	}
	else
	{
		Draw3dLine(dc,framerect,GetSysColor(COLOR_3DSHADOW),GetSysColor(COLOR_3DHILIGHT));
		framerect.DeflateRect(1,1);
		Draw3dLine(dc,framerect,GetSysColor(COLOR_3DHILIGHT),GetSysColor(COLOR_3DSHADOW));
	}

	if(_tcslen(grptext))
	{
		CRect fontrect(controlrect);
		fontrect.bottom = controlrect.top+fontsizerect.Height();

		if((Style & 0xF00) == BS_RIGHT)
		{
			fontrect.right -= 6;
			fontrect.left = fontrect.right - fontsizerect.Width();
		}
		else if((Style & 0xF00) == BS_CENTER)
		{
			fontrect.left += (controlrect.Width()-fontsizerect.Width())/2;
			fontrect.right = fontrect.left + fontsizerect.Width();
		}
		else //BS_LEFT or default
		{
			fontrect.left += 6;
			fontrect.right = fontrect.left + fontsizerect.Width();
		}

		fontrect.InflateRect(2,0);
		dc.FillRect(fontrect,GetSysColor(COLOR_BTNFACE));
		fontrect.DeflateRect(2,0);

		//Draw Caption
		dc.SetBkMode(OPAQUE);
		dc.SetBkColor(GetSysColor(COLOR_BTNFACE));

		dc.DrawText(grptext,-1,fontrect,DT_SINGLELINE|DT_LEFT);
	}

}
