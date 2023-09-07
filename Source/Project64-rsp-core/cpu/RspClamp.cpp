#include "RspClamp.h"

uint16_t clamp16(int32_t Value)
{
    if (Value > 0x7FFF)
    {
        return 0x7FFF;
    }
    if (Value < (int32_t)0xffff8000)
    {
        return 0x8000;
    }
    return (uint16_t)Value;
}

int64_t clip48(uint64_t Value)
{
    enum : uint64_t
    {
        b = 1ull << (48 - 1),
        m = b * 2 - 1
    };
    return ((Value & m) ^ b) - b;
}