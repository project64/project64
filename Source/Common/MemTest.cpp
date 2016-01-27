#include "stdafx.h"

#if defined(_MSC_VER)
#include <crtdbg.h>
#else
#define _ASSERTE(expr)          ((void)0)
#endif

#ifdef _DEBUG

#pragma warning(disable:4786)	//Disable std library warning
#pragma warning(disable:4530)	//Disable std library warning
#include <string>
#include <malloc.h>

#include "MemTest.h"
#ifdef MEM_LEAK_TEST

#include <shellapi.h>                //Needed for ShellExecute
#pragma comment(lib, "shell32.lib")  //Needed for ShellExecute
#undef new
#undef malloc
#undef realloc
#undef free
#undef VirtualAlloc
#undef VirtualFree

#ifndef MB_SERVICE_NOTIFICATION
#define MB_SERVICE_NOTIFICATION          0x00200000L
#endif

CMemList *MemList(void)
{
    static CMemList m_MemList;

    return &m_MemList;
}

CMemList::CMemList()
{
    MemList.clear();
    hSemaphone = CreateSemaphore(NULL, 1, 1, NULL);
    State = Initialized;
    order = 0;
    LogAllocate = false;
    ThreadID = 0;
    m_hModule = NULL;

    for (UINT_PTR TestLoc = ((UINT_PTR)::MemList) & ~0xFFF; TestLoc != 0; TestLoc -= 0x1000)
    {
        WORD HeaderID = *(WORD *)TestLoc;
        if (HeaderID != 'ZM')
        {
            continue;
        }
        m_hModule = (HMODULE)TestLoc;
        break;
    }
}

CMemList::~CMemList()
{
    size_t ItemsLeft = MemList.size();
    if (ItemsLeft > 0)
    {
		char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
		char fname[_MAX_FNAME], ext[_MAX_EXT], LogFileName[_MAX_PATH];

        memset(path_buffer, 0, sizeof(path_buffer));
        memset(drive, 0, sizeof(drive));
        memset(dir, 0, sizeof(dir));
        memset(fname, 0, sizeof(fname));
        memset(ext, 0, sizeof(ext));
        memset(LogFileName, 0, sizeof(LogFileName));

        GetModuleFileName(m_hModule, path_buffer, sizeof(path_buffer));
        _splitpath(path_buffer, drive, dir, fname, ext);

        _makepath(LogFileName, drive, dir, fname, "leak.csv");

        HANDLE hLogFile = INVALID_HANDLE_VALUE;
        do
        {
            hLogFile = CreateFile(
                LogFileName,
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
                );
            if (hLogFile == INVALID_HANDLE_VALUE)
            {
                if (GetLastError() == ERROR_SHARING_VIOLATION)
                {
					TCHAR Msg[3000];
                    sprintf(Msg, "%s\nCan not be opened for writing please close app using this file\n\nTry Again ?", LogFileName);
                    int Result = MessageBox(NULL, Msg, "Memory Leak", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION);
                    if (Result == IDNO)
                    {
                        break;
                    }
                }
            }
        } while (hLogFile == INVALID_HANDLE_VALUE);

        if (hLogFile != INVALID_HANDLE_VALUE)
        {
            SetFilePointer(hLogFile, 0, NULL, FILE_BEGIN);

            DWORD dwWritten = 0;
            char  Msg[800];
            _snprintf(Msg, sizeof(Msg), "Order, Source File, Line Number, Mem Size\r\n");
            WriteFile(hLogFile, Msg, (DWORD)strlen(Msg), &dwWritten, NULL);

            for (MEMLIST_ITER item = MemList.begin(); item != MemList.end(); item++)
            {
                _snprintf(Msg, sizeof(Msg), "%d,%s, %d, %d\r\n",
                    (*item).second.order,
                    (*item).second.File,
                    (*item).second.line,
                    (*item).second.size);
                WriteFile(hLogFile, Msg, (DWORD)strlen(Msg), &dwWritten, NULL);
            }
            CloseHandle(hLogFile);
        }
        char Msg[3000];
        sprintf(Msg, "%s%s\n\nMemory Leaks detected\n\nOpen the Log File ?", fname, ext);
        int Result = MessageBox(NULL, Msg, "Memory Leak", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION);
        if (Result == IDYES)
        {
            ShellExecute(NULL, "open", LogFileName, NULL, NULL, SW_SHOW);
        }
    }
    CloseHandle(hSemaphone);
    hSemaphone = NULL;
    State = NotInitialized;
}

void * CMemList::AddItem(size_t size, char * filename, int line)
{
    void *res = malloc(size);
    if (res == NULL)
    {
        return res;
    }
    RecordAddItem(res, size, filename, line);
    return res;
}

void CMemList::RecordAddItem(void * ptr, size_t size, const char * filename, int line)
{
    __try
    {
        if (State == Initialized && hSemaphone != NULL)
        {
            DWORD CurrentThread = GetCurrentThreadId();
            DWORD Result = WaitForSingleObject(hSemaphone, CurrentThread != ThreadID ? 30000 : 0);
            if (Result != WAIT_TIMEOUT)
            {
                ThreadID = CurrentThread;

                DEBUG_LOCATION info;
                strncpy(info.File, filename, sizeof(info.File));
                info.line = line;
                info.size = (int)size;
                info.order = order++;

                Insert(ptr, info);

                long dwSemCount = 0;
                ThreadID = (DWORD)-1;
                ReleaseSemaphore(hSemaphone, 1, &dwSemCount);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //_asm int 3
    }
}

void CMemList::Insert(void *res, DEBUG_LOCATION &info)
{
    MemList.insert(MEMLIST::value_type(res, info));
}

void * CMemList::ReAllocItem(void * ptr, size_t size, const char * filename, int line)
{
    void *res = realloc(ptr, size);
    if (res == NULL)
    {
        return res;
    }
    if (ptr != res)
    {
        __try
        {
            if (State == Initialized && hSemaphone != NULL)
            {
                DWORD CurrentThread = GetCurrentThreadId();
                DWORD Result = WaitForSingleObject(hSemaphone, CurrentThread != ThreadID ? 30000 : 0);
                if (Result != WAIT_TIMEOUT)
                {
                    ThreadID = CurrentThread;
                    //Add new pointer
                    DEBUG_LOCATION info;
                    strncpy(info.File, filename, sizeof(info.File));
                    info.line = line;
                    info.size = (int)size;
                    info.order = order++;

                    Insert(res, info);

                    //remove old pointer
                    Remove(ptr);

                    long dwSemCount = 0;
                    ThreadID = (DWORD)-1;
                    ReleaseSemaphore(hSemaphone, 1, &dwSemCount);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //_asm int 3
        }
    }
    return res;
}

void CMemList::Remove(void *ptr)
{
    //remove old pointer
    MEMLIST_ITER item = MemList.find(ptr);
    if (item != MemList.end())
    {
        MemList.erase(ptr);
    }
}

void CMemList::removeItem(void * ptr, bool bFree)
{
    if (bFree)
    {
        free(ptr);
    }

    __try
    {
        if (State == Initialized && hSemaphone != NULL)
        {
            DWORD CurrentThread = GetCurrentThreadId();
            DWORD Result = WaitForSingleObject(hSemaphone, CurrentThread != ThreadID ? 30000 : 0);
            if (Result != WAIT_TIMEOUT)
            {
                ThreadID = CurrentThread;

                Remove(ptr);

                long dwSemCount = 0;
                ThreadID = (DWORD)-1;
                ReleaseSemaphore(hSemaphone, 1, &dwSemCount);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //_asm int 3
    }
}

void  MemTest_AddLeak(char* Comment)
{
    MemList()->AddItem(1, Comment, -1);
}

void* MemTest_malloc(size_t size, char* filename, int line)
{
    return MemList()->AddItem(size, filename, line);
}

void* MemTest_realloc(void* ptr, size_t size, char* filename, int line)
{
    return MemList()->ReAllocItem(ptr, size, filename, line);
}

void* operator new (size_t size, char* filename, int line)
{
    return MemList()->AddItem(size, filename, line);
}

void* operator new (size_t size)
{
    return MemList()->AddItem(size, "Unknown", 0);
}

void* operator new [](size_t size, char* filename, int line)
{
    return MemList()->AddItem(size, filename, line);
}

void* operator new [](size_t size)
{
    return MemList()->AddItem(size, "Unknown", 0);
}

void operator delete (void* ptr)
{
    MemList()->removeItem(ptr, true);
}

void operator delete[](void* ptr)
{
    delete ptr;
}

LPVOID MemTest_VirtualAlloc(
LPVOID lpAddress,        // region to reserve or commit
SIZE_T dwSize,           // size of region
DWORD flAllocationType,  // type of allocation
DWORD flProtect,          // type of access protection
LPCSTR filename,
int line)
{
    LPVOID ptr = VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);

    if (ptr && lpAddress == NULL && (flAllocationType & MEM_RESERVE) != 0)
    {
        MemList()->RecordAddItem(ptr, dwSize, filename, line);
    }
    return ptr;
}

BOOL MemTest_VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
    if ((dwFreeType & MEM_RELEASE) != 0)
    {
        MemList()->removeItem(lpAddress, false);
    }
    return VirtualFree(lpAddress, dwSize, dwFreeType);
}

#endif
#endif