// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2014 Bobby Smiles
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#pragma once
#include <assert.h>

#define S 1
#define S16 2
#define S8 3

enum
{
    TASK_TYPE = 0xfc0,
    TASK_FLAGS = 0xfc4,
    TASK_UCODE_BOOT = 0xfc8,
    TASK_UCODE_BOOT_SIZE = 0xfcc,
    TASK_UCODE = 0xfd0,
    TASK_UCODE_SIZE = 0xfd4,
    TASK_UCODE_DATA = 0xfd8,
    TASK_UCODE_DATA_SIZE = 0xfdc,
    TASK_DRAM_STACK = 0xfe0,
    TASK_DRAM_STACK_SIZE = 0xfe4,
    TASK_OUTPUT_BUFF = 0xfe8,
    TASK_OUTPUT_BUFF_SIZE = 0xfec,
    TASK_DATA_PTR = 0xff0,
    TASK_DATA_SIZE = 0xff4,
    TASK_YIELD_DATA_PTR = 0xff8,
    TASK_YIELD_DATA_SIZE = 0xffc
};

static inline unsigned int align(unsigned int x, unsigned amount)
{
    --amount;
    return (x + amount) & ~amount;
}

static inline uint8_t* u8(const unsigned char* buffer, unsigned address)
{
    return (uint8_t*)(buffer + (address ^ S8));
}

static inline uint16_t* u16(const unsigned char* buffer, unsigned address)
{
    assert((address & 1) == 0);
    return (uint16_t*)(buffer + (address ^ S16));
}

static inline uint32_t* u32(const unsigned char* buffer, unsigned address)
{
    assert((address & 3) == 0);
    return (uint32_t*)(buffer + address);
}

void load_u8 (uint8_t*  dst, const unsigned char* buffer, unsigned address, size_t count);
void load_u16(uint16_t* dst, const unsigned char* buffer, unsigned address, size_t count);
void load_u32(uint32_t* dst, const unsigned char* buffer, unsigned address, size_t count);
void store_u16(unsigned char* buffer, unsigned address, const uint16_t* src, size_t count);
void store_u32(unsigned char* buffer, unsigned address, const uint32_t* src, size_t count);

static inline uint32_t* dmem_u32(CHle * hle, uint16_t address)
{
    return u32(hle->dmem(), address & 0xfff);
}

static inline void dmem_store_u32(CHle * hle, const uint32_t* src, uint16_t address, size_t count)
{
    store_u32(hle->dmem(), address & 0xfff, src, count);
}

// Convenient functions DRAM access
static inline uint8_t* dram_u8(CHle * hle, uint32_t address)
{
    return u8(hle->dram(), address & 0xffffff);
}

static inline uint16_t* dram_u16(CHle * hle, uint32_t address)
{
    return u16(hle->dram(), address & 0xffffff);
}

static inline uint32_t* dram_u32(CHle * hle, uint32_t address)
{
    return u32(hle->dram(), address & 0xffffff);
}

static inline void dram_load_u8(CHle * hle, uint8_t* dst, uint32_t address, size_t count)
{
    load_u8(dst, hle->dram(), address & 0xffffff, count);
}

static inline void dram_load_u16(CHle * hle, uint16_t* dst, uint32_t address, size_t count)
{
    load_u16(dst, hle->dram(), address & 0xffffff, count);
}

static inline void dram_load_u32(CHle * hle, uint32_t* dst, uint32_t address, size_t count)
{
    load_u32(dst, hle->dram(), address & 0xffffff, count);
}

static inline void dram_store_u16(CHle * hle, const uint16_t* src, uint32_t address, size_t count)
{
    store_u16(hle->dram(), address & 0xffffff, src, count);
}

static inline void dram_store_u32(CHle * hle, const uint32_t* src, uint32_t address, size_t count)
{
    store_u32(hle->dram(), address & 0xffffff, src, count);
}
