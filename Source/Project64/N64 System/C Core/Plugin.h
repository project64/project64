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
#ifndef __PLUGIN_C_H__
#define __PLUGIN_C_H__

#include <Windows.h>


 /*
#define DefaultGFXDll				"Jabo_Direct3D8.dll"
#define DefaultRSPDll				"RSP.dll"
#define DefaultAudioDll				"Jabo_Dsound.dll"
#define DefaultControllerDll		"Jabo_DInput.dll"

#define PLUGIN_TYPE_RSP				1
#define PLUGIN_TYPE_GFX				2
#define PLUGIN_TYPE_AUDIO			3
#define PLUGIN_TYPE_CONTROLLER		4
*/

//#define SYSTEM_NTSC					0
//#define SYSTEM_PAL					1
//#define SYSTEM_MPAL					2

#ifndef PLUGIN_INFO_STRUCT
#define PLUGIN_INFO_STRUCT

typedef struct {
	WORD Version;        /* Should be set to 1 */
	WORD Type;           /* Set to PLUGIN_TYPE_GFX */
	char Name[100];      /* Name of the DLL */

	/* If DLL supports memory these memory options then set them to TRUE or FALSE
	   if it does not support it */
	BOOL NormalMemory;   /* a normal BYTE array */ 
	BOOL MemoryBswaped;  /* a normal BYTE array where the memory has been pre
	                          bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;

#endif 

typedef struct {
	HWND hWnd;			/* Render window */
	HWND hStatusBar;    /* if render window does not have a status bar then this is NULL */

	BOOL MemoryBswaped;    // If this is set to TRUE, then the memory has been pre
	                       //   bswap on a dword (32 bits) boundry 
						   //	eg. the first 8 bytes are stored like this:
	                       //        4 3 2 1   8 7 6 5

	BYTE * HEADER;	// This is the rom header (first 40h bytes of the rom
					// This will be in the same memory format as the rest of the memory.
	BYTE * RDRAM;
	BYTE * DMEM;
	BYTE * IMEM;

	DWORD * MI__INTR_REG;

	DWORD * DPC__START_REG;
	DWORD * DPC__END_REG;
	DWORD * DPC__CURRENT_REG;
	DWORD * DPC__STATUS_REG;
	DWORD * DPC__CLOCK_REG;
	DWORD * DPC__BUFBUSY_REG;
	DWORD * DPC__PIPEBUSY_REG;
	DWORD * DPC__TMEM_REG;

	DWORD * VI__STATUS_REG;
	DWORD * VI__ORIGIN_REG;
	DWORD * VI__WIDTH_REG;
	DWORD * VI__INTR_REG;
	DWORD * VI__V_CURRENT_LINE_REG;
	DWORD * VI__TIMING_REG;
	DWORD * VI__V_SYNC_REG;
	DWORD * VI__H_SYNC_REG;
	DWORD * VI__LEAP_REG;
	DWORD * VI__H_START_REG;
	DWORD * VI__V_START_REG;
	DWORD * VI__V_BURST_REG;
	DWORD * VI__X_SCALE_REG;
	DWORD * VI__Y_SCALE_REG;

	void (__cdecl *CheckInterrupts)( void );
} GFX_INFO;

typedef struct {
	HINSTANCE hInst;
	BOOL MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
	                          bswap on a dword (32 bits) boundry */
	BYTE * RDRAM;
	BYTE * DMEM;
	BYTE * IMEM;

	DWORD * MI__INTR_REG;

	DWORD * SP__MEM_ADDR_REG;
	DWORD * SP__DRAM_ADDR_REG;
	DWORD * SP__RD_LEN_REG;
	DWORD * SP__WR_LEN_REG;
	DWORD * SP__STATUS_REG;
	DWORD * SP__DMA_FULL_REG;
	DWORD * SP__DMA_BUSY_REG;
	DWORD * SP__PC_REG;
	DWORD * SP__SEMAPHORE_REG;

	DWORD * DPC__START_REG;
	DWORD * DPC__END_REG;
	DWORD * DPC__CURRENT_REG;
	DWORD * DPC__STATUS_REG;
	DWORD * DPC__CLOCK_REG;
	DWORD * DPC__BUFBUSY_REG;
	DWORD * DPC__PIPEBUSY_REG;
	DWORD * DPC__TMEM_REG;

	void ( __cdecl *CheckInterrupts)( void );
	void (__cdecl *ProcessDlist)( void );
	void (__cdecl *ProcessAlist)( void );
	void (__cdecl *ProcessRdpList)( void );
} RSP_INFO_1_0;

#ifndef CONTROL_STRUCTS
#define CONTROL_STRUCTS

typedef union {
	DWORD Value;
	struct {
		unsigned R_DPAD       : 1;
		unsigned L_DPAD       : 1;
		unsigned D_DPAD       : 1;
		unsigned U_DPAD       : 1;
		unsigned START_BUTTON : 1;
		unsigned Z_TRIG       : 1;
		unsigned B_BUTTON     : 1;
		unsigned A_BUTTON     : 1;

		unsigned R_CBUTTON    : 1;
		unsigned L_CBUTTON    : 1;
		unsigned D_CBUTTON    : 1;
		unsigned U_CBUTTON    : 1;
		unsigned R_TRIG       : 1;
		unsigned L_TRIG       : 1;
		unsigned Reserved1    : 1;
		unsigned Reserved2    : 1;

		signed   Y_AXIS       : 8;

		signed   X_AXIS       : 8;
	};
} BUTTONS;

typedef struct {
	BOOL Present;
	BOOL RawData;
	int  Plugin;
} CONTROL;

#endif

typedef struct {
	HWND hMainWindow;
	HINSTANCE hinst;

	BOOL MemoryBswaped;		// If this is set to TRUE, then the memory has been pre
							//   bswap on a dword (32 bits) boundry, only effects header. 
							//	eg. the first 8 bytes are stored like this:
							//        4 3 2 1   8 7 6 5
	BYTE * HEADER;			// This is the rom header (first 40h bytes of the rom)
	CONTROL *Controls;		// A pointer to an array of 4 controllers .. eg:
							// CONTROL Controls[4];
} CONTROL_INFO;

typedef struct {
	HINSTANCE hInst;
	BOOL MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
	                          bswap on a dword (32 bits) boundry */
	BYTE * RDRAM;
	BYTE * DMEM;
	BYTE * IMEM;

	DWORD * MI__INTR_REG;

	DWORD * SP__MEM_ADDR_REG;
	DWORD * SP__DRAM_ADDR_REG;
	DWORD * SP__RD_LEN_REG;
	DWORD * SP__WR_LEN_REG;
	DWORD * SP__STATUS_REG;
	DWORD * SP__DMA_FULL_REG;
	DWORD * SP__DMA_BUSY_REG;
	DWORD * SP__PC_REG;
	DWORD * SP__SEMAPHORE_REG;

	DWORD * DPC__START_REG;
	DWORD * DPC__END_REG;
	DWORD * DPC__CURRENT_REG;
	DWORD * DPC__STATUS_REG;
	DWORD * DPC__CLOCK_REG;
	DWORD * DPC__BUFBUSY_REG;
	DWORD * DPC__PIPEBUSY_REG;
	DWORD * DPC__TMEM_REG;

	void ( __cdecl *CheckInterrupts)( void );
	void (__cdecl *ProcessDlist)( void );
	void (__cdecl *ProcessAlist)( void );
	void (__cdecl *ProcessRdpList)( void );
	void (__cdecl *ShowCFB)( void );
} RSP_INFO_1_1;

typedef struct {
	/* Menu */
	/* Items should have an ID between 5001 and 5100 */
	HMENU hRSPMenu;
	void (__cdecl *ProcessMenuItem) ( int ID );

	/* Break Points */
	BOOL UseBPoints;
	char BPPanelName[20];
	void (__cdecl *Add_BPoint)      ( void );
	void (__cdecl *CreateBPPanel)   ( HWND hDlg, RECT rcBox );
	void (__cdecl *HideBPPanel)     ( void );
	void (__cdecl *PaintBPPanel)    ( PAINTSTRUCT ps );
	void (__cdecl *ShowBPPanel)     ( void );
	void (__cdecl *RefreshBpoints)  ( HWND hList );
	void (__cdecl *RemoveBpoint)    ( HWND hList, int index );
	void (__cdecl *RemoveAllBpoint) ( void );
	
	/* RSP command Window */
	void (__cdecl *Enter_RSP_Commands_Window) ( void );
} RSPDEBUG_INFO;

typedef struct {
	/* Menu */
	/* Items should have an ID between 5101 and 5200 */
	HMENU hGFXMenu;
	void (__cdecl *ProcessMenuItem) ( int ID );

	/* Break Points */
	BOOL UseBPoints;
	char BPPanelName[20];
	void (__cdecl *Add_BPoint)      ( void );
	void (__cdecl *CreateBPPanel)   ( HWND hDlg, RECT rcBox );
	void (__cdecl *HideBPPanel)     ( void );
	void (__cdecl *PaintBPPanel)    ( PAINTSTRUCT ps );
	void (__cdecl *ShowBPPanel)     ( void );
	void (__cdecl *RefreshBpoints)  ( HWND hList );
	void (__cdecl *RemoveBpoint)    ( HWND hList, int index );
	void (__cdecl *RemoveAllBpoint) ( void );
	
	/* GFX command Window */
	void (__cdecl *Enter_GFX_Commands_Window) ( void );
} GFXDEBUG_INFO;

typedef struct {
	void (__cdecl *UpdateBreakPoints)( void );
	void (__cdecl *UpdateMemory)( void );
	void (__cdecl *UpdateR4300iRegisters)( void );
	void (__cdecl *Enter_BPoint_Window)( void );
	void (__cdecl *Enter_R4300i_Commands_Window)( void );
	void (__cdecl *Enter_R4300i_Register_Window)( void );
	void (__cdecl *Enter_RSP_Commands_Window) ( void );
	void (__cdecl *Enter_Memory_Window)( void );
} DEBUG_INFO;

typedef struct {
	HWND hwnd;
	HINSTANCE hinst;

	BOOL MemoryBswaped;    // If this is set to TRUE, then the memory has been pre
	                       //   bswap on a dword (32 bits) boundry 
						   //	eg. the first 8 bytes are stored like this:
	                       //        4 3 2 1   8 7 6 5
	BYTE * HEADER;	// This is the rom header (first 40h bytes of the rom
					// This will be in the same memory format as the rest of the memory.
	BYTE * RDRAM;
	BYTE * DMEM;
	BYTE * IMEM;

	DWORD * MI__INTR_REG;

	DWORD * AI__DRAM_ADDR_REG;
	DWORD * AI__LEN_REG;
	DWORD * AI__CONTROL_REG;
	DWORD * AI__STATUS_REG;
	DWORD * AI__DACRATE_REG;
	DWORD * AI__BITRATE_REG;

	void (__cdecl *CheckInterrupts)( void );
} AUDIO_INFO;

/*** Conteroller plugin's ****/
#ifndef __cplusplus
#define PLUGIN_NONE					1
#define PLUGIN_MEMPAK				2
#define PLUGIN_RUMBLE_PAK			3 
#define PLUGIN_TANSFER_PAK			4 // not implemeted for non raw data
#define PLUGIN_RAW					5 // the controller plugin is passed in raw data
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******** All DLLs have this function **************/
extern void (__cdecl *GetDllInfo)             ( PLUGIN_INFO * PluginInfo );

/********** RSP DLL: Functions *********************/
//void (__cdecl *GetRspDebugInfo)    ( RSPDEBUG_INFO * DebugInfo );
//extern void (__cdecl *RSPCloseDLL)        ( void );
//extern void (__cdecl *RSPDllAbout)        ( HWND hWnd );
//extern void (__cdecl *RSPDllConfig)       ( HWND hWnd );
//extern void (__cdecl *RSPRomClosed)       ( void );
extern DWORD (__cdecl *DoRspCycles)       ( DWORD );
//extern void (__cdecl *InitiateRSP_1_0)    ( RSP_INFO_1_0 Rsp_Info, DWORD * Cycles);
//extern void (__cdecl *InitiateRSP_1_1)    ( RSP_INFO_1_1 Rsp_Info, DWORD * Cycles);
//extern void (__cdecl *InitiateRSPDebugger)( DEBUG_INFO DebugInfo);

/********** GFX DLL: Functions *********************/
extern void (__cdecl *CaptureScreen)      ( const char * );
extern void (__cdecl *ChangeWindow)       ( void );
//extern void (__cdecl *GetGfxDebugInfo)    ( GFXDEBUG_INFO * GFXDebugInfo );
//extern void (__cdecl *GFXCloseDLL)        ( void );
//extern void (__cdecl *GFXDllAbout)        ( HWND hParent );
//extern void (__cdecl *GFXDllConfig)       ( HWND hParent );
//extern void (__cdecl *GfxRomClosed)       ( void );
//extern void (__cdecl *GfxRomOpen)         ( void );
extern void (__cdecl *DrawScreen)         ( void );
//extern void (__cdecl *FrameBufferRead)    ( DWORD addr );
//extern void (__cdecl *FrameBufferWrite)   ( DWORD addr, DWORD Bytes );
//BOOL (__cdecl *InitiateGFX)        ( GFX_INFO Gfx_Info );
//extern void (__cdecl *InitiateGFXDebugger)( DEBUG_INFO DebugInfo);
extern void (__cdecl *MoveScreen)         ( int xpos, int ypos );
extern void (__cdecl *ProcessDList)       ( void );
extern void (__cdecl *ProcessRDPList)     ( void );
extern void (__cdecl *ShowCFB)			   ( void );
extern void (__cdecl *UpdateScreen)       ( void );
extern void (__cdecl *ViStatusChanged)    ( void );
extern void (__cdecl *ViWidthChanged)     ( void );

/************ Audio DLL: Functions *****************/
//extern void (__cdecl *AiCloseDLL)       ( void );
//extern void (__cdecl *AiDacrateChanged) ( int SystemType );
extern void (__cdecl *AiLenChanged)     ( void );
//extern void (__cdecl *AiDllAbout)       ( HWND hParent );
//extern void (__cdecl *AiDllConfig)      ( HWND hParent );
//extern void (__cdecl *AiDllTest)        ( HWND hParent );
extern DWORD (__cdecl *AiReadLength)    ( void );
//extern void (__cdecl *AiRomClosed)      ( void );
//extern void (__cdecl *AiUpdate)         ( BOOL Wait );
//extern BOOL (__cdecl *InitiateAudio)    ( AUDIO_INFO Audio_Info );
extern void (__cdecl *ProcessAList)     ( void );

/********** Controller DLL: Functions **************/
//extern void (__cdecl *ContCloseDLL)     ( void );
extern void (__cdecl *ControllerCommand)( int Control, BYTE * Command );
//extern void (__cdecl *ContDllAbout)     ( HWND hParent );
//extern void (__cdecl *ContConfig)       ( HWND hParent );
//extern void (__cdecl *InitiateControllers_1_0)( HWND hMainWindow, CONTROL Controls[4] );
//extern void (__cdecl *InitiateControllers_1_1)( CONTROL_INFO ControlInfo );
extern void (__cdecl *GetKeys)          ( int Control, BUTTONS * Keys );
extern void (__cdecl *ReadController)   ( int Control, BYTE * Command );
//extern void (__cdecl *ContRomOpen)      ( void );
//extern void (__cdecl *ContRomClosed)    ( void );
//extern void (__cdecl *WM_KeyDown)       ( WPARAM wParam, LPARAM lParam );
//extern void (__cdecl *WM_KeyUp)         ( WPARAM wParam, LPARAM lParam );
extern void (__cdecl *RumbleCommand)	 ( int Control, BOOL bRumble );

#ifdef __cplusplus
}
#endif

/********** Plugin: Functions *********************/
void GetPluginDir        ( char * Directory );
void GetSnapShotDir      ( char * Directory );
void PluginConfiguration ( HWND hWnd );
void SetupPlugins        ( HWND hWnd );
void SetupPluginScreen   ( HWND hDlg );
void ShutdownPlugins     ( void );

/********** External Global Variables ***************/
#define MaxDlls	100
extern char RspDLL[100], GfxDLL[100], AudioDLL[100],ControllerDLL[100], * PluginNames[MaxDlls];
extern DWORD PluginCount, RspTaskValue;
extern GFXDEBUG_INFO GFXDebug;
extern RSPDEBUG_INFO RspDebug;
extern WORD RSPVersion;
extern BOOL PluginsInitilized;

#endif