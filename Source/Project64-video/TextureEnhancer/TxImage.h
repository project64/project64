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

#ifndef __TXIMAGE_H__
#define __TXIMAGE_H__

#include <stdio.h>
#include <png/png.h>
#include "TxInternal.h"

typedef struct tagBITMAPFILEHEADER {
    unsigned short bfType;
    unsigned long  bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned long  bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    unsigned long  biSize;
    long           biWidth;
    long           biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned long  biCompression;
    unsigned long  biSizeImage;
    long           biXPelsPerMeter;
    long           biYPelsPerMeter;
    unsigned long  biClrUsed;
    unsigned long  biClrImportant;
} BITMAPINFOHEADER;

#define DDSD_CAPS	0x00000001
#define DDSD_HEIGHT	0x00000002
#define DDSD_WIDTH	0x00000004
#define DDSD_PITCH	0x00000008
#define DDSD_PIXELFORMAT	0x00001000
#define DDSD_MIPMAPCOUNT	0x00020000
#define DDSD_LINEARSIZE	0x00080000
#define DDSD_DEPTH	0x00800000

#define DDPF_ALPHAPIXELS	0x00000001
#define DDPF_FOURCC	0x00000004
#define DDPF_RGB	0x00000040

#define DDSCAPS_COMPLEX	0x00000008
#define DDSCAPS_TEXTURE	0x00001000
#define DDSCAPS_MIPMAP	0x00400000

typedef struct tagDDSPIXELFORMAT {
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long dwFourCC;
    unsigned long dwRGBBitCount;
    unsigned long dwRBitMask;
    unsigned long dwGBitMask;
    unsigned long dwBBitMask;
    unsigned long dwRGBAlphaBitMask;
} DDSPIXELFORMAT;

typedef struct tagDDSFILEHEADER {
    unsigned long dwMagic;
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long dwHeight;
    unsigned long dwWidth;
    unsigned long dwLinearSize;
    unsigned long dwDepth;
    unsigned long dwMipMapCount;
    unsigned long dwReserved1[11];
    DDSPIXELFORMAT ddpf;
    unsigned long dwCaps1;
    unsigned long dwCaps2;
} DDSFILEHEADER;

class TxImage
{
private:
    bool getPNGInfo(FILE *fp, png_structp *png_ptr, png_infop *info_ptr);
    bool getBMPInfo(FILE *fp, BITMAPFILEHEADER *bmp_fhdr, BITMAPINFOHEADER *bmp_ihdr);
    bool getDDSInfo(FILE *fp, DDSFILEHEADER *dds_fhdr);
public:
    TxImage() {}
    ~TxImage() {}
    uint8* readPNG(FILE* fp, int* width, int* height, uint16* format);
    bool writePNG(uint8* src, FILE* fp, int width, int height, int rowStride, uint16 format, uint8 *palette);
    uint8* readBMP(FILE* fp, int* width, int* height, uint16* format);
    uint8* readDDS(FILE* fp, int* width, int* height, uint16* format);
};

#endif /* __TXIMAGE_H__ */
