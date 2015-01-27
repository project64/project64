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

typedef union {
	DWORD Value;
	struct {
		unsigned R_DPAD : 1;
		unsigned L_DPAD : 1;
		unsigned D_DPAD : 1;
		unsigned U_DPAD : 1;
		unsigned START_BUTTON : 1;
		unsigned Z_TRIG : 1;
		unsigned B_BUTTON : 1;
		unsigned A_BUTTON : 1;

		unsigned R_CBUTTON : 1;
		unsigned L_CBUTTON : 1;
		unsigned D_CBUTTON : 1;
		unsigned U_CBUTTON : 1;
		unsigned R_TRIG : 1;
		unsigned L_TRIG : 1;
		unsigned Reserved1 : 1;
		unsigned Reserved2 : 1;

		signed   Y_AXIS : 8;

		signed   X_AXIS : 8;
	};
} BUTTONS;

typedef struct {
	DWORD Present;
	DWORD RawData;
	int   Plugin;
} CONTROL;

enum PluginType {
	PLUGIN_NONE = 1,
	PLUGIN_MEMPAK = 2,
	PLUGIN_RUMBLE_PAK = 3,
	PLUGIN_TANSFER_PAK = 4, // not implemeted for non raw data
	PLUGIN_RAW = 5, // the controller plugin is passed in raw data
};

class CCONTROL {
	friend CControl_Plugin; //controller plugin class has full access

	DWORD   & m_Present;
	DWORD   & m_RawData;
	int     & m_PlugType;
	BUTTONS   m_Buttons;

public:
	CCONTROL(DWORD &Present, DWORD &RawData, int &PlugType);
	inline bool  Present(void) const { return m_Present != 0; }
	inline DWORD Buttons(void) const { return m_Buttons.Value; }
	inline PluginType Plugin(void) const { return static_cast<PluginType>(m_PlugType); }
};

class CControl_Plugin : public CPlugin
{
public:
	CControl_Plugin(const char * FileName);
	~CControl_Plugin();

	virtual int GetDefaultSettingStartRange() const { return FirstCtrlDefaultSet; }
	virtual int GetSettingStartRange() const { return FirstCtrlSettings; }

	bool Initiate(CN64System * System, CMainGui * RenderWindow);
	void SetControl(CControl_Plugin const * const Plugin);
	void UpdateKeys(void);

	void UnloadPlugin();

	void(__cdecl *WM_KeyDown)       (DWORD wParam, DWORD lParam);
	void(__cdecl *WM_KeyUp)         (DWORD wParam, DWORD lParam);
	void(__cdecl *RumbleCommand)	 (int Control, BOOL bRumble);
	void(__cdecl *GetKeys)          (int Control, BUTTONS * Keys);
	void(__cdecl *ReadController)   (int Control, BYTE * Command);
	void(__cdecl *ControllerCommand)(int Control, BYTE * Command);

	inline CCONTROL const * Controller(int control) { return m_Controllers[control]; }
	inline CONTROL * PluginControllers(void) { return m_PluginControllers; }

protected:
	CControl_Plugin(void);								// Disable default constructor
	CControl_Plugin(const CControl_Plugin&);			// Disable copy constructor
	CControl_Plugin& operator=(const CControl_Plugin&);	// Disable assignment

	bool Init(const char * FileName);

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

	typedef void(__cdecl *fInitiateControllers_1_0)(HWND hMainWindow, CONTROL Controls[4]);
	typedef void(__cdecl *fInitiateControllers_1_1)(CONTROL_INFO * ControlInfo);

	void * InitiateControllers;
	bool   m_AllocatedControllers;

	// What the different controls are set up as
	CONTROL m_PluginControllers[4];
	CCONTROL * m_Controllers[4];
};