/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

class CBitmapPicture :
	public CWindowImpl <CBitmapPicture>
{
public:
	BEGIN_MSG_MAP(CBitmapPicture)
		MESSAGE_HANDLER(WM_PAINT, OnPaint);
	END_MSG_MAP()

	CBitmapPicture();

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL &/*bHandled*/);

	bool SetIcon(LPCWSTR lpszResourceName, uint32_t nWidth, uint32_t nHeight);
    void SetBitmap(LPCWSTR lpszResourceName);
    void SetBackroundBrush(HBRUSH brush);

private:
	CBitmapPicture(const CBitmapPicture&);
	CBitmapPicture& operator=(const CBitmapPicture&);

	bool CBitmapPicture::SetBitmap(HBITMAP hBitmap);

	int m_nResourceID;
	std::wstring m_strResourceName;
	uint32_t m_IconWidth, m_IconHeight;
	bool m_ResourceIcon;
	BITMAP m_bmInfo;
	CBitmap m_hBitmap;
	CBrush m_BackgroundBrush;
};