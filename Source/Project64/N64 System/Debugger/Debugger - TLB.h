class CDebugTlb :
	public CDebugDialog<CDebugTlb>
{

	BEGIN_MSG_MAP_EX(CDebugTlb)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_CODE_HANDLER(BN_CLICKED,OnClicked)
	END_MSG_MAP()

	LRESULT				OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT				OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);

public: 
	enum { IDD = IDD_Debugger_TLB };

	CDebugTlb(CDebugger * debugger);
	virtual ~CDebugTlb(void);

	void RefreshTLBWindow ( void );
};

//class CDebugTlb
//{
//	CTLB * g_TLB;
//
//	//Debugger
//	WND_HANDLE m_hDebugWnd;
//	void SetupDebugWindow (void);
//	static void CreateDebugWindow ( CDebugTlb * _this );
//	friend DWORD CALLBACK DebugWndProc ( WND_HANDLE, DWORD, DWORD, DWORD );
//
//public: 
//	CDebugTlb(CTLB * g_TLB);
//	~CDebugTlb(void);
//
//	//debugger function
//	void ShowTLBWindow    ( void );
//	void RefreshTLBWindow ( void );
//};
