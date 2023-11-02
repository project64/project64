// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2012 Bobby Smiles
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"

// Global functions
void load_u8(uint8_t * dst, const unsigned char * buffer, unsigned address, size_t count)
{
    while (count != 0)
    {
        *(dst++) = *u8(buffer, address);
        address += 1;
        --count;
    }
}

void store_u16(unsigned char * buffer, unsigned address, const uint16_t * src, size_t count)
{
    while (count != 0)
    {
        *u16(buffer, address) = *(src++);
        address += 2;
        --count;
    }
}

void load_u32(uint32_t * dst, const unsigned char * buffer, unsigned address, size_t count)
{
    // Optimization for uint32_t
    memcpy(dst, u32(buffer, address), count * sizeof(uint32_t));
}

void load_u16(uint16_t * dst, const unsigned char * buffer, unsigned address, size_t count)
{
    while (count != 0)
    {
        *(dst++) = *u16(buffer, address);
        address += 2;
        --count;
    }
}

void store_u32(unsigned char * buffer, unsigned address, const uint32_t * src, size_t count)
{
    // Optimization for uint32_t
    memcpy(u32(buffer, address), src, count * sizeof(uint32_t));
}
