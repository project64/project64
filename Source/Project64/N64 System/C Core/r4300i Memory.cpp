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
void __cdecl Refresh_Memory ( void ) {
}

#ifdef OLD_CODE

#if (!defined(EXTERNAL_RELEASE))
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "main.h"
#include "CPU.h"
#include "debugger.h"
#include "resource.h"

#define IDC_VADDR			0x100
#define IDC_PADDR			0x101
#define IDC_LIST_VIEW		0x102
#define IDC_SCRL_BAR		0x103

void Setup_Memory_Window (HWND hDlg);

LRESULT CALLBACK Memory_Window_Proc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

static HWND Memory_Win_hDlg, hAddrEdit, hVAddr, hPAddr, hList, hScrlBar;
static int InMemoryWindow = FALSE;

void Create_Memory_Window ( int Child ) {
	DWORD ThreadID;
	if ( Child ) {
		InMemoryWindow = TRUE;
		DialogBox( hInst, "MEMORY", NULL,(DLGPROC) Memory_Window_Proc );
		InMemoryWindow = FALSE;
	} else {
		if (!InMemoryWindow) {
			CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Create_Memory_Window,
				(LPVOID)TRUE,0, &ThreadID);	
		} else {
			SetForegroundWindow(Memory_Win_hDlg);
		}	
	}
}

void __cdecl Enter_Memory_Window ( void ) {
	if (!HaveDebugger) { return; }
    Create_Memory_Window ( FALSE );
}

void Insert_MemoryLineDump (unsigned int location, int InsertPos) {
	char Output[20], Hex[60], Ascii[20], HexAddOn[15], AsciiAddOn[15], OldText[100];
	LV_ITEM item, OldItem;
	int count;
	MIPS_WORD word;

	location       = location << 4;
	item.mask      = LVIF_TEXT | LVIF_PARAM;
	item.iItem     = InsertPos;
	item.lParam	   = location;
	sprintf(Output,"0x%08X",location);
	item.pszText   = Output;
	item.iSubItem  = 0;

	OldItem.mask       = LVIF_TEXT | LVIF_PARAM;
	OldItem.iItem      = InsertPos;
	OldItem.pszText    = OldText;
	OldItem.cchTextMax = sizeof( OldText )-1;
	OldItem.iSubItem   = 0;
	
	if (ListView_GetItemCount(hList) <= InsertPos) {
		ListView_InsertItem(hList,&item);
	} else {
		ListView_GetItem(hList,&OldItem);
		if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
			ListView_SetItem(hList,&item);
		}
	}

	sprintf(Hex,"\0"); sprintf(Ascii,"\0"); 
	
	__try {
		if ( SendMessage(hVAddr,BM_GETSTATE,0,0) & BST_CHECKED != 0 ) {
			for (count = 0; count < 4; count ++) {
				if (r4300i_LW_VAddr(location, &word.UW)) {
					sprintf(HexAddOn,"%02X %02X %02X %02X",word.UB[3],
						word.UB[2],word.UB[1],word.UB[0]);
					sprintf(AsciiAddOn,"0x%08X",word.UW);
					sprintf(AsciiAddOn,"%c%c%c%c",
						word.UB[3]==0 ? ' ':word.UB[3],
						word.UB[2]==0 ? ' ':word.UB[2],
						word.UB[1]==0 ? ' ':word.UB[1],
						word.UB[0]==0 ? ' ':word.UB[0]);
				} else {
					strcpy(HexAddOn,"** ** ** **");
					strcpy(AsciiAddOn,"****");
				}
				if (count != 3) {
					strcat(HexAddOn,"-");
				}
				strcat(Hex,HexAddOn);
				strcat(Ascii,AsciiAddOn);
				location += 4;
			}
		} else {
			for (count = 0; count < 4; count ++) {
				if (location < 0x1FFFFFFC) {
					r4300i_LW_PAddr(location, &word.UW);
					sprintf(HexAddOn,"%02X %02X %02X %02X",word.UB[3],
						word.UB[2],word.UB[1],word.UB[0]);
					sprintf(AsciiAddOn,"0x%08X",word.UW);
					sprintf(AsciiAddOn,"%c%c%c%c",
						word.UB[3]==0 ? ' ':word.UB[3],
						word.UB[2]==0 ? ' ':word.UB[2],
						word.UB[1]==0 ? ' ':word.UB[1],
						word.UB[0]==0 ? ' ':word.UB[0]);
				} else {
					strcpy(HexAddOn,"** ** ** **");
					strcpy(AsciiAddOn,"****");
				}
				if (count != 3) {
					strcat(HexAddOn,"-");
				}
				strcat(Hex,HexAddOn);
				strcat(Ascii,AsciiAddOn);
				location += 4;
			}
		}
	} __except( r4300i_Command_MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		DisplayError(GS(MSG_UNKNOWN_MEM_ACTION));
		PostQuitMessage(0);
	}

	
	item.mask      = LVIF_TEXT;
	item.pszText   = Hex;
	item.iSubItem  = 1;
	OldItem.mask       = LVIF_TEXT;
	OldItem.iSubItem   = 1;
	ListView_GetItem(hList,&OldItem);
	if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
		ListView_SetItem(hList,&item);
	}	
	item.pszText   = Ascii;
	item.iSubItem  = 2;
	OldItem.iSubItem   = 2;
	ListView_GetItem(hList,&OldItem);
	if ( strcmp( item.pszText, OldItem.pszText ) != 0 ) {
		ListView_SetItem(hList,&item);
	} 
}

LRESULT CALLBACK Memory_Window_Proc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)  {	
	PAINTSTRUCT ps;
	RECT rcBox;

	switch (uMsg) {
	case WM_INITDIALOG:
		Memory_Win_hDlg = hDlg;
		Setup_Memory_Window ( hDlg );
		break;
	case WM_MOVE:
		if (IsIconic(hDlg)) { break; }
		StoreCurrentWinPos("Memory",hDlg);
		break;
	case WM_PAINT:
		BeginPaint( hDlg, &ps );
		SelectObject(ps.hdc, GetStockObject(ANSI_FIXED_FONT));
		SetBkMode( ps.hdc, TRANSPARENT );
		TextOut(ps.hdc,25,17,"Address:",8);
		rcBox.left   = 5;
		rcBox.top    = 5;
		rcBox.right  = 666;
		rcBox.bottom = 310;
		DrawEdge( ps.hdc, &rcBox, EDGE_RAISED, BF_RECT );
		rcBox.left   = 8;
		rcBox.top    = 8;
		rcBox.right  = 663;
		rcBox.bottom = 307;
		DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );
		EndPaint( hDlg, &ps );
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ADDR_EDIT:
			if (HIWORD(wParam) == EN_CHANGE ) {
				Refresh_Memory();
			}
			break;
		case IDC_VADDR:
		case IDC_PADDR:
			Refresh_Memory();
			break;
		case IDC_CLOSE_BUTTON:
		case IDCANCEL:
			EndDialog( hDlg, IDCANCEL );
			break;
		}
	case WM_VSCROLL:
		if ((HWND)lParam == hScrlBar) {
			unsigned int location;
			char Value[20];

			GetWindowText(hAddrEdit,Value,sizeof(Value));
			location = AsciiToHex(Value);
			switch (LOWORD(wParam))  {			
			case SB_LINEDOWN:
				if (location < 0xFFFFFFEF) {
					sprintf(Value,"%08X",location + 0x10);
					SetWindowText(hAddrEdit,Value);
				} else {
					sprintf(Value,"%08X",0xFFFFFFFF);
					SetWindowText(hAddrEdit,Value);
				}
				break;
			case SB_LINEUP:
				if (location > 0x10 ) {
					sprintf(Value,"%08X",location - 0x10);
					SetWindowText(hAddrEdit,Value);
				} else {
					sprintf(Value,"%08X",0);
					SetWindowText(hAddrEdit,Value);
				}
				break;
			case SB_PAGEDOWN:				
				if (location < 0xFFFFFEFF) {
					sprintf(Value,"%08X",location + 0x100);
					SetWindowText(hAddrEdit,Value);
				} else {
					sprintf(Value,"%08X",0xFFFFFFFF);
					SetWindowText(hAddrEdit,Value);
				}
				break;			
			case SB_PAGEUP:
				if (location > 0x100 ) {
					sprintf(Value,"%08X",location - 0x100);
					SetWindowText(hAddrEdit,Value);
				} else {
					sprintf(Value,"%08X",0);
					SetWindowText(hAddrEdit,Value);
				}
				break;
			}
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void __cdecl Refresh_Memory ( void ) {
	DWORD location;
	char Value[20];
	int count;

	if (InMemoryWindow == FALSE) { return; }

	GetWindowText(hAddrEdit,Value,sizeof(Value));
	location = (AsciiToHex(Value) >> 4);
	if (location > 0x0FFFFFF0) { location = 0x0FFFFFF0; }
	for (count = 0 ; count < 16;count ++ ){
		Insert_MemoryLineDump ( count + location , count );
	}
}

void Setup_Memory_Window (HWND hDlg) {
#define WindowWidth  683
#define WindowHeight 341
	DWORD X, Y;
	
	hVAddr = CreateWindowEx(0,"BUTTON", "Virtual Addressing", WS_CHILD | WS_VISIBLE | 
		BS_AUTORADIOBUTTON, 215,13,150,21,hDlg,(HMENU)IDC_VADDR,hInst,NULL );
	SendMessage(hVAddr,BM_SETCHECK, BST_CHECKED,0);
	
	hPAddr = CreateWindowEx(0,"BUTTON", "Physical Addressing", WS_CHILD | WS_VISIBLE | 
		BS_AUTORADIOBUTTON, 375,13,155,21,hDlg,(HMENU)IDC_PADDR,hInst,NULL );

	hList = CreateWindowEx(WS_EX_CLIENTEDGE,WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | 
		/*LVS_OWNERDRAWFIXED | */ LVS_REPORT | LVS_NOSORTHEADER, 14,39,622,261,hDlg,
		(HMENU)IDC_LIST_VIEW,hInst,NULL );
	if (hList) {
		LV_COLUMN  col;
		int count;

		col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		col.fmt  = LVCFMT_LEFT;

		col.pszText  = "Address";
		col.cx       = 90;
		col.iSubItem = 0;
		ListView_InsertColumn ( hList, 0, &col);

		col.pszText  = "Memory Dump";
		col.cx       = 390;
		col.iSubItem = 1;
		ListView_InsertColumn ( hList, 1, &col);

		col.cx       = 140;
		col.pszText  = "Memory Ascii";
		col.iSubItem = 2;
		ListView_InsertColumn ( hList, 2, &col);
		ListView_SetItemCount ( hList, 16);
		SendMessage(hList,WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT),0);
		for (count = 0 ; count < 16;count ++ ){
			Insert_MemoryLineDump (count,count);
		}

	}	
	
	SetWindowPos(GetDlgItem(hDlg,IDC_CLOSE_BUTTON),NULL, 560,13,90,21, SWP_NOZORDER | 
		SWP_SHOWWINDOW);
	
	hAddrEdit = GetDlgItem(hDlg,IDC_ADDR_EDIT);
	SetWindowText(hAddrEdit,"00000000");
	SendMessage(hAddrEdit,EM_SETLIMITTEXT,(WPARAM)8,(LPARAM)0);
	SetWindowPos(hAddrEdit,NULL, 100,13,100,21, SWP_NOZORDER | SWP_SHOWWINDOW);
	SendMessage(hAddrEdit,WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT),0);

	hScrlBar = CreateWindowEx(WS_EX_STATICEDGE, "SCROLLBAR","", WS_CHILD | WS_VISIBLE | 
		WS_TABSTOP | SBS_VERT, 635,39,18,260, hDlg, (HMENU)IDC_SCRL_BAR, hInst, NULL );
	if (hScrlBar) {
		SCROLLINFO si;

		si.cbSize = sizeof(si);
		si.fMask  = SIF_RANGE | SIF_POS | SIF_PAGE;
		si.nMin   = 0;
		si.nMax   = 300;
		si.nPos   = 145;
		si.nPage  = 10;
		SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);		
	} 
	
	if ( !GetStoredWinPos( "Memory", &X, &Y ) ) {
		X = (GetSystemMetrics( SM_CXSCREEN ) - WindowWidth) / 2;
		Y = (GetSystemMetrics( SM_CYSCREEN ) - WindowHeight) / 2;
	}
	
	SetWindowPos(hDlg,NULL,X,Y,WindowWidth,WindowHeight, SWP_NOZORDER | SWP_SHOWWINDOW);		 
	
}
#endif

#endif