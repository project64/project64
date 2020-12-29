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
#include "DebuggerUI.h"
#include "ScriptSystem.h"
#include <Project64/UserInterface/WTLControls/TooltipDialog.h>

class CScriptList : public CListViewCtrl
{
public:
    BEGIN_MSG_MAP_EX(CScriptList)
    END_MSG_MAP()
};

class CEditEval : public CWindowImpl<CEditEval, CEdit>
{
private:
    //static char* m_EvalString;
    static const int HISTORY_MAX_ENTRIES = 20;
    vector<wchar_t*> m_History;
    int m_HistoryIdx;
    CDebugScripts* m_ScriptWindow;

public:
    CEditEval()
    {
        m_HistoryIdx = 0;
    }

    void SetScriptWindow(CDebugScripts* scriptWindow)
    {
        m_ScriptWindow = scriptWindow;
    }

    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BOOL Attach(HWND hWndNew)
    {
        return SubclassWindow(hWndNew);
    }

    BEGIN_MSG_MAP_EX(CEditEval)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    END_MSG_MAP()
};

class CEditConsole : public CWindowImpl<CEditEval, CEdit>
{
private:
    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        if (GetKeyState(VK_CONTROL) < 0)
        {
            if (wParam == 'A')
            {
                this->SetSelAll();
            }
        }
        return FALSE;
    }
public:
    BOOL Attach(HWND hWndNew)
    {
        return SubclassWindow(hWndNew);
    }

    BEGIN_MSG_MAP_EX(CEditEval)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    END_MSG_MAP()
};

class CDebugScripts :
    public CDebugDialog<CDebugScripts>,
    public CDialogResize<CDebugScripts>,
    public CToolTipDialog<CDebugScripts>
{
private:
    enum {
        WM_REFRESH_LIST = WM_USER + 1,
        WM_CONSOLE_PRINT = WM_USER + 2,
        WM_CONSOLE_CLEAR = WM_USER + 3
    };

    CEditEval m_EvalEdit;
    CEditConsole m_ConsoleEdit;
    CScriptList m_ScriptList;
    CStatusBarCtrl m_StatusBar;
    char* m_SelectedScriptName;

    HANDLE m_hQuitScriptDirWatchEvent;
    HANDLE m_hScriptDirWatchThread;
    static DWORD WINAPI ScriptDirWatchProc(void *ctx);

    void RunSelected();
    void StopSelected();
    void ToggleSelected();
    void EditSelected();
    void RefreshStatus();
    void ConsoleCopy();

public:
    enum { IDD = IDD_Debugger_Scripts };

    CDebugScripts(CDebuggerUI * debugger);
    virtual ~CDebugScripts(void);

    void EvaluateInSelectedInstance(const char* code);
    void ConsolePrint(const char* text);
    void ConsoleClear();
    void RefreshList();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(void);
    LRESULT OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnScriptListDblClicked(NMHDR* pNMHDR);
    LRESULT OnScriptListRClicked(NMHDR* pNMHDR);
    LRESULT OnScriptListCustomDraw(NMHDR* pNMHDR);
    LRESULT OnScriptListItemChanged(NMHDR* pNMHDR);
    void OnExitSizeMove(void);

    LRESULT OnConsoleLog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnConsoleClear(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRefreshList(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BEGIN_MSG_MAP_EX(CDebugScripts)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_DBLCLK, OnScriptListDblClicked)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_RCLICK, OnScriptListRClicked)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_CUSTOMDRAW, OnScriptListCustomDraw)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, LVN_ITEMCHANGED, OnScriptListItemChanged)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove);
        MESSAGE_HANDLER(WM_CONSOLE_PRINT, OnConsoleLog)
        MESSAGE_HANDLER(WM_CONSOLE_CLEAR, OnConsoleClear)
        MESSAGE_HANDLER(WM_REFRESH_LIST, OnRefreshList)
        CHAIN_MSG_MAP(CDialogResize<CDebugScripts>)
        CHAIN_MSG_MAP_MEMBER(m_ScriptList)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugScripts)
        DLGRESIZE_CONTROL(IDC_CONSOLE_EDIT, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_SCRIPT_LIST, DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_CLEAR_BTN, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_COPY_BTN, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_SCRIPTS_GRP, DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_OUTPUT_GRP, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_EVAL_LBL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_EVAL_EDIT, DLSZ_SIZE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_RUN_BTN, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_STOP_BTN, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_SCRIPTDIR_BTN, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_STATUSBAR, DLSZ_SIZE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_TOOLTIP_MAP()
        TOOLTIP(IDC_CLEAR_BTN, "Clear console output")
        TOOLTIP(IDC_COPY_BTN, "Copy console output to the clipboard")
        TOOLTIP(IDC_RUN_BTN, "Run selected script")
        TOOLTIP(IDC_STOP_BTN, "Stop selected script")
        TOOLTIP(IDC_SCRIPTDIR_BTN, "Open scripts directory in file explorer")
    END_TOOLTIP_MAP()
};
