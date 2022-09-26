#pragma once
#include <wtl/atlmisc.h>

class CDropArrows : public CWindowImpl<CDropArrows>
{
public:
    CDropArrows()
    {
        m_bVertical = FALSE;
        m_nSpanLength = 0;
    }

    ~CDropArrows()
    {
    }

    DECLARE_WND_CLASS(_T( "DropArrows" ))

protected:
    CBrush m_bshArrowBrush;
    CRgn m_rgnArrowRegion;
    BOOL m_bVertical;
    int m_nSpanLength;

public:
    BOOL Create(HWND hWndParent, int nSpanLength, BOOL bVertical)
    {
        if (!m_bshArrowBrush.CreateSolidBrush(RGB(255, 0, 0)))
            return FALSE;

        m_bVertical = bVertical;
        m_nSpanLength = nSpanLength + 20;

        CRect area(0, 0, m_bVertical ? 12 : m_nSpanLength, m_bVertical ? m_nSpanLength : 12);
        if (CWindowImpl<CDropArrows>::Create(hWndParent, area, nullptr, WS_POPUP | WS_DISABLED, WS_EX_TOOLWINDOW) == nullptr)
            return FALSE;

        POINT ptArrow[7];

        ptArrow[0].x = bVertical ? 8 : 0;
        ptArrow[0].y = bVertical ? 0 : 9;
        ptArrow[1].x = bVertical ? 8 : 4;
        ptArrow[1].y = bVertical ? 4 : 9;
        ptArrow[2].x = bVertical ? 11 : 4;
        ptArrow[2].y = bVertical ? 4 : 12;
        ptArrow[3].x = bVertical ? 6 : 10;
        ptArrow[3].y = bVertical ? 9 : 6;
        ptArrow[4].x = bVertical ? 1 : 4;
        ptArrow[4].y = bVertical ? 4 : 0;
        ptArrow[5].x = bVertical ? 4 : 4;
        ptArrow[5].y = bVertical ? 4 : 4;
        ptArrow[6].x = bVertical ? 4 : 0;
        ptArrow[6].y = bVertical ? 0 : 4;

        CRgn rgnFirstArrow;
        if (!rgnFirstArrow.CreatePolygonRgn(ptArrow, 7, ALTERNATE))
            return FALSE;

        ptArrow[0].x = bVertical ? 4 : m_nSpanLength;
        ptArrow[0].y = bVertical ? m_nSpanLength : 4;
        ptArrow[1].x = bVertical ? 4 : m_nSpanLength - 4;
        ptArrow[1].y = bVertical ? m_nSpanLength - 4 : 4;
        ptArrow[2].x = bVertical ? 0 : m_nSpanLength - 4;
        ptArrow[2].y = bVertical ? m_nSpanLength - 4 : 0;
        ptArrow[3].x = bVertical ? 6 : m_nSpanLength - 10;
        ptArrow[3].y = bVertical ? m_nSpanLength - 10 : 6;
        ptArrow[4].x = bVertical ? 12 : m_nSpanLength - 4;
        ptArrow[4].y = bVertical ? m_nSpanLength - 4 : 12;
        ptArrow[5].x = bVertical ? 8 : m_nSpanLength - 4;
        ptArrow[5].y = bVertical ? m_nSpanLength - 4 : 9;
        ptArrow[6].x = bVertical ? 8 : m_nSpanLength;
        ptArrow[6].y = bVertical ? m_nSpanLength : 9;

        CRgn rgnSecondArrow;
        if (!rgnSecondArrow.CreatePolygonRgn(ptArrow, 7, ALTERNATE))
            return FALSE;

        if (!m_rgnArrowRegion.CreateRectRgn(0, 0, bVertical ? 12 : nSpanLength, bVertical ? nSpanLength : 12))
            return FALSE;

        m_rgnArrowRegion.CombineRgn(rgnFirstArrow, rgnSecondArrow, RGN_OR);

        SetWindowRgn(m_rgnArrowRegion, FALSE);

        return TRUE;
    }

    BOOL Show(CPoint point)
    {
        return IsWindow() ? SetWindowPos(nullptr, m_bVertical ? point.x - 7 : point.x - (m_nSpanLength / 2), m_bVertical ? point.y - (m_nSpanLength / 2) : point.y - 5, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE) : FALSE;
    }

    BOOL Hide()
    {
        return IsWindow() ? ShowWindow(SW_HIDE) : FALSE;
    }

    BEGIN_MSG_MAP_EX(CDropArrows)
    MSG_WM_DESTROY(OnDestroy)
    MSG_WM_ERASEBKGND(OnEraseBkgnd)
    END_MSG_MAP()

    void OnDestroy()
    {
        m_bshArrowBrush.DeleteObject();
        m_rgnArrowRegion.DeleteObject();
    }

    BOOL OnEraseBkgnd(HDC dc)
    {
        CDCHandle dcErase(dc);
        dcErase.FillRect(CRect(0, 0, m_bVertical ? 12 : m_nSpanLength, m_bVertical ? m_nSpanLength : 12), m_bshArrowBrush);
        return TRUE;
    }
};
