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
#pragma once
#include <Common/stdtypes.h>

#define NO_OPTIONS          0x00000000

#define FILTER_MASK         0x000000ff
#define NO_FILTER           0x00000000
#define SMOOTH_FILTER_MASK  0x0000000f
#define NO_SMOOTH_FILTER    0x00000000
#define SMOOTH_FILTER_1     0x00000001
#define SMOOTH_FILTER_2     0x00000002
#define SMOOTH_FILTER_3     0x00000003
#define SMOOTH_FILTER_4     0x00000004
#define SHARP_FILTER_MASK   0x000000f0
#define NO_SHARP_FILTER     0x00000000
#define SHARP_FILTER_1      0x00000010
#define SHARP_FILTER_2      0x00000020

#define ENHANCEMENT_MASK    0x00000f00
#define NO_ENHANCEMENT      0x00000000
#define X2_ENHANCEMENT      0x00000100
#define X2SAI_ENHANCEMENT   0x00000200
#define HQ2X_ENHANCEMENT    0x00000300
#define LQ2X_ENHANCEMENT    0x00000400
#define HQ4X_ENHANCEMENT    0x00000500
#define HQ2XS_ENHANCEMENT   0x00000600
#define LQ2XS_ENHANCEMENT   0x00000700

#define COMPRESSION_MASK    0x0000f000
#define NO_COMPRESSION      0x00000000
#define FXT1_COMPRESSION    0x00001000
#define NCC_COMPRESSION     0x00002000
#define S3TC_COMPRESSION    0x00003000

#define HIRESTEXTURES_MASK  0x000f0000
#define NO_HIRESTEXTURES    0x00000000
#define GHQ_HIRESTEXTURES   0x00010000
#define RICE_HIRESTEXTURES  0x00020000
#define JABO_HIRESTEXTURES  0x00030000

#define COMPRESS_TEX        0x00100000
#define COMPRESS_HIRESTEX   0x00200000
#define GZ_TEXCACHE         0x00400000
#define GZ_HIRESTEXCACHE    0x00800000
#define DUMP_TEXCACHE       0x01000000
#define DUMP_HIRESTEXCACHE  0x02000000
#define TILE_HIRESTEX       0x04000000
#define UNDEFINED_0         0x08000000
#define FORCE16BPP_HIRESTEX 0x10000000
#define FORCE16BPP_TEX      0x20000000
#define LET_TEXARTISTS_FLY  0x40000000 /* a little freedom for texture artists */
#define DUMP_TEX            0x80000000

struct GHQTexInfo {
    unsigned char *data;
    int width;
    int height;
    unsigned short format;

    int smallLodLog2;
    int largeLodLog2;
    int aspectRatioLog2;

    int tiles;
    int untiled_width;
    int untiled_height;

    unsigned char is_hires_tex;
};

/* Callback to display hires texture info.
 * Gonetz <gonetz(at)ngs.ru>
 *
 * void DispInfo(const char *format, ...)
 * {
 *   va_list args;
 *   char buf[INFO_BUF];
 *
 *   va_start(args, format);
 *   vsprintf(buf, format, args);
 *   va_end(args);
 *
 *   printf(buf);
 * }
 */

#define INFO_BUF 4095
typedef void(*dispInfoFuncExt)(const char *format, ...);

#ifndef TXFILTER_DLL
bool ext_ghq_init(int maxwidth, /* maximum texture width supported by hardware */
    int maxheight,/* maximum texture height supported by hardware */
    int maxbpp,   /* maximum texture bpp supported by hardware */
    int options,  /* options */
    int cachesize,/* cache textures to system memory */
    const char *path,   /* plugin directory. must be smaller than MAX_PATH */
    const char *ident,  /* name of ROM. must be no longer than 64 in character. */
    dispInfoFuncExt callback /* callback function to display info */
);

void ext_ghq_shutdown(void);

bool ext_ghq_txfilter(unsigned char *src,        /* input texture */
    int srcwidth,              /* width of input texture */
    int srcheight,             /* height of input texture */
    unsigned short srcformat,  /* format of input texture */
    uint64_t g64crc,             /* glide64 crc */
    GHQTexInfo *info           /* output */
);

bool ext_ghq_hirestex(uint64_t g64crc,             /* glide64 crc */
    uint64_t r_crc64,            /* checksum hi:palette low:texture */
    unsigned short *palette,   /* palette for CI textures */
    GHQTexInfo *info           /* output */
);

uint64_t ext_ghq_checksum(unsigned char *src, /* input texture */
    int width,          /* width of texture */
    int height,         /* height of texture */
    int size,           /* type of texture pixel */
    int rowStride,      /* row stride in bytes */
    unsigned char *palette /* palette */
);

bool ext_ghq_dmptx(unsigned char *src,   /* input texture (must be in 3Dfx Glide format) */
    int width,            /* width of texture */
    int height,           /* height of texture */
    int rowStridePixel,   /* row stride of input texture in pixels */
    unsigned short gfmt,  /* glide format of input texture */
    unsigned short n64fmt,/* N64 format hi:format low:size */
    uint64_t r_crc64        /* checksum hi:palette low:texture */
);

bool ext_ghq_reloadhirestex();
#endif /* TXFILTER_DLL */
