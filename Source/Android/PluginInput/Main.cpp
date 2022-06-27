#include "Version.h"
#include <Project64-plugin-spec/Input.h>
#include <stdio.h>
#include <string.h>

#ifdef ANDROID
#include <jni.h>
#endif

static CONTROL_INFO g_control_info;
BUTTONS g_buttons;

void ShowAboutWindow (void * hParent);

/*
Function: CloseDLL
Purpose:  This function is called when the emulator is closing
down allowing the DLL to de-initialize.
Input:    None
Output:   None
*/

EXPORT void CALL CloseDLL (void)
{
}

/*
Function: ControllerCommand
Purpose:  To process the raw data that has just been sent to a
specific controller.
Input:    Controller Number (0 to 3) and -1 signaling end of
processing the PIF RAM.
- Pointer of data to be processed.
Output:   None

Note:     This function is only needed if the DLL is allowing raw
data, or the plugin is set to raw

The data that is being processed looks like this:
Initialize controller: 01 03 00 FF FF FF
Read controller:      01 04 01 FF FF FF FF
*/

EXPORT void CALL ControllerCommand ( int /*Control*/, uint8_t * /*Command*/)
{
}

/*
Function: DllAbout
Purpose:  This function is optional function that is provided
to give further information about the DLL.
Input:    A handle to the window that calls this function
Output:   None
*/

EXPORT void CALL DllAbout ( void * hParent )
{
#ifdef _WIN32
	ShowAboutWindow(hParent);
#endif
}

/*
Function: DllConfig
Purpose:  This function is optional function that is provided
to allow the user to configure the DLL
Input:    A handle to the window that calls this function
Output:   None
*/

EXPORT void CALL DllConfig ( void * /*hParent*/ )
{
}

/*
Function: DllTest
Purpose:  This function is optional function that is provided
to allow the user to test the DLL
Input:    A handle to the window that calls this function
Output:   None
*/

EXPORT void CALL DllTest ( void * /*hParent*/ )
{
}

/*
Function: GetDllInfo
Purpose:  This function allows the emulator to gather information
about the DLL by filling in the PluginInfo structure.
Input:    A pointer to a PLUGIN_INFO structure that needs to be
filled by the function. (see def above)
Output:   None
*/

EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
    PluginInfo->Version = 0x0101;
    PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
#ifdef _DEBUG
    sprintf(PluginInfo->Name, "Android input debug plugin %s", VER_FILE_VERSION_STR);
#else
    sprintf(PluginInfo->Name, "Android input plugin %s", VER_FILE_VERSION_STR);
#endif
}

/*
Function: GetKeys
Purpose:  To get the current state of the controllers buttons.
Input:    Controller Number (0 to 3)
- A pointer to a BUTTONS structure to be filled with
the controller state.
Output:   None
*/

EXPORT void CALL GetKeys(int Control, BUTTONS * Keys )
{
    if (Control == 0)
    {
        *Keys = g_buttons;
    }
}

/*
Function: InitiateControllers
Purpose:  This function initializes how each of the controllers
should be handled.
Input:    The handle to the main window.
- A controller structure that needs to be filled for
the emulator to know how to handle each controller.
Output:   None
*/

EXPORT void CALL InitiateControllers (CONTROL_INFO * ControlInfo)
{
    g_control_info = *ControlInfo;
    g_control_info.Controls[0].Present = true;
    g_control_info.Controls[0].Plugin = PLUGIN_MEMPAK;
}

/*
Function: ReadController
Purpose:  To process the raw data in the PIF RAM that is about to
be read.
Input:    Controller Number (0 to 3) and -1 signaling end of
processing the PIF RAM.
- Pointer of data to be processed.
Output:   None
Note:     This function is only needed if the DLL is allowing raw
data.
*/

EXPORT void CALL ReadController ( int /*Control*/, uint8_t * /*Command*/ )
{
}

/*
Function: RomClosed
Purpose:  This function is called when a ROM is closed.
Input:    None
Output:   None
*/

EXPORT void CALL RomClosed (void)
{
}

/*
Function: RomOpen
Purpose:  This function is called when a ROM is open (from the
emulation thread)
Input:    None
Output:   None
*/

EXPORT void CALL RomOpen (void)
{
    memset(&g_buttons, 0, sizeof(g_buttons));
}

/*
Function: WM_KeyDown
Purpose:  To pass the WM_KeyDown message from the emulator to the
plugin.
Input:    wParam and lParam of the WM_KEYDOWN message.
Output:   None
*/

EXPORT void CALL WM_KeyDown( uint32_t /*wParam*/, uint32_t /*lParam*/ )
{
}

/*
Function: WM_KeyUp
Purpose:  To pass the WM_KEYUP message from the emulator to the
plugin.
Input:    wParam and lParam of the WM_KEYDOWN message.
Output:   None
*/

EXPORT void CALL WM_KeyUp( uint32_t /*wParam*/, uint32_t /*lParam*/ )
{
}

#ifdef ANDROID
EXPORT void CALL Java_emu_project64_jni_NativeInput_setState(JNIEnv* env, jclass jcls, jint controllerNum, jbooleanArray Buttons, jint pXAxis, jint pYAxis)
{
    jboolean* elements = env->GetBooleanArrayElements(Buttons, NULL);
    if (controllerNum == 0)
    {
        g_buttons.R_DPAD = elements[0];
        g_buttons.L_DPAD = elements[1];
        g_buttons.D_DPAD = elements[2];
        g_buttons.U_DPAD = elements[3];
        g_buttons.START_BUTTON = elements[4];
        g_buttons.Z_TRIG = elements[5];
        g_buttons.B_BUTTON = elements[6];
        g_buttons.A_BUTTON = elements[7];
        g_buttons.R_CBUTTON = elements[8];
        g_buttons.L_CBUTTON = elements[9];
        g_buttons.D_CBUTTON = elements[10];
        g_buttons.U_CBUTTON = elements[11];
        g_buttons.R_TRIG = elements[12];
        g_buttons.L_TRIG = elements[13];
        g_buttons.Reserved1 = elements[14];
        g_buttons.Reserved2 = elements[15];
        g_buttons.Y_AXIS = pXAxis;
        g_buttons.X_AXIS = pYAxis;
    }
    env->ReleaseBooleanArrayElements(Buttons, elements, 0);
}
#endif
