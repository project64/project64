#include "Debugger UI.h"

CDumpMemory::CDumpMemory(CDebugger * debugger) :
	CDebugDialog<CDumpMemory>(debugger)
{
}

CDumpMemory::~CDumpMemory()
{
}

LRESULT	CDumpMemory::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_StartAddress.Attach(GetDlgItem(IDC_E_START_ADDR));
	m_EndAddress.Attach(GetDlgItem(IDC_E_END_ADDR));
	m_PC.Attach(GetDlgItem(IDC_E_ALT_PC));

	m_StartAddress.SetDisplayType(CEditNumber::DisplayHex);
	m_EndAddress.SetDisplayType(CEditNumber::DisplayHex);
	m_PC.SetDisplayType(CEditNumber::DisplayHex);

	m_StartAddress.SetValue(0x80000000,true,true);
	m_EndAddress.SetValue(0x803FFFF0,true,true);
	m_PC.SetValue(0x80000000);
	HWND hFormatList = GetDlgItem(IDC_FORMAT);
	int pos = ::SendMessage(hFormatList,CB_ADDSTRING,(WPARAM)0,(LPARAM)"TEXT - Disassembly + PC");
	::SendMessage(hFormatList,CB_SETITEMDATA,(WPARAM)pos,(LPARAM)DisassemblyWithPC);
	::SendMessage(hFormatList,CB_SETCURSEL,(WPARAM)0,(LPARAM)0);

	WindowCreated();
	return TRUE;
}

LRESULT	CDumpMemory::OnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	switch(wID)
	{
	case IDCANCEL:
		EndDialog(0);
		break;
	case IDC_BTN_CHOOSE_FILE:
		{
			char FileName[_MAX_PATH],Directory[_MAX_PATH];
			OPENFILENAME openfilename;

			memset(&FileName, 0, sizeof(FileName));
			memset(&openfilename, 0, sizeof(openfilename));
			strcpy(Directory,CPath(CPath::MODULE_DIRECTORY));
			openfilename.lStructSize  = sizeof( openfilename );
			openfilename.hwndOwner    = m_hWnd;
			openfilename.lpstrFilter  = "Text file (*.txt)\0*.txt;\0All files (*.*)\0*.*\0";
			openfilename.lpstrFile    = FileName;
			openfilename.lpstrInitialDir    = Directory;
			openfilename.nMaxFile     = MAX_PATH;
			openfilename.Flags        = OFN_HIDEREADONLY;
			_N64System->ExternalEvent(PauseCPU_DumpMemory); 
			if (GetOpenFileName (&openfilename)) 
			{							
				char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
				_splitpath( FileName, drive, dir, fname, ext );
				if (strlen(ext) == 0)
				{
                    strcat(FileName,".txt");
				}
				SetDlgItemText(IDC_FILENAME,FileName);
			}	
			_N64System->ExternalEvent(ResumeCPU_DumpMemory); 
		}
		break;
	case IDOK:
		{
			TCHAR FileName[MAX_PATH];
			int CurrentFormatSel = SendDlgItemMessage(IDC_FORMAT,CB_GETCURSEL,0,0);
			DumpFormat Format = (DumpFormat)SendDlgItemMessage(IDC_FORMAT,CB_GETITEMDATA,CurrentFormatSel,0);
			DWORD StartPC =m_StartAddress.GetValue();
			DWORD EndPC = m_EndAddress.GetValue();
			DWORD DumpPC = m_PC.GetValue();
			GetDlgItemText(IDC_FILENAME,FileName,sizeof(FileName));
			if (strlen(FileName) == 0) 
			{
				Notify().DisplayError("Please Choose target file");
				::SetFocus(GetDlgItem(IDC_FILENAME));
				return false;
			}
			if (SendDlgItemMessage(IDC_USE_ALT_PC,BM_GETSTATE, 0,0) != BST_CHECKED)
			{
				DumpPC = _Reg->PROGRAM_COUNTER;
			}
			//disable buttons
			::EnableWindow(GetDlgItem(IDC_E_START_ADDR),FALSE);
			::EnableWindow(GetDlgItem(IDC_E_END_ADDR),FALSE);
			::EnableWindow(GetDlgItem(IDC_E_ALT_PC),FALSE);
			::EnableWindow(GetDlgItem(IDC_USE_ALT_PC),FALSE);
			::EnableWindow(GetDlgItem(IDC_FILENAME),FALSE);
			::EnableWindow(GetDlgItem(IDC_BTN_CHOOSE_FILE),FALSE);
			::EnableWindow(GetDlgItem(IDC_FORMAT),FALSE);
			::EnableWindow(GetDlgItem(IDOK),FALSE);
			::EnableWindow(GetDlgItem(IDCANCEL),FALSE);
			_N64System->ExternalEvent(PauseCPU_DumpMemory); 
			if (!DumpMemory(FileName,Format,StartPC,EndPC,DumpPC))
			{
				//enable buttons
				_N64System->ExternalEvent(ResumeCPU_DumpMemory); 
				return false;
			}
			_N64System->ExternalEvent(ResumeCPU_DumpMemory); 
		}
		EndDialog(0);
		break;
	}
	return FALSE;
}

//#include "..\\..\\User Interface.h"
//#include "..\\..\\N64 System.h"
//#include <windows.h>
//
//DWORD CDumpMemory::m_StartAddress = 0x80000000;
//DWORD CDumpMemory::m_EndAddress   = 0x803FFFF0;
//CDumpMemory::CDumpMemory(CMipsMemory * MMU) :
//	m_Window(NULL), _MMU(MMU)
//{
//}
//CDumpMemory::~CDumpMemory()
//{
//}
//void CDumpMemory::DisplayDump(WND_HANDLE & hParent)
//{
//	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Cheats_DumpMemory), 
//			(HWND)hParent, (DLGPROC)WinProc,(LPARAM)this);
//}
//DWORD CDumpMemory::AsciiToHex (const char * HexValue) {
//	DWORD Count, Finish, Value = 0;
//	Finish = strlen(HexValue);
//	if (Finish > 8 ) { Finish = 8; }
//	for (Count = 0; Count < Finish; Count++){
//		Value = (Value << 4);
//		switch( HexValue[Count] ) {
//		case '0': break;
//		case '1': Value += 1; break;
//		case '2': Value += 2; break;
//		case '3': Value += 3; break;
//		case '4': Value += 4; break;
//		case '5': Value += 5; break;
//		case '6': Value += 6; break;
//		case '7': Value += 7; break;
//		case '8': Value += 8; break;
//		case '9': Value += 9; break;
//		case 'A': Value += 10; break;
//		case 'a': Value += 10; break;
//		case 'B': Value += 11; break;
//		case 'b': Value += 11; break;
//		case 'C': Value += 12; break;
//		case 'c': Value += 12; break;
//		case 'D': Value += 13; break;
//		case 'd': Value += 13; break;
//		case 'E': Value += 14; break;
//		case 'e': Value += 14; break;
//		case 'F': Value += 15; break;
//		case 'f': Value += 15; break;
//		default: 
//			Value = (Value >> 4);
//			Count = Finish;
//		}
//	}
//	return Value;
//}
//int CALLBACK CDumpMemory::WinProc (WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam) 
//{
//	switch (uMsg) {
//	case WM_INITDIALOG:
//		{
//			CDumpMemory * _this = (CDumpMemory *)lParam;
//			SetProp((HWND)hDlg,"Class",_this);
//			_this->m_Window = hDlg;
//			SetDlgItemText((HWND)hDlg,IDC_E_START_ADDR,stdstr("0x%X",m_StartAddress).c_str());
//			SetDlgItemText((HWND)hDlg,IDC_E_END_ADDR,stdstr("0x%X",m_EndAddress).c_str());
//			SetDlgItemText((HWND)hDlg,IDC_E_ALT_PC,"0x80000000");
//			HWND hFormatList = GetDlgItem((HWND)hDlg,IDC_FORMAT);
//			int pos = SendMessage(hFormatList,CB_ADDSTRING,(WPARAM)0,(LPARAM)"TEXT - Disassembly + PC");
//			SendMessage(hFormatList,CB_SETITEMDATA,(WPARAM)pos,(LPARAM)DisassemblyWithPC);
//			SendMessage(hFormatList,CB_SETCURSEL,(WPARAM)0,(LPARAM)0);
//		}
//		break;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) 
//		{
//		case IDC_E_START_ADDR:
//		case IDC_E_END_ADDR:
//		case IDC_E_ALT_PC:
//			if (HIWORD(wParam) == EN_UPDATE) {
//				CDumpMemory * _this = (CDumpMemory *)GetProp((HWND)hDlg,"Class");
//				TCHAR szTmp[20], szTmp2[20];
//				DWORD Value;
//				GetDlgItemText((HWND)hDlg,LOWORD(wParam),szTmp,sizeof(szTmp));
//				Value = szTmp[1] =='x'?AsciiToHex(&szTmp[2]):AsciiToHex(szTmp);
//				//if (Value > Stop)  { Value = Stop; }
//				//if (Value < Start) { Value = Start; }
//				sprintf(szTmp2,"0x%X",Value);
//				if (strcmp(szTmp,szTmp2) != 0) {
//					SetDlgItemText((HWND)hDlg,LOWORD(wParam),szTmp2);
//					if (_this->SelStop == 0) { _this->SelStop = strlen(szTmp2); _this->SelStart = _this->SelStop; }
//					SendDlgItemMessage((HWND)hDlg,LOWORD(wParam),EM_SETSEL,(WPARAM)_this->SelStart,(LPARAM)_this->SelStop);
//				} else {
//					WORD NewSelStart, NewSelStop;
//					SendDlgItemMessage((HWND)hDlg,LOWORD(wParam),EM_GETSEL,(WPARAM)&NewSelStart,(LPARAM)&NewSelStop);
//					if (NewSelStart != 0) { _this->SelStart = NewSelStart; _this->SelStop = NewSelStop; }
//				}
//			}
//			break;
//		case IDC_BTN_CHOOSE_FILE:
//			{
//				CDumpMemory * _this = (CDumpMemory *)GetProp((HWND)hDlg,"Class");
//				OPENFILENAME openfilename;
//				char FileName[_MAX_PATH],Directory[_MAX_PATH];
//				memset(&FileName, 0, sizeof(FileName));
//				memset(&openfilename, 0, sizeof(openfilename));
//				strcpy(Directory,_Settings->LoadString(ApplicationDir).c_str());
//				openfilename.lStructSize  = sizeof( openfilename );
//				openfilename.hwndOwner    = (HWND)hDlg;
//				openfilename.lpstrFilter  = "Text file (*.txt)\0*.txt;\0All files (*.*)\0*.*\0";
//				openfilename.lpstrFile    = FileName;
//				openfilename.lpstrInitialDir    = Directory;
//				openfilename.nMaxFile     = MAX_PATH;
//				openfilename.Flags        = OFN_HIDEREADONLY;
//				if (GetOpenFileName (&openfilename)) 
//				{							
//					char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
//					_splitpath( FileName, drive, dir, fname, ext );
//					if (strlen(ext) == 0)
//					{
//                        strcat(FileName,".txt");
//					}
//					SetDlgItemText((HWND)hDlg,IDC_FILENAME,FileName);
//				}	
//			}
//			break;
//		case IDCANCEL:
//			RemoveProp((HWND)hDlg,"Class");
//			EndDialog((HWND)hDlg,0);
//			break;
//		case IDOK:
//			{
//				CDumpMemory * _this = (CDumpMemory *)GetProp((HWND)hDlg,"Class");
//				TCHAR szTmp[20], FileName[MAX_PATH];
//				int CurrentFormatSel = SendDlgItemMessage((HWND)hDlg,IDC_FORMAT,CB_GETCURSEL,0,0);
//				DumpFormat Format = (DumpFormat)SendDlgItemMessage((HWND)hDlg,IDC_FORMAT,CB_GETITEMDATA,CurrentFormatSel,0);
//				GetDlgItemText((HWND)hDlg,IDC_E_START_ADDR,szTmp,sizeof(szTmp));
//				DWORD StartPC = szTmp[1] =='x'?AsciiToHex(&szTmp[2]):AsciiToHex(szTmp);
//				GetDlgItemText((HWND)hDlg,IDC_E_END_ADDR,szTmp,sizeof(szTmp));
//				DWORD EndPC = szTmp[1] =='x'?AsciiToHex(&szTmp[2]):AsciiToHex(szTmp);
//				GetDlgItemText((HWND)hDlg,IDC_E_ALT_PC,szTmp,sizeof(szTmp));
//				DWORD DumpPC = szTmp[1] =='x'?AsciiToHex(&szTmp[2]):AsciiToHex(szTmp);
//				GetDlgItemText((HWND)hDlg,IDC_FILENAME,FileName,sizeof(FileName));
//				if (strlen(FileName) == 0) 
//				{
//					Notify().DisplayError("Please Choose target file");
//					SetFocus(GetDlgItem((HWND)hDlg,IDC_FILENAME));
//					return false;
//				}
//				if (SendDlgItemMessage((HWND)hDlg,IDC_USE_ALT_PC,BM_GETSTATE, 0,0) != BST_CHECKED)
//				{
//					DumpPC = _this->_MMU->SystemRegisters()->PROGRAM_COUNTER;
//				}
//				//disable buttons
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_E_START_ADDR),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_E_END_ADDR),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_E_ALT_PC),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_USE_ALT_PC),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_FILENAME),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_BTN_CHOOSE_FILE),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_FORMAT),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDOK),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDCANCEL),FALSE);
//				if (!_this->DumpMemory(FileName,Format,StartPC,EndPC,DumpPC))
//				{
//					//enable buttons
//					return false;
//				}
//			}
//			RemoveProp((HWND)hDlg,"Class");
//			EndDialog((HWND)hDlg,0);
//			break;
//		}
//		break;
//	default:
//		return false;
//	}
//	return true;
//}

bool CDumpMemory::DumpMemory ( LPCSTR FileName,DumpFormat Format, DWORD StartPC, DWORD EndPC, DWORD DumpPC )
{
	switch (Format)
	{
	case DisassemblyWithPC:
		{
			CLog LogFile;
			if (!LogFile.Open(FileName))
			{
				Notify().DisplayError("Failed to open\n%s",FileName);
				return false;
			}
			LogFile.SetFlush(false);
			LogFile.SetTruncateFile(false);
			char Command[200];
			for (COpcode OpCode(StartPC);  OpCode.PC() < EndPC; OpCode.Next())
			{
				const char * szOpName = OpCode.OpcodeName();
				OpCode.OpcodeParam(Command);
				LogFile.LogF("%X: %-15s%s\r\n",OpCode.PC(),szOpName,Command);
			}
			m_StartAddress.SetValue(StartPC,true,true);
			m_EndAddress.SetValue(EndPC,true,true);
			return true;
		}
		break;
	}
	return false;
}
//
//CDumpMemory::CDumpMemory(CMipsMemory * MMU) :
//	m_Window(NULL), _MMU(MMU)
//{
//}
//
//CDumpMemory::~CDumpMemory()
//{
//}
//
//void CDumpMemory::DisplayDump(WND_HANDLE & hParent)
//{
//	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Cheats_DumpMemory), 
//			(HWND)hParent, (DLGPROC)WinProc,(LPARAM)this);
//}
//
//DWORD CDumpMemory::AsciiToHex (const char * HexValue) {
//	DWORD Count, Finish, Value = 0;
//
//	Finish = strlen(HexValue);
//	if (Finish > 8 ) { Finish = 8; }
//
//	for (Count = 0; Count < Finish; Count++){
//		Value = (Value << 4);
//		switch( HexValue[Count] ) {
//		case '0': break;
//		case '1': Value += 1; break;
//		case '2': Value += 2; break;
//		case '3': Value += 3; break;
//		case '4': Value += 4; break;
//		case '5': Value += 5; break;
//		case '6': Value += 6; break;
//		case '7': Value += 7; break;
//		case '8': Value += 8; break;
//		case '9': Value += 9; break;
//		case 'A': Value += 10; break;
//		case 'a': Value += 10; break;
//		case 'B': Value += 11; break;
//		case 'b': Value += 11; break;
//		case 'C': Value += 12; break;
//		case 'c': Value += 12; break;
//		case 'D': Value += 13; break;
//		case 'd': Value += 13; break;
//		case 'E': Value += 14; break;
//		case 'e': Value += 14; break;
//		case 'F': Value += 15; break;
//		case 'f': Value += 15; break;
//		default: 
//			Value = (Value >> 4);
//			Count = Finish;
//		}
//	}
//	return Value;
//}
//
//int CALLBACK CDumpMemory::WinProc (WND_HANDLE hDlg,DWORD uMsg,DWORD wParam, DWORD lParam) 
//{
//	switch (uMsg) {
//	case WM_INITDIALOG:
//		{
//			CDumpMemory * _this = (CDumpMemory *)lParam;
//			SetProp((HWND)hDlg,"Class",_this);
//			_this->m_Window = hDlg;
//			SetDlgItemText((HWND)hDlg,IDC_E_START_ADDR,stdstr("0x%X",m_StartAddress).c_str());
//			SetDlgItemText((HWND)hDlg,IDC_E_END_ADDR,stdstr("0x%X",m_EndAddress).c_str());
//			SetDlgItemText((HWND)hDlg,IDC_E_ALT_PC,"0x80000000");
//
//			HWND hFormatList = GetDlgItem((HWND)hDlg,IDC_FORMAT);
//			int pos = SendMessage(hFormatList,CB_ADDSTRING,(WPARAM)0,(LPARAM)"TEXT - Disassembly + PC");
//			SendMessage(hFormatList,CB_SETITEMDATA,(WPARAM)pos,(LPARAM)DisassemblyWithPC);
//			SendMessage(hFormatList,CB_SETCURSEL,(WPARAM)0,(LPARAM)0);
//
//		}
//		break;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) 
//		{
//		case IDC_E_START_ADDR:
//		case IDC_E_END_ADDR:
//		case IDC_E_ALT_PC:
//			if (HIWORD(wParam) == EN_UPDATE) {
//				CDumpMemory * _this = (CDumpMemory *)GetProp((HWND)hDlg,"Class");
//
//				TCHAR szTmp[20], szTmp2[20];
//				DWORD Value;
//
//				GetDlgItemText((HWND)hDlg,LOWORD(wParam),szTmp,sizeof(szTmp));
//				Value = szTmp[1] =='x'?AsciiToHex(&szTmp[2]):AsciiToHex(szTmp);
//				//if (Value > Stop)  { Value = Stop; }
//				//if (Value < Start) { Value = Start; }
//				sprintf(szTmp2,"0x%X",Value);
//				if (strcmp(szTmp,szTmp2) != 0) {
//					SetDlgItemText((HWND)hDlg,LOWORD(wParam),szTmp2);
//					if (_this->SelStop == 0) { _this->SelStop = strlen(szTmp2); _this->SelStart = _this->SelStop; }
//					SendDlgItemMessage((HWND)hDlg,LOWORD(wParam),EM_SETSEL,(WPARAM)_this->SelStart,(LPARAM)_this->SelStop);
//				} else {
//					WORD NewSelStart, NewSelStop;
//					SendDlgItemMessage((HWND)hDlg,LOWORD(wParam),EM_GETSEL,(WPARAM)&NewSelStart,(LPARAM)&NewSelStop);
//					if (NewSelStart != 0) { _this->SelStart = NewSelStart; _this->SelStop = NewSelStop; }
//				}
//			}
//			break;
//		case IDC_BTN_CHOOSE_FILE:
//			{
//				CDumpMemory * _this = (CDumpMemory *)GetProp((HWND)hDlg,"Class");
//
//				OPENFILENAME openfilename;
//				char FileName[_MAX_PATH],Directory[_MAX_PATH];
//
//				memset(&FileName, 0, sizeof(FileName));
//				memset(&openfilename, 0, sizeof(openfilename));
//
//				strcpy(Directory,_Settings->LoadString(ApplicationDir).c_str());
//
//				openfilename.lStructSize  = sizeof( openfilename );
//				openfilename.hwndOwner    = (HWND)hDlg;
//				openfilename.lpstrFilter  = "Text file (*.txt)\0*.txt;\0All files (*.*)\0*.*\0";
//				openfilename.lpstrFile    = FileName;
//				openfilename.lpstrInitialDir    = Directory;
//				openfilename.nMaxFile     = MAX_PATH;
//				openfilename.Flags        = OFN_HIDEREADONLY;
//
//				if (GetOpenFileName (&openfilename)) 
//				{							
//					char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
//
//					_splitpath( FileName, drive, dir, fname, ext );
//					if (strlen(ext) == 0)
//					{
//                        strcat(FileName,".txt");
//					}
//					SetDlgItemText((HWND)hDlg,IDC_FILENAME,FileName);
//				}	
//			}
//			break;
//		case IDCANCEL:
//			RemoveProp((HWND)hDlg,"Class");
//			EndDialog((HWND)hDlg,0);
//			break;
//		case IDOK:
//			{
//				CDumpMemory * _this = (CDumpMemory *)GetProp((HWND)hDlg,"Class");
//				TCHAR szTmp[20], FileName[MAX_PATH];
//
//				int CurrentFormatSel = SendDlgItemMessage((HWND)hDlg,IDC_FORMAT,CB_GETCURSEL,0,0);
//				DumpFormat Format = (DumpFormat)SendDlgItemMessage((HWND)hDlg,IDC_FORMAT,CB_GETITEMDATA,CurrentFormatSel,0);
//
//				GetDlgItemText((HWND)hDlg,IDC_E_START_ADDR,szTmp,sizeof(szTmp));
//				DWORD StartPC = szTmp[1] =='x'?AsciiToHex(&szTmp[2]):AsciiToHex(szTmp);
//				GetDlgItemText((HWND)hDlg,IDC_E_END_ADDR,szTmp,sizeof(szTmp));
//				DWORD EndPC = szTmp[1] =='x'?AsciiToHex(&szTmp[2]):AsciiToHex(szTmp);
//				GetDlgItemText((HWND)hDlg,IDC_E_ALT_PC,szTmp,sizeof(szTmp));
//				DWORD DumpPC = szTmp[1] =='x'?AsciiToHex(&szTmp[2]):AsciiToHex(szTmp);
//				GetDlgItemText((HWND)hDlg,IDC_FILENAME,FileName,sizeof(FileName));
//
//				if (strlen(FileName) == 0) 
//				{
//					Notify().DisplayError("Please Choose target file");
//					SetFocus(GetDlgItem((HWND)hDlg,IDC_FILENAME));
//					return false;
//				}
//
//				if (SendDlgItemMessage((HWND)hDlg,IDC_USE_ALT_PC,BM_GETSTATE, 0,0) != BST_CHECKED)
//				{
//					DumpPC = _this->_MMU->SystemRegisters()->PROGRAM_COUNTER;
//				}
//				//disable buttons
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_E_START_ADDR),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_E_END_ADDR),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_E_ALT_PC),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_USE_ALT_PC),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_FILENAME),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_BTN_CHOOSE_FILE),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDC_FORMAT),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDOK),FALSE);
//				EnableWindow(GetDlgItem((HWND)hDlg,IDCANCEL),FALSE);
//				if (!_this->DumpMemory(FileName,Format,StartPC,EndPC,DumpPC))
//				{
//					//enable buttons
//					return false;
//				}
//			}
//			RemoveProp((HWND)hDlg,"Class");
//			EndDialog((HWND)hDlg,0);
//			break;
//		}
//		break;
//	default:
//		return false;
//	}
//	return true;
//}
//
//bool CDumpMemory::DumpMemory ( LPCSTR FileName,DumpFormat Format, DWORD StartPC, DWORD EndPC, DWORD DumpPC )
//{
//	switch (Format)
//	{
//	case DisassemblyWithPC:
//		{
//			CLog LogFile(FileName);
//			if (!LogFile.IsOpen())
//			{
//				Notify().DisplayError("Failed to open\n%s",FileName);
//				return false;
//			}
//
//			for (COpcode OpCode(_MMU,StartPC);  OpCode.PC() < EndPC; OpCode.Next())
//			{
//				LogFile.Log("%X: %s",OpCode.PC(),OpCode.Name().c_str());
//			}
//			m_StartAddress = StartPC;
//			m_EndAddress   = EndPC;
//			return true;
//		}
//		break;
//	}
//	return false;
//}
