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
#include <stdio.h>
#include <stdlib.h>

#include "opcode.h"
#include "Rsp.h"
#include "CPU.h"
#include "RSP Registers.h"
#include "RSP Command.h"
#include "memory.h"
#include "breakpoint.h"
#include "Types.h"

#define RSP_MaxCommandLines		30

#define RSP_Status_PC            1
#define RSP_Status_BP            2

#define IDC_LIST					1000
#define IDC_ADDRESS					1001
#define IDC_FUNCTION_COMBO			1002
#define IDC_GO_BUTTON				1003
#define IDC_BREAK_BUTTON			1004
#define IDC_STEP_BUTTON				1005
#define IDC_SKIP_BUTTON				1006
#define IDC_BP_BUTTON				1007
#define IDC_R4300I_REGISTERS_BUTTON	1008
#define IDC_R4300I_DEBUGGER_BUTTON	1009
#define IDC_RSP_REGISTERS_BUTTON	1010
#define IDC_MEMORY_BUTTON			1011
#define IDC_SCRL_BAR				1012

void Paint_RSP_Commands (HWND hDlg);
void RSP_Commands_Setup ( HWND hDlg );
LRESULT CALLBACK RSP_Commands_Proc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct {
	DWORD Location;
	DWORD opcode;
	char  String[150];
    DWORD status;
} RSPCOMMANDLINE;

RSPCOMMANDLINE RSPCommandLine[30];
HWND RSPCommandshWnd, hList, hAddress, hFunctionlist, hGoButton, hBreakButton,
	hStepButton, hSkipButton, hBPButton, hR4300iRegisters, hR4300iDebugger, hRSPRegisters,
	hMemory, hScrlBar;
Boolean InRSPCommandsWindow;
char CommandName[100];
DWORD Stepping_Commands, WaitingForStep;

void Create_RSP_Commands_Window ( int Child )
{
	DWORD ThreadID;

	if ( Child )
	{
		InRSPCommandsWindow = TRUE;
		DialogBox( hinstDLL, "RSPCOMMAND", NULL,(DLGPROC)RSP_Commands_Proc );

		InRSPCommandsWindow = FALSE;
		memset(RSPCommandLine,0,sizeof(RSPCommandLine));
		SetRSPCommandToRunning();
	}
	else
	{
		if (!InRSPCommandsWindow)
		{
			Stepping_Commands = TRUE;
			CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Create_RSP_Commands_Window,
				(LPVOID)TRUE,0, &ThreadID);
		}
		else
		{
			SetForegroundWindow(RSPCommandshWnd);
		}	
	}
}

void Disable_RSP_Commands_Window ( void )
{
	SCROLLINFO si;

	if (!InRSPCommandsWindow) { return; }
	EnableWindow(hList,            FALSE);
	EnableWindow(hAddress,         FALSE);
	EnableWindow(hScrlBar,         FALSE);
	EnableWindow(hGoButton,        FALSE);
	EnableWindow(hStepButton,      FALSE);
	EnableWindow(hSkipButton,      FALSE);
	EnableWindow(hR4300iRegisters, FALSE);
	EnableWindow(hRSPRegisters,    FALSE);
	EnableWindow(hR4300iDebugger,     FALSE);
	EnableWindow(hMemory,          FALSE);

	si.cbSize = sizeof(si);
	si.fMask  = SIF_RANGE | SIF_POS | SIF_PAGE;
	si.nMin   = 0;
	si.nMax   = 0;
	si.nPos   = 1;
	si.nPage  = 1;
	SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
}

int DisplayRSPCommand (DWORD location, int InsertPos)
{
	uint32_t OpCode;
	DWORD LinesUsed = 1, status;
    Boolean Redraw = FALSE;

	RSP_LW_IMEM(location, &OpCode);

	status = 0;
	if (location == *PrgCount) {status = RSP_Status_PC; }
	if (CheckForRSPBPoint(location)) { status |= RSP_Status_BP; }
	if (RSPCommandLine[InsertPos].opcode != OpCode) { Redraw = TRUE; }
	if (RSPCommandLine[InsertPos].Location != location) { Redraw = TRUE; }
	if (RSPCommandLine[InsertPos].status != status) { Redraw = TRUE; }
	if (Redraw)
	{
		RSPCommandLine[InsertPos].Location = location;
		RSPCommandLine[InsertPos].status = status;
		RSPCommandLine[InsertPos].opcode = OpCode;
		sprintf(RSPCommandLine[InsertPos].String," 0x%04X\t%s",0x1000 | location, 
			RSPOpcodeName ( OpCode, 0x1000 | location ));
		if ( SendMessage(hList,LB_GETCOUNT,0,0) <= InsertPos)
		{
			SendMessage(hList,LB_INSERTSTRING,(WPARAM)InsertPos, (LPARAM)location);
		}
		else
		{
			RECT ItemRC;
			SendMessage(hList,LB_GETITEMRECT,(WPARAM)InsertPos, (LPARAM)&ItemRC);
			RedrawWindow(hList,&ItemRC,NULL, RDW_INVALIDATE );
		}
	}
	return LinesUsed;
}

void DumpRSPCode (void)
{
	char string[100], LogFileName[255], *p ;
	uint32_t OpCode;
	DWORD location, dwWritten;
	HANDLE hLogFile = NULL;

	strcpy(LogFileName,GetCommandLine() + 1);
	
	if (strchr(LogFileName,'\"'))
	{
		p = strchr(LogFileName,'\"');
		*p = '\0';
	}
	
	if (strchr(LogFileName,'\\'))
	{
		p = LogFileName;
		while (strchr(p,'\\'))
		{
			p = strchr(p,'\\');
			p++;
		}
		p -= 1;
		*p = '\0';
	}

	strcat(LogFileName,"\\RSP code.txt");

	hLogFile = CreateFile(LogFileName,GENERIC_WRITE, FILE_SHARE_READ,NULL,CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetFilePointer(hLogFile,0,NULL,FILE_BEGIN);

	for (location = 0; location < 0x1000; location += 4) {
		unsigned int characters_to_write;
		int characters_converted;

		RSP_LW_IMEM(location, &OpCode);
		characters_converted = sprintf(
			&string[0],
			" 0x%03X\t%s\r\n",
			location,
			RSPOpcodeName(OpCode, location)
		);

		if (characters_converted < 0) {
			DisplayError("Failed to sprintf IMEM from 0x%03X.", location);
			break;
		}
		characters_to_write = (unsigned)characters_converted;
		WriteFile(hLogFile, string, characters_to_write, &dwWritten, NULL);
	}
	CloseHandle(hLogFile);
}

void DumpRSPData (void)
{
	char string[100], LogFileName[255], *p ;
	uint32_t value;
	DWORD location, dwWritten;
	HANDLE hLogFile = NULL;

	strcpy(LogFileName,GetCommandLine() + 1);

	if (strchr(LogFileName,'\"'))
	{
		p = strchr(LogFileName,'\"');
		*p = '\0';
	}

	if (strchr(LogFileName,'\\'))
	{
		p = LogFileName;
		while (strchr(p,'\\')) {
			p = strchr(p,'\\');
			p++;
		}
		p -= 1;
		*p = '\0';
	}

	strcat(LogFileName,"\\RSP data.txt");

	hLogFile = CreateFile(LogFileName,GENERIC_WRITE, FILE_SHARE_READ,NULL,CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetFilePointer(hLogFile,0,NULL,FILE_BEGIN);

	for (location = 0; location < 0x1000; location += 4)
	{
		unsigned int characters_to_write;
		int characters_converted;

		RSP_LW_DMEM(location, &value);
		characters_converted = sprintf(
			&string[0],
			" 0x%03X\t0x%08X\r\n",
			location,
			value
		);

		if (characters_converted < 0) {
			DisplayError("Failed to sprintf DMEM from 0x%03X.", location);
			break;
		}
		characters_to_write = (unsigned)characters_converted;
		WriteFile(hLogFile, string, characters_to_write, &dwWritten, NULL);
	}
	CloseHandle(hLogFile);
}

void DrawRSPCommand ( LPARAM lParam )
{
	char Command[150], Offset[30], Instruction[30], Arguments[40];
	int printed_offset, printed_instruction, printed_arguments;
	LPDRAWITEMSTRUCT ditem;
	COLORREF oldColor = {0};
	int ResetColor;
	HBRUSH hBrush;
	RECT TextRect;
	char *p1, *p2;

	ditem  = (LPDRAWITEMSTRUCT)lParam;
	strcpy(Command, RSPCommandLine[ditem->itemID].String);

	if (strchr(Command,'\t'))
	{
		p1 = strchr(Command,'\t');
		printed_offset = sprintf(Offset, "%.*s", p1 - Command, Command);
		p1++;
		if (strchr(p1,'\t'))
		{
			p2 = strchr(p1,'\t');
			printed_instruction = sprintf(Instruction, "%.*s", p2 - p1, p1);
			printed_arguments   = sprintf(Arguments, "%s", p2 + 1);
		}
		else
		{
			printed_instruction = sprintf(Instruction, "%s", p1);
			printed_arguments   = sprintf(Arguments, "\0");
		}
		Command[0] = '\0';
	}
	else
	{
		printed_offset      = sprintf(Offset, "\0");
		printed_instruction = sprintf(Instruction, "\0");
		printed_arguments   = sprintf(Arguments, "\0");
	}

	if (printed_offset < 0 || printed_instruction < 0 || printed_arguments < 0)
	{
		DisplayError("Failed to sprintf from item %u.", ditem -> itemID);
	}

	if (*PrgCount == RSPCommandLine[ditem->itemID].Location)
	{
		ResetColor = TRUE;
		hBrush     = (HBRUSH)(COLOR_HIGHLIGHT + 1);
		oldColor   = SetTextColor(ditem->hDC,RGB(255,255,255));
	}
	else
	{
		ResetColor = FALSE;
		hBrush     = (HBRUSH)GetStockObject(WHITE_BRUSH);
	}

	if (CheckForRSPBPoint( RSPCommandLine[ditem->itemID].Location ))
	{
		ResetColor = TRUE;
		if (*PrgCount == RSPCommandLine[ditem->itemID].Location)
		{
			SetTextColor(ditem->hDC,RGB(255,0,0));
		}
		else
		{
			oldColor = SetTextColor(ditem->hDC,RGB(255,0,0));
		}
	}

	FillRect( ditem->hDC, &ditem->rcItem,hBrush);
	SetBkMode( ditem->hDC, TRANSPARENT );

	if (Command[0] == '\0')
	{
		SetRect(&TextRect,ditem->rcItem.left,ditem->rcItem.top, ditem->rcItem.left + 83,
			ditem->rcItem.bottom);
		DrawText(
			ditem->hDC,
			&Offset[0], printed_offset,
			&TextRect,
			DT_SINGLELINE | DT_VCENTER
		);

		SetRect(&TextRect,ditem->rcItem.left + 83,ditem->rcItem.top, ditem->rcItem.left + 165,
			ditem->rcItem.bottom);
		DrawText(
			ditem->hDC,
			&Instruction[0], printed_instruction,
			&TextRect,
			DT_SINGLELINE | DT_VCENTER
		);

		SetRect(&TextRect,ditem->rcItem.left + 165,ditem->rcItem.top, ditem->rcItem.right,
			ditem->rcItem.bottom);
		DrawText(
			ditem->hDC,
			&Arguments[0], printed_arguments,
			&TextRect,
			DT_SINGLELINE | DT_VCENTER
		);
	}
	else
	{
		DrawText(
			ditem->hDC,
			&Command[0], (signed int)strlen(Command),
			&ditem->rcItem,
			DT_SINGLELINE | DT_VCENTER
		);
	}

	if (ResetColor == TRUE) {
		SetTextColor( ditem->hDC, oldColor );
	}
}

void Enable_RSP_Commands_Window ( void )
{
	SCROLLINFO si;

	if (!InRSPCommandsWindow) { return; }
	EnableWindow(hList,            TRUE);
	EnableWindow(hAddress,         TRUE);
	EnableWindow(hScrlBar,         TRUE);
	EnableWindow(hGoButton,        TRUE);
	EnableWindow(hStepButton,      TRUE);
	EnableWindow(hSkipButton,      FALSE);
	EnableWindow(hR4300iRegisters, TRUE);
	EnableWindow(hRSPRegisters,    TRUE);
	EnableWindow(hR4300iDebugger,  TRUE);
	EnableWindow(hMemory,          TRUE);
	SendMessage(hBPButton, BM_SETSTYLE, BS_PUSHBUTTON,TRUE);
	SendMessage(hStepButton, BM_SETSTYLE, BS_DEFPUSHBUTTON,TRUE);
	SendMessage(RSPCommandshWnd, DM_SETDEFID,IDC_STEP_BUTTON,0);

	if (Stepping_Commands) {
		si.cbSize = sizeof(si);
		si.fMask  = SIF_RANGE | SIF_POS | SIF_PAGE;
		si.nMin   = 0;
		si.nMax   = (0x1000 >> 2) -1;
		si.nPos   = (*PrgCount >> 2);
		si.nPage  = 30;
		SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);

		SetRSPCommandViewto( *PrgCount );
		SetForegroundWindow(RSPCommandshWnd);
	}
}

void Enter_RSP_Commands_Window ( void )
{
    Create_RSP_Commands_Window ( FALSE );
}

void Paint_RSP_Commands (HWND hDlg)
{
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;

	BeginPaint( hDlg, &ps );

	rcBox.left   = 5;   rcBox.top    = 5;
	rcBox.right  = 343; rcBox.bottom = 463;
	DrawEdge( ps.hdc, &rcBox, EDGE_RAISED, BF_RECT );
		
	rcBox.left   = 8;   rcBox.top    = 8;
	rcBox.right  = 340; rcBox.bottom = 460;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );

	rcBox.left   = 347; rcBox.top    = 7;
	rcBox.right  = 446; rcBox.bottom = 42;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );
		
	rcBox.left   = 352; rcBox.top    = 2;
	rcBox.right  = 400; rcBox.bottom = 15;
	FillRect( ps.hdc, &rcBox,(HBRUSH)COLOR_WINDOW);

	rcBox.left   = 14; rcBox.top    = 14;
	rcBox.right  = 88; rcBox.bottom = 32;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED , BF_RECT );

	rcBox.left   = 86; rcBox.top    = 14;
	rcBox.right  = 173; rcBox.bottom = 32;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED , BF_RECT );

	rcBox.left   = 171; rcBox.top    = 14;
	rcBox.right  = 320; rcBox.bottom = 32;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED , BF_RECT );

	hOldFont = (HFONT)SelectObject( ps.hdc,GetStockObject(DEFAULT_GUI_FONT ) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );
		
	TextOut( ps.hdc, 23,16,"Offset",6);
	TextOut( ps.hdc, 97,16,"Instruction",11);
	TextOut( ps.hdc, 180,16,"Arguments",9);
	TextOut( ps.hdc, 354,2," Address ",9);
	TextOut( ps.hdc, 358,19,"0x1",3);
	
	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
		
	EndPaint( hDlg, &ps );
}

void RefreshRSPCommands ( void )
{
	DWORD location, LinesUsed;
	char AsciiAddress[20];
	int count;

	if (InRSPCommandsWindow == FALSE) { return; }

	GetWindowText(hAddress,AsciiAddress,sizeof(AsciiAddress));
	location = AsciiToHex(AsciiAddress) & ~3;

	if (location > 0xF88) { location = 0xF88; }
	for (count = 0 ; count < RSP_MaxCommandLines; count += LinesUsed )
	{
		LinesUsed = DisplayRSPCommand ( location, count );
		location += 4;
	}
}

LRESULT CALLBACK RSP_Commands_Proc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		RSPCommandshWnd = hDlg;
		RSP_Commands_Setup( hDlg );
		break;
	case WM_MOVE:
		//StoreCurrentWinPos("RSP Commands",hDlg);
		break;
	case WM_DRAWITEM:
		if (wParam == IDC_LIST)
		{
			DrawRSPCommand (lParam);
		}
		break;
	case WM_PAINT:
		Paint_RSP_Commands( hDlg );
		RedrawWindow(hScrlBar,NULL,NULL, RDW_INVALIDATE |RDW_ERASE);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LIST:
			if (HIWORD(wParam) == LBN_DBLCLK )
			{
				LRESULT Selected;
				DWORD Location;

				Selected = SendMessage(hList, LB_GETCURSEL, 0, 0);
				Location = RSPCommandLine[Selected].Location;
				if (Location != (DWORD)-1)
				{
					if (CheckForRSPBPoint(Location))
					{
						RemoveRSPBreakPoint(Location);
					}
					else
					{
						AddRSP_BPoint(Location, FALSE);
					}
					RefreshRSPCommands();
				}
			}
			break;
		case IDC_ADDRESS:
			if (HIWORD(wParam) == EN_CHANGE )
			{
				RefreshRSPCommands();
			}
			break;
		case IDC_GO_BUTTON:
			SetRSPCommandToRunning();
			break;
		case IDC_BREAK_BUTTON:	
			SetRSPCommandToStepping();
			break;
		case IDC_STEP_BUTTON:			
			WaitingForStep = FALSE;
			break;
		/*case IDC_SKIP_BUTTON:
			SkipNextRSPOpCode = TRUE;
			WaitingFor_RSPStep   = FALSE;
			break;*/
		case IDC_BP_BUTTON:	
			if (DebugInfo.Enter_BPoint_Window != NULL)
			{
				DebugInfo.Enter_BPoint_Window();
			}
			break;
		case IDC_RSP_REGISTERS_BUTTON:
			Enter_RSP_Register_Window();
			break;
		case IDC_R4300I_DEBUGGER_BUTTON: 
			if (DebugInfo.Enter_R4300i_Commands_Window != NULL)
			{
				DebugInfo.Enter_R4300i_Commands_Window();
			}
			break;
		case IDC_R4300I_REGISTERS_BUTTON:
			if (DebugInfo.Enter_R4300i_Register_Window != NULL)
			{
				DebugInfo.Enter_R4300i_Register_Window();
			}
			break;
		case IDC_MEMORY_BUTTON:
			if (DebugInfo.Enter_Memory_Window != NULL)
			{
				DebugInfo.Enter_Memory_Window();
			}
			break;
		case IDCANCEL:			
			EndDialog( hDlg, IDCANCEL );
			break;
		}
		break;
	case WM_VSCROLL:
		if ((HWND)lParam == hScrlBar)
		{
			DWORD location;
			char Value[20];
			SCROLLINFO si;

			GetWindowText(hAddress,Value,sizeof(Value));
			location = AsciiToHex(Value) & ~3;

			switch (LOWORD(wParam))
			{
			case SB_THUMBTRACK:
				sprintf(Value,"%03X",((short int)HIWORD(wParam) << 2 ));
				SetWindowText(hAddress,Value);
				si.cbSize = sizeof(si);
				si.fMask  = SIF_POS;
				si.nPos   = (short int)HIWORD(wParam);
				SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
				break;
			case SB_LINEDOWN:
				if (location < 0xF88)
				{
					sprintf(Value,"%03X",location + 0x4);
					SetWindowText(hAddress,Value);
					si.cbSize = sizeof(si);
					si.fMask  = SIF_POS;
					si.nPos   = ((location + 0x4) >> 2);
					SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
				}
				else
				{
					sprintf(Value,"%03X",0xF88);
					SetWindowText(hAddress,Value);
					si.cbSize = sizeof(si);
					si.fMask  = SIF_POS;
					si.nPos   = (0xFFC >> 2);
					SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
				}
				break;
			case SB_LINEUP:
				if (location > 0x4 )
				{
					sprintf(Value,"%03X",location - 0x4);
					SetWindowText(hAddress,Value);
					si.cbSize = sizeof(si);
					si.fMask  = SIF_POS;
					si.nPos   = ((location - 0x4) >> 2);
					SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
				}
				else
				{
					sprintf(Value,"%03X",0);
					SetWindowText(hAddress,Value);
					si.cbSize = sizeof(si);
					si.fMask  = SIF_POS;
					si.nPos   = 0;
					SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
				}
				break;
			case SB_PAGEDOWN:				
				if ((location + 0x74)< 0xF88)
				{
					sprintf(Value,"%03X",location + 0x74);
					SetWindowText(hAddress,Value);
					si.cbSize = sizeof(si);
					si.fMask  = SIF_POS;
					si.nPos   = ((location + 0x74) >> 2);
					SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
				}
				else
				{
					sprintf(Value,"%03X",0xF88);
					SetWindowText(hAddress,Value);
					si.cbSize = sizeof(si);
					si.fMask  = SIF_POS;
					si.nPos   = (0xF8F >> 2);
					SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
				}
				break;
			case SB_PAGEUP:
				if ((location - 0x74) > 0x74 )
				{
					sprintf(Value,"%03X",location - 0x74);
					SetWindowText(hAddress,Value);
					si.cbSize = sizeof(si);
					si.fMask  = SIF_POS;
					si.nPos   = ((location - 0x74) >> 2);
					SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
				}
				else
				{
					sprintf(Value,"%03X",0);
					SetWindowText(hAddress,Value);
					si.cbSize = sizeof(si);
					si.fMask  = SIF_POS;
					si.nPos   = 0;
					SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
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

void RSP_Commands_Setup ( HWND hDlg )
{
#define WindowWidth  457
#define WindowHeight 494
	char Location[10];
	DWORD X, Y, WndPos;
	
	hList = CreateWindowEx(WS_EX_STATICEDGE, "LISTBOX","", WS_CHILD | WS_VISIBLE | 
		LBS_OWNERDRAWFIXED | LBS_NOTIFY,14,30,303,445, hDlg, 
		(HMENU)IDC_LIST, hinstDLL,NULL );
	if ( hList)
	{
		SendMessage(hList,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		SendMessage(hList,LB_SETITEMHEIGHT, (WPARAM)0,(LPARAM)MAKELPARAM(14, 0));
	}

	sprintf(Location, "%03X", PrgCount ? *PrgCount : 0);
	hAddress = CreateWindowEx(0,"EDIT",Location, WS_CHILD | ES_UPPERCASE | WS_VISIBLE | 
		WS_BORDER | WS_TABSTOP,375,17,36,18, hDlg,(HMENU)IDC_ADDRESS,hinstDLL, NULL );
	if (hAddress)
	{
		SendMessage(hAddress,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		SendMessage(hAddress,EM_SETLIMITTEXT, (WPARAM)3,(LPARAM)0);
	}

	hFunctionlist = CreateWindowEx(0,"COMBOBOX","", WS_CHILD | WS_VSCROLL |
		CBS_DROPDOWNLIST | CBS_SORT | WS_TABSTOP,352,56,89,150,hDlg,
		(HMENU)IDC_FUNCTION_COMBO,hinstDLL,NULL);
	if (hFunctionlist)
	{
		SendMessage(hFunctionlist,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	} 

	hGoButton = CreateWindowEx(WS_EX_STATICEDGE, "BUTTON","&Go", WS_CHILD | 
		BS_DEFPUSHBUTTON | WS_VISIBLE | WS_TABSTOP, 347,56,100,24, hDlg,(HMENU)IDC_GO_BUTTON,
		hinstDLL,NULL );
	if (hGoButton)
	{
		SendMessage(hGoButton,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	} 

	hBreakButton = CreateWindowEx(WS_EX_STATICEDGE, "BUTTON","&Break", WS_DISABLED | 
		WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,85,100,24,hDlg,
		(HMENU)IDC_BREAK_BUTTON,hinstDLL,NULL );
	if (hBreakButton)
	{
		SendMessage(hBreakButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hStepButton = CreateWindowEx(WS_EX_STATICEDGE, "BUTTON","&Step", WS_CHILD | 
		BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,114,100,24,hDlg,
		(HMENU)IDC_STEP_BUTTON,hinstDLL,NULL );
	if (hStepButton)
	{
		SendMessage(hStepButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hSkipButton = CreateWindowEx(WS_EX_STATICEDGE, "BUTTON","&Skip", WS_CHILD | 
		BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,143,100,24,hDlg,
		(HMENU)IDC_SKIP_BUTTON,hinstDLL,NULL );
	if (hSkipButton)
	{
		SendMessage(hSkipButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	WndPos = 324;
	if (DebugInfo.Enter_BPoint_Window == NULL) { WndPos += 29;}
	if (DebugInfo.Enter_R4300i_Commands_Window == NULL) { WndPos += 29;}
	if (DebugInfo.Enter_R4300i_Register_Window == NULL) { WndPos += 29;}
	if (DebugInfo.Enter_Memory_Window == NULL) { WndPos += 29;}

	if (DebugInfo.Enter_BPoint_Window != NULL)
	{
		hBPButton = CreateWindowEx(WS_EX_STATICEDGE, "BUTTON","&Break Points", WS_CHILD | 
			BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,WndPos,100,24,hDlg,
			(HMENU)IDC_BP_BUTTON,hinstDLL,NULL );
		if (hBPButton)
		{
			SendMessage(hBPButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		}
	}

	WndPos += 29;
	hRSPRegisters = CreateWindowEx(WS_EX_STATICEDGE,"BUTTON", "RSP &Registers...",
		WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,WndPos,100,24,hDlg,
		(HMENU)IDC_RSP_REGISTERS_BUTTON,hinstDLL,NULL );
	if (hRSPRegisters)
	{
		SendMessage(hRSPRegisters,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	} 

	WndPos += 29;
	if (DebugInfo.Enter_R4300i_Commands_Window != NULL)
	{
		hR4300iDebugger = CreateWindowEx(WS_EX_STATICEDGE,"BUTTON", "R4300i &Debugger...", 
			WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,WndPos,100,24,hDlg,
			(HMENU)IDC_R4300I_DEBUGGER_BUTTON,hinstDLL,NULL );
		if (hR4300iDebugger)
		{
			SendMessage(hR4300iDebugger,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		}
	}

	WndPos += 29;
	if (DebugInfo.Enter_R4300i_Register_Window != NULL)
	{
		hR4300iRegisters = CreateWindowEx(WS_EX_STATICEDGE,"BUTTON","R4300i R&egisters...",
			WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,WndPos,100,24,hDlg,
			(HMENU)IDC_R4300I_REGISTERS_BUTTON,hinstDLL,NULL );
		if (hR4300iRegisters)
		{
			SendMessage(hR4300iRegisters,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		}
	}

	WndPos += 29;
	if (DebugInfo.Enter_Memory_Window != NULL)
	{
		hMemory = CreateWindowEx(WS_EX_STATICEDGE,"BUTTON", "&Memory...", WS_CHILD | 
			BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,WndPos,100,24,hDlg,
			(HMENU)IDC_MEMORY_BUTTON,hinstDLL,NULL );
		if (hMemory)
		{
			SendMessage(hMemory,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		}
	}

	hScrlBar = CreateWindowEx(WS_EX_STATICEDGE, "SCROLLBAR","", WS_CHILD | WS_VISIBLE | 
		WS_TABSTOP | SBS_VERT, 318,14,18,439, hDlg, (HMENU)IDC_SCRL_BAR, hinstDLL, NULL );

	if ( RSP_Running )
	{
		Enable_RSP_Commands_Window();
	}
	else
	{
		Disable_RSP_Commands_Window();
	}

	//if ( !GetStoredWinPos("RSP Commands", &X, &Y ) ) {
		X = (GetSystemMetrics( SM_CXSCREEN ) - WindowWidth) / 2;
		Y = (GetSystemMetrics( SM_CYSCREEN ) - WindowHeight) / 2;
	//}
	SetWindowText(hDlg,"RSP Commands");

	SetWindowPos(hDlg,NULL,X,Y,WindowWidth,WindowHeight, SWP_NOZORDER | SWP_SHOWWINDOW);
}

static const char unused_op[] = "invalid";
static const char* mnemonics_primary[8 << 3] = {
    "SPECIAL","REGIMM" ,"J"      ,"JAL"    ,"BEQ"    ,"BNE"    ,"BLEZ"   ,"BGTZ"   ,
    "ADDI"   ,"ADDIU"  ,"SLTI"   ,"SLTIU"  ,"ANDI"   ,"ORI"    ,"XORI"   ,"LUI"    ,
    "COP0"   ,unused_op,"COP2"   ,unused_op,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    "LB"     ,"LH"     ,unused_op,"LW"     ,"LBU"    ,"LHU"    ,unused_op,unused_op,
    "SB"     ,"SH"     ,unused_op,"SW"     ,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,"LWC2"   ,unused_op,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,"SWC2"   ,unused_op,unused_op,unused_op,unused_op,unused_op,
};/*   000   |   001   |   010   |   011   |   100   |   101   |   110   |   111  */
static const char* mnemonics_special[8 << 3] = {
    "SLL"    ,unused_op,"SRL"    ,"SRA"    ,"SLLV"   ,unused_op,"SRLV"   ,"SRAV"   ,
    "JR"     ,"JALR"   ,unused_op,unused_op,unused_op,"BREAK"  ,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    "ADD"    ,"ADDU"   ,"SUB"    ,"SUBU"   ,"AND"    ,"OR"     ,"XOR"    ,"NOR"    ,
    unused_op,unused_op,"SLT"    ,"SLTU"   ,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
};/*   000   |   001   |   010   |   011   |   100   |   101   |   110   |   111  */
static const char* mnemonics_regimm[8 << 2] = {
    "BLTZ"   ,"BGEZ"   ,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    "BLTZAL" ,"BGEZAL" ,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
};/*   000   |   001   |   010   |   011   |   100   |   101   |   110   |   111  */
static const char* mnemonics_cop0[8 << 2] = {
    "MFC0"   ,unused_op,unused_op,unused_op,"MTC0"   ,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
};/*   000   |   001   |   010   |   011   |   100   |   101   |   110   |   111  */
static const char* mnemonics_cop2[8 << 2] = {
    "MFC2"   ,unused_op,"CFC2"   ,unused_op,"MTC2"   ,unused_op,"CTC2"   ,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    "C2"     ,"C2"     ,"C2"     ,"C2"     ,"C2"     ,"C2"     ,"C2"     ,"C2"     ,
    "C2"     ,"C2"     ,"C2"     ,"C2"     ,"C2"     ,"C2"     ,"C2"     ,"C2"     ,
};/*   000   |   001   |   010   |   011   |   100   |   101   |   110   |   111  */
static const char* mnemonics_vector[8 << 3] = {
    "VMULF"  ,"VMULU"  ,unused_op,unused_op,"VMUDL"  ,"VMUDM"  ,"VMUDN"  ,"VMUDH"  ,
    "VMACF"  ,"VMACU"  ,unused_op,"VMACQ"  ,"VMADL"  ,"VMADM"  ,"VMADN"  ,"VMADH"  ,
    "VADD"   ,"VSUB"   ,unused_op,"VABS"   ,"VADDC"  ,"VSUBC"  ,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,"VSAW"   ,unused_op,unused_op,
    "VLT"    ,"VEQ"    ,"VNE"    ,"VGE"    ,"VCL"    ,"VCH"    ,"VCR"    ,"VMRG"   ,
    "VAND"   ,"VNAND"  ,"VOR"    ,"VNOR"   ,"VXOR"   ,"VNXOR"  ,unused_op,unused_op,
    "VRCP"   ,"VRCPL"  ,"VRCPH"  ,"VMOV"   ,"VRSQ"   ,"VRSQL"  ,"VRSQH"  ,"VNOP"   ,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
};/*   000   |   001   |   010   |   011   |   100   |   101   |   110   |   111  */
static const char* mnemonics_lwc2[8 << 2] = {
    "LBV"    ,"LSV"    ,"LLV"    ,"LDV"    ,"LQV"    ,"LRV"    ,"LPV"    ,"LUV"    ,
    "LHV"    ,"LFV"    ,unused_op,"LTV"    ,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
};/*   000   |   001   |   010   |   011   |   100   |   101   |   110   |   111  */
static const char* mnemonics_swc2[8 << 2] = {
    "SBV"    ,"SSV"    ,"SLV"    ,"SDV"    ,"SQV"    ,"SRV"    ,"SPV"    ,"SUV"    ,
    "SHV"    ,"SFV"    ,"SWV"    ,"STV"    ,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
    unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,unused_op,
};/*   000   |   001   |   010   |   011   |   100   |   101   |   110   |   111  */


char * RSPSpecialName ( DWORD OpCode, DWORD PC )
{
	OPCODE command;
	command.Hex = OpCode;
	
	PC = PC; // unused

	if (strcmp(mnemonics_special[command.funct], unused_op) == 0)
	{
		sprintf(CommandName, "RSP: Unknown\t%02X %02X %02X %02X",
			command.Ascii[3],
			command.Ascii[2],
			command.Ascii[1],
			command.Ascii[0]
		);
	}
	else if (command.Hex == 0x00000000)
	{
		strcpy(CommandName, "NOP");
	}
	else if (command.funct >= RSP_SPECIAL_SLL && command.funct < RSP_SPECIAL_SLLV)
	{
		sprintf(CommandName, "%s\t%s, %s, 0x%X",
			mnemonics_special[command.funct],
			GPR_Name(command.rd),
			GPR_Name(command.rt),
			command.sa
		);
	}
	else if (command.funct >= RSP_SPECIAL_SLLV && command.funct < RSP_SPECIAL_JR)
	{
		sprintf(CommandName, "%s\t%s, %s, %s",
			mnemonics_special[command.funct],
			GPR_Name(command.rd),
			GPR_Name(command.rt),
			GPR_Name(command.rs)
		);
	}
	else if (command.funct == RSP_SPECIAL_JR)
	{
		sprintf(CommandName, "%s\t%s",
			mnemonics_special[command.funct],
			GPR_Name(command.rs)
		);
	}
	else if (command.funct == RSP_SPECIAL_JALR)
	{
		sprintf(CommandName, "%s\t%s, %s",
			mnemonics_special[command.funct],
			GPR_Name(command.rd),
			GPR_Name(command.rs)
		);
	}
	else if (command.funct == RSP_SPECIAL_BREAK)
	{
		strcpy(CommandName, mnemonics_special[RSP_SPECIAL_BREAK]);
	}
	else
	{
		sprintf(CommandName, "%s\t%s, %s, %s",
			mnemonics_special[command.funct],
			GPR_Name(command.rd),
			GPR_Name(command.rs),
			GPR_Name(command.rt)
		);
	}
	return CommandName;
}

char * RSPRegimmName ( DWORD OpCode, DWORD PC )
{
	OPCODE command;
	command.Hex = OpCode;

	if (strcmp(mnemonics_regimm[command.rt], unused_op) == 0)
	{
		sprintf(
			CommandName,
			"RSP: Unknown\t%02X %02X %02X %02X",
			command.Ascii[3],
			command.Ascii[2],
			command.Ascii[1],
			command.Ascii[0]
		);
	}
	else if (command.rt == RSP_REGIMM_BGEZAL && command.rs == 0)
	{ /* MIPS pseudo-instruction:  BAL (Branch and Link) */
		sprintf(
			CommandName,
			"BAL\t0x%04X",
			(PC + ((short)command.offset << 2) + 4) & 0x1FFC
		);
	}
	else
	{
		sprintf(
			CommandName,
			"%s\t%s, 0x%04X",
			mnemonics_regimm[command.rt],
			GPR_Name(command.rs),
			(PC + ((short)command.offset << 2) + 4) & 0x1FFC
		);
	}
	return CommandName;
}

char * RSPCop0Name ( DWORD OpCode, DWORD PC )
{
	OPCODE command;
	command.Hex = OpCode;

	PC = PC; // unused

	if (strcmp(mnemonics_cop0[command.rs], unused_op) == 0)
	{
		sprintf(
			CommandName,
			"RSP: Unknown\t%02X %02X %02X %02X",
			command.Ascii[3],
			command.Ascii[2],
			command.Ascii[1],
			command.Ascii[0]
		);
	}
	else
	{
		sprintf(
			CommandName,
			"%s\t%s, %s",
			mnemonics_cop0[command.rs],
			GPR_Name(command.rt),
			COP0_Name(command.rd)
		);
	}
	return CommandName;
}

char * RSPCop2Name ( DWORD OpCode, DWORD PC )
{
	OPCODE command;
	command.Hex = OpCode;

	PC = PC; // unused

	if ( ( command.rs & 0x10 ) == 0 )
	{
		if (strcmp(mnemonics_cop2[command.rs], unused_op) == 0)
		{
			sprintf(CommandName, "RSP: Unknown\t%02X %02X %02X %02X",
				command.Ascii[3],
				command.Ascii[2],
				command.Ascii[1],
				command.Ascii[0]
			);
		}
		else if (command.rs & 002) /* CFC2 or CTC2 */
		{
			sprintf(CommandName, "%s\t%s, %d",
				mnemonics_cop2[command.rs],
				GPR_Name(command.rt),
				command.rd % 4
			);
		}
		else
		{
			sprintf(CommandName, "%s\t%s, $v%d[%d]",
				mnemonics_cop2[command.rs],
				GPR_Name(command.rt),
				command.rd,
				command.sa >> 1
			);
		}
	}
	else
	{
		if (strcmp(mnemonics_vector[command.funct], unused_op) == 0)
		{
			sprintf(CommandName, "RSP: Unknown\t%02X %02X %02X %02X",
				command.Ascii[3],
				command.Ascii[2],
				command.Ascii[1],
				command.Ascii[0]
			);
		}
		else if (command.funct >= RSP_VECTOR_VRCP && command.funct < RSP_VECTOR_VNOOP)
		{ /* RSP division -- VRCP[L,H], VRSQ[L,H], and VMOV */
			sprintf(CommandName, "%s\t$v%d[%d], $v%d%s",
				mnemonics_vector[command.funct],
				command.sa,
				command.rd & 0x7,
				command.rt,
				ElementSpecifier(command.rs & 0xF)
			);
		}
		else if (command.funct == RSP_VECTOR_VNOOP)
		{
			strcpy(CommandName, mnemonics_vector[RSP_VECTOR_VNOOP]);
		}
		else
		{
			sprintf(CommandName, "%s\t$v%d, $v%d, $v%d%s",
				mnemonics_vector[command.funct],
				command.sa,
				command.rd,
				command.rt,
				ElementSpecifier(command.rs & 0xF)
			);
		}
	}
	return CommandName;
}

char * RSPLc2Name ( DWORD OpCode, DWORD PC )
{
	OPCODE command;
	command.Hex = OpCode;

	PC = PC; // unused

	if (strcmp(mnemonics_lwc2[command.rd], unused_op) == 0)
	{
		sprintf(
			CommandName,
			"RSP: Unknown\t%02X %02X %02X %02X",
			command.Ascii[3],
			command.Ascii[2],
			command.Ascii[1],
			command.Ascii[0]
		);
	}
	else
	{
		sprintf(
			CommandName,
			"%s\t$v%d[%d], %c0x%03X(%s)",
			mnemonics_lwc2[command.rd],
			command.rt,
			command.del,
			(command.voffset < 0) ? '-' : '+',
			abs(command.voffset),
			GPR_Name(command.base)
		);
	}
	return CommandName;
}

char * RSPSc2Name ( DWORD OpCode, DWORD PC )
{
	OPCODE command;
	command.Hex = OpCode;

	PC = PC; // unused

	if (strcmp(mnemonics_swc2[command.rd], unused_op) == 0)
	{
		sprintf(
			CommandName,
			"RSP: Unknown\t%02X %02X %02X %02X",
			command.Ascii[3],
			command.Ascii[2],
			command.Ascii[1],
			command.Ascii[0]
		);
	}
	else
	{
		sprintf(
			CommandName,
			"%s\t$v%d[%d], %c0x%03X(%s)",
			mnemonics_swc2[command.rd],
			command.rt,
			command.del,
			(command.voffset < 0) ? '-' : '+',
			abs(command.voffset),
			GPR_Name(command.base)
		);
	}
	return CommandName;
}

char * RSPOpcodeName ( DWORD OpCode, DWORD PC )
{
	OPCODE command;
	command.Hex = OpCode;

	switch (command.op)
	{
	case RSP_SPECIAL:
		return RSPSpecialName(OpCode,PC);
		break;
	case RSP_REGIMM:
		return RSPRegimmName(OpCode,PC);
		break;
	case RSP_J:
	case RSP_JAL:
		sprintf(CommandName, "%s\t0x%04X",
			mnemonics_primary[command.op],
			(command.target << 2) & 0x1FFC
		);
		break;
	case RSP_BEQ:
		if (command.rs == 0 && command.rt == 0)
		{
			sprintf(CommandName, "%s\t0x%04X",
				"B",
				(PC + ((short)command.offset << 2) + 4) & 0x1FFC
			);
			break;
		}
		else if (command.rs == 0 || command.rt == 0)
		{
			sprintf(CommandName, "%s\t%s, 0x%04X",
				"BEQZ",
				GPR_Name(command.rs == 0 ? command.rt : command.rs),
				(PC + ((short)command.offset << 2) + 4) & 0x1FFC
			);
			break;
		}
		/* else { fall through to show the full BEQ } */
	case RSP_BNE:
		sprintf(CommandName, "%s\t%s, %s, 0x%04X",
			mnemonics_primary[command.op],
			GPR_Name(command.rs),
			GPR_Name(command.rt),
			(PC + ((short)command.offset << 2) + 4) & 0x1FFC
		);
		break;
	case RSP_BLEZ:
	case RSP_BGTZ:
		sprintf(CommandName, "%s\t%s, 0x%04X",
			mnemonics_primary[command.op],
			GPR_Name(command.rs),
			(PC + ((short)command.offset << 2) + 4) & 0x1FFC
		);
		break;
	case RSP_ADDI:
	case RSP_ADDIU:
	case RSP_SLTI:
	case RSP_SLTIU:
	case RSP_ANDI:
	case RSP_ORI:
	case RSP_XORI:
		sprintf(CommandName, "%s\t%s, %s, 0x%04X",
			mnemonics_primary[command.op],
			GPR_Name(command.rt),
			GPR_Name(command.rs),
			command.immediate
		);
		break;
	case RSP_LUI:
		sprintf(CommandName, "%s\t%s, 0x%04X",
			mnemonics_primary[RSP_LUI],
			GPR_Name(command.rt),
			command.immediate
		);
		break;
	case RSP_CP0:
		return RSPCop0Name(OpCode,PC);
		break;
	case RSP_CP2:
		return RSPCop2Name(OpCode,PC);
		break;
	case RSP_LB:
	case RSP_LH:
	case RSP_LW:
	case RSP_LBU:
	case RSP_LHU:
	case RSP_SB:
	case RSP_SH:
	case RSP_SW:
		sprintf(CommandName, "%s\t%s, %c0x%04X(%s)",
			mnemonics_primary[command.op],
			GPR_Name(command.rt),
			((int16_t)command.offset < 0) ? '-' : '+',
			abs((int16_t)command.offset),
			GPR_Name(command.base)
		);
		break;
	case RSP_LC2:
		return RSPLc2Name(OpCode,PC);
		break;
	case RSP_SC2:
		return RSPSc2Name(OpCode,PC);
		break;
	default:
		sprintf(CommandName, "RSP: Unknown\t%02X %02X %02X %02X",
			command.Ascii[3],
			command.Ascii[2],
			command.Ascii[1],
			command.Ascii[0]
		);
	}
	return CommandName;
}

void SetRSPCommandToRunning ( void )
{
	Stepping_Commands = FALSE;
	if (InRSPCommandsWindow == FALSE) { return; }
	EnableWindow(hGoButton,    FALSE);
	EnableWindow(hBreakButton, TRUE);
	EnableWindow(hStepButton,  FALSE);
	EnableWindow(hSkipButton,  FALSE);
	SendMessage(RSPCommandshWnd, DM_SETDEFID,IDC_BREAK_BUTTON,0);
	SendMessage(hGoButton, BM_SETSTYLE,BS_PUSHBUTTON,TRUE);
	SendMessage(hBreakButton, BM_SETSTYLE,BS_DEFPUSHBUTTON,TRUE);
	SetFocus(hBreakButton);
}

void SetRSPCommandToStepping ( void )
{
	if (InRSPCommandsWindow == FALSE) { return; }
	EnableWindow(hGoButton,    TRUE);
	EnableWindow(hBreakButton, FALSE);
	EnableWindow(hStepButton,  TRUE);
	EnableWindow(hSkipButton,  TRUE);
	SendMessage(hBreakButton, BM_SETSTYLE, BS_PUSHBUTTON,TRUE);
	SendMessage(hStepButton, BM_SETSTYLE, BS_DEFPUSHBUTTON,TRUE);
	SendMessage(RSPCommandshWnd, DM_SETDEFID,IDC_STEP_BUTTON,0);
	SetFocus(hStepButton);
	Stepping_Commands = TRUE;
}

void SetRSPCommandViewto ( UINT NewLocation )
{
	unsigned int location;
	char Value[20];

	if (InRSPCommandsWindow == FALSE) { return; }

	GetWindowText(hAddress,Value,sizeof(Value));
	location = AsciiToHex(Value) & ~3;

	if ( NewLocation < location || NewLocation >= location + 120 )
	{
		sprintf(Value,"%03X",NewLocation);
		SetWindowText(hAddress,Value);
	}
	else
	{
		RefreshRSPCommands();
	}
}
