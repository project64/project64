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
#include "dxtn.h"

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3

TAPI void TAPIENTRY
fetch_2d_texel_rgb_dxt1(int texImage_RowStride,
    const byte *texImage_Data,
    int i, int j,
    byte *texel)
{
    dxt1_rgb_decode_1(texImage_Data, texImage_RowStride, i, j, texel);
}

TAPI void TAPIENTRY
fetch_2d_texel_rgba_dxt1(int texImage_RowStride,
    const byte *texImage_Data,
    int i, int j,
    byte *texel)
{
    dxt1_rgba_decode_1(texImage_Data, texImage_RowStride, i, j, texel);
}

TAPI void TAPIENTRY
fetch_2d_texel_rgba_dxt3(int texImage_RowStride,
    const byte *texImage_Data,
    int i, int j,
    byte *texel)
{
    dxt3_rgba_decode_1(texImage_Data, texImage_RowStride, i, j, texel);
}

TAPI void TAPIENTRY
fetch_2d_texel_rgba_dxt5(int texImage_RowStride,
    const byte *texImage_Data,
    int i, int j,
    byte *texel)
{
    dxt5_rgba_decode_1(texImage_Data, texImage_RowStride, i, j, texel);
}

TAPI void TAPIENTRY
tx_compress_dxtn(int srccomps, int width, int height,
    const byte *source, int destformat, byte *dest,
    int destRowStride)
{
    int srcRowStride = width * srccomps;
    int rv;

    switch (destformat) {
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        rv = dxt1_rgb_encode(width, height, srccomps,
            source, srcRowStride,
            dest, destRowStride);
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        rv = dxt1_rgba_encode(width, height, srccomps,
            source, srcRowStride,
            dest, destRowStride);
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        rv = dxt3_rgba_encode(width, height, srccomps,
            source, srcRowStride,
            dest, destRowStride);
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        rv = dxt5_rgba_encode(width, height, srccomps,
            source, srcRowStride,
            dest, destRowStride);
        break;
    default:
        assert(0);
    }

    /*return rv;*/
}