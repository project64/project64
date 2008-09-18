class CDebugMemorySearch :
	public CDebugDialog<CDebugMemorySearch>
{
	typedef struct {
		DWORD PAddr;
		DWORD Value;
	} SearchResultItem;
	
	typedef std::vector<SearchResultItem> SearchResult;
	
	BEGIN_MSG_MAP_EX(CDebugMemorySearch)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_CODE_HANDLER(BN_CLICKED,OnClicked)
		NOTIFY_HANDLER_EX(IDC_LST_RESULTS,NM_RCLICK,OnResultRClick)
	END_MSG_MAP()

	LRESULT				OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT				OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);
	LRESULT             OnResultRClick ( LPNMHDR lpnmh );

	void EnableUnknownOptions ( bool Enable );
	void EnableValueOptions   ( bool Enable );
	void EnableTextOptions    ( bool Enable );
	void EnableJalOptions     ( bool Enable );
	void AddAlignmentOptions  ( CComboBox  & ctrl );
	
	CEditNumber   m_PAddrStart, m_SearchLen, m_SearchValue, m_MaxSearch;
	CComboBox     m_UnknownOptions, m_ValueSize, m_UnknownSize;
	CListViewCtrl m_SearchResults;
	SearchResult  m_SearchResult;
	bool          m_HaveResults;
	CN64System  * m_System;
	
	void FixUnknownOptions ( bool Reset );
	void SearchForUnknown  ( void );
	void SearchForValue    ( void );
	void SearchForText     ( void );
	void Reset             ( void );

public: 
	enum { IDD = IDD_Debugger_Search };

	CDebugMemorySearch(CN64System * System, CMipsMemory * MMU, CDebugger * debugger);
	virtual ~CDebugMemorySearch(void);

};
