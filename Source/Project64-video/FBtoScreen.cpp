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
#include <Project64-video/Renderer/Renderer.h>
#include "Gfx_1.3.h"
#include "FBtoScreen.h"
#include "TexCache.h"
#include <Project64-video/trace.h>

static gfxChipID_t SetupFBtoScreenCombiner(uint32_t texture_size, uint32_t opaque)
{
    gfxChipID_t tmu;
    if (voodoo.tmem_ptr[GFX_TMU0] + texture_size < voodoo.tex_max_addr[0])
    {
        tmu = GFX_TMU0;
        gfxTexCombine(GFX_TMU1, GFX_COMBINE_FUNCTION_NONE, GFX_COMBINE_FACTOR_NONE, GFX_COMBINE_FUNCTION_NONE, GFX_COMBINE_FACTOR_NONE, false, false);
        gfxTexCombine(GFX_TMU0, GFX_COMBINE_FUNCTION_LOCAL, GFX_COMBINE_FACTOR_NONE, GFX_COMBINE_FUNCTION_LOCAL, GFX_COMBINE_FACTOR_NONE, false, false);
    }
    else
    {
        if (voodoo.tmem_ptr[GFX_TMU1] + texture_size >= voodoo.tex_max_addr[1])
            ClearCache();
        tmu = GFX_TMU1;
        gfxTexCombine(GFX_TMU1,
            GFX_COMBINE_FUNCTION_LOCAL,
            GFX_COMBINE_FACTOR_NONE,
            GFX_COMBINE_FUNCTION_LOCAL,
            GFX_COMBINE_FACTOR_NONE,
            false,
            false);
        gfxTexCombine(GFX_TMU0,
            GFX_COMBINE_FUNCTION_SCALE_OTHER,
            GFX_COMBINE_FACTOR_ONE,
            GFX_COMBINE_FUNCTION_SCALE_OTHER,
            GFX_COMBINE_FACTOR_ONE,
            false,
            false);
    }
    gfxTextureFilterMode_t filter = (rdp.filter_mode != 2) ? GFX_TEXTUREFILTER_POINT_SAMPLED : GFX_TEXTUREFILTER_BILINEAR;
    gfxTexFilterMode(tmu, filter, filter);
    gfxTexClampMode(tmu,
        GFX_TEXTURECLAMP_CLAMP,
        GFX_TEXTURECLAMP_CLAMP);
    //  gfxConstantColorValue (0xFFFFFFFF);
    gfxColorCombine(GFX_COMBINE_FUNCTION_SCALE_OTHER,
        GFX_COMBINE_FACTOR_ONE,
        GFX_COMBINE_LOCAL_NONE,
        GFX_COMBINE_OTHER_TEXTURE,
        //    GFX_COMBINE_OTHER_CONSTANT,
        false);
    gfxAlphaCombine(GFX_COMBINE_FUNCTION_SCALE_OTHER,
        GFX_COMBINE_FACTOR_ONE,
        GFX_COMBINE_LOCAL_NONE,
        GFX_COMBINE_OTHER_TEXTURE,
        false);
    if (opaque)
    {
        gfxAlphaTestFunction(GFX_CMP_ALWAYS);
        gfxAlphaBlendFunction(GFX_BLEND_ONE,
            GFX_BLEND_ZERO,
            GFX_BLEND_ONE,
            GFX_BLEND_ZERO);
    }
    else
    {
        gfxAlphaBlendFunction(GFX_BLEND_SRC_ALPHA,
            GFX_BLEND_ONE_MINUS_SRC_ALPHA,
            GFX_BLEND_ONE,
            GFX_BLEND_ZERO);
    }
    gfxDepthBufferFunction(GFX_CMP_ALWAYS);
    gfxCullMode(GFX_CULL_DISABLE);
    gfxDepthMask(false);
    rdp.update |= UPDATE_COMBINE | UPDATE_ZBUF_ENABLED | UPDATE_CULL_MODE;
    return tmu;
}

static void DrawRE2Video(FB_TO_SCREEN_INFO & fb_info, float scale)
{
    float scale_y = (float)fb_info.width / rdp.vi_height;
    float height = g_scr_res_x / scale_y;
    float ul_x = 0.5f;
    float ul_y = (g_scr_res_y - height) / 2.0f;
    float lr_y = g_scr_res_y - ul_y - 1.0f;
    float lr_x = g_scr_res_x - 1.0f;
    float lr_u = (fb_info.width - 1)*scale;
    float lr_v = (fb_info.height - 1)*scale;
    gfxVERTEX v[4] = {
        { ul_x, ul_y, 1, 1, 0.5f, 0.5f, 0.5f, 0.5f, { 0.5f, 0.5f, 0.5f, 0.5f } },
        { lr_x, ul_y, 1, 1, lr_u, 0.5f, lr_u, 0.5f, { lr_u, 0.5f, lr_u, 0.5f } },
        { ul_x, lr_y, 1, 1, 0.5f, lr_v, 0.5f, lr_v, { 0.5f, lr_v, 0.5f, lr_v } },
        { lr_x, lr_y, 1, 1, lr_u, lr_v, lr_u, lr_v, { lr_u, lr_v, lr_u, lr_v } }
    };
    gfxDrawTriangle(&v[0], &v[2], &v[1]);
    gfxDrawTriangle(&v[2], &v[3], &v[1]);
}

static void DrawRE2Video256(FB_TO_SCREEN_INFO & fb_info)
{
    WriteTrace(TraceRDP, TraceDebug, "DrawRE2Video256. ul_x=%d, ul_y=%d, lr_x=%d, lr_y=%d, size=%d, addr=%08lx", fb_info.ul_x, fb_info.ul_y, fb_info.lr_x, fb_info.lr_y, fb_info.size, fb_info.addr);
    uint32_t * src = (uint32_t*)(gfx.RDRAM + fb_info.addr);
    gfxTexInfo t_info;
    t_info.smallLodLog2 = GFX_LOD_LOG2_256;
    t_info.largeLodLog2 = GFX_LOD_LOG2_256;
    t_info.aspectRatioLog2 = GFX_ASPECT_LOG2_1x1;
    uint16_t * tex = (uint16_t*)texture_buffer;
    uint16_t * dst = tex;
    uint32_t col;
    uint8_t r, g, b;
    fb_info.height = minval(256, fb_info.height);
    for (uint32_t h = 0; h < fb_info.height; h++)
    {
        for (uint32_t w = 0; w < 256; w++)
        {
            col = *(src++);
            r = (uint8_t)((col >> 24) & 0xFF);
            r = (uint8_t)((float)r / 255.0f * 31.0f);
            g = (uint8_t)((col >> 16) & 0xFF);
            g = (uint8_t)((float)g / 255.0f * 63.0f);
            b = (uint8_t)((col >> 8) & 0xFF);
            b = (uint8_t)((float)b / 255.0f * 31.0f);
            *(dst++) = (r << 11) | (g << 5) | b;
        }
        src += (fb_info.width - 256);
    }
    t_info.format = GFX_TEXFMT_RGB_565;
    t_info.data = tex;
    gfxChipID_t tmu = SetupFBtoScreenCombiner(gfxTexTextureMemRequired(GFX_MIPMAPLEVELMASK_BOTH, &t_info), fb_info.opaque);
    gfxTexDownloadMipMap(tmu,
        voodoo.tex_min_addr[tmu] + voodoo.tmem_ptr[tmu],
        GFX_MIPMAPLEVELMASK_BOTH,
        &t_info);
    gfxTexSource(tmu,
        voodoo.tex_min_addr[tmu] + voodoo.tmem_ptr[tmu],
        GFX_MIPMAPLEVELMASK_BOTH,
        &t_info);
    DrawRE2Video(fb_info, 1.0f);
}

static void DrawFrameBufferToScreen256(FB_TO_SCREEN_INFO & fb_info)
{
    if (g_settings->hacks(CSettings::hack_RE2))
    {
        DrawRE2Video256(fb_info);
        return;
    }
    WriteTrace(TraceRDP, TraceDebug, "DrawFrameBufferToScreen256. ul_x=%d, ul_y=%d, lr_x=%d, lr_y=%d, size=%d, addr=%08lx", fb_info.ul_x, fb_info.ul_y, fb_info.lr_x, fb_info.lr_y, fb_info.size, fb_info.addr);
    uint32_t width = fb_info.lr_x - fb_info.ul_x + 1;
    uint32_t height = fb_info.lr_y - fb_info.ul_y + 1;
    gfxTexInfo t_info;
    uint8_t * image = gfx.RDRAM + fb_info.addr;
    uint32_t width256 = ((width - 1) >> 8) + 1;
    uint32_t height256 = ((height - 1) >> 8) + 1;
    t_info.smallLodLog2 = t_info.largeLodLog2 = GFX_LOD_LOG2_256;
    t_info.aspectRatioLog2 = GFX_ASPECT_LOG2_1x1;
    t_info.format = GFX_TEXFMT_ARGB_1555;
    uint16_t * tex = (uint16_t*)texture_buffer;
    t_info.data = tex;
    uint32_t tex_size = gfxTexTextureMemRequired(GFX_MIPMAPLEVELMASK_BOTH, &t_info);
    gfxChipID_t tmu = SetupFBtoScreenCombiner(tex_size*width256*height256, fb_info.opaque);
    uint16_t * src = (uint16_t*)image;
    src += fb_info.ul_x + fb_info.ul_y * fb_info.width;
    uint32_t * src32 = (uint32_t*)image;
    src32 += fb_info.ul_x + fb_info.ul_y * fb_info.width;
    uint32_t w_tail = width % 256;
    uint32_t h_tail = height % 256;
    uint16_t c;
    uint32_t c32;
    uint32_t idx;
    uint32_t bound = BMASK + 1 - fb_info.addr;
    bound = fb_info.size == 2 ? bound >> 1 : bound >> 2;
    uint8_t r, g, b, a;
    uint32_t cur_width, cur_height, cur_tail;
    uint32_t tex_adr = voodoo.tex_min_addr[tmu] + voodoo.tmem_ptr[tmu];
    if ((voodoo.tmem_ptr[tmu] < TEXMEM_2MB_EDGE) && (voodoo.tmem_ptr[tmu] + tex_size*width256*height256 > TEXMEM_2MB_EDGE))
    {
        tex_adr = TEXMEM_2MB_EDGE;
    }
    for (uint32_t h = 0; h < height256; h++)
    {
        for (uint32_t w = 0; w < width256; w++)
        {
            cur_width = (256 * (w + 1) < width) ? 256 : w_tail;
            cur_height = (256 * (h + 1) < height) ? 256 : h_tail;
            cur_tail = 256 - cur_width;
            uint16_t * dst = tex;
            if (fb_info.size == 2)
            {
                for (uint32_t y = 0; y < cur_height; y++)
                {
                    for (uint32_t x = 0; x < cur_width; x++)
                    {
                        idx = (x + 256 * w + (y + 256 * h)*fb_info.width) ^ 1;
                        if (idx >= bound)
                            break;
                        c = src[idx];
                        *(dst++) = (c >> 1) | ((c & 1) << 15);
                    }
                    dst += cur_tail;
                }
            }
            else
            {
                for (uint32_t y = 0; y < cur_height; y++)
                {
                    for (uint32_t x = 0; x < cur_width; x++)
                    {
                        idx = (x + 256 * w + (y + 256 * h)*fb_info.width);
                        if (idx >= bound)
                            break;
                        c32 = src32[idx];
                        r = (uint8_t)((c32 >> 24) & 0xFF);
                        r = (uint8_t)((float)r / 255.0f * 31.0f);
                        g = (uint8_t)((c32 >> 16) & 0xFF);
                        g = (uint8_t)((float)g / 255.0f * 63.0f);
                        b = (uint8_t)((c32 >> 8) & 0xFF);
                        b = (uint8_t)((float)b / 255.0f * 31.0f);
                        a = (c32 & 0xFF) ? 1 : 0;
                        *(dst++) = (a << 15) | (r << 10) | (g << 5) | b;
                    }
                    dst += cur_tail;
                }
            }
            gfxTexDownloadMipMap(tmu, tex_adr, GFX_MIPMAPLEVELMASK_BOTH, &t_info);
            gfxTexSource(tmu, tex_adr, GFX_MIPMAPLEVELMASK_BOTH, &t_info);
            tex_adr += tex_size;
            float ul_x = (float)(fb_info.ul_x + 256 * w);
            float ul_y = (float)(fb_info.ul_y + 256 * h);
            float lr_x = (ul_x + (float)(cur_width)) * rdp.scale_x;
            float lr_y = (ul_y + (float)(cur_height)) * rdp.scale_y;
            ul_x *= rdp.scale_x;
            ul_y *= rdp.scale_y;
            ul_x += rdp.offset_x;
            ul_y += rdp.offset_y;
            lr_x += rdp.offset_x;
            lr_y += rdp.offset_y;

            float lr_u = (float)(cur_width - 1);
            float lr_v = (float)(cur_height - 1);
            // Make the vertices
            gfxVERTEX v[4] = {
                { ul_x, ul_y, 1, 1, 0.5f, 0.5f, 0.5f, 0.5f, { 0.5f, 0.5f, 0.5f, 0.5f } },
                { lr_x, ul_y, 1, 1, lr_u, 0.5f, lr_u, 0.5f, { lr_u, 0.5f, lr_u, 0.5f } },
                { ul_x, lr_y, 1, 1, 0.5f, lr_v, 0.5f, lr_v, { 0.5f, lr_v, 0.5f, lr_v } },
                { lr_x, lr_y, 1, 1, lr_u, lr_v, lr_u, lr_v, { lr_u, lr_v, lr_u, lr_v } }
            };
            gfxDrawTriangle(&v[0], &v[2], &v[1]);
            gfxDrawTriangle(&v[2], &v[3], &v[1]);
        }
    }
}

bool DrawFrameBufferToScreen(FB_TO_SCREEN_INFO & fb_info)
{
    if (fb_info.width < 200 || fb_info.size < 2)
        return false;
    uint32_t width = fb_info.lr_x - fb_info.ul_x + 1;
    uint32_t height = fb_info.lr_y - fb_info.ul_y + 1;
    uint32_t max_size = 512;
    if (width > (uint32_t)max_size || height > (uint32_t)max_size)
    {
        DrawFrameBufferToScreen256(fb_info);
        return true;
    }
    WriteTrace(TraceRDP, TraceDebug, "DrawFrameBufferToScreen. ul_x=%d, ul_y=%d, lr_x=%d, lr_y=%d, size=%d, addr=%08lx", fb_info.ul_x, fb_info.ul_y, fb_info.lr_x, fb_info.lr_y, fb_info.size, fb_info.addr);
    gfxTexInfo t_info;
    uint8_t * image = gfx.RDRAM + fb_info.addr;
    uint32_t texwidth;
    float scale;
    if (width <= 256)
    {
        texwidth = 256;
        scale = 1.0f;
        t_info.smallLodLog2 = t_info.largeLodLog2 = GFX_LOD_LOG2_256;
    }
    else
    {
        texwidth = 512;
        scale = 0.5f;
        t_info.smallLodLog2 = t_info.largeLodLog2 = GFX_LOD_LOG2_512;
    }

    if (height <= (texwidth >> 1))
    {
        t_info.aspectRatioLog2 = GFX_ASPECT_LOG2_2x1;
    }
    else
    {
        t_info.aspectRatioLog2 = GFX_ASPECT_LOG2_1x1;
    }

    if (fb_info.size == 2)
    {
        uint16_t * tex = (uint16_t*)texture_buffer;
        uint16_t * dst = tex;
        uint16_t * src = (uint16_t*)image;
        src += fb_info.ul_x + fb_info.ul_y * fb_info.width;
        uint16_t c;
        uint32_t idx;
        const uint32_t bound = (BMASK + 1 - fb_info.addr) >> 1;
        bool empty = true;
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                idx = (x + y*fb_info.width) ^ 1;
                if (idx >= bound)
                    break;
                c = src[idx];
                if (c) empty = false;
                *(dst++) = (c >> 1) | ((c & 1) << 15);
            }
            dst += texwidth - width;
        }
        if (empty)
            return false;
        t_info.format = GFX_TEXFMT_ARGB_1555;
        t_info.data = tex;
    }
    else
    {
        uint32_t * tex = (uint32_t*)texture_buffer;
        uint32_t * dst = tex;
        uint32_t * src = (uint32_t*)image;
        src += fb_info.ul_x + fb_info.ul_y * fb_info.width;
        uint32_t col;
        uint32_t idx;
        const uint32_t bound = (BMASK + 1 - fb_info.addr) >> 2;
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                idx = x + y*fb_info.width;
                if (idx >= bound)
                    break;
                col = src[idx];
                *(dst++) = (col >> 8) | 0xFF000000;
            }
            dst += texwidth - width;
        }
        t_info.format = GFX_TEXFMT_ARGB_8888;
        t_info.data = tex;
    }

    gfxChipID_t tmu = SetupFBtoScreenCombiner(gfxTexTextureMemRequired(GFX_MIPMAPLEVELMASK_BOTH, &t_info), fb_info.opaque);
    gfxTexDownloadMipMap(tmu, voodoo.tex_min_addr[tmu] + voodoo.tmem_ptr[tmu], GFX_MIPMAPLEVELMASK_BOTH, &t_info);
    gfxTexSource(tmu, voodoo.tex_min_addr[tmu] + voodoo.tmem_ptr[tmu], GFX_MIPMAPLEVELMASK_BOTH, &t_info);
    if (g_settings->hacks(CSettings::hack_RE2))
    {
        DrawRE2Video(fb_info, scale);
    }
    else
    {
        float ul_x = fb_info.ul_x * rdp.scale_x + rdp.offset_x;
        float ul_y = fb_info.ul_y * rdp.scale_y + rdp.offset_y;
        float lr_x = fb_info.lr_x * rdp.scale_x + rdp.offset_x;
        float lr_y = fb_info.lr_y * rdp.scale_y + rdp.offset_y;
        float lr_u = (width - 1)*scale;
        float lr_v = (height - 1)*scale;
        // Make the vertices
        gfxVERTEX v[4] = {
            { ul_x, ul_y, 1, 1, 0.5f, 0.5f, 0.5f, 0.5f, { 0.5f, 0.5f, 0.5f, 0.5f } },
            { lr_x, ul_y, 1, 1, lr_u, 0.5f, lr_u, 0.5f, { lr_u, 0.5f, lr_u, 0.5f } },
            { ul_x, lr_y, 1, 1, 0.5f, lr_v, 0.5f, lr_v, { 0.5f, lr_v, 0.5f, lr_v } },
            { lr_x, lr_y, 1, 1, lr_u, lr_v, lr_u, lr_v, { lr_u, lr_v, lr_u, lr_v } }
        };
        gfxDrawTriangle(&v[0], &v[2], &v[1]);
        gfxDrawTriangle(&v[2], &v[3], &v[1]);
    }
    return true;
}

static void DrawDepthBufferToScreen256(FB_TO_SCREEN_INFO & fb_info)
{
    WriteTrace(TraceRDP, TraceDebug, "DrawDepthBufferToScreen256. ul_x=%d, ul_y=%d, lr_x=%d, lr_y=%d, size=%d, addr=%08lx", fb_info.ul_x, fb_info.ul_y, fb_info.lr_x, fb_info.lr_y, fb_info.size, fb_info.addr);
    uint32_t width = fb_info.lr_x - fb_info.ul_x + 1;
    uint32_t height = fb_info.lr_y - fb_info.ul_y + 1;
    gfxTexInfo t_info;
    uint8_t * image = gfx.RDRAM + fb_info.addr;
    uint32_t width256 = ((width - 1) >> 8) + 1;
    uint32_t height256 = ((height - 1) >> 8) + 1;
    t_info.smallLodLog2 = t_info.largeLodLog2 = GFX_LOD_LOG2_256;
    t_info.aspectRatioLog2 = GFX_ASPECT_LOG2_1x1;
    t_info.format = GFX_TEXFMT_ALPHA_INTENSITY_88;
    uint16_t * tex = (uint16_t*)texture_buffer;
    t_info.data = tex;
    uint32_t tex_size = gfxTexTextureMemRequired(GFX_MIPMAPLEVELMASK_BOTH, &t_info);
    gfxChipID_t tmu = SetupFBtoScreenCombiner(tex_size*width256*height256, fb_info.opaque);
    gfxConstantColorValue(rdp.fog_color);
    gfxColorCombine(GFX_COMBINE_FUNCTION_SCALE_OTHER,
        GFX_COMBINE_FACTOR_ONE,
        GFX_COMBINE_LOCAL_NONE,
        GFX_COMBINE_OTHER_CONSTANT,
        false);
    uint16_t * src = (uint16_t*)image;
    src += fb_info.ul_x + fb_info.ul_y * fb_info.width;
    uint32_t w_tail = width % 256;
    uint32_t h_tail = height % 256;
    uint32_t cur_width, cur_height, cur_tail;
    uint32_t tex_adr = voodoo.tex_min_addr[tmu] + voodoo.tmem_ptr[tmu];
    if ((voodoo.tmem_ptr[tmu] < TEXMEM_2MB_EDGE) && (voodoo.tmem_ptr[tmu] + tex_size*width256*height256 > TEXMEM_2MB_EDGE))
    {
        tex_adr = TEXMEM_2MB_EDGE;
    }
    for (uint32_t h = 0; h < height256; h++)
    {
        for (uint32_t w = 0; w < width256; w++)
        {
            cur_width = (256 * (w + 1) < width) ? 256 : w_tail;
            cur_height = (256 * (h + 1) < height) ? 256 : h_tail;
            cur_tail = 256 - cur_width;
            uint16_t * dst = tex;
            for (uint32_t y = 0; y < cur_height; y++)
            {
                for (uint32_t x = 0; x < cur_width; x++)
                {
                    *(dst++) = rdp.pal_8[src[(x + 256 * w + (y + 256 * h)*fb_info.width) ^ 1] >> 8];
                }
                dst += cur_tail;
            }
            gfxTexDownloadMipMap(tmu, tex_adr, GFX_MIPMAPLEVELMASK_BOTH, &t_info);
            gfxTexSource(tmu, tex_adr, GFX_MIPMAPLEVELMASK_BOTH, &t_info);
            tex_adr += tex_size;
            float ul_x = (float)(fb_info.ul_x + 256 * w);
            float ul_y = (float)(fb_info.ul_y + 256 * h);
            float lr_x = (ul_x + (float)(cur_width)) * rdp.scale_x + rdp.offset_x;
            float lr_y = (ul_y + (float)(cur_height)) * rdp.scale_y + rdp.offset_y;
            ul_x = ul_x * rdp.scale_x + rdp.offset_x;
            ul_y = ul_y * rdp.scale_y + rdp.offset_y;
            float lr_u = (float)(cur_width - 1);
            float lr_v = (float)(cur_height - 1);
            // Make the vertices
            gfxVERTEX v[4] = {
                { ul_x, ul_y, 1, 1, 0.5f, 0.5f, 0.5f, 0.5f, { 0.5f, 0.5f, 0.5f, 0.5f } },
                { lr_x, ul_y, 1, 1, lr_u, 0.5f, lr_u, 0.5f, { lr_u, 0.5f, lr_u, 0.5f } },
                { ul_x, lr_y, 1, 1, 0.5f, lr_v, 0.5f, lr_v, { 0.5f, lr_v, 0.5f, lr_v } },
                { lr_x, lr_y, 1, 1, lr_u, lr_v, lr_u, lr_v, { lr_u, lr_v, lr_u, lr_v } }
            };
            gfxDrawTriangle(&v[0], &v[2], &v[1]);
            gfxDrawTriangle(&v[2], &v[3], &v[1]);
        }
    }
}

static void DrawHiresDepthBufferToScreen(FB_TO_SCREEN_INFO & fb_info)
{
    WriteTrace(TraceRDP, TraceDebug, "DrawHiresDepthBufferToScreen. ul_x=%d, ul_y=%d, lr_x=%d, lr_y=%d, size=%d, addr=%08lx", fb_info.ul_x, fb_info.ul_y, fb_info.lr_x, fb_info.lr_y, fb_info.size, fb_info.addr);
    gfxTexInfo t_info;
    float scale = 0.25f;
    gfxLOD_t LOD = GFX_LOD_LOG2_1024;
    if (g_scr_res_x > 1024)
    {
        scale = 0.125f;
        LOD = GFX_LOD_LOG2_2048;
    }
    t_info.format = GFX_TEXFMT_ALPHA_INTENSITY_88;
    t_info.smallLodLog2 = t_info.largeLodLog2 = LOD;
    t_info.aspectRatioLog2 = GFX_ASPECT_LOG2_1x1;
    gfxConstantColorValue(rdp.fog_color);
    gfxColorCombine(GFX_COMBINE_FUNCTION_LOCAL,
        GFX_COMBINE_FACTOR_NONE,
        GFX_COMBINE_LOCAL_CONSTANT,
        GFX_COMBINE_OTHER_NONE,
        false);
    gfxAlphaCombine(GFX_COMBINE_FUNCTION_SCALE_OTHER,
        GFX_COMBINE_FACTOR_ONE,
        GFX_COMBINE_LOCAL_NONE,
        GFX_COMBINE_OTHER_TEXTURE,
        false);
    gfxAlphaBlendFunction(GFX_BLEND_SRC_ALPHA,
        GFX_BLEND_ONE_MINUS_SRC_ALPHA,
        GFX_BLEND_ONE,
        GFX_BLEND_ZERO);
    gfxDepthBufferFunction(GFX_CMP_ALWAYS);
    gfxDepthMask(false);
    gfxCullMode(GFX_CULL_DISABLE);
    gfxTexCombine(GFX_TMU1,
        GFX_COMBINE_FUNCTION_NONE,
        GFX_COMBINE_FACTOR_NONE,
        GFX_COMBINE_FUNCTION_NONE,
        GFX_COMBINE_FACTOR_NONE,
        false,
        false);
    gfxTexCombine(GFX_TMU0,
        GFX_COMBINE_FUNCTION_LOCAL,
        GFX_COMBINE_FACTOR_NONE,
        GFX_COMBINE_FUNCTION_LOCAL,
        GFX_COMBINE_FACTOR_NONE,
        false,
        false);
    //  gfxAuxBufferExt( GFX_BUFFER_AUXBUFFER );
    gfxTexSource(rdp.texbufs[0].tmu, rdp.texbufs[0].begin, GFX_MIPMAPLEVELMASK_BOTH, &(t_info));
    float ul_x = (float)rdp.scissor.ul_x;
    float ul_y = (float)rdp.scissor.ul_y;
    float lr_x = (float)rdp.scissor.lr_x;
    float lr_y = (float)rdp.scissor.lr_y;
    float ul_u = (float)rdp.scissor.ul_x * scale;
    float ul_v = (float)rdp.scissor.ul_y * scale;
    float lr_u = (float)rdp.scissor.lr_x * scale;
    float lr_v = (float)rdp.scissor.lr_y * scale;
    // Make the vertices
    gfxVERTEX v[4] = {
        { ul_x, ul_y, 1, 1, ul_u, ul_v, ul_u, ul_v, { ul_u, ul_v, ul_u, ul_v } },
        { lr_x, ul_y, 1, 1, lr_u, ul_v, lr_u, ul_v, { lr_u, ul_v, lr_u, ul_v } },
        { ul_x, lr_y, 1, 1, ul_u, lr_v, ul_u, lr_v, { ul_u, lr_v, ul_u, lr_v } },
        { lr_x, lr_y, 1, 1, lr_u, lr_v, lr_u, lr_v, { lr_u, lr_v, lr_u, lr_v } }
    };
    gfxDrawTriangle(&v[0], &v[2], &v[1]);
    gfxDrawTriangle(&v[2], &v[3], &v[1]);
    //  gfxAuxBufferExt( GFX_BUFFER_TEXTUREAUXBUFFER_EXT );
    rdp.update |= UPDATE_COMBINE | UPDATE_ZBUF_ENABLED | UPDATE_CULL_MODE;
}

void DrawDepthBufferToScreen(FB_TO_SCREEN_INFO & fb_info)
{
    uint32_t width = fb_info.lr_x - fb_info.ul_x + 1;
    uint32_t height = fb_info.lr_y - fb_info.ul_y + 1;
    if (width > (uint32_t)2048 || height > (uint32_t)2048 || width > 512)
    {
        DrawDepthBufferToScreen256(fb_info);
        return;
    }
    WriteTrace(TraceRDP, TraceDebug, "DrawDepthBufferToScreen. ul_x=%d, ul_y=%d, lr_x=%d, lr_y=%d, size=%d, addr=%08lx", fb_info.ul_x, fb_info.ul_y, fb_info.lr_x, fb_info.lr_y, fb_info.size, fb_info.addr);
    gfxTexInfo t_info;
    uint8_t * image = gfx.RDRAM + fb_info.addr;
    uint32_t texwidth;
    float scale;
    if (width <= 256)
    {
        texwidth = 256;
        scale = 1.0f;
        t_info.smallLodLog2 = t_info.largeLodLog2 = GFX_LOD_LOG2_256;
    }
    else
    {
        texwidth = 512;
        scale = 0.5f;
        t_info.smallLodLog2 = t_info.largeLodLog2 = GFX_LOD_LOG2_512;
    }

    if (height <= (texwidth >> 1))
    {
        t_info.aspectRatioLog2 = GFX_ASPECT_LOG2_2x1;
    }
    else
    {
        t_info.aspectRatioLog2 = GFX_ASPECT_LOG2_1x1;
    }

    uint16_t * tex = (uint16_t*)texture_buffer;
    uint16_t * dst = tex;
    uint16_t * src = (uint16_t*)image;
    src += fb_info.ul_x + fb_info.ul_y * fb_info.width;
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            *(dst++) = rdp.pal_8[src[(x + y*fb_info.width) ^ 1] >> 8];
        }
        dst += texwidth - width;
    }
    t_info.format = GFX_TEXFMT_ALPHA_INTENSITY_88;
    t_info.data = tex;

    gfxChipID_t tmu = SetupFBtoScreenCombiner(gfxTexTextureMemRequired(GFX_MIPMAPLEVELMASK_BOTH, &t_info), fb_info.opaque);
    gfxConstantColorValue(rdp.fog_color);
    gfxColorCombine(GFX_COMBINE_FUNCTION_SCALE_OTHER,
        GFX_COMBINE_FACTOR_ONE,
        GFX_COMBINE_LOCAL_NONE,
        GFX_COMBINE_OTHER_CONSTANT,
        false);
    gfxTexDownloadMipMap(tmu, voodoo.tex_min_addr[tmu] + voodoo.tmem_ptr[tmu], GFX_MIPMAPLEVELMASK_BOTH, &t_info);
    gfxTexSource(tmu, voodoo.tex_min_addr[tmu] + voodoo.tmem_ptr[tmu], GFX_MIPMAPLEVELMASK_BOTH, &t_info);
    float ul_x = fb_info.ul_x * rdp.scale_x + rdp.offset_x;
    float ul_y = fb_info.ul_y * rdp.scale_y + rdp.offset_y;
    float lr_x = fb_info.lr_x * rdp.scale_x + rdp.offset_x;
    float lr_y = fb_info.lr_y * rdp.scale_y + rdp.offset_y;
    float lr_u = (width - 1)*scale;
    float lr_v = (height - 1)*scale;
    float zero = scale*0.5f;
    // Make the vertices
    gfxVERTEX v[4] = {
        { ul_x, ul_y, 1, 1, zero, zero, zero, zero, { zero, zero, zero, zero } },
        { lr_x, ul_y, 1, 1, lr_u, zero, lr_u, zero, { lr_u, zero, lr_u, zero } },
        { ul_x, lr_y, 1, 1, zero, lr_v, zero, lr_v, { zero, lr_v, zero, lr_v } },
        { lr_x, lr_y, 1, 1, lr_u, lr_v, lr_u, lr_v, { lr_u, lr_v, lr_u, lr_v } }
    };
    gfxDrawTriangle(&v[0], &v[2], &v[1]);
    gfxDrawTriangle(&v[2], &v[3], &v[1]);
}