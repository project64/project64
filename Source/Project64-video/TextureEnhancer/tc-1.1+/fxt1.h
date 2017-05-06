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
