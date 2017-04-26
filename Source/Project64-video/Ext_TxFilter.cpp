/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2007 Hiroshi Morii                                         *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/
#include <memory.h>
#include <stdlib.h>
#include "Ext_TxFilter.h"

extern "C" boolean txfilter_init(int maxwidth, int maxheight, int maxbpp, int options, int cachesize, const char *path, const char *ident, dispInfoFuncExt callback);
extern "C" void txfilter_shutdown(void);
extern "C" boolean txfilter(unsigned char *src, int srcwidth, int srcheight, unsigned short srcformat, uint64 g64crc, GHQTexInfo *info);
extern "C" boolean txfilter_hirestex(uint64 g64crc, uint64 r_crc64, unsigned short *palette, GHQTexInfo *info);
extern "C" uint64 txfilter_checksum(unsigned char *src, int width, int height, int size, int rowStride, unsigned char *palette);
extern "C" boolean txfilter_dmptx(unsigned char *src, int width, int height, int rowStridePixel, unsigned short gfmt, unsigned short n64fmt, uint64 r_crc64);
extern "C" boolean txfilter_reloadhirestex();

void ext_ghq_shutdown(void)
{
    txfilter_shutdown();
}

boolean ext_ghq_init(int maxwidth, int maxheight, int maxbpp, int options, int cachesize,
    const char *path, const char *ident, dispInfoFuncExt callback)
{
    return txfilter_init(maxwidth, maxheight, maxbpp, options, cachesize, path, ident, callback);
}

boolean ext_ghq_txfilter(unsigned char *src, int srcwidth, int srcheight, unsigned short srcformat,
    uint64 g64crc, GHQTexInfo *info)
{
    return txfilter(src, srcwidth, srcheight, srcformat, g64crc, info);;
}

boolean ext_ghq_hirestex(uint64 g64crc, uint64 r_crc64, unsigned short *palette, GHQTexInfo *info)
{
    boolean ret = txfilter_hirestex(g64crc, r_crc64, palette, info);
    return ret;
}

uint64 ext_ghq_checksum(unsigned char *src, int width, int height, int size, int rowStride, unsigned char *palette)
{
    uint64 ret = txfilter_checksum(src, width, height, size, rowStride, palette);
    return ret;
}

boolean ext_ghq_dmptx(unsigned char *src, int width, int height, int rowStridePixel, unsigned short gfmt, unsigned short n64fmt, uint64 r_crc64)
{
    boolean ret = txfilter_dmptx(src, width, height, rowStridePixel, gfmt, n64fmt, r_crc64);
    return ret;
}

boolean ext_ghq_reloadhirestex()
{
    boolean ret = txfilter_reloadhirestex();

    return ret;
}