/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/

#pragma once
#include "DebuggerUI.h"

class CDebugStackTrace :
	public CDebugDialog<CDebugStackTrace>,
	public CDialogResize<CDebugStackTrace>
{
public:
	enum { IDD = IDD_Debugger_StackTrace };

	CDebugStackTrace(CDebuggerUI * debugger);
	virtual ~CDebugStackTrace(void);

	void RefreshList();

private:
	
	HANDLE m_AutoRefreshThread;
	static DWORD WINAPI AutoRefreshProc(void* _this);
	
	CListViewCtrl m_List;

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnListDblClicked(NMHDR* pNMHDR);
	LRESULT OnDestroy(void);

	BEGIN_MSG_MAP_EX(CDebugStackTrace)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
		NOTIFY_HANDLER_EX(IDC_STACKTRACE_LIST, NM_DBLCLK, OnListDblClicked)
		CHAIN_MSG_MAP(CDialogResize<CDebugStackTrace>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CDebugStackTrace)
		DLGRESIZE_CONTROL(IDC_STACKTRACE_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()
};