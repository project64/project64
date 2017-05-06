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
dxt1_rgb_decode_1(const void *texture, int stride /* in pixels */,
    int i, int j, byte *rgba);

TAPI void TAPIENTRY
dxt1_rgba_decode_1(const void *texture, int stride /* in pixels */,
    int i, int j, byte *rgba);

TAPI void TAPIENTRY
dxt3_rgba_decode_1(const void *texture, int stride /* in pixels */,
    int i, int j, byte *rgba);

TAPI void TAPIENTRY
dxt5_rgba_decode_1(const void *texture, int stride /* in pixels */,
    int i, int j, byte *rgba);

#endif
