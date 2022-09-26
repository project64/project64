#pragma once

#include "Debugger-AddSymbol.h"
#include "MemoryScanner.h"

class CEditMixed :
    public CWindowImpl<CEditMixed, CEdit>,
    public CMixed
{
public:
    CEditMixed(void);
    ~CEditMixed(void);

    BOOL Attach(HWND hWndNew);

    DisplayFormat GetDisplayFormat(void);
    void SetDisplayFormat(DisplayFormat fmt);

    bool GetValue(uint8_t & value);
    bool GetValue(int8_t & value);
    bool GetValue(uint16_t & value);
    bool GetValue(int16_t & value);
    bool GetValue(uint32_t & value);
    bool GetValue(int32_t & value);
    bool GetValue(uint64_t & value);
    bool GetValue(int64_t & value);
    bool GetValue(float & value);
    bool GetValue(double & value);
    bool GetValueString(const wchar_t *& value, int & length);
    bool GetValueHexString(const wchar_t *& value, int & length);

    BEGIN_MSG_MAP_EX(CEditMixed)
    END_MSG_MAP()

private:
    ValueType m_Type;
    DisplayFormat m_DisplayFormat;
    wchar_t * m_String;
    int m_StringLength;
    void ReloadString(void);
};

class CSetValueDlg : public CDialogImpl<CSetValueDlg>
{
public:
    BEGIN_MSG_MAP_EX(CSetValueDlg)
    {
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
        COMMAND_HANDLER(IDOK, BN_CLICKED, OnOK);
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel);
        MSG_WM_DESTROY(OnDestroy);
    }
    END_MSG_MAP()

    enum
    {
        IDD = IDD_Debugger_Search_SetValue
    };

    typedef struct
    {
        const char * str;
        DWORD_PTR data;
    } ComboItem;

    CSetValueDlg(void);
    virtual ~CSetValueDlg(void);

    INT_PTR DoModal(const char * caption, const char * label, const char * initialText);
    INT_PTR DoModal(const char * caption, const char * label, DWORD_PTR initialData, const ComboItem items[]);
    const std::string & GetEnteredString(void);
    DWORD_PTR GetEnteredData(void);

private:
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnDestroy(void);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);

    enum Mode
    {
        Mode_TextBox,
        Mode_ComboBox
    };

    Mode m_Mode;

    const char * m_Caption;
    const char * m_Label;
    const ComboItem * m_ComboItems;

    const char * m_InitialText;
    std::string m_EnteredString;

    DWORD_PTR m_InitialData;
    DWORD_PTR m_EnteredData;

    CStatic m_Prompt;
    CEdit m_Value;
    CComboBox m_CmbValue;
};

class CDebugMemorySearch :
    public CDebugDialog<CDebugMemorySearch>,
    public CDialogResize<CDebugMemorySearch>
{
public:
    enum
    {
        IDD = IDD_Debugger_Search
    };

    CDebugMemorySearch(CDebuggerUI * debugger);
    virtual ~CDebugMemorySearch(void);

    void GameReset(void);

private:
    enum
    {
        WM_GAMERESET = WM_USER + 1
    };

    enum
    {
        TIMER_ID_AUTO_REFRESH
    };

    enum
    {
        GS_LINE_LEN = (sizeof("00000000 0000\n") - 1),
        GS_TWOBYTE = 0x01000000
    };

    enum
    {
        WatchListCtrl_Col_Lock,
        WatchListCtrl_Col_BP,
        WatchListCtrl_Col_Address,
        WatchListCtrl_Col_Description,
        WatchListCtrl_Col_Type,
        WatchListCtrl_Col_Value,
        WatchListCtrl_Num_Columns
    };

    enum
    {
        ResultsListCtrl_Col_Address,
        ResultsListCtrl_Col_Value,
        ResultsListCtrl_Col_Previous
    };

    CDebugMemorySearch(void);
    CDebugMemorySearch(const CDebugMemorySearch &);
    CDebugMemorySearch & operator=(const CDebugMemorySearch &);

    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static CDebugMemorySearch * _this;
    static HHOOK hWinMessageHook;

    static const CSetValueDlg::ComboItem ModalChangeTypeItems[];

    LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnDestroy(void);
    LRESULT OnGameReset(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    void OnExitSizeMove(void);
    void OnSizing(UINT fwSide, LPRECT pRect);
    void OnTimer(UINT_PTR nIDEvent);
    LRESULT OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnMouseDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnMouseUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    void OnInterceptMouseWheel(WPARAM wParam, LPARAM lParam);
    void OnInterceptMouseMove(WPARAM wParam, LPARAM lParam);
    LRESULT OnMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnHexCheckbox(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnSearchButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnResetButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnRdramButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnRomButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnSpmemButton(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnScanTypeChanged(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnValueTypeChanged(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnResultsCustomDraw(LPNMHDR lpnmh);
    LRESULT OnResultsDblClick(LPNMHDR lpnmh);
    LRESULT OnResultsRClick(LPNMHDR lpnmh);
    LRESULT OnResultsPopupViewMemory(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnResultsPopupAddToWatchList(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnResultsPopupAddAllToWatchList(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnResultsPopupSetValue(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnResultsPopupRemove(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListItemChanged(LPNMHDR lpnmh);
    LRESULT OnWatchListCustomDraw(LPNMHDR lpnmh);
    LRESULT OnWatchListRClick(LPNMHDR lpnmh);
    LRESULT OnWatchListDblClick(LPNMHDR lpnmh);
    LRESULT OnWatchListPopupLock(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupReadBP(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupWriteBP(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupViewMemory(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupAddSymbol(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupHexadecimal(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupChangeValue(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupChangeDescription(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupChangeType(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupChangeAddress(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupChangeAddressBy(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupRemove(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupRemoveAll(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupCopyGamesharkCode(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);
    LRESULT OnWatchListPopupCopyAddressAndDescription(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL & bHandled);

    CScanResult * GetFirstSelectedScanResult(void);
    CScanResult * GetFirstSelectedWatchListResult(void);

    void ResetResults(void);
    void Search(void);
    void UpdateOptions(void);
    void UpdateResultsList(bool bUpdateScrollbar = false, bool bResetScrollPos = true);
    void UpdateWatchList(bool bUpdateScrollbar = false);
    void RefreshResultsListValues(void);
    void RefreshWatchListValues(void);
    void RemoveWatchListItem(int index);
    void ClearWatchList(void);
    void ReloadWatchList(void);
    void FlushWatchList(void);
    void LoadWatchList(void);
    void RemoveSelectedWatchListItems(void);
    void AddResultToWatchList(int resultIndex);

    void SeparatorMoveCtrl(WORD ctrlId, int yChange, bool bResize);

    CPath GetWatchListPath(void);

    // Generic UI utility
    void FixListHeader(CListViewCtrl & listCtrl);
    void SetComboBoxSelByData(CComboBox & cb, DWORD_PTR data);
    bool MouseHovering(WORD ctrlId, int hMargin = 0, int vMargin = 0);
    int GetNumVisibleRows(CListViewCtrl & list);

    bool m_bJalSelected;
    bool m_bJalHexWasChecked;
    bool m_bJalUnsignedWasChecked;

    stdstr m_StrGame;
    DWORD m_ThreadId;
    CFile m_WatchListFile;

    CSetValueDlg m_SetValueDlg;
    CAddSymbolDlg m_AddSymbolDlg;

    CEditMixed m_SearchValue;
    CEditNumber32 m_AddrStart, m_AddrEnd;
    CComboBox m_SearchTypeOptions, m_ValueTypeOptions;
    CListViewCtrl m_ResultsListCtrl, m_WatchListCtrl;
    CScrollBar m_ResultsScrollbar, m_WatchListScrollbar;
    CButton m_PhysicalCheckbox, m_HexCheckbox;
    CButton m_UnsignedCheckbox, m_IgnoreCaseCheckbox, m_UnkEncodingCheckbox;

    CMemoryScanner m_MemoryScanner;
    std::vector<CScanResult> m_WatchList;

    bool m_bDraggingSeparator;
    CRect m_InitialSeparatorRect, m_LastSeparatorRect;

    int m_ListCtrlRowHeight;

    HCURSOR m_hCursorSizeNS;

    BEGIN_MSG_MAP_EX(CDebugMemorySearch)
    {
        MESSAGE_HANDLER(WM_GAMERESET, OnGameReset);
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
        MSG_WM_DESTROY(OnDestroy);
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove);
        MSG_WM_SIZING(OnSizing);
        MSG_WM_TIMER(OnTimer);
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel);
        MESSAGE_HANDLER(WM_VSCROLL, OnScroll);
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseDown);
        MESSAGE_HANDLER(WM_LBUTTONUP, OnMouseUp);
        MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem);
        MESSAGE_HANDLER(WM_SETFONT, OnSetFont);
        COMMAND_HANDLER(IDC_CHK_HEX, BN_CLICKED, OnHexCheckbox);
        COMMAND_HANDLER(IDC_BTN_SEARCH, BN_CLICKED, OnSearchButton);
        COMMAND_HANDLER(IDC_BTN_RESET, BN_CLICKED, OnResetButton);
        COMMAND_HANDLER(IDC_CMB_SCANTYPE, CBN_SELCHANGE, OnScanTypeChanged);
        COMMAND_HANDLER(IDC_CMB_VALUETYPE, CBN_SELCHANGE, OnValueTypeChanged);
        COMMAND_HANDLER(IDC_BTN_RDRAM, BN_CLICKED, OnRdramButton);
        COMMAND_HANDLER(IDC_BTN_ROM, BN_CLICKED, OnRomButton);
        COMMAND_HANDLER(IDC_BTN_SPMEM, BN_CLICKED, OnSpmemButton);
        NOTIFY_HANDLER_EX(IDC_LST_RESULTS, NM_CUSTOMDRAW, OnResultsCustomDraw);
        NOTIFY_HANDLER_EX(IDC_LST_RESULTS, NM_DBLCLK, OnResultsDblClick);
        NOTIFY_HANDLER_EX(IDC_LST_RESULTS, NM_RCLICK, OnResultsRClick);
        COMMAND_HANDLER(ID_RESULTS_VIEWMEMORY, BN_CLICKED, OnResultsPopupViewMemory);
        COMMAND_HANDLER(ID_RESULTS_ADDTOWATCHLIST, BN_CLICKED, OnResultsPopupAddToWatchList);
        COMMAND_HANDLER(ID_RESULTS_ADDALLTOWATCHLIST, BN_CLICKED, OnResultsPopupAddAllToWatchList);
        COMMAND_HANDLER(ID_RESULTS_CHANGEVALUE, BN_CLICKED, OnResultsPopupSetValue);
        COMMAND_HANDLER(ID_RESULTS_REMOVE, BN_CLICKED, OnResultsPopupRemove);
        NOTIFY_HANDLER_EX(IDC_LST_WATCHLIST, LVN_ITEMCHANGED, OnWatchListItemChanged);
        NOTIFY_HANDLER_EX(IDC_LST_WATCHLIST, NM_CUSTOMDRAW, OnWatchListCustomDraw);
        NOTIFY_HANDLER_EX(IDC_LST_WATCHLIST, NM_DBLCLK, OnWatchListDblClick);
        NOTIFY_HANDLER_EX(IDC_LST_WATCHLIST, NM_RCLICK, OnWatchListRClick);
        COMMAND_HANDLER(ID_WATCHLIST_VIEWMEMORY, BN_CLICKED, OnWatchListPopupViewMemory);
        COMMAND_HANDLER(ID_WATCHLIST_LOCKVALUE, BN_CLICKED, OnWatchListPopupLock);
        COMMAND_HANDLER(ID_WATCHLIST_READBP, BN_CLICKED, OnWatchListPopupReadBP);
        COMMAND_HANDLER(ID_WATCHLIST_WRITEBP, BN_CLICKED, OnWatchListPopupWriteBP);
        COMMAND_HANDLER(ID_WATCHLIST_ADDSYMBOL, BN_CLICKED, OnWatchListPopupAddSymbol);
        COMMAND_HANDLER(ID_WATCHLIST_HEXADECIMAL, BN_CLICKED, OnWatchListPopupHexadecimal);
        COMMAND_HANDLER(ID_WATCHLIST_CHANGE_VALUE, BN_CLICKED, OnWatchListPopupChangeValue);
        COMMAND_HANDLER(ID_WATCHLIST_CHANGE_DESCRIPTION, BN_CLICKED, OnWatchListPopupChangeDescription);
        COMMAND_HANDLER(ID_WATCHLIST_CHANGE_TYPE, BN_CLICKED, OnWatchListPopupChangeType);
        COMMAND_HANDLER(ID_WATCHLIST_CHANGE_ADDRESS, BN_CLICKED, OnWatchListPopupChangeAddress);
        COMMAND_HANDLER(ID_WATCHLIST_CHANGE_ADDRESSBY, BN_CLICKED, OnWatchListPopupChangeAddressBy);
        COMMAND_HANDLER(ID_WATCHLIST_REMOVE, BN_CLICKED, OnWatchListPopupRemove);
        COMMAND_HANDLER(ID_WATCHLIST_REMOVEALL, BN_CLICKED, OnWatchListPopupRemoveAll);
        COMMAND_HANDLER(ID_WATCHLIST_COPY_GSCODE, BN_CLICKED, OnWatchListPopupCopyGamesharkCode);
        COMMAND_HANDLER(ID_WATCHLIST_COPY_ADDRDESC, BN_CLICKED, OnWatchListPopupCopyAddressAndDescription);
        CHAIN_MSG_MAP(CDialogResize<CDebugMemorySearch>);
    }
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugMemorySearch)
    DLGRESIZE_CONTROL(IDC_LST_WATCHLIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    DLGRESIZE_CONTROL(IDC_SCRL_WATCHLIST, DLSZ_MOVE_X | DLSZ_SIZE_Y)
    DLGRESIZE_CONTROL(IDC_NUM_RESULTS, DLSZ_SIZE_X)
    DLGRESIZE_CONTROL(IDC_LST_RESULTS, DLSZ_SIZE_X)
    DLGRESIZE_CONTROL(IDC_SCRL_RESULTS, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_SEPARATOR, DLSZ_SIZE_X)
    DLGRESIZE_CONTROL(IDC_BTN_SEARCH, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_BTN_RESET, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_GRP_SEARCH, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_CHK_HEX, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_CHK_UNSIGNED, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_CHK_IGNORECASE, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_CHK_UNKENCODING, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_SEARCH_VALUE, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_LBL_SCANTYPE, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_LBL_VALUETYPE, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_CMB_SCANTYPE, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_CMB_VALUETYPE, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_LBL_ADDRSTART, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_LBL_ADDREND, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_ADDR_START, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_ADDR_END, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_CHK_PHYSICAL, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_BTN_RDRAM, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_BTN_ROM, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_BTN_SPMEM, DLSZ_MOVE_X)
    DLGRESIZE_CONTROL(IDC_GRP_ADDR, DLSZ_MOVE_X)
    END_DLGRESIZE_MAP()
};
