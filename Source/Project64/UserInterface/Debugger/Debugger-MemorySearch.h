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

class CDebugMemorySearch :
    public CDebugDialog < CDebugMemorySearch >
{
public:
    enum { IDD = IDD_Debugger_Search };

    CDebugMemorySearch(CDebuggerUI * debugger);
    virtual ~CDebugMemorySearch(void);

private:
    CDebugMemorySearch(void);									// Disable default constructor
    CDebugMemorySearch(const CDebugMemorySearch&);				// Disable copy constructor
    CDebugMemorySearch& operator=(const CDebugMemorySearch&);	// Disable assignment

    enum MemorySize
    {
        _8Bit,
        _16Bit,
        _32Bit,
    };

    //Searching for value
    enum SearchMemChangeState
    {
        SearchChangeState_Reset,
        SearchChangeState_Changed,
        SearchChangeState_Unchanged,
        SearchChangeState_Greater,
        SearchChangeState_Lessthan,
    };

    struct SearchResultItem
    {
        DWORD PAddr;
        DWORD Value;
    };

    typedef std::vector<SearchResultItem> SearchResult;

    BEGIN_MSG_MAP_EX(CDebugMemorySearch)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        NOTIFY_HANDLER_EX(IDC_LST_RESULTS, NM_RCLICK, OnResultRClick)
        NOTIFY_HANDLER_EX(IDC_LST_RESULTS, NM_DBLCLK, OnResultDblClick)
    END_MSG_MAP()

    LRESULT				OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT				OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);
    LRESULT             OnResultRClick(LPNMHDR lpnmh);
    LRESULT             OnResultDblClick(LPNMHDR lpnmh);

    void EnableUnknownOptions(bool Enable);
    void EnableValueOptions(bool Enable);
    void EnableTextOptions(bool Enable);
    void EnableJalOptions(bool Enable);
    void AddAlignmentOptions(CComboBox  & ctrl);

    CEditNumber   m_PAddrStart, m_SearchLen, m_SearchValue, m_MaxSearch;
    CComboBox     m_UnknownOptions, m_ValueSize, m_UnknownSize;
    CListViewCtrl m_SearchResults;
    SearchResult  m_SearchResult;
    bool          m_HaveResults;

    //Searching memory
    BYTE  *       m_MemoryState;
    DWORD         m_MemoryStateSize;

    void FixUnknownOptions(bool Reset);
    void SearchForUnknown(void);
    void SearchForValue(void);
    void SearchForText(void);
    void Reset(void);
    bool SearchSetBaseForChanges(void);
    bool SearchForChanges(SearchMemChangeState SearchType, MemorySize Size, DWORD &StartAddress, DWORD &Len, DWORD &OldValue, DWORD &NewValue);
    bool SearchForValue(DWORD Value, MemorySize Size, DWORD &StartAddress, DWORD &Len);
};
