#include "stdafx.h"
#include "Util.h"
#ifdef _WIN32
#include <windows.h>
#include <Tlhelp32.h>
#else
#include <unistd.h>
#endif

pjutil::DynLibHandle pjutil::DynLibOpen(const char *pccLibraryPath, bool ShowErrors)
{
    if (pccLibraryPath == NULL)
    {
        return NULL;
    }
    UINT LastErrorMode = SetErrorMode(ShowErrors ? 0 : SEM_FAILCRITICALERRORS);
    pjutil::DynLibHandle lib = (pjutil::DynLibHandle)LoadLibrary(pccLibraryPath);
    SetErrorMode(LastErrorMode);
    return lib;
}

void * pjutil::DynLibGetProc(pjutil::DynLibHandle LibHandle, const char * ProcedureName)
{
    if (ProcedureName == NULL)
        return NULL;

    return GetProcAddress((HMODULE)LibHandle, ProcedureName);
}

void pjutil::DynLibClose(pjutil::DynLibHandle LibHandle)
{
    FreeLibrary((HMODULE)LibHandle);
}

#ifdef _WIN32
static void EmptyThreadFunction(void)
{
}

void pjutil::DynLibCallDllMain(void)
{
    //jabo had a bug so I call CreateThread so the dllmain in the plugins will get called again with thread attached
    DWORD ThreadID;
    HANDLE hthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)EmptyThreadFunction, NULL, 0, &ThreadID);
    CloseHandle(hthread);
}
#endif

void pjutil::Sleep(uint32_t timeout)
{
#ifdef _WIN32
    ::Sleep(timeout);
#else
	sleep(timeout);
#endif
}

#ifdef _WIN32
bool pjutil::TerminatedExistingExe()
{
    bool bTerminated = false;
    bool AskedUser = false;
    DWORD pid = GetCurrentProcessId();

    HANDLE nSearch = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (nSearch != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 lppe;

        memset(&lppe, 0, sizeof(PROCESSENTRY32));
        lppe.dwSize = sizeof(PROCESSENTRY32);
        stdstr ModuleName = CPath(CPath::MODULE_FILE).GetNameExtension();

        if (Process32First(nSearch, &lppe))
        {
            do
            {
                if (_stricmp(lppe.szExeFile, ModuleName.c_str()) != 0 ||
                    lppe.th32ProcessID == pid)
                {
                    continue;
                }
                if (!AskedUser)
                {
                    AskedUser = true;
                    int res = MessageBox(NULL, stdstr_f("%s currently running\n\nTerminate pid %d now?", ModuleName.c_str(), lppe.th32ProcessID).c_str(), stdstr_f("Terminate %s",ModuleName.c_str()).c_str(), MB_YESNO | MB_ICONEXCLAMATION);
                    if (res != IDYES)
                    {
                        break;
                    }
                }
                HANDLE hHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, lppe.th32ProcessID);
                if (hHandle != NULL)
                {
                    if (TerminateProcess(hHandle, 0))
                    {
                        bTerminated = true;
                    }
                    else
                    {
                        MessageBox(NULL, stdstr_f("Failed to terminate pid %d", lppe.th32ProcessID).c_str(), stdstr_f("Terminate %s failed!",ModuleName.c_str()).c_str(), MB_YESNO | MB_ICONEXCLAMATION);
                    }
                    CloseHandle(hHandle);
                }
            } while (Process32Next(nSearch, &lppe));
        }
        CloseHandle(nSearch);
    }
    return bTerminated;
}
#endif
