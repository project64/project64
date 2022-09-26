#pragma once
#include "DebuggerUI.h"

class CDebugDMALogView :
    public CDebugDialog<CDebugDMALogView>,
    public CDialogResize<CDebugDMALogView>
{
public:
    enum
    {
        IDD = IDD_Debugger_DMALog
    };

    CDebugDMALogView(CDebuggerUI * debugger);
    virtual ~CDebugDMALogView(void);

    void RefreshDMALogWindow(bool bReset = false);

private:
    enum
    {
        WM_REFRESH = WM_USER + 1
    };

    void RefreshList(void);
    void Export(void);

    CDMALog * m_DMALog;

    int m_nLastStartIndex;
    bool m_bConvertingAddress;

    bool m_bUniqueRomAddresses;
    bool m_bFilterChanged;

    // Return true if entry meets requirements
    bool FilterEntry(int dmaLogIndex);

    CListViewCtrl m_DMAList;
    CEdit m_DMARamEdit;
    CEdit m_DMARomEdit;
    CStatic m_BlockInfo;

    bool m_bCustomDrawClrNext;

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnRefresh(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL & bHandled);
    LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnRamAddrChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled);
    LRESULT OnRomAddrChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled);
    LRESULT OnCustomDrawList(NMHDR * pNMHDR);
    LRESULT OnDestroy(void);
    void OnExitSizeMove(void);

    BEGIN_MSG_MAP_EX(CDebugDMALogView)
    {
        MESSAGE_HANDLER(WM_REFRESH, OnRefresh);
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked);
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate);
        COMMAND_HANDLER(IDC_DMA_RAM_EDIT, EN_CHANGE, OnRamAddrChanged);
        COMMAND_HANDLER(IDC_DMA_ROM_EDIT, EN_CHANGE, OnRomAddrChanged);
        NOTIFY_HANDLER_EX(IDC_DMA_LIST, NM_CUSTOMDRAW, OnCustomDrawList);
        CHAIN_MSG_MAP(CDialogResize<CDebugDMALogView>);
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove);
    }
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugDMALogView)
    DLGRESIZE_CONTROL(IDC_DMA_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    DLGRESIZE_CONTROL(IDC_TRACE_STATIC, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_BLOCK_INFO, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_RAM_STATIC, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_ROM_STATIC, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_DMA_ROM_EDIT, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_DMA_RAM_EDIT, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_CLEAR_BTN, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_EXPORT_BTN, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()
};
