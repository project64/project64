// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2004 Daniel Borca
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#ifndef FXT1_H_included
#define FXT1_H_included

TAPI int TAPIENTRY
fxt1_encode(int width, int height, int comps,
    const void *source, int srcRowStride,
    void *dest, int destRowStride);

TAPI void TAPIENTRY
fxt1_decode_1(const void *texture, int stride /* in pixels */,
    int i, int j, byte *rgba);

#endif
