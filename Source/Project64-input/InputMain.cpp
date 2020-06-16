/****************************************************************************
*                                                                           *
* Project64-audio - A Nintendo 64 audio plugin.                             *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2020 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "ControllerSpec1.1.h"
#include "Version.h"
#include <stdio.h>

/******************************************************************
Function: CloseDLL
Purpose:  This function is called when the emulator is closing
down allowing the dll to de-initialise.
input:    none
output:   none
*******************************************************************/
EXPORT void CALL CloseDLL(void)
{
}

/******************************************************************
Function: ControllerCommand
Purpose:  To process the raw data that has just been sent to a
specific controller.
input:    - Controller Number (0 to 3) and -1 signalling end of
processing the pif ram.
- Pointer of data to be processed.
output:   none

note:     This function is only needed if the DLL is allowing raw
data, or the plugin is set to raw

the data that is being processed looks like this:
initilize controller: 01 03 00 FF FF FF
read controller:      01 04 01 FF FF FF FF
*******************************************************************/
EXPORT void CALL ControllerCommand(int32_t /*Control*/, uint8_t * /*Command*/)
{
}

/******************************************************************
Function: DllAbout
Purpose:  This function is optional function that is provided
to give further information about the DLL.
input:    a handle to the window that calls this function
output:   none
*******************************************************************/
/*EXPORT void CALL DllAbout(void * hParent)
{
}*/

/******************************************************************
Function: DllConfig
Purpose:  This function is optional function that is provided
to allow the user to configure the dll
input:    a handle to the window that calls this function
output:   none
*******************************************************************/
EXPORT void CALL DllConfig(void * /*hParent*/)
{
}

/******************************************************************
Function: DllTest
Purpose:  This function is optional function that is provided
to allow the user to test the dll
input:    a handle to the window that calls this function
output:   none
*******************************************************************/
EXPORT void CALL DllTest(void * /*hParent*/)
{
}

/******************************************************************
Function: GetDllInfo
Purpose:  This function allows the emulator to gather information
about the dll by filling in the PluginInfo structure.
input:    a pointer to a PLUGIN_INFO stucture that needs to be
filled by the function. (see def above)
output:   none
*******************************************************************/
EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = CONTROLLER_SPECS_VERSION;
    PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
#ifdef _DEBUG
    sprintf(PluginInfo->Name, "Project64 Input Plugin (Debug): %s", VER_FILE_VERSION_STR);
#else
    sprintf(PluginInfo->Name, "Project64 Input Plugin: %s", VER_FILE_VERSION_STR);
#endif
    PluginInfo->MemoryBswaped = true;
    PluginInfo->NormalMemory = false;
}

/******************************************************************
Function: GetKeys
Purpose:  To get the current state of the controllers buttons.
input:    - Controller Number (0 to 3)
- A pointer to a BUTTONS structure to be filled with
the controller state.
output:   none
*******************************************************************/
EXPORT void CALL GetKeys(int32_t /*Control*/, BUTTONS * /*Keys*/)
{
}

/******************************************************************
  Function: InitiateControllers
  Purpose:  This function initialises how each of the controllers
            should be handled.
  input:    - A controller structure that needs to be filled for
              the emulator to know how to handle each controller.
  output:   none
*******************************************************************/
EXPORT void CALL InitiateControllers(CONTROL_INFO * /*ControlInfo*/)
{
}

/******************************************************************
Function: ReadController
Purpose:  To process the raw data in the pif ram that is about to
be read.
input:    - Controller Number (0 to 3) and -1 signalling end of
processing the pif ram.
- Pointer of data to be processed.
output:   none
note:     This function is only needed if the DLL is allowing raw
data.
*******************************************************************/
EXPORT void CALL ReadController(int /*Control*/, uint8_t * /*Command*/)
{
}

/******************************************************************
Function: RomClosed
Purpose:  This function is called when a rom is closed.
input:    none
output:   none
*******************************************************************/
EXPORT void CALL RomClosed(void)
{
}

/******************************************************************
Function: RomOpen
Purpose:  This function is called when a rom is open. (from the
emulation thread)
input:    none
output:   none
*******************************************************************/
EXPORT void CALL RomOpen(void)
{
}

/******************************************************************
Function: WM_KeyDown
Purpose:  To pass the WM_KeyDown message from the emulator to the
plugin.
input:    wParam and lParam of the WM_KEYDOWN message.
output:   none
*******************************************************************/
EXPORT void CALL WM_KeyDown(uint32_t /*wParam*/, uint32_t /*lParam*/)
{
}

/******************************************************************
Function: WM_KeyUp
Purpose:  To pass the WM_KEYUP message from the emulator to the
plugin.
input:    wParam and lParam of the WM_KEYDOWN message.
output:   none
*******************************************************************/
EXPORT void CALL WM_KeyUp(uint32_t /*wParam*/, uint32_t /*lParam*/)
{
}

