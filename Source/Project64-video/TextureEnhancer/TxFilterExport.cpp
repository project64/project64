// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2003 Rice1964
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#ifdef WIN32
#pragma warning(disable: 4786)
#endif

#include "TxFilter.h"

TxFilter *txFilter = nullptr;

#ifdef __cplusplus
extern "C" {
#endif

    TAPI bool TAPIENTRY
        txfilter_init(int maxwidth, int maxheight, int maxbpp, int options, int cachesize,
            const char *path, const char * ident,
            dispInfoFuncExt callback)
    {
        if (txFilter) return 0;

        txFilter = new TxFilter(maxwidth, maxheight, maxbpp, options, cachesize,
            path, ident, callback);

        return (txFilter ? 1 : 0);
    }

    void txfilter_shutdown(void)
    {
        if (txFilter) delete txFilter;

        txFilter = nullptr;
    }

    TAPI bool TAPIENTRY
        txfilter(uint8 *src, int srcwidth, int srcheight, uint16 srcformat,
            uint64_t g64crc, GHQTexInfo *info)
    {
        if (txFilter)
            return txFilter->filter(src, srcwidth, srcheight, srcformat,
                g64crc, info);

        return 0;
    }

    TAPI bool TAPIENTRY
        txfilter_hirestex(uint64_t g64crc, uint64_t r_crc64, uint16 *palette, GHQTexInfo *info)
    {
        if (txFilter)
            return txFilter->hirestex(g64crc, r_crc64, palette, info);

        return 0;
    }

    TAPI uint64_t TAPIENTRY
        txfilter_checksum(uint8 *src, int width, int height, int size, int rowStride, uint8 *palette)
    {
        if (txFilter)
            return txFilter->checksum64(src, width, height, size, rowStride, palette);

        return 0;
    }

    TAPI bool TAPIENTRY
        txfilter_dmptx(uint8 *src, int width, int height, int rowStridePixel, uint16 gfmt, uint16 n64fmt, uint64_t r_crc64)
    {
        if (txFilter)
            return txFilter->dmptx(src, width, height, rowStridePixel, gfmt, n64fmt, r_crc64);

        return 0;
    }

    TAPI bool TAPIENTRY
        txfilter_reloadhirestex()
    {
        if (txFilter)
            return txFilter->reloadhirestex();

        return 0;
    }

#ifdef __cplusplus
}
#endif
