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

#ifndef __TXQUANTIZE_H__
#define __TXQUANTIZE_H__

/* Glide64 DXTn workaround
 * (0:disable, 1:enable) */
#define GLIDE64_DXTN 1

#include "TxInternal.h"
#include "TxUtil.h"

class TxQuantize
{
private:
    TxUtil *_txUtil;
    int _numcore;

    fxtCompressTexFuncExt _tx_compress_fxt1;
    dxtCompressTexFuncExt _tx_compress_dxtn;

    /* fast optimized... well, sort of. */
    void ARGB1555_ARGB8888(uint32* src, uint32* dst, int width, int height);
    void ARGB4444_ARGB8888(uint32* src, uint32* dst, int width, int height);
    void RGB565_ARGB8888(uint32* src, uint32* dst, int width, int height);
    void A8_ARGB8888(uint32* src, uint32* dst, int width, int height);
    void AI44_ARGB8888(uint32* src, uint32* dst, int width, int height);
    void AI88_ARGB8888(uint32* src, uint32* dst, int width, int height);

    void ARGB8888_ARGB1555(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_ARGB4444(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_RGB565(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_A8(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_AI44(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_AI88(uint32* src, uint32* dst, int width, int height);

    /* quality */
    void ARGB8888_RGB565_ErrD(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_ARGB1555_ErrD(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_ARGB4444_ErrD(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_AI44_ErrD(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_AI88_Slow(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_I8_Slow(uint32* src, uint32* dst, int width, int height);

    /* compressors */
    bool FXT1(uint8 *src, uint8 *dest,
        int srcwidth, int srcheight, uint16 srcformat,
        int *destwidth, int *destheight, uint16 *destformat);
    bool DXTn(uint8 *src, uint8 *dest,
        int srcwidth, int srcheight, uint16 srcformat,
        int *destwidth, int *destheight, uint16 *destformat);

public:
    TxQuantize();
    ~TxQuantize();

    /* others */
    void P8_16BPP(uint32* src, uint32* dst, int width, int height, uint32* palette);

    bool quantize(uint8* src, uint8* dest, int width, int height, uint16 srcformat, uint16 destformat, bool fastQuantizer = 1);

    bool compress(uint8 *src, uint8 *dest,
        int srcwidth, int srcheight, uint16 srcformat,
        int *destwidth, int *destheight, uint16 *destformat,
        int compressionType);

#if 0 /* unused */
    void ARGB8888_I8(uint32* src, uint32* dst, int width, int height);
    void I8_ARGB8888(uint32* src, uint32* dst, int width, int height);

    void ARGB1555_ABGR8888(uint32* src, uint32* dst, int width, int height);
    void ARGB4444_ABGR8888(uint32* src, uint32* dst, int width, int height);
    void ARGB8888_ABGR8888(uint32* src, uint32* dst, int width, int height);
#endif
};

#endif /* __TXQUANTIZE_H__ */
