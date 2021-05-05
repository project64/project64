// Project64 controller plugin spec, version 1.1

#pragma once
#include <stdint.h>

enum { PLUGIN_TYPE_CONTROLLER = 4 };

#if defined(_WIN32)
#define EXPORT      extern "C" __declspec(dllexport)
#define CALL        __cdecl
#else
#define EXPORT      extern "C" __attribute__((visibility("default")))
#define CALL
#endif

enum 
{
    CONTROLLER_SPECS_VERSION = 0x0102
};

enum
{
    PLUGIN_NONE = 1,
    PLUGIN_MEMPAK = 2,
    PLUGIN_RUMBLE_PAK = 3,
    PLUGIN_TRANSFER_PAK = 4,
    PLUGIN_RAW = 5,
};

// Structures

typedef struct
{
    uint16_t Version;    // Should be set to 0x0101
    uint16_t Type;       // Set to PLUGIN_TYPE_CONTROLLER
    char Name[100];      // Name of the DLL
    int32_t NormalMemory;
    int32_t MemoryBswaped;
} PLUGIN_INFO;

typedef struct
{
    int32_t Present;
    int32_t RawData;
    int32_t Plugin;
} CONTROL;

#pragma warning(push)
#pragma warning(disable : 4201) // warning C4201: non-standard extension used: nameless struct/union

typedef union
{
    uint32_t Value;
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

#pragma warning(pop)

typedef struct
{
    void * hwnd;
    void * hinst;
    int32_t MemoryBswaped;    // Set this to true
    uint8_t * HEADER;         // This is the ROM header (first 40h bytes of the ROM)
    CONTROL * Controls;       // A pointer to an array of 4 controllers
} CONTROL_INFO;

/*
Function: CloseDLL
Purpose: This function is called when the emulator is closing
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

EXPORT void CALL ControllerCommand(int32_t Control, uint8_t * Command);

/*
Function: DllAbout
Purpose: This function is optional function that is provided
to give further information about the DLL.
Input: A handle to the window that calls this function.
Output: None
*/

EXPORT void CALL DllAbout(void * hParent);

/*
Function: DllConfig
Purpose: This function is optional function that is provided
to allow the user to configure the DLL.
Input: A handle to the window that calls this function.
Output: None
*/

EXPORT void CALL DllConfig(void * hParent);

/*
Function: DllTest
Purpose: This function is optional function that is provided
to allow the user to test the DLL.
Input: A handle to the window that calls this function.
Output: None
*/

EXPORT void CALL DllTest(void * hParent);

/*
Function: GetDllInfo
Purpose: This function allows the emulator to gather information
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

EXPORT void CALL GetKeys(int32_t Control, BUTTONS * Keys);

/*
Function: InitiateControllers
Purpose: This function initializes how each of the controllers
should be handled.
Input: The handle to the main window.
- A controller structure that needs to be filled for
the emulator to know how to handle each controller.
Output: None
*/

EXPORT void CALL InitiateControllers(CONTROL_INFO * ControlInfo);

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

EXPORT void CALL ReadController(int Control, uint8_t * Command);

/*
Function: RomClosed
Purpose: This function is called when a ROM is closed.
Input: None
Output: None
*/

EXPORT void CALL RomClosed(void);

/*
Function: RomOpen
Purpose: This function is called when a ROM is open. (from the
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

EXPORT void CALL WM_KeyDown(uint32_t wParam, uint32_t lParam);

/*
Function: WM_KeyUp
Purpose: To pass the WM_KEYUP message from the emulator to the
plugin.
Input: wParam and lParam of the WM_KEYDOWN message.
Output: None
*/

EXPORT void CALL WM_KeyUp(uint32_t wParam, uint32_t lParam);
