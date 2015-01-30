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
#include "stdafx.h"

CControl_Plugin::CControl_Plugin(const char * FileName) : CPlugin(),
	m_AllocatedControllers(false),
	InitiateControllers(NULL),
	WM_KeyDown(NULL),
	WM_KeyUp(NULL),
	RumbleCommand(NULL),
	GetKeys(NULL),
	ReadController(NULL),
	ControllerCommand(NULL)
{
	memset(&m_PluginControllers, 0, sizeof(m_PluginControllers));
	memset(&m_Controllers, 0, sizeof(m_Controllers));

	Init(FileName);
}

bool CControl_Plugin::Init(const char * FileName)
{
	if (!CPlugin::Init(FileName))
	{
		UnloadPlugin();
		return false;
	}

	// Find entries for functions in DLL
	LoadFunction(InitiateControllers);
	LoadFunction(ControllerCommand);
	LoadFunction(GetKeys);
	LoadFunction(ReadController);
	LoadFunction(WM_KeyDown);
	LoadFunction(WM_KeyUp);
	LoadFunction(RumbleCommand);

	if (InitiateControllers == NULL
		// NOTE: Comment says PluginLoaded is a 0x0101+ function, but the check states 0x0102
		// Assuming existing code logic is correct...
		|| (m_PluginInfo.Version >= 0x0102
		&& PluginLoaded == NULL))
	{
		UnloadPlugin();
		return false;
	}

	if (PluginLoaded != NULL)
		PluginLoaded();

	// Allocate our own controller
	m_AllocatedControllers = true;
	for (int i = 0; i < 4; i++)
		m_Controllers[i] = new CCONTROL(m_PluginControllers[i].Present, m_PluginControllers[i].RawData, m_PluginControllers[i].Plugin);

	return true;
}

bool CControl_Plugin::Initiate(CN64System * System, CMainGui * RenderWindow)
{
	for (int i = 0; i < 4; i++)
	{
		m_PluginControllers[i].Present = FALSE;
		m_PluginControllers[i].RawData = FALSE;
		m_PluginControllers[i].Plugin = PLUGIN_NONE;
	}

	// Test Plugin version
	if (m_PluginInfo.Version == 0x0100)
	{
		((fInitiateControllers_1_0)InitiateControllers)((HWND)RenderWindow->m_hMainWindow, m_PluginControllers);
		m_Initilized = true;
	}
	else if (m_PluginInfo.Version >= 0x0101)
	{
		CONTROL_INFO ControlInfo;
		BYTE Buffer[100];

		ControlInfo.Controls = m_PluginControllers;
		ControlInfo.HEADER = (System == NULL ? Buffer : g_Rom->GetRomAddress());
		ControlInfo.hinst = GetModuleHandle(NULL);
		ControlInfo.hMainWindow = (HWND)RenderWindow->m_hMainWindow;
		ControlInfo.MemoryBswaped = TRUE;

		((fInitiateControllers_1_1)InitiateControllers)(&ControlInfo);
		m_Initilized = true;
	}

	//jabo had a bug so I call CreateThread so his dllmain gets called again
	DWORD ThreadID;
	HANDLE hthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DummyFunction, NULL, 0, &ThreadID);
	CloseHandle(hthread);

	return m_Initilized;
}

void CControl_Plugin::UnloadPlugin()
{
	CPlugin::UnloadPlugin();

	if (m_AllocatedControllers)
	{
		for (int count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++)
		{
			delete m_Controllers[count];
			m_Controllers[count] = NULL;
		}
	}

	m_AllocatedControllers = false;
	ControllerCommand = NULL;
	GetKeys = NULL;
	ReadController = NULL;
	WM_KeyDown = NULL;
	WM_KeyUp = NULL;
}

void CControl_Plugin::UpdateKeys(void) {
	if (!m_AllocatedControllers) { return; }
	for (int cont = 0; cont < sizeof(m_Controllers) / sizeof(m_Controllers[0]); cont++) {
		if (!m_Controllers[cont]->m_Present) { continue; }
		if (!m_Controllers[cont]->m_RawData) {
			GetKeys(cont, &m_Controllers[cont]->m_Buttons);
		}
		else {
			g_Notify->BreakPoint(__FILE__, __LINE__);
		}
	}
	if (ReadController) { ReadController(-1, NULL); }
}

void CControl_Plugin::SetControl(CControl_Plugin const * const Plugin)
{
	if (m_AllocatedControllers) {
		for (int count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++) {
			delete m_Controllers[count];
			m_Controllers[count] = NULL;
		}
	}
	m_AllocatedControllers = false;
	for (int count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++) {
		m_Controllers[count] = Plugin->m_Controllers[count];
	}
}

CControl_Plugin::~CControl_Plugin()
{
	Close();
	UnloadPlugin();
}

CCONTROL::CCONTROL(DWORD &Present, DWORD &RawData, int &PlugType) :
m_Present(Present), m_RawData(RawData), m_PlugType(PlugType)
{
	m_Buttons.Value = 0;
}