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

/******************************************************************
  Function: CloseDLL
  Purpose:  This function is called when the emulator is closing
  down allowing the dll to de-initialise.
  input:    none
  output:   none
  *******************************************************************/
void CloseDLL(void)
{
    if (g_hle)
    {
        delete g_hle;
        g_hle = NULL;
    }
}

/******************************************************************
  Function: DllAbout
  Purpose:  This function is optional function that is provided
  to give further information about the DLL.
  input:    a handle to the window that calls this function
  output:   none
  *******************************************************************/
void DllAbout(void * hParent)
{
#ifdef _WIN32
	MessageBox((HWND)hParent, L"need to do", L"About", MB_OK | MB_ICONINFORMATION);
#endif
}

/******************************************************************
Function: DoRspCycles
Purpose:  This function is to allow the RSP to run in parrel with
the r4300 switching control back to the r4300 once the
function ends.
input:    The number of cylces that is meant to be executed
output:   The number of cycles that was executed. This value can
be greater than the number of cycles that the RSP
should have performed.
(this value is ignored if the RSP is stoped)
*******************************************************************/
uint32_t DoRspCycles(uint32_t Cycles)
{
    if (g_hle)
    {
        g_hle->hle_execute();
    }
    return Cycles;
}

/******************************************************************
  Function: GetDllInfo
  Purpose:  This function allows the emulator to gather information
  about the dll by filling in the PluginInfo structure.
  input:    a pointer to a PLUGIN_INFO stucture that needs to be
  filled by the function. (see def above)
  output:   none
  *******************************************************************/
void GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = 0x0102;
    PluginInfo->Type = PLUGIN_TYPE_RSP;
#ifdef _DEBUG
    sprintf(PluginInfo->Name, "RSP HLE Debug Plugin %s", VER_FILE_VERSION_STR);
#else
    sprintf(PluginInfo->Name, "RSP HLE Plugin %s", VER_FILE_VERSION_STR);
#endif
    PluginInfo->NormalMemory = false;
    PluginInfo->MemoryBswaped = true;
}

/******************************************************************
Function: InitiateRSP
Purpose:  This function is called when the DLL is started to give
information from the emulator that the n64 RSP
interface needs
input:    Rsp_Info is passed to this function which is defined
above.
CycleCount is the number of cycles between switching
control between teh RSP and r4300i core.
output:   none
*******************************************************************/
void InitiateRSP(RSP_INFO Rsp_Info, uint32_t * /*CycleCount*/)
{
    if (g_hle)
    {
        delete g_hle;
        g_hle = NULL;
    }
    g_hle = new CHle(Rsp_Info);
}

/******************************************************************
Function: RomOpen
Purpose:  This function is called when a rom is opened.
input:    none
output:   none
*******************************************************************/
void RomOpen(void)
{
}

/******************************************************************
Function: RomClosed
Purpose:  This function is called when a rom is closed.
input:    none
output:   none
*******************************************************************/
void RomClosed(void)
{
}

void PluginLoaded(void)
{
}