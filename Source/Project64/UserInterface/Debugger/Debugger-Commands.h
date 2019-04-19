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

#include "OpInfo.h"
#include "Breakpoints.h"
#include "Debugger-AddBreakpoint.h"
#include "Debugger-RegisterTabs.h"

#include <Project64/UserInterface/WTLControls/TooltipDialog.h>

class CCommandList :
    public CWindowImpl<CCommandList, CListViewCtrl>
{
public:
    enum
    {
        COL_ARROWS,
        COL_ADDRESS,
        COL_COMMAND,
        COL_PARAMETERS,
        COL_SYMBOL
    };

    void Attach(HWND hWndNew);

    BEGIN_MSG_MAP_EX(CCommandsList)
    END_MSG_MAP()
};

class CEditOp;
class CDebugCommandsView;

class CEditOp : public CWindowImpl<CEditOp, CEdit>
{
private:
    CDebugCommandsView* m_CommandsWindow;

    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
        if (wParam == VK_RETURN || wParam == VK_ESCAPE)
        {
            bHandled = TRUE;
            return 0;
        }
        bHandled = FALSE;
        return 0;
    }

    BEGIN_MSG_MAP_EX(CEditOp)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
        MESSAGE_HANDLER(WM_CHAR, OnKeyUp)
        END_MSG_MAP()

public:
    void SetCommandsWindow(CDebugCommandsView* commandsWindow);
    BOOL Attach(HWND hWndNew);
};

class CDebugCommandsView :
    public CDebugDialog<CDebugCommandsView>,
    public CDialogResize<CDebugCommandsView>,
    public CToolTipDialog<CDebugCommandsView>,
    public CDebugSettings
{
    friend class CEditOp;

public:
    enum { IDD = IDD_Debugger_Commands };

    CDebugCommandsView(CDebuggerUI * debugger, SyncEvent &StepEvent);
    virtual ~CDebugCommandsView(void);

    void ShowAddress(uint32_t address, bool top, bool bUserInput = false);
    void ShowPIRegTab();

    void Reset();

private:
    BEGIN_MSG_MAP_EX(CDebugCommandsView)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_SIZING, OnSizing)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_VSCROLL, OnScroll)
        MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
        COMMAND_HANDLER(IDC_ADDR_EDIT, EN_CHANGE, OnAddrChanged)
        COMMAND_HANDLER(IDC_PC_EDIT, EN_CHANGE, OnPCChanged)
        COMMAND_CODE_HANDLER(LBN_DBLCLK, OnListBoxClicked)
        COMMAND_HANDLER(IDC_BACK_BTN, BN_CLICKED, OnBackButton)
        COMMAND_HANDLER(IDC_FORWARD_BTN, BN_CLICKED, OnForwardButton)
        COMMAND_HANDLER(IDC_VIEWPC_BTN, BN_CLICKED, OnViewPCButton)
        COMMAND_HANDLER(IDC_SYMBOLS_BTN, BN_CLICKED, OnSymbolsButton)
        COMMAND_HANDLER(ID_POPUPMENU_RUNTO, BN_CLICKED, OnPopupmenuRunTo)
        COMMAND_HANDLER(IDC_GO_BTN, BN_CLICKED, OnGoButton)
        COMMAND_HANDLER(IDC_STEP_BTN, BN_CLICKED, OnStepButton)
        COMMAND_HANDLER(IDC_STEPOVER_BTN, BN_CLICKED, OnStepOverButton)
        COMMAND_HANDLER(IDC_SKIP_BTN, BN_CLICKED, OnSkipButton)
        COMMAND_HANDLER(IDC_CLEARBP_BTN, BN_CLICKED, OnClearBPButton)
        COMMAND_HANDLER(IDC_ADDBP_BTN, BN_CLICKED, OnAddBPButton)
        COMMAND_HANDLER(IDC_RMBP_BTN, BN_CLICKED, OnRemoveBPButton)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
        COMMAND_HANDLER(ID_POPUPMENU_EDIT, BN_CLICKED, OnPopupmenuEdit)
        COMMAND_HANDLER(ID_POPUPMENU_INSERTNOP, BN_CLICKED, OnPopupmenuInsertNOP)
        COMMAND_HANDLER(ID_POPUPMENU_RESTORE, BN_CLICKED, OnPopupmenuRestore)
        COMMAND_HANDLER(ID_POPUPMENU_RESTOREALL, BN_CLICKED, OnPopupmenuRestoreAll)
        COMMAND_HANDLER(ID_POPUPMENU_ADDSYMBOL, BN_CLICKED, OnPopupmenuAddSymbol)
        COMMAND_HANDLER(ID_POPUPMENU_FOLLOWJUMP, BN_CLICKED, OnPopupmenuFollowJump)
        COMMAND_HANDLER(ID_POPUPMENU_VIEWMEMORY, BN_CLICKED, OnPopupmenuViewMemory)
        COMMAND_HANDLER(ID_POPUPMENU_TOGGLEBP, BN_CLICKED, OnPopupmenuToggleBP)
        COMMAND_HANDLER(ID_POPUPMENU_CLEARBPS, BN_CLICKED, OnPopupmenuClearBP   )
        NOTIFY_HANDLER_EX(IDC_CMD_LIST, NM_CLICK, OnCommandListClicked)
        NOTIFY_HANDLER_EX(IDC_CMD_LIST, NM_DBLCLK, OnCommandListDblClicked)
        NOTIFY_HANDLER_EX(IDC_CMD_LIST, NM_RCLICK, OnCommandListRightClicked)
        NOTIFY_HANDLER_EX(IDC_REG_TABS, TCN_SELCHANGE, OnRegisterTabChange)
        NOTIFY_HANDLER_EX(IDC_CMD_LIST, NM_CUSTOMDRAW, OnCustomDrawList)
        MSG_WM_DESTROY(OnDestroy)
        CHAIN_MSG_MAP(CDialogResize<CDebugCommandsView>)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugCommandsView)
        DLGRESIZE_CONTROL(IDC_GO_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_STEP_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_STEPOVER_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_SKIP_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_ADDR_EDIT, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_SYMBOLS_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_OPCODE_BOX, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_BP_LIST, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_ADDBP_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_RMBP_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_CLEARBP_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_REG_TABS, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_BACK_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_FORWARD_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_PC_STATIC, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_PC_EDIT, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_VIEWPC_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_CMD_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_SCRL_BAR, DLSZ_MOVE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_TOOLTIP_MAP()
        TOOLTIP(IDC_SKIP_BTN, "Skip (F1)")
        TOOLTIP(IDC_STEP_BTN, "Step (F2)")
        TOOLTIP(IDC_STEPOVER_BTN, "Step Over (F3)")
        TOOLTIP(IDC_GO_BTN, "Go (F4)")
        TOOLTIP(IDC_ADDBP_BTN, "Add breakpoint...")
        TOOLTIP(IDC_RMBP_BTN, "Remove selected breakpoint")
        TOOLTIP(IDC_CLEARBP_BTN, "Remove all breakpoints")
        TOOLTIP(IDC_VIEWPC_BTN, "Jump to program counter")
        TOOLTIP(IDC_BP_LIST, "Active breakpoints")
        TOOLTIP(IDC_SYMBOLS_BTN, "Symbols...")
    END_TOOLTIP_MAP()

    static void StaticSteppingOpsChanged(CDebugCommandsView * __this) { __this->SteppingOpsChanged(); }
    static void StaticWaitingForStepChanged(CDebugCommandsView * __this) { __this->WaitingForStepChanged(); }

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT	OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSizing(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnAddrChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPCChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnListBoxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBackButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnForwardButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnViewPCButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnSymbolsButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnPopupmenuRunTo(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnGoButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnStepButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnStepOverButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnSkipButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnClearBPButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnAddBPButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnRemoveBPButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnPopupmenuEdit(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnPopupmenuInsertNOP(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnPopupmenuRestore(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnPopupmenuRestoreAll(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnPopupmenuAddSymbol(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnPopupmenuFollowJump(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnPopupmenuViewMemory(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnPopupmenuToggleBP(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnPopupmenuClearBP(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT	OnOpKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT	OnCommandListClicked(NMHDR* pNMHDR);
    LRESULT	OnCommandListDblClicked(NMHDR* pNMHDR);
    LRESULT	OnCommandListRightClicked(NMHDR* pNMHDR);
    LRESULT OnRegisterTabChange(NMHDR* pNMHDR);
    LRESULT OnCustomDrawList(NMHDR* pNMHDR);
    LRESULT OnDestroy(void);
    void OnExitSizeMove(void);

    void ClearEditedOps();
    void EditOp(uint32_t address, uint32_t op);
    void RestoreOp(uint32_t address);
    void RestoreAllOps();
    BOOL IsOpEdited(uint32_t address);
    void BeginOpEdit(uint32_t address);
    void EndOpEdit();

    void RefreshBreakpointList();
    void RemoveSelectedBreakpoints();

    void HistoryPushState();
    void ToggleHistoryButtons();

    static CDebugCommandsView* _this;
    static HHOOK hWinMessageHook;
    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);
    void InterceptKeyDown(WPARAM wParam, LPARAM lParam);
    void InterceptMouseWheel(WPARAM wParam, LPARAM lParam);

    static const char* GetCodeAddressNotes(uint32_t vAddr);
    static const char* GetDataAddressNotes(uint32_t vAddr);

    void CPUSkip();
    void CPUResume();
    void CPUStepOver();
    void WaitingForStepChanged(void);
    void SteppingOpsChanged(void);

    void AddBranchArrow(int startPos, int endPos);
    void ClearBranchArrows();
    void DrawBranchArrows(HDC listDC);

    void RedrawCommandsAndRegisters();

    CBreakpoints* m_Breakpoints;
    vector<uint32_t> m_History;
    int m_HistoryIndex;

    CAddBreakpointDlg m_AddBreakpointDlg;
    CAddSymbolDlg m_AddSymbolDlg;

    uint32_t m_StartAddress;
    CEditNumber32 m_PCEdit;
    bool m_bIgnorePCChange;

    CEditNumber32 m_AddressEdit;
    bool m_bIgnoreAddrChange;

    CCommandList m_CommandList;
    int m_CommandListRows;
    CListBox m_BreakpointList;
    CScrollBar m_Scrollbar;

    CRegisterTabs m_RegisterTabs;
    SyncEvent & m_StepEvent;

    CButton m_BackButton;
    CButton m_ForwardButton;

    CButton m_ViewPCButton;
    CButton m_StepButton;
    CButton m_StepOverButton;
    CButton m_SkipButton;
    CButton m_GoButton;

    bool m_bEditing;
    CEditOp  m_OpEdit;

    uint32_t m_SelectedAddress;
    COpInfo  m_SelectedOpInfo;
    OPCODE&  m_SelectedOpCode = m_SelectedOpInfo.m_OpCode;

    uint32_t m_FollowAddress;
    uint32_t m_RowHeight;

    typedef struct
    {
        uint32_t address;
        uint32_t originalOp;
    } EditedOp;

    vector<EditedOp> m_EditedOps;

    typedef struct
    {
        int startPos;
        int endPos;
        int startMargin;
        int endMargin;
        int margin;
    } BRANCHARROW;

    std::vector<BRANCHARROW> m_BranchArrows;
    vector<bool> m_bvAnnotatedLines;
    bool m_Attached;
};
