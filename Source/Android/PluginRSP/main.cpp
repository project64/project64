#include "stdafx.h"
#include "Rsp.h"

CHle * g_hle = NULL;

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
Purpose:  This function is called when the emulator is closing
down allowing the DLL to de-initialize.
Input:    None
Output:   None
*/

void CloseDLL(void)
{
    if (g_hle)
    {
        delete g_hle;
        g_hle = NULL;
    }
}

/*
Function: DllAbout
Purpose:  This function is optional function that is provided
to give further information about the DLL.
Input:    A handle to the window that calls this function
Output:   None
*/

void DllAbout(void * hParent)
{
#ifdef _WIN32
	MessageBox((HWND)hParent, L"need to do", L"About", MB_OK | MB_ICONINFORMATION);
#endif
}

/*
Function: DoRspCycles
Purpose:  This function is to allow the RSP to run in parallel with
the r4300 switching control back to the r4300 once the
function ends.
Input:    The number of cycles that is meant to be executed
Output:   The number of cycles that was executed. This value can
be greater than the number of cycles that the RSP
should have performed.
(this value is ignored if the RSP is stopped)
*/

uint32_t DoRspCycles(uint32_t Cycles)
{
    if (g_hle)
    {
        g_hle->hle_execute();
    }
    return Cycles;
}

/*
Function: GetDllInfo
Purpose:  This function allows the emulator to gather information
about the DLL by filling in the PluginInfo structure.
Input:    A pointer to a PLUGIN_INFO structure that needs to be
filled by the function. (see def above)
Output:   None
*/

void GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = 0x0102;
    PluginInfo->Type = PLUGIN_TYPE_RSP;
#ifdef _DEBUG
    sprintf(PluginInfo->Name, "RSP HLE debug plugin %s", VER_FILE_VERSION_STR);
#else
    sprintf(PluginInfo->Name, "RSP HLE plugin %s", VER_FILE_VERSION_STR);
#endif
    PluginInfo->NormalMemory = false;
    PluginInfo->MemoryBswaped = true;
}

/*
Function: InitiateRSP
Purpose:  This function is called when the DLL is started to give
information from the emulator that the N64 RSP
interface needs
Input:    Rsp_Info is passed to this function which is defined
above.
CycleCount is the number of cycles between switching
control between the RSP and r4300i core.
Output:   None
*/

void InitiateRSP(RSP_INFO Rsp_Info, uint32_t * /*CycleCount*/)
{
    if (g_hle)
    {
        delete g_hle;
        g_hle = NULL;
    }
    g_hle = new CHle(Rsp_Info);
}

/*
Function: RomOpen
Purpose:  This function is called when a ROM is opened
Input:    None
Output:   None
*/

void RomOpen(void)
{
}

/*
Function: RomClosed
Purpose:  This function is called when a ROM is closed
Input:    None
Output:   None
*/

void RomClosed(void)
{
}

void PluginLoaded(void)
{
}
