#include <stdint.h>

uint32_t __builtin_clz(uint32_t val)
{
    /* Binary search for the leading one bit.  */
    int cnt = 0;

    if ((val & 0xFFFF0000U) == 0)
    {
        cnt += 16;
        val <<= 16;
    }
    if ((val & 0xFF000000U) == 0)
    {
        cnt += 8;
        val <<= 8;
    }
    if ((val & 0xF0000000U) == 0)
    {
        cnt += 4;
        val <<= 4;
    }
    if ((val & 0xC0000000U) == 0)
    {
        cnt += 2;
        val <<= 2;
    }
    if ((val & 0x80000000U) == 0)
    {
        cnt++;
        val <<= 1;
    }
    if ((val & 0x80000000U) == 0)
    {
        cnt++;
    }
    return cnt;
}

uint32_t __builtin_clzll(uint64_t val)
{
    int cnt = 0;

    if ((val >> 32) == 0)
    {
        cnt += 32;
    }
    else
    {
        val >>= 32;
    }

    return cnt + __builtin_clz((uint32_t)val);
}
