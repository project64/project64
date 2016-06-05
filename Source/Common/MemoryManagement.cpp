#include "stdafx.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include "MemoryManagement.h"

#ifdef _WIN32
static bool TranslateFromMemProtect(MEM_PROTECTION memProtection, int & OsMemProtection)
{
    switch (memProtection)
    {
    case MEM_NOACCESS: OsMemProtection = PAGE_NOACCESS; break;
    case MEM_READONLY: OsMemProtection = PAGE_READONLY; break;
    case MEM_READWRITE: OsMemProtection = PAGE_READWRITE; break;
    case MEM_EXECUTE_READWRITE: OsMemProtection = PAGE_EXECUTE_READWRITE; break;
    default:
        return false;
    }
    return true;
}

static bool TranslateToMemProtect(int OsMemProtection, MEM_PROTECTION & memProtection)
{
    switch (OsMemProtection)
    {
    case PAGE_NOACCESS: memProtection = MEM_NOACCESS; break;
    case PAGE_READONLY: memProtection = MEM_READONLY; break;
    case PAGE_READWRITE: memProtection = MEM_READWRITE; break;
    case PAGE_EXECUTE_READWRITE: memProtection = MEM_EXECUTE_READWRITE; break;
    default:
        return false;
    }
    return true;
}
#endif

void* AllocateAddressSpace(size_t size)
{
#ifdef _WIN32
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_TOP_DOWN, PAGE_NOACCESS);
#else
    void * ptr = mmap((void*)0, size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
#endif
}

bool FreeAddressSpace(void* addr, size_t size)
{
#ifdef _WIN32
    return VirtualFree(addr, 0, MEM_RELEASE) != 0;
#else
    msync(addr, size, MS_SYNC);
    munmap(addr, size);
#endif
}

void* CommitMemory(void* addr, size_t size, MEM_PROTECTION memProtection)
{
#ifdef _WIN32
    int OsMemProtection;
    if (!TranslateFromMemProtect(memProtection, OsMemProtection))
    {
        return NULL;
    }
    return VirtualAlloc(addr, size, MEM_COMMIT, OsMemProtection);
#else
    void * ptr = mmap(addr, size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED|MAP_ANON, -1, 0);
    msync(addr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
#endif
}

bool DecommitMemory(void* addr, size_t size)
{
#ifdef _WIN32
    return VirtualFree((void*)addr, size, MEM_DECOMMIT) != 0;
#else
    // instead of unmapping the address, we're just gonna trick 
    // the TLB to mark this as a new mapped area which, due to 
    // demand paging, will not be committed until used.
 
    mmap(addr, size, PROT_NONE, MAP_FIXED|MAP_PRIVATE|MAP_ANON, -1, 0);
    msync(addr, size, MS_SYNC|MS_INVALIDATE);
	
	return true;
#endif
}

bool ProtectMemory(void* addr, size_t size, MEM_PROTECTION memProtection, MEM_PROTECTION * OldProtect)
{
#ifdef _WIN32
    int OsMemProtection;
    if (!TranslateFromMemProtect(memProtection, OsMemProtection))
    {
        return NULL;
    }

    DWORD OldOsProtect;
    BOOL res = VirtualProtect(addr, size, OsMemProtection, &OldOsProtect);
    if (OldProtect != NULL)
    {
        if (!TranslateToMemProtect(OldOsProtect, *OldProtect))
        {
            return NULL;
        }
    }
    return res != 0;
#else
	return false;
#endif
}