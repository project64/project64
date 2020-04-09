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

CModifiedEditBox::CModifiedEditBox(bool bString /* = true */, HWND hWnd /* = NULL */) :
CEdit(hWnd),
m_Changed(false),
m_Reset(false),
m_BoldFont(NULL),
m_OriginalFont(NULL),
m_TextField(NULL),
m_bString(bString)
{
}

CModifiedEditBox::~CModifiedEditBox()
{
    if (m_BoldFont)
    {
        DeleteObject(m_BoldFont);
    }
}

void CModifiedEditBox::SetReset(bool Reset)
{
    m_Reset = Reset;
    if (m_Reset)
    {
        SetChanged(false);
    }
}

void CModifiedEditBox::SetChanged(bool Changed)
{
    m_Changed = Changed;
    if (m_Changed)
    {
        SetReset(false);
        if (m_BoldFont == NULL)
        {
            m_OriginalFont = (HFONT)SendMessage(WM_GETFONT);

            LOGFONT lfSystemVariableFont;
            GetObject(m_OriginalFont, sizeof(LOGFONT), &lfSystemVariableFont);
            lfSystemVariableFont.lfWeight = FW_BOLD;

            m_BoldFont = CreateFontIndirect(&lfSystemVariableFont);
        }
        SendMessage(WM_SETFONT, (WPARAM)m_BoldFont);
        InvalidateRect(NULL);
        if (m_TextField)
        {
            ::SendMessage(m_TextField, WM_SETFONT, (WPARAM)m_BoldFont, 0);
            ::InvalidateRect(m_TextField, NULL, true);
        }
    }
    else
    {
        if (m_OriginalFont)
        {
            SendMessage(WM_SETFONT, (WPARAM)m_OriginalFont);
            InvalidateRect(NULL);
            if (m_TextField)
            {
                ::SendMessage(m_TextField, WM_SETFONT, (WPARAM)m_OriginalFont, 0);
                ::InvalidateRect(m_TextField, NULL, true);
            }
        }
    }
}

stdstr CModifiedEditBox::GetWindowText(void)
{
    ATLASSERT(::IsWindow(m_hWnd));
    return ::GetCWindowText(this);
}

void CModifiedEditBox::SetTextField(HWND hWnd)
{
    if (m_TextField && m_OriginalFont)
    {
        ::SendMessage(m_TextField, WM_SETFONT, (WPARAM)m_OriginalFont, 0);
    }
    m_TextField = hWnd;
    if (m_Changed && m_BoldFont)
    {
        ::SendMessage(m_TextField, WM_SETFONT, (WPARAM)m_BoldFont, 0);
    }
}