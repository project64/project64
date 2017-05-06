/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2007  Hiroshi Morii                                        *
* Copyright (C) 2004  Daniel Borca                                         *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/

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