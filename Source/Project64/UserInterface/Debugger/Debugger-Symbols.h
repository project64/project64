#pragma once

#include "DebuggerUI.h"
#include "Symbols.h"

class CDebugSymbols :
    public CDebugDialog<CDebugSymbols>,
    public CDialogResize<CDebugSymbols>
{
private:
    enum
    {
        SymbolsListView_Col_Address,
        SymbolsListView_Col_Type,
        SymbolsListView_Col_Name,
        SymbolsListView_Col_Value,
        SymbolsListView_Col_Description,
        SymbolsListView_Num_Columns
    };

    struct SymbolCacheItem
    {
        int m_Id;
        wchar_t m_Address[16];
        wchar_t m_Type[16];
        wchar_t m_Name[48];
        wchar_t m_Value[64];
        wchar_t m_Description[256];

        SymbolCacheItem(CSymbol & symbol, CSymbolTable * symbolTable)
        {
            char szValue[64];
            symbolTable->GetValueString(szValue, &symbol);

            std::wstring strType = stdstr(symbol.TypeName()).ToUTF16();
            std::wstring strName = stdstr(symbol.m_Name).ToUTF16();
            std::wstring strValue = stdstr(szValue).ToUTF16();
            std::wstring strDesc = stdstr(symbol.m_Description).ToUTF16();

            m_Id = symbol.m_Id;
            wnsprintf(m_Address, sizeof(m_Address) / sizeof(wchar_t), L"%08X", symbol.m_Address);
            wcsncpy(m_Type, strType.c_str(), sizeof(m_Type) / sizeof(wchar_t));
            wcsncpy(m_Name, strName.c_str(), sizeof(m_Name) / sizeof(wchar_t));
            wcsncpy(m_Value, strValue.c_str(), sizeof(m_Value) / sizeof(wchar_t));
            wcsncpy(m_Description, strDesc.c_str(), sizeof(m_Description) / sizeof(wchar_t));
        }
    };

    static const CSetValueDlg::ComboItem ModalChangeTypeItems[];

    CSymbolTable * m_SymbolTable;
    CListViewCtrl m_SymbolsListView;
    CSetValueDlg m_SetValueDlg;
    CAddSymbolDlg m_AddSymbolDlg;

    size_t m_SymbolCacheStartIndex;
    std::vector<SymbolCacheItem> m_SymbolCache;

    bool m_bFiltering;
    wchar_t m_FilterText[64];
    std::vector<SymbolCacheItem> m_FilteredSymbols;

public:
    enum
    {
        IDD = IDD_Debugger_Symbols
    };
    enum
    {
        TIMER_ID_AUTO_REFRESH
    };

    CDebugSymbols(CDebuggerUI * debugger);
    //virtual ~CDebugSymbols(void);

    void Refresh();
    void UpdateFilteredSymbols();
    int GetListItemSymbolId(int nItem);
    int ColumnHitTest(POINT & pt);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
    LRESULT OnFilterChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
    LRESULT OnListDblClicked(NMHDR * pNMHDR);
    LRESULT OnListGetDispInfo(NMHDR * pNMHDR);
    LRESULT OnListCacheHint(NMHDR * pNMHDR);
    LRESULT OnDestroy(void);
    void OnExitSizeMove(void);
    void OnTimer(UINT_PTR nIDEvent);

    BEGIN_MSG_MAP_EX(CDebugSymbols)
    {
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked);
        COMMAND_HANDLER(IDC_FILTER_EDIT, EN_CHANGE, OnFilterChanged);
        NOTIFY_HANDLER_EX(IDC_SYMBOLS_LIST, NM_DBLCLK, OnListDblClicked);
        NOTIFY_HANDLER_EX(IDC_SYMBOLS_LIST, LVN_GETDISPINFO, OnListGetDispInfo);
        NOTIFY_HANDLER_EX(IDC_SYMBOLS_LIST, LVN_ODCACHEHINT, OnListCacheHint);
        MSG_WM_TIMER(OnTimer);
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove);
        MSG_WM_DESTROY(OnDestroy);
        CHAIN_MSG_MAP(CDialogResize<CDebugSymbols>);
    }
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CDebugSymbols)
    DLGRESIZE_CONTROL(IDC_FILTER_EDIT, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_FILTER_STATIC, DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_REMOVESYMBOL_BTN, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_ADDSYMBOL_BTN, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    DLGRESIZE_CONTROL(IDC_SYMBOLS_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()
};
