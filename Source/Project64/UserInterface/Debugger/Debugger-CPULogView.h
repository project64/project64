#pragma once
#include "DebuggerUI.h"

class CDebugCPULogView :
    public CDebugDialog<CDebugCPULogView>,
    public CDialogResize<CDebugCPULogView>,
    public CToolTipDialog<CDebugCPULogView>
{
public:
    enum { IDD = IDD_Debugger_CPULog };

    CDebugCPULogView(CDebuggerUI * debugger);
    virtual ~CDebugCPULogView(void);

    void RefreshList(bool bUpdateBuffer = true);

private:
    CCPULog*      m_CPULogCopy;

    int           m_RowHeight;
    int           m_LogStartIndex;

    CListViewCtrl m_CPUListView;
    CEdit         m_StateInfoEdit;
    CEditNumber32 m_BuffSizeEdit;
    CButton       m_EnabledChk;
    CScrollBar    m_Scrollbar;
    CButton       m_ExportBtn;

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);
    LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnListDblClicked(NMHDR* pNMHDR);
    LRESULT OnListItemChanged(NMHDR* pNMHDR);
    LRESULT OnMeasureItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
    LRESULT OnScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
    void    OnExitSizeMove(void);
    LRESULT OnDestroy(void);
    

    void InterceptMouseWheel(WPARAM wParam, LPARAM lParam);

    static CDebugCPULogView* _this;
    static HHOOK hWinMessageHook;
    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);

    void ToggleLoggingEnabled(void);
    void ShowRegStates(size_t stateIndex);
    void Export(void);

    int GetNumVisibleRows(CListViewCtrl& list);
    bool MouseHovering(WORD ctrlId, int xMargin = 0, int yMargin = 0);

    BEGIN_MSG_MAP_EX(CDebugCPULogView)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        NOTIFY_HANDLER_EX(IDC_CPU_LIST, NM_DBLCLK, OnListDblClicked)
        NOTIFY_HANDLER_EX(IDC_CPU_LIST, LVN_ITEMCHANGED, OnListItemChanged)
        MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
        MESSAGE_HANDLER(WM_VSCROLL, OnScroll)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
        CHAIN_MSG_MAP(CDialogResize<CDebugCPULogView>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugCPULogView)
        DLGRESIZE_CONTROL(IDC_CPU_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_REGSTATES_GRP, DLSZ_MOVE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_STATEINFO_EDIT, DLSZ_MOVE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_SCRL_BAR, DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_TOOLTIP_MAP()
        TOOLTIP(IDC_BUFFSIZE_EDIT, "Maximum number of states to keep (1024 = 416kB)") // sizeof(CPUState)
    END_TOOLTIP_MAP()
};
