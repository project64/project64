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

#if defined(__cplusplus)
extern "C" {
#endif

#include <Common/stdtypes.h>

#if defined(_WIN32)
#define EXPORT          __declspec(dllexport)
#define CALL            _cdecl
#else
#define EXPORT          __attribute__((visibility("default")))
#define CALL
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
    uint16_t Version;        /* Should be set to 0x0101 */
    uint16_t Type;           /* Set to PLUGIN_TYPE_RSP */
	char Name[100];      /* Name of the DLL */

	/* If DLL supports memory these memory options then set them to TRUE or FALSE
	   if it does not support it */
    int NormalMemory;   /* a normal BYTE array */ 
    int MemoryBswaped;  /* a normal BYTE array where the memory has been pre
                              bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;

typedef struct {
    void * hInst;
    int MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
                              bswap on a dword (32 bits) boundry */
    uint8_t * RDRAM;
    uint8_t * DMEM;
    uint8_t * IMEM;

    uint32_t * MI_INTR_REG;

    uint32_t * SP_MEM_ADDR_REG;
    uint32_t * SP_DRAM_ADDR_REG;
    uint32_t * SP_RD_LEN_REG;
    uint32_t * SP_WR_LEN_REG;
    uint32_t * SP_STATUS_REG;
    uint32_t * SP_DMA_FULL_REG;
    uint32_t * SP_DMA_BUSY_REG;
    uint32_t * SP_PC_REG;
    uint32_t * SP_SEMAPHORE_REG;

    uint32_t * DPC_START_REG;
    uint32_t * DPC_END_REG;
    uint32_t * DPC_CURRENT_REG;
    uint32_t * DPC_STATUS_REG;
    uint32_t * DPC_CLOCK_REG;
    uint32_t * DPC_BUFBUSY_REG;
    uint32_t * DPC_PIPEBUSY_REG;
    uint32_t * DPC_TMEM_REG;

	void (*CheckInterrupts)( void );
	void (*ProcessDList)( void );
	void (*ProcessAList)( void );
	void (*ProcessRdpList)( void );
	void (*ShowCFB)( void );
} RSP_INFO;

typedef struct {
	/* Menu */
	/* Items should have an ID between 5001 and 5100 */
    void * hRSPMenu;
	void (*ProcessMenuItem) ( int ID );

	/* Break Points */
    int UseBPoints;
	char BPPanelName[20];
	void (*Add_BPoint)      ( void );
    void (*CreateBPPanel) (void * hDlg, RECT rcBox);
	void (*HideBPPanel)     ( void );
	void (*PaintBPPanel)    ( PAINTSTRUCT ps );
	void (*ShowBPPanel)     ( void );
    void (*RefreshBpoints)(void * hList);
    void (*RemoveBpoint)  (void * hList, int index);
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

EXPORT void CloseDLL(void);
EXPORT void DllAbout(void * hParent);
EXPORT uint32_t DoRspCycles(uint32_t Cycles);
EXPORT void GetDllInfo(PLUGIN_INFO * PluginInfo);
EXPORT void GetRspDebugInfo(RSPDEBUG_INFO * DebugInfo);
EXPORT void InitiateRSP(RSP_INFO Rsp_Info, uint32_t * CycleCount);
EXPORT void InitiateRSPDebugger(DEBUG_INFO Debug_Info);
EXPORT void RomOpen(void);
EXPORT void RomClosed(void);
EXPORT void DllConfig(void * hWnd);
EXPORT void EnableDebugging(int Enabled);
EXPORT void PluginLoaded(void);

uint32_t AsciiToHex(char * HexValue);
void DisplayError (char * Message, ...);
int GetStoredWinPos(char * WinName, uint32_t * X, uint32_t * Y);

#define InterpreterCPU	0
#define RecompilerCPU	1

extern int DebuggingEnabled, Profiling, IndvidualBlock, ShowErrors, BreakOnStart, LogRDP, LogX86Code;
extern uint32_t CPUCore;
extern DEBUG_INFO DebugInfo;
extern RSP_INFO RSPInfo;
extern void * hinstDLL;

#if defined(__cplusplus)
}
#endif
