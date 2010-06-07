#include "stdafx.h"

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
#include "main.h"
#include "CPU.h"
#include "debugger.h"
#include "plugin.h"

int CheckForR4300iBPoint ( DWORD Location ) {
	return FALSE;
}

int	NoOfBpoints;


#ifdef OLD_CODE
#if (!defined(EXTERNAL_RELEASE))
#include <windows.h>

#include <commctrl.h>
#include <stdio.h>
#include "main.h"
#include "CPU.h"
#include "debugger.h"
#include "plugin.h"

#define IDC_LIST				100
#define IDC_TAB_CONTROL			101
#define IDC_REMOVE_BUTTON		103
#define IDC_REMOVEALL_BUTTON	104
#define IDC_LOCATION_EDIT		105
#define IDC_FUNCTION_COMBO		106

void BPoint_AddButtonPressed ( void);
void Create_BPoint_Window    ( int );
void DrawBPItem              ( LPARAM );
void HideBPointPanel         ( int Panel);
void Paint_BPoint_Win        ( HWND );
void __cdecl RefreshBreakPoints      (void);
void Setup_BPoint_Win        ( HWND );
void ShowBPointPanel         ( int Panel);
LRESULT CALLBACK BPoint_Proc   ( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK RefreshBPProc ( HWND, UINT, WPARAM, LPARAM );

static HWND BPoint_Win_hDlg, hTab, hList, hStatic, hR4300iLocation, hFunctionlist,
    hAddButton, hRemoveButton, hRemoveAllButton;
static BOOL InBPWindow = FALSE;
static FARPROC RefProc;
int RSPBP_count;

int Add_R4300iBPoint( DWORD Location, int Confirm ) {
	int count;

	if (NoOfBpoints == MaxBPoints) { 
		DisplayError("Max amount of Break Points set");
		return FALSE;
	}

	for (count = 0; count < NoOfBpoints; count ++) {
		if (BPoint[count].Location == Location) {
			DisplayError("You already have this Break Point");
			return FALSE;
		}
	}

	if (Confirm) {
		char Message[150], Label[100];
		int Response;

		sprintf(Label,"%s", LabelName(Location));
		sprintf(Message,"0x%08X", Location);
		if(strcmp(Label,Message) == 0) {
			sprintf(Message,"Break when:\n\nR4300i's Program Counter = 0x%08X\n\nIs this correct?",
				Location); 
		} else {
			sprintf(Message,"Break on:\n\n%s (R4300i PC = 0x%08X)\n\nIs this correct?",
				Label, Location); 
		}
		Response = MessageBox(BPoint_Win_hDlg, Message, AppName, MB_YESNO | MB_ICONINFORMATION);
		if (Response == IDNO) {
			return FALSE;
		}
	}
	BPoint[NoOfBpoints].Location = Location;
	NoOfBpoints += 1;
	RefreshBreakPoints();
	/*if (CPU_Action.Stepping || hMipsCPU == NULL) {
		ClearAllx86Code();	
	} else {
		CPU_Action.ResetX86Code = TRUE;
		CPU_Action.do_or_check_something += 1; 
	}*/	
	return TRUE;
}

void BPoint_AddButtonPressed (void) {
	DWORD Selected, Location;
	char Title[10];
	TC_ITEM item;

	item.mask = TCIF_PARAM;
	TabCtrl_GetItem( hTab, TabCtrl_GetCurSel( hTab ), &item );
	switch( item.lParam ) {
	case R4300i_BP:
		GetWindowText(hR4300iLocation,Title,sizeof(Title));
		if (!Add_R4300iBPoint(AsciiToHex(Title),TRUE )) {
			SendMessage(hR4300iLocation,EM_SETSEL,(WPARAM)0,(LPARAM)-1);
			SetFocus(hR4300iLocation);	
		}
		break;
	case R4300i_FUNCTION:
		Selected = SendMessage(hFunctionlist,CB_GETCURSEL,0,0);
		Location = SendMessage(hFunctionlist,CB_GETITEMDATA,(WPARAM)Selected,0);
		Add_R4300iBPoint(Location,TRUE );
		SetFocus(hFunctionlist);	
		break;
	case RSP_BP:
		if (RspDebug.UseBPoints) { RspDebug.Add_BPoint(); }
		break;
	}

}

LRESULT CALLBACK BPoint_Proc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {	
	static RECT rcDisp;
	static int CurrentPanel = R4300i_BP;
	int selected;
	TC_ITEM item;

	switch (uMsg) {
	case WM_INITDIALOG:
		BPoint_Win_hDlg = hDlg;
		Setup_BPoint_Win( hDlg );
		RefreshBreakPoints();
		break;
	case WM_MOVE:
		StoreCurrentWinPos("Break Point",hDlg);
		break;
	case WM_SIZE:
		GetClientRect( hDlg, &rcDisp);
		TabCtrl_AdjustRect( hTab, FALSE, &rcDisp );
		break;
	case WM_PAINT:
		Paint_BPoint_Win( hDlg );
		return TRUE;
	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code) {
		case TCN_SELCHANGE:
			InvalidateRect( hTab, &rcDisp, TRUE );
			HideBPointPanel (CurrentPanel);			
			item.mask = TCIF_PARAM;
			TabCtrl_GetItem( hTab, TabCtrl_GetCurSel( hTab ), &item );
			CurrentPanel = item.lParam;
			InvalidateRect( hStatic, NULL, FALSE );
			ShowBPointPanel ( CurrentPanel );			
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			BPoint_AddButtonPressed();
			break;
		case IDC_REMOVE_BUTTON:
			selected = SendMessage(hList,LB_GETCURSEL,0,0);
			if (selected < NoOfBpoints) {
				DWORD location;
				location = SendMessage(hList,LB_GETITEMDATA,selected,0);
				RemoveR4300iBreakPoint(location);
				break;
			}
			selected -= NoOfBpoints;
			if (selected < RSPBP_count) {
				RspDebug.RemoveBpoint(hList,SendMessage(hList,LB_GETCURSEL,0,0));
				break;
			}
			DisplayError("what is this BP");
			break;
		case IDC_REMOVEALL_BUTTON:
			NoOfBpoints = 0;
			if (RspDebug.UseBPoints) { RspDebug.RemoveAllBpoint(); }
			RefreshBreakPoints();
			RefreshR4300iCommands();
			break;
		case IDCANCEL:
			CurrentPanel = R4300i_BP;
			EndDialog( hDlg, IDCANCEL );
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int CheckForR4300iBPoint ( DWORD Location ) {
	int count;
	
	for (count = 0; count < NoOfBpoints; count ++){
		if (BPoint[count].Location == Location) {
			return TRUE;
		}
	}
	return FALSE;
}

void Create_BPoint_Window (int Child) {
	DWORD ThreadID;

	if (Child) {
		InBPWindow = TRUE;
		DialogBox( hInst, "BLANK", NULL,(DLGPROC) BPoint_Proc );
		InBPWindow = FALSE;
	} else {
		if (!InBPWindow) {
			CloseHandle(CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Create_BPoint_Window,
				(LPVOID)TRUE,0, &ThreadID));	
		} else {
			BringWindowToTop(BPoint_Win_hDlg);
		}	
	}
}

void __cdecl Enter_BPoint_Window ( void ) {
    Create_BPoint_Window ( FALSE );
}

void HideBPointPanel ( int Panel) {
	switch( Panel ) {
	case R4300i_BP: ShowWindow(hR4300iLocation, FALSE); break;
	case R4300i_FUNCTION: ShowWindow(hFunctionlist, FALSE); break;
	case RSP_BP: if (RspDebug.UseBPoints) { RspDebug.HideBPPanel(); } break;
	}
}

void Paint_BPoint_Win (HWND hDlg) {
	RECT rcBox;
	PAINTSTRUCT ps;
	HFONT hOldFont;
	int OldBkMode;

	BeginPaint( hDlg, &ps );
	rcBox.left   = 5;
	rcBox.top    = 175;
	rcBox.right  = 255;
	rcBox.bottom = 290;
	DrawEdge( ps.hdc, &rcBox, EDGE_RAISED, BF_RECT );
	rcBox.left   = 8;
	rcBox.top    = 178;
	rcBox.right  = 252;
	rcBox.bottom = 287;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );
	
	hOldFont = SelectObject(ps.hdc,GetStockObject(DEFAULT_GUI_FONT));
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );
		
	TextOut( ps.hdc, 9,159,"Breakpoints: ",13);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );

	EndPaint( hDlg, &ps );
}

void __cdecl RefreshBreakPoints (void) {
	char Message[100];
	int count;

	if (!InBPWindow) { return; }

	SendMessage(hList,LB_RESETCONTENT,0,0);
	for (count = 0; count < NoOfBpoints; count ++ ) {
		sprintf(Message," at 0x%08X (r4300i)", BPoint[count].Location);
		SendMessage(hList,LB_ADDSTRING,0,(LPARAM)Message);	
		SendMessage(hList,LB_SETITEMDATA,count,(LPARAM)BPoint[count].Location);	
	}
	count = SendMessage(hList,LB_GETCOUNT,0,0);
	if (RspDebug.UseBPoints) { RspDebug.RefreshBpoints(hList); }
	RSPBP_count = SendMessage(hList,LB_GETCOUNT,0,0) - count;

	if (SendMessage(hList,LB_GETCOUNT,0,0) > 0) {
		EnableWindow( hRemoveButton, TRUE );
		EnableWindow( hRemoveAllButton, TRUE );
	}
}

LRESULT CALLBACK RefreshBPProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	TC_ITEM item;

	switch( uMsg ) {
	case WM_PAINT:
		BeginPaint( hWnd, &ps );
		rcBox.left   = 15;
		rcBox.top    = 40;
		rcBox.right  = 235;
		rcBox.bottom = 125;
		DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

		hOldFont = SelectObject( ps.hdc,
			GetStockObject(DEFAULT_GUI_FONT) );
		OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

		item.mask = TCIF_PARAM;
		TabCtrl_GetItem( hTab, TabCtrl_GetCurSel( hTab ), &item );
		switch (item.lParam) {
		case R4300i_BP:
			TextOut( ps.hdc, 29,60,"Break when the Program Counter equals",37);
			TextOut( ps.hdc, 59,85,"0x",2);
			break;
		case R4300i_FUNCTION:
			TextOut( ps.hdc, 75,60,"Break on label:",15);
			break;
		case RSP_BP:
			if (RspDebug.UseBPoints) { RspDebug.PaintBPPanel(ps); }
			break;
		}
		
		SelectObject( ps.hdc,hOldFont );
		SetBkMode( ps.hdc, OldBkMode );

		EndPaint( hWnd, &ps );
		break;
	default:
		return( (*RefProc)(hWnd, uMsg, wParam, lParam) );
	}
	return( FALSE );
}

void RemoveR4300iBreakPoint (DWORD Location) {
	int count, location = -1;
	
	for (count = 0; count < NoOfBpoints; count ++){
		if (BPoint[count].Location == Location) {
			location = count;
			count = NoOfBpoints;
		}
	}
	
	if (location >= 0) {
		for (count = location; count < NoOfBpoints - 1; count ++ ){ 
			BPoint[count].Location = BPoint[count + 1].Location;
		}
		NoOfBpoints -= 1;
		RefreshBreakPoints ();
	}
	
	/*if (CPU_Action.Stepping || hMipsCPU == NULL) {
		ClearAllx86Code();	
	} else {
		CPU_Action.ResetX86Code = TRUE;
		CPU_Action.do_or_check_something += 1; 
	}*/
}

void Setup_BPoint_Win (HWND hDlg) {
#define WindowWidth  350
#define WindowHeight 320
	DWORD X, Y;

	hAddButton = CreateWindowEx(0,"BUTTON","&Add",
		BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		262,26,75,22,hDlg,(HMENU)IDOK,
		hInst,NULL );	
	if ( hAddButton ) {
		SendMessage(hAddButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hRemoveButton = CreateWindowEx(0,"BUTTON","&Remove",
		WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | 
		WS_DISABLED,262,177,75,22,hDlg,(HMENU)IDC_REMOVE_BUTTON,
		hInst,NULL );	
	if ( hRemoveButton ) {
		SendMessage(hRemoveButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hRemoveAllButton = CreateWindowEx(0,"BUTTON","Remove A&ll", WS_CHILD |
		BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,262,202,75,22,hDlg,
		(HMENU)IDC_REMOVEALL_BUTTON,hInst,NULL );	
	if ( hRemoveAllButton ) {
		SendMessage(hRemoveAllButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hTab = CreateWindowEx(0,WC_TABCONTROL,"", WS_CHILD | WS_TABSTOP | WS_VISIBLE,
		5,6,250,150,hDlg,(HMENU)IDC_TAB_CONTROL,hInst,NULL );
	if (hTab) {
		TC_ITEM item;
		
		SendMessage(hTab, WM_SETFONT, (WPARAM)GetStockObject( DEFAULT_GUI_FONT ), 0 );
		item.mask    = TCIF_TEXT | TCIF_PARAM;
		item.pszText = " R4300i ";
		item.lParam  = R4300i_BP;
		TabCtrl_InsertItem( hTab,0, &item);		
		if (NoOfMapEntries != 0) {
			item.mask    = TCIF_TEXT | TCIF_PARAM;
			item.pszText = " Function ";
			item.lParam  = R4300i_FUNCTION;
			TabCtrl_InsertItem( hTab,1, &item);		
		}
		if (RspDebug.UseBPoints) { 
			RECT rcBox;
			rcBox.left   = 15;  rcBox.top    = 40;
			rcBox.right  = 235; rcBox.bottom = 125;
			item.mask    = TCIF_TEXT | TCIF_PARAM;
			item.pszText = RspDebug.BPPanelName;
			item.lParam  = RSP_BP;
			TabCtrl_InsertItem( hTab,2, &item);	
			RspDebug.CreateBPPanel(hDlg,rcBox);
		}
	}

	hR4300iLocation = CreateWindowEx(0,"EDIT","", WS_CHILD | WS_VISIBLE | WS_BORDER | 
		ES_UPPERCASE | WS_TABSTOP,83,90,100,17,hDlg,(HMENU)IDC_LOCATION_EDIT,hInst,NULL);		
	if (hR4300iLocation) {
		char Title[20];
		SendMessage(hR4300iLocation,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		SendMessage(hR4300iLocation,EM_SETLIMITTEXT,(WPARAM)8,(LPARAM)0);
		sprintf(Title,"%08X",PROGRAM_COUNTER);
		SetWindowText(hR4300iLocation,Title);
	}

	hFunctionlist = CreateWindowEx(0,"COMBOBOX","", WS_CHILD | WS_VSCROLL |
		CBS_DROPDOWNLIST | CBS_SORT | WS_TABSTOP,55,90,150,150,hDlg,
		(HMENU)IDC_FUNCTION_COMBO,hInst,NULL);		
	if (hFunctionlist) {
		DWORD count, pos;
		
		SendMessage(hFunctionlist,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		for (count = 0; count < NoOfMapEntries; count ++ ) {
			pos = SendMessage(hFunctionlist,CB_ADDSTRING,(WPARAM)0,(LPARAM)MapTable[count].Label);
			SendMessage(hFunctionlist,CB_SETITEMDATA,(WPARAM)pos,(LPARAM)MapTable[count].VAddr);
		}
		SendMessage(hFunctionlist,CB_SETCURSEL,(WPARAM)1,(LPARAM)0);
	}

	hList = CreateWindowEx(WS_EX_STATICEDGE, "LISTBOX","", WS_CHILD | WS_VISIBLE | LBS_DISABLENOSCROLL |WS_VSCROLL,
		16,187,228,112, hDlg, (HMENU)IDC_LIST, hInst,NULL );
	if ( hList) {
		SendMessage(hList,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hStatic = CreateWindowEx(0,"STATIC","", WS_CHILD | WS_VISIBLE, 5,6,250,150,hDlg,0,hInst,NULL );
	RefProc = (FARPROC)SetWindowLong( hStatic,GWL_WNDPROC,(long)RefreshBPProc);
	
	SetWindowText(hDlg," Breakpoints");

	if ( !GetStoredWinPos( "Break Point", &X, &Y ) ) {
		X = (GetSystemMetrics( SM_CXSCREEN ) - WindowWidth) / 2;
		Y = (GetSystemMetrics( SM_CYSCREEN ) - WindowHeight) / 2;
	}
	SetWindowPos(hDlg,NULL,X,Y,WindowWidth,WindowHeight, SWP_NOZORDER | SWP_SHOWWINDOW);
}

void ShowBPointPanel ( int Panel) {
	switch( Panel ) {
	case R4300i_BP: ShowWindow(hR4300iLocation, TRUE); break;
	case R4300i_FUNCTION: ShowWindow(hFunctionlist, TRUE); break;
	case RSP_BP: if (RspDebug.UseBPoints) { RspDebug.ShowBPPanel(); } break;
	}
}

void UpdateBPointGUI (void) {
	TC_ITEM item;
	DWORD count;

	if (!InBPWindow) { return; }
	
	if (TabCtrl_GetCurSel(hTab) != 0) {
		InvalidateRect( hTab, NULL, TRUE );
		item.mask = TCIF_PARAM;
		TabCtrl_GetItem( hTab, TabCtrl_GetCurSel( hTab ), &item );
		HideBPointPanel (item.lParam);
		TabCtrl_SetCurSel(hTab, 0);
		TabCtrl_GetItem( hTab, TabCtrl_GetCurSel( hTab ), &item );
		InvalidateRect( hStatic, NULL, FALSE );
		ShowBPointPanel ( item.lParam );
	}
	
	for (count = TabCtrl_GetItemCount(hTab); count > 1; count--) {
		TabCtrl_DeleteItem(hTab, count - 1);
	}

	if (NoOfMapEntries > 0) {
		item.mask    = TCIF_TEXT | TCIF_PARAM;
		item.pszText = " Function ";
		item.lParam  = R4300i_FUNCTION;
		TabCtrl_InsertItem( hTab,1, &item);
	}
	if (RspDebug.UseBPoints) { 
		RECT rcBox;
		rcBox.left   = 15;  rcBox.top    = 40;
		rcBox.right  = 235; rcBox.bottom = 125;
		item.mask    = TCIF_TEXT | TCIF_PARAM;
		item.pszText = RspDebug.BPPanelName;
		item.lParam  = RSP_BP;
		TabCtrl_InsertItem( hTab,2, &item);	
		RspDebug.CreateBPPanel(BPoint_Win_hDlg, rcBox);
	}
	InvalidateRect( BPoint_Win_hDlg, NULL, TRUE );
	RefreshBreakPoints ();
}

void UpdateBP_FunctionList (void) {
	DWORD pos, count;
	
	if (!InBPWindow) { return; }

	SendMessage(hFunctionlist,CB_RESETCONTENT,(WPARAM)0,(LPARAM)0);
	for (count = 0; count < NoOfMapEntries; count ++ ) {
		pos = SendMessage(hFunctionlist,CB_ADDSTRING,(WPARAM)0,(LPARAM)MapTable[count].Label);
		SendMessage(hFunctionlist,CB_SETITEMDATA,(WPARAM)pos,(LPARAM)MapTable[count].VAddr);
	}
	SendMessage(hFunctionlist,CB_SETCURSEL,(WPARAM)1,(LPARAM)0);
	UpdateBPointGUI();
}
#endif
#endif