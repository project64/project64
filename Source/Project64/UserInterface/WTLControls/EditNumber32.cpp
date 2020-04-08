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
#include "stdafx.h"
#include "EditNumber32.h"

CEditNumber32::CEditNumber32(void) :
    m_DisplayType(DisplayDec)
{
}

CEditNumber32::~CEditNumber32(void)
{
}

bool CEditNumber32::IsHexConvertableText(LPTSTR _text)
{
    int start, end;
    GetSel(start, end);

    char WindowText[200];
    GetWindowText(WindowText, sizeof(WindowText));

    bool bPaste = true;
    size_t Len = strlen(WindowText);
    char head = Len > 0 ? WindowText[0] : 0;
    char second = Len > 1 ? WindowText[1] : 0;

    if (second == 'X' || second == 'x')
    {
        if (end <= 1)
        {
            bPaste = false;
        }
    }
    if (!bPaste) { return bPaste; }
    //Check
    unsigned int i = 0;
    if (strlen(_text) >= 2)
    {
        if (_text[0] == '0' && (_text[1] == 'x' || _text[1] == 'X'))
        {
            if ((second == 'x' || second == 'X') && (!(start == 0 && end >= 2)))
            {
                bPaste = false;
            }
            else if (start > 0)
            {
                bPaste = false;
            }
            else
            {
                i += 2;
            }
        }
    }
    if (!bPaste) return bPaste;
    if (strlen(_text) >= 1)
    {
        if (head == '0' && (_text[0] == 'x' || _text[0] == 'X'))
        {
            i++;
        }
        if ((_text[0] == 'x' || _text[0] == 'X'))
        {
            if (head != '0'&&start == 0)
            {
                bPaste = false;
            }
            else if (!(start == 1 && end >= 1 && head == '0'))
            {
                bPaste = false;
            }
        }
    }
    if (!bPaste) return bPaste;
    for (; i < strlen(_text); i++)
    {
        char c = _text[i];
        if (!(c >= 48 && c <= 57 || c >= 'A'&&c <= 'F' || c >= 'a'&&c <= 'f' || c == ' '))
        {
            bPaste = false;
            break;
        }
    }
    return bPaste;
}

void CEditNumber32::FormatClipboard()
{
    LPTSTR  lptstr, lptstrCopy;
    HGLOBAL hglb;
    if (!this->OpenClipboard() || !IsClipboardFormatAvailable(CF_TEXT))
    {
        return;
    }
    hglb = GetClipboardData(CF_TEXT);
    if (hglb != NULL)
    {
        lptstr = (LPTSTR)GlobalLock(hglb);
        for (unsigned int i = 0; i < strlen(lptstr); i++)
        {
            if (lptstr[i] != 'X'&&lptstr[i] != 'x')
            {
                lptstr[i] = (char)toupper(lptstr[i]);
            }
            if (lptstr[i] == 'X')
            {
                lptstr[i] = 'x';
            }
            if (lptstr[i] == ' ' && (i < strlen(lptstr)))
            {
                strcpy(&lptstr[i], &lptstr[i + 1]);
            }
        }
        hglb = GlobalAlloc(GMEM_MOVEABLE, (strlen(lptstr) + 1) * sizeof(TCHAR));
        if (hglb == NULL)
        {
            CloseClipboard();
            return;
        }
        lptstrCopy = (LPTSTR)GlobalLock(hglb);
        memcpy(lptstrCopy, lptstr, strlen(lptstr) + 1);
        GlobalUnlock(lptstr);
        GlobalUnlock(hglb);
        SetClipboardData(CF_TEXT, hglb);
        CloseClipboard();
    }
}

LRESULT CEditNumber32::OnValidateValue(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    bHandled = true;
    return true;
}

LRESULT CEditNumber32::OnPaste(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    //Paste
    bHandled = false;

    if (!IsClipboardFormatAvailable(CF_TEXT))
    {
        bHandled = true;
        return true;
    }
    if (!OpenClipboard())
    {
        bHandled = true;
        return true;
    }

    HGLOBAL hglb = GetClipboardData(CF_TEXT);
    if (hglb != NULL)
    {
        LPTSTR  lptstr = (LPTSTR)GlobalLock(hglb);
        //Check invalid hex string
        if (!IsHexConvertableText(lptstr))
        {
            bHandled = true;
        }
        GlobalUnlock(lptstr);
    }
    CloseClipboard();
    if (!bHandled)
    {
        FormatClipboard();
    }
    return true;
}

LRESULT CEditNumber32::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int start, end;
    GetSel(start, end);
    //CString text;
    char WindowText[200];
    GetWindowText(WindowText, sizeof(WindowText));
    size_t Len = strlen(WindowText);

    char head = Len > 0 ? WindowText[0] : 0;
    char second = Len > 1 ? WindowText[1] : 0;

    if (uMsg == WM_CHAR)
    {
        size_t MaxLen = 30;

        if (m_DisplayType == DisplayHex)
        {
            MaxLen = 8;
            if (second == 'x' || second == 'X')
            {
                MaxLen += 2;
            }
        }
        int c = (int)wParam;
        if (wParam < 32)
        {
            if (wParam == 8 && (second == 'x' || second == 'X') && head == '0' && end == 1)
            {
                //does not allow to delete '0' before x
                bHandled = true;
            }
            else {
                bHandled = false;
            }
            return TRUE;
        }

        if (second == 'x' || second == 'X')
        {
            //does not allow to change head except select includes first and second
            if (start <= 1 && end <= 1)
            {
                bHandled = true;
                return TRUE;
            }
        }
        if (start == 1 && (c == 'X' || c == 'x') && head == '0')
        {
            if (c == 'X')
            {
                SendMessage(uMsg, 'x', lParam);
                bHandled = true;
            }
            else {
                bHandled = false;
            }
            return true;
        }
        else if (c >= '0' && c <= '9' || c >= 'A' && c <= 'F')
        {
            if (Len >= MaxLen && start == end)
            {
                bHandled = true;
                return true;
            }
            bHandled = false;
            return true;
        }
        else if (c >= 'a' && c <= 'f')
        {
            if (Len >= MaxLen && start == end)
            {
                bHandled = true;
                return true;
            }
            SendMessage(uMsg, wParam - 32, lParam);
            bHandled = true;
            return true;
        }
        bHandled = true;
        return true;
    }

    bHandled = false;
    return false;
}

BOOL CEditNumber32::Attach(HWND hWndNew)
{
    return SubclassWindow(hWndNew);
}

BOOL CEditNumber32::AttachToDlgItem(HWND parent, UINT dlgID)
{
    return SubclassWindow(::GetDlgItem(parent, dlgID));
}

void CEditNumber32::SetDisplayType(DisplayType Type)
{
    DWORD lCurrentValue = GetValue();
    m_DisplayType = Type;
    SetValue(lCurrentValue);
}

uint32_t CEditNumber32::GetValue(void)
{
    char text[200];
    GetWindowText(text, sizeof(text));
    if (m_DisplayType == DisplayDec)
    {
        return atoi(text);
    }

    size_t Finish = strlen(text);
    char second = Finish > 1 ? text[1] : 0;
    size_t Start = (second == 'x' || second == 'X') ? 2 : 0;

    if (Finish > 8 + Start) { Finish = 8 + Start; }

    DWORD Value = 0;
    for (size_t i = Start; i < Finish; i++)
    {
        Value = (Value << 4);
        switch (text[i])
        {
        case '0': break;
        case '1': Value += 1; break;
        case '2': Value += 2; break;
        case '3': Value += 3; break;
        case '4': Value += 4; break;
        case '5': Value += 5; break;
        case '6': Value += 6; break;
        case '7': Value += 7; break;
        case '8': Value += 8; break;
        case '9': Value += 9; break;
        case 'A': Value += 10; break;
        case 'a': Value += 10; break;
        case 'B': Value += 11; break;
        case 'b': Value += 11; break;
        case 'C': Value += 12; break;
        case 'c': Value += 12; break;
        case 'D': Value += 13; break;
        case 'd': Value += 13; break;
        case 'E': Value += 14; break;
        case 'e': Value += 14; break;
        case 'F': Value += 15; break;
        case 'f': Value += 15; break;
        default:
            Value = (Value >> 4);
            i = Finish;
        }
    }
    return Value;
}

void CEditNumber32::SetValue(uint32_t Value, DisplayMode Display)
{
    char text[200];
    if (m_DisplayType == DisplayDec)
    {
        sprintf(text, "%d", Value);
    }
    else
    {
        sprintf(
            text,
            "%s%0*X",
            (Display & DisplayMode::ShowHexIdent) == DisplayMode::ShowHexIdent ? "0x" : "",
            (Display & DisplayMode::ZeroExtend) == DisplayMode::ZeroExtend ? 8 : 0,
            Value);
    }
    SetWindowText(text);
}