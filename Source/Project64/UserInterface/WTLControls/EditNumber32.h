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

#include <wtl/atlctrls.h>
#include <stdio.h>

class CEditNumber :
    public CWindowImpl<CEditNumber, CEdit>
{
public:
    enum DisplayType
    {
        DisplayHex,
        DisplayDec,
    };

protected:
    enum
    {
        WM_VALIDATE_VALUE = WM_USER + 0x97
    };

    DisplayType  m_DisplayType;

    BEGIN_MSG_MAP(CEditNumber)
        MESSAGE_HANDLER(WM_CHAR, OnKeyDown)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_PASTE, OnPaste)
        MESSAGE_HANDLER(WM_VALIDATE_VALUE, OnValidateValue);
    END_MSG_MAP()

    bool IsHexConvertableText(LPTSTR _text);
    void FormatClipboard();

    LRESULT OnValidateValue(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
    LRESULT OnPaste(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:
    CEditNumber(void);
    virtual ~CEditNumber(void);

    BOOL Attach(HWND hWndNew);
    BOOL AttachToDlgItem(HWND parent, UINT dlgID);
    void SetDisplayType(DisplayType Type);
    DWORD GetValue(void);
    void SetValue(DWORD Value, bool ShowHexIdent = true, bool ZeroExtend = false);
};
