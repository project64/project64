#include "stdafx.h"
#include "HexEditCtrl.h"
#include <unordered_map> 

CHexEditCtrl::CHexEditCtrl(void) :
    m_BaseAddress(0x80000000),
    m_DrawnBaseAddress(0xFFFFFFFF),
    m_SelStartAddress(0),
    m_SelEndAddress(0),
    m_SelStartCellSide(HX_LEFT),
    m_SelEndCellSide(HX_LEFT),
    m_bInsertMode(false),
    m_CaretAddress(0),
    m_bCaretLoNibble(false),
    m_bCaretVisible(false),
    m_bHaveCaret(false),
    m_bShowHotAddress(false),
    m_HotAddress(0),
    m_Font(NULL),
    m_BackBMP(NULL),
    m_BackDC(NULL),
    m_CharWidth(0),
    m_CharHeight(0),
    m_FocusedColumn(HX_COL_NONE),
    m_hCursorIBeam(NULL),
    m_hCursorDefault(NULL),
    m_DragScrollDelta(0),
    m_AddressColumnRect({0}),
    m_HexDataColumnRect({0}),
    m_AsciiColumnRect({0}),
    m_bDblClicked(false),
    m_bLButtonDown(false),
    m_bMouseDragging(false),
    m_bLayoutChanged(false),
    m_OldBytes(NULL),
    m_NewBytes(NULL),
    m_NumBytesPerGroup(4),
    m_NumByteGroupsPerRow(0),
    m_NumVisibleRows(0),
    m_NumVisibleBytes(0),
    m_RealSelStartAddress(0),
    m_RealSelEndAddress(0),
    m_bHaveRealSel(false)
{
    WNDCLASS wc;
    if (!GetClassInfo(GetModuleHandle(NULL), _T("HexEditCtrl"), &wc))
    {
        GetWndClassInfo().m_wc.lpfnWndProc = m_pfnSuperWindowProc;
        GetWndClassInfo().Register(&m_pfnSuperWindowProc);
    }
}

CHexEditCtrl::~CHexEditCtrl(void)
{
}

int CALLBACK CHexEditCtrl::HaveFontCb(CONST LOGFONTW* lplf, CONST TEXTMETRICW* /*lptm*/, DWORD /*FontType*/, LPARAM lParam)
{
    const wchar_t * name = (const wchar_t*)lParam;
    if (wcscmp(lplf->lfFaceName, name) == 0)
    {
        return 0;
    }
    return 1;
}

bool CHexEditCtrl::HaveFont(HDC hdc, const char* name)
{
    if (EnumFonts(hdc, stdstr(name).ToUTF16().c_str(), HaveFontCb, (LPARAM)stdstr(name).ToUTF16().c_str()) == 0)
    {
        return true;
    }
    return false;
}

BOOL CHexEditCtrl::Attach(HWND hWnd)
{
    if (m_hWnd != NULL)
    {
        return FALSE;
    }

    if (!CWindowImpl<CHexEditCtrl>::SubclassWindow(hWnd))
    {
        return FALSE;
    }

    CRect wndRc;
    if (!GetWindowRect(&wndRc))
    {
        return FALSE;
    }

    HDC hdc = GetDC();
    HBITMAP hOldBMP;
    HFONT   hOldFont;

    m_BackDC = CreateCompatibleDC(hdc);
    m_BackBMP = CreateCompatibleBitmap(hdc, wndRc.Width(), wndRc.Height());
    hOldBMP = (HBITMAP)SelectObject(m_BackDC, m_BackBMP);
    DeleteObject(hOldBMP);

    m_hCursorIBeam = LoadCursor(NULL, IDC_IBEAM);
    m_hCursorDefault = LoadCursor(NULL, IDC_ARROW);

    float dpiScale = ::GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;

    if (HaveFont(hdc, "Consolas"))
    {
        m_Font = CreateFont((int)(14 * dpiScale), 0, 0, 0,
            FW_DONTCARE,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            FF_DONTCARE | FIXED_PITCH,
            L"Consolas");
    }
    else
    {
        m_Font = (HFONT)GetStockObject(ANSI_FIXED_FONT);
    }

    hOldFont = (HFONT)SelectObject(m_BackDC, m_Font);
    DeleteObject(hOldFont);

    TEXTMETRIC tm;
    GetTextMetrics(m_BackDC, &tm);
    m_CharHeight = tm.tmHeight;
    m_CharWidth = tm.tmAveCharWidth;

    UpdateLayoutInfo();
    ReallocByteBuffers();

    CRect clrRc(0, 0, wndRc.Width(), wndRc.Height());
    HBRUSH hbrush = CreateSolidBrush(BKCOLOR_DEFAULT);
    FillRect(m_BackDC, clrRc, hbrush);
    DeleteObject(hbrush);

    SetTimer(TIMER_ID_AUTO_REFRESH, 20, NULL);
    SetTimer(TIMER_ID_DRAG_SCROLL, 50, NULL);
    
    ReleaseDC(hdc);

    return TRUE;
}

HWND CHexEditCtrl::Detach(void)
{
    if (m_hWnd == NULL)
    {
        return NULL;
    }

    KillTimer(TIMER_ID_AUTO_REFRESH);
    KillTimer(TIMER_ID_DRAG_SCROLL);

    if (m_BackBMP != NULL)
    {
        DeleteObject(m_BackBMP);
        m_BackBMP = NULL;
    }
    
    if (m_BackDC != NULL)
    {
        DeleteObject(m_BackDC);
        m_BackDC = NULL;
    }
    
    if (m_Font != NULL)
    {
        DeleteObject(m_Font);
        m_Font = NULL;
    }

    if (m_NewBytes != NULL)
    {
        free(m_NewBytes);
        m_NewBytes = NULL;
    }

    if (m_OldBytes != NULL)
    {
        free(m_OldBytes);
        m_OldBytes = NULL;
    }

    return CWindowImpl<CHexEditCtrl>::UnsubclassWindow();
}

void CHexEditCtrl::Draw(void)
{
    Notify(HXN_REDRAWSTARTED);
    
    int startCellIndex = 0;
    uint32_t startAddress = m_BaseAddress;
    int numBytesToUpdate = m_NumVisibleBytes;
    bool bIgnoreDiff = false;

    if (m_BaseAddress != m_DrawnBaseAddress)
    {
        m_bShowHotAddress = false;
        int64_t addrDelta = (int64_t)m_BaseAddress - (int64_t)m_DrawnBaseAddress;

        // scroll optimization
        if ((addrDelta % m_NumBytesPerRow) == 0 && abs(addrDelta) < (m_NumVisibleBytes - m_NumBytesPerRow))
        {
            int rowDelta = (int)(addrDelta / m_NumBytesPerRow);
            int numBytesScrolled = abs(rowDelta) * m_NumBytesPerRow;
            int numBytesToShift = (m_NumVisibleBytes - numBytesScrolled) - m_NumBytesPerRow;
            int shiftSrcIndex = 0, shiftDstIndex = 0;
            
            numBytesToUpdate = numBytesScrolled + m_NumBytesPerRow;

            CRect rcScrollArea;
            rcScrollArea.left = m_HexDataColumnRect.left;
            rcScrollArea.right = m_AsciiColumnRect.right;
            rcScrollArea.top = m_HexDataColumnRect.top;
            rcScrollArea.bottom = m_HexDataColumnRect.bottom;

            if (rowDelta > 0)
            {
                rcScrollArea.bottom -= m_CharHeight;
                startCellIndex = m_NumVisibleBytes - numBytesToUpdate;
                shiftSrcIndex = 0 + numBytesScrolled;
                shiftDstIndex = 0;
            }
            else if (rowDelta < 0)
            {
                rcScrollArea.top += m_CharHeight;
                startCellIndex = 0;
                shiftSrcIndex = 0 + m_NumBytesPerRow;
                shiftDstIndex = shiftSrcIndex + numBytesScrolled;
            }

            startAddress = m_BaseAddress + startCellIndex;

            memmove(&m_OldBytes[shiftDstIndex], &m_OldBytes[shiftSrcIndex], numBytesToShift * sizeof(HXBYTEINFO));
            memmove(&m_NewBytes[shiftDstIndex], &m_NewBytes[shiftSrcIndex], numBytesToShift * sizeof(HXBYTEINFO));

            ScrollDC(m_BackDC, 0, -rowDelta * m_CharHeight, &rcScrollArea, &rcScrollArea, NULL, NULL);
            InvalidateRect(&rcScrollArea, false);
        }

        DrawAddressColumn();
        DrawHeader();

        m_DrawnBaseAddress = m_BaseAddress;
        bIgnoreDiff = true;
    }

    if (m_bLayoutChanged)
    {
        bIgnoreDiff = true;
        m_bLayoutChanged = false;
    }

    int numOverflowBytes = 0;

    if (startAddress + numBytesToUpdate < startAddress)
    {
        numOverflowBytes = (startAddress + numBytesToUpdate);
    }

    for (int i = 0; i < numOverflowBytes; i++)
    {
        m_NewBytes[numBytesToUpdate - numOverflowBytes + i].bHidden = true;
    }

    NotifyGetByteInfo(startAddress, numBytesToUpdate - numOverflowBytes, bIgnoreDiff, &m_OldBytes[startCellIndex], &m_NewBytes[startCellIndex]);

    std::unordered_map<HXBYTEINFO, HXRECTPAIR> drawnByteRects;

    for (int i = 0; i < numBytesToUpdate; i++)
    {
        uint32_t address = startAddress + i;

        HXBYTEINFO* oldByte = &m_OldBytes[startCellIndex + i];
        HXBYTEINFO* newByte = &m_NewBytes[startCellIndex + i];
        
        if (IsSelected(address))
        {
            // override owner-provided colors if selected
            if (newByte->bkColor != BKCOLOR_DEFAULT)
            {
                // blend owner color with selection color if bkcolor isn't default
                newByte->bkColor = BlendColor(BKCOLOR_SEL_FOCUSED, newByte->bkColor);
                newByte->color = COLOR_SEL_FOCUSED;
            }
            else
            {
                newByte->bkColor = BKCOLOR_SEL_FOCUSED;
                newByte->color = COLOR_SEL_FOCUSED;
            }
        }

        if (address == m_HotAddress && m_bShowHotAddress && !m_bMouseDragging)
        {
            newByte->bkColor = BlendColor(BKCOLOR_HOT, newByte->bkColor);
        }

        // redraw cell if value or colors have changed
        if (*newByte != *oldByte)
        {
            CRect rcHex, rcAscii;
            GetHexCellPos(startCellIndex + i, &rcHex);
            GetAsciiCellPos(startCellIndex + i, &rcAscii);

            // check if a similar HXBYTEINFO has already been drawn
            std::unordered_map<HXBYTEINFO, HXRECTPAIR>::const_iterator drawnByte = drawnByteRects.find(*newByte);
            
            if (drawnByte != drawnByteRects.end())
            {
                HXRECTPAIR src = drawnByte->second;
                BitBlt(m_BackDC, rcHex.left, rcHex.top, src.rcHex.Width(), src.rcHex.Height(),
                    m_BackDC, src.rcHex.left, src.rcHex.top, SRCCOPY);
                BitBlt(m_BackDC, rcAscii.left, rcAscii.top, src.rcAscii.Width(), src.rcAscii.Height(),
                    m_BackDC, src.rcAscii.left, src.rcAscii.top, SRCCOPY);
                InvalidateRect(&rcHex, false);
                InvalidateRect(&rcAscii, false);
            }
            else if (newByte->bHidden)
            {
                HXRECTPAIR rectPair;
                Text(rcHex.left, rcHex.top, "  ", BKCOLOR_DEFAULT, BKCOLOR_DEFAULT, &rectPair.rcHex);
                Text(rcAscii.left, rcAscii.top, " ", BKCOLOR_DEFAULT, BKCOLOR_DEFAULT, &rectPair.rcAscii);
                drawnByteRects[*newByte] = rectPair;
            }
            else
            {
                COLORREF hexBkColor = newByte->bkColor;
                COLORREF hexColor = newByte->color;
                COLORREF asciiBkColor = newByte->bkColor;
                COLORREF asciiColor = newByte->color;

                if (IsSelected(address))
                {
                    if (m_FocusedColumn == HX_COL_ASCII)
                    {
                        hexBkColor = BKCOLOR_SEL_UNFOCUSED;
                        hexColor = COLOR_SEL_UNFOCUSED;
                    }
                    else
                    {
                        asciiBkColor = BKCOLOR_SEL_UNFOCUSED;
                        asciiColor = COLOR_SEL_UNFOCUSED;
                    }
                }

                HXRECTPAIR rectPair;

                if (newByte->bValid)
                {
                    char szHexByte[4], szAsciiByte[2];
                    sprintf(szHexByte, "%02X", newByte->value);
                    sprintf(szAsciiByte, "%c", ByteAscii(newByte->value));
                    Text(rcHex.left, rcHex.top, szHexByte, hexBkColor, hexColor, &rectPair.rcHex);
                    Text(rcAscii.left, rcAscii.top, szAsciiByte, asciiBkColor, asciiColor, &rectPair.rcAscii);
                }
                else
                {
                    Text(rcHex.left, rcHex.top, "**", hexBkColor, hexColor, &rectPair.rcHex);
                    Text(rcAscii.left, rcAscii.top, ".", asciiBkColor, asciiColor, &rectPair.rcAscii);
                }

                drawnByteRects[*newByte] = rectPair;
            }
        }

        *oldByte = *newByte;
    }

    UpdateCaretUI(false);
}

void CHexEditCtrl::HitTest(int x, int y, HXHITTEST* pht)
{
    memset(pht, 0, sizeof(HXHITTEST));
    pht->column = HX_COL_NONE;

    CPoint pt(x, y);
    
    if (PtInRect(&m_AddressColumnRect, pt))
    {
        pht->column = HX_COL_ADDRESS;
    }

    int headerHeight = m_CharHeight;

    // clamp row
    int row = (y - headerHeight) / m_CharHeight;
    row = max(0, row);
    row = min(m_NumVisibleRows - 1, row);

    uint32_t rowAddress = SatAdd32(m_BaseAddress, row * m_NumBytesPerRow);
    
    if (x >= m_HexDataColumnRect.left && x < m_HexDataColumnRect.right)
    {
        if (PtInRect(&m_HexDataColumnRect, pt))
        {
            pht->column = HX_COL_HEXDATA;
        }

        int groupWidth = (m_NumBytesPerGroup * m_CharWidth * 2) + (m_CharWidth * 1);
        int nGroup = (x - m_HexDataColumnRect.left) / groupWidth;
        int groupX = m_HexDataColumnRect.left + nGroup * groupWidth;
        int groupCharIdx = (x - groupX) / (m_CharWidth);
        uint32_t address = SatAdd32(rowAddress, nGroup * m_NumBytesPerGroup + groupCharIdx / 2);
        pht->hexAddress = address;
        pht->hexCellSide = (groupCharIdx & 1) ? HX_RIGHT : HX_LEFT; // todo fix for wrap
        pht->asciiAddress = address; // approximate
        pht->asciiCellSide = HX_LEFT;
    }
    else if (x >= m_AsciiColumnRect.left && x < m_AsciiColumnRect.right)
    {
        if (PtInRect(&m_AsciiColumnRect, pt))
        {
            pht->column = HX_COL_ASCII;
        }

        int asciiX = x - m_AsciiColumnRect.left;
        int idx = (asciiX / m_CharWidth);
        pht->asciiAddress = SatAdd32(rowAddress, idx);
        pht->asciiCellSide = ((asciiX % m_CharWidth) > (m_CharWidth / 2)) ? HX_RIGHT : HX_LEFT;
        pht->hexAddress = SatAdd32(rowAddress, (m_NumBytesPerRow - 1)); // approximate
        pht->hexCellSide = HX_RIGHT;
    }
    else if (x < m_HexDataColumnRect.left)
    {
        // approximate
        pht->hexAddress = rowAddress;
        pht->hexCellSide = HX_LEFT;
        pht->asciiAddress = rowAddress;
        pht->asciiCellSide = HX_LEFT;
    }
    else if (x >= m_AsciiColumnRect.right)
    {
        // approximate
        pht->hexAddress = SatAdd32(rowAddress, (m_NumBytesPerRow - 1));
        pht->hexCellSide = HX_RIGHT;
        pht->asciiAddress = SatAdd32(rowAddress, (m_NumBytesPerRow - 1));
        pht->asciiCellSide = HX_RIGHT;
    }
}

bool CHexEditCtrl::UpdateCaretUI(bool bEnsureVisible, bool bTop)
{
    if (bEnsureVisible)
    {
        EnsureCaretAddressVisible(bTop);
    }

    if (!m_bHaveCaret)
    {
        return false;
    }

    if (!IsCaretAddressVisible())
    {
        HideCaret();
        return false;
    }

    ShowCaret();
    int index = m_CaretAddress - m_BaseAddress;
    CRect rcCell;

    int xoffs = 0;

    if (m_FocusedColumn == HX_COL_ASCII)
    {
        if ((int)((m_RealSelEndAddress - m_BaseAddress) % m_NumBytesPerRow) == m_NumBytesPerRow - 1)
        {
            // left-to-right selection ends on the end of a row
            index--;
            xoffs = m_CharWidth;
        }

        GetAsciiCellPos(index, &rcCell);
        SetCaretPos(rcCell.left + xoffs, rcCell.top);
    }
    else
    {
        if (GetSelDirection() > 0)
        {
            if ((int)((m_RealSelEndAddress - m_BaseAddress) % m_NumBytesPerRow) == m_NumBytesPerRow - 1)
            {
                // left-to-right selection ends on the end of a row
                index--;
                xoffs = m_CharWidth * 2;
            }
            else if ((int)((m_RealSelEndAddress - m_BaseAddress) % m_NumBytesPerGroup) == m_NumBytesPerGroup - 1)
            {
                // left-to-right selection ends on the end of a group
                xoffs = -m_CharWidth;
            }
        }

        GetHexCellPos(index, &rcCell);
        SetCaretPos(rcCell.left + (m_bCaretLoNibble ? m_CharWidth : 0) + xoffs, rcCell.top);
    }

    return true;
}

void CHexEditCtrl::Text(int x, int y, const char *text, COLORREF bg, COLORREF fg, CRect *rcOut)
{
    std::wstring textOuput = stdstr(text).ToUTF16(CP_ACP);
    size_t length = textOuput.length();
    int calcWidth = length * m_CharWidth;

    CRect rc(x, y, 0, 0);
    COLORREF orgBg = ::SetBkColor(m_BackDC, bg);
    COLORREF orgFg = ::SetTextColor(m_BackDC, fg);
    ::DrawText(m_BackDC, textOuput.c_str(), -1, &rc, DT_TOP | DT_NOPREFIX | DT_CALCRECT);
    rc.right = rc.left + calcWidth; // just in case
    ::DrawText(m_BackDC, textOuput.c_str(), -1, &rc, DT_TOP | DT_NOPREFIX);
    InvalidateRect(&rc, false);
    ::SetBkColor(m_BackDC, orgBg);
    ::SetBkColor(m_BackDC, orgFg);

    *rcOut = rc;
}

void CHexEditCtrl::UpdateRealSelection(void)
{
    uint32_t start = m_SelStartAddress;
    uint32_t end = m_SelEndAddress;
    bool bHaveSel = true;

    if (start < end)
    {
        if (m_SelEndCellSide == HX_LEFT) end--;
        if (m_SelStartCellSide == HX_RIGHT) start++;
    }
    else if (end < start)
    {
        if (start - end == 1)
        {
            if (m_SelStartCellSide == HX_LEFT && m_SelEndCellSide == HX_RIGHT)
            {
                bHaveSel = false;
            }
        }

        if (m_SelStartCellSide == HX_LEFT) start--;
        if (m_SelEndCellSide == HX_RIGHT) end++;

        swap(start, end);
    }
    else if(start == end)
    {
        if (m_SelStartCellSide == m_SelEndCellSide)
        {
            bHaveSel = false;
        }
    }

    if (m_RealSelStartAddress != start ||
        m_RealSelEndAddress != end ||
        m_bHaveRealSel != bHaveSel)
    {
        m_bHaveRealSel = bHaveSel;
        m_RealSelStartAddress = start;
        m_RealSelEndAddress = end;
        Notify(HXN_SELCHANGED);
    }
}

bool CHexEditCtrl::IsSelected(uint32_t address)
{
    return m_bHaveRealSel && (address >= m_RealSelStartAddress && address <= m_RealSelEndAddress);
}

void CHexEditCtrl::DrawAddressColumn()
{
    int headerHeight = m_CharHeight;
    for (int nRow = 0; nRow < m_NumVisibleRows; nRow++)
    {
        CRect rcAddress;
        uint32_t rowAddress = m_BaseAddress + (nRow * m_NumBytesPerRow);
        int y = headerHeight + nRow * m_CharHeight;

        if (rowAddress >= m_BaseAddress)
        {
            Text(0, y, stdstr_f(" %08X ", rowAddress).c_str(), BKCOLOR_ADDR, COLOR_ADDR, &rcAddress);
        }
        else
        {
            // wrapped
            Text(0, y, "          ", BKCOLOR_ADDR, COLOR_ADDR, &rcAddress);
        }
    }
}

void CHexEditCtrl::DrawHeader()
{
    CRect rcClient;
    GetClientRect(&rcClient);
    CRect rcHeader = { 0, 0, rcClient.Width(), m_CharHeight };
    HBRUSH br = CreateSolidBrush(BKCOLOR_ADDR);
    FillRect(m_BackDC, &rcHeader, br);
    DeleteObject(br);

    int groupWidth = m_NumBytesPerGroup * m_CharWidth * 2 + m_CharWidth;

    for (int nGroup = 0; nGroup < m_NumByteGroupsPerRow; nGroup++)
    {
        int groupX = m_HexDataColumnRect.left + nGroup * groupWidth;
        int offs = nGroup * m_NumBytesPerGroup;
        CRect dummy;
        Text(groupX, 0, stdstr_f("%02X", offs).c_str(), BKCOLOR_ADDR, COLOR_ADDR, &dummy);
    }

    InvalidateRect(&rcHeader, false);
}

void CHexEditCtrl::GetHexCellPos(int index, CRect* rc)
{
    int nRow = index / m_NumBytesPerRow;
    int rowOffs = (index % m_NumBytesPerRow);
    int nGroup = rowOffs / m_NumBytesPerGroup;
    int byteOffs = rowOffs % m_NumBytesPerGroup;

    int addrColumnWidth = (m_CharWidth * 11);
    int byteWidth = (m_CharWidth * 2);
    int hexGroupWidth = (byteWidth * m_NumBytesPerGroup) + (m_CharWidth * 1);
    
    int headerHeight = m_CharHeight;

    rc->left = addrColumnWidth + (nGroup * hexGroupWidth) + (byteOffs * byteWidth);
    rc->top = headerHeight + nRow * m_CharHeight;
    rc->right = rc->left + m_CharWidth * 2;
    rc->bottom = rc->top + m_CharHeight;
}

void CHexEditCtrl::GetAsciiCellPos(int index, CRect* rc)
{
    int nRow = index / m_NumBytesPerRow;
    int rowOffs = (index % m_NumBytesPerRow);

    int addrColumnWidth = (m_CharWidth * 11);
    int byteWidth = (m_CharWidth * 2);
    int hexGroupWidth = (byteWidth * m_NumBytesPerGroup) + (m_CharWidth * 1);
    int hexColumnWidth = (m_NumByteGroupsPerRow * hexGroupWidth);
    int asciiColumnX = 0 + addrColumnWidth + hexColumnWidth;
    int headerHeight = m_CharHeight;

    rc->left = asciiColumnX + (rowOffs * m_CharWidth);
    rc->top = headerHeight + nRow * m_CharHeight;
    rc->right = rc->left + m_CharWidth;
    rc->bottom = rc->top + m_CharHeight;
}

char CHexEditCtrl::ByteAscii(uint8_t value)
{
    // ISO 8859-1
    if ((value >= 0x20 && value <= 0x7E) || value >= 0xA1)
    {
        return (char)value;
    }

    return '.';
}

uint8_t CHexEditCtrl::HexCharValue(char c)
{
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'A' && c <= 'F') return (c - 'A') + 0x0A;
    if (c >= 'a' && c <= 'f') return (c - 'a') + 0x0A;

    return 0;
}

void CHexEditCtrl::CaretIncrementNibble(void)
{
    if (!m_bCaretLoNibble)
    {
        m_bCaretLoNibble = true;
    }
    else
    {
        m_bCaretLoNibble = false;
        m_CaretAddress++;
    }
}

void CHexEditCtrl::CaretDecrementNibble(void)
{
    if (m_bCaretLoNibble)
    {
        m_bCaretLoNibble = false;
    }
    else
    {
        m_bCaretLoNibble = true;
        m_CaretAddress--;
    }
}

void CHexEditCtrl::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_ID_AUTO_REFRESH)
    {
        Draw();
    }
    else if (nIDEvent == TIMER_ID_DRAG_SCROLL)
    {
        if (m_DragScrollDelta != 0)
        {
            int numBytesToScroll = m_DragScrollDelta * m_NumBytesPerRow;
            int64_t newCaretAddress = (int64_t)m_CaretAddress + numBytesToScroll;
            
            if (newCaretAddress < 0 && m_BaseAddress == 0)
            {
                return;
            }

            else if (newCaretAddress > UINT_MAX)
            {
                return;
            }

            m_CaretAddress = SatAdd32(m_CaretAddress, numBytesToScroll);
            m_SelEndAddress = SatAdd32(m_SelEndAddress, numBytesToScroll);

            UpdateRealSelection();
            UpdateCaretUI(true);
        }
    }
}

void CHexEditCtrl::OnPaint(CDCHandle dc)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);
    CRect rc = ps.rcPaint;

    BitBlt(hdc,
        rc.left, rc.top,
        rc.Width(), rc.Height(),
        m_BackDC,
        rc.left, rc.top,
        SRCCOPY);

    EndPaint(&ps);
}

void CHexEditCtrl::OnRButtonDown(UINT /*nFlags*/, CPoint point)
{
    SetFocus();

    HXHITTEST ht;
    HitTest(point.x, point.y, &ht);

    if (ht.column == HX_COL_HEXDATA)
    {
        if (!IsSelected(ht.hexAddress))
        {
            m_CaretAddress = ht.hexAddress;
            m_bCaretLoNibble = HX_LEFT;
            CancelSelection();
        }
    }
    else if (ht.column == HX_COL_ASCII)
    {
        if (!IsSelected(ht.asciiAddress))
        {
            m_CaretAddress = ht.asciiAddress;
            CancelSelection();
        }
    }
}

void CHexEditCtrl::OnRButtonUp(UINT /*nFlags*/, CPoint point)
{
    HXHITTEST ht;
    HitTest(point.x, point.y, &ht);

    if (ht.column == HX_COL_HEXDATA)
    {
        NotifyRightClick(ht.hexAddress);
    }
    else if (ht.column == HX_COL_ASCII)
    {
        NotifyRightClick(ht.asciiAddress);
    }
}

void CHexEditCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_bLButtonDown = true;

    SetFocus();

    HXHITTEST ht;
    HitTest(point.x, point.y, &ht);

    m_FocusedColumn = ht.column;
   
    if (m_FocusedColumn == HX_COL_HEXDATA)
    {
        m_CaretAddress = ht.hexAddress;
        m_bCaretLoNibble = (ht.hexCellSide == HX_RIGHT);

        if (nFlags & MK_SHIFT)
        {
            m_SelEndAddress = ht.hexAddress;
            m_SelEndCellSide = ht.hexCellSide;
            UpdateRealSelection();
            
            if (GetSelDirection() > 0)
            {
                m_CaretAddress = m_RealSelEndAddress + 1;
            }
            else
            {
                m_CaretAddress = m_RealSelStartAddress;
            }
            m_bCaretLoNibble = false;
        }
        else
        {
            m_SelStartAddress = ht.hexAddress;
            m_SelEndAddress = ht.hexAddress;
            m_SelStartCellSide = ht.hexCellSide;
            m_SelEndCellSide = ht.hexCellSide;
            UpdateRealSelection();
        }
    }
    else if (m_FocusedColumn == HX_COL_ASCII)
    {
        m_CaretAddress = ht.asciiAddress;

        if (nFlags & MK_SHIFT)
        {
            m_SelEndAddress = ht.asciiAddress;
        }
        else
        {
            m_CaretAddress = ht.asciiAddress;
            m_SelStartCellSide = ht.asciiCellSide;
            m_SelEndCellSide = ht.asciiCellSide;
            m_SelStartAddress = ht.asciiAddress;
            m_SelEndAddress = ht.asciiAddress;
            if (ht.asciiCellSide)
            {
                m_CaretAddress++;
            }
        }

        UpdateRealSelection();
    }

    UpdateCaretUI(false);
    Draw();

    SetCapture();
}

void CHexEditCtrl::OnLButtonDblClk(UINT /*nFlags*/, CPoint point)
{
    m_bDblClicked = true;

    HXHITTEST ht;
    HitTest(point.x, point.y, &ht);

    if (m_FocusedColumn == HX_COL_HEXDATA)
    {
        // select word
        uint32_t offset = (ht.hexAddress - m_BaseAddress);
        uint32_t wordOffset = offset - (offset % m_NumBytesPerGroup);
        uint32_t wordAddress = m_BaseAddress + wordOffset;
        m_SelStartAddress = wordAddress;
        m_SelEndAddress = wordAddress + (m_NumBytesPerGroup - 1);
        m_SelStartCellSide = HX_LEFT;
        m_SelEndCellSide = HX_RIGHT;
        m_CaretAddress = m_SelEndAddress + 1;
        m_bCaretLoNibble = false;
        UpdateRealSelection();
        UpdateCaretUI(false);
    }
    if (m_FocusedColumn == HX_COL_ASCII)
    {
        // select row
        uint32_t offset = (ht.asciiAddress - m_BaseAddress);
        uint32_t rowOffset = (ht.asciiAddress - m_BaseAddress) - (offset % m_NumBytesPerRow);
        uint32_t rowAddress = m_BaseAddress + rowOffset;
        m_SelStartAddress = rowAddress;
        m_SelEndAddress = rowAddress + (m_NumBytesPerRow - 1);
        m_SelStartCellSide = HX_LEFT;
        m_SelEndCellSide = HX_RIGHT;
        m_CaretAddress = m_SelEndAddress + 1;
        UpdateRealSelection();
        UpdateCaretUI(false);
    }
}

void CHexEditCtrl::OnLButtonUp(UINT /*nFlags*/, CPoint point)
{
    m_bLButtonDown = false;
    m_bMouseDragging = false;

    if (m_DragScrollDelta != 0)
    {
        m_bDblClicked = false;
        m_DragScrollDelta = 0;
        ReleaseCapture();
        return;
    }

    HXHITTEST ht;
    HitTest(point.x, point.y, &ht);

    if (m_bDblClicked)
    {
        m_bDblClicked = false;
        return;
    }

    ReleaseCapture();
}

void CHexEditCtrl::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
    if (m_bLButtonDown)
    {
        m_bMouseDragging = true;
    }

    HXHITTEST ht;
    HitTest(point.x, point.y, &ht);

    if (ht.column == HX_COL_NONE || ht.column == HX_COL_ADDRESS)
    {
        m_bShowHotAddress = false;
    }
    else
    {
        m_bShowHotAddress = true;

        if (ht.column == HX_COL_HEXDATA)
        {
            if (m_HotAddress != ht.hexAddress)
            {
                m_HotAddress = ht.hexAddress;
                Notify(HXN_HOTADDRCHANGED);
            }
        }
        else if (ht.column == HX_COL_ASCII)
        {
            if (m_HotAddress != ht.asciiAddress)
            {
                m_HotAddress = ht.asciiAddress;
                Notify(HXN_HOTADDRCHANGED);
            }
        }
    }

    if (!m_bLButtonDown)
    {
        return;
    }

    m_DragScrollDelta = 0;

    if (point.y > m_HexDataColumnRect.bottom)
    {
        m_DragScrollDelta = 1 + (point.y - m_HexDataColumnRect.bottom) / m_CharHeight;
    }
    else if (point.y < m_HexDataColumnRect.top)
    {
        m_DragScrollDelta = -1 + (point.y - m_HexDataColumnRect.top) / m_CharHeight;
    }

    if (m_FocusedColumn == HX_COL_HEXDATA)
    {
        m_CaretAddress = ht.hexAddress;
        m_SelEndAddress = ht.hexAddress;
        m_SelEndCellSide = ht.hexCellSide;
        m_bCaretLoNibble = (ht.hexCellSide == HX_RIGHT);

        if (m_SelEndAddress - m_SelStartAddress == 1 &&
            m_SelStartCellSide == HX_RIGHT &&
            m_SelEndCellSide == HX_LEFT)
        {
            m_SelStartCellSide = HX_LEFT;
        }

        if (GetSelDirection() != 0 && m_SelEndCellSide == HX_RIGHT)
        {
            m_bCaretLoNibble = false;
            m_CaretAddress++;
        }
    }
    else if (m_FocusedColumn == HX_COL_ASCII)
    {
        m_CaretAddress = ht.asciiAddress;
        m_SelEndAddress = ht.asciiAddress;
        m_SelEndCellSide = ht.asciiCellSide;

        if (GetSelDirection() != 0 && m_SelEndCellSide == HX_RIGHT)
        {
            m_CaretAddress++;
        }
    }

    UpdateRealSelection();
}

BOOL CHexEditCtrl::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
    m_BaseAddress = SatAdd32(m_BaseAddress, -(zDelta / 120) * m_NumBytesPerRow);
    Notify(HXN_BASEADDRCHANGED);
    return FALSE;
}

void CHexEditCtrl::OnSetFocus(CWindow /*wndOld*/)
{
    ::CreateCaret(m_hWnd, NULL, 2, m_CharHeight);
    m_bHaveCaret = true;
    UpdateCaretUI(false);
}

void CHexEditCtrl::OnKillFocus(CWindow /*wndFocus*/)
{
    m_bCaretVisible = false;
    m_bHaveCaret = false;
    ::DestroyCaret();
}

UINT CHexEditCtrl::OnGetDlgCode(LPMSG /*lpMsg*/)
{
    return DLGC_WANTALLKEYS;
}

void CHexEditCtrl::OnChar(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
    if (::GetKeyState(VK_CONTROL) & 0x8000)
    {
        return;
    }

    if (nChar == VK_BACK || nChar == VK_TAB || nChar == VK_RETURN)
    {
        return;
    }

    if (m_FocusedColumn == HX_COL_HEXDATA)
    {
        if (isxdigit(nChar))
        {
            int selDirection = GetSelDirection();

            if (selDirection < 0)
            {
                m_CaretAddress = m_SelEndAddress;
                m_bCaretLoNibble = false;
            }
            else if (selDirection > 0)
            {
                m_CaretAddress = m_SelStartAddress;
                m_bCaretLoNibble = false;
            }

            NotifySetNibble(m_CaretAddress, m_bCaretLoNibble, HexCharValue((char)nChar));

            CancelSelection();
            CaretIncrementNibble();
            UpdateCaretUI(true);
        }
    }
    else if (m_FocusedColumn == HX_COL_ASCII)
    {
        int selDirection = GetSelDirection();
        if (selDirection < 0)
        {
            m_CaretAddress = m_SelEndAddress;
        }
        else if(selDirection > 0)
        {
            m_CaretAddress = m_SelStartAddress;
        }

        NotifySetByte(m_CaretAddress, (uint8_t)nChar);

        CancelSelection();
        m_CaretAddress++;
        UpdateCaretUI(true);
    }
}

void CHexEditCtrl::Paste(bool bAdvanceCaret)
{
    uint32_t targetAddress = m_bHaveRealSel ? m_RealSelStartAddress : m_CaretAddress;
    int retLength = NotifyPaste(targetAddress);

    if (retLength != 0)
    {
        if (bAdvanceCaret)
        {
            m_CaretAddress = targetAddress + retLength;
            UpdateCaretUI(true);
        }
        CancelSelection();
    }
}

void CHexEditCtrl::Copy(void)
{
    if (m_bHaveRealSel)
    {
        Notify(HXN_COPY);
    }
}

void CHexEditCtrl::SetBaseAddress(uint32_t address)
{
    if (m_BaseAddress != address)
    {
        m_BaseAddress = address;
        Draw();
    }
}

uint32_t CHexEditCtrl::GetCaretAddress(void)
{
    return m_CaretAddress;
}

uint32_t CHexEditCtrl::GetHotAddress(void)
{
    return m_HotAddress;
}

uint32_t CHexEditCtrl::GetBaseAddress(void)
{
    return m_BaseAddress;
}

int CHexEditCtrl::GetNumBytesPerRow(void)
{
    return m_NumBytesPerRow;
}

int CHexEditCtrl::GetNumVisibleBytes(void)
{
    return m_NumVisibleBytes;
}

int CHexEditCtrl::GetNumBytesPerGroup(void)
{
    return m_NumBytesPerGroup;
}

bool CHexEditCtrl::GetSelectionRange(uint32_t* startAddress, uint32_t* endAddress)
{
    *startAddress = m_RealSelStartAddress;
    *endAddress = m_RealSelEndAddress;
    return m_bHaveRealSel;
}

bool CHexEditCtrl::GetInsertMode(void)
{
    return m_bInsertMode;
}

HXCOLUMN CHexEditCtrl::GetFocusedColumn(void)
{
    return m_FocusedColumn;
}

void CHexEditCtrl::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
    if (nChar != VK_CONTROL && (GetKeyState(VK_CONTROL) & 0x8000))
    {
        NotifyCtrlKeyPressed(nChar);

        if (nChar == 'V')
        {
            Paste();
        }
        else if (nChar == 'B')
        {
            Paste(false);
        }
        else if (nChar == 'C')
        {
            Copy();
        }
        else if (nChar == 'X')
        {
            Copy();
            if (m_bHaveRealSel)
            {
                NotifyFillRange(m_RealSelStartAddress, m_RealSelEndAddress, 0);
                m_CaretAddress = m_RealSelStartAddress;
                m_bCaretLoNibble = false;
                CancelSelection();
            }
        }
        else if (nChar == 'A')
        {
            SelectAllVisible();
        }
    }


    if (nChar == VK_DOWN)
    {
        m_CaretAddress = SatAdd32(m_CaretAddress, m_NumBytesPerRow);

        if (GetKeyState(VK_SHIFT) & 0x8000)
        {
            m_SelEndAddress += m_NumBytesPerRow;
            if (m_bCaretLoNibble)
            {
                m_SelStartCellSide = HX_LEFT;
                m_SelEndCellSide = HX_LEFT;
                m_bCaretLoNibble = false;
            }
            UpdateRealSelection();
        }
        else
        {
            CancelSelection();
        }
        
        UpdateCaretUI(true);
    }
    else if (nChar == VK_UP)
    {
        m_CaretAddress -= m_NumBytesPerRow;

        if (GetKeyState(VK_SHIFT) & 0x8000)
        {
            m_SelEndAddress -= m_NumBytesPerRow;
            if (m_bCaretLoNibble)
            {
                m_SelStartCellSide = HX_LEFT;
                m_SelEndCellSide = HX_LEFT;
                m_bCaretLoNibble = false;
            }
            UpdateRealSelection();
        }
        else
        {
            CancelSelection();
        }
        
        UpdateCaretUI(true);
    }
    else if (nChar == VK_RIGHT)
    {
        if (m_FocusedColumn == HX_COL_HEXDATA)
        {
            if (GetKeyState(VK_SHIFT) & 0x8000)
            {
                if (m_SelStartAddress == m_SelEndAddress &&
                    m_SelStartCellSide == HX_RIGHT &&
                    m_SelEndCellSide == HX_RIGHT)
                {
                    m_SelStartCellSide = HX_LEFT;
                    m_SelEndCellSide = HX_RIGHT;
                }
                else
                {
                    m_SelEndAddress++;
                }

                m_bCaretLoNibble = false;
                m_CaretAddress++;
                UpdateRealSelection();
            }
            else if (GetKeyState(VK_CONTROL) & 0x8000)
            {
                CaretIncrementNibble();
                CancelSelection();
            }
            else
            {
                m_bCaretLoNibble = false;
                m_CaretAddress++;
                m_SelStartAddress = m_CaretAddress;
                m_SelEndAddress = m_CaretAddress;
                m_SelStartCellSide = HX_LEFT;
                m_SelEndCellSide = HX_LEFT;
                CancelSelection();
            }
        }
        else if (m_FocusedColumn == HX_COL_ASCII)
        {
            m_CaretAddress++;

            if (GetKeyState(VK_SHIFT))
            {
                m_SelEndCellSide = HX_LEFT;
                m_SelEndAddress = m_CaretAddress;
                UpdateRealSelection();
            }
            else
            {
                CancelSelection();
            }
        }

        UpdateCaretUI(true);
    }
    else if (nChar == VK_LEFT)
    {
        if (m_FocusedColumn == HX_COL_HEXDATA)
        {
            if(GetKeyState(VK_SHIFT) & 0x8000)
            {
                m_SelEndCellSide = HX_LEFT;
                m_SelEndAddress--;
                if (m_bCaretLoNibble)
                {
                    m_SelStartCellSide = HX_LEFT;
                }
                m_CaretAddress--;
                m_bCaretLoNibble = false;
                UpdateRealSelection();
            }
            else if (GetKeyState(VK_CONTROL) & 0x8000)
            {
                CaretDecrementNibble();
                CancelSelection();
            }
            else
            {
                if (m_bCaretLoNibble)
                {
                    CaretDecrementNibble();
                }
                else
                {
                    m_CaretAddress--;
                }

                CancelSelection();
            }
        }
        else if (m_FocusedColumn == HX_COL_ASCII)
        {
            m_CaretAddress--;

            if (GetKeyState(VK_SHIFT))
            {
                m_SelEndCellSide = HX_LEFT;
                m_SelEndAddress = m_CaretAddress;
                UpdateRealSelection();
            }
            else
            {
                CancelSelection();
            }
        }

        UpdateCaretUI(true);
    }
    else if (nChar == VK_NEXT || nChar == VK_PRIOR) // page down, page up
    {
        int delta = (nChar == VK_NEXT) ? m_NumVisibleBytes : -m_NumVisibleBytes;

        if (IsCaretAddressVisible())
        {
            m_BaseAddress += delta;
            m_CaretAddress += delta;
            Notify(HXN_BASEADDRCHANGED);
        }
        else
        {
            m_CaretAddress += delta;
            UpdateCaretUI(true, true);
        }

        CancelSelection();
    }
    else if (nChar == VK_INSERT)
    {
        m_bInsertMode = !m_bInsertMode;
        Notify(HXN_INSERTMODECHANGED);
    }
    else if (nChar == VK_RETURN)
    {
        Notify(HXN_ENTERPRESSED);
    }
    else if (nChar == VK_HOME)
    {
        UpdateCaretUI(true);
    }
    else if (nChar == VK_BACK)
    {
        if (m_bHaveRealSel)
        {
            NotifyFillRange(m_RealSelStartAddress, m_RealSelEndAddress, 0);
            CancelSelection();
            m_CaretAddress = m_RealSelStartAddress;
            UpdateCaretUI(true);
        }
        else
        {
            if (m_FocusedColumn == HX_COL_HEXDATA)
            {
                uint32_t address = m_CaretAddress + (m_bCaretLoNibble ? 0 : -1);
                NotifySetNibble(address, !m_bCaretLoNibble, 0);
                CaretDecrementNibble();
                UpdateCaretUI(true);
            }
            else if (m_FocusedColumn == HX_COL_ASCII)
            {
                NotifySetByte(m_CaretAddress - 1, 0);
                m_CaretAddress--;
                UpdateCaretUI(true);
            }
        }
    }
    else if (nChar == VK_DELETE)
    {
        if (m_FocusedColumn == HX_COL_HEXDATA || m_FocusedColumn == HX_COL_ASCII)
        {
            if (!m_bHaveRealSel)
            {
                NotifySetByte(m_CaretAddress, 0);
            }
            else
            {
                NotifyFillRange(m_RealSelStartAddress, m_RealSelEndAddress, 0);
            }
        }
    }
}

int CHexEditCtrl::GetSelDirection(void)
{
    if (m_SelStartAddress < m_SelEndAddress) return 1; // right
    if (m_SelStartAddress > m_SelEndAddress) return -1; // left
    if (m_SelStartCellSide == m_SelEndCellSide) return 0; // no selection
    if (m_SelStartCellSide == HX_LEFT && m_SelEndCellSide == HX_RIGHT) return 1; // right (single byte)
    if (m_SelStartCellSide == HX_RIGHT && m_SelEndCellSide == HX_LEFT) return -1; // left (single byte)
    return 0;
}

void CHexEditCtrl::CancelSelection(void)
{
    m_SelStartAddress = m_CaretAddress;
    m_SelEndAddress = m_CaretAddress;
    m_SelStartCellSide = m_bCaretLoNibble ? HX_RIGHT : HX_LEFT;
    m_SelEndCellSide = m_bCaretLoNibble ? HX_RIGHT : HX_LEFT;
    UpdateRealSelection();
}

void CHexEditCtrl::SelectAllVisible(void)
{
    uint32_t lastVisibleByteAddress = (m_BaseAddress + m_NumVisibleBytes) - 1;
    m_SelStartAddress = m_BaseAddress;
    m_SelStartCellSide = HX_LEFT;
    m_SelEndAddress = lastVisibleByteAddress;
    m_SelEndCellSide = HX_RIGHT;
    m_CaretAddress = lastVisibleByteAddress + 1;
    m_bCaretLoNibble = false;
    UpdateRealSelection();
}

bool CHexEditCtrl::IsCaretAddressVisible(void)
{
    return m_CaretAddress >= m_BaseAddress && m_CaretAddress <= SatAdd32(m_BaseAddress, m_NumVisibleBytes);
}

uint32_t CHexEditCtrl::LineAddress(uint32_t address)
{
    return address - ((address - (m_BaseAddress % m_NumBytesPerRow)) % m_NumBytesPerRow);
}

void CHexEditCtrl::EnsureCaretAddressVisible(bool bTop)
{
    uint32_t oldBaseAddress = m_BaseAddress;
    uint32_t caretLineAddress = LineAddress(m_CaretAddress);
    uint32_t lastVisibleLineAddress = m_BaseAddress + (m_NumVisibleBytes - m_NumBytesPerRow);

    if (bTop || caretLineAddress < m_BaseAddress)
    {
        m_BaseAddress = caretLineAddress;
    }
    else if (caretLineAddress >= lastVisibleLineAddress + m_NumBytesPerRow)
    {
        m_BaseAddress = SatAdd32(m_BaseAddress, caretLineAddress - lastVisibleLineAddress);
    }

    if (oldBaseAddress != m_BaseAddress)
    {
        Notify(HXN_BASEADDRCHANGED);
    }
}

LRESULT CHexEditCtrl::Notify(UINT code)
{
    UINT_PTR nID = ::GetDlgCtrlID(m_hWnd);
    NMHDR nmh = { m_hWnd, nID, code };
    return ::SendMessage(GetParent(), WM_NOTIFY, (WPARAM)nID, (LPARAM)&nmh);
}

LRESULT CHexEditCtrl::NotifySetByte(uint32_t address, uint8_t value)
{
    UINT_PTR nID = ::GetDlgCtrlID(m_hWnd);
    NMHXSETBYTE nmsb = { { m_hWnd, nID, HXN_SETBYTE }, m_bInsertMode, address, value };
    return ::SendMessage(GetParent(), WM_NOTIFY, (WPARAM)nID, (LPARAM)&nmsb);
}

LRESULT CHexEditCtrl::NotifySetNibble(uint32_t address, bool bLoNibble, uint8_t value)
{
    UINT_PTR nID = ::GetDlgCtrlID(m_hWnd);
    NMHXSETNIBBLE nmsn = { { m_hWnd, nID, HXN_SETNIBBLE }, m_bInsertMode, address, bLoNibble, value };
    return ::SendMessage(GetParent(), WM_NOTIFY, (WPARAM)nID, (LPARAM)&nmsn);
}

LRESULT CHexEditCtrl::NotifyFillRange(uint32_t startAddress, uint32_t endAddress, uint8_t value)
{
    UINT_PTR nID = ::GetDlgCtrlID(m_hWnd);
    NMHXFILLRANGE nmfr = { { m_hWnd, nID, HXN_FILLRANGE }, m_bInsertMode, startAddress, endAddress, value };
    return ::SendMessage(GetParent(), WM_NOTIFY, (WPARAM)nID, (LPARAM)&nmfr);
}

LRESULT CHexEditCtrl::NotifyCtrlKeyPressed(int nChar)
{
    UINT_PTR nID = ::GetDlgCtrlID(m_hWnd);
    NMHXCTRLKEYPRESSED nmck = { { m_hWnd, nID, HXN_CTRLKEYPRESSED }, nChar };
    return ::SendMessage(GetParent(), WM_NOTIFY, (WPARAM)nID, (LPARAM)&nmck);
}

LRESULT CHexEditCtrl::NotifyPaste(uint32_t address)
{
    UINT_PTR nID = ::GetDlgCtrlID(m_hWnd);
    NMHXPASTE nmp = { { m_hWnd, nID, HXN_PASTE }, address, m_FocusedColumn };
    return ::SendMessage(GetParent(), WM_NOTIFY, (WPARAM)nID, (LPARAM)&nmp);
}

LRESULT CHexEditCtrl::NotifyRightClick(uint32_t address)
{
    UINT_PTR nID = ::GetDlgCtrlID(m_hWnd);
    NMHXRCLICK nmrc = { { m_hWnd, nID, HXN_RCLICK }, address };
    return ::SendMessage(GetParent(), WM_NOTIFY, (WPARAM)nID, (LPARAM)&nmrc);
}

LRESULT CHexEditCtrl::NotifyGetByteInfo(uint32_t address, size_t numBytes, bool bIgnoreDiff, HXBYTEINFO* oldBytes, HXBYTEINFO* newBytes)
{
    UINT_PTR nID = ::GetDlgCtrlID(m_hWnd);
    NMHXGETBYTEINFO nmgbi = { { m_hWnd, nID, HXN_GETBYTEINFO }, address, numBytes, bIgnoreDiff, oldBytes, newBytes  };
    return ::SendMessage(GetParent(), WM_NOTIFY, nmgbi.nmh.idFrom, (LPARAM)&nmgbi);
}

BOOL CHexEditCtrl::OnSetCursor(CWindow /*wnd*/, UINT /*nHitTest*/, UINT /*message*/)
{
    CPoint point(::GetMessagePos());
    ScreenToClient(&point);

    HXHITTEST ht;
    HitTest(point.x, point.y, &ht);

    if (ht.column == HX_COL_HEXDATA || ht.column == HX_COL_ASCII)
    {
        SetCursor(m_hCursorIBeam);
    }
    else
    {
        SetCursor(m_hCursorDefault);
    }
    return FALSE;
}

void CHexEditCtrl::ShowCaret(void)
{
    if (!m_bCaretVisible)
    {
        ::ShowCaret(m_hWnd);
        m_bCaretVisible = true;
    }
}

void CHexEditCtrl::HideCaret(void)
{
    if (m_bCaretVisible)
    {
        ::HideCaret(m_hWnd);
        m_bCaretVisible = false;
    }
}

uint32_t CHexEditCtrl::SatAdd32(uint32_t a, int b)
{
    int64_t c = (int64_t)a + b;
    if (c > UINT_MAX)
    {
        return UINT_MAX;
    }
    if (c < 0)
    {
        return 0;
    }
    return (uint32_t)c;
}

uint32_t CHexEditCtrl::SatAdd32(uint32_t a, uint32_t b)
{
    uint32_t c = a + b;
    if (c < a)
    {
        return (uint32_t)-1;
    }
    return c;
}

COLORREF CHexEditCtrl::BlendColor(COLORREF c1, COLORREF c2)
{
    int r1 = GetRValue(c1);
    int g1 = GetGValue(c1);
    int b1 = GetBValue(c1);
    int r2 = GetRValue(c2);
    int g2 = GetGValue(c2);
    int b2 = GetBValue(c2);
    return RGB((r1+r2*2)/3, (g1+g2*2)/3, (b1+b2*2)/3);
}

void CHexEditCtrl::UpdateLayoutInfo(void)
{
    CRect clientRect;
    GetClientRect(&clientRect);

    int addressColumnWidth = 11 * m_CharWidth;
    int byteWidth = m_CharWidth * 2;
    int byteGroupWidth = (m_NumBytesPerGroup * byteWidth) + (m_CharWidth * 1);
    int asciiGroupWidth = (m_NumBytesPerGroup * m_CharWidth);
    int headerHeight = m_CharHeight;

    m_NumByteGroupsPerRow = (clientRect.Width() - addressColumnWidth) / (byteGroupWidth + asciiGroupWidth);
    m_NumBytesPerRow = m_NumByteGroupsPerRow * m_NumBytesPerGroup;
    m_NumVisibleRows = (clientRect.Height() - headerHeight) / m_CharHeight;
    m_NumVisibleBytes = m_NumVisibleRows * m_NumBytesPerRow;

    int hexDataColumnWidth = m_NumByteGroupsPerRow * byteGroupWidth;
    int asciiColumnWidth = m_NumBytesPerRow * m_CharWidth;

    int addressColumnLeft = 0;
    int addressColumnRight = addressColumnLeft + addressColumnWidth;
    int hexDataColumnLeft = addressColumnRight;
    int hexDataColumnRight = hexDataColumnLeft + hexDataColumnWidth;
    int asciiColumnLeft = hexDataColumnRight;
    int asciiColumnRight = asciiColumnLeft + asciiColumnWidth;

    int columnsTop = 0 + headerHeight;
    int columnsBottom = columnsTop + m_NumVisibleRows * m_CharHeight;

    m_AddressColumnRect = { addressColumnLeft, columnsTop, addressColumnRight, columnsBottom };
    m_HexDataColumnRect = { hexDataColumnLeft, columnsTop, hexDataColumnRight, columnsBottom };
    m_AsciiColumnRect = { asciiColumnLeft, columnsTop, asciiColumnRight, columnsBottom };

    m_bLayoutChanged = true;
}

void CHexEditCtrl::ReallocByteBuffers(void)
{
    m_NewBytes = (HXBYTEINFO*)realloc(m_NewBytes, m_NumVisibleBytes * sizeof(HXBYTEINFO));
    m_OldBytes = (HXBYTEINFO*)realloc(m_OldBytes, m_NumVisibleBytes * sizeof(HXBYTEINFO));

    for (int i = 0; i < m_NumVisibleBytes; i++)
    {
        m_NewBytes[i] = { 0 };
        m_OldBytes[i] = { 0 };
    }
}

void CHexEditCtrl::OnWindowPosChanged(LPWINDOWPOS /*lpWndPos*/)
{
    int oldNumRows = m_NumVisibleRows;
    int oldNumGroups = m_NumByteGroupsPerRow;

    UpdateLayoutInfo();

    if (oldNumRows != m_NumVisibleRows || oldNumGroups != m_NumByteGroupsPerRow)
    {
        ReallocByteBuffers();

        CRect rc;
        GetClientRect(&rc);
        m_BackBMP = CreateCompatibleBitmap(m_BackDC, rc.Width(), rc.Height());
        HBITMAP hOldBMP = (HBITMAP)SelectObject(m_BackDC, m_BackBMP);
        DeleteObject(hOldBMP);

        CRect clrRc(0, 0, rc.Width(), rc.Height());
        HBRUSH hbrush = CreateSolidBrush(BKCOLOR_DEFAULT);
        FillRect(m_BackDC, clrRc, hbrush);
        DeleteObject(hbrush);

        Draw();
        DrawAddressColumn();
        DrawHeader();
    }
}

void CHexEditCtrl::SetByteGroupSize(int nBytes)
{
    m_NumBytesPerGroup = nBytes;
    Notify(HXN_GROUPSIZECHANGED);

    UpdateLayoutInfo();
    ReallocByteBuffers();

    CRect rc;
    GetClientRect(&rc);
    CRect clrRc(0, 0, rc.Width(), rc.Height());
    HBRUSH hbrush = CreateSolidBrush(BKCOLOR_DEFAULT);
    FillRect(m_BackDC, clrRc, hbrush);
    DeleteObject(hbrush);

    Draw();
    DrawAddressColumn();
    DrawHeader();

    int addressColumnWidth = 11 * m_CharWidth;
    int headerHeight = m_CharHeight;
    CRect rcInv = { addressColumnWidth, headerHeight, rc.Width(), rc.Height() };
    InvalidateRect(&rcInv, true);
}
