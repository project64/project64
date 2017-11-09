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
    vector<char*> m_History;
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
    public CDebugDialog < CDebugScripts >,
    public CDialogResize<CDebugScripts>
{
private:
    CEdit m_InstanceInfoEdit;
    CEditEval m_EvalEdit;
    CEditConsole m_ConsoleEdit;
    CScriptList m_ScriptList;
    char* m_SelectedScriptName;

    void RefreshStatus();

public:
    enum { IDD = IDD_Debugger_Scripts };

    CDebugScripts(CDebuggerUI * debugger);
    virtual ~CDebugScripts(void);

    void ConsolePrint(const char* text);
    void ConsoleClear();
    void ConsoleCopy();

    void RefreshList();
    void RefreshConsole();

    void EvaluateInSelectedInstance(char* code);
    void RunSelected();
    void StopSelected();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(void)
    {
        return 0;
    }

    LRESULT OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnScriptListDblClicked(NMHDR* pNMHDR);
    LRESULT OnScriptListClicked(NMHDR* pNMHDR);
    LRESULT OnScriptListRClicked(NMHDR* pNMHDR);
    LRESULT OnScriptListCustomDraw(NMHDR* pNMHDR);

    BEGIN_MSG_MAP_EX(CDebugScripts)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_DBLCLK, OnScriptListDblClicked)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_CLICK, OnScriptListClicked)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_RCLICK, OnScriptListRClicked)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_CUSTOMDRAW, OnScriptListCustomDraw)
        CHAIN_MSG_MAP_MEMBER(m_ScriptList)
        MSG_WM_DESTROY(OnDestroy)
        CHAIN_MSG_MAP(CDialogResize<CDebugScripts>)
        END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugScripts)
        DLGRESIZE_CONTROL(IDC_CTX_INFO_EDIT, DLSZ_SIZE_X)
        DLGRESIZE_CONTROL(IDC_EVAL_EDIT, DLSZ_SIZE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_CONSOLE_EDIT, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_SCRIPT_LIST, DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_CLEAR_BTN, DLSZ_MOVE_X)
        DLGRESIZE_CONTROL(IDC_COPY_BTN, DLSZ_MOVE_X)
    END_DLGRESIZE_MAP()
};