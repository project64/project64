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

class CGfxPlugin : public CPlugin
{
	typedef struct {
		/* Menu */
		/* Items should have an ID between 5101 and 5200 */
		HMENU hGFXMenu;
		void(__cdecl *ProcessMenuItem) (int ID);

		/* Break Points */
		BOOL UseBPoints;
		char BPPanelName[20];
		void(__cdecl *Add_BPoint)      (void);
		void(__cdecl *CreateBPPanel)   (HWND hDlg, RECT_STRUCT rcBox);
		void(__cdecl *HideBPPanel)     (void);
		void(__cdecl *PaintBPPanel)    (WINDOWS_PAINTSTRUCT ps);
		void(__cdecl *ShowBPPanel)     (void);
		void(__cdecl *RefreshBpoints)  (HWND hList);
		void(__cdecl *RemoveBpoint)    (HWND hList, int index);
		void(__cdecl *RemoveAllBpoint) (void);

		/* GFX command Window */
		void(__cdecl *Enter_GFX_Commands_Window) (void);
	} GFXDEBUG_INFO;

	typedef struct {
		void(__cdecl *UpdateBreakPoints)(void);
		void(__cdecl *UpdateMemory)(void);
		void(__cdecl *UpdateR4300iRegisters)(void);
		void(__cdecl *Enter_BPoint_Window)(void);
		void(__cdecl *Enter_R4300i_Commands_Window)(void);
		void(__cdecl *Enter_R4300i_Register_Window)(void);
		void(__cdecl *Enter_RSP_Commands_Window) (void);
		void(__cdecl *Enter_Memory_Window)(void);
	} DEBUG_INFO;

public:
	CGfxPlugin(const char * FileName);
	~CGfxPlugin();

	bool Initiate(CN64System * System, CMainGui * RenderWindow);

	void(__cdecl *CaptureScreen)      (const char *);
	void(__cdecl *ChangeWindow)       (void);
	void(__cdecl *DrawScreen)         (void);
	void(__cdecl *DrawStatus)         (const char * lpString, BOOL RightAlign);
	void(__cdecl *MoveScreen)         (int xpos, int ypos);
	void(__cdecl *ProcessDList)       (void);
	void(__cdecl *ProcessRDPList)     (void);
	void(__cdecl *ShowCFB)			   (void);
	void(__cdecl *UpdateScreen)       (void);
	void(__cdecl *ViStatusChanged)    (void);
	void(__cdecl *ViWidthChanged)     (void);
	void(__cdecl *SoftReset)          (void);

	//Rom Browser
	HMENU(__cdecl * GetRomBrowserMenu)  (void); /* Items should have an ID between 4101 and 4200 */
	void(__cdecl * OnRomBrowserMenuItem) (int MenuID, HWND hParent, BYTE * HEADER);

	HMENU GetDebugMenu(void) { return m_GFXDebug.hGFXMenu; }
	void ProcessMenuItem(int id);

	void UnloadPlugin();

private:
	CGfxPlugin(void);							// Disable default constructor
	CGfxPlugin(const CGfxPlugin&);				// Disable copy constructor
	CGfxPlugin& operator=(const CGfxPlugin&);	// Disable assignment

	virtual int GetDefaultSettingStartRange() const { return FirstGfxDefaultSet; }
	virtual int GetSettingStartRange() const { return FirstGfxSettings; }

	bool Init(const char * FileName);

	GFXDEBUG_INFO m_GFXDebug;


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

		void(__cdecl *CheckInterrupts)(void);
	} GFX_INFO;

	BOOL(__cdecl *InitiateGFX)		(GFX_INFO Gfx_Info);
	void(__cdecl *GetDebugInfo)	(GFXDEBUG_INFO * GFXDebugInfo);
	void(__cdecl *InitiateDebugger)(DEBUG_INFO DebugInfo);

	static void __cdecl DummyDrawScreen(void) {}
	static void __cdecl DummyMoveScreen(int /*xpos*/, int /*ypos*/) {}
	static void __cdecl DummyViStatusChanged(void) {}
	static void __cdecl DummyViWidthChanged(void) {}
	static void __cdecl DummySoftReset(void) {}
};