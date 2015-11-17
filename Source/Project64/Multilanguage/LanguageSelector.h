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

class CLanguageSelector
{
public:
    CLanguageSelector();

    void Select(void);

private:
    CLanguageSelector(const CLanguageSelector&);				// Disable copy constructor
    CLanguageSelector& operator=(const CLanguageSelector&);		// Disable assignment

    static LRESULT CALLBACK LangSelectProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};