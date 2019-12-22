#pragma once
#include <stdafx.h>

enum
{
    HXN_REDRAWSTARTED,
    HXN_GETBYTEINFO,
    HXN_SETBYTE,
    HXN_SETNIBBLE,
    HXN_FILLRANGE,
    HXN_RCLICK,
    HXN_INSERTMODECHANGED,
    HXN_HOTADDRCHANGED,
    HXN_BASEADDRCHANGED,
    HXN_GROUPSIZECHANGED,
    HXN_SELCHANGED,
    HXN_CTRLKEYPRESSED,
    HXN_ENTERPRESSED,
    HXN_COPY,
    HXN_PASTE,
};

enum HXCOLUMN
{
    HX_COL_NONE = -1,
    HX_COL_ADDRESS,
    HX_COL_HEXDATA,
    HX_COL_ASCII
};

typedef struct HXBYTEINFO_S
{
    bool     bHidden;
    bool     bValid;
    uint8_t  value;
    COLORREF color;
    COLORREF bkColor;

    bool operator==(const HXBYTEINFO_S& b) const
    {
        return memcmp(this, &b, sizeof(HXBYTEINFO_S)) == 0;
    }

    bool operator!=(const HXBYTEINFO_S& b) const
    {
        return memcmp(this, &b, sizeof(HXBYTEINFO_S)) != 0;
    }
} HXBYTEINFO;

template<>
struct std::hash<HXBYTEINFO>
{
    std::size_t operator()(const HXBYTEINFO& k) const
    {
        return (size_t)(k.bValid * 0xFFFFFFFF) ^ (k.value * 0x1010101) ^ k.color ^ k.bkColor;
    }
};

typedef struct
{
    NMHDR       nmh;
    uint32_t    address;
    size_t      numBytes;
    bool        bIgnoreDiff;
    HXBYTEINFO* oldBytes;
    HXBYTEINFO* newBytes;
} NMHXGETBYTEINFO;

typedef struct
{
    NMHDR    nmh;
    bool     bInsert;
    uint32_t address;
    uint8_t  value;
} NMHXSETBYTE;

typedef struct
{
    NMHDR    nmh;
    bool     bInsert;
    uint32_t address;
    bool     bLoNibble;
    uint8_t  value;
} NMHXSETNIBBLE;

typedef struct
{
    NMHDR    nmh;
    bool     bInsert;
    uint32_t startAddress;
    uint32_t endAddress;
    uint8_t  value;
} NMHXFILLRANGE;

typedef struct
{
    NMHDR    nmh;
    uint32_t address;
    uint32_t length;
    uint8_t* data;
} NMHXSETBYTES;

typedef struct
{
    NMHDR    nmh;
    uint32_t address;
} NMHXRCLICK;

typedef struct
{
    NMHDR    nmh;
    uint32_t address;
    HXCOLUMN column;
} NMHXPASTE;

typedef struct
{
    NMHDR nmh;
    int   nChar;
} NMHXCTRLKEYPRESSED;

class CHexEditCtrl :
    public CWindowImpl<CHexEditCtrl>
{
public:
    CHexEditCtrl(void);
    ~CHexEditCtrl(void);
    DECLARE_WND_CLASS(_T("HexEditCtrl"))
    BOOL     Attach(HWND hWnd);
    HWND     Detach(void);

    static char ByteAscii(uint8_t value);
    static uint8_t HexCharValue(char c);
    static int CALLBACK HaveFontCb(CONST LOGFONTA *lplf, CONST TEXTMETRICA *lptm, DWORD FontType, LPARAM lParam);
    static bool HaveFont(HDC hdc, const char* name);

    void     Draw(void);
    
    void     Copy(void);
    void     Paste(bool bAdvanceCaret = true);
    
    void     SetBaseAddress(uint32_t address);
    void     SetByteGroupSize(int nBytes);

    uint32_t GetBaseAddress(void);
    uint32_t GetCaretAddress(void);
    uint32_t GetHotAddress(void);
    int      GetNumBytesPerGroup(void);
    int      GetNumBytesPerRow(void);
    int      GetNumVisibleBytes(void);
    bool     GetSelectionRange(uint32_t* startAddress, uint32_t* endAddress);
    HXCOLUMN GetFocusedColumn(void);
    bool     GetInsertMode(void);

private:
    enum HXCELLSIDE
    {
        HX_LEFT,
        HX_RIGHT
    };

    enum
    {
        TIMER_ID_AUTO_REFRESH,
        TIMER_ID_DRAG_SCROLL
    };

    enum
    {
        BKCOLOR_DEFAULT       = RGB(255, 255, 255),
        BKCOLOR_ADDR          = RGB(220, 220, 220),
        COLOR_ADDR            = RGB(40, 40, 40),
        BKCOLOR_SEL_FOCUSED   = RGB(51, 153, 255),
        COLOR_SEL_FOCUSED     = RGB(255, 255, 255),
        BKCOLOR_SEL_UNFOCUSED = RGB(200, 200, 200),
        COLOR_SEL_UNFOCUSED   = RGB(0, 0, 0),
        BKCOLOR_HOT           = RGB(140, 140, 140)
    };

    typedef struct
    {
        HXCOLUMN    column;
        uint32_t    asciiAddress;
        HXCELLSIDE  asciiCellSide;
        uint32_t    hexAddress;
        HXCELLSIDE  hexCellSide;
    } HXHITTEST;

    typedef struct
    {
        CRect rcHex;
        CRect rcAscii;
    } HXRECTPAIR;

    uint32_t    m_BaseAddress;
    uint32_t    m_DrawnBaseAddress;
    uint32_t    m_SelStartAddress;
    HXCELLSIDE  m_SelStartCellSide;
    uint32_t    m_SelEndAddress;
    HXCELLSIDE  m_SelEndCellSide;
    uint32_t    m_RealSelStartAddress;
    uint32_t    m_RealSelEndAddress;
    bool        m_bHaveRealSel;
    bool        m_bInsertMode;
    bool        m_bHaveCaret;
    bool        m_bCaretVisible;
    uint32_t    m_CaretAddress;
    bool        m_bCaretLoNibble;
    bool        m_bShowHotAddress;
    uint32_t    m_HotAddress;
    HXCOLUMN    m_FocusedColumn;
    HFONT       m_Font;
    HBITMAP     m_BackBMP;
    HDC         m_BackDC;
    HCURSOR     m_hCursorIBeam;
    HCURSOR     m_hCursorDefault;
    int         m_DragScrollDelta;
    bool        m_bDblClicked;
    bool        m_bLButtonDown;
    bool        m_bMouseDragging;
    bool        m_bLayoutChanged;
    int         m_CharWidth;
    int         m_CharHeight;
    CRect       m_AddressColumnRect;
    CRect       m_HexDataColumnRect;
    CRect       m_AsciiColumnRect;
    int         m_NumBytesPerGroup;
    int         m_NumByteGroupsPerRow;
    int         m_NumBytesPerRow;
    int         m_NumVisibleRows;
    int         m_NumVisibleBytes;
    HXBYTEINFO* m_NewBytes;
    HXBYTEINFO* m_OldBytes;

    static COLORREF BlendColor(COLORREF c1, COLORREF c2);
    static uint32_t SatAdd32(uint32_t a, uint32_t b);
    static uint32_t SatAdd32(uint32_t a, int b);

    void    DrawAddressColumn(void);
    void    DrawHeader(void);
    void    Text(int x, int y, const char *text, COLORREF bg, COLORREF fg, CRect* rcOut);
    bool    IsSelected(uint32_t address);
    int     GetSelDirection(void);
    void    CancelSelection(void);
    void    SelectAllVisible(void);
    void    UpdateRealSelection(void);

    uint32_t LineAddress(uint32_t address);

    void    GetHexCellPos(int index, CRect* rc);
    void    GetAsciiCellPos(int index, CRect* rc);
    void    HitTest(int x, int y, HXHITTEST* pht);

    void    ShowCaret(void);
    void    HideCaret(void);
    void    CaretIncrementNibble(void);
    void    CaretDecrementNibble(void);
    bool    UpdateCaretUI(bool bEnsureVisible, bool bTop = false);
    void    EnsureCaretAddressVisible(bool bTop = false);
    bool    IsCaretAddressVisible(void);

    void    UpdateLayoutInfo(void);
    void    ReallocByteBuffers(void);

    LRESULT Notify(UINT code);
    LRESULT NotifyGetByteInfo(uint32_t address, size_t numBytes, bool bIgnoreDiff, HXBYTEINFO* oldBytes, HXBYTEINFO* newBytes);
    LRESULT NotifySetByte(uint32_t address, uint8_t value);
    LRESULT NotifySetNibble(uint32_t address, bool bLoNibble, uint8_t value);
    LRESULT NotifyFillRange(uint32_t startAddress, uint32_t endAddress, uint8_t value);
    LRESULT NotifyCtrlKeyPressed(int nChar);
    LRESULT NotifyPaste(uint32_t address);
    LRESULT NotifyRightClick(uint32_t address);

    void    OnLButtonDown(UINT nFlags, CPoint point);
    void    OnLButtonDblClk(UINT nFlags, CPoint point);
    void    OnLButtonUp(UINT nFlags, CPoint point);
    void    OnRButtonDown(UINT nFlags, CPoint point);
    void    OnRButtonUp(UINT nFlags, CPoint point);
    void    OnMouseMove(UINT nFlags, CPoint point);
    BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    void    OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void    OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    void    OnSetFocus(CWindow wndOld);
    void    OnKillFocus(CWindow wndFocus);
    void    OnTimer(UINT_PTR nIDEvent);
    UINT    OnGetDlgCode(LPMSG lpMsg);
    void    OnPaint(CDCHandle dc);
    BOOL    OnSetCursor(CWindow wnd, UINT nHitTest, UINT message);
    void    OnWindowPosChanged(LPWINDOWPOS lpWndPos);

    BEGIN_MSG_MAP_EX(CHexEditCtrl)
        MSG_WM_RBUTTONDOWN(OnRButtonDown)
        MSG_WM_LBUTTONDOWN(OnLButtonDown)
        MSG_WM_LBUTTONUP(OnLButtonUp)
        MSG_WM_RBUTTONUP(OnRButtonUp)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
        MSG_WM_MOUSEMOVE(OnMouseMove)
        MSG_WM_MOUSEWHEEL(OnMouseWheel)
        MSG_WM_KEYDOWN(OnKeyDown)
        MSG_WM_CHAR(OnChar)
        MSG_WM_SETFOCUS(OnSetFocus)
        MSG_WM_KILLFOCUS(OnKillFocus)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_GETDLGCODE(OnGetDlgCode)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_SETCURSOR(OnSetCursor)
        MSG_WM_WINDOWPOSCHANGED(OnWindowPosChanged)
    END_MSG_MAP()
};
