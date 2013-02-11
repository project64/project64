/*
 * RSP Compiler plug in for Project 64 (A Nintendo 64 emulator).
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

#if defined(__cplusplus)
extern "C" {
#endif

/************ Profiling **************/
#define Default_ProfilingOn			FALSE
#define Default_IndvidualBlock		FALSE
#define Default_ShowErrors			FALSE
#define Default_AudioHle			FALSE

/* Note: BOOL, BYTE, WORD, DWORD, TRUE, FALSE are defined in windows.h */

#define PLUGIN_TYPE_RSP				1
#define PLUGIN_TYPE_GFX				2
#define PLUGIN_TYPE_AUDIO			3
#define PLUGIN_TYPE_CONTROLLER		4

typedef struct {
	WORD Version;        /* Should be set to 0x0101 */
	WORD Type;           /* Set to PLUGIN_TYPE_RSP */
	char Name[100];      /* Name of the DLL */

	/* If DLL supports memory these memory options then set them to TRUE or FALSE
	   if it does not support it */
	BOOL NormalMemory;   /* a normal BYTE array */ 
	BOOL MemoryBswaped;  /* a normal BYTE array where the memory has been pre
	                          bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;

typedef struct {
	HINSTANCE hInst;
	BOOL MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
	                          bswap on a dword (32 bits) boundry */
	BYTE * RDRAM;
	BYTE * DMEM;
	BYTE * IMEM;

	DWORD * MI_INTR_REG;

	DWORD * SP_MEM_ADDR_REG;
	DWORD * SP_DRAM_ADDR_REG;
	DWORD * SP_RD_LEN_REG;
	DWORD * SP_WR_LEN_REG;
	DWORD * SP_STATUS_REG;
	DWORD * SP_DMA_FULL_REG;
	DWORD * SP_DMA_BUSY_REG;
	DWORD * SP_PC_REG;
	DWORD * SP_SEMAPHORE_REG;

	DWORD * DPC_START_REG;
	DWORD * DPC_END_REG;
	DWORD * DPC_CURRENT_REG;
	DWORD * DPC_STATUS_REG;
	DWORD * DPC_CLOCK_REG;
	DWORD * DPC_BUFBUSY_REG;
	DWORD * DPC_PIPEBUSY_REG;
	DWORD * DPC_TMEM_REG;

	void (*CheckInterrupts)( void );
	void (*ProcessDList)( void );
	void (*ProcessAList)( void );
	void (*ProcessRdpList)( void );
	void (*ShowCFB)( void );
} RSP_INFO;

typedef struct {
	/* Menu */
	/* Items should have an ID between 5001 and 5100 */
	HMENU hRSPMenu;
	void (*ProcessMenuItem) ( int ID );

	/* Break Points */
	BOOL UseBPoints;
	char BPPanelName[20];
	void (*Add_BPoint)      ( void );
	void (*CreateBPPanel)   ( HWND hDlg, RECT rcBox );
	void (*HideBPPanel)     ( void );
	void (*PaintBPPanel)    ( PAINTSTRUCT ps );
	void (*ShowBPPanel)     ( void );
	void (*RefreshBpoints)  ( HWND hList );
	void (*RemoveBpoint)    ( HWND hList, int index );
	void (*RemoveAllBpoint) ( void );
	
	/* RSP command Window */
	void (*Enter_RSP_Commands_Window) ( void );
} RSPDEBUG_INFO;

typedef struct {
	void (*UpdateBreakPoints)( void );
	void (*UpdateMemory)( void );
	void (*UpdateR4300iRegisters)( void );
	void (*Enter_BPoint_Window)( void );
	void (*Enter_R4300i_Commands_Window)( void );
	void (*Enter_R4300i_Register_Window)( void );
	void (*Enter_RSP_Commands_Window) ( void );
	void (*Enter_Memory_Window)( void );
} DEBUG_INFO;

__declspec(dllexport) void CloseDLL (void);
__declspec(dllexport) void DllAbout ( HWND hParent );
__declspec(dllexport) DWORD DoRspCycles ( DWORD Cycles );
__declspec(dllexport) void GetDllInfo ( PLUGIN_INFO * PluginInfo );
__declspec(dllexport) void GetRspDebugInfo ( RSPDEBUG_INFO * DebugInfo );
__declspec(dllexport) void InitiateRSP ( RSP_INFO Rsp_Info, DWORD * CycleCount);
__declspec(dllexport) void InitiateRSPDebugger ( DEBUG_INFO Debug_Info);
__declspec(dllexport) void RomOpen (void);
__declspec(dllexport) void RomClosed (void);
__declspec(dllexport) void DllConfig (HWND hWnd);
__declspec(dllexport) void EnableDebugging (BOOL Enabled);
__declspec(dllexport) void PluginLoaded (void);

DWORD AsciiToHex (char * HexValue);
void DisplayError (char * Message, ...);
int  GetStoredWinPos( char * WinName, DWORD * X, DWORD * Y );

#define InterpreterCPU	0
#define RecompilerCPU	1

extern BOOL DebuggingEnabled, Profiling, IndvidualBlock, ShowErrors, BreakOnStart, LogRDP, LogX86Code;
extern DWORD CPUCore;
extern DEBUG_INFO DebugInfo;
extern RSP_INFO RSPInfo;
extern HINSTANCE hinstDLL;

#if defined(__cplusplus)
}
#endif
