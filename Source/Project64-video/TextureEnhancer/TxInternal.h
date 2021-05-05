// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2007 Hiroshi Morii
// Copyright(C) 2003 Rice1964
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#ifndef __INTERNAL_H__
#define __INTERNAL_H__

#include "Ext_TxFilter.h"

// DLL exports
#ifdef TXFILTER_DLL
#define TAPI __declspec(dllexport)
#define TAPIENTRY
#else
#define TAPI
#define TAPIENTRY
#endif

#include <stdint.h>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t  uint32;

#ifdef _WIN32
#define KBHIT(key) ((GetAsyncKeyState(key) & 0x8001) == 0x8001)
#else
#define KBHIT(key) (0)
#endif

// From OpenGL glext.h
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3

// For explicit fxt1 compression
#define CC_CHROMA 0x0
#define CC_HI     0x1
#define CC_ALPHA  0x2

// In-memory zlib texture compression
#define GR_TEXFMT_GZ                 0x8000

#endif /* __INTERNAL_H__ */
