// Common controller plugin specification, version 1.1 maintained by Zilmar

// All questions or suggestions should go through the emutalk plugin forum.
// http://www.emutalk.net/cgi-bin/ikonboard/ikonboard.cgi?s=3bd272222f66ffff;act=SF;f=20

#ifndef _CONTR_H_INCLUDED__
#define _CONTR_H_INCLUDED__

#if defined(__cplusplus)
extern "C" {
#endif

// Note: BOOL, BYTE, WORD, DWORD, TRUE, FALSE are defined in windows.h

#define PLUGIN_TYPE_CONTROLLER      4

#ifndef SPECS_VERSION
#define SPECS_VERSION           0x0101
#endif

// Controller plugins
#define PLUGIN_NONE                 1
#define PLUGIN_MEMPAK               2
// not implemented for non-raw data
#define PLUGIN_RUMBLE_PAK           3
// not implemented for non-raw data
#define PLUGIN_TRANSFER_PAK         4
// The controller plugin is passed in raw data
#define PLUGIN_RAW                  5

/*
Note about controller plugin's:
The rumble pak needs a function for the force feedback joystick and transfer pak
probably needs a function for the plugin to be able to select the Game Boy ROM and
EEPROM...maybe this should be done by the emu instead of the plugin, but I think
it probably should be done by the plugin. I will see about adding these functions
in the next specification.
*/

#define EXPORT                      __declspec(dllexport)
#define CALL                        _cdecl

    // Structures
    typedef struct
    {
        WORD Version;        // Should be set to 0x0101
        WORD Type;           // Set to PLUGIN_TYPE_CONTROLLER
        char Name[100];      // Name of the DLL
        BOOL Reserved1;
        BOOL Reserved2;
    } PLUGIN_INFO;

    typedef struct
    {
        BOOL Present;
        BOOL RawData;
        int  Plugin;
    } CONTROL;

    typedef union
    {
        DWORD Value;
        struct
        {
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

            signed   X_AXIS : 8;

            signed   Y_AXIS : 8;
        };
    } BUTTONS;

    typedef struct
    {
        HWND hMainWindow;
        HINSTANCE hinst;

        BOOL MemoryBswaped;     // If this is set to TRUE, then the memory has been pre-bswap'd on a DWORD (32-bit) boundary, only effects header.
        //  eg. the first 8 bytes are stored like this:
        //  4 3 2 1   8 7 6 5
        BYTE * HEADER;          // This is the ROM header (first 40h bytes of the ROM)
        CONTROL *Controls;      // A pointer to an array of 4 controllers. eg:
        // CONTROL Controls[4];
    } CONTROL_INFO;

    /*
    Function: CloseDLL
    Purpose:  This function is called when the emulator is closing
    down allowing the DLL to de-initialize.
    Input: None
    Output: None
    */
	
    EXPORT void CALL CloseDLL(void);

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
	
    EXPORT void CALL ControllerCommand(int Control, BYTE * Command);

    /*
    Function: DllAbout
    Purpose: This function is optional function that is provided
    to give further information about the DLL.
    Input: A handle to the window that calls this function
    Output: None
    */
	
    EXPORT void CALL DllAbout(HWND hParent);

    /*
    Function: DllConfig
    Purpose: This function is optional function that is provided
    to allow the user to configure the DLL.
    Input: A handle to the window that calls this function
    Output: None
    */
	
    EXPORT void CALL DllConfig(HWND hParent);

    /*
    Function: DllTest
    Purpose: This function is optional function that is provided
    to allow the user to test the DLL.
    Input: A handle to the window that calls this function
    Output: None
    */
	
    EXPORT void CALL DllTest(HWND hParent);

    /*
    Function: GetDllInfo
    Purpose:  This function allows the emulator to gather information
    about the DLL by filling in the PluginInfo structure.
    Input: A pointer to a PLUGIN_INFO structure that needs to be
    filled by the function. (see def above)
    Output: None
    */
	
    EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo);

    /*
    Function: GetKeys
    Purpose: To get the current state of the controllers buttons.
    Input: Controller number (0 to 3)
    - A pointer to a BUTTONS structure to be filled with
    the controller state.
    Output: None
    */
	
    EXPORT void CALL GetKeys(int Control, BUTTONS * Keys);

    /*
    Function: InitiateControllers
    Purpose: This function initializes how each of the controllers
    should be handled.
    Input: The handle to the main window.
    - A controller structure that needs to be filled for
    the emulator to know how to handle each controller.
    Output: None
    */
	
#if (SPECS_VERSION < 0x0101)
EXPORT void CALL InitiateControllers(void * hMainWindow, CONTROL Controls[4]);
#elif (SPECS_VERSION == 0x0101)
EXPORT void CALL InitiateControllers(CONTROL_INFO ControlInfo);
// Typo in the official specs, but it works!
#else
EXPORT void CALL InitiateControllers(CONTROL_INFO * ControlInfo);
#endif

    /*
    Function: ReadController
    Purpose: To process the raw data in the PIF RAM that is about to
    be read.
    Input: Controller Number (0 to 3) and -1 signaling end of
    processing the PIF RAM.
    - Pointer of data to be processed.
    Output: None
    Note: This function is only needed if the DLL is allowing raw
    data.
    */
	
    EXPORT void CALL ReadController(int Control, BYTE * Command);

    /*
    Function: RomClosed
    Purpose:  This function is called when a rom is closed.
    Input: None
    Output: None
    */
	
    EXPORT void CALL RomClosed(void);

    /*
    Function: RomOpen
    Purpose:  This function is called when a rom is open. (from the
    emulation thread)
    Input: None
    Output: None
    */
	
    EXPORT void CALL RomOpen(void);

    /*
    Function: WM_KeyDown
    Purpose: To pass the WM_KeyDown message from the emulator to the
    plugin.
    Input: wParam and lParam of the WM_KEYDOWN message.
    Output: None
    */
	
    EXPORT void CALL WM_KeyDown(WPARAM wParam, LPARAM lParam);

    /*
    Function: WM_KeyUp
    Purpose:  To pass the WM_KEYUP message from the emulator to the
    plugin.
    Input: wParam and lParam of the WM_KEYDOWN message.
    Output: None
    */
	
    EXPORT void CALL WM_KeyUp(WPARAM wParam, LPARAM lParam);

#if defined(__cplusplus)
}
#endif
#endif
