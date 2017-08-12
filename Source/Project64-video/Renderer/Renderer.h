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
#include <Project64-video/Renderer/types.h>

void gfxClipWindow(uint32_t minx, uint32_t miny, uint32_t maxx, uint32_t maxy);
void gfxColorMask(bool rgb, bool a);
uint32_t gfxTexMinAddress(gfxChipID_t tmu);
bool gfxSstWinClose();
void gfxTextureBufferExt(gfxChipID_t tmu, uint32_t startAddress, gfxLOD_t lodmin, gfxLOD_t lodmax, gfxAspectRatio_t aspect, gfxTextureFormat_t fmt, uint32_t evenOdd);
uint32_t gfxTexMaxAddress(gfxChipID_t tmu);
uint32_t gfxTexTextureMemRequired(uint32_t evenOdd, gfxTexInfo *info);
void gfxConstantColorValue(gfxColor_t value);
void gfxColorCombine(gfxCombineFunction_t function, gfxCombineFactor_t factor, gfxCombineLocal_t local, gfxCombineOther_t other, bool invert);
void gfxAlphaCombine(gfxCombineFunction_t function, gfxCombineFactor_t factor, gfxCombineLocal_t local, gfxCombineOther_t other, bool invert);
void gfxTexCombine(gfxChipID_t tmu, gfxCombineFunction_t rgb_function, gfxCombineFactor_t rgb_factor, gfxCombineFunction_t alpha_function, gfxCombineFactor_t alpha_factor, bool rgb_invert, bool alpha_invert);
void gfxAlphaBlendFunction(gfxAlphaBlendFnc_t rgb_sf, gfxAlphaBlendFnc_t rgb_df, gfxAlphaBlendFnc_t alpha_sf, gfxAlphaBlendFnc_t alpha_df);
void gfxAlphaTestReferenceValue(gfxAlpha_t value);
void gfxAlphaTestFunction(gfxCmpFnc_t function);
void gfxFogMode(gfxFogMode_t mode);
void gfxFogGenerateLinear(float nearZ, float farZ);
void gfxFogColorValue(gfxColor_t fogcolor);
void gfxChromakeyMode(gfxChromakeyMode_t mode);
void gfxChromakeyValue(gfxColor_t value);
void gfxStippleMode(gfxStippleMode_t mode);
void gfxColorCombineExt(gfxCCUColor_t a, gfxCombineMode_t a_mode, gfxCCUColor_t b, gfxCombineMode_t b_mode, gfxCCUColor_t c, bool c_invert, gfxCCUColor_t d, bool d_invert, uint32_t shift, bool invert);
void gfxAlphaCombineExt(gfxACUColor_t a, gfxCombineMode_t a_mode, gfxACUColor_t b, gfxCombineMode_t b_mode, gfxACUColor_t c, bool c_invert, gfxACUColor_t d, bool d_invert, uint32_t shift, bool invert);
void gfxTexColorCombineExt(gfxChipID_t tmu, gfxTCCUColor_t a, gfxCombineMode_t a_mode, gfxTCCUColor_t b, gfxCombineMode_t b_mode, gfxTCCUColor_t c, bool c_invert, gfxTCCUColor_t d, bool d_invert, uint32_t shift, bool invert);
void gfxTexAlphaCombineExt(gfxChipID_t tmu, gfxTACUColor_t a, gfxCombineMode_t a_mode, gfxTACUColor_t b, gfxCombineMode_t b_mode, gfxTACUColor_t c, bool c_invert, gfxTACUColor_t d, bool d_invert, uint32_t shift, bool invert);
void gfxConstantColorValueExt(gfxChipID_t tmu, gfxColor_t value);
void gfxCullMode(gfxCullMode_t mode);
void gfxDepthBufferMode(gfxDepthBufferMode_t mode);
void gfxDepthBufferFunction(gfxCmpFnc_t function);
void gfxDepthMask(bool mask);
void gfxDrawTriangle(const void *a, const void *b, const void *c);
void gfxDepthBiasLevel(int32_t level);
void gfxDrawLine(const void *a, const void *b);
void gfxDrawVertexArray(gfxDrawMode_t mode, uint32_t Count, void *pointers2);
void gfxDrawVertexArrayContiguous(gfxDrawMode_t mode, uint32_t Count, void *pointers, uint32_t stride);

bool gfxSstWinOpen(gfxColorFormat_t color_format, gfxOriginLocation_t origin_location, int nColBuffers, int nAuxBuffers);
void gfxAuxBufferExt(gfxBuffer_t buffer);
void gfxRenderBuffer(gfxBuffer_t buffer);
void gfxBufferClear(gfxColor_t color, gfxAlpha_t alpha, uint32_t depth);
void gfxBufferSwap(uint32_t swap_interval);
bool gfxLfbLock(gfxLock_t type, gfxBuffer_t buffer, gfxLfbWriteMode_t writeMode, gfxOriginLocation_t origin, bool pixelPipeline, gfxLfbInfo_t *info);
bool gfxLfbUnlock(gfxLock_t type, gfxBuffer_t buffer);
bool gfxLfbReadRegion(gfxBuffer_t src_buffer, uint32_t src_x, uint32_t src_y, uint32_t src_width, uint32_t src_height, uint32_t dst_stride, void *dst_data);
bool gfxLfbWriteRegion(gfxBuffer_t dst_buffer, uint32_t dst_x, uint32_t dst_y, gfxLfbSrcFmt_t src_format, uint32_t src_width, uint32_t src_height, bool pixelPipeline, int32_t src_stride, void *src_data);
uint32_t gfxTexCalcMemRequired(gfxLOD_t lodmin, gfxLOD_t lodmax, gfxAspectRatio_t aspect, gfxTextureFormat_t fmt);
void gfxLoadGammaTable(uint32_t nentries, uint32_t *red, uint32_t *green, uint32_t *blue);
void gfxGetGammaTableExt(uint32_t /*nentries*/, uint32_t *red, uint32_t *green, uint32_t *blue);
void gfxGammaCorrectionRGB(float gammaR, float gammaG, float gammaB);
void gfxTexDownloadMipMap(gfxChipID_t tmu, uint32_t startAddress, gfxMipMapLevelMask_t evenOdd, gfxTexInfo *info);
void gfxTexSource(gfxChipID_t tmu, uint32_t startAddress, uint32_t evenOdd, gfxTexInfo *info);
void gfxTexDetailControl(gfxChipID_t tmu, int lod_bias, uint8_t detail_scale, float detail_max);
void gfxTexClampMode(gfxChipID_t tmu, gfxTextureClampMode_t s_clampmode, gfxTextureClampMode_t t_clampmode);
void gfxTexFilterMode(gfxChipID_t tmu, gfxTextureFilterMode_t minfilter_mode, gfxTextureFilterMode_t magfilter_mode);

extern uint32_t nbTextureUnits;
extern uint32_t g_scr_res_x, g_scr_res_y, g_res_x, g_res_y;
