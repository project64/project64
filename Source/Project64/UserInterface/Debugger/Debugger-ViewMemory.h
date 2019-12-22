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

#include "Debugger-AddSymbol.h"
#include <UserInterface/WTLControls/HexEditCtrl.h>
#include <Project64/UserInterface/WTLControls/TooltipDialog.h>

typedef struct
{
    NMHDR nmh;
    int   nItem;
} NMMTRCLICK;

enum
{
    MTCN_RCLICK
};

class CMemTabCtrl :
    public CWindowImpl<CMemTabCtrl, CTabCtrl>
{
private:
    int m_nItemRClick;

public:
    CMemTabCtrl() :
        m_nItemRClick(-1)
    {
    }

    BOOL Attach(HWND hWndNew)
    {
        return SubclassWindow(hWndNew);
    }

private:
    void OnLButtonDblClk(UINT /*nFlags*/, CPoint /*point*/)
    {
        NMHDR nmh = { m_hWnd, (UINT_PTR)::GetDlgCtrlID(m_hWnd), NM_DBLCLK };
        ::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, NM_DBLCLK, (LPARAM)&nmh);
    }

    void OnRButtonDown(UINT /*nFlags*/, CPoint point)
    {
        TCHITTESTINFO ht = { point, 0};
        int nItem = ::SendMessage(m_hWnd, TCM_HITTEST, 0, (LPARAM)&ht);
        if (nItem != -1)
        {
            m_nItemRClick = nItem;
        }
    }

    void OnRButtonUp(UINT /*nFlags*/, CPoint point)
    {
        TCHITTESTINFO ht = { point, 0 };
        int nItem = ::SendMessage(m_hWnd, TCM_HITTEST, 0, (LPARAM)&ht);
        if (nItem != -1 && nItem == m_nItemRClick)
        {
            NMMTRCLICK nmrc = { { m_hWnd, (UINT_PTR)::GetDlgCtrlID(m_hWnd), MTCN_RCLICK }, nItem };
            ::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, MTCN_RCLICK, (LPARAM)&nmrc);
        }
        m_nItemRClick = -1;
    }

    BEGIN_MSG_MAP_EX(CMemTabCtrl)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
        MSG_WM_RBUTTONDOWN(OnRButtonDown)
        MSG_WM_RBUTTONUP(OnRButtonUp)
        MSG_WM_RBUTTONDBLCLK(OnRButtonDown)
    END_MSG_MAP()
};

class CDebugMemoryView :
    public CDebugDialog<CDebugMemoryView>,
    public CDialogResize<CDebugMemoryView>,
    public CToolTipDialog<CDebugMemoryView>
{
public:
    enum { IDD = IDD_Debugger_Memory };

    CDebugMemoryView(CDebuggerUI * debugger);
    virtual ~CDebugMemoryView(void);

    void ShowAddress(uint32_t address, bool bVirtual);

private:
    enum
    {
        WM_SHOWADDRESS = WM_USER + 1
    };

    enum
    {
        COLOR_DEFAULT = RGB(0, 0, 0),
        COLOR_READONLY = RGB(0, 100, 0),
        BKCOLOR_DEFAULT = RGB(255, 255, 255),
        COLOR_CHANGED = RGB(255, 0, 0),
        BKCOLOR_HOT = RGB(240, 240, 240),
        BKCOLOR_LOCKED = RGB(100, 100, 0),
        COLOR_BP = RGB(255, 255, 255),
        BKCOLOR_RWBP = RGB(100, 0, 100),
        BKCOLOR_RBP = RGB(0, 0, 100),
        BKCOLOR_WBP = RGB(100, 0, 0),
        BKCOLOR_CPUREAD = RGB(200, 200, 255),
        BKCOLOR_CPUWRITE = RGB(255, 200, 200),
        BKCOLOR_SYMBOL0 = RGB(208, 240, 208),
        BKCOLOR_SYMBOL1 = RGB(176, 208, 176),
        BKCOLOR_SAFEEDIT = RGB(255, 230, 255),
        COLOR_SAFEEDIT = RGB(255, 0, 255)
    };

    enum
    {
        MEMSB_HOTADDR,
        MEMSB_BLOCK,
        MEMSB_BLOCKLEN,
        MEMSB_DMAINFO,
        MEMSB_SAFEMODE,
        MEMSB_NUM_PANES
    };

    enum
    {
        MEMSB_HOTADDR_W   = 160,
        MEMSB_BLOCK_W     = 120 + MEMSB_HOTADDR_W,
        MEMSB_BLOCKLEN_W  = 60  + MEMSB_BLOCK_W,
        MEMSB_DMAINFO_W   = 60 + MEMSB_BLOCKLEN_W,
        MEMSB_SAFEMODE_W  = -1
    };

    enum edit_type_t
    {
        SE_FILL,
        //SE_DATA
    };

    typedef struct
    {
        edit_type_t type;
        uint32_t startAddress;
        uint32_t endAddress;
        uint8_t  value;
        //uint8_t* data;
    } edit_t;

    typedef struct
    {
        uint32_t vaddr;
        uint32_t paddr;
        uint32_t size;
        const char* caption;
    } jump_item_t;

    typedef struct
    {
        uint32_t address;
        bool     bVirtual;
        int      numBytesPerGroup;
    } tab_info_t;

    static jump_item_t JumpItems[];
    static int GetJumpItemIndex(uint32_t address, bool bVirtual);

    CHexEditCtrl   m_HexEditCtrl;
    CEditNumber32  m_MemAddr;
    CAddSymbolDlg  m_AddSymbolDlg;
    CButton        m_VirtualCheckbox;
    CMemTabCtrl    m_TabCtrl;
    CStatusBarCtrl m_StatusBar;
    CComboBox      m_CmbJump;

    std::vector<tab_info_t> m_TabData;
    CBreakpoints*  m_Breakpoints;

    int            m_WriteTargetColorStride;
    int            m_ReadTargetColorStride;
    int            m_SymbolColorStride;
    int            m_SymbolColorPhase;
    uint32_t       m_ContextMenuAddress;
    uint32_t       m_HotAddress;
    bool           m_bIgnoreAddressInput;
    bool           m_bVirtualMemory;

    bool                m_bSafeEditMode;
    std::vector<edit_t> m_SafeEditQueue;
    
    bool     GetByte(uint32_t address, uint8_t* value);
    bool     SetByte(uint32_t address, uint8_t value);
    void     SetupJumpMenu(bool bVirtual);
    bool     GetSafeEditValue(uint32_t address, uint8_t* value);
    void     ApplySafeEdits(void);
    void     CopyTextToClipboard(const char* text);
    void     CopyBytesToClipboard(uint32_t startAddress, uint32_t endAddress, bool bHex, bool bIncludeAddresses = false, bool bRowAddresses = false);
    void     CopyGameSharkCodeToClipboard(uint32_t startAddress, uint32_t endAddress);
    void     FillRange(uint32_t startAddress, uint32_t endAddress, uint8_t value);
    void     FollowPointer(bool bContextMenuAddress = true);
    void     JumpToSelection(void);
    int      AddTab(uint32_t address, bool bVirtual, int numBytesPerGroup);
    int      InsertTab(int nItem, uint32_t address, bool bVirtual, int numBytesPerGroup);
    void     DeleteTab(int index);
    void     UpdateCurrentTab(uint32_t address);
    void     OpenNewTab(uint32_t address, bool bVirtual, int numBytesPerGroup, bool bInsert = false, bool bOpenExisting = false);
    void     OpenDuplicateTab(void);
    void     CloseTab(int nItem);
    void     CloseCurrentTab(void);
    void     TabSelChanged(void);

    LRESULT  OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT  OnShowAddress(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT  OnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    void     OnAddrChanged(UINT Code, int id, HWND ctl);
    void     OnVScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
    void     OnExitSizeMove(void);
    LRESULT  OnDestroy(void);
    LRESULT  OnHxGetByteInfo(LPNMHDR lpNMHDR);
    LRESULT  OnHxSetNibble(LPNMHDR lpNMHDR);
    LRESULT  OnHxSetByte(LPNMHDR lpNMHDR);
    LRESULT  OnHxRightClick(LPNMHDR lpNMHDR);
    LRESULT  OnHxEnterPressed(LPNMHDR lpNMHDR);
    LRESULT  OnHxRedrawStarted(LPNMHDR lpNMHDR);
    LRESULT  OnHxBaseAddrChanged(LPNMHDR lpNMHDR);
    LRESULT  OnHxHotAddrChanged(LPNMHDR lpNMHDR);
    LRESULT  OnHxCopy(LPNMHDR lpNMHDR);
    LRESULT  OnHxPaste(LPNMHDR lpNMHDR);
    LRESULT  OnHxCtrlKeyPressed(LPNMHDR lpNMHDR);
    LRESULT  OnHxFillRange(LPNMHDR lpNMHDR);
    LRESULT  OnHxInsertModeChanged(LPNMHDR lpNMHDR);
    LRESULT  OnHxSelectionChanged(LPNMHDR lpNMHDR);
    LRESULT  OnHxGroupSizeChanged(LPNMHDR lpNMHDR);
    LRESULT  OnTabSelChange(LPNMHDR lpNMHDR);
    LRESULT  OnTabDblClick(LPNMHDR lpNMHDR);
    LRESULT  OnTabRClick(LPNMHDR lpNMHDR);
    LRESULT  OnStatusBarClick(LPNMHDR lpNMHDR);
    void     OnJumpComboSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);

    BEGIN_MSG_MAP_EX(CDebugMemoryView)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SHOWADDRESS, OnShowAddress)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        COMMAND_HANDLER_EX(IDC_ADDR_EDIT, EN_CHANGE, OnAddrChanged)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_VSCROLL(OnVScroll)
        COMMAND_HANDLER_EX(IDC_CMB_JUMP, CBN_SELCHANGE, OnJumpComboSelChange)
        NOTIFY_HANDLER_EX(IDC_STATUSBAR, NM_CLICK, OnStatusBarClick)
        NOTIFY_HANDLER_EX(IDC_MEMTABS, NM_DBLCLK, OnTabDblClick)
        NOTIFY_HANDLER_EX(IDC_MEMTABS, TCN_SELCHANGE, OnTabSelChange)
        NOTIFY_HANDLER_EX(IDC_MEMTABS, MTCN_RCLICK, OnTabRClick)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_GETBYTEINFO, OnHxGetByteInfo)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_SETNIBBLE, OnHxSetNibble)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_SETBYTE, OnHxSetByte)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_RCLICK, OnHxRightClick)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_HOTADDRCHANGED, OnHxHotAddrChanged)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_REDRAWSTARTED, OnHxRedrawStarted)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_BASEADDRCHANGED, OnHxBaseAddrChanged)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_HOTADDRCHANGED, OnHxHotAddrChanged)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_PASTE, OnHxPaste)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_CTRLKEYPRESSED, OnHxCtrlKeyPressed)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_FILLRANGE, OnHxFillRange)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_COPY, OnHxCopy)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_INSERTMODECHANGED, OnHxInsertModeChanged)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_ENTERPRESSED, OnHxEnterPressed)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_SELCHANGED, OnHxSelectionChanged)
        NOTIFY_HANDLER_EX(IDC_HEXEDIT, HXN_GROUPSIZECHANGED, OnHxGroupSizeChanged)
        CHAIN_MSG_MAP(CDialogResize<CDebugMemoryView>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugMemoryView)
        DLGRESIZE_CONTROL(IDC_CMB_JUMP, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_STATUSBAR, DLSZ_SIZE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_HEXEDIT, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_MEMTABS, DLSZ_SIZE_X)
        DLGRESIZE_CONTROL(IDC_SCRL_BAR, DLSZ_MOVE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_TOOLTIP_MAP()
        TOOLTIP(IDC_SYMBOLS_BTN, "Symbols...")
        TOOLTIP(IDC_CHK_VADDR, "Checked = Use virtual address space (CPU), Unchecked = Use physical address space (RCP)")
    END_TOOLTIP_MAP()
};

