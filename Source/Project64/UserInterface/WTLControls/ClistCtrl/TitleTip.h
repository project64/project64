#pragma once
#include <wtl/atlgdi.h>

class CTitleTip : public CWindowImpl<CTitleTip>
{
public:
    CTitleTip()
    {
        m_hWndParent = nullptr;
    }

    ~CTitleTip()
    {
    }

    DECLARE_WND_CLASS_EX(_T( "TitleTip" ), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_SAVEBITS, COLOR_WINDOW)

protected:
    HWND m_hWndParent;
    stdstr m_strToolTip;

    COLORREF m_rgbBackground;
    COLORREF m_rgbTextColour;
    COLORREF m_rgbBorderOuter;
    COLORREF m_rgbBorderInner;
    COLORREF m_rgbBackgroundTop;
    COLORREF m_rgbBackgroundBottom;

    CFont m_fntTitleFont;
    CToolTipCtrl m_ttToolTip;

public:
    BOOL Create(HWND hWndParent)
    {
        m_hWndParent = hWndParent;

        m_rgbBackground = GetSysColor(COLOR_INFOBK);
        m_rgbTextColour = GetSysColor(COLOR_INFOTEXT);
        m_rgbBorderOuter = RGB(220, 220, 220);
        m_rgbBorderInner = RGB(245, 245, 245);
        m_rgbBackgroundTop = RGB(250, 250, 250);
        m_rgbBackgroundBottom = RGB(235, 235, 235);

        CRect Area(nullptr);
        if (CWindowImpl<CTitleTip>::Create(hWndParent, Area, nullptr, WS_POPUP, WS_EX_TOOLWINDOW | WS_EX_TOPMOST) == nullptr)
            return FALSE;

        // Create the tooltip
        if (!m_ttToolTip.Create(m_hWnd))
            return FALSE;
        m_ttToolTip.SetMaxTipWidth(SHRT_MAX);

        // Get system message font
        WTL::CLogFont logFont;
        logFont.SetMessageBoxFont();
        if (!m_fntTitleFont.IsNull())
            m_fntTitleFont.DeleteObject();
        return (m_fntTitleFont.CreateFontIndirect(&logFont) != nullptr);
    }

    BOOL Show(CRect & rcRect, LPCTSTR lpszItemText, LPCTSTR lpszToolTip)
    {
        stdstr strItemText;
        strItemText.FromUTF16(lpszItemText);

        if (!IsWindow() || strItemText.empty())
            return FALSE;

        m_strToolTip.FromUTF16(lpszToolTip);
        SetWindowText(strItemText.ToUTF16().c_str());

        CClientDC dcClient(m_hWnd);

        HFONT hOldFont = dcClient.SelectFont(m_fntTitleFont);

        CRect rcTextExtent(rcRect);

        // Calculate item text extent
        dcClient.DrawTextW(strItemText.ToUTF16().c_str(), (int)strItemText.length(), rcTextExtent, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_CALCRECT);

        dcClient.SelectFont(hOldFont);

        // Do not show title tip if entire text is visible
        if (rcTextExtent.Width() <= rcRect.Width() - 1)
            return FALSE;

        if (m_strToolTip.empty())
            m_ttToolTip.Activate(FALSE);
        else
        {
            m_ttToolTip.Activate(TRUE);
            m_ttToolTip.AddTool(m_hWnd, (LPCTSTR)m_strToolTip.substr(0, SHRT_MAX).c_str());
        }

        // Show title tip at new location
        if (!SetWindowPos(nullptr, rcRect.left - 4, rcRect.top, rcTextExtent.Width() + 11, rcRect.Height(), SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS))
            return FALSE;

        SetCapture();

        return TRUE;
    }

    BOOL Hide()
    {
        if (GetCapture() == m_hWnd)
            ReleaseCapture();
        return IsWindow() ? ShowWindow(SW_HIDE) : FALSE;
    }

    BEGIN_MSG_MAP_EX(CTitleTip)
    MSG_WM_DESTROY(OnDestroy)
    MESSAGE_RANGE_HANDLER_EX(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseRange)
    MSG_WM_ERASEBKGND(OnEraseBkgnd)
    MSG_WM_PAINT(OnPaint)
    END_MSG_MAP()

    void OnDestroy()
    {
        if (m_ttToolTip.IsWindow())
            m_ttToolTip.DestroyWindow();
        m_ttToolTip.m_hWnd = nullptr;
    }

    LRESULT OnMouseRange(UINT nMessage, WPARAM wParam, LPARAM lParam)
    {
        SetMsgHandled(FALSE);

        if (m_ttToolTip.IsWindow())
        {
            MSG msgRelay = {m_hWnd, nMessage, wParam, lParam};
            m_ttToolTip.RelayEvent(&msgRelay);
        }

        CPoint ptMouse(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        ClientToScreen(&ptMouse);

        if (nMessage == WM_MOUSEMOVE)
        {
            CRect rcWindow;
            GetWindowRect(rcWindow);
            if (!rcWindow.PtInRect(ptMouse))
                Hide();
            return 0;
        }

        CWindow wndParent(m_hWndParent);
        UINT nHitTest = (UINT)wndParent.SendMessage(WM_NCHITTEST, 0, MAKELPARAM(ptMouse.x, ptMouse.y));

        // Forward notification through to parent
        if (nHitTest == HTCLIENT)
        {
            wndParent.ScreenToClient(&ptMouse);
            wndParent.PostMessage(nMessage, wParam, MAKELPARAM(ptMouse.x, ptMouse.y));
        }
        else
        {
            switch (nMessage)
            {
            case WM_LBUTTONDOWN:
                wndParent.PostMessage(WM_NCLBUTTONDOWN, nHitTest, MAKELPARAM(ptMouse.x, ptMouse.y));
                break;
            case WM_LBUTTONUP:
                wndParent.PostMessage(WM_NCLBUTTONUP, nHitTest, MAKELPARAM(ptMouse.x, ptMouse.y));
                break;
            case WM_LBUTTONDBLCLK:
                wndParent.PostMessage(WM_NCLBUTTONDBLCLK, nHitTest, MAKELPARAM(ptMouse.x, ptMouse.y));
                break;
            case WM_RBUTTONDOWN:
                wndParent.PostMessage(WM_NCRBUTTONDOWN, nHitTest, MAKELPARAM(ptMouse.x, ptMouse.y));
                break;
            case WM_RBUTTONUP:
                wndParent.PostMessage(WM_NCRBUTTONUP, nHitTest, MAKELPARAM(ptMouse.x, ptMouse.y));
                break;
            case WM_RBUTTONDBLCLK:
                wndParent.PostMessage(WM_NCRBUTTONDBLCLK, nHitTest, MAKELPARAM(ptMouse.x, ptMouse.y));
                break;
            case WM_MBUTTONDOWN:
                wndParent.PostMessage(WM_NCMBUTTONDOWN, nHitTest, MAKELPARAM(ptMouse.x, ptMouse.y));
                break;
            case WM_MBUTTONUP:
                wndParent.PostMessage(WM_NCMBUTTONUP, nHitTest, MAKELPARAM(ptMouse.x, ptMouse.y));
                break;
            case WM_MBUTTONDBLCLK:
                wndParent.PostMessage(WM_NCMBUTTONDBLCLK, nHitTest, MAKELPARAM(ptMouse.x, ptMouse.y));
                break;
            }
        }

        return 0;
    }

    BOOL OnEraseBkgnd(HDC /*dc*/)
    {
        return TRUE;
    }

    void OnPaint(HDC /*dc*/)
    {
        CPaintDC dcPaint(m_hWnd);

        int nContextState = dcPaint.SaveDC();

        CRect rcClient;
        GetClientRect(rcClient);

        CRect rcTitleTip(rcClient);

        dcPaint.SetBkColor(m_rgbBackground);
        dcPaint.ExtTextOut(rcTitleTip.left, rcTitleTip.top, ETO_OPAQUE, rcTitleTip, _T( "" ), 0, nullptr);

        CBrush bshTitleFrame;
        bshTitleFrame.CreateSolidBrush(m_rgbTextColour);
        dcPaint.FrameRect(rcTitleTip, bshTitleFrame);

        int nTextLength = GetWindowTextLength();
        stdstr strItemText;
        strItemText.reserve(nTextLength + 1);
        strItemText.resize(nTextLength);
        GetWindowText((LPTSTR)strItemText.c_str(), nTextLength + 1);

        dcPaint.SelectFont(m_fntTitleFont);
        dcPaint.SetTextColor(m_rgbTextColour);
        dcPaint.SetBkMode(TRANSPARENT);

        CRect rcItemText(rcClient);
        rcItemText.OffsetRect(4, 0);

        dcPaint.DrawTextW(strItemText.ToUTF16().c_str(), (int)strItemText.length(), rcItemText, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);

        dcPaint.RestoreDC(nContextState);
    }
};
