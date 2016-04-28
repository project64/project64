#include "stdafx.h"
#ifdef _WIN
#include <windows.h>
#else
#include <sys/mman.h>
#include <map>
#endif
#include "MemoryManagement.h"


#ifndef _WIN32
std::map<void*,MEM_PROTECTION> mapAddrProtection; // Current protection flags of an address
#endif

static bool TranslateFromMemProtect(MEM_PROTECTION memProtection, int & OsMemProtection)
{
    switch (memProtection)
    {
        case MEM_NOACCESS:
        {
        #ifdef _WIN32
            OsMemProtection = PAGE_NOACCESS;
        #else
            OsMemProtection = PROT_NONE;
        #endif
            break;
        }
            
        case MEM_READONLY:
        {
        #ifdef _WIN32
            OsMemProtection = PAGE_READONLY;
        #else
            OsMemProtection = PROT_READ;
        #endif
            break;
        }
        case MEM_READWRITE:
        {
        #ifdef _WIN32
            OsMemProtection = PAGE_READWRITE;
        #else
            OsMemProtection = PROT_READ | PROT_WRITE;
        #endif
            break;
        }
            
        case MEM_EXECUTE_READWRITE:
        {
        #ifdef _WIN32
            OsMemProtection = PAGE_EXECUTE_READWRITE;
        #else
            OsMemProtection = PROT_READ | PROT_WRITE | PROT_EXEC;
        #endif
            break;
        }
    default:
        return false;
    }
    return true;
}

static bool TranslateToMemProtect(int OsMemProtection, MEM_PROTECTION & memProtection)
{
    switch (OsMemProtection)
    {
#ifdef _WIN32
    case PAGE_NOACCESS: memProtection = MEM_NOACCESS; break;
    case PAGE_READONLY: memProtection = MEM_READONLY; break;
    case PAGE_READWRITE: memProtection = MEM_READWRITE; break;
    case PAGE_EXECUTE_READWRITE: memProtection = MEM_EXECUTE_READWRITE; break;
#else
    case PROT_NONE: memProtection = MEM_NOACCESS; break;
    case PROT_READ: memProtection = MEM_READONLY; break;
    case (PROT_READ | PROT_WRITE): memProtection = MEM_READWRITE; break;
    case (PROT_READ | PROT_WRITE | PROT_EXEC): memProtection = MEM_EXECUTE_READWRITE; break;
#endif
    default:
        return false;
    }
    return true;
}

void* AllocateAddressSpace(size_t size)
{
#ifdef _WIN32
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_TOP_DOWN, PAGE_NOACCESS);
#else
    void * ptr = mmap((void*)NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(ptr == MAP_FAILED)
        return nullptr;
    
    msync(ptr, size, MS_SYNC | MS_INVALIDATE);
    mapAddrProtection[ptr] = MEM_NOACCESS;
    return ptr;
#endif
}

bool FreeAddressSpace(void* addr, size_t size)
{
#ifdef _WIN32
    return VirtualFree(addr, 0, MEM_RELEASE) != 0;
#else
    if(msync(addr, size, MS_SYNC) != 0)
        return false;
    
     if(munmap(addr, size) != 0)
         return false;
    
    mapAddrProtection.erase(addr);
    return true;
#endif
}

void* CommitMemory(void* addr, size_t size, MEM_PROTECTION memProtection)
{
    int OsMemProtection;
    if (!TranslateFromMemProtect(memProtection, OsMemProtection))
    {
        return nullptr;
    }
    
#ifdef _WIN32
    return VirtualAlloc(addr, size, MEM_COMMIT, OsMemProtection);
#else
    void * ptr = mmap(addr, size, OsMemProtection, MAP_FIXED | MAP_SHARED| MAP_ANONYMOUS, -1, 0);
    if(ptr == MAP_FAILED)
        return nullptr;
    
    msync(addr, size, MS_SYNC | MS_INVALIDATE);
    mapAddrProtection[addr] = memProtection;
    return ptr;
#endif
}

bool DecommitMemory(void* addr, size_t size)
{
#ifdef _WIN32
    return VirtualFree((void*)addr, size, MEM_DECOMMIT) != 0;
#else
    if(mmap(addr, size, PROT_NONE, MAP_FIXED | MAP_PRIVATE| MAP_ANONYMOUS, -1, 0) == MAP_FAILED)
        return false;
    
    mapAddrProtection[addr] = MEM_NOACCESS;
    return (msync(addr, size, MS_SYNC | MS_INVALIDATE) == 0);
#endif
}

bool ProtectMemory(void* addr, size_t size, MEM_PROTECTION memProtection, MEM_PROTECTION * OldProtect)
{
    int OsMemProtection;
    if (!TranslateFromMemProtect(memProtection, OsMemProtection))
    {
        return false;
    }

    bool res = false;
#ifdef _WIN32
    DWORD OldOsProtect;
    res = VirtualProtect(addr, size, OsMemProtection, &OldOsProtect) !=0;
    if (OldProtect != NULL)
    {
        if (!TranslateToMemProtect(OldOsProtect, *OldProtect))
        {
            return false;
        }
    }
#else
    res = mprotect(addr, size, OsMemProtection) != 0;
    if (OldProtect != NULL)
    {
        *OldProtect = mapAddrProtection[addr];        
    }
    mapAddrProtection[addr] = memProtection;
#endif
    return res;
}