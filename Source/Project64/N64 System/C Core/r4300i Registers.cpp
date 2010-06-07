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

void __cdecl UpdateCurrentR4300iRegisterPanel ( void ) {
}

#ifdef OLD_CODE
#if (!defined(EXTERNAL_RELEASE))
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "main.h"
#include "CPU.h"
#include "r4300i Registers.h"

#define GeneralPurpose			1
#define ControlProcessor0		2
#define FloatingRegisters 		3 
#define SpecialRegister	 		4 
#define RDRAMRegisters	 		5
#define SPRegisters				6
#define MIPSInterface	 		7
#define VideoInterface 		  	8
#define AudioInterface		  	9
#define PeripheralInterface 	10
#define RDRAMInterface	 		11
#define SerialInterface			12

#define IDC_TAB_CONTROL			1000

void Create_R4300i_Register_Window     ( int );
void PaintR4300iAIPanel                ( HWND );
void PaintR4300iCP0Panel               ( HWND );
void PaintR4300iFPRPanel               ( HWND );
void PaintR4300iGPRPanel               ( HWND );
void PaintR4300iMIPanel                ( HWND );
void PaintR4300iRDRamPanel             ( HWND );
void PaintR4300iPIPanel                ( HWND );
void PaintR4300iRIPanel                ( HWND );/*
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

void PaintR4300iSIPanel                ( HWND );
void PaintR4300iSPPanel                ( HWND );
void PaintR4300iSpecialPanel           ( HWND );
void PaintR4300iMIPanel                ( HWND );
void PaintR4300iVIPanel                ( HWND );
void PaintR4300iRIPanel                ( HWND );

void SetupR4300iAIPanel                ( HWND );
void SetupR4300iCP0Panel               ( HWND );
void SetupR4300iFPRPanel               ( HWND );
void SetupR4300iGPRPanel               ( HWND );
void SetupR4300iMIPanel                ( HWND );
void SetupR4300iRDRamPanel             ( HWND );
void SetupR4300iPIPanel                ( HWND );
void SetupR4300iRIPanel                ( HWND );
void SetupR4300iSIPanel                ( HWND );
void SetupR4300iSPPanel                ( HWND );
void SetupR4300iSpecialPanel           ( HWND );
void SetupR4300iVIPanel                ( HWND );
void SetupR4300iRegistersMain          ( HWND );
void ShowR4300iRegisterPanel           ( int );

LRESULT CALLBACK RefreshR4300iRegProc  ( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK R4300i_Registers_Proc ( HWND, UINT, WPARAM, LPARAM );

HWND R4300i_Registers_hDlg, hTab, hStatic, hGPR[32], hCP0[32], hFPR[32], hSpecial[6],
	hRDRam[10], hSP[10], hMI[4], hVI[14], hAI[6], hPI[13], hRI[8], hSI[4];
int InR4300iRegisterWindow = FALSE;
FARPROC r4300iRegRefreshProc;

void Create_R4300i_Register_Window ( int Child ) {
	DWORD ThreadID;
	
	if ( Child ) {
		InR4300iRegisterWindow = TRUE;
		DialogBox( hInst, "BLANK", NULL,(DLGPROC) R4300i_Registers_Proc );
		InR4300iRegisterWindow = FALSE;
	} else {
		if (!InR4300iRegisterWindow) {
			CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Create_R4300i_Register_Window,
				(LPVOID)TRUE,0, &ThreadID);	
		} else {
			SetForegroundWindow(R4300i_Registers_hDlg);
		}	
	}
}

void __cdecl Enter_R4300i_Register_Window ( void ) {
	if (!HaveDebugger) { return; }
    Create_R4300i_Register_Window ( FALSE );
}

void HideR4300iRegisterPanel ( int Panel) {
	int count;

	switch( Panel ) {
	case GeneralPurpose:
		for (count = 0; count < 32;count ++) { ShowWindow(hGPR[count],FALSE); }
		break;
	case ControlProcessor0:
		for (count = 0; count < 32;count ++) { ShowWindow(hCP0[count],FALSE); }
		break;
	case FloatingRegisters:
		for (count = 0; count < 32;count ++) { ShowWindow(hFPR[count],FALSE); }
		break;
	case SpecialRegister:
		for (count = 0; count < 6;count ++) { ShowWindow(hSpecial[count],FALSE); }
		break;
	case RDRAMRegisters:
		for (count = 0; count < 10;count ++) { ShowWindow(hRDRam[count],FALSE); }
		break;
	case SPRegisters:
		for (count = 0; count < 10;count ++) { ShowWindow(hSP[count],FALSE); }
		break;
	case MIPSInterface:
		for (count = 0; count < 4;count ++) { ShowWindow(hMI[count],FALSE); }
		break;
	case VideoInterface:
		for (count = 0; count < 14;count ++) { ShowWindow(hVI[count],FALSE); }
		break;
	case AudioInterface:
		for (count = 0; count < 6;count ++) { ShowWindow(hAI[count],FALSE); }
		break;
	case PeripheralInterface:
		for (count = 0; count < 13;count ++) { ShowWindow(hPI[count],FALSE); }
		break;
	case RDRAMInterface:
		for (count = 0; count < 13;count ++) { ShowWindow(hRI[count],FALSE); }
		break;
	case SerialInterface:
		for (count = 0; count < 4;count ++) { ShowWindow(hSI[count],FALSE); }
		break;
	}
}

void PaintR4300iAIPanel (HWND hWnd) {	
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 184;
	rcBox.top    = 44;
	rcBox.right  = 450;
	rcBox.bottom = 250;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 207,66,"AI_DRAM_ADDR_REG:",17);
	TextOut( ps.hdc, 207,96,"AI_LEN_REG:",11);
	TextOut( ps.hdc, 207,126,"AI_CONTROL_REG:",15);
	TextOut( ps.hdc, 207,156,"AI_STATUS_REG:",14);
	TextOut( ps.hdc, 207,186,"AI_DACRATE_REG:",15);
	TextOut( ps.hdc, 207,216,"AI_BITRATE_REG:",15);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintR4300iCP0Panel (HWND hWnd) {	
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 10;
	rcBox.top    = 34;
	rcBox.right  = 650;
	rcBox.bottom = 270;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
		GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 55,49,"Index:",6);
	TextOut( ps.hdc, 55,73,"Random:",7);
	TextOut( ps.hdc, 55,97,"EntryLo0:",9);
	TextOut( ps.hdc, 55,121,"EntryLo1:",9);
	TextOut( ps.hdc, 55,145,"Context:",8);
	TextOut( ps.hdc, 55,169,"PageMask:",9);
	TextOut( ps.hdc, 55,193,"Wired:",6);
	TextOut( ps.hdc, 55,217,"BadVaddr:",9);
	TextOut( ps.hdc, 55,241,"Count:",6);
	TextOut( ps.hdc, 260,49,"EntryHi:",8);
	TextOut( ps.hdc, 260,73,"Compare:",8);
	TextOut( ps.hdc, 260,97,"Status:",7);
	TextOut( ps.hdc, 260,121,"Cause:",6);
	TextOut( ps.hdc, 260,145,"EPC:",4);
	TextOut( ps.hdc, 260,169,"PRId:",5);
	TextOut( ps.hdc, 260,193,"Config:",7);
	TextOut( ps.hdc, 260,217,"LLAddr:",7);
	TextOut( ps.hdc, 455,49,"WatchLo:",8);
	TextOut( ps.hdc, 455,73,"WatchHi:",8);
	TextOut( ps.hdc, 455,97,"XContext:",9);
	TextOut( ps.hdc, 455,121,"Parity Error:",13);
	TextOut( ps.hdc, 455,145,"Cache Error:",12);
	TextOut( ps.hdc, 455,169,"TagLo:",6);
	TextOut( ps.hdc, 455,193,"TagHi:",6);
	TextOut( ps.hdc, 455,217,"ErrorEPC:",9);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );

	EndPaint( hWnd, &ps );
}

void PaintR4300iFPRPanel (HWND hWnd) {
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 10;
	rcBox.top    = 29;
	rcBox.right  = 650;
	rcBox.bottom = 275;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );	

	TextOut( ps.hdc, 25,46," Reg 0:",7);
	TextOut( ps.hdc, 25,66," Reg 1:",7);
	TextOut( ps.hdc, 25,86," Reg 2:",7);
	TextOut( ps.hdc, 25,106," Reg 3:",7);
	TextOut( ps.hdc, 25,126," Reg 4:",7);
	TextOut( ps.hdc, 25,146," Reg 5:",7);
	TextOut( ps.hdc, 25,166," Reg 6:",7);
	TextOut( ps.hdc, 25,186," Reg 7:",7);
	TextOut( ps.hdc, 25,206," Reg 8:",7);
	TextOut( ps.hdc, 25,226," Reg 9:",7);
	TextOut( ps.hdc, 25,246,"Reg 10:",7);
	TextOut( ps.hdc, 240,46,"Reg 11:",7);
	TextOut( ps.hdc, 240,66,"Reg 12:",7);
	TextOut( ps.hdc, 240,86,"Reg 13:",7);
	TextOut( ps.hdc, 240,106,"Reg 14:",7);
	TextOut( ps.hdc, 240,126,"Reg 15:",7);
	TextOut( ps.hdc, 240,146,"Reg 16:",7);
	TextOut( ps.hdc, 240,166,"Reg 17:",7);
	TextOut( ps.hdc, 240,186,"Reg 18:",7);
	TextOut( ps.hdc, 240,206,"Reg 19:",7);
	TextOut( ps.hdc, 240,226,"Reg 20:",7);
	TextOut( ps.hdc, 240,246,"Reg 21:",7);
	TextOut( ps.hdc, 450,46,"Reg 22:",7);
	TextOut( ps.hdc, 450,66,"Reg 23:",7);
	TextOut( ps.hdc, 450,86,"Reg 24:",7);
	TextOut( ps.hdc, 450,106,"Reg 25:",7);
	TextOut( ps.hdc, 450,126,"Reg 26:",7);
	TextOut( ps.hdc, 450,146,"Reg 27:",7);
	TextOut( ps.hdc, 450,166,"Reg 28:",7);
	TextOut( ps.hdc, 450,186,"Reg 29:",7);
	TextOut( ps.hdc, 450,206,"Reg 30:",7);
	TextOut( ps.hdc, 450,226,"Reg 31:",7);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintR4300iGPRPanel (HWND hWnd) {	
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 10;
	rcBox.top    = 29;
	rcBox.right  = 650;
	rcBox.bottom = 275;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 20,46,"R0 - Reg 0:",11);
	TextOut( ps.hdc, 20,66,"AT - Reg 1:",11);
	TextOut( ps.hdc, 20,86,"V0 - Reg 2:",11);
	TextOut( ps.hdc, 20,106,"V1 - Reg 3:",11);
	TextOut( ps.hdc, 20,126,"A0 - Reg 4:",11);
	TextOut( ps.hdc, 20,146,"A1 - Reg 5:",11);
	TextOut( ps.hdc, 20,166,"A2 - Reg 6:",11);
	TextOut( ps.hdc, 20,186,"A3 - Reg 7:",11);
	TextOut( ps.hdc, 20,206,"T0 - Reg 8:",11);
	TextOut( ps.hdc, 20,226,"T1 - Reg 9:",11);
	TextOut( ps.hdc, 20,246,"T2 - Reg 10:",12);
	TextOut( ps.hdc, 225,46,"T3 - Reg 11:",12);
	TextOut( ps.hdc, 225,66,"T4 - Reg 12:",12);
	TextOut( ps.hdc, 225,86,"T5 - Reg 13:",12);
	TextOut( ps.hdc, 225,106,"T6 - Reg 14:",12);
	TextOut( ps.hdc, 225,126,"T7 - Reg 15:",12);
	TextOut( ps.hdc, 225,146,"S0 - Reg 16:",12);
	TextOut( ps.hdc, 225,166,"S1 - Reg 17:",12);
	TextOut( ps.hdc, 225,186,"S2 - Reg 18:",12);
	TextOut( ps.hdc, 225,206,"S3 - Reg 19:",12);
	TextOut( ps.hdc, 225,226,"S4 - Reg 20:",12);
	TextOut( ps.hdc, 225,246,"S5 - Reg 21:",12);
	TextOut( ps.hdc, 435,46,"S6 - Reg 22:",12);
	TextOut( ps.hdc, 435,66,"S7 - Reg 23:",12);
	TextOut( ps.hdc, 435,86,"T8 - Reg 24:",12);
	TextOut( ps.hdc, 435,106,"T9 - Reg 25:",12);
	TextOut( ps.hdc, 435,126,"K0 - Reg 26:",12);
	TextOut( ps.hdc, 435,146,"K1 - Reg 27:",12);
	TextOut( ps.hdc, 435,166,"GP - Reg 28:",12);
	TextOut( ps.hdc, 435,186,"SP - Reg 29:",12);
	TextOut( ps.hdc, 435,206,"S8 - Reg 30:",12);
	TextOut( ps.hdc, 435,226,"RA - Reg 31:",12);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintR4300iRDRamPanel (HWND hWnd) { 
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;	
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 155;
	rcBox.top    = 34;
	rcBox.right  = 495;
	rcBox.bottom = 270;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 190,56,"RDRAM_CONFIG_REG:",17);
	TextOut( ps.hdc, 190,76,"RDRAM_DEVICE_ID_REG:",20);
	TextOut( ps.hdc, 190,96,"RDRAM_DELAY_REG:",16);
	TextOut( ps.hdc, 190,116,"RDRAM_MODE_REG:",15);
	TextOut( ps.hdc, 190,136,"RDRAM_REF_INTERVAL_REG:",23);
	TextOut( ps.hdc, 190,156,"RDRAM_REF_ROW_REG:",18);
	TextOut( ps.hdc, 190,176,"RDRAM_RAS_INTERVAL_REG:",23);
	TextOut( ps.hdc, 190,196,"RDRAM_MIN_INTERVAL_REG:",23);
	TextOut( ps.hdc, 190,216,"RDRAM_ADDR_SELECT_REG:",22);
	TextOut( ps.hdc, 190,236,"RDRAM_DEVICE_MANUF_REG:",23);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintR4300iRIPanel (HWND hWnd) { 
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;	
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 95;
	rcBox.top    = 64;
	rcBox.right  = 565;
	rcBox.bottom = 215;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 110,86,"RI_MODE_REG:",12);
	TextOut( ps.hdc, 110,116,"RI_CONFIG_REG:",14);
	TextOut( ps.hdc, 110,146,"RI_CURRENT_LOAD_REG:",20);
	TextOut( ps.hdc, 110,176,"RI_SELECT_REG:",14);
	TextOut( ps.hdc, 360,86,"RI_REFRESH_REG:",15);
	TextOut( ps.hdc, 360,116,"RI_LATENCY_REG:",15);
	TextOut( ps.hdc, 360,146,"RI_RERROR_REG:",14);
	TextOut( ps.hdc, 360,176,"RI_WERROR_REG:",14);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintR4300iSIPanel (HWND hWnd) { 
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 175;
	rcBox.top    = 64;
	rcBox.right  = 475;
	rcBox.bottom = 210;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 200,86,"SI_DRAM_ADDR_REG:",17);
	TextOut( ps.hdc, 200,116,"SI_PIF_ADDR_RD64B_REG:",22);
	TextOut( ps.hdc, 200,146,"SI_PIF_ADDR_WR64B_REG:",22);
	TextOut( ps.hdc, 200,176,"SI_STATUS_REG:",14);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintR4300iSPPanel (HWND hWnd) { 
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;	
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 95;
	rcBox.top    = 64;
	rcBox.right  = 565;
	rcBox.bottom = 232;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 110,81,"SP_MEM_ADDR_REG:",16);
	TextOut( ps.hdc, 110,111,"SP_DRAM_ADDR_REG:",17);
	TextOut( ps.hdc, 110,141,"SP_RD_LEN_REG:",14);
	TextOut( ps.hdc, 110,171,"SP_WR_LEN_REG:",14);
	TextOut( ps.hdc, 110,201,"SP_STATUS_REG:",14);
	TextOut( ps.hdc, 340,81,"SP_DMA_FULL_REG:",16);
	TextOut( ps.hdc, 340,111,"SP_DMA_BUSY_REG:",16);
	TextOut( ps.hdc, 340,141,"SP_SEMAPHORE_REG:",17);
	TextOut( ps.hdc, 340,171,"SP_PC_REG:",10);
	TextOut( ps.hdc, 340,201,"SP_IBIST_REG:",13);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintR4300iSpecialPanel (HWND hWnd) { 
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 155;
	rcBox.top    = 34;
	rcBox.right  = 495;
	rcBox.bottom = 270;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 210,66,"Program Counter:",16);
	TextOut( ps.hdc, 210,96,"Multi/Divide HI:",16);
	TextOut( ps.hdc, 210,126,"Multi/Divide LO:",16);
	TextOut( ps.hdc, 210,156,"Load/Link Bit:",14);
	TextOut( ps.hdc, 210,186,"Implementation/Revision:",24);
	TextOut( ps.hdc, 210,216,"Control/Status:",15);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintR4300iMIPanel (HWND hWnd) { 
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;	
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 175;
	rcBox.top    = 64;
	rcBox.right  = 475;
	rcBox.bottom = 210;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,
	GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 210,86,"MI_MODE_REG:",12);
	TextOut( ps.hdc, 210,116,"MI_VERSION_REG:",15);
	TextOut( ps.hdc, 210,146,"MI_INTR_REG:",12);
	TextOut( ps.hdc, 210,176,"MI_INTR_MASK_REG:",17);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintR4300iPIPanel (HWND hWnd) { 
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;	
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 85;
	rcBox.top    = 34;
	rcBox.right  = 575;
	rcBox.bottom = 270;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc, GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 95,55,"PI_DRAM_ADDR_REG:",17);
	TextOut( ps.hdc, 95,85,"PI_CART_ADDR_REG:",17);
	TextOut( ps.hdc, 95,115,"PI_RD_LEN_REG:",14);
	TextOut( ps.hdc, 95,145,"PI_WR_LEN_REG:",14);
	TextOut( ps.hdc, 95,175,"PI_STATUS_REG:",14);
	TextOut( ps.hdc, 95,205,"PI_DOMAIN1_REG:",15);
	TextOut( ps.hdc, 95,235,"PI_BSD_DOM1_PWD_REG:",20);
	TextOut( ps.hdc, 330,55,"PI_BSD_DOM1_PGS_REG:",20);
	TextOut( ps.hdc, 330,85,"PI_BSD_DOM1_RLS_REG:",20);
	TextOut( ps.hdc, 330,115,"PI_DOMAIN2_REG:",15);
	TextOut( ps.hdc, 330,145,"PI_BSD_DOM2_PWD_REG:",20);
	TextOut( ps.hdc, 330,175,"PI_BSD_DOM2_PGS_REG:",20);
	TextOut( ps.hdc, 330,205,"PI_BSD_DOM2_RLS_REG:",20);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

void PaintR4300iVIPanel (HWND hWnd) { 
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;	
	int OldBkMode;
	BeginPaint( hWnd, &ps );
	
	rcBox.left   = 85;
	rcBox.top    = 34;
	rcBox.right  = 575;
	rcBox.bottom = 270;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	hOldFont = SelectObject( ps.hdc,GetStockObject(DEFAULT_GUI_FONT) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );

	TextOut( ps.hdc, 95,55,"VI_STATUS_REG:",14);
	TextOut( ps.hdc, 95,85,"VI_DRAM_ADDR_REG:",17);
	TextOut( ps.hdc, 95,115,"VI_WIDTH_REG:",13);
	TextOut( ps.hdc, 95,145,"VI_INTR_REG:",12);
	TextOut( ps.hdc, 95,175,"VI_V_CURRENT_LINE_REG:",22);
	TextOut( ps.hdc, 95,205,"VI_TIMING_REG:",14);
	TextOut( ps.hdc, 95,235,"VI_V_SYNC_REG:",14);
	TextOut( ps.hdc, 345,55,"VI_H_SYNC_REG:",14);
	TextOut( ps.hdc, 345,85,"VI_H_SYNC_LEAP_REG:",19);
	TextOut( ps.hdc, 345,115,"VI_H_START_REG:",15);
	TextOut( ps.hdc, 345,145,"VI_V_START_REG:",15);
	TextOut( ps.hdc, 345,175,"VI_V_BURST_REG:",15);
	TextOut( ps.hdc, 345,205,"VI_X_SCALE_REG:",15);
	TextOut( ps.hdc, 345,235,"VI_Y_SCALE_REG:",15);
		
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
	EndPaint( hWnd, &ps );
}

LRESULT CALLBACK R4300i_Registers_Proc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {	
	static RECT rcDisp;
	static int CurrentPanel = GeneralPurpose;
	TC_ITEM item;

	switch (uMsg) {
	case WM_INITDIALOG:
		R4300i_Registers_hDlg = hDlg;
		SetupR4300iRegistersMain( hDlg );
		break;
	case WM_MOVE:
		StoreCurrentWinPos("R4300i Registers",hDlg);
		break;
	case WM_SIZE:
		GetClientRect( hDlg, &rcDisp);
		TabCtrl_AdjustRect( hTab, FALSE, &rcDisp );
		break;
	case WM_NOTIFY:
		switch (((NMHDR *)lParam)->code) {
		case TCN_SELCHANGE:
			InvalidateRect( hTab, &rcDisp, TRUE );
			HideR4300iRegisterPanel (CurrentPanel);			
			item.mask = TCIF_PARAM;
			TabCtrl_GetItem( hTab, TabCtrl_GetCurSel( hTab ), &item );
			CurrentPanel = item.lParam;
			InvalidateRect( hStatic, NULL, FALSE );
			UpdateCurrentR4300iRegisterPanel();
			ShowR4300iRegisterPanel ( CurrentPanel );			
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
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK RefreshR4300iRegProc ( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
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
				PaintR4300iGPRPanel (hWnd);
				break;
			case ControlProcessor0:
				PaintR4300iCP0Panel (hWnd);
				break;
			case FloatingRegisters:
				PaintR4300iFPRPanel (hWnd);
				break;
			case SpecialRegister:
				PaintR4300iSpecialPanel (hWnd);
				break;
			case RDRAMRegisters:
				PaintR4300iRDRamPanel (hWnd);
				break;
			case SPRegisters:
				PaintR4300iSPPanel (hWnd);
				break;
			case MIPSInterface:
				PaintR4300iMIPanel (hWnd);
				break;
			case VideoInterface:
				PaintR4300iVIPanel (hWnd);
				break;
			case AudioInterface:
				PaintR4300iAIPanel (hWnd);
				break;
			case PeripheralInterface:
				PaintR4300iPIPanel (hWnd);
				break;
			case RDRAMInterface:
				PaintR4300iRIPanel (hWnd);
				break;
			case SerialInterface:
				PaintR4300iSIPanel (hWnd);
				break;
			}

		}
		break;
	default:
		return( (*r4300iRegRefreshProc)(hWnd, uMsg, wParam, lParam) );
	}
	return( FALSE );
}

void SetupR4300iAIPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 6;count ++) {
		hAI[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,342,(count*30) + 69,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hAI[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupR4300iCP0Panel (HWND hDlg) {
	int count, top;
	for (count = 0;count < 32;count ++) { hCP0[count] = NULL; }
	top = 53;
	for (count = 0; count < 10;count ++) {
		if (count == 7) { continue; }
		hCP0[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,130,top,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hCP0[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		top += 24;
	}

	top = 53;
	for (count = 0; count < 8;count ++) {
		hCP0[count + 10] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,330,top,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hCP0[count + 10],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		top += 24;
	}

	top = 53;
	for (count = 0; count < 13;count ++) {
		if (count >= 3 && count <= 7 ) { continue; }
		hCP0[count + 18] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,535,top,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hCP0[count + 18],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		top += 24;
	}

}

void SetupR4300iFPRPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 11;count ++) {
		hFPR[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,90,(count*20) + 50,135,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hFPR[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);

	}
	for (count = 0; count < 11;count ++) {
		hFPR[count + 11] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,300,(count*20) + 50,135,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hFPR[ count + 11 ],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 10;count ++) {
		hFPR[count + 22] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD |  
			ES_READONLY | WS_BORDER | WS_TABSTOP,510,(count*20) + 50,135,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hFPR[ count + 22 ],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupR4300iGPRPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 11;count ++) {
		hGPR[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,90,(count*20) + 50,135,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hGPR[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);

	}
	for (count = 0; count < 11;count ++) {
		hGPR[count + 11] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,300,(count*20) + 50,135,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hGPR[ count + 11 ],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 10;count ++) {
		hGPR[count + 22] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD |  
			ES_READONLY | WS_BORDER | WS_TABSTOP,510,(count*20) + 50,135,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hGPR[ count + 22 ],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupR4300iMIPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 4;count ++) {
		hMI[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,340,(count*30) + 89,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hMI[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupR4300iPIPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 7;count ++) {
		hPI[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,245,(count*30) + 58,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hPI[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 6;count ++) {
		hPI[count + 7] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,480,(count*30) + 58,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hPI[count + 7],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupR4300iRDRamPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 10;count ++) {
		hRDRam[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,375,(count*20) + 58,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hRDRam[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupR4300iRIPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 4;count ++) {
		hRI[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,260,(count*30) + 90,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hRI[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	for (count = 0; count < 4;count ++) {
		hRI[count + 4] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,475,(count*30) + 90,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hRI[count + 4],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupR4300iSIPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 4;count ++) {
		hSI[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,360,(count*30) + 89,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hSI[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

}

void SetupR4300iSPPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 5;count ++) {
		hSP[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,250,(count*30) + 84,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hSP[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 5;count ++) {
		hSP[count + 5] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,475,(count*30) + 84,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hSP[ count + 5 ],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupR4300iSpecialPanel (HWND hDlg) {
	int count;
	hSpecial[0] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
		ES_READONLY | WS_BORDER | WS_TABSTOP,345,70,80,19, 
		hDlg,0,hInst, NULL );
	hSpecial[1] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
		ES_READONLY | WS_BORDER | WS_TABSTOP,345,100,135,19, 
		hDlg,0,hInst, NULL );
	hSpecial[2] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
		ES_READONLY | WS_BORDER | WS_TABSTOP,345,130,135,19, 
		hDlg,0,hInst, NULL );
	hSpecial[3] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
		ES_READONLY | WS_BORDER | WS_TABSTOP,345,160,34,19, 
		hDlg,0,hInst, NULL );
	hSpecial[4] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
		ES_READONLY | WS_BORDER | WS_TABSTOP,345,190,80,19, 
		hDlg,0,hInst, NULL );
	hSpecial[5] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
		ES_READONLY | WS_BORDER | WS_TABSTOP,345,220,80,19, 
		hDlg,0,hInst, NULL );
	for (count = 0; count < 6;count ++) {
		SendMessage(hSpecial[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupR4300iVIPanel (HWND hDlg) {
	int count;

	for (count = 0; count < 7;count ++) {
		hVI[count] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,250,(count*30) + 58,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hVI[count],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	for (count = 0; count < 7;count ++) {
		hVI[count + 7] = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","", WS_CHILD | 
			ES_READONLY | WS_BORDER | WS_TABSTOP,480,(count*30) + 58,80,19, 
			hDlg,0,hInst, NULL );
		SendMessage(hVI[count + 7],WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
}

void SetupR4300iRegistersMain (HWND hDlg) {
#define WindowWidth  675
#define WindowHeight 325
	DWORD X, Y;

	hTab = CreateWindowEx(0,WC_TABCONTROL,"", WS_TABSTOP | WS_CHILD | WS_VISIBLE,5,6,660,290,
		hDlg,(HMENU)IDC_TAB_CONTROL,hInst,NULL );
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
		item.lParam  = FloatingRegisters;
		item.pszText = " floating-point Registers ";
		TabCtrl_InsertItem( hTab,2, &item);	
		item.lParam  = SpecialRegister;
		item.pszText = " Special Registers ";
		TabCtrl_InsertItem( hTab,3, &item);	
		item.lParam  = RDRAMRegisters;
		item.pszText = " RDRAM Registers ";
		TabCtrl_InsertItem( hTab,4, &item);	
		item.lParam  = SPRegisters;
		item.pszText = " SP Registers ";
		TabCtrl_InsertItem( hTab,5, &item);	
		item.lParam  = MIPSInterface;
		item.pszText = " MIPS Interface ";
		TabCtrl_InsertItem( hTab,6, &item);
		item.lParam  = VideoInterface  ;
		item.pszText = " Video Interface   ";
		TabCtrl_InsertItem( hTab,7, &item);	
		item.lParam  = AudioInterface ;
		item.pszText = " Audio Interface  ";
		TabCtrl_InsertItem( hTab,8, &item);	
		item.lParam  = PeripheralInterface;
		item.pszText = " Peripheral Interface ";
		TabCtrl_InsertItem( hTab,9, &item);	
		item.lParam  = RDRAMInterface;
		item.pszText = " RDRAM Interface ";
		TabCtrl_InsertItem( hTab,10, &item);	
		item.lParam  = SerialInterface;
		item.pszText = " Serial Interface ";
		TabCtrl_InsertItem( hTab,11, &item);	
	}
	
	SetupR4300iAIPanel ( hDlg );
	SetupR4300iCP0Panel ( hDlg );
	SetupR4300iFPRPanel ( hDlg );
	SetupR4300iGPRPanel ( hDlg );
	SetupR4300iMIPanel ( hDlg );
	SetupR4300iRDRamPanel ( hDlg );
	SetupR4300iPIPanel ( hDlg );
	SetupR4300iRIPanel ( hDlg );
	SetupR4300iSIPanel ( hDlg );
	SetupR4300iSPPanel ( hDlg );
	SetupR4300iSpecialPanel ( hDlg );
	SetupR4300iVIPanel ( hDlg);

	hStatic = CreateWindowEx(0,"STATIC","", WS_CHILD|WS_VISIBLE, 5,6,660,290,hDlg,0,hInst,NULL );
	r4300iRegRefreshProc = (FARPROC)SetWindowLong( hStatic,GWL_WNDPROC,(long)RefreshR4300iRegProc);

	ShowR4300iRegisterPanel ( GeneralPurpose );
	UpdateCurrentR4300iRegisterPanel ();
	SetWindowText(hDlg," R4300i Registers");
	
	if ( !GetStoredWinPos( "R4300i Registers", &X, &Y ) ) {
		X = (GetSystemMetrics( SM_CXSCREEN ) - WindowWidth) / 2;
		Y = (GetSystemMetrics( SM_CYSCREEN ) - WindowHeight) / 2;
	}
	SetWindowPos(hDlg,NULL,X,Y,WindowWidth,WindowHeight, SWP_NOZORDER | SWP_SHOWWINDOW);
}

void ShowR4300iRegisterPanel ( int Panel) {
	int count;

	switch( Panel ) {
	case GeneralPurpose:
		for (count = 0; count < 32;count ++) { ShowWindow(hGPR[count],TRUE); }
		break;
	case ControlProcessor0:
		for (count = 0; count < 32;count ++) { ShowWindow(hCP0[count],TRUE); }
		break;
	case FloatingRegisters:
		for (count = 0; count < 32;count ++) { ShowWindow(hFPR[count],TRUE); }
		break;
	case SpecialRegister:
		for (count = 0; count < 6;count ++) { ShowWindow(hSpecial[count],TRUE); }
		break;
	case RDRAMRegisters:
		for (count = 0; count < 10;count ++) { ShowWindow(hRDRam[count],TRUE); }
		break;
	case SPRegisters:
		for (count = 0; count < 10;count ++) { ShowWindow(hSP[count],TRUE); }
		break;
	case MIPSInterface:
		for (count = 0; count < 4;count ++) { ShowWindow(hMI[count],TRUE); }
		break;
	case VideoInterface:
		for (count = 0; count < 14;count ++) { ShowWindow(hVI[count],TRUE); }
		break;
	case AudioInterface:
		for (count = 0; count < 6;count ++) { ShowWindow(hAI[count],TRUE); }
		break;
	case PeripheralInterface:
		for (count = 0; count < 13;count ++) { ShowWindow(hPI[count],TRUE); }
		break;
	case RDRAMInterface:
		for (count = 0; count < 8;count ++) { ShowWindow(hRI[count],TRUE); }
		break;
	case SerialInterface:
		for (count = 0; count < 4;count ++) { ShowWindow(hSI[count],TRUE); }
		break;
	}
}

void __cdecl UpdateCurrentR4300iRegisterPanel ( void ) {
	char RegisterValue[60], OldWinText[60];
	int count, nSel;
	TC_ITEM item;

	if (!InR4300iRegisterWindow) { return; }
	nSel = TabCtrl_GetCurSel( hTab );
	if ( nSel > -1 ) {
		item.mask = TCIF_PARAM;
		TabCtrl_GetItem( hTab, nSel, &item );
		switch( item.lParam ) {
		case GeneralPurpose:
			for (count = 0; count < 32;count ++) {
				GetWindowText(hGPR[count],OldWinText,60);
				sprintf(RegisterValue," 0x%08X - %08X",GPR[count].W[1],GPR[count].W[0]);
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hGPR[count],RegisterValue);
				}
			}
			break;
		case ControlProcessor0:
			for (count = 0; count < 32;count ++) {
				GetWindowText(hCP0[count],OldWinText,60);
				sprintf( RegisterValue," 0x%08X",CP0[count] );
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hCP0[count],RegisterValue);
				}
			}
			break;
		case FloatingRegisters:
			for (count = 0; count < 32;count ++) {
				GetWindowText(hFPR[count],OldWinText,60);
				sprintf(RegisterValue," 0x%08X - %08X",FPR[count].W[1],FPR[count].W[0]);
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hFPR[count],RegisterValue);
				}
			}
			break;
		case SpecialRegister:
			GetWindowText(hSpecial[0],OldWinText,60);
			sprintf( RegisterValue," 0x%08X",PROGRAM_COUNTER );
			if ( strcmp( RegisterValue, OldWinText) != 0 ) {
				SetWindowText(hSpecial[0],RegisterValue);
			}			
			GetWindowText(hSpecial[0],OldWinText,60);
			sprintf(RegisterValue," 0x%08X - %08X",HI.W[1],HI.W[0]);
			if ( strcmp( RegisterValue, OldWinText) != 0 ) {
				SetWindowText(hSpecial[1],RegisterValue);
			}			
			GetWindowText(hSpecial[0],OldWinText,60);
			sprintf(RegisterValue," 0x%08X - %08X",LO.W[1],LO.W[0]);
			if ( strcmp( RegisterValue, OldWinText) != 0 ) {
				SetWindowText(hSpecial[2],RegisterValue);
			}			
			GetWindowText(hSpecial[0],OldWinText,60);
			sprintf(RegisterValue," 0x%08X",REVISION_REGISTER);
			if ( strcmp( RegisterValue, OldWinText) != 0 ) {
				SetWindowText(hSpecial[4],RegisterValue);
			}			
			GetWindowText(hSpecial[0],OldWinText,60);
			sprintf(RegisterValue," 0x%08X",FSTATUS_REGISTER);
			if ( strcmp( RegisterValue, OldWinText) != 0 ) {
				SetWindowText(hSpecial[5],RegisterValue);
			}			
			break;
		case RDRAMRegisters:
			for (count = 0; count < 10;count ++) {
				GetWindowText(hRDRam[count],OldWinText,60);
				sprintf(RegisterValue,"  0x%08X",RegRDRAM[count]);
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hRDRam[count],RegisterValue);
				}
			}
			break;
		case SPRegisters:
			for (count = 0; count < 10;count ++) {
				GetWindowText(hSP[count],OldWinText,60);
				sprintf(RegisterValue," 0x%08X",RegSP[count]);
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hSP[count],RegisterValue);
				}
			}
			break;
		case MIPSInterface:
			for (count = 0; count < 4;count ++) {
				GetWindowText(hMI[count],OldWinText,60);
				sprintf(RegisterValue," 0x%08X",RegMI[count]);
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hMI[count],RegisterValue);
				}
			}
			break;
		case VideoInterface:
			for (count = 0; count < 14;count ++) {
				GetWindowText(hVI[count],OldWinText,60);
				sprintf(RegisterValue," 0x%08X",RegVI[count]);
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hVI[count],RegisterValue);
				}
			}
			break;
		case AudioInterface:
			for (count = 0; count < 6;count ++) {
				GetWindowText(hAI[count],OldWinText,60);
				sprintf(RegisterValue," 0x%08X",RegAI[count]);
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hAI[count],RegisterValue);
				}
			}
			break;
		case PeripheralInterface:
			for (count = 0; count < 13;count ++) {
				GetWindowText(hPI[count],OldWinText,60);
				sprintf(RegisterValue," 0x%08X",RegPI[count]);
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hPI[count],RegisterValue);
				}
			}
			break;
		case RDRAMInterface:
			for (count = 0; count < 8;count ++) {
				GetWindowText(hRI[count],OldWinText,60);
				sprintf(RegisterValue," 0x%08X",RegRI[count]);
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hRI[count],RegisterValue);
				}
			}
			break;
		case SerialInterface:
			for (count = 0; count < 4;count ++) {
				GetWindowText(hSI[count],OldWinText,60);
				sprintf(RegisterValue," 0x%08X",RegSI[count]);
				if ( strcmp( RegisterValue, OldWinText) != 0 ) {
					SetWindowText(hSI[count],RegisterValue);
				}
			}
			break;
		}
	}
}
#endif

#endif