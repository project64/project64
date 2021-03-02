// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#include "TexLoad4b.h"
#include "TexLoad8b.h"
#include "TexLoad16b.h"
#include "TexLoad32b.h"

#include <string.h>

uint32_t LoadNone(uintptr_t /*dst*/, uintptr_t /*src*/, int /*wid_64*/, int /*height*/, int /*line*/, int /*real_width*/, int /*tile*/)
{
    memset(texture, 0, 4096 * 4);
    return (1 << 16) | GFX_TEXFMT_ARGB_1555;
}

typedef uint32_t(*texfunc)(uintptr_t, uintptr_t, int, int, int, int, int);
texfunc load_table[4][5] = {	// [size][format]
    { Load4bSelect,
    LoadNone,
    Load4bCI,
    Load4bIA,
    Load4bI },

    { Load8bCI,
    LoadNone,
    Load8bCI,
    Load8bIA,
    Load8bI },

    { Load16bRGBA,
    Load16bYUV,
    Load16bRGBA,
    Load16bIA,
    LoadNone },

    { Load32bRGBA,
    LoadNone,
    LoadNone,
    LoadNone,
    LoadNone }
};
