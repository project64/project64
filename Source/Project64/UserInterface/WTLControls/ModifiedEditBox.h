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

class CModifiedEditBox : 
	public CEdit
{
	bool   m_Changed;
	bool   m_Reset;
	HFONT  m_BoldFont;
	HFONT  m_OriginalFont;
	HWND   m_TextField;
	bool   m_bString;

public:
	// Constructors
	CModifiedEditBox(bool bString = true, HWND hWnd = NULL);
	~CModifiedEditBox();
	
	void SetReset ( bool Reset );
	void SetChanged (bool Changed);
	stdstr GetWindowText();
	void SetTextField (HWND hWnd);

	inline bool IsChanged ( void ) const 
	{
		return m_Changed;
	}
	inline bool IsReset ( void ) const 
	{
		return m_Reset;
	}
	inline bool IsbString ( void ) const 
	{
		return m_bString;
	}
};

