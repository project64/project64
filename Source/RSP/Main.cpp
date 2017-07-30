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

#ifdef _WIN32
#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1910
#include <intrin.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Common/StdString.h>
#include <Common/stdtypes.h>
#include "../Settings/Settings.h"

extern "C" {
#include "Rsp.h"
#include "Cpu.h"
#include "Recompiler CPU.h"
#include "RSP Command.h"
#include "RSP Registers.h"
#include "memory.h"
#include "breakpoint.h"
#include "Profiling.h"
#include "log.h"
#include "resource.h"
#include "Version.h"
#include "Types.h"

void ClearAllx86Code(void);
void ProcessMenuItem(int ID);
#ifdef _WIN32
Boolean CALLBACK CompilerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
HMENU hRSPMenu = NULL;
#endif

Boolean GraphicsHle = TRUE, AudioHle, ConditionalMove;
Boolean DebuggingEnabled = FALSE, 
        Profiling, 
        IndvidualBlock, 
        ShowErrors, 
        BreakOnStart = FALSE,
        LogRDP = FALSE,
        LogX86Code = FALSE;
uint32_t CPUCore = RecompilerCPU;

void * hMutex = NULL;

DEBUG_INFO DebugInfo;
RSP_INFO RSPInfo;
void * hinstDLL;

extern uint8_t * pLastSecondary;
}

enum {
	Set_BreakOnStart, Set_CPUCore, Set_LogRDP, Set_LogX86Code, Set_Profiling, Set_IndvidualBlock,
	Set_ShowErrors, 

	//Compiler settings
	Set_CheckDest, Set_Accum, Set_Mmx, Set_Mmx2, Set_Sse, Set_Sections,
	Set_ReOrdering, Set_GPRConstants, Set_Flags, Set_AlignVector,

	//Game Settings
	Set_JumpTableSize, Set_Mfc0Count, Set_SemaphoreExit
};

short Set_AudioHle = 0, Set_GraphicsHle = 0;

/************ DLL info **************/
const char * AppName ( void ) 
{
	static stdstr_f Name("RSP %s", VER_FILE_VERSION_STR);
	return Name.c_str();
}
const char * AboutMsg ( void ) 
{
	static stdstr_f Msg("RSP emulation Plugin\nMade for Project64 (c)\nVersion %s\n\nby Jabo & Zilmar", VER_FILE_VERSION_STR);
	return Msg.c_str();
}

/************ Functions ***********/
uint32_t AsciiToHex(char * HexValue)
{
    size_t Finish, Count;
    uint32_t Value = 0;

    Finish = strlen(HexValue);
	if (Finish > 8 ) { Finish = 8; }

    for (Count = 0; Count < Finish; Count++)
	{
		Value = (Value << 4);
		switch( HexValue[Count] )
		{
		case '0': break;
		case '1': Value += 1; break;
		case '2': Value += 2; break;
		case '3': Value += 3; break;
		case '4': Value += 4; break;
		case '5': Value += 5; break;
		case '6': Value += 6; break;
		case '7': Value += 7; break;
		case '8': Value += 8; break;
		case '9': Value += 9; break;
		case 'A': Value += 10; break;
		case 'a': Value += 10; break;
		case 'B': Value += 11; break;
		case 'b': Value += 11; break;
		case 'C': Value += 12; break;
		case 'c': Value += 12; break;
		case 'D': Value += 13; break;
		case 'd': Value += 13; break;
		case 'E': Value += 14; break;
		case 'e': Value += 14; break;
		case 'F': Value += 15; break;
		case 'f': Value += 15; break;
		default: 
			Value = (Value >> 4);
			Count = Finish;
		}
	}
	return Value;
}

void DisplayError(char* Message, ...)
{
    char Msg[400];
    va_list ap;

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
#ifdef _WIN32
    MessageBox(NULL, Msg, "Error", MB_OK | MB_ICONERROR);
#else
    fputs(&Msg[0], stderr);
#endif
}

/******************************************************************
  Function: CloseDLL
  Purpose:  This function is called when the emulator is closing
            down allowing the dll to de-initialise.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CloseDLL(void)
{
	FreeMemory();
}

/******************************************************************
  Function: DllAbout
  Purpose:  This function is optional function that is provided
            to give further information about the DLL.
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void DllAbout(void * hParent)
{
#ifdef _WIN32
    MessageBox((HWND)hParent, AboutMsg(), "About", MB_OK | MB_ICONINFORMATION);
#else
    puts(AboutMsg());
#endif
}

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD /*fdwReason*/, LPVOID /*lpvReserved*/)
{
    hinstDLL = hinst;
    return TRUE;
}

void FixMenuState(void)
{
    EnableMenuItem(hRSPMenu, ID_RSPCOMMANDS,MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
    EnableMenuItem(hRSPMenu, ID_RSPREGISTERS,MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
    EnableMenuItem(hRSPMenu, ID_PROFILING_RESETSTATS, MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
    EnableMenuItem(hRSPMenu, ID_PROFILING_GENERATELOG, MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
    EnableMenuItem(hRSPMenu, ID_DUMP_RSPCODE, MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
    EnableMenuItem(hRSPMenu, ID_DUMP_DMEM, MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));

    CheckMenuItem(hRSPMenu, ID_CPUMETHOD_RECOMPILER, MF_BYCOMMAND | (CPUCore == RecompilerCPU  ?  MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_CPUMETHOD_INTERPT,    MF_BYCOMMAND | (CPUCore == InterpreterCPU ?  MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_BREAKONSTARTOFTASK, MF_BYCOMMAND | (BreakOnStart ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_LOGRDPCOMMANDS, MF_BYCOMMAND | (LogRDP ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_SETTINGS_LOGX86CODE, MF_BYCOMMAND | (LogX86Code ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_PROFILING_ON, MF_BYCOMMAND | (Profiling ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_PROFILING_OFF, MF_BYCOMMAND | (Profiling ? MFS_UNCHECKED : MF_CHECKED));
    CheckMenuItem(hRSPMenu, ID_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND | (IndvidualBlock ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_SHOWCOMPILERERRORS, MF_BYCOMMAND | (ShowErrors ? MFS_CHECKED : MF_UNCHECKED));
}
#endif

/******************************************************************
  Function: GetDllInfo
  Purpose:  This function allows the emulator to gather information
            about the dll by filling in the PluginInfo structure.
  input:    a pointer to a PLUGIN_INFO stucture that needs to be
            filled by the function. (see def above)
  output:   none
*******************************************************************/ 
EXPORT void GetDllInfo(PLUGIN_INFO * PluginInfo)
{
	PluginInfo->Version = 0x0103;
	PluginInfo->Type = PLUGIN_TYPE_RSP;
#ifdef _DEBUG
	sprintf(PluginInfo->Name, "RSP Debug Plugin %s", VER_FILE_VERSION_STR);
#else
	sprintf(PluginInfo->Name, "RSP Plugin %s", VER_FILE_VERSION_STR);
#endif
	PluginInfo->NormalMemory = FALSE;
	PluginInfo->MemoryBswaped = TRUE;
}

/******************************************************************
  Function: GetRspDebugInfo
  Purpose:  This function allows the emulator to gather information
            about the debug capabilities of the dll by filling in
			the DebugInfo structure.
  input:    a pointer to a RSPDEBUG_INFO stucture that needs to be
            filled by the function. (see def above)
  output:   none
*******************************************************************/ 

EXPORT void GetRspDebugInfo(RSPDEBUG_INFO * DebugInfo)
{
#ifdef _WIN32
	if (hRSPMenu == NULL)
	{
		hRSPMenu = LoadMenu((HINSTANCE)hinstDLL,MAKEINTRESOURCE(RspMenu));
		FixMenuState();
	}
	DebugInfo->hRSPMenu = hRSPMenu;
#endif
	DebugInfo->ProcessMenuItem = ProcessMenuItem;

	DebugInfo->UseBPoints = TRUE;
	sprintf(DebugInfo->BPPanelName," RSP ");
	DebugInfo->Add_BPoint = Add_BPoint;
	DebugInfo->CreateBPPanel = CreateBPPanel;
	DebugInfo->HideBPPanel = HideBPPanel;
	DebugInfo->PaintBPPanel = PaintBPPanel;
	DebugInfo->RefreshBpoints = RefreshBpoints;
	DebugInfo->RemoveAllBpoint = RemoveAllBpoint;
	DebugInfo->RemoveBpoint = RemoveBpoint;
	DebugInfo->ShowBPPanel = ShowBPPanel;

	DebugInfo->Enter_RSP_Commands_Window = Enter_RSP_Commands_Window;
}

/******************************************************************
  Function: InitiateRSP
  Purpose:  This function is called when the DLL is started to give
            information from the emulator that the n64 RSP 
			interface needs
  input:    Rsp_Info is passed to this function which is defined
            above.
			CycleCount is the number of cycles between switching
			control between teh RSP and r4300i core.
  output:   none
*******************************************************************/ 

RSP_COMPILER Compiler;

void DetectCpuSpecs(void)
{
	DWORD Intel_Features = 0;
	DWORD AMD_Features = 0;

#if defined(_MSC_VER)
	__try {
#ifdef _M_IX86
		_asm {
			/* Intel features */
			mov eax, 1
			cpuid
			mov [Intel_Features], edx

			/* AMD features */
			mov eax, 80000001h
			cpuid
			or [AMD_Features], edx
		}
#else
		int cpuInfo[4];
		__cpuid(cpuInfo, 1);
		Intel_Features = cpuInfo[3];
		__cpuid(cpuInfo, 0x80000001);
		AMD_Features = cpuInfo[3];
#endif
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		AMD_Features = Intel_Features = 0;
    }
#else
/*
 * To do:  With GCC, there is <cpuid.h>, but __cpuid() there is a macro and
 *         needs five arguments, not two.  Also, GCC lacks SEH.
 */
	AMD_Features = Intel_Features = 0;
#endif

	if (Intel_Features & 0x02000000)
	{
		Compiler.mmx2 = TRUE;
		Compiler.sse = TRUE;
	}
	if (Intel_Features & 0x00800000)
	{
		Compiler.mmx = TRUE;
	}
	if (AMD_Features & 0x40000000)
	{
		Compiler.mmx2 = TRUE;
	}
	if (Intel_Features & 0x00008000)
	{
		ConditionalMove = TRUE;
	}
	else
	{
		ConditionalMove = FALSE;
	}
}

EXPORT void InitiateRSP(RSP_INFO Rsp_Info, uint32_t * CycleCount)
{
	RSPInfo = Rsp_Info;
	AudioHle = GetSystemSetting(Set_AudioHle);
	GraphicsHle = GetSystemSetting(Set_GraphicsHle);
	
	*CycleCount = 0;
	AllocateMemory();
	InitilizeRSPRegisters();
	Build_RSP();
	#ifdef GenerateLog
	Start_Log();
	#endif
}

/******************************************************************
  Function: InitiateRSPDebugger
  Purpose:  This function is called when the DLL is started to give
            information from the emulator that the n64 RSP 
			interface needs to intergrate the debugger with the
			rest of the emulator.
  input:    DebugInfo is passed to this function which is defined
            above.
  output:   none
*******************************************************************/ 
EXPORT void InitiateRSPDebugger(DEBUG_INFO Debug_Info)
{
	DebugInfo = Debug_Info;
}

#ifdef _WIN32
void ProcessMenuItem(int ID)
{
	UINT uState;

	switch (ID)
	{
	case ID_RSPCOMMANDS: Enter_RSP_Commands_Window(); break;
	case ID_RSPREGISTERS: Enter_RSP_Register_Window(); break;
	case ID_DUMP_RSPCODE: DumpRSPCode(); break;
	case ID_DUMP_DMEM: DumpRSPData(); break;
	case ID_PROFILING_ON:
	case ID_PROFILING_OFF:
		{
			uState = GetMenuState(hRSPMenu, ID_PROFILING_ON, MF_BYCOMMAND);
			
			if (uState & MFS_CHECKED)
			{
				CheckMenuItem( hRSPMenu, ID_PROFILING_ON, MF_BYCOMMAND | MFS_UNCHECKED );
				CheckMenuItem( hRSPMenu, ID_PROFILING_OFF, MF_BYCOMMAND | MFS_CHECKED );
				SetSetting(Set_Profiling,FALSE);
				if (DebuggingEnabled) { Profiling = FALSE; }
			}
			else
			{
				CheckMenuItem( hRSPMenu, ID_PROFILING_ON, MF_BYCOMMAND | MFS_CHECKED );
				CheckMenuItem( hRSPMenu, ID_PROFILING_OFF, MF_BYCOMMAND | MFS_UNCHECKED );
				SetSetting(Set_Profiling,TRUE);
				if (DebuggingEnabled) {  Profiling = TRUE; }
			}
		}
		break;
	case ID_PROFILING_RESETSTATS: ResetTimerList(); break;
	case ID_PROFILING_GENERATELOG: GenerateTimerResults(); break;
	case ID_PROFILING_LOGINDIVIDUALBLOCKS:
		{
			uState = GetMenuState(hRSPMenu, ID_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND);
			
			if (uState & MFS_CHECKED)
			{
				CheckMenuItem( hRSPMenu, ID_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND | MFS_UNCHECKED );
				SetSetting(Set_IndvidualBlock,FALSE);
				if (DebuggingEnabled) { IndvidualBlock = FALSE; }
			}
			else
			{
				CheckMenuItem( hRSPMenu, ID_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND | MFS_CHECKED );
				SetSetting(Set_IndvidualBlock,TRUE);
				if (DebuggingEnabled) {  IndvidualBlock = TRUE; }
			}
		}
		break;
	case ID_SHOWCOMPILERERRORS:
		{
			uState = GetMenuState(hRSPMenu, ID_SHOWCOMPILERERRORS, MF_BYCOMMAND);
			
			if (uState & MFS_CHECKED)
			{
				CheckMenuItem( hRSPMenu, ID_SHOWCOMPILERERRORS, MF_BYCOMMAND | MFS_UNCHECKED );
				SetSetting(Set_ShowErrors,FALSE);
				if (DebuggingEnabled) { ShowErrors = FALSE; }
			}
			else
			{
				CheckMenuItem( hRSPMenu, ID_SHOWCOMPILERERRORS, MF_BYCOMMAND | MFS_CHECKED );
				SetSetting(Set_ShowErrors,TRUE);
				if (DebuggingEnabled) {  ShowErrors = TRUE; }
			}
		}
		break;
	case ID_COMPILER:		
		DialogBox((HINSTANCE)hinstDLL, "RSPCOMPILER", HWND_DESKTOP, (DLGPROC)CompilerDlgProc);
		break;
	case ID_BREAKONSTARTOFTASK:
		{
			uState = GetMenuState(hRSPMenu, ID_BREAKONSTARTOFTASK, MF_BYCOMMAND);
			
			if (uState & MFS_CHECKED)
			{
				CheckMenuItem( hRSPMenu, ID_BREAKONSTARTOFTASK, MF_BYCOMMAND | MFS_UNCHECKED );
				SetSetting(Set_BreakOnStart,FALSE);
				if (DebuggingEnabled) { BreakOnStart = FALSE; }
			}
			else
			{
				CheckMenuItem( hRSPMenu, ID_BREAKONSTARTOFTASK, MF_BYCOMMAND | MFS_CHECKED );
				SetSetting(Set_BreakOnStart,TRUE);
				if (DebuggingEnabled) {  BreakOnStart = TRUE; }
			}
		}
		break;
	case ID_LOGRDPCOMMANDS:
		{
			uState = GetMenuState(hRSPMenu, ID_LOGRDPCOMMANDS, MF_BYCOMMAND);
			
			if (uState & MFS_CHECKED)
			{
				CheckMenuItem( hRSPMenu, ID_LOGRDPCOMMANDS, MF_BYCOMMAND | MFS_UNCHECKED );
				SetSetting(Set_LogRDP,FALSE);
				if (DebuggingEnabled) 
				{
					LogRDP = FALSE; 
					StopRDPLog();
				}
			}
			else
			{
				CheckMenuItem( hRSPMenu, ID_LOGRDPCOMMANDS, MF_BYCOMMAND | MFS_CHECKED );
				SetSetting(Set_LogRDP,TRUE);
				if (DebuggingEnabled)
				{
					LogRDP = TRUE; 
					StartRDPLog();
				}
			}
		}
		break;
	case ID_SETTINGS_LOGX86CODE:
		{
			uState = GetMenuState(hRSPMenu, ID_SETTINGS_LOGX86CODE, MF_BYCOMMAND);
			
			if (uState & MFS_CHECKED)
			{
				CheckMenuItem( hRSPMenu, ID_SETTINGS_LOGX86CODE, MF_BYCOMMAND | MFS_UNCHECKED );
				SetSetting(Set_LogX86Code,FALSE);
				if (DebuggingEnabled) 
				{
					LogX86Code = FALSE; 
					StopCPULog();
				}
			}
			else
			{
				CheckMenuItem( hRSPMenu, ID_SETTINGS_LOGX86CODE, MF_BYCOMMAND | MFS_CHECKED );
				SetSetting(Set_LogX86Code,TRUE);
				if (DebuggingEnabled)
				{
					LogX86Code = TRUE; 
					StartCPULog();
				}
			}
		}
		break;
	case ID_CPUMETHOD_RECOMPILER:
		{
			SetSetting(Set_CPUCore,RecompilerCPU);
			CPUCore = RecompilerCPU;
			FixMenuState();
			SetCPU(RecompilerCPU);
		}
		break;
	case ID_CPUMETHOD_INTERPT:
		{
			SetSetting(Set_CPUCore,InterpreterCPU);
			CPUCore = InterpreterCPU;
			FixMenuState();
			SetCPU(InterpreterCPU);
		}
		break;
	}
}
#endif

/******************************************************************
  Function: RomOpen
  Purpose:  This function is called when a rom is opened.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void RomOpen(void)
{
	ClearAllx86Code();
	if (DebuggingEnabled)
	{
		EnableDebugging(true);
	}
	JumpTableSize = GetSetting(Set_JumpTableSize); 
	Mfc0Count = GetSetting(Set_Mfc0Count);
	SemaphoreExit = GetSetting(Set_SemaphoreExit);
}

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void RomClosed(void)
{
	if (Profiling)
	{
		StopTimer();
		GenerateTimerResults();
	}
	ClearAllx86Code();
	StopRDPLog();
	StopCPULog();

#ifdef GenerateLog
	Stop_Log();
#endif
}

#ifdef _WIN32
static BOOL GetBooleanCheck(HWND hDlg, DWORD DialogID)
{
	return (IsDlgButtonChecked(hDlg, DialogID) == BST_CHECKED) ? TRUE : FALSE;
}

BOOL CALLBACK CompilerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
	char Buffer[256];

	switch (uMsg)
	{
	case WM_INITDIALOG:	
		if (Compiler.bDest == TRUE)
			CheckDlgButton(hDlg, IDC_COMPILER_DEST, BST_CHECKED);
		if (Compiler.mmx == TRUE) 
			CheckDlgButton(hDlg, IDC_CHECK_MMX, BST_CHECKED);
		if (Compiler.mmx2 == TRUE)
			CheckDlgButton(hDlg, IDC_CHECK_MMX2, BST_CHECKED);
		if (Compiler.sse == TRUE) 
			CheckDlgButton(hDlg, IDC_CHECK_SSE, BST_CHECKED);
		
		if (Compiler.bAlignVector == TRUE)
			CheckDlgButton(hDlg, IDC_COMPILER_ALIGNVEC, BST_CHECKED);

		if (Compiler.bSections == TRUE) 
			CheckDlgButton(hDlg, IDC_COMPILER_SECTIONS, BST_CHECKED);
		if (Compiler.bGPRConstants == TRUE)
			CheckDlgButton(hDlg, IDC_COMPILER_GPRCONSTANTS, BST_CHECKED);
		if (Compiler.bReOrdering == TRUE)
			CheckDlgButton(hDlg, IDC_COMPILER_REORDER, BST_CHECKED);
		if (Compiler.bFlags == TRUE)
			CheckDlgButton(hDlg, IDC_COMPILER_FLAGS, BST_CHECKED);
		if (Compiler.bAccum == TRUE)
			CheckDlgButton(hDlg, IDC_COMPILER_ACCUM, BST_CHECKED);

		SetTimer(hDlg, 1, 250, NULL);
		break;
	case WM_TIMER:
		sprintf(Buffer, "x86: %2.2f KB / %2.2f KB", (float)(RecompPos - RecompCode) / 1024.0F,
			pLastSecondary?(float)((pLastSecondary - RecompCodeSecondary) / 1024.0F):0);

		SetDlgItemText(hDlg, IDC_COMPILER_BUFFERS, Buffer);
		break;
	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:
			Compiler.bDest = GetBooleanCheck(hDlg, IDC_COMPILER_DEST);
			Compiler.bAccum = GetBooleanCheck(hDlg, IDC_COMPILER_ACCUM);
			Compiler.mmx = GetBooleanCheck(hDlg, IDC_CHECK_MMX);
			Compiler.mmx2 = GetBooleanCheck(hDlg, IDC_CHECK_MMX2);
			Compiler.sse = GetBooleanCheck(hDlg, IDC_CHECK_SSE);
			Compiler.bSections = GetBooleanCheck(hDlg, IDC_COMPILER_SECTIONS);
			Compiler.bReOrdering = GetBooleanCheck(hDlg, IDC_COMPILER_REORDER);
			Compiler.bGPRConstants = GetBooleanCheck(hDlg, IDC_COMPILER_GPRCONSTANTS);
			Compiler.bFlags = GetBooleanCheck(hDlg, IDC_COMPILER_FLAGS);
			Compiler.bAlignVector = GetBooleanCheck(hDlg, IDC_COMPILER_ALIGNVEC);
			SetSetting(Set_CheckDest,Compiler.bDest);
			SetSetting(Set_Accum,Compiler.bAccum);
			SetSetting(Set_Mmx,Compiler.mmx);
			SetSetting(Set_Mmx2,Compiler.mmx2);
			SetSetting(Set_Sse,Compiler.sse);
			SetSetting(Set_Sections,Compiler.bSections);
			SetSetting(Set_ReOrdering,Compiler.bReOrdering);
			SetSetting(Set_GPRConstants,Compiler.bGPRConstants);
			SetSetting(Set_Flags,Compiler.bFlags);
			SetSetting(Set_AlignVector,Compiler.bAlignVector);

			KillTimer(hDlg, 1);
			EndDialog(hDlg, TRUE);
			break;
		case IDCANCEL:	
			KillTimer(hDlg, 1);
			EndDialog(hDlg, TRUE);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK ConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
	HWND hWndItem;
	DWORD value;

	switch (uMsg)
	{
	case WM_INITDIALOG:	
		if (AudioHle == TRUE)
		{
			CheckDlgButton(hDlg, IDC_AUDIOHLE, BST_CHECKED);
		}
		if (GraphicsHle == TRUE)
		{
			CheckDlgButton(hDlg, IDC_GRAPHICSHLE, BST_CHECKED);
		}

		hWndItem = GetDlgItem(hDlg, IDC_COMPILER_SELECT);
		ComboBox_AddString(hWndItem, "Interpreter");
		ComboBox_AddString(hWndItem, "Recompiler");
		ComboBox_SetCurSel(hWndItem, CPUCore);
		break;
	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:
			hWndItem = GetDlgItem(hDlg, IDC_COMPILER_SELECT);
			value = ComboBox_GetCurSel(hWndItem);
			SetCPU(value);

			AudioHle = GetBooleanCheck(hDlg, IDC_AUDIOHLE);
			GraphicsHle = GetBooleanCheck(hDlg, IDC_GRAPHICSHLE);

			EndDialog(hDlg, TRUE);
			break;
		case IDCANCEL:	
			EndDialog(hDlg, TRUE);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
#endif

/*EXPORT void DllConfig(HWND hWnd)
{
	// DialogBox(hinstDLL, "RSPCONFIG", hWnd, ConfigDlgProc);
	DialogBox(hinstDLL, "RSPCONFIG", GetForegroundWindow(), ConfigDlgProc);
}*/

EXPORT void EnableDebugging(Boolean Enabled)
{
	DebuggingEnabled = Enabled;
	if (DebuggingEnabled)
	{
		BreakOnStart   = GetSetting(Set_BreakOnStart);
		CPUCore        = GetSetting(Set_CPUCore);
		LogRDP         = GetSetting(Set_LogRDP);
		LogX86Code     = GetSetting(Set_LogX86Code);
		Profiling      = GetSetting(Set_Profiling);
		IndvidualBlock = GetSetting(Set_IndvidualBlock);
		ShowErrors     = GetSetting(Set_ShowErrors);
	
		Compiler.bDest         = GetSetting(Set_CheckDest);
		Compiler.bAccum        = GetSetting(Set_Accum);
		Compiler.mmx           = GetSetting(Set_Mmx);
		Compiler.mmx2          = GetSetting(Set_Mmx2);
		Compiler.sse           = GetSetting(Set_Sse);
		Compiler.bSections     = GetSetting(Set_Sections);
		Compiler.bReOrdering   = GetSetting(Set_ReOrdering);
		Compiler.bGPRConstants = GetSetting(Set_GPRConstants);
		Compiler.bFlags        = GetSetting(Set_Flags);
		Compiler.bAlignVector  = GetSetting(Set_AlignVector);
		SetCPU(CPUCore);
	}
#ifdef _WIN32
    FixMenuState();
#else
    fputs("FixMenuState()\n", stderr);
#endif
	if (LogRDP)
	{
		StartRDPLog();
	}
	if (LogX86Code)
	{
		StartCPULog();
	}
}

EXPORT void PluginLoaded(void)
{
	BreakOnStart   = false;
	CPUCore        = RecompilerCPU;
	LogRDP         = FALSE;
	LogX86Code     = FALSE;
	Profiling      = FALSE;
	IndvidualBlock = FALSE;
	ShowErrors     = FALSE;

	memset(&Compiler, 0, sizeof(Compiler));
	
	Compiler.bDest         = TRUE;
	Compiler.bAlignVector  = FALSE;
	Compiler.bFlags        = TRUE;
	Compiler.bReOrdering   = TRUE;
	Compiler.bSections     = TRUE;
	Compiler.bAccum        = TRUE;
	Compiler.bGPRConstants = TRUE;
	DetectCpuSpecs();


	SetModuleName("RSP");
	Set_GraphicsHle = FindSystemSettingId("HLE GFX");
	Set_AudioHle = FindSystemSettingId("HLE Audio");
	
	RegisterSetting(Set_BreakOnStart,   Data_DWORD_General,"Break on Start", NULL,BreakOnStart,NULL);
	RegisterSetting(Set_CPUCore,        Data_DWORD_General,"CPU Method",     NULL,CPUCore,NULL);
	RegisterSetting(Set_LogRDP,         Data_DWORD_General,"Log RDP",        NULL,LogRDP,NULL);
	RegisterSetting(Set_LogX86Code,     Data_DWORD_General,"Log X86 Code",   NULL,LogX86Code,NULL);
	RegisterSetting(Set_Profiling,      Data_DWORD_General,"Profiling",      NULL,Profiling,NULL);
	RegisterSetting(Set_IndvidualBlock, Data_DWORD_General,"Indvidual Block",NULL,IndvidualBlock,NULL);
	RegisterSetting(Set_ShowErrors,     Data_DWORD_General,"Show Errors",    NULL,ShowErrors,NULL);

	//Compiler settings
	RegisterSetting(Set_CheckDest,      Data_DWORD_General,"Check Dest Vector", NULL,Compiler.bDest,NULL);
	RegisterSetting(Set_Accum,          Data_DWORD_General,"Check Dest Accum", NULL,Compiler.bAccum,NULL);
	RegisterSetting(Set_Mmx,            Data_DWORD_General,"Use MMX", NULL,Compiler.mmx,NULL);
	RegisterSetting(Set_Mmx2,           Data_DWORD_General,"Use MMX2", NULL,Compiler.mmx2,NULL);
	RegisterSetting(Set_Sse,            Data_DWORD_General,"Use SSE", NULL,Compiler.sse,NULL);
	RegisterSetting(Set_Sections,       Data_DWORD_General,"Use precompiled sections", NULL,Compiler.bSections,NULL);
	RegisterSetting(Set_ReOrdering,     Data_DWORD_General,"Reorder opcodes", NULL,Compiler.bReOrdering,NULL);
	RegisterSetting(Set_GPRConstants,   Data_DWORD_General,"Detect GPR Constants", NULL,Compiler.bGPRConstants,NULL);
	RegisterSetting(Set_Flags,          Data_DWORD_General,"Check Flag Usage", NULL,Compiler.bFlags,NULL);
	RegisterSetting(Set_AlignVector,    Data_DWORD_General,"Assume Vector loads align", NULL,Compiler.bAlignVector,NULL);

	RegisterSetting(Set_JumpTableSize,  Data_DWORD_Game,"JumpTableSize",NULL,0x800,NULL);
	RegisterSetting(Set_Mfc0Count, Data_DWORD_Game, "Mfc0Count", NULL, 0x0, NULL);
	RegisterSetting(Set_SemaphoreExit, Data_DWORD_Game, "SemaphoreExit", NULL, 0x0, NULL);

	AudioHle       = Set_AudioHle != 0 ? GetSystemSetting(Set_AudioHle) : false;
	GraphicsHle    = Set_GraphicsHle != 0 ? GetSystemSetting(Set_GraphicsHle) : true;
#ifdef _WIN32
    hMutex = (HANDLE)CreateMutex(NULL, FALSE, NULL);
#endif
	SetCPU(CPUCore);
}

#ifdef _WIN32
void UseUnregisteredSetting(int /*SettingID*/)
{
    DebugBreak();
}
#endif
