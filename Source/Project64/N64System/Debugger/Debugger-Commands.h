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

class CCommandList : public CWindowImpl<CCommandList, CListViewCtrl>
{
public:
	enum {
		COL_ARROWS,
		COL_ADDRESS,
		COL_COMMAND,
		COL_PARAMETERS,
		COL_SYMBOL
	};

	enum {
		ROW_HEIGHT = 13,
	};

	void Attach(HWND hWndNew)
	{
		CListViewCtrl::Attach(hWndNew);
		ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0);
		SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
		AddColumn("", COL_ARROWS);
		AddColumn("Address", COL_ADDRESS);
		AddColumn("Command", COL_COMMAND);
		AddColumn("Parameters", COL_PARAMETERS);
		AddColumn("Symbol", COL_SYMBOL);
		SetColumnWidth(COL_ARROWS, 30);
		SetColumnWidth(COL_ADDRESS, 65);
		SetColumnWidth(COL_COMMAND, 60);
		SetColumnWidth(COL_PARAMETERS, 120);
		SetColumnWidth(COL_SYMBOL, 140);
	}
	
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
	LRESULT OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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
	public CToolTipDialog<CDebugCommandsView>
{
	friend class CEditOp;

public:
	enum { IDD = IDD_Debugger_Commands };

	CDebugCommandsView(CDebuggerUI * debugger);
	virtual ~CDebugCommandsView(void);

	void ShowAddress(DWORD address, BOOL top);
	void ShowPIRegTab();
	
	void Reset();

private:
	CBreakpoints* m_Breakpoints;
	vector<uint32_t> m_History;
	int m_HistoryIndex;

	CAddBreakpointDlg m_AddBreakpointDlg;
	CAddSymbolDlg m_AddSymbolDlg;

	DWORD m_StartAddress;
	CRect m_DefaultWindowRect;

	CEditNumber m_PCEdit;
	bool        m_bIgnorePCChange;

	CEditNumber m_AddressEdit;
	bool        m_bIgnoreAddrChange;

	CCommandList m_CommandList;
	int m_CommandListRows;
	CListBox m_BreakpointList;
	CScrollBar m_Scrollbar;

	CRegisterTabs m_RegisterTabs;

	CButton m_BackButton;
	CButton m_ForwardButton;

	CButton m_ViewPCButton;
	CButton m_StepButton;
	CButton m_SkipButton;
	CButton m_GoButton;

	bool m_bEditing;
	CEditOp  m_OpEdit;

	uint32_t m_SelectedAddress;
	COpInfo  m_SelectedOpInfo;
	OPCODE&  m_SelectedOpCode = m_SelectedOpInfo.m_OpCode;

	uint32_t m_FollowAddress;

	typedef struct {
		uint32_t address;
		uint32_t originalOp;
	} EditedOp;

	vector<EditedOp> m_EditedOps;

	typedef struct {
		int startPos;
		int endPos;
		int startMargin;
		int endMargin;
		int margin;
	} BRANCHARROW;

	vector<BRANCHARROW> m_BranchArrows;
	void AddBranchArrow(int startPos, int endPos);
	void ClearBranchArrows();
	void DrawBranchArrows(HDC listDC);

	vector<bool> m_bvAnnotatedLines;

	void ClearEditedOps();
	void EditOp(uint32_t address, uint32_t op);
	void RestoreOp(uint32_t address);
	void RestoreAllOps();
	BOOL IsOpEdited(uint32_t address);
	void BeginOpEdit(uint32_t address);
	void EndOpEdit();

	void GotoEnteredAddress();
	void CheckCPUType();
	void RefreshBreakpointList();
	void RemoveSelectedBreakpoints();
	
	bool AddressSafe(uint32_t vaddr);

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
	void CPUStepInto();
	void CPUResume();

	LRESULT	OnInitDialog         (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT	OnActivate           (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT	OnSizing             (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT	OnScroll             (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT	OnMeasureItem        (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnAddrChanged        (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnPCChanged        (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnListBoxClicked     (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT	OnClicked            (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT	OnOpKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	
	LRESULT	OnCommandListClicked(NMHDR* pNMHDR);
	LRESULT	OnCommandListDblClicked(NMHDR* pNMHDR);
	LRESULT	OnCommandListRightClicked (NMHDR* pNMHDR);
	LRESULT OnRegisterTabChange  (NMHDR* pNMHDR);
	LRESULT OnCustomDrawList     (NMHDR* pNMHDR);
	LRESULT OnDestroy            (void);

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
		COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
		NOTIFY_HANDLER_EX(IDC_CMD_LIST, NM_CLICK, OnCommandListClicked)
		NOTIFY_HANDLER_EX(IDC_CMD_LIST, NM_DBLCLK, OnCommandListDblClicked)
		NOTIFY_HANDLER_EX(IDC_CMD_LIST, NM_RCLICK, OnCommandListRightClicked)
		NOTIFY_HANDLER_EX(IDC_REG_TABS, TCN_SELCHANGE, OnRegisterTabChange)
		NOTIFY_HANDLER_EX(IDC_CMD_LIST, NM_CUSTOMDRAW, OnCustomDrawList)
		MSG_WM_DESTROY(OnDestroy)
		CHAIN_MSG_MAP(CDialogResize<CDebugCommandsView>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CDebugCommandsView)
		DLGRESIZE_CONTROL(IDC_GO_BTN, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_STEP_BTN, DLSZ_MOVE_X)
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
		TOOLTIP(IDC_GO_BTN, "Go (F4)")
		TOOLTIP(IDC_ADDBP_BTN, "Add breakpoint...")
		TOOLTIP(IDC_RMBP_BTN, "Remove selected breakpoint")
		TOOLTIP(IDC_CLEARBP_BTN, "Remove all breakpoints")
		TOOLTIP(IDC_VIEWPC_BTN, "Jump to program counter")
		TOOLTIP(IDC_BP_LIST, "Active breakpoints")
		TOOLTIP(IDC_SYMBOLS_BTN, "Symbols...")
	END_TOOLTIP_MAP()
};
