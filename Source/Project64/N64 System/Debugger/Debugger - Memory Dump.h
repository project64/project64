class CDumpMemory :
	public CDebugDialog<CDumpMemory>
{

	enum DumpFormat
	{
		DisassemblyWithPC	
	};

	BEGIN_MSG_MAP_EX(CDumpMemory)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_CODE_HANDLER(BN_CLICKED,OnClicked)
	END_MSG_MAP()

	LRESULT				OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT				OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);
	
	bool DumpMemory ( LPCSTR FileName,DumpFormat Format, DWORD StartPC, DWORD EndPC, DWORD DumpPC );

	CEditNumber m_StartAddress, m_EndAddress, m_PC;
	CN64System * const m_System;
	
public: 
	enum { IDD = IDD_Cheats_DumpMemory };

	CDumpMemory(CN64System * System, CMipsMemory * MMU, CDebugger * debugger);
	virtual ~CDumpMemory(void);

};
//class CDumpMemory 
//{
//	enum DumpFormat
//	{
//		DisassemblyWithPC	
//	};
//	CMipsMemory * _MMU;
//	WND_HANDLE   m_Window;
//	WORD SelStart, SelStop;
//	static DWORD m_StartAddress;
//	static DWORD m_EndAddress;
//	
//	bool DumpMemory(LPCSTR FileName,DumpFormat Format, DWORD StartPC, DWORD EndPC, DWORD DumpPC);
//
//	static DWORD AsciiToHex ( const char * HexValue );
//	static int CALLBACK WinProc (WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam);
//
//
//public: 
//	CDumpMemory(CMipsMemory * MMU);
//	~CDumpMemory(void);
//
//	void DisplayDump ( WND_HANDLE & hParent );
//};
