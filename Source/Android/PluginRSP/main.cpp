#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/Version.h>
#include <Settings/Settings.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>

void * g_hinstDLL;

BOOL WINAPI DllMain(void * hinst, DWORD /*fdwReason*/, LPVOID /*lpvReserved*/)
{
    g_hinstDLL = hinst;
    return true;
}
#endif

/*
Function: CloseDLL
Purpose: This function is called when the emulator is closing
down allowing the DLL to de-initialize.
Input: None
Output: None
*/

void CloseDLL(void)
{
    FreeRSP();
}

/*
Function: DllAbout
Purpose: This function is optional function that is provided
to give further information about the DLL.
Input: A handle to the window that calls this function
Output: None
*/

void DllAbout(void * hParent)
{
#ifdef _WIN32
    MessageBox((HWND)hParent, L"need to do", L"About", MB_OK | MB_ICONINFORMATION);
#endif
}

/*
Function: GetDllInfo
Purpose: This function allows the emulator to gather information
about the DLL by filling in the PluginInfo structure.
Input: A pointer to a PLUGIN_INFO structure that needs to be
filled by the function. (see def above)
Output: None
*/

EXPORT void GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = 0x0103;
    PluginInfo->Type = PLUGIN_TYPE_RSP;
#ifdef _DEBUG
    sprintf(PluginInfo->Name, "RSP Basic Debug Plugin %s", VER_FILE_VERSION_STR);
#else
    sprintf(PluginInfo->Name, "RSP Basic Plugin %s", VER_FILE_VERSION_STR);
#endif
    PluginInfo->Reserved2 = false;
    PluginInfo->Reserved1 = true;
}

/*
Function: InitiateRSP
Purpose: This function is called when the DLL is started to give
information from the emulator that the N64 RSP interface needs.
Input: Rsp_Info is passed to this function which is defined
above.
CycleCount is the number of cycles between switching
control between the RSP and r4300i core.
Output: None
*/

EXPORT void InitiateRSP(RSP_INFO Rsp_Info, uint32_t * CycleCount)
{
    InitilizeRSP(Rsp_Info);
    *CycleCount = 0;
}

/*
Function: RomOpen
Purpose: This function is called when a ROM is opened.
Input: None
Output: None
*/

EXPORT void RomOpen(void)
{
    RspRomOpened();
}

/*
Function: RomClosed
Purpose: This function is called when a ROM is closed.
Input: None
Output: None
*/

EXPORT void RomClosed(void)
{
    RspRomClosed();
}

void PluginLoaded(void)
{
    RspPluginLoaded();
}

void UseUnregisteredSetting(int /*SettingID*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}