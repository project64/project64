#pragma once
#include <stdafx.h>

class CDebugMMU
{
private:
    uint32_t* PAddrWordPtr(uint32_t paddr);
public:
    bool DebugLW_PAddr(uint32_t paddr, uint32_t& value);
    bool DebugLW_VAddr(uint32_t vaddr, uint32_t& value);
    bool DebugLB_PAddr(uint32_t paddr, uint8_t& value);
    bool DebugLB_VAddr(uint32_t vaddr, uint8_t& value);
    bool DebugSB_PAddr(uint32_t paddr, uint8_t value);
    bool DebugSB_VAddr(uint32_t vaddr, uint8_t value);
};