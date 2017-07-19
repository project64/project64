#include "stdafx.h"
#include "Util.h"
#ifdef _WIN32
#include <windows.h>
#include <Tlhelp32.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#endif

pjutil::DynLibHandle pjutil::DynLibOpen(const char *pccLibraryPath, bool ShowErrors)
{
    if (pccLibraryPath == NULL)
    {
        return NULL;
    }
#ifdef _WIN32
    UINT LastErrorMode = SetErrorMode(ShowErrors ? 0 : SEM_FAILCRITICALERRORS);
    pjutil::DynLibHandle lib = (pjutil::DynLibHandle)LoadLibrary(pccLibraryPath);
    SetErrorMode(LastErrorMode);
#else
    pjutil::DynLibHandle lib = (pjutil::DynLibHandle)dlopen(pccLibraryPath, RTLD_NOW);
#endif
    return lib;
}

void * pjutil::DynLibGetProc(pjutil::DynLibHandle LibHandle, const char * ProcedureName)
{
    if (ProcedureName == NULL)
        return NULL;

#ifdef _WIN32
    return GetProcAddress((HMODULE)LibHandle, ProcedureName);
#else
    return dlsym(LibHandle, ProcedureName);
#endif
}

void pjutil::DynLibClose(pjutil::DynLibHandle LibHandle)
{
    if (LibHandle != NULL)
    {
#ifdef _WIN32
        FreeLibrary((HMODULE)LibHandle);
#else
        dlclose(LibHandle);
#endif
    }
}

void pjutil::Sleep(uint32_t timeout)
{
#ifdef _WIN32
    ::Sleep(timeout);
#else
    int was_error;
    struct timespec elapsed, tv;
    elapsed.tv_sec = timeout / 1000;
    elapsed.tv_nsec = (timeout % 1000) * 1000000;
    do {
        errno = 0;
        tv.tv_sec = elapsed.tv_sec;
        tv.tv_nsec = elapsed.tv_nsec;
        was_error = nanosleep(&tv, &elapsed);
    } while (was_error && (errno == EINTR));
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
                        WaitForSingleObject(hHandle, 30 * 1000);
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
