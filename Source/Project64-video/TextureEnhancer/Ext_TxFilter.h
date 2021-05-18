// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2003 Rice1964
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#pragma once
#include <stdint.h>

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
#define LET_TEXARTISTS_FLY  0x40000000 // A little freedom for texture artists
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

/* Callback to display high resolution texture info.
 * Gonetz
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
bool ext_ghq_init(int maxwidth, // Maximum texture width supported by hardware
    int maxheight, // Maximum texture height supported by hardware
    int maxbpp,   // Maximum texture bpp supported by hardware
    int options,  // Options
    int cachesize, // Cache textures to system memory
    const char *path,   // Plugin directory, must be smaller than MAX_PATH
    const char *ident,  // Name of ROM, must be no longer than 64 characters
    dispInfoFuncExt callback // Callback function to display info
);

void ext_ghq_shutdown(void);

bool ext_ghq_txfilter(unsigned char *src,        // Input texture
    int srcwidth,              // Width of input texture
    int srcheight,             // Height of input texture
    unsigned short srcformat,  // Format of input texture
    uint64_t g64crc,             // Glide64 CRC
    GHQTexInfo *info           // Output
);

bool ext_ghq_hirestex(uint64_t g64crc,             // Glide64 CRC
    uint64_t r_crc64,            // Checksum hi:palette low:texture
    unsigned short *palette,   // Palette for CI textures
    GHQTexInfo *info           // Output
);

uint64_t ext_ghq_checksum(unsigned char *src, // Input texture
    int width,          // Width of texture
    int height,         // Height of texture
    int size,           // Type of texture pixel
    int rowStride,      // Row stride in bytes
    unsigned char *palette // Palette
);

bool ext_ghq_dmptx(unsigned char *src,   // Input texture (must be in 3DFX Glide format)
    int width,            // Width of texture
    int height,           // Height of texture
    int rowStridePixel,   // Row stride of input texture in pixels
    unsigned short gfmt,  // Glide format of input texture
    unsigned short n64fmt,// N64 format hi:format low:size
    uint64_t r_crc64        // Checksum hi:palette low:texture
);

bool ext_ghq_reloadhirestex();
#endif // TXFILTER_DLL
