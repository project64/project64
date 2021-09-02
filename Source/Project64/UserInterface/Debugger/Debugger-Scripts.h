#pragma once
#include "DebuggerUI.h"
#include "ScriptSystem.h"
#include <Project64/UserInterface/WTLControls/EditConInput.h>
#include <Project64/UserInterface/WTLControls/TooltipDialog.h>
#include "Debugger-ScriptsAutorun.h"
#include <string>
#include <vector>

class CDebugScripts :
    public CDebugDialog<CDebugScripts>,
    public CDialogResize<CDebugScripts>,
    public CToolTipDialog<CDebugScripts>
{
private:
    enum {
        WM_REFRESH_LIST = WM_USER + 1,
        WM_SCRIPT_STATUS = WM_USER + 2,
        WM_CONSOLE_PRINT = WM_USER + 3,
        WM_CONSOLE_CLEAR = WM_USER + 4
    };

    enum {
        CONFLUSH_TIMER_ID = 0,
        CONFLUSH_TIMER_INTERVAL = 50
    };

    CScriptsAutorunDlg m_AutorunDlg;

    CEditConInput m_ConInputEdit;
    CEditConOutput m_ConOutputEdit;
    CListViewCtrl m_ScriptList;
    CStatusBarCtrl m_StatusBar;

    HFONT m_MonoFont, m_MonoBoldFont;

    stdstr m_InstallDir;
    stdstr m_ScriptsDir;

    stdstr m_SelectedScriptName;
    std::vector<wchar_t*> m_InputHistory;
    size_t m_InputHistoryIndex;

    stdstr m_ConOutputBuffer;

    HANDLE m_hQuitScriptDirWatchEvent;
    HANDLE m_hScriptDirWatchThread;
    static DWORD WINAPI ScriptDirWatchProc(void *ctx);

    void RunSelected();
    void StopSelected();
    void ToggleSelected();
    void EditSelected();
    void RefreshStatus();
    void ConsoleCopy();

    void SendInput(const char* name, const char* code);

public:
    enum { IDD = IDD_Debugger_Scripts };

    CDebugScripts(CDebuggerUI * debugger);
    virtual ~CDebugScripts(void);

    void ConsolePrint(const char* text);
    void ConsoleClear();
    void RefreshList();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtlColorEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(void);
    LRESULT OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnScriptListDblClicked(NMHDR* pNMHDR);
    LRESULT OnScriptListRClicked(NMHDR* pNMHDR);
    LRESULT OnScriptListCustomDraw(NMHDR* pNMHDR);
    LRESULT OnScriptListItemChanged(NMHDR* pNMHDR);
    LRESULT OnInputSpecialKey(NMHDR* pNMHDR);
    void OnExitSizeMove(void);
    void OnTimer(UINT_PTR nIDEvent);

    LRESULT OnConsolePrint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnConsoleClear(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRefreshList(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BEGIN_MSG_MAP_EX(CDebugScripts)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnCtlColorEdit)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_DBLCLK, OnScriptListDblClicked)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_RCLICK, OnScriptListRClicked)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_CUSTOMDRAW, OnScriptListCustomDraw)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, LVN_ITEMCHANGED, OnScriptListItemChanged)
        NOTIFY_HANDLER_EX(IDC_EVAL_EDIT, CIN_SPECIALKEY, OnInputSpecialKey)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove);
        MESSAGE_HANDLER(WM_CONSOLE_PRINT, OnConsolePrint)
        MESSAGE_HANDLER(WM_CONSOLE_CLEAR, OnConsoleClear)
        MESSAGE_HANDLER(WM_REFRESH_LIST, OnRefreshList)
        CHAIN_MSG_MAP(CDialogResize<CDebugScripts>)
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
        DLGRESIZE_CONTROL(IDC_SCRIPTDIR_BTN, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_STATUSBAR, DLSZ_SIZE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_TOOLTIP_MAP()
        TOOLTIP(IDC_CLEAR_BTN, "Clear console output")
        TOOLTIP(IDC_COPY_BTN, "Copy console output to the clipboard")
        TOOLTIP(IDC_RUN_BTN, "Toggle selected script")
        TOOLTIP(IDC_SCRIPTDIR_BTN, "Open scripts directory in file explorer")
    END_TOOLTIP_MAP()
};
