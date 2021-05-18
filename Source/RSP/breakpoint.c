#include <windows.h>
#include <stdio.h>
#include "Rsp.h"
#include "CPU.h"
#include "breakpoint.h"

#define IDC_LOCATION_EDIT		105
HWND BPoint_Win_hDlg, hRSPLocation = NULL;

void Add_BPoint ( void )
{
	char Title[10];

	GetWindowText(hRSPLocation,Title,sizeof(Title));
	if (!AddRSP_BPoint(AsciiToHex(Title),TRUE )) {
		SendMessage(hRSPLocation,EM_SETSEL,(WPARAM)0,(LPARAM)-1);
		SetFocus(hRSPLocation);
	}
}

int AddRSP_BPoint( DWORD Location, int Confirm )
{
	int count;

	if (NoOfBpoints == MaxBPoints)
	{
		DisplayError("Max amount of breakpoints set");
		return FALSE;
	}

	for (count = 0; count < NoOfBpoints; count ++)
	{
		if (BPoint[count].Location == Location)
		{
			DisplayError("You already have this breakpoint");
			return FALSE;
		}
	}

	if (Confirm)
	{
		char Message[150];
		int Response;

		sprintf(Message,"Break when:\n\nRSP's program counter = 0x%03X\n\nIs this correct?",
			Location);
		Response = MessageBox(BPoint_Win_hDlg, Message, "Breakpoint", MB_YESNO | MB_ICONINFORMATION);
		if (Response == IDNO)
		{
			return FALSE;
		}
	}
	BPoint[NoOfBpoints].Location = Location;
	NoOfBpoints += 1;
	if (DebugInfo.UpdateBreakPoints)
	{
		DebugInfo.UpdateBreakPoints();
	}
	return TRUE;
}

int CheckForRSPBPoint ( DWORD Location )
{
	int count;
	
	for (count = 0; count < NoOfBpoints; count ++)
	{
		if (BPoint[count].Location == Location)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void CreateBPPanel ( void * hDlg, rectangle rcBox )
{
	if (hRSPLocation != NULL) { return; }

	rcBox = rcBox; // Remove warning of unused

	BPoint_Win_hDlg = hDlg;
	
	hRSPLocation = CreateWindowEx(0,"EDIT","", WS_CHILD | WS_BORDER | ES_UPPERCASE | WS_TABSTOP,
		83,90,100,17,hDlg,(HMENU)IDC_LOCATION_EDIT,RSPInfo.hInst,NULL);
	if (hRSPLocation)
	{
		char Title[20];
		SendMessage(hRSPLocation,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		SendMessage(hRSPLocation,EM_SETLIMITTEXT,(WPARAM)3,(LPARAM)0);
		sprintf(Title,"%03X",*PrgCount);
		SetWindowText(hRSPLocation,Title);
	}
}

void HideBPPanel ( void )
{
	ShowWindow(hRSPLocation,FALSE);
}

void PaintBPPanel ( window_paint ps )
{
	TextOut( ps.hdc, 29,60,"Break when the program counter equals",37);
	TextOut( ps.hdc, 59,85,"0x",2);
}

void ShowBPPanel ( void )
{
	ShowWindow(hRSPLocation,TRUE);
}

void RefreshBpoints ( void * hList )
{
	char Message[100];
	LRESULT location;
	int count;

	for (count = 0; count < NoOfBpoints; count ++ ) {
		sprintf(Message," at 0x%03X (RSP)", BPoint[count].Location);
		location = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)Message);
		SendMessage(
			hList,
			LB_SETITEMDATA,
			(WPARAM)location,
			(LPARAM)BPoint[count].Location
		);
	}
}

void RemoveAllBpoint ( void )
{
	NoOfBpoints = 0;
}

void RemoveBpoint ( HWND hList, int index )
{
	LRESULT response;
	uint32_t location;

	response = SendMessage(hList, LB_GETITEMDATA, (WPARAM)index, 0);
	if (response < 0 || response > 0x7FFFFFFFL)
	{
		DisplayError(
			"LB_GETITEMDATA response for %i out of DWORD range.",
			index
		);
	}
	location = (uint32_t)response;
	RemoveRSPBreakPoint(location);
}

void RemoveRSPBreakPoint (DWORD Location)
{
	int count, location = -1;
	
	for (count = 0; count < NoOfBpoints; count ++)
	{
		if (BPoint[count].Location == Location)
		{
			location = count;
			count = NoOfBpoints;
		}
	}
	
	if (location >= 0)
	{
		for (count = location; count < NoOfBpoints - 1; count ++ )
		{
			BPoint[count].Location = BPoint[count + 1].Location;
		}
		NoOfBpoints -= 1;
		if (DebugInfo.UpdateBreakPoints)
		{
			DebugInfo.UpdateBreakPoints();
		}
	}
}
