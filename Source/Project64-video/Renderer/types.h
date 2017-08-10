/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2003-2009  Sergey 'Gonetz' Lipski                          *
* Copyright (C) 2002 Dave2001                                              *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/

#pragma once
#include <Common/stdtypes.h>

typedef uint8_t gfxAlpha_t;
typedef uint32_t gfxColor_t;
typedef uint32_t gfxCCUColor_t;
typedef uint32_t gfxACUColor_t;
typedef uint32_t gfxTCCUColor_t;
typedef uint32_t gfxTACUColor_t;

enum gfxStippleMode_t
{
    GFX_STIPPLE_DISABLE = 0x0,
    GFX_STIPPLE_PATTERN = 0x1,
    GFX_STIPPLE_ROTATE = 0x2,
};

enum gfxMipMapLevelMask_t
{
    GFX_MIPMAPLEVELMASK_EVEN = 1L << 0,
    GFX_MIPMAPLEVELMASK_ODD = 1L << 1,
    GFX_MIPMAPLEVELMASK_BOTH = (GFX_MIPMAPLEVELMASK_EVEN | GFX_MIPMAPLEVELMASK_ODD),
};

enum gfxCombineMode_t
{
    GFX_FUNC_MODE_ZERO = 0x00,
    GFX_FUNC_MODE_X = 0x01,
    GFX_FUNC_MODE_ONE_MINUS_X = 0x02,
    GFX_FUNC_MODE_NEGATIVE_X = 0x03,
    GFX_FUNC_MODE_X_MINUS_HALF = 0x04,
};

enum gfxLOD_t
{
    GFX_LOD_LOG2_1 = 0x0,
    GFX_LOD_LOG2_2 = 0x1,
    GFX_LOD_LOG2_4 = 0x2,
    GFX_LOD_LOG2_8 = 0x3,
    GFX_LOD_LOG2_16 = 0x4,
    GFX_LOD_LOG2_32 = 0x5,
    GFX_LOD_LOG2_64 = 0x6,
    GFX_LOD_LOG2_128 = 0x7,
    GFX_LOD_LOG2_256 = 0x8,
    GFX_LOD_LOG2_512 = 0x9,
    GFX_LOD_LOG2_1024 = 0xa,
    GFX_LOD_LOG2_2048 = 0xb,
};

enum gfxBuffer_t
{
    GFX_BUFFER_FRONTBUFFER = 0x0,
    GFX_BUFFER_BACKBUFFER = 0x1,
    GFX_BUFFER_AUXBUFFER = 0x2,
    GFX_BUFFER_DEPTHBUFFER = 0x3,
    GFX_BUFFER_ALPHABUFFER = 0x4,
    GFX_BUFFER_TRIPLEBUFFER = 0x5,
    GFX_BUFFER_TEXTUREBUFFER_EXT = 0x6,
    GFX_BUFFER_TEXTUREAUXBUFFER_EXT = 0x7,
};

enum gfxTextureFormat_t
{
    GFX_TEXFMT_8BIT = 0x0,
    GFX_TEXFMT_RGB_332 = GFX_TEXFMT_8BIT,
    GFX_TEXFMT_YIQ_422 = 0x1,
    GFX_TEXFMT_ALPHA_8 = 0x2, /* (0..0xFF) alpha     */
    GFX_TEXFMT_INTENSITY_8 = 0x3, /* (0..0xFF) intensity */
    GFX_TEXFMT_ALPHA_INTENSITY_44 = 0x4,
    GFX_TEXFMT_P_8 = 0x5, /* 8-bit palette */
    GFX_TEXFMT_RSVD0 = 0x6, /* GFX_TEXFMT_P_8_RGBA */
    GFX_TEXFMT_P_8_6666 = GFX_TEXFMT_RSVD0,
    GFX_TEXFMT_P_8_6666_EXT = GFX_TEXFMT_RSVD0,
    GFX_TEXFMT_RSVD1 = 0x7,
    GFX_TEXFMT_16BIT = 0x8,
    GFX_TEXFMT_ARGB_8332 = GFX_TEXFMT_16BIT,
    GFX_TEXFMT_AYIQ_8422 = 0x9,
    GFX_TEXFMT_RGB_565 = 0xa,
    GFX_TEXFMT_ARGB_1555 = 0xb,
    GFX_TEXFMT_ARGB_4444 = 0xc,
    GFX_TEXFMT_ALPHA_INTENSITY_88 = 0xd,
    GFX_TEXFMT_AP_88 = 0xe, /* 8-bit alpha 8-bit palette */
    GFX_TEXFMT_RSVD2 = 0xf,
    GFX_TEXFMT_RSVD4 = GFX_TEXFMT_RSVD2,
    GFX_TEXFMT_ARGB_CMP_FXT1 = 0x11,
    GFX_TEXFMT_ARGB_8888 = 0x12,
    GFX_TEXFMT_YUYV_422 = 0x13,
    GFX_TEXFMT_UYVY_422 = 0x14,
    GFX_TEXFMT_AYUV_444 = 0x15,
    GFX_TEXFMT_ARGB_CMP_DXT1 = 0x16,
    GFX_TEXFMT_ARGB_CMP_DXT2 = 0x17,
    GFX_TEXFMT_ARGB_CMP_DXT3 = 0x18,
    GFX_TEXFMT_ARGB_CMP_DXT4 = 0x19,
    GFX_TEXFMT_ARGB_CMP_DXT5 = 0x1A,
    GFX_TEXFMT_RGB_888 = 0xFF,
    GFX_TEXFMT_GZ = 0x8000,
};

enum gfxAspectRatio_t
{
    GFX_ASPECT_LOG2_8x1 = 3,       /* 8W x 1H */
    GFX_ASPECT_LOG2_4x1 = 2,       /* 4W x 1H */
    GFX_ASPECT_LOG2_2x1 = 1,       /* 2W x 1H */
    GFX_ASPECT_LOG2_1x1 = 0,       /* 1W x 1H */
    GFX_ASPECT_LOG2_1x2 = -1,       /* 1W x 2H */
    GFX_ASPECT_LOG2_1x4 = -2,       /* 1W x 4H */
    GFX_ASPECT_LOG2_1x8 = -3,       /* 1W x 8H */
};

enum gfxCombineFunction_t
{
    GFX_COMBINE_FUNCTION_ZERO = 0x0,
    GFX_COMBINE_FUNCTION_NONE = GFX_COMBINE_FUNCTION_ZERO,
    GFX_COMBINE_FUNCTION_LOCAL = 0x1,
    GFX_COMBINE_FUNCTION_LOCAL_ALPHA = 0x2,
    GFX_COMBINE_FUNCTION_SCALE_OTHER = 0x3,
    GFX_COMBINE_FUNCTION_BLEND_OTHER = GFX_COMBINE_FUNCTION_SCALE_OTHER,
    GFX_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL = 0x4,
    GFX_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA = 0x5,
    GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL = 0x6,
    GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL = 0x7,
    GFX_COMBINE_FUNCTION_BLEND = GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL,
    GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA = 0x8,
    GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL = 0x9,
    GFX_COMBINE_FUNCTION_BLEND_LOCAL = GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL,
    GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA = 0x10,
};

enum gfxFogMode_t
{
    GFX_FOG_DISABLE = 0x0,
    GFX_FOG_WITH_TABLE_ON_FOGCOORD_EXT = 0x1,
    GFX_FOG_WITH_TABLE_ON_Q = 0x2,
    GFX_FOG_WITH_TABLE_ON_W = GFX_FOG_WITH_TABLE_ON_Q,
    GFX_FOG_WITH_ITERATED_Z = 0x3,
    GFX_FOG_WITH_ITERATED_ALPHA_EXT = 0x4,
    GFX_FOG_MULT2 = 0x100,
    GFX_FOG_ADD2 = 0x200,
};

enum gfxCombineFactor_t
{
    GFX_COMBINE_FACTOR_ZERO = 0x0,
    GFX_COMBINE_FACTOR_NONE = GFX_COMBINE_FACTOR_ZERO,
    GFX_COMBINE_FACTOR_LOCAL = 0x1,
    GFX_COMBINE_FACTOR_OTHER_ALPHA = 0x2,
    GFX_COMBINE_FACTOR_LOCAL_ALPHA = 0x3,
    GFX_COMBINE_FACTOR_TEXTURE_ALPHA = 0x4,
    GFX_COMBINE_FACTOR_TEXTURE_RGB = 0x5,
    GFX_COMBINE_FACTOR_DETAIL_FACTOR = GFX_COMBINE_FACTOR_TEXTURE_ALPHA,
    GFX_COMBINE_FACTOR_LOD_FRACTION = 0x5,
    GFX_COMBINE_FACTOR_ONE = 0x8,
    GFX_COMBINE_FACTOR_ONE_MINUS_LOCAL = 0x9,
    GFX_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA = 0xa,
    GFX_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA = 0xb,
    GFX_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA = 0xc,
    GFX_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR = GFX_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA,
    GFX_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION = 0xd,
};

enum gfxCullMode_t
{
    GFX_CULL_DISABLE = 0x0,
    GFX_CULL_NEGATIVE = 0x1,
    GFX_CULL_POSITIVE = 0x2,
};

enum gfxCombineLocal_t
{
    GFX_COMBINE_LOCAL_ITERATED = 0x0,
    GFX_COMBINE_LOCAL_CONSTANT = 0x1,
    GFX_COMBINE_LOCAL_NONE = GFX_COMBINE_LOCAL_CONSTANT,
    GFX_COMBINE_LOCAL_DEPTH = 0x2,
};

enum gfxCombineOther_t
{
    GFX_COMBINE_OTHER_ITERATED = 0x0,
    GFX_COMBINE_OTHER_TEXTURE = 0x1,
    GFX_COMBINE_OTHER_CONSTANT = 0x2,
    GFX_COMBINE_OTHER_NONE = GFX_COMBINE_OTHER_CONSTANT,
};

enum gfxAlphaBlendFnc_t
{
    GFX_BLEND_ZERO = 0x0,
    GFX_BLEND_SRC_ALPHA = 0x1,
    GFX_BLEND_SRC_COLOR = 0x2,
    GFX_BLEND_DST_COLOR = GFX_BLEND_SRC_COLOR,
    GFX_BLEND_DST_ALPHA = 0x3,
    GFX_BLEND_ONE = 0x4,
    GFX_BLEND_ONE_MINUS_SRC_ALPHA = 0x5,
    GFX_BLEND_ONE_MINUS_SRC_COLOR = 0x6,
    GFX_BLEND_ONE_MINUS_DST_COLOR = GFX_BLEND_ONE_MINUS_SRC_COLOR,
    GFX_BLEND_ONE_MINUS_DST_ALPHA = 0x7,
    GFX_BLEND_RESERVED_8 = 0x8,
    GFX_BLEND_RESERVED_9 = 0x9,
    GFX_BLEND_RESERVED_A = 0xa,
    GFX_BLEND_RESERVED_B = 0xb,
    GFX_BLEND_RESERVED_C = 0xc,
    GFX_BLEND_RESERVED_D = 0xd,
    GFX_BLEND_RESERVED_E = 0xe,
    GFX_BLEND_ALPHA_SATURATE = 0xf,
    GFX_BLEND_PREFOG_COLOR = GFX_BLEND_ALPHA_SATURATE,
};

enum gfxCmpFnc_t
{
    GFX_CMP_NEVER = 0x0,
    GFX_CMP_LESS = 0x1,
    GFX_CMP_EQUAL = 0x2,
    GFX_CMP_LEQUAL = 0x3,
    GFX_CMP_GREATER = 0x4,
    GFX_CMP_NOTEQUAL = 0x5,
    GFX_CMP_GEQUAL = 0x6,
    GFX_CMP_ALWAYS = 0x7,
};

enum GFX_CMBX
{
    GFX_CMBX_ZERO = 0x00,
    GFX_CMBX_TEXTURE_ALPHA = 0x01,
    GFX_CMBX_ALOCAL = 0x02,
    GFX_CMBX_AOTHER = 0x03,
    GFX_CMBX_B = 0x04,
    GFX_CMBX_CONSTANT_ALPHA = 0x05,
    GFX_CMBX_CONSTANT_COLOR = 0x06,
    GFX_CMBX_DETAIL_FACTOR = 0x07,
    GFX_CMBX_ITALPHA = 0x08,
    GFX_CMBX_ITRGB = 0x09,
    GFX_CMBX_LOCAL_TEXTURE_ALPHA = 0x0a,
    GFX_CMBX_LOCAL_TEXTURE_RGB = 0x0b,
    GFX_CMBX_LOD_FRAC = 0x0c,
    GFX_CMBX_OTHER_TEXTURE_ALPHA = 0x0d,
    GFX_CMBX_OTHER_TEXTURE_RGB = 0x0e,
    GFX_CMBX_TEXTURE_RGB = 0x0f,
    GFX_CMBX_TMU_CALPHA = 0x10,
    GFX_CMBX_TMU_CCOLOR = 0x11,
};

enum gfxChipID_t
{
    GFX_TMU0 = 0x0,
    GFX_TMU1 = 0x1,
    GFX_TMU2 = 0x2,
};

enum gfxDepthBufferMode_t
{
    GFX_DEPTHBUFFER_DISABLE = 0x0,
    GFX_DEPTHBUFFER_ZBUFFER = 0x1,
    GFX_DEPTHBUFFER_WBUFFER = 0x2,
    GFX_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS = 0x3,
    GFX_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS = 0x4,
};

enum gfxLfbWriteMode_t
{
    GFX_LFBWRITEMODE_565 = 0x0, /* RGB:RGB */
    GFX_LFBWRITEMODE_555 = 0x1, /* RGB:RGB */
    GFX_LFBWRITEMODE_1555 = 0x2, /* ARGB:ARGB */
    GFX_LFBWRITEMODE_RESERVED1 = 0x3,
    GFX_LFBWRITEMODE_888 = 0x4, /* RGB */
    GFX_LFBWRITEMODE_8888 = 0x5, /* ARGB */
    GFX_LFBWRITEMODE_RESERVED2 = 0x6,
    GFX_LFBWRITEMODE_RESERVED3 = 0x7,
    GFX_LFBWRITEMODE_RESERVED4 = 0x8,
    GFX_LFBWRITEMODE_RESERVED5 = 0x9,
    GFX_LFBWRITEMODE_RESERVED6 = 0xa,
    GFX_LFBWRITEMODE_RESERVED7 = 0xb,
    GFX_LFBWRITEMODE_565_DEPTH = 0xc, /* RGB:DEPTH */
    GFX_LFBWRITEMODE_555_DEPTH = 0xd, /* RGB:DEPTH */
    GFX_LFBWRITEMODE_1555_DEPTH = 0xe, /* ARGB:DEPTH */
    GFX_LFBWRITEMODE_ZA16 = 0xf, /* DEPTH:DEPTH */
    GFX_LFBWRITEMODE_ANY = 0xFF,
};

enum gfxDrawMode_t
{
    GFX_POINTS = 0,
    GFX_LINE_STRIP = 1,
    GFX_LINES = 2,
    GFX_POLYGON = 3,
    GFX_TRIANGLE_STRIP = 4,
    GFX_TRIANGLE_FAN = 5,
    GFX_TRIANGLES = 6,
    GFX_TRIANGLE_STRIP_CONTINUE = 7,
    GFX_TRIANGLE_FAN_CONTINUE = 8,
};

enum gfxColorFormat_t
{
    GFX_COLORFORMAT_ARGB = 0x0,
    GFX_COLORFORMAT_ABGR = 0x1,
    GFX_COLORFORMAT_RGBA = 0x2,
    GFX_COLORFORMAT_BGRA = 0x3,
};

enum gfxLock_t
{
    GFX_LFB_READ_ONLY = 0x00,
    GFX_LFB_WRITE_ONLY = 0x01,
    GFX_LFB_IDLE = 0x00,
    GFX_LFB_NOIDLE = 0x10,
};

enum gfxOriginLocation_t
{
    GFX_ORIGIN_UPPER_LEFT = 0x0,
    GFX_ORIGIN_LOWER_LEFT = 0x1,
    GFX_ORIGIN_ANY = 0xFF,
};

enum gfxTextureFilterMode_t
{
    GFX_TEXTUREFILTER_POINT_SAMPLED = 0x0,
    GFX_TEXTUREFILTER_BILINEAR = 0x1,
};

enum gfxTextureClampMode_t
{
    GFX_TEXTURECLAMP_WRAP = 0x0,
    GFX_TEXTURECLAMP_CLAMP = 0x1,
    GFX_TEXTURECLAMP_MIRROR_EXT = 0x2,
};

enum gfxLfbSrcFmt_t
{
    GFX_LFB_SRC_FMT_565 = 0x00,
    GFX_LFB_SRC_FMT_555 = 0x01,
    GFX_LFB_SRC_FMT_1555 = 0x02,
    GFX_LFB_SRC_FMT_888 = 0x04,
    GFX_LFB_SRC_FMT_8888 = 0x05,
    GFX_LFB_SRC_FMT_565_DEPTH = 0x0c,
    GFX_LFB_SRC_FMT_555_DEPTH = 0x0d,
    GFX_LFB_SRC_FMT_1555_DEPTH = 0x0e,
    GFX_LFB_SRC_FMT_ZA16 = 0x0f,
    GFX_LFB_SRC_FMT_RLE16 = 0x80,
};

enum gfxChromakeyMode_t
{
    GFX_CHROMAKEY_DISABLE = 0x0,
    GFX_CHROMAKEY_ENABLE = 0x1,
};

typedef struct
{
    gfxLOD_t           smallLodLog2;
    gfxLOD_t           largeLodLog2;
    gfxAspectRatio_t   aspectRatioLog2;
    gfxTextureFormat_t format;
    void               *data;
} gfxTexInfo;

typedef struct {
    int32_t            size;
    void               *lfbPtr;
    uint32_t           strideInBytes;
    gfxLfbWriteMode_t   writeMode;
    gfxOriginLocation_t origin;
} gfxLfbInfo_t;
