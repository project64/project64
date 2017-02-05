/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64RomClass.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include "ControllerPlugin.h"

CControl_Plugin::CControl_Plugin(void) :
    WM_KeyDown(NULL),
    WM_KeyUp(NULL),
    RumbleCommand(NULL),
    GetKeys(NULL),
    ReadController(NULL),
    ControllerCommand(NULL),
    m_AllocatedControllers(false)
{
    memset(&m_PluginControllers, 0, sizeof(m_PluginControllers));
    memset(&m_Controllers, 0, sizeof(m_Controllers));
}

CControl_Plugin::~CControl_Plugin()
{
    Close(NULL);
    UnloadPlugin();
}

bool CControl_Plugin::LoadFunctions(void)
{
    // Find entries for functions in DLL
    void(CALL *InitiateControllers)(void);
    LoadFunction(InitiateControllers);
    LoadFunction(ControllerCommand);
    LoadFunction(GetKeys);
    LoadFunction(ReadController);
    LoadFunction(WM_KeyDown);
    LoadFunction(WM_KeyUp);
    LoadFunction(RumbleCommand);

    //Make sure dll had all needed functions
    if (InitiateControllers == NULL) { UnloadPlugin(); return false; }

    if (m_PluginInfo.Version >= 0x0102)
    {
        if (PluginOpened == NULL) { UnloadPlugin(); return false; }
    }

    // Allocate our own controller
    m_AllocatedControllers = true;
    for (int32_t i = 0; i < 4; i++)
    {
        m_Controllers[i] = new CCONTROL(m_PluginControllers[i].Present, m_PluginControllers[i].RawData, m_PluginControllers[i].Plugin);
    }
    return true;
}

bool CControl_Plugin::Initiate(CN64System * System, RenderWindow * Window)
{
    static uint8_t Buffer[100];

    for (int32_t i = 0; i < 4; i++)
    {
        m_PluginControllers[i].Present = false;
        m_PluginControllers[i].RawData = false;
        m_PluginControllers[i].Plugin = PLUGIN_NONE;
    }

    // Test Plugin version
    if (m_PluginInfo.Version == 0x0100)
    {
        //Get Function from DLL
        void(CALL *InitiateControllers_1_0)(void * hMainWindow, CONTROL Controls[4]);
        _LoadFunction("InitiateControllers", InitiateControllers_1_0);
        if (InitiateControllers_1_0 == NULL) { return false; }
#ifdef _WIN32
        InitiateControllers_1_0(Window->GetWindowHandle(), m_PluginControllers);
#else
        InitiateControllers_1_0(NULL, m_PluginControllers);
#endif
        m_Initialized = true;
    }
    else if (m_PluginInfo.Version >= 0x0101)
    {
        CONTROL_INFO ControlInfo;
        ControlInfo.Controls = m_PluginControllers;
        ControlInfo.HEADER = (System == NULL ? Buffer : g_Rom->GetRomAddress());
#ifdef _WIN32
        ControlInfo.hinst = Window ? Window->GetModuleInstance() : NULL;
        ControlInfo.hMainWindow = Window ? Window->GetWindowHandle() : NULL;
#else
        ControlInfo.hinst = NULL;
        ControlInfo.hMainWindow = NULL;
#endif
        ControlInfo.MemoryBswaped = true;

        if (m_PluginInfo.Version == 0x0101)
        {
            //Get Function from DLL
            void(CALL *InitiateControllers_1_1)(CONTROL_INFO ControlInfo);
            _LoadFunction("InitiateControllers", InitiateControllers_1_1);
            if (InitiateControllers_1_1 == NULL) { return false; }

            InitiateControllers_1_1(ControlInfo);
            m_Initialized = true;
        }
        else if (m_PluginInfo.Version >= 0x0102)
        {
            //Get Function from DLL
            void(CALL *InitiateControllers_1_2)(CONTROL_INFO * ControlInfo);
            _LoadFunction("InitiateControllers", InitiateControllers_1_2);
            if (InitiateControllers_1_2 == NULL) { return false; }

            InitiateControllers_1_2(&ControlInfo);
            m_Initialized = true;
        }
    }
    return m_Initialized;
}

void CControl_Plugin::UnloadPluginDetails(void)
{
    if (m_AllocatedControllers)
    {
        for (int32_t count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++)
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

void CControl_Plugin::UpdateKeys(void)
{
    if (!m_AllocatedControllers) { return; }
    for (int32_t cont = 0; cont < sizeof(m_Controllers) / sizeof(m_Controllers[0]); cont++)
    {
        if (!m_Controllers[cont]->m_Present) { continue; }
        if (!m_Controllers[cont]->m_RawData)
        {
            GetKeys(cont, &m_Controllers[cont]->m_Buttons);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    if (ReadController) { ReadController(-1, NULL); }
}

void CControl_Plugin::SetControl(CControl_Plugin const * const Plugin)
{
    if (m_AllocatedControllers)
    {
        for (int32_t count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++)
        {
            delete m_Controllers[count];
            m_Controllers[count] = NULL;
        }
    }
    m_AllocatedControllers = false;
    for (int32_t count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++)
    {
        m_Controllers[count] = Plugin->m_Controllers[count];
    }
}

CCONTROL::CCONTROL(int32_t &Present, int32_t &RawData, int32_t &PlugType) :
    m_Present(Present), m_RawData(RawData), m_PlugType(PlugType)
{
    m_Buttons.Value = 0;
}
