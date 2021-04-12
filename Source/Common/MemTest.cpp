#include "MemTest.h"
#include <map>

#if defined(MEM_LEAK_TEST) && defined(_WIN32)

#undef new

#include <Windows.h>
#include <shellapi.h>

static bool InInit = false;

class CMemList 
{
    typedef struct 
    {
        char File[300];
        int line;
        int size;
        int order;
    } DEBUG_LOCATION;

    typedef std::map<void *, DEBUG_LOCATION> MEMLIST;
    typedef MEMLIST::const_iterator MEMLIST_ITER;

public:
    CMemList();
    ~CMemList();
    
    void removeItem(void * ptr);
    void RecordAddItem(void * ptr, size_t size, const char * filename, int line);
    void DumpItems(void);

private:
    MEMLIST * m_MemList;
    HMODULE m_hModule;
    CRITICAL_SECTION m_cs;
    uint32_t m_NextOrder;
};

CMemList *MemList(void)
{
    static CMemList m_MemList;

    return &m_MemList;
}

CMemList::CMemList() :
    m_MemList(nullptr),
    m_hModule(nullptr),
    m_cs({0}),
    m_NextOrder(1)
{
    InInit = true;
    InitializeCriticalSection(&m_cs);
    m_MemList = new MEMLIST;

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
    InInit = false;
}

CMemList::~CMemList()
{
    InInit = true;
    size_t ItemsLeft = m_MemList->size();
    if (ItemsLeft > 0)
    {
        DumpItems();
    }
    delete m_MemList;
    m_MemList = nullptr;
    DeleteCriticalSection(&m_cs);
}

void CMemList::RecordAddItem(void * ptr, size_t size, const char * filename, int line)
{
    EnterCriticalSection(&m_cs);
    if (m_cs.RecursionCount > 1)
    {
        LeaveCriticalSection(&m_cs);
        return;
    }

    DEBUG_LOCATION info;
    strncpy(info.File, filename, sizeof(info.File));
    info.line = line;
    info.size = (int)size;
    info.order = m_NextOrder++;
    m_MemList->insert(MEMLIST::value_type(ptr, info));
    LeaveCriticalSection(&m_cs);
}

void CMemList::removeItem(void * ptr)
{
    EnterCriticalSection(&m_cs);
    if (m_cs.RecursionCount > 1)
    {
        LeaveCriticalSection(&m_cs);
        return;
    }
    MEMLIST_ITER item = m_MemList->find(ptr);
    if (item != m_MemList->end())
    {
        m_MemList->erase(item);
    }

    LeaveCriticalSection(&m_cs);
}

void CMemList::DumpItems(void)
{
    char path_buffer[_MAX_PATH] = { 0 }, drive[_MAX_DRIVE] = { 0 }, dir[_MAX_DIR] = { 0 };
    char fname[_MAX_FNAME] = { 0 }, ext[_MAX_EXT] = { 0 }, LogFileName[_MAX_PATH] = { 0 };

    GetModuleFileNameA(m_hModule, path_buffer, sizeof(path_buffer));
    _splitpath(path_buffer, drive, dir, fname, ext);
    _makepath(LogFileName, drive, dir, fname, "leak.csv");

    HANDLE hLogFile = INVALID_HANDLE_VALUE;
    do
    {
        hLogFile = CreateFileA( LogFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr );
        if (hLogFile == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_SHARING_VIOLATION)
            {
                char Msg[3000];
                sprintf(Msg, "%s\nCan not be opened for writing please close app using this file\n\nTry Again ?", LogFileName);
                int Result = MessageBoxA(nullptr, Msg, "Memory Leak", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION);
                if (Result == IDNO)
                {
                    break;
                }
            }
        }
    } while (hLogFile == INVALID_HANDLE_VALUE);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hLogFile, 0, nullptr, FILE_BEGIN);

        DWORD dwWritten = 0;
        char  Msg[800];
        _snprintf(Msg, sizeof(Msg), "Order, Source File, Line Number, Mem Size\r\n");
        WriteFile(hLogFile, Msg, (DWORD)strlen(Msg), &dwWritten, nullptr);

        for (MEMLIST_ITER item = m_MemList->begin(); item != m_MemList->end(); item++)
        {
            _snprintf(Msg, sizeof(Msg), "%d,%s, %d, %d\r\n", (*item).second.order, (*item).second.File, (*item).second.line, (*item).second.size);
            WriteFile(hLogFile, Msg, (DWORD)strlen(Msg), &dwWritten, nullptr);
        }
        CloseHandle(hLogFile);
    }
    char Msg[3000];
    sprintf(Msg, "%s%s\n\nMemory Leaks detected\n\nOpen the Log File ?", fname, ext);
    int Result = MessageBoxA(nullptr, Msg, "Memory Leak", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION);
    if (Result == IDYES)
    {
        ShellExecuteA(nullptr, "open", LogFileName, nullptr, nullptr, SW_SHOW);
    }
}

void* AllocateMemory(size_t size, const char* filename, unsigned int line)
{
    void * res = malloc(size);
    if (res == nullptr)
    {
        return res;
    }
    if (!InInit)
    {
        MemList()->RecordAddItem(res, size, filename, line);
    }
    return res;
}

void* operator new (size_t size, const char* filename, unsigned int line)
{
    return AllocateMemory(size, filename, line);
}

void* operator new[] (size_t size, const char* filename, unsigned int line)
{
    return AllocateMemory(size, filename, line);
}

void* operator new (size_t size)
{
    return AllocateMemory(size, "Unknown", 0);
}

void operator delete (void* ptr)
{
    free(ptr);
    if (!InInit)
    {
        MemList()->removeItem(ptr);
    }
}

void operator delete[](void* ptr)
{
    delete ptr;
}

void operator delete (void* ptr, const char* /*filename*/, unsigned int /*line*/)
{
    delete ptr;
}

void operator delete[](void* ptr, const char* /*filename*/, unsigned int /*line*/)
{
    delete ptr;
}

#endif
