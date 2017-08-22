#pragma once

enum MEM_PROTECTION
{
    MEM_NOACCESS,
    MEM_READONLY,
    MEM_READWRITE,
    MEM_EXECUTE_READWRITE,
};

void* AllocateAddressSpace(size_t size, void * base_address = 0);
bool FreeAddressSpace(void* addr, size_t size);
void* CommitMemory(void* addr, size_t size, MEM_PROTECTION memProtection);
bool DecommitMemory(void* addr, size_t size);
bool ProtectMemory(void* addr, size_t size, MEM_PROTECTION memProtection, MEM_PROTECTION * OldProtect = NULL);
