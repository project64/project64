#include "Util.h"
#include "StdString.h"
#include "path.h"
#ifdef _WIN32
#include <windows.h>
#include <Tlhelp32.h>
#else
#include <time.h>
#endif

static bool IsWinVistaOrLater()
{
#ifdef _WIN32
    return GetProcAddress(LoadLibrary(TEXT("KERNEL32")), "CancelIoEx") != nullptr;
#else
    return false;
#endif
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
        DWORD processAllAccess = IsWinVistaOrLater() ? (PROCESS_ALL_ACCESS) : (PROCESS_ALL_ACCESS & ~0xF000);
        PROCESSENTRY32 lppe;

        memset(&lppe, 0, sizeof(PROCESSENTRY32));
        lppe.dwSize = sizeof(PROCESSENTRY32);
        stdstr ModuleName = CPath(CPath::MODULE_FILE).GetNameExtension();

        if (Process32First(nSearch, &lppe))
        {
            do
            {
                if (_wcsicmp(lppe.szExeFile, ModuleName.ToUTF16().c_str()) != 0 ||
                    lppe.th32ProcessID == pid)
                {
                    continue;
                }
                if (!AskedUser)
                {
                    stdstr_f Message("%s currently running\n\nTerminate PID %d now?", ModuleName.c_str(), lppe.th32ProcessID);
                    stdstr_f Caption("Terminate %s", ModuleName.c_str());

                    AskedUser = true;
                    int res = MessageBox(nullptr, Message.ToUTF16().c_str(), Caption.ToUTF16().c_str(), MB_YESNO | MB_ICONEXCLAMATION);
                    if (res != IDYES)
                    {
                        break;
                    }
                }
                HANDLE hHandle = OpenProcess(processAllAccess, FALSE, lppe.th32ProcessID);
                if (hHandle != nullptr)
                {
                    if (TerminateProcess(hHandle, 0))
                    {
                        bTerminated = true;
                        WaitForSingleObject(hHandle, 30 * 1000);
                    }
                    else
                    {
                        stdstr_f Message("Failed to terminate PID %d", lppe.th32ProcessID);
                        stdstr_f Caption("Terminate %s failed!", ModuleName.c_str());
                        MessageBox(nullptr, Message.ToUTF16().c_str(), Caption.ToUTF16().c_str(), MB_YESNO | MB_ICONEXCLAMATION);
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
