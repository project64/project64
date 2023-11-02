// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2014 Bobby Smiles
// Copyright(C) 2009 Richard Goedeken
// Copyright(C) 2002 Hacktarux
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#include <stdint.h>
#include <string.h>

#include "alist.h"
#include "hle.h"
#include "mem.h"
#include "ucodes.h"

enum
{
    DMEM_BASE = 0x5c0
};

// Helper functions

static uint32_t get_address(CHle * hle, uint32_t so)
{
    return alist_get_address(hle, so, hle->alist_audio().segments, N_SEGMENTS);
}

static void set_address(CHle * hle, uint32_t so)
{
    alist_set_address(hle, so, hle->alist_audio().segments, N_SEGMENTS);
}

static void clear_segments(CHle * hle)
{
    memset(hle->alist_audio().segments, 0, N_SEGMENTS * sizeof(hle->alist_audio().segments[0]));
}

// Audio commands definition

static void SPNOOP(CHle * UNUSED(hle), uint32_t UNUSED(w1), uint32_t UNUSED(w2))
{
}

static void CLEARBUFF(CHle * hle, uint32_t w1, uint32_t w2)
{
    uint16_t dmem = (w1 + DMEM_BASE) & 0xFFFF;
    uint16_t count = w2;

    if (count == 0)
        return;

    alist_clear(hle, dmem, align(count, 16));
}

static void ENVMIXER(CHle * hle, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16) & 0xFF;
    uint32_t address = get_address(hle, w2);

    alist_envmix_exp(
        hle,
        flags & A_INIT,
        flags & A_AUX,
        hle->alist_audio().out, hle->alist_audio().dry_right,
        hle->alist_audio().wet_left, hle->alist_audio().wet_right,
        hle->alist_audio().in, hle->alist_audio().count,
        hle->alist_audio().dry, hle->alist_audio().wet,
        hle->alist_audio().vol,
        hle->alist_audio().target,
        hle->alist_audio().rate,
        address);
}

static void ENVMIXER_GE(CHle * hle, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16);
    uint32_t address = get_address(hle, w2);

    alist_envmix_ge(
        hle,
        flags & A_INIT,
        flags & A_AUX,
        hle->alist_audio().out, hle->alist_audio().dry_right,
        hle->alist_audio().wet_left, hle->alist_audio().wet_right,
        hle->alist_audio().in, hle->alist_audio().count,
        hle->alist_audio().dry, hle->alist_audio().wet,
        hle->alist_audio().vol,
        hle->alist_audio().target,
        hle->alist_audio().rate,
        address);
}

static void RESAMPLE(CHle * hle, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16) & 0xFF;
    uint16_t pitch = w1 & 0xFFFF;
    uint32_t address = get_address(hle, w2);

    alist_resample(
        hle,
        flags & 0x1,
        flags & 0x2,
        hle->alist_audio().out,
        hle->alist_audio().in,
        align(hle->alist_audio().count, 16),
        pitch << 1,
        address);
}

static void SETVOL(CHle * hle, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16) & 0xFF;

    if (flags & A_AUX)
    {
        hle->alist_audio().dry = w1 & 0xFFFF;
        hle->alist_audio().wet = w2 & 0xFFFF;
    }
    else
    {
        unsigned lr = (flags & A_LEFT) ? 0 : 1;

        if (flags & A_VOL)
        {
            hle->alist_audio().vol[lr] = w1 & 0xFFFF;
        }
        else
        {
            hle->alist_audio().target[lr] = w1 & 0xFFFF;
            hle->alist_audio().rate[lr] = w2;
        }
    }
}

static void SETLOOP(CHle * hle, uint32_t UNUSED(w1), uint32_t w2)
{
    hle->alist_audio().loop = get_address(hle, w2);
}

static void ADPCM(CHle * hle, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16) & 0xFF;
    uint32_t address = get_address(hle, w2);

    alist_adpcm(
        hle,
        flags & 0x1,
        flags & 0x2,
        false, // Unsupported in this microcode
        hle->alist_audio().out,
        hle->alist_audio().in,
        align(hle->alist_audio().count, 32),
        hle->alist_audio().table,
        hle->alist_audio().loop,
        address);
}

static void LOADBUFF(CHle * hle, uint32_t UNUSED(w1), uint32_t w2)
{
    uint32_t address = get_address(hle, w2);

    if (hle->alist_audio().count == 0)
    {
        return;
    }

    alist_load(hle, hle->alist_audio().in, address, hle->alist_audio().count);
}

static void SAVEBUFF(CHle * hle, uint32_t UNUSED(w1), uint32_t w2)
{
    uint32_t address = get_address(hle, w2);

    if (hle->alist_audio().count == 0)
    {
        return;
    }
    alist_save(hle, hle->alist_audio().out, address, hle->alist_audio().count);
}

static void SETBUFF(CHle * hle, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16) & 0xFF;

    if (flags & A_AUX)
    {
        hle->alist_audio().dry_right = (w1 + DMEM_BASE) & 0xFFFF;
        hle->alist_audio().wet_left = (w2 >> 16) + DMEM_BASE;
        hle->alist_audio().wet_right = (w2 + DMEM_BASE) & 0xFFFF;
    }
    else
    {
        hle->alist_audio().in = (w1 + DMEM_BASE) & 0xFFFF;
        hle->alist_audio().out = ((w2 >> 16) + DMEM_BASE) & 0xFFFF;
        hle->alist_audio().count = w2 & 0xFFFF;
    }
}

static void DMEMMOVE(CHle * hle, uint32_t w1, uint32_t w2)
{
    uint16_t dmemi = (w1 + DMEM_BASE) & 0xFFFF;
    uint16_t dmemo = (w2 >> 16) + DMEM_BASE;
    uint16_t count = (w2)&0xFFFF;

    if (count == 0)
        return;

    alist_move(hle, dmemo, dmemi, align(count, 16));
}

static void LOADADPCM(CHle * hle, uint32_t w1, uint32_t w2)
{
    uint16_t count = (w1 & 0xFFFF);
    uint32_t address = get_address(hle, w2);

    dram_load_u16(hle, (uint16_t *)hle->alist_audio().table, address, align(count, 8) >> 1);
}

static void INTERLEAVE(CHle * hle, uint32_t UNUSED(w1), uint32_t w2)
{
    uint16_t left = (w2 >> 16) + DMEM_BASE;
    uint16_t right = (w2 + DMEM_BASE) & 0xFFFF;

    if (hle->alist_audio().count == 0)
        return;

    alist_interleave(hle, hle->alist_audio().out, left, right, align(hle->alist_audio().count, 16));
}

static void MIXER(CHle * hle, uint32_t w1, uint32_t w2)
{
    int16_t gain = (w1)&0xFFFF;
    uint16_t dmemi = ((w2 >> 16) + DMEM_BASE) & 0xFFFF;
    uint16_t dmemo = (w2 + DMEM_BASE) & 0xFFFF;

    if (hle->alist_audio().count == 0)
        return;

    alist_mix(hle, dmemo, dmemi, align(hle->alist_audio().count, 32), gain);
}

static void SEGMENT(CHle * hle, uint32_t UNUSED(w1), uint32_t w2)
{
    set_address(hle, w2);
}

static void POLEF(CHle * hle, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16);
    uint16_t gain = w1;
    uint32_t address = get_address(hle, w2);

    if (hle->alist_audio().count == 0)
        return;

    alist_polef(
        hle,
        flags & A_INIT,
        hle->alist_audio().out,
        hle->alist_audio().in,
        align(hle->alist_audio().count, 16),
        gain,
        hle->alist_audio().table,
        address);
}

// Global functions

void alist_process_audio(CHle * hle)
{
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP, ADPCM, CLEARBUFF, ENVMIXER,
        LOADBUFF, RESAMPLE, SAVEBUFF, SEGMENT,
        SETBUFF, SETVOL, DMEMMOVE, LOADADPCM,
        MIXER, INTERLEAVE, POLEF, SETLOOP};

    clear_segments(hle);
    alist_process(hle, ABI, 0x10);
}

void alist_process_audio_ge(CHle * hle)
{
    static const acmd_callback_t ABI[0x10] =
        {
            SPNOOP, ADPCM, CLEARBUFF, ENVMIXER_GE,
            LOADBUFF, RESAMPLE, SAVEBUFF, SEGMENT,
            SETBUFF, SETVOL, DMEMMOVE, LOADADPCM,
            MIXER, INTERLEAVE, POLEF, SETLOOP};

    clear_segments(hle);
    alist_process(hle, ABI, 0x10);
}

void alist_process_audio_bc(CHle * hle)
{
    static const acmd_callback_t ABI[0x10] =
        {
            SPNOOP, ADPCM, CLEARBUFF, ENVMIXER_GE,
            LOADBUFF, RESAMPLE, SAVEBUFF, SEGMENT,
            SETBUFF, SETVOL, DMEMMOVE, LOADADPCM,
            MIXER, INTERLEAVE, POLEF, SETLOOP};

    clear_segments(hle);
    alist_process(hle, ABI, 0x10);
}
