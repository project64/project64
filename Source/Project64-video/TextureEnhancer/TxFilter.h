/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2007 Hiroshi Morii                                         *
* Copyright (C) 2003 Rice1964                                              *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/

#ifndef __TXFILTER_H__
#define __TXFILTER_H__

#include "TxInternal.h"
#include "TxQuantize.h"
#include "TxHiResCache.h"
#include "TxTexCache.h"
#include "TxUtil.h"
#include "TxImage.h"
#include <string>

class TxFilter
{
private:
    int _numcore;

    uint8 *_tex1;
    uint8 *_tex2;
    int _maxwidth;
    int _maxheight;
    int _maxbpp;
    int _options;
    int _cacheSize;
    std::string _ident;
    std::string _path;
    TxQuantize *_txQuantize;
    TxTexCache *_txTexCache;
    TxHiResCache *_txHiResCache;
    TxUtil *_txUtil;
    TxImage *_txImage;
    bool _initialized;
    void clear();
public:
    ~TxFilter();
    TxFilter(int maxwidth,
        int maxheight,
        int maxbpp,
        int options,
        int cachesize,
        const char *path,
        const char *ident,
        dispInfoFuncExt callback);
    bool filter(uint8 *src,
        int srcwidth,
        int srcheight,
        uint16 srcformat,
        uint64_t g64crc, /* glide64 crc, 64bit for future use */
        GHQTexInfo *info);
    bool hirestex(uint64_t g64crc, /* glide64 crc, 64bit for future use */
        uint64_t r_crc64,   /* checksum hi:palette low:texture */
        uint16 *palette,
        GHQTexInfo *info);
    uint64_t checksum64(uint8 *src, int width, int height, int size, int rowStride, uint8 *palette);
    bool dmptx(uint8 *src, int width, int height, int rowStridePixel, uint16 gfmt, uint16 n64fmt, uint64_t r_crc64);
    bool reloadhirestex();
};

#endif /* __TXFILTER_H__ */
