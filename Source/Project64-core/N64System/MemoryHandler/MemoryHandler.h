#pragma once
#include <stdint.h>

__interface MemoryHandler
{
    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);
};