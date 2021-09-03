#pragma once
#include <unordered_set>

class CScriptsAutorunDlg :
    public CDialogImpl<CScriptsAutorunDlg>
{
public:
    enum { IDD = IDD_Debugger_ScriptsAutorun };

    CScriptsAutorunDlg();
    virtual ~CScriptsAutorunDlg();

    INT_PTR DoModal(CDebuggerUI* debugger, stdstr selectedScriptName);

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(void);
    LRESULT OnOKCancel(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled);
    LRESULT OnScriptListDblClicked(NMHDR* pNMHDR);
    LRESULT OnAutorunListDblClicked(NMHDR* pNMHDR);
    LRESULT OnCtrlSetFocus(NMHDR* pNMHDR);
    LRESULT OnRefreshScriptList(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRefreshAutorunList(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BEGIN_MSG_MAP_EX(CAddBreakpointDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MSG_WM_DESTROY(OnDestroy)
        COMMAND_HANDLER(IDOK, BN_CLICKED, OnOKCancel)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnOKCancel)
        COMMAND_HANDLER(IDC_ADD_BTN, BN_CLICKED, OnAdd)
        COMMAND_HANDLER(IDC_REMOVE_BTN, BN_CLICKED, OnRemove)
        NOTIFY_HANDLER_EX(IDC_SCRIPT_LIST, NM_DBLCLK, OnScriptListDblClicked)
        NOTIFY_HANDLER_EX(IDC_AUTORUN_LIST, NM_DBLCLK, OnAutorunListDblClicked)
        NOTIFY_CODE_HANDLER_EX(NM_SETFOCUS, OnCtrlSetFocus)
        MESSAGE_HANDLER(WM_REFRESH_LIST, OnRefreshScriptList)
        MESSAGE_HANDLER(WM_REFRESH_AUTORUN_LIST, OnRefreshAutorunList)
    END_MSG_MAP()

private:
    enum
    {
        WM_REFRESH_LIST = WM_USER + 1,
        WM_REFRESH_AUTORUN_LIST = WM_USER + 2
    };
    
    CDebuggerUI* m_Debugger;
    CScriptSystem* m_ScriptSystem;

    stdstr m_InitSelectedScriptName;
    //std::set<std::string> m_AutorunSet;

    bool m_bScriptListNeedsRefocus;
    bool m_bAutorunListNeedsRefocus;

    CListViewCtrl m_ScriptListView;
    CListViewCtrl m_AutorunListView;

    HANDLE m_hQuitScriptDirWatchEvent;
    HANDLE m_hScriptDirWatchThread;
    static DWORD WINAPI ScriptDirWatchProc(void* ctx);

    void AddSelected();
    void RemoveSelected();
    void RefreshScriptList();
    void RefreshAutorunList();
    //void LoadAutorunSet();
    //void SaveAutorunSet();
};
