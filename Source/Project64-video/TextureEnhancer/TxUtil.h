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

#ifndef __TXUTIL_H__
#define __TXUTIL_H__

/* maximum number of CPU cores allowed */
#define MAX_NUMCORE 8

#include "TxInternal.h"
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
    void tx_compress_dxtn(int srccomps, int width, int height,
        const void *source, int destformat, void *dest,
        int destRowStride);

    int fxt1_encode(int width, int height, int comps,
        const void *source, int srcRowStride,
        void *dest, int destRowStride);
#ifdef __cplusplus
}
#endif

typedef void(*dxtCompressTexFuncExt)(int srccomps, int width,
    int height, const void *srcPixData,
    int destformat, void *dest,
    int dstRowStride);

typedef int(*fxtCompressTexFuncExt)(int width, int height, int comps,
    const void *source, int srcRowStride,
    void *dest, int destRowStride);

class TxLoadLib
{
private:
    fxtCompressTexFuncExt _tx_compress_fxt1;
    dxtCompressTexFuncExt _tx_compress_dxtn;
    TxLoadLib();
public:
    static TxLoadLib* getInstance() {
        static TxLoadLib txLoadLib;
        return &txLoadLib;
    }
    ~TxLoadLib();
    fxtCompressTexFuncExt getfxtCompressTexFuncExt();
    dxtCompressTexFuncExt getdxtCompressTexFuncExt();
};

class TxUtil
{
private:
    uint32 Adler32(const uint8* data, int Len, uint32 Adler);
    uint32 Adler32(const uint8* src, int width, int height, int size, int rowStride);
    uint32 RiceCRC32(const uint8* src, int width, int height, int size, int rowStride);
    bool RiceCRC32_CI4(const uint8* src, int width, int height, int size, int rowStride,
        uint32* crc32, uint32* cimax);
    bool RiceCRC32_CI8(const uint8* src, int width, int height, int size, int rowStride,
        uint32* crc32, uint32* cimax);
    int log2(int num);
public:
    TxUtil() { }
    ~TxUtil() { }
    int sizeofTx(int width, int height, uint16 format);
    uint32 checksumTx(uint8 *data, int width, int height, uint16 format);
#if 0 /* unused */
    uint32 chkAlpha(uint32* src, int width, int height);
#endif
    uint32 checksum(uint8 *src, int width, int height, int size, int rowStride);
    uint64_t checksum64(uint8 *src, int width, int height, int size, int rowStride, uint8 *palette);
    int grLodLog2(int w, int h);
    int grAspectRatioLog2(int w, int h);
    int getNumberofProcessors();
};

class TxMemBuf
{
private:
    uint8 *_tex[2];
    uint32 _size[2];
    TxMemBuf();
public:
    static TxMemBuf* getInstance() {
        static TxMemBuf txMemBuf;
        return &txMemBuf;
    }
    ~TxMemBuf();
    bool init(int maxwidth, int maxheight);
    void shutdown(void);
    uint8 *get(unsigned int num);
    uint32 size_of(unsigned int num);
};

#endif /* __TXUTIL_H__ */
