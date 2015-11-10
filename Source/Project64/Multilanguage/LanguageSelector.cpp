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

void CLanguageSelector::Select ( void ) 
{
    DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_Lang_Select),NULL,(DLGPROC)LangSelectProc, (LPARAM)this);
}

static WNDPROC pfnWndLangSelectOkProc = NULL;
static HBITMAP hOkButton = NULL;

DWORD CALLBACK LangSelectOkProc (HWND hWnd, DWORD uMsg, DWORD wParam, DWORD lParam)
{
    static bool m_fPressed = false;
    static HBITMAP hOkButtonDown = NULL;

    switch (uMsg)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;

            if (BeginPaint(hWnd,&ps))
            {
                if (m_fPressed)
                {
                    if (hOkButtonDown == NULL)
                    {
                        hOkButtonDown = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_LANG_OK_DOWN));
                    }
                    if (hOkButtonDown)
                    {
                        RECT rcClient;
                        GetClientRect(hWnd, &rcClient);

                        BITMAP bmTL1;
                        GetObject(hOkButtonDown, sizeof(BITMAP), &bmTL1);
                        HDC     memdc	= CreateCompatibleDC(ps.hdc);
                        HGDIOBJ save	= SelectObject(memdc, hOkButtonDown);
                        BitBlt(ps.hdc, 0, 0, bmTL1.bmWidth, bmTL1.bmHeight, memdc, 0, 0, SRCCOPY);
                        SelectObject(memdc, save);
                        DeleteDC(memdc);
                    }
                }
                else
                {
                    if (hOkButton)
                    {
                        RECT rcClient;
                        GetClientRect(hWnd, &rcClient);

                        BITMAP bmTL1;
                        GetObject(hOkButton, sizeof(BITMAP), &bmTL1);
                        HDC     memdc	= CreateCompatibleDC(ps.hdc);
                        HGDIOBJ save	= SelectObject(memdc, hOkButton);
                        BitBlt(ps.hdc, 0, 0, bmTL1.bmWidth, bmTL1.bmHeight, memdc, 0, 0, SRCCOPY);
                        SelectObject(memdc, save);
                        DeleteDC(memdc);
                    }
                }
                EndPaint(hWnd,&ps);
            }
        }
        break;
    case WM_MOUSEMOVE:
        if (::GetCapture() == hWnd)
        {
            POINT ptCursor = { ((int)(short)LOWORD(lParam)), ((int)(short)HIWORD(lParam)) };
            ClientToScreen(hWnd, &ptCursor);
            RECT rect;
            GetWindowRect(hWnd, &rect);
            bool uPressed = ::PtInRect(&rect, ptCursor)==TRUE;
            if ( m_fPressed != uPressed )
            {
                m_fPressed = uPressed;
                ::InvalidateRect(hWnd, NULL, TRUE);
                UpdateWindow(hWnd);
            }
        }
        break;
    case WM_LBUTTONDOWN:
        {
            LRESULT lRet = 0;
            lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
            SetCapture(hWnd);
            if ( ::GetCapture()==hWnd )
            {
                m_fPressed = true;

                if (m_fPressed)
                {
                    ::InvalidateRect(hWnd, NULL, TRUE);
                    UpdateWindow(hWnd);
                }
            }
            return lRet;
        }
        break;
    case WM_LBUTTONUP:
        {
            LRESULT lRet = 0;
            lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
            if ( ::GetCapture() == hWnd )
            {
                ::ReleaseCapture();
                if ( m_fPressed )
                {
                    ::SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), BN_CLICKED), (LPARAM)hWnd);
                }
            }
            m_fPressed = false;

            return lRet;
        }
        break;
    }

    return CallWindowProc(pfnWndLangSelectOkProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CLanguageSelector::LangSelectProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP hbmpBackgroundTop = NULL;
    static HBITMAP hbmpBackgroundBottom = NULL;
    static HBITMAP hbmpBackgroundMiddle = NULL;
    static HFONT   hTextFont = NULL;
    static CLanguageSelector * lngClass;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowPos(hDlg,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE);
        {
            lngClass = (CLanguageSelector *)lParam;

            LanguageList LangList = g_Lang->GetLangList();
            if (LangList.size() == 0)
            {
                EndDialog(hDlg,0);
            }
            for (LanguageList::iterator Language = LangList.begin(); Language != LangList.end(); Language++)
            {
                int index = SendMessageW(GetDlgItem(hDlg,IDC_LANG_SEL),CB_ADDSTRING,0,(WPARAM)Language->LanguageName.c_str());
                if (_wcsicmp(Language->LanguageName.c_str(),L"English") == 0)
                {
                    SendMessage(GetDlgItem(hDlg,IDC_LANG_SEL),CB_SETCURSEL,index,0);
                }
            }

            int Index = SendMessage(GetDlgItem(hDlg,IDC_LANG_SEL),CB_GETCURSEL,0,0);
            if (Index < 0)
            {
                SendMessage(GetDlgItem(hDlg,IDC_LANG_SEL),CB_SETCURSEL,0,0);
            }

            enum { ROUND_EDGE = 15 };

            DWORD dwStyle = GetWindowLong(hDlg, GWL_STYLE);
            dwStyle &= ~(WS_CAPTION|WS_SIZEBOX);
            SetWindowLong(hDlg, GWL_STYLE, dwStyle);

            // Use the size of the image
            hbmpBackgroundTop    = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT_TOP));
            hbmpBackgroundBottom = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT_BOTTOM));
            hbmpBackgroundMiddle = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT_MIDDLE));
            BITMAP bmTL;
            GetObject(hbmpBackgroundTop, sizeof(BITMAP), &bmTL);

            if (hbmpBackgroundTop)
            {
                //				int iHeight = bmTL.bmHeight;
                int iWidth  = bmTL.bmWidth;

                RECT rect;
                GetWindowRect(hDlg, &rect);
                rect.left -= rect.left;
                rect.bottom -= rect.top;
                rect.top -= rect.top;

                // Tweaked
                HRGN hWindowRegion= CreateRoundRectRgn
                    (
                    rect.left,
                    rect.top,
                    rect.left+iWidth+GetSystemMetrics(SM_CXEDGE)-1,
                    rect.bottom+GetSystemMetrics(SM_CYEDGE)-1,
                    ROUND_EDGE,
                    ROUND_EDGE
                    );

                if (hWindowRegion)
                {
                    SetWindowRgn(hDlg, hWindowRegion, TRUE);
                    DeleteObject(hWindowRegion);
                }
            }
            hTextFont = ::CreateFont
                (
                18,
                0,
                0,
                0,
                FW_NORMAL,
                0,
                0,
                0,
                DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                PROOF_QUALITY,
                DEFAULT_PITCH|FF_DONTCARE,
                "Arial"
                );
            SendDlgItemMessage(hDlg,IDC_SELECT_LANG,WM_SETFONT,(WPARAM)hTextFont,TRUE);
        }

        hOkButton                = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_LANG_OK));
        pfnWndLangSelectOkProc   = (WNDPROC)::GetWindowLongPtr(GetDlgItem(hDlg,IDOK), GWLP_WNDPROC);
        ::SetWindowLongPtr(GetDlgItem(hDlg,IDOK), GWLP_WNDPROC,(LONG_PTR)LangSelectOkProc);
        break;
    case WM_NCHITTEST:
        {
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);
            RECT client, a;
            GetClientRect(hDlg,&a);
            GetClientRect(hDlg,&client);
            ClientToScreen(hDlg,(LPPOINT)&client);
            client.right += client.left;
            client.bottom += client.top;

            int nCaption = GetSystemMetrics(SM_CYCAPTION)*4;

            LRESULT lResult = HTCLIENT;

            //check caption
            if (xPos <= client.right && xPos >= client.left &&
                (yPos >= client.top+ 0)&& (yPos <= client.top + 0+nCaption))
            {
                lResult = HTCAPTION;
            }
            SetWindowLong(hDlg, DWLP_MSGRESULT, lResult);
            return TRUE;
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
    case WM_PAINT:
        {
            PAINTSTRUCT ps;

            if (BeginPaint(hDlg,&ps))
            {
                RECT rcClient;
                GetClientRect(hDlg, &rcClient);

                BITMAP bmTL_top, bmTL_bottom, bmTL_Middle;
                GetObject(hbmpBackgroundTop, sizeof(BITMAP), &bmTL_top);
                GetObject(hbmpBackgroundBottom, sizeof(BITMAP), &bmTL_bottom);
                GetObject(hbmpBackgroundMiddle, sizeof(BITMAP), &bmTL_Middle);

                HDC     memdc	= CreateCompatibleDC(ps.hdc);
                HGDIOBJ save	= SelectObject(memdc, hbmpBackgroundTop);
                BitBlt(ps.hdc, 0, 0, bmTL_top.bmWidth, bmTL_top.bmHeight, memdc, 0, 0, SRCCOPY);
                SelectObject(memdc, save);
                DeleteDC(memdc);

                memdc	= CreateCompatibleDC(ps.hdc);
                save	= SelectObject(memdc, hbmpBackgroundMiddle);
                for (int x = bmTL_top.bmHeight; x < rcClient.bottom; x += bmTL_Middle.bmHeight)
                {
                    //BitBlt(ps.hdc, 0, bmTL_top.bmHeight, bmTL_Middle.bmWidth, rcClient.bottom - (bmTL_bottom.bmHeight + bmTL_top.bmHeight), memdc, 0, 0, SRCCOPY);
                    BitBlt(ps.hdc, 0, x, bmTL_Middle.bmWidth, bmTL_Middle.bmHeight, memdc, 0, 0, SRCCOPY);
                }
                SelectObject(memdc, save);
                DeleteDC(memdc);

                BITMAP ;
                memdc	= CreateCompatibleDC(ps.hdc);
                save	= SelectObject(memdc, hbmpBackgroundBottom);
                BitBlt(ps.hdc, 0, rcClient.bottom - bmTL_bottom.bmHeight, bmTL_bottom.bmWidth, bmTL_bottom.bmHeight, memdc, 0, 0, SRCCOPY);
                SelectObject(memdc, save);
                DeleteDC(memdc);

                BITMAP ;

                EndPaint(hDlg,&ps);
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
            if (hbmpBackgroundBottom)
            {
                DeleteObject(hbmpBackgroundBottom);
            }
            if (hbmpBackgroundMiddle)
            {
                DeleteObject(hbmpBackgroundMiddle);
            }

            if (hTextFont)
            {
                ::DeleteObject(hTextFont);
            }

            {
                int Index = SendMessage(GetDlgItem(hDlg,IDC_LANG_SEL),CB_GETCURSEL,0,0);

                if (Index >= 0)
                {
                    wchar_t String[255];
                    SendMessageW(GetDlgItem(hDlg,IDC_LANG_SEL),CB_GETLBTEXT,Index,(LPARAM)String);
                    g_Lang->SetLanguage(String);
                }
            }

            EndDialog(hDlg,0);
            break;
        }
    default:
        return FALSE;
    }
    return TRUE;
}