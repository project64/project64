/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */
#ifdef todelete

#if (!defined(EXTERNAL_RELEASE))
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "main.h"
#include "cpu.h"
#include "..\..\User Interface\resource.h"

LRESULT CALLBACK TLB_Proc (HWND, UINT, WPARAM, LPARAM);
HWND TlbDlg = NULL;

void Create_TLB_Window ( int Child ) {
	if ( Child ) {
		DialogBox( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Debugger_TLB), NULL,(DLGPROC) TLB_Proc );
		TlbDlg = NULL;
	} else {
		if (TlbDlg == NULL) {
			DWORD ThreadID;
		
			CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Create_TLB_Window,(LPVOID)TRUE,0, 
				&ThreadID);	
		} else {
			SetForegroundWindow(TlbDlg);
		}	
	}
}

void Enter_TLB_Window ( void ) {
	if (!HaveDebugger) { return; }
    Create_TLB_Window ( FALSE );
}

void RefreshTLBWindow (void) {
	HWND hList = GetDlgItem(TlbDlg,IDC_LIST);
	char Output[100], OldText[100];
	LV_ITEM item, OldItem;
	int count;

	if (!TlbDlg) { return; }
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
	
	hList = GetDlgItem(TlbDlg,IDC_LIST2);
	
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
				FastTlb[count].PHYSSTART,FastTlb[count].VEND - FastTlb[count].VSTART + FastTlb[count].PHYSSTART);
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

void SetupTLBWindow (HWND hDlg) {
	LV_COLUMN  col;
	DWORD X, Y;

	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt  = LVCFMT_LEFT;

	col.pszText  = "Index";
	col.cx       = 40;
	col.iSubItem = 0;
	ListView_InsertColumn ( GetDlgItem(hDlg,IDC_LIST), 0, &col);

	col.pszText  = "Page Mask";
	col.cx       = 90;
	col.iSubItem = 1;
	ListView_InsertColumn ( GetDlgItem(hDlg,IDC_LIST), 1, &col);

	col.pszText  = "Entry Hi";
	col.cx       = 90;
	col.iSubItem = 2;
	ListView_InsertColumn ( GetDlgItem(hDlg,IDC_LIST), 2, &col);

	col.pszText  = "Entry Lo0";
	col.cx       = 90;
	col.iSubItem = 3;
	ListView_InsertColumn ( GetDlgItem(hDlg,IDC_LIST), 3, &col);

	col.pszText  = "Entry Lo1";
	col.cx       = 90;
	col.iSubItem = 4;
	ListView_InsertColumn ( GetDlgItem(hDlg,IDC_LIST), 4, &col);

	col.pszText  = "Index";
	col.cx       = 40;
	col.iSubItem = 0;
	ListView_InsertColumn ( GetDlgItem(hDlg,IDC_LIST2), 0, &col);

	col.pszText  = "Valid";
	col.cx       = 40;
	col.iSubItem = 1;
	ListView_InsertColumn ( GetDlgItem(hDlg,IDC_LIST2), 1, &col);

	col.pszText  = "Dirty";
	col.cx       = 40;
	col.iSubItem = 2;
	ListView_InsertColumn ( GetDlgItem(hDlg,IDC_LIST2), 2, &col);

	col.pszText  = "Rule";
	col.cx       = 280;
	col.iSubItem = 3;
	ListView_InsertColumn ( GetDlgItem(hDlg,IDC_LIST2), 3, &col);

	RefreshTLBWindow();
	SendMessage(GetDlgItem(hDlg,IDC_TLB_ENTRIES),BM_SETCHECK, BST_CHECKED,0);

	if (GetStoredWinPos( "TLB Window", &X, &Y )) {
		SetWindowPos(hDlg,NULL,X,Y,0,0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
	}
}

LRESULT CALLBACK TLB_Proc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {	
	switch (uMsg) {
	case WM_INITDIALOG:
		TlbDlg = hDlg;
		SetupTLBWindow(hDlg);
		break;
	case WM_MOVE:
		StoreCurrentWinPos("TLB Window",hDlg);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TLB_ENTRIES:
			ShowWindow(GetDlgItem(hDlg,IDC_LIST),SW_SHOW);
			ShowWindow(GetDlgItem(hDlg,IDC_LIST2),SW_HIDE);
			break;
		case IDC_TLB_RULES:
			ShowWindow(GetDlgItem(hDlg,IDC_LIST),SW_HIDE);
			ShowWindow(GetDlgItem(hDlg,IDC_LIST2),SW_SHOW);
			break;
		case IDCANCEL:
			EndDialog( hDlg, IDCANCEL );
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
#endif

#endif