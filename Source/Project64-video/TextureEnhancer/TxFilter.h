// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2003 Rice1964
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

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
        uint64_t g64crc, // Glide64 CRC, 64-bit for future use
        GHQTexInfo *info);
    bool hirestex(uint64_t g64crc, // Glide64 CRC, 64-bit for future use
        uint64_t r_crc64,   // Checksum hi:palette low:texture
        uint16 *palette,
        GHQTexInfo *info);
    uint64_t checksum64(uint8 *src, int width, int height, int size, int rowStride, uint8 *palette);
    bool dmptx(uint8 *src, int width, int height, int rowStridePixel, uint16 gfmt, uint16 n64fmt, uint64_t r_crc64);
    bool reloadhirestex();
};

#endif /* __TXFILTER_H__ */
