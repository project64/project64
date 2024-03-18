#pragma once

class CCopyCommandsDlg : public CDialogImpl<CCopyCommandsDlg>
{
public:
    enum
    {
        IDD = IDD_Debugger_CopyCommands
    };

    INT_PTR DoModal(CDebuggerUI * debugger);
    INT_PTR DoModal(CDebuggerUI * debugger, uint32_t initAddress);
    INT_PTR DoModal(CDebuggerUI * debugger, uint32_t initAddress, uint32_t initCount);

private:
    CDebuggerUI * m_Debugger;

    bool m_bHaveAddress;
    bool m_bHaveCount;
    uint32_t m_InitAddress;
    uint32_t m_InitCount;

    CEditNumber32 m_AddressEdit;
    CEditNumber32 m_CountEdit;

    LRESULT OnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled);
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnDestroy(void)
    {
        return 0;
    }

    BEGIN_MSG_MAP_EX(CCopyCommandsDlg)
    {
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked);
        MSG_WM_DESTROY(OnDestroy);
    }
    END_MSG_MAP()
};
