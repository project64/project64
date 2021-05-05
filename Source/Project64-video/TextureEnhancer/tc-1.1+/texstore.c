// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2004 Daniel Borca
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#include <assert.h>

#include "types.h"
#include "internal.h"

void
_mesa_upscale_teximage2d(unsigned int inWidth, unsigned int inHeight,
    unsigned int outWidth, unsigned int outHeight,
    unsigned int comps,
    const byte *src, int srcRowStride,
    byte *dest)
{
    unsigned int i, j, k;

    assert(outWidth >= inWidth);
    assert(outHeight >= inHeight);

#if 1 /* H.Morii - faster loops */
    for (i = 0; i < inHeight; i++) {
        for (j = 0; j < inWidth; j++) {
            const int aa = (i * outWidth + j) * comps;
            const int bb = i * srcRowStride + j * comps;
            for (k = 0; k < comps; k++) {
                dest[aa + k] = src[bb + k];
            }
        }
        for (; j < outWidth; j++) {
            const int aa = (i * outWidth + j) * comps;
            const int bb = i * srcRowStride + (j - inWidth) * comps;
            for (k = 0; k < comps; k++) {
                dest[aa + k] = src[bb + k];
            }
        }
    }
    for (; i < outHeight; i++) {
        for (j = 0; j < inWidth; j++) {
            const int aa = (i * outWidth + j) * comps;
            const int bb = (i - inHeight) * srcRowStride + j * comps;
            for (k = 0; k < comps; k++) {
                dest[aa + k] = src[bb + k];
            }
        }
        for (; j < outWidth; j++) {
            const int aa = (i * outWidth + j) * comps;
            const int bb = (i - inHeight) * srcRowStride + (j - inWidth) * comps;
            for (k = 0; k < comps; k++) {
                dest[aa + k] = src[bb + k];
            }
        }
    }
#else
    for (i = 0; i < outHeight; i++) {
        const int ii = i % inHeight;
        for (j = 0; j < outWidth; j++) {
            const int jj = j % inWidth;
            const int aa = (i * outWidth + j) * comps;
            const int bb = ii * srcRowStride + jj * comps;
            for (k = 0; k < comps; k++) {
                dest[aa + k] = src[bb + k];
            }
        }
    }
#endif
}
