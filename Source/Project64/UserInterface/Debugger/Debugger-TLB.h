#pragma once

class CDebugTlb :
    public CDebugDialog < CDebugTlb >
{
    BEGIN_MSG_MAP_EX(CDebugTlb)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);
    void OnExitSizeMove(void);

public:
    enum { IDD = IDD_Debugger_TLB };

    CDebugTlb(CDebuggerUI * debugger);
    virtual ~CDebugTlb(void);

    void RefreshTLBWindow(void);
};
