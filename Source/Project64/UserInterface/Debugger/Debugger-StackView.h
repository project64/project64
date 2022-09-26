#pragma once

class CDebugStackView :
    public CDebugDialog<CDebugStackView>,
    public CDialogResize<CDebugStackView>
{
public:
    enum
    {
        IDD = IDD_Debugger_Stack
    };

    CDebugStackView(CDebuggerUI * debugger);
    virtual ~CDebugStackView(void);

    void Refresh();

private:
    CListViewCtrl m_StackList;
    CStatic m_SPStatic;

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnDestroy(void);
    LRESULT OnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled);
    void OnExitSizeMove(void);

    BEGIN_MSG_MAP_EX(CDebugStackView)
    {
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
        MSG_WM_DESTROY(OnDestroy);
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked);
        CHAIN_MSG_MAP(CDialogResize<CDebugStackView>);
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove);
    }
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugStackView)
    DLGRESIZE_CONTROL(IDC_STACK_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()
};
