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
#include "LanguageSelector.h"

CLanguageSelector::CLanguageSelector()
{
}

void CLanguageSelector::Select(void)
{
    DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_Lang_Select), NULL, (DLGPROC)LangSelectProc, (LPARAM)this);
}

LRESULT CALLBACK CLanguageSelector::LangSelectProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP hbmpBackgroundTop = NULL;
    static HFONT   hTextFont = NULL;
    static CLanguageSelector * lngClass;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        lngClass = (CLanguageSelector *)lParam;

        LanguageList LangList = g_Lang->GetLangList();
        if (LangList.size() == 0)
        {
            EndDialog(hDlg, 0);
        }
        for (LanguageList::iterator Language = LangList.begin(); Language != LangList.end(); Language++)
        {
            int index = SendMessageW(GetDlgItem(hDlg, IDC_LANG_SEL), CB_ADDSTRING, 0, (WPARAM)stdstr(Language->LanguageName).ToUTF16().c_str());
            if (_stricmp(Language->LanguageName.c_str(), "English") == 0)
            {
                SendMessage(GetDlgItem(hDlg, IDC_LANG_SEL), CB_SETCURSEL, index, 0);
            }
        }

        int Index = SendMessage(GetDlgItem(hDlg, IDC_LANG_SEL), CB_GETCURSEL, 0, 0);
        if (Index < 0)
        {
            SendMessage(GetDlgItem(hDlg, IDC_LANG_SEL), CB_SETCURSEL, 0, 0);
        }

        //Get Windows DPI Scale
        float DPIScale = CClientDC(hDlg).GetDeviceCaps(LOGPIXELSX) / 96.0f;

        // Use the size of the image
        hbmpBackgroundTop = LoadBitmap(GetModuleHandle(NULL), DPIScale <= 1.0f ? MAKEINTRESOURCE(IDB_ABOUT_LOGO) : MAKEINTRESOURCE(IDB_ABOUT_LOGO_HDPI));
        BITMAP bmTL;
        GetObject(hbmpBackgroundTop, sizeof(BITMAP), &bmTL);

        hTextFont = ::CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
        SendDlgItemMessage(hDlg, IDC_SELECT_LANG, WM_SETFONT, (WPARAM)hTextFont, TRUE);
    }
    break;
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
    }
    break;
    case WM_ERASEBKGND:
    {
        HPEN outline;
        HBRUSH fill;
        RECT rect;

        outline = CreatePen(PS_SOLID, 1, 0x00FFFFFF);
        fill = CreateSolidBrush(0x00FFFFFF);
        SelectObject((HDC)wParam, outline);
        SelectObject((HDC)wParam, fill);

        GetClientRect(hDlg, &rect);

        Rectangle((HDC)wParam, rect.left, rect.top, rect.right, rect.bottom);
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        if (BeginPaint(hDlg, &ps))
        {
            RECT rcClient;
            GetClientRect(hDlg, &rcClient);

            BITMAP bmTL_top;
            GetObject(hbmpBackgroundTop, sizeof(BITMAP), &bmTL_top);

            HDC     memdc = CreateCompatibleDC(ps.hdc);
            HGDIOBJ save = SelectObject(memdc, hbmpBackgroundTop);
            SetStretchBltMode(ps.hdc, HALFTONE);
            StretchBlt(ps.hdc, 0, 0, rcClient.right, (int)(bmTL_top.bmHeight * rcClient.right / bmTL_top.bmWidth), memdc, 0, 0, bmTL_top.bmWidth, bmTL_top.bmHeight, SRCCOPY);
            SelectObject(memdc, save);
            DeleteDC(memdc);

            EndPaint(hDlg, &ps);
        }
    }
    break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (hbmpBackgroundTop)
            {
                DeleteObject(hbmpBackgroundTop);
            }

            if (hTextFont)
            {
                ::DeleteObject(hTextFont);
            }

            {
                int Index = SendMessage(GetDlgItem(hDlg, IDC_LANG_SEL), CB_GETCURSEL, 0, 0);

                if (Index >= 0)
                {
                    wchar_t String[255];
                    SendMessageW(GetDlgItem(hDlg, IDC_LANG_SEL), CB_GETLBTEXT, Index, (LPARAM)String);
                    g_Lang->SetLanguage(stdstr().FromUTF16(String).c_str());
                }
            }

            EndDialog(hDlg, 0);
            break;
        }
    default:
        return FALSE;
    }
    return TRUE;
}