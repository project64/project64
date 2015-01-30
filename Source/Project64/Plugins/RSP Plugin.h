/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

class CRSP_Plugin : public CPlugin
{
	typedef struct {
		/* Menu */
		/* Items should have an ID between 5001 and 5100 */
		HMENU hRSPMenu;
		void (__cdecl *ProcessMenuItem) ( int ID );

		/* Break Points */
		BOOL UseBPoints;
		char BPPanelName[20];
		void (__cdecl *Add_BPoint)      ( void );
		void (__cdecl *CreateBPPanel)   ( HMENU hDlg, RECT_STRUCT rcBox );
		void (__cdecl *HideBPPanel)     ( void );
		void (__cdecl *PaintBPPanel)    ( WINDOWS_PAINTSTRUCT ps );
		void (__cdecl *ShowBPPanel)     ( void );
		void (__cdecl *RefreshBpoints)  ( HMENU hList );
		void (__cdecl *RemoveBpoint)    ( HMENU hList, int index );
		void (__cdecl *RemoveAllBpoint) ( void );
		
		/* RSP command Window */
		void (__cdecl *Enter_RSP_Commands_Window) ( void );
	} RSPDEBUG_INFO;

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

public:
	CRSP_Plugin(const char * FileName);
	~CRSP_Plugin();

	virtual int GetDefaultSettingStartRange() const { return FirstRSPDefaultSet; }
	virtual int GetSettingStartRange() const { return FirstRSPSettings; }

	bool Initiate(CPlugins * Plugins, CN64System * System);

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

		void(__cdecl *CheckInterrupts)(void);
		void(__cdecl *ProcessDlist)(void);
		void(__cdecl *ProcessAlist)(void);
		void(__cdecl *ProcessRdpList)(void);
		void(__cdecl *ShowCFB)(void);
	} RSP_INFO_1_1;

	void(__cdecl *InitiateRSP)	(RSP_INFO_1_1 Audio_Info, DWORD * Cycles);
	DWORD(__cdecl *DoRspCycles)	(DWORD);
	void(__cdecl *EnableDebugging)(BOOL Enable);

	HMENU GetDebugMenu (void ) { return m_RSPDebug.hRSPMenu; }
	void ProcessMenuItem(int id);

	void UnloadPlugin(); 

protected:
	CRSP_Plugin(void);							// Disable default constructor
	CRSP_Plugin(const CRSP_Plugin&);			// Disable copy constructor
	CRSP_Plugin& operator=(const CRSP_Plugin&);	// Disable assignment

	bool Init(const char * FileName);

	RSPDEBUG_INFO m_RSPDebug;
	DWORD         m_CycleCount;

	void(__cdecl *GetDebugInfo)     (RSPDEBUG_INFO * GFXDebugInfo);
	void(__cdecl *InitiateDebugger) (DEBUG_INFO DebugInfo);
};
