#include "stdafx.h"

#include "ControllerPlugin.h"
#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/N64System/SystemGlobals.h>

CControl_Plugin::CControl_Plugin(void) :
    WM_KeyDown(nullptr),
    WM_KeyUp(nullptr),
    RumbleCommand(nullptr),
    GetKeys(nullptr),
    ReadController(nullptr),
    ControllerCommand(nullptr),
    m_AllocatedControllers(false)
{
    memset(&m_PluginControllers, 0, sizeof(m_PluginControllers));
    memset(&m_Controllers, 0, sizeof(m_Controllers));
}

CControl_Plugin::~CControl_Plugin()
{
    Close(nullptr);
    UnloadPlugin();
}

bool CControl_Plugin::LoadFunctions(void)
{
    // Find entries for functions in DLL
    void(CALL * InitiateControllers)(void);
    LoadFunction(InitiateControllers);
    LoadFunction(ControllerCommand);
    LoadFunction(GetKeys);
    LoadFunction(ReadController);
    LoadFunction(WM_KeyDown);
    LoadFunction(WM_KeyUp);
    LoadFunction(RumbleCommand);
    LoadFunction(WM_KillFocus);
    LoadFunction(EmulationPaused);

    // Make sure DLL had all needed functions
    if (InitiateControllers == nullptr)
    {
        UnloadPlugin();
        return false;
    }

    if (m_PluginInfo.Version >= 0x0102)
    {
        if (PluginOpened == nullptr)
        {
            UnloadPlugin();
            return false;
        }
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
        m_PluginControllers[i].Present = PRESENT_NONE;
        m_PluginControllers[i].RawData = false;
        m_PluginControllers[i].Plugin = PLUGIN_NONE;
    }

    // Test plugin version
    if (m_PluginInfo.Version == 0x0100)
    {
        // Get function from DLL
        void(CALL * InitiateControllers_1_0)(void * hMainWindow, CONTROL Controls[4]);
        _LoadFunction("InitiateControllers", InitiateControllers_1_0);
        if (InitiateControllers_1_0 == nullptr)
        {
            return false;
        }
#ifdef _WIN32
        InitiateControllers_1_0(Window->GetWindowHandle(), m_PluginControllers);
#else
        InitiateControllers_1_0(nullptr, m_PluginControllers);
#endif
        m_Initialized = true;
    }
    else if (m_PluginInfo.Version >= 0x0101)
    {
        CONTROL_INFO ControlInfo;
        ControlInfo.Controls = m_PluginControllers;
        ControlInfo.HEADER = (System == nullptr ? Buffer : g_Rom->GetRomAddress());
#ifdef _WIN32
        ControlInfo.hinst = Window ? Window->GetModuleInstance() : nullptr;
        ControlInfo.hWnd = Window ? Window->GetWindowHandle() : nullptr;
#else
        ControlInfo.hinst = nullptr;
        ControlInfo.hWnd = nullptr;
#endif
        ControlInfo.Reserved = true;

        if (m_PluginInfo.Version == 0x0101)
        {
            // Get function from DLL
            void(CALL * InitiateControllers_1_1)(CONTROL_INFO ControlInfo);
            _LoadFunction("InitiateControllers", InitiateControllers_1_1);
            if (InitiateControllers_1_1 == nullptr)
            {
                return false;
            }

            InitiateControllers_1_1(ControlInfo);
            m_Initialized = true;
        }
        else if (m_PluginInfo.Version >= 0x0102)
        {
            // Get function from DLL
            void(CALL * InitiateControllers_1_2)(CONTROL_INFO * ControlInfo);
            _LoadFunction("InitiateControllers", InitiateControllers_1_2);
            if (InitiateControllers_1_2 == nullptr)
            {
                return false;
            }

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
            m_Controllers[count] = nullptr;
        }
    }

    m_AllocatedControllers = false;
    ControllerCommand = nullptr;
    GetKeys = nullptr;
    ReadController = nullptr;
    WM_KeyDown = nullptr;
    WM_KeyUp = nullptr;
}

void CControl_Plugin::UpdateKeys(void)
{
    if (!m_AllocatedControllers)
    {
        return;
    }
    for (int32_t cont = 0; cont < sizeof(m_Controllers) / sizeof(m_Controllers[0]); cont++)
    {
        if (!m_Controllers[cont]->Present())
        {
            continue;
        }
        if (!m_Controllers[cont]->m_RawData)
        {
            GetKeys(cont, &m_Controllers[cont]->m_Buttons);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    if (ReadController)
    {
        ReadController(-1, nullptr);
    }
}

void CControl_Plugin::SetControl(CControl_Plugin const * const Plugin)
{
    if (m_AllocatedControllers)
    {
        for (int32_t count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++)
        {
            delete m_Controllers[count];
            m_Controllers[count] = nullptr;
        }
    }
    m_AllocatedControllers = false;
    for (int32_t count = 0; count < sizeof(m_Controllers) / sizeof(m_Controllers[0]); count++)
    {
        m_Controllers[count] = Plugin->m_Controllers[count];
    }
}

CCONTROL::CCONTROL(int32_t & Present, int32_t & RawData, int32_t & PlugType) :
    m_Present(Present), m_RawData(RawData), m_PlugType(PlugType)
{
    m_Buttons.Value = 0;
}
