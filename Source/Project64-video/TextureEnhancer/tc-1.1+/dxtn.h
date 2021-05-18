// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2004 Daniel Borca
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#ifndef DXTN_H_included
#define DXTN_H_included

TAPI int TAPIENTRY
dxt1_rgb_encode(int width, int height, int comps,
    const void *source, int srcRowStride,
    void *dest, int destRowStride);

TAPI int TAPIENTRY
dxt1_rgba_encode(int width, int height, int comps,
    const void *source, int srcRowStride,
    void *dest, int destRowStride);

TAPI int TAPIENTRY
dxt3_rgba_encode(int width, int height, int comps,
    const void *source, int srcRowStride,
    void *dest, int destRowStride);

TAPI int TAPIENTRY
dxt5_rgba_encode(int width, int height, int comps,
    const void *source, int srcRowStride,
    void *dest, int destRowStride);

TAPI void TAPIENTRY
dxt1_rgb_decode_1(const void *texture, int stride, // In pixels
    int i, int j, byte *rgba);

TAPI void TAPIENTRY
dxt1_rgba_decode_1(const void *texture, int stride, // In pixels
    int i, int j, byte *rgba);

TAPI void TAPIENTRY
dxt3_rgba_decode_1(const void *texture, int stride, // In pixels
    int i, int j, byte *rgba);

TAPI void TAPIENTRY
dxt5_rgba_decode_1(const void *texture, int stride, // In pixels
    int i, int j, byte *rgba);

#endif
