#pragma once
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef EXPORT
#if defined(__cplusplus)
#if defined(_WIN32)
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C" __attribute__((visibility("default")))
#endif
#else
#if defined(_WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
#endif
#endif

#ifndef CALL
#if defined(_WIN32)
#define CALL __cdecl
#else
#define CALL
#endif
#endif

enum PLUGIN_TYPE
{
    PLUGIN_TYPE_NONE = 0,
    PLUGIN_TYPE_RSP = 1,
    PLUGIN_TYPE_VIDEO = 2,
    PLUGIN_TYPE_AUDIO = 3,
    PLUGIN_TYPE_CONTROLLER = 4,
};

typedef struct
{
    uint16_t Version;    // Should be set plugin spec version eg VIDEO_SPECS_VERSION
    uint16_t Type;       // Set to the plugin type, eg PLUGIN_TYPE_VIDEO
    char Name[100];      // Name of the DLL
    int32_t Reserved1;
    int32_t Reserved2;
} PLUGIN_INFO;

/*
Function: CloseDLL
Purpose: This function is called when the emulator is closing
down allowing the dll to de-initialise.
Input: none
Output: none
*/
EXPORT void CALL CloseDLL(void);

/*
Function: DllAbout
Purpose: This function is optional function that is provided
to give further information about the DLL.
Input: a handle to the window that calls this function
Output: none
*/
EXPORT void CALL DllAbout(void * hParent);

/*
Function: DllConfig
Purpose: This function is optional function that is provided
to allow the user to configure the dll
Input: a handle to the window that calls this function
Output: none
*/
EXPORT void CALL DllConfig(void * hParent);

/*
Function: GetDllInfo
Purpose: This function allows the emulator to gather information
about the dll by filling in the PluginInfo structure.
Input: a pointer to a PLUGIN_INFO stucture that needs to be
filled by the function. (see def above)
Output: none
*/
EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo);

/*
Function: RomClosed
Purpose: This function is called when a rom is closed.
Input: none
Output: none
*/
EXPORT void CALL RomClosed(void);

/*
Function: RomOpen
Purpose: This function is called when a rom is open. (from the
emulation thread)
Input: none
Output: none
*/
EXPORT void CALL RomOpen(void);

/*
Function: PluginLoaded
Purpose: This function is called when the plugin is loaded
Input: none
Output: none
*/
EXPORT void CALL PluginLoaded(void);

#if defined(__cplusplus)
}
#endif
