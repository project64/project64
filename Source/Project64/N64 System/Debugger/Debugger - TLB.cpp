#include "Debugger UI.h"

CDebugTlb::CDebugTlb(CMipsMemory * MMU, CDebugger * debugger) :
	CDebugDialog<CDebugTlb>(MMU,debugger)
{
}

CDebugTlb::~CDebugTlb()
{
}

LRESULT	CDebugTlb::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	LV_COLUMN  col;

	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt  = LVCFMT_LEFT;

	col.pszText  = "Index";
	col.cx       = 40;
	col.iSubItem = 0;
	ListView_InsertColumn ( GetDlgItem(IDC_LIST), 0, &col);

	col.pszText  = "Page Mask";
	col.cx       = 90;
	col.iSubItem = 1;
	ListView_InsertColumn ( GetDlgItem(IDC_LIST), 1, &col);

	col.pszText  = "Entry Hi";
	col.cx       = 90;
	col.iSubItem = 2;
	ListView_InsertColumn ( GetDlgItem(IDC_LIST), 2, &col);

	col.pszText  = "Entry Lo0";
	col.cx       = 90;
	col.iSubItem = 3;
	ListView_InsertColumn ( GetDlgItem(IDC_LIST), 3, &col);

	col.pszText  = "Entry Lo1";
	col.cx       = 90;
	col.iSubItem = 4;
	ListView_InsertColumn ( GetDlgItem(IDC_LIST), 4, &col);

	col.pszText  = "Index";
	col.cx       = 40;
	col.iSubItem = 0;
	ListView_InsertColumn ( GetDlgItem(IDC_LIST2), 0, &col);

	col.pszText  = "Valid";
	col.cx       = 40;
	col.iSubItem = 1;
	ListView_InsertColumn ( GetDlgItem(IDC_LIST2), 1, &col);

	col.pszText  = "Dirty";
	col.cx       = 40;
	col.iSubItem = 2;
	ListView_InsertColumn ( GetDlgItem(IDC_LIST2), 2, &col);

	col.pszText  = "Rule";
	col.cx       = 270;
	col.iSubItem = 3;
	ListView_InsertColumn ( GetDlgItem(IDC_LIST2), 3, &col);

	RefreshTLBWindow();
	SendMessage(GetDlgItem(IDC_TLB_ENTRIES),BM_SETCHECK, BST_CHECKED,0);
	
//	if (Settings().Load(TLBWindowLeft) <= 0)
//	{
//		SetWindowPos(NULL,Settings().Load(TLBWindowLeft),Settings().Load(TLBWindowTop),0,0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
//	}
	WindowCreated();
	return TRUE;
}

LRESULT CDebugTlb::OnClicked (WORD wNotifyCode, WORD wID, HWND , BOOL& bHandled)
{
	switch(wID)
	{
	case IDCANCEL:
		EndDialog(0);
		break;
	case IDC_TLB_ENTRIES:
		::ShowWindow(GetDlgItem(IDC_LIST),SW_SHOW);
		::ShowWindow(GetDlgItem(IDC_LIST2),SW_HIDE);
		break;
	case IDC_TLB_RULES:
		::ShowWindow(GetDlgItem(IDC_LIST),SW_HIDE);
		::ShowWindow(GetDlgItem(IDC_LIST2),SW_SHOW);
		break;
	}
	return FALSE;
}

void CDebugTlb::RefreshTLBWindow (void) {
	HWND hList = GetDlgItem(IDC_LIST);
	char Output[100], OldText[100];
	LV_ITEM item, OldItem;
	int count;

	TLB * tlb         = m_MMU->tlb;
	FASTTLB * FastTlb = m_MMU->FastTlb;
	
	if (m_hWnd == NULL) 
	{
		return; 
	}
	
	for (count = 0; count < 32; count ++) {
		sprintf(Output,"0x%02X",count);
		item.mask      = LVIF_TEXT;
		item.iItem     = count;
		item.pszText   = Output;
		item.iSubItem  = 0;

		OldItem.mask       = LVIF_TEXT;
		OldItem.iItem      = count;
		OldItem.pszText    = OldText;
		OldItem.cchTextMax = sizeof( OldText )-1;
		OldItem.iSubItem   = 0;

		if (ListView_GetItemCount(hList) <= count) {
			ListView_InsertItem(hList,&item);
		} else {
			ListView_GetItem(hList,&OldItem);
			if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
				ListView_SetItem(hList,&item);
			}
		}
		if (tlb[count].EntryDefined) {
			sprintf(Output,"0x%08X",tlb[count].PageMask.Value);
		} else {
			strcpy(Output,"................");
		}
		item.iSubItem  = 1;
		OldItem.iSubItem   = 1;
		ListView_GetItem(hList,&OldItem);
		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
			ListView_SetItem(hList,&item);
		}

		if (tlb[count].EntryDefined) {
			sprintf(Output,"0x%08X",tlb[count].EntryHi.Value);
		} else {
			strcpy(Output,"................");
		}
		item.iSubItem  = 2;
		OldItem.iSubItem   = 2;
		ListView_GetItem(hList,&OldItem);
		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
			ListView_SetItem(hList,&item);
		}

		if (tlb[count].EntryDefined) {
			sprintf(Output,"0x%08X",tlb[count].EntryLo0.Value);
		} else {
			strcpy(Output,"................");
		}
		item.iSubItem  = 3;
		OldItem.iSubItem   = 3;
		ListView_GetItem(hList,&OldItem);
		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
			ListView_SetItem(hList,&item);
		}

		if (tlb[count].EntryDefined) {
			sprintf(Output,"0x%08X",tlb[count].EntryLo1.Value);
		} else {
			strcpy(Output,"................");
		}
		item.iSubItem  = 4;
		OldItem.iSubItem   = 4;
		ListView_GetItem(hList,&OldItem);
		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
			ListView_SetItem(hList,&item);
		}
	}
	
	hList = GetDlgItem(IDC_LIST2);
	
	for (count = 0; count < 64; count ++) {
		sprintf(Output,"0x%02X",count);
		item.mask      = LVIF_TEXT;
		item.iItem     = count;
		item.pszText   = Output;
		item.iSubItem  = 0;

		OldItem.mask       = LVIF_TEXT;
		OldItem.iItem      = count;
		OldItem.pszText    = OldText;
		OldItem.cchTextMax = sizeof( OldText )-1;
		OldItem.iSubItem   = 0;

		if (ListView_GetItemCount(hList) <= count) {
			item.iItem = ListView_InsertItem(hList,&item);
		} else {
			ListView_GetItem(hList,&OldItem);
			if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
				ListView_SetItem(hList,&item);
			}
		}

		if (FastTlb[count].ValidEntry) {
			sprintf(Output,"%s",FastTlb[count].VALID?"Yes":"No");
		} else {
			strcpy(Output,"................");
		}
		item.iSubItem  = 1;
		OldItem.iSubItem   = 1;
		ListView_GetItem(hList,&OldItem);
		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
			ListView_SetItem(hList,&item);
		}

		if (FastTlb[count].ValidEntry && FastTlb[count].VALID) {
			sprintf(Output,"%s",FastTlb[count].DIRTY?"Yes":"No");
		} else {
			strcpy(Output,"................");
		}
		item.iSubItem  = 2;
		OldItem.iSubItem   = 2;
		ListView_GetItem(hList,&OldItem);
		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
			ListView_SetItem(hList,&item);
		}

		if (FastTlb[count].ValidEntry && FastTlb[count].VALID) {
			sprintf(Output,"%08X:%08X -> %08X:%08X",FastTlb[count].VSTART,FastTlb[count].VEND,
				FastTlb[count].PHYSSTART,FastTlb[count].PHYSEND);
		} else {
			strcpy(Output,"................");
		}
		item.iSubItem  = 3;
		OldItem.iSubItem  = 3;
		ListView_GetItem(hList,&OldItem);
		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
			ListView_SetItem(hList,&item);
		}
	}
}

//{
//	if (m_hDebugWnd)
//	{
//		SetForegroundWindow((HWND)m_hDebugWnd);
//	} else {
//		DWORD ThreadID;
//		HANDLE hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)CreateDebugWindow,(LPVOID)this,0, &ThreadID);
//		CloseHandle(hThread);
//	}
//}
//
//void CDebugTlb::CreateDebugWindow ( CDebugTlb * _this ) {
//	DialogBoxParam( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Debugger_TLB), NULL,(DLGPROC) DebugWndProc, (LPARAM)_this );
//	_this->m_hDebugWnd = NULL;
//}
//
//void CDebugTlb::SetupDebugWindow (void) 
//{
//	LV_COLUMN  col;
//
//	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
//	col.fmt  = LVCFMT_LEFT;
//
//	col.pszText  = "Index";
//	col.cx       = 40;
//	col.iSubItem = 0;
//	ListView_InsertColumn ( GetDlgItem((HWND)m_hDebugWnd,IDC_LIST), 0, &col);
//
//	col.pszText  = "Page Mask";
//	col.cx       = 90;
//	col.iSubItem = 1;
//	ListView_InsertColumn ( GetDlgItem((HWND)m_hDebugWnd,IDC_LIST), 1, &col);
//
//	col.pszText  = "Entry Hi";
//	col.cx       = 90;
//	col.iSubItem = 2;
//	ListView_InsertColumn ( GetDlgItem((HWND)m_hDebugWnd,IDC_LIST), 2, &col);
//
//	col.pszText  = "Entry Lo0";
//	col.cx       = 90;
//	col.iSubItem = 3;
//	ListView_InsertColumn ( GetDlgItem((HWND)m_hDebugWnd,IDC_LIST), 3, &col);
//
//	col.pszText  = "Entry Lo1";
//	col.cx       = 90;
//	col.iSubItem = 4;
//	ListView_InsertColumn ( GetDlgItem((HWND)m_hDebugWnd,IDC_LIST), 4, &col);
//
//	col.pszText  = "Index";
//	col.cx       = 40;
//	col.iSubItem = 0;
//	ListView_InsertColumn ( GetDlgItem((HWND)m_hDebugWnd,IDC_LIST2), 0, &col);
//
//	col.pszText  = "Valid";
//	col.cx       = 40;
//	col.iSubItem = 1;
//	ListView_InsertColumn ( GetDlgItem((HWND)m_hDebugWnd,IDC_LIST2), 1, &col);
//
//	col.pszText  = "Dirty";
//	col.cx       = 40;
//	col.iSubItem = 2;
//	ListView_InsertColumn ( GetDlgItem((HWND)m_hDebugWnd,IDC_LIST2), 2, &col);
//
//	col.pszText  = "Rule";
//	col.cx       = 280;
//	col.iSubItem = 3;
//	ListView_InsertColumn ( GetDlgItem((HWND)m_hDebugWnd,IDC_LIST2), 3, &col);
//
//	RefreshTLBWindow();
//	SendMessage(GetDlgItem((HWND)m_hDebugWnd,IDC_TLB_ENTRIES),BM_SETCHECK, BST_CHECKED,0);
//	
//	if (Settings().Load(TLBWindowLeft) <= 0)
//	{
//		SetWindowPos((HWND)m_hDebugWnd,NULL,Settings().Load(TLBWindowLeft),Settings().Load(TLBWindowTop),0,0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
//	}
//}
//
//void CDebugTlb::RefreshTLBWindow (void) {
//	HWND hList = GetDlgItem((HWND)m_hDebugWnd,IDC_LIST);
//	char Output[100], OldText[100];
//	LV_ITEM item, OldItem;
//	int count;
//
//	TLB * tlb         = _TLB->tlb;
//	FASTTLB * FastTlb = _TLB->FastTlb;
//	
//	if (!(HWND)m_hDebugWnd) { return; }
//	for (count = 0; count < 32; count ++) {
//		sprintf(Output,"0x%02X",count);
//		item.mask      = LVIF_TEXT;
//		item.iItem     = count;
//		item.pszText   = Output;
//		item.iSubItem  = 0;
//
//		OldItem.mask       = LVIF_TEXT;
//		OldItem.iItem      = count;
//		OldItem.pszText    = OldText;
//		OldItem.cchTextMax = sizeof( OldText )-1;
//		OldItem.iSubItem   = 0;
//
//		if (ListView_GetItemCount(hList) <= count) {
//			ListView_InsertItem(hList,&item);
//		} else {
//			ListView_GetItem(hList,&OldItem);
//			if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
//				ListView_SetItem(hList,&item);
//			}
//		}
//		if (tlb[count].EntryDefined) {
//			sprintf(Output,"0x%08X",tlb[count].PageMask.Value);
//		} else {
//			strcpy(Output,"................");
//		}
//		item.iSubItem  = 1;
//		OldItem.iSubItem   = 1;
//		ListView_GetItem(hList,&OldItem);
//		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
//			ListView_SetItem(hList,&item);
//		}
//
//		if (tlb[count].EntryDefined) {
//			sprintf(Output,"0x%08X",tlb[count].EntryHi.Value);
//		} else {
//			strcpy(Output,"................");
//		}
//		item.iSubItem  = 2;
//		OldItem.iSubItem   = 2;
//		ListView_GetItem(hList,&OldItem);
//		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
//			ListView_SetItem(hList,&item);
//		}
//
//		if (tlb[count].EntryDefined) {
//			sprintf(Output,"0x%08X",tlb[count].EntryLo0.Value);
//		} else {
//			strcpy(Output,"................");
//		}
//		item.iSubItem  = 3;
//		OldItem.iSubItem   = 3;
//		ListView_GetItem(hList,&OldItem);
//		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
//			ListView_SetItem(hList,&item);
//		}
//
//		if (tlb[count].EntryDefined) {
//			sprintf(Output,"0x%08X",tlb[count].EntryLo1.Value);
//		} else {
//			strcpy(Output,"................");
//		}
//		item.iSubItem  = 4;
//		OldItem.iSubItem   = 4;
//		ListView_GetItem(hList,&OldItem);
//		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
//			ListView_SetItem(hList,&item);
//		}
//	}
//	
//	hList = GetDlgItem((HWND)m_hDebugWnd,IDC_LIST2);
//	
//	for (count = 0; count < 64; count ++) {
//		sprintf(Output,"0x%02X",count);
//		item.mask      = LVIF_TEXT;
//		item.iItem     = count;
//		item.pszText   = Output;
//		item.iSubItem  = 0;
//
//		OldItem.mask       = LVIF_TEXT;
//		OldItem.iItem      = count;
//		OldItem.pszText    = OldText;
//		OldItem.cchTextMax = sizeof( OldText )-1;
//		OldItem.iSubItem   = 0;
//
//		if (ListView_GetItemCount(hList) <= count) {
//			item.iItem = ListView_InsertItem(hList,&item);
//		} else {
//			ListView_GetItem(hList,&OldItem);
//			if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
//				ListView_SetItem(hList,&item);
//			}
//		}
//
//		if (FastTlb[count].ValidEntry) {
//			sprintf(Output,"%s",FastTlb[count].VALID?"Yes":"No");
//		} else {
//			strcpy(Output,"................");
//		}
//		item.iSubItem  = 1;
//		OldItem.iSubItem   = 1;
//		ListView_GetItem(hList,&OldItem);
//		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
//			ListView_SetItem(hList,&item);
//		}
//
//		if (FastTlb[count].ValidEntry && FastTlb[count].VALID) {
//			sprintf(Output,"%s",FastTlb[count].DIRTY?"Yes":"No");
//		} else {
//			strcpy(Output,"................");
//		}
//		item.iSubItem  = 2;
//		OldItem.iSubItem   = 2;
//		ListView_GetItem(hList,&OldItem);
//		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
//			ListView_SetItem(hList,&item);
//		}
//
//		if (FastTlb[count].ValidEntry && FastTlb[count].VALID) {
//			sprintf(Output,"%08X:%08X -> %08X:%08X",FastTlb[count].VSTART,FastTlb[count].VEND,
//				FastTlb[count].PHYSSTART,FastTlb[count].PHYSEND);
//		} else {
//			strcpy(Output,"................");
//		}
//		item.iSubItem  = 3;
//		OldItem.iSubItem  = 3;
//		ListView_GetItem(hList,&OldItem);
//		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
//			ListView_SetItem(hList,&item);
//		}
//	}
//}
//
//DWORD CALLBACK DebugWndProc (WND_HANDLE hDlg, DWORD uMsg, DWORD wParam, DWORD lParam) 
//{	
//	switch (uMsg) {
//	case WM_INITDIALOG:
//		{
//			CDebugTlb * _this = (CDebugTlb *)lParam;
//			SetProp((HWND)hDlg,"Class",_this);
//			_this->m_hDebugWnd = hDlg;
//			_this->SetupDebugWindow();
//		}
//		break;
//	case WM_MOVE:
//		{		
//			RECT WinRect;
//			GetWindowRect((HWND)hDlg, &WinRect );
//
//			Settings().Save(TLBWindowLeft,WinRect.left);
//			Settings().Save(TLBWindowTop,WinRect.top);
//		}
//		break;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDC_TLB_ENTRIES:
//			ShowWindow(GetDlgItem((HWND)hDlg,IDC_LIST),SW_SHOW);
//			ShowWindow(GetDlgItem((HWND)hDlg,IDC_LIST2),SW_HIDE);
//			break;
//		case IDC_TLB_RULES:
//			ShowWindow(GetDlgItem((HWND)hDlg,IDC_LIST),SW_HIDE);
//			ShowWindow(GetDlgItem((HWND)hDlg,IDC_LIST2),SW_SHOW);
//			break;
//		case IDCANCEL:
//			EndDialog( (HWND)hDlg, IDCANCEL );
//			break;
//		}
//		break;
//	default:
//		return FALSE;
//	}
//	return TRUE;
//}