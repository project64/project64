#include "ControllerSpec1.1.h"
#include "InputConfigUI.h"
#include "Version.h"
#include "CProject64Input.h"
#include "InputSettings.h"
#include <stdio.h>

/*
Function: CloseDLL
Purpose: This function is called when the emulator is closing
down allowing the DLL to de-initialize.
Input: None
Output: None
*/

EXPORT void CALL CloseDLL(void)
{
    CleanupInputSettings();
}

/*
Function: ControllerCommand
Purpose: To process the raw data that has just been sent to a
specific controller.
Input: Controller number (0 to 3) and -1 signaling end of
processing the PIF RAM.
- Pointer of data to be processed.
Output: None
Note: This function is only needed if the DLL is allowing raw
data, or the plugin is set to raw.
The data that is being processed looks like this:
Initialize controller: 01 03 00 FF FF FF
Read controller: 01 04 01 FF FF FF FF
*/

EXPORT void CALL ControllerCommand(int32_t /*Control*/, uint8_t * /*Command*/)
{
}

/*
Function: DllAbout
Purpose: This function is optional function that is provided
to give further information about the DLL.
Input: A handle to the window that calls this function.
Output: None
*/

//EXPORT void CALL DllAbout(void * hParent)
//{
//}

/*
Function: DllConfig
Purpose: This function is optional function that is provided
to allow the user to configure the DLL.
Input: A handle to the window that calls this function.
Output: None
*/

#ifdef _WIN32
EXPORT void CALL DllConfig(void * hParent)
{
    if (g_InputPlugin != nullptr)
    {
        g_InputPlugin->UnlockMouse();
    }
    ConfigInput(hParent);
}
#endif

/*
Function: DllTest
Purpose: This function is optional function that is provided
to allow the user to test the DLL.
Input: A handle to the window that calls this function.
Output: None
*/

EXPORT void CALL DllTest(void * /*hParent*/)
{
}

/*
Function: GetDllInfo
Purpose: This function allows the emulator to gather information
about the DLL by filling in the PluginInfo structure.
Input: A pointer to a PLUGIN_INFO structure that needs to be
filled by the function. (see def above)
Output: None
*/

EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = CONTROLLER_SPECS_VERSION;
    PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
#ifdef _DEBUG
    sprintf(PluginInfo->Name, "Project64 input plugin (Debug): %s", VER_FILE_VERSION_STR);
#else
    sprintf(PluginInfo->Name, "Project64 input plugin: %s", VER_FILE_VERSION_STR);
#endif
    PluginInfo->MemoryBswaped = true;
    PluginInfo->NormalMemory = false;
}

/*
Function: GetKeys
Purpose: To get the current state of the controllers buttons.
Input: Controller number (0 to 3)
- A pointer to a BUTTONS structure to be filled with
the controller state.
Output: None
*/

EXPORT void CALL GetKeys(int32_t Control, BUTTONS * Keys)
{
    g_InputPlugin->GetKeys(Control, Keys);
}

/*
Function: InitiateControllers
Purpose: This function initializes how each of the controllers
should be handled.
Input: A controller structure that needs to be filled for
the emulator to know how to handle each controller.
Output: None
*/

EXPORT void CALL InitiateControllers(CONTROL_INFO * ControlInfo)
{
    if (g_InputPlugin != nullptr)
    {
        g_InputPlugin->InitiateControllers(ControlInfo);
    }
}

/*
Function: ReadController
Purpose: To process the raw data in the PIF RAM that is about to
be read.
Input: Controller number (0 to 3) and -1 signaling end of
processing the PIF RAM.
- Pointer of data to be processed.
Output: None
Note: This function is only needed if the DLL is allowing raw
data.
*/

EXPORT void CALL ReadController(int /*Control*/, uint8_t * /*Command*/)
{
}

/*
Function: RomClosed
Purpose: This function is called when a ROM is closed.
Input: None
Output: None
*/

EXPORT void CALL RomClosed(void)
{
    if (g_InputPlugin != nullptr)
    {
        g_InputPlugin->UnlockMouse();
    }
}

/*
Function: RomOpen
Purpose: This function is called when a ROM is open. (from the
emulation thread)
Input: None
Output: None
*/

EXPORT void CALL RomOpen(void)
{
    if (g_InputPlugin != nullptr)
    {
        g_InputPlugin->LockMouse();
    }
}

/*
Function: EmulationPaused
Purpose: This function is called when the emulation is paused. (from the
emulation thread)
Input: None
Output: None
*/

EXPORT void CALL EmulationPaused(void)
{
    if (g_InputPlugin != nullptr)
    {
        g_InputPlugin->UnlockMouse();
    }
}

/*
Function: WM_KeyDown
Purpose: To pass the WM_KeyDown message from the emulator to the
plugin.
Input: wParam and lParam of the WM_KEYDOWN message.
Output: None
*/

EXPORT void CALL WM_KeyDown(uint32_t /*wParam*/, uint32_t /*lParam*/)
{
}

/*
Function: WM_KeyUp
Purpose: To pass the WM_KEYUP message from the emulator to the
plugin.
Input: wParam and lParam of the WM_KEYDOWN message.
Output: None
*/

EXPORT void CALL WM_KeyUp(uint32_t /*wParam*/, uint32_t /*lParam*/)
{
}

/*
Function: WM_KillFocus
Purpose: To pass the WM_KILLFOCUS message from the emulator to the
plugin.
Input: wParam and lParam of the WM_KILLFOCUS message.
Output: None
*/

EXPORT void CALL WM_KillFocus(uint32_t /*wParam*/, uint32_t /*lParam*/)
{
    if (g_InputPlugin != nullptr)
    {
        g_InputPlugin->UnlockMouse();
    }
}

EXPORT void CALL PluginLoaded(void)
{
    SetupInputSettings();
}

#include <Windows.h>

extern "C" int WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID /*lpReserved*/)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_InputPlugin = new CProject64Input(hinst);
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        delete g_InputPlugin;
        g_InputPlugin = nullptr;
    }
    return TRUE;
}
