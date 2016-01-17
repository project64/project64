#include "stdafx.h"
#include <windows.h>
#include "MemoryManagement.h"

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

void* AllocateAddressSpace(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_TOP_DOWN, PAGE_NOACCESS);
}

bool FreeAddressSpace(void* addr, size_t size)
{
    return VirtualFree(addr, 0, MEM_RELEASE) != 0;
}

void* CommitMemory(void* addr, size_t size, MEM_PROTECTION memProtection)
{
    int OsMemProtection;
    if (!TranslateFromMemProtect(memProtection, OsMemProtection))
    {
        return NULL;
    }
    return VirtualAlloc(addr, size, MEM_COMMIT, OsMemProtection);
}

bool DecommitMemory(void* addr, size_t size)
{
    return VirtualFree((void*)addr, size, MEM_DECOMMIT) != 0;
}

bool ProtectMemory(void* addr, size_t size, MEM_PROTECTION memProtection, MEM_PROTECTION * OldProtect)
{
    int OsMemProtection;
    if (!TranslateFromMemProtect(memProtection, OsMemProtection))
    {
        return NULL;
    }

    DWORD OldOsProtect;
    BOOL res = VirtualProtect(addr, size, OsMemProtection, &OldOsProtect);
    if (OldProtect != NULL)
    {
    }
    return res != 0;
}