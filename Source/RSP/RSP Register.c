/*
 * RSP Compiler plug in for Project64 (A Nintendo 64 emulator).
 *
 * (c) Copyright 2001 jabo (jabo@emulation64.com) and
 * zilmar (zilmar@emulation64.com)
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

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "Rsp.h"
#include "Types.h"

#define GeneralPurpose			1
#define ControlProcessor0		2
#define HiddenRegisters		    3
#define Vector1					4
#define Vector2					5

#define IDC_TAB_CONTROL			1000

void Create_RSP_Register_Window ( int );
void HideRSP_RegisterPanel    ( int );
void PaintRSP_HiddenPanel     ( HWND );
void PaintRSP_CP0Panel        ( HWND );
void PaintRSP_GPRPanel        ( HWND );
void PaintRSP_Vector1_Panel   ( HWND );
void PaintRSP_Vector2_Panel   ( HWND );
void ShowRSP_RegisterPanel    ( int );
void SetupRSP_HiddenPanel     ( HWND );
void SetupRSP_CP0Panel        ( HWND );
void SetupRSP_GPRPanel        ( HWND );
void SetupRSP_RegistersMain   ( HWND );
void SetupRSP_Vect1Panel      ( HWND );
void SetupRSP_Vect2Panel      ( HWND );
void UpdateRSPRegistersScreen ( void );

LRESULT CALLBACK RefreshRSP_RegProc ( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK RSP_Registers_Proc ( HWND, UINT, WPARAM, LPARAM );

HWND RSP_Registers_hDlg, hTab, hStatic, hGPR[32], hCP0[16], hHIDDEN[12],
	hVECT1[16], hVECT2[16];
int InRSPRegisterWindow = FALSE;
FARPROC RefreshProc;

/*** RSP Registers ***/
UWORD32   RSP_GPR[32], RSP_Flags[4];
UDWORD  RSP_ACCUM[8];
VECTOR  RSP_Vect[32];

char * GPR_Strings[32] = {
	"R0", "AT", "V0", "V1", "A0", "A1", "A2", "A3",
	"T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7",
	"S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7",
	"T8", "T9", "K0", "K1", "GP", "SP", "S8", "RA"
};

void Create_RSP_Register_Window ( int Child ) {
	DWORD ThreadID;
	if ( Child ) {
		InRSPRegisterWindow = TRUE;
		DialogBox( hinstDLL, "RSPREGISTERS", NULL,(DLGPROC) RSP_Registers_Proc );
		InRSPRegisterWindow = FALSE;
	} else {
		if (!InRSPRegisterWindow) {
			CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Create_RSP_Register_Window,
				(LPVOID)TRUE,0, &ThreadID);
		} else {
			SetForegroundWindow(RSP_Registers_hDlg);
		}	
	}
}

void Enter_RSP_Register_Window ( void ) {
    Create_RSP_Register_Window ( FALSE );
}

void HideRSP_RegisterPanel ( int Panel) {
	int count;

	switch( Panel ) {
	case GeneralPurpose:
		for (count = 0; count < 32;count ++) { ShowWindow(hGPR[count], FALSE ); }
		break;
	case ControlProcessor0:
		for (count = 0; count < 16;count ++) { ShowWindow(hCP0[count], FALSE ); }
		break;
	case HiddenRegisters:
		for (count = 0; count < 12;count ++) { ShowWindow(hHIDDEN[count], FALSE ); }
		break;
	case Vector1:
		for (count = 0; count < 16;count ++) { ShowWindow(hVECT1[count], FALSE ); }
		break;
	case Vector2:
		for (count = 0; count < 16;count ++) { ShowWindow(hVECT2[count], FALSE ); }
		break;
	}
}

void InitilizeRSPRegisters (void) {
	memset(RSP_GPR,0,sizeof(RSP_GPR));
	memset(RSP_Vect,0,sizeof(RSP_Vect));
}

void PaintRSP_HiddenPanel (HWND hWnd) {	
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 66; rcBox.top    = 39;
	rcBox.right  = 320; rcBox.bottom = 265;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );
	
	rcBox.left   = 350; rcBox.top    = 39;
	rcBox.right  = 510; rcBox.bottom = 265;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	rcBox.left   = 75; rcBox.top    = 35;
	rcBox.right  = 150; rcBox.bottom = 50;
	FillRect( ps.hdc, &rcBox,(HBRUSH)COLOR_WINDOW);

	rcBox.left   = 365; rcBox.top    = 35;
	rcBox.right  = 425; rcBox.bottom = 50;
	FillRect( ps.hdc, &rcBox,(HBRUSH)COLOR_WINDOW);

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 80,34,"Accumulators",12);
	TextOut( ps.hdc, 80,56,"Accumulator 0:",14);
	TextOut( ps.hdc, 80,81,"Accumulator 1:",14);
	TextOut( ps.hdc, 80,106,"Accumulator 2:",14);
	TextOut( ps.hdc, 80,131,"Accumulator 3:",14);
	TextOut( ps.hdc, 80,156,"Accumulator 4:",14);
	TextOut( ps.hdc, 80,181,"Accumulator 5:",14);
	TextOut( ps.hdc, 80,206,"Accumulator 6:",14);
	TextOut( ps.hdc, 80,231,"Accumulator 7:",14);
		
	TextOut( ps.hdc, 371,34,"RSP Flags",9);
	TextOut( ps.hdc, 375,86,"Flag 0:",7);
	TextOut( ps.hdc, 375,116,"Flag 2:",7);
	TextOut( ps.hdc, 375,146,"Flag 3:",7);
	TextOut( ps.hdc, 375,176,"Flag 4:",7);

	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintRSP_CP0Panel (HWND hWnd) {	
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 41;
	rcBox.top    = 29;
	rcBox.right  = 573;
	rcBox.bottom = 275;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 53,48,"Reg 0 - SP memory address:",26);
	TextOut( ps.hdc, 53,76,"Reg 1 - SP DRAM DMA address:",28);
	TextOut( ps.hdc, 53,104,"Reg 2 - SP read DMA length:",27);
	TextOut( ps.hdc, 53,132,"Reg 3 - SP write DMA length:",28);
	TextOut( ps.hdc, 53,160,"Reg 4 - SP status:",18);
	TextOut( ps.hdc, 53,188,"Reg 5 - SP DMA full:",20);
	TextOut( ps.hdc, 53,216,"Reg 6 - SP DMA busy:",20);
	TextOut( ps.hdc, 53,244,"Reg 7 - SP semaphore:",21);
	TextOut( ps.hdc, 313,48,"Reg 8 - DP CMD DMA start:",25);
	TextOut( ps.hdc, 313,76,"Reg 9 - DP CMD DMA end:",23);
	TextOut( ps.hdc, 313,104,"Reg 10 - DP CMD DMA current:",28);
	TextOut( ps.hdc, 313,132,"Reg 11 - DP CMD status:",23);
	TextOut( ps.hdc, 313,160,"Reg 12 - DP clock counter:",26);
	TextOut( ps.hdc, 313,188,"Reg 13 - DP buffer busy counter:",32);
	TextOut( ps.hdc, 313,216,"Reg 14 - DP pipe busy counter:",30);
	TextOut( ps.hdc, 313,244,"Reg 15 - DP TMEM load counter:",30);
	
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintRSP_GPRPanel (HWND hWnd) {
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 41;
	rcBox.top    = 29;
	rcBox.right  = 573;
	rcBox.bottom = 275;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 53,46,"R0 - Reg 0:",11);
	TextOut( ps.hdc, 53,66,"AT - Reg 1:",11);
	TextOut( ps.hdc, 53,86,"V0 - Reg 2:",11);
	TextOut( ps.hdc, 53,106,"V1 - Reg 3:",11);
	TextOut( ps.hdc, 53,126,"A0 - Reg 4:",11);
	TextOut( ps.hdc, 53,146,"A1 - Reg 5:",11);
	TextOut( ps.hdc, 53,166,"A2 - Reg 6:",11);
	TextOut( ps.hdc, 53,186,"A3 - Reg 7:",11);
	TextOut( ps.hdc, 53,206,"T0 - Reg 8:",11);
	TextOut( ps.hdc, 53,226,"T1 - Reg 9:",11);
	TextOut( ps.hdc, 53,246,"T2 - Reg 10:",12);
	TextOut( ps.hdc, 228,46,"T3 - Reg 11:",12);
	TextOut( ps.hdc, 228,66,"T4 - Reg 12:",12);
	TextOut( ps.hdc, 228,86,"T5 - Reg 13:",12);
	TextOut( ps.hdc, 228,106,"T6 - Reg 14:",12);
	TextOut( ps.hdc, 228,126,"T7 - Reg 15:",12);
	TextOut( ps.hdc, 228,146,"S0 - Reg 16:",12);
	TextOut( ps.hdc, 228,166,"S1 - Reg 17:",12);
	TextOut( ps.hdc, 228,186,"S2 - Reg 18:",12);
	TextOut( ps.hdc, 228,206,"S3 - Reg 19:",12);
	TextOut( ps.hdc, 228,226,"S4 - Reg 20:",12);
	TextOut( ps.hdc, 228,246,"S5 - Reg 21:",12);
	TextOut( ps.hdc, 408,46,"S6 - Reg 22:",12);
	TextOut( ps.hdc, 408,66,"S7 - Reg 23:",12);
	TextOut( ps.hdc, 408,86,"T8 - Reg 24:",12);
	TextOut( ps.hdc, 408,106,"T9 - Reg 25:",12);
	TextOut( ps.hdc, 408,126,"K0 - Reg 26:",12);
	TextOut( ps.hdc, 408,146,"K1 - Reg 27:",12);
	TextOut( ps.hdc, 408,166,"GP - Reg 28:",12);
	TextOut( ps.hdc, 408,186,"SP - Reg 29:",12);
	TextOut( ps.hdc, 408,206,"S8 - Reg 30:",12);
	TextOut( ps.hdc, 408,226,"RA - Reg 31:",12);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintRSP_Vector1_Panel (HWND hWnd) {
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 10;
	rcBox.top    = 29;
	rcBox.right  = 606;
	rcBox.bottom = 275;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 20,48,"$v0:",4);
	TextOut( ps.hdc, 20,76,"$v1:",4);
	TextOut( ps.hdc, 20,104,"$v2:",4);
	TextOut( ps.hdc, 20,132,"$v3:",4);
	TextOut( ps.hdc, 20,160,"$v4:",4);
	TextOut( ps.hdc, 20,188,"$v5:",4);
	TextOut( ps.hdc, 20,216,"$v6:",4);
	TextOut( ps.hdc, 20,244,"$v7:",4);
	TextOut( ps.hdc, 310,48,"$v8:",4);
	TextOut( ps.hdc, 310,76,"$v9:",4);
	TextOut( ps.hdc, 310,104,"$v10:",5);
	TextOut( ps.hdc, 310,132,"$v11:",5);
	TextOut( ps.hdc, 310,160,"$v12:",5);
	TextOut( ps.hdc, 310,188,"$v13:",5);
	TextOut( ps.hdc, 310,216,"$v14:",5);
	TextOut( ps.hdc, 310,244,"$v15:",5);
	
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintRSP_Vector2_Panel (HWND hWnd) {
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 10;
	rcBox.top    = 29;
	rcBox.right  = 606;
	rcBox.bottom = 275;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 20,48,"$v16:",5);
	TextOut( ps.hdc, 20,76,"$v17:",5);
	TextOut( ps.hdc, 20,104,"$v18:",5);
	TextOut( ps.hdc, 20,132,"$v19:",5);
	TextOut( ps.hdc, 20,160,"$v20:",5);
	TextOut( ps.hdc, 20,188,"$v21:",5);
	TextOut( ps.hdc, 20,216,"$v22:",5);
	TextOut( ps.hdc, 20,244,"$v23:",5);
	TextOut( ps.hdc, 310,48,"$v24:",5);
	TextOut( ps.hdc, 310,76,"$v25:",5);
	TextOut( ps.hdc, 310,104,"$v26:",5);
	TextOut( ps.hdc, 310,132,"$v27:",5);
	TextOut( ps.hdc, 310,160,"$v28:",5);
	TextOut( ps.hdc, 310,188,"$v29:",5);
	TextOut( ps.hdc, 310,216,"$v30:",5);
	TextOut( ps.hdc, 310,244,"$v31:",5);
	
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

LRESULT CALLBACK RefreshRSP_RegProc ( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	int nSel;
	TC_ITEM item;

	switch( uMsg ) {
	case WM_PAINT:
		nSel = TabCtrl_GetCurSel( hTab );
		if ( nSel > -1 ) {
			item.mask = TCIF_PARAM;
			TabCtrl_GetItem( hTab, nSel, &item );
			switch( item.lParam ) {
			case GeneralPurpose:
				PaintRSP_GPRPanel (hWnd);
				break;
			case ControlProcessor0:
				PaintRSP_CP0Panel (hWnd);
				break;
			case HiddenRegisters:
				PaintRSP_HiddenPanel (hWnd);
				break;
			case Vector1:
				PaintRSP_Vector1_Panel (hWnd);
				break;
			case Vector2:
				PaintRSP_Vector2_Panel (hWnd);
				break;
			}
		}
		break;
	}

	return CallWindowProc(RefreshProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK RSP_Registers_Proc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {	
	static RECT rcDisp;
	static int CurrentPanel = GeneralPurpose;
	TC_ITEM item;

	switch (uMsg) {
	case WM_INITDIALOG:
		RSP_Registers_hDlg = hDlg;
		SetupRSP_RegistersMain( hDlg );
		break;
	case WM_MOVE:
		//StoreCurrentWinPos("RSP Registers",hDlg);
		break;
	case WM_SIZE:
		GetClientRect( hDlg, &rcDisp);
		TabCtrl_AdjustRect( hTab, FALSE, &rcDisp );
		break;
	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code) {
		case TCN_SELCHANGE:
			InvalidateRect( hTab, &rcDisp, TRUE );
			HideRSP_RegisterPanel (CurrentPanel);
			item.mask = TCIF_PARAM;
			TabCtrl_GetItem( hTab, TabCtrl_GetCurSel( hTab ), &item );
			CurrentPanel = (int)item.lParam;
			InvalidateRect( hStatic, NULL, FALSE );
			UpdateRSPRegistersScreen();
			ShowRSP_RegisterPanel ( CurrentPanel );
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			CurrentPanel = GeneralPurpose;
			EndDialog( hDlg, IDCANCEL );
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

void SetupRSP_HiddenPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 8;count ++) {
		hHIDDEN[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,170,(count*25) + 60,140,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hHIDDEN[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 4;count ++) {
		hHIDDEN[count + 8] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,425,(count*30) + 90,55,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hHIDDEN[count + 8],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupRSP_CP0Panel (HWND hDlg) {
	int count;

	for (count = 0; count < 8;count ++) {
		hCP0[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,225,(count*28) + 53,75,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hCP0[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 8;count ++) {
		hCP0[count + 8] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,485,(count*28) + 53,75,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hCP0[ count + 8 ],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupRSP_GPRPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 11;count ++) {
		hGPR[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,135,(count*20) + 50,75,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hGPR[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 11;count ++) {
		hGPR[count + 11] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,315,(count*20) + 50,75,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hGPR[ count + 11 ],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 10;count ++) {
		hGPR[count + 22] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD |  
			ES_READONLY | WS_BORDER | WS_TABSTOP,485,(count*20) + 50,75,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hGPR[ count + 22 ],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupRSP_RegistersMain (HWND hDlg) {
#define WindowWidth  630
#define WindowHeight 325
	DWORD X, Y;

	hTab = CreateWindowEx(0,WC_TABCONTROL,"", WS_TABSTOP | WS_CHILD | WS_VISIBLE,5,6,616,290,
		hDlg,(HMENU)IDC_TAB_CONTROL,hinstDLL,NULL );
	if ( hTab ) {
		TC_ITEM item;
		SendMessage(hTab, WM_SETFONT, (WPARAM)GetStockObject( DEFAULT_GUI_FONT ), 0);
		item.mask    = TCIF_TEXT | TCIF_PARAM;
		item.pszText = " General Purpose ";
		item.lParam  = GeneralPurpose;
		TabCtrl_InsertItem( hTab,0, &item);
		item.lParam  = ControlProcessor0;
		item.pszText = " Control Processor 0 ";
		TabCtrl_InsertItem( hTab,1, &item);
		item.lParam  = HiddenRegisters;
		item.pszText = " Hidden Registers ";
		TabCtrl_InsertItem( hTab,2, &item);
		item.lParam  = Vector1;
		item.pszText = " RSP Vectors $v0 - $v15 ";
		TabCtrl_InsertItem( hTab,3, &item);
		item.lParam  = Vector2;
		item.pszText = " RSP Vectors $v16 - $v31 ";
		TabCtrl_InsertItem( hTab,4, &item);
	}
	
    SetupRSP_HiddenPanel ( hDlg );
	SetupRSP_CP0Panel   ( hDlg );
	SetupRSP_GPRPanel   ( hDlg );
	SetupRSP_Vect1Panel ( hDlg );
	SetupRSP_Vect2Panel ( hDlg );

	hStatic = CreateWindowEx(0,"STATIC","", WS_CHILD|WS_VISIBLE, 5,6,616,290,hDlg,0,hinstDLL,NULL );
#ifdef _M_IX86
	RefreshProc = (FARPROC)SetWindowLong(hStatic, GWL_WNDPROC, (long)RefreshRSP_RegProc);
#else
	DebugBreak();
#endif

	UpdateRSPRegistersScreen ();
	ShowRSP_RegisterPanel ( GeneralPurpose );
	SetWindowText(hDlg," RSP Registers");
	
	//if ( !GetStoredWinPos( "RSP Registers", &X, &Y ) ) {
		X = (GetSystemMetrics( SM_CXSCREEN ) - WindowWidth) / 2;
		Y = (GetSystemMetrics( SM_CYSCREEN ) - WindowHeight) / 2;
	//}
	SetWindowPos(hDlg,NULL,X,Y,WindowWidth,WindowHeight, SWP_NOZORDER | SWP_SHOWWINDOW);
}

void SetupRSP_Vect1Panel (HWND hDlg) {
	int count;

	for (count = 0; count < 8;count ++) {
		hVECT1[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,55,(count*28) + 52,254,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hVECT1[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 8;count ++) {
		hVECT1[count + 8] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,345,(count*28) + 52,254,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hVECT1[count + 8],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupRSP_Vect2Panel (HWND hDlg) {
	int count;

	for (count = 0; count < 8;count ++) {
		hVECT2[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,55,(count*28) + 52,254,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hVECT2[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 8;count ++) {
		hVECT2[count + 8] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,345,(count*28) + 52,254,19, 
			hDlg,0,hinstDLL, NULL );
		SendMessage(hVECT2[count + 8],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void ShowRSP_RegisterPanel ( int Panel) {
	int count;

	switch( Panel ) {
	case GeneralPurpose:
		for (count = 0; count < 32;count ++) { ShowWindow(hGPR[count], TRUE ); }
		break;
	case ControlProcessor0:
		for (count = 0; count < 16;count ++) { ShowWindow(hCP0[count], TRUE ); }
		break;
	case HiddenRegisters:
		for (count = 0; count < 12;count ++) { ShowWindow(hHIDDEN[count], TRUE ); }
		break;
	case Vector1:
		for (count = 0; count < 16;count ++) { ShowWindow(hVECT1[count], TRUE ); }
		break;
	case Vector2:
		for (count = 0; count < 16;count ++) { ShowWindow(hVECT2[count], TRUE ); }
		break;
	}
}

void UpdateRSPRegistersScreen ( void ) {
	char RegisterValue[100];
	int count, nSel;
	TC_ITEM item;

	if (!InRSPRegisterWindow) { return; }
	nSel = TabCtrl_GetCurSel( hTab );
	if ( nSel > -1 ) {
		item.mask = TCIF_PARAM;
		TabCtrl_GetItem( hTab, nSel, &item );
		switch( item.lParam ) {
		case GeneralPurpose:
			for (count = 0; count < 32;count ++) {
				sprintf(RegisterValue," 0x%08X",RSP_GPR[count].UW);
				SetWindowText(hGPR[count],RegisterValue);
			}
			break;
		case ControlProcessor0:
			if (RSPInfo.SP_MEM_ADDR_REG)
			{
				sprintf(RegisterValue," 0x%08X",*RSPInfo.SP_MEM_ADDR_REG);
				SetWindowText(hCP0[0],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.SP_DRAM_ADDR_REG);
				SetWindowText(hCP0[1],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.SP_RD_LEN_REG);
				SetWindowText(hCP0[2],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.SP_WR_LEN_REG);
				SetWindowText(hCP0[3],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.SP_STATUS_REG);
				SetWindowText(hCP0[4],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.SP_DMA_FULL_REG);
				SetWindowText(hCP0[5],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.SP_DMA_BUSY_REG);
				SetWindowText(hCP0[6],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.SP_SEMAPHORE_REG);
				SetWindowText(hCP0[7],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.DPC_START_REG);
				SetWindowText(hCP0[8],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.DPC_END_REG);
				SetWindowText(hCP0[9],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.DPC_CURRENT_REG);
				SetWindowText(hCP0[10],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.DPC_STATUS_REG);
				SetWindowText(hCP0[11],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.DPC_CLOCK_REG);
				SetWindowText(hCP0[12],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.DPC_BUFBUSY_REG);
				SetWindowText(hCP0[13],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.DPC_PIPEBUSY_REG);
				SetWindowText(hCP0[14],RegisterValue);
				sprintf(RegisterValue," 0x%08X",*RSPInfo.DPC_TMEM_REG);
				SetWindowText(hCP0[15],RegisterValue);
			}
			break;
		case HiddenRegisters:
			for (count = 0; count < 8;count ++) { 
				sprintf(RegisterValue," 0x%08X - %08X",RSP_ACCUM[count].W[1], RSP_ACCUM[count].W[0]);
				SetWindowText(hHIDDEN[count],RegisterValue);
			}
			for (count = 0; count < 3;count ++) { 
				sprintf(RegisterValue," 0x%04X",RSP_Flags[count].UHW[0]);
				SetWindowText(hHIDDEN[count + 8],RegisterValue);
			}
			sprintf(RegisterValue," 0x%04X",RSP_Flags[2].UHW[0]);
			SetWindowText(hHIDDEN[11],RegisterValue);
			break;
		case Vector1:
			for (count = 0; count < 16;count ++) { 
				sprintf(RegisterValue," 0x%08X - %08X - %08X - %08X", RSP_Vect[count].W[3],
					RSP_Vect[count].W[2], RSP_Vect[count].W[1], RSP_Vect[count].W[0]);
				SetWindowText(hVECT1[count],RegisterValue);
			}
			break;
		case Vector2:
			for (count = 0; count < 16;count ++) { 
				sprintf(RegisterValue," 0x%08X - %08X - %08X - %08X", RSP_Vect[count + 16].W[3],
					RSP_Vect[count + 16].W[2], RSP_Vect[count + 16].W[1], RSP_Vect[count + 16].W[0]);
				SetWindowText(hVECT2[count],RegisterValue);
			}
			break;
		}
	}
}
