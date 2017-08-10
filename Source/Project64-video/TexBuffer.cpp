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
#include <string.h>
#include "Gfx_1.3.h"
#include "TexBuffer.h"
#include "CRC.h"
#include <Project64-video/trace.h>

static TBUFF_COLOR_IMAGE * AllocateTextureBuffer(COLOR_IMAGE & cimage)
{
    TBUFF_COLOR_IMAGE texbuf;
    texbuf.addr = cimage.addr;
    texbuf.end_addr = cimage.addr + ((cimage.width*cimage.height) << cimage.size >> 1);
    texbuf.width = cimage.width;
    texbuf.height = cimage.height;
    texbuf.format = cimage.format;
    texbuf.size = cimage.size;
    texbuf.scr_width = minval(cimage.width * rdp.scale_x, g_scr_res_x);
    float height = minval(rdp.vi_height, cimage.height);
    if (cimage.status == ci_copy_self || (cimage.status == ci_copy && cimage.width == rdp.frame_buffers[rdp.main_ci_index].width))
        height = rdp.vi_height;
    texbuf.scr_height = height * rdp.scale_y;
    //  texbuf.scr_height = texbuf.height * rdp.scale_y;

    uint16_t max_size = maxval((uint16_t)texbuf.scr_width, (uint16_t)texbuf.scr_height);
    if (max_size > voodoo.max_tex_size) //texture size is too large
        return 0;
    uint32_t tex_size;
    //calculate LOD
    switch ((max_size - 1) >> 6)
    {
    case 0:
        texbuf.info.smallLodLog2 = texbuf.info.largeLodLog2 = GFX_LOD_LOG2_64;
        tex_size = 64;
        break;
    case 1:
        texbuf.info.smallLodLog2 = texbuf.info.largeLodLog2 = GFX_LOD_LOG2_128;
        tex_size = 128;
        break;
    case 2:
    case 3:
        texbuf.info.smallLodLog2 = texbuf.info.largeLodLog2 = GFX_LOD_LOG2_256;
        tex_size = 256;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        texbuf.info.smallLodLog2 = texbuf.info.largeLodLog2 = GFX_LOD_LOG2_512;
        tex_size = 512;
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        texbuf.info.smallLodLog2 = texbuf.info.largeLodLog2 = GFX_LOD_LOG2_1024;
        tex_size = 1024;
        break;
    default:
        texbuf.info.smallLodLog2 = texbuf.info.largeLodLog2 = GFX_LOD_LOG2_2048;
        tex_size = 2048;
    }
    //calculate aspect
    if (texbuf.scr_width >= texbuf.scr_height)
    {
        if ((texbuf.scr_width / texbuf.scr_height) >= 2)
        {
            texbuf.info.aspectRatioLog2 = GFX_ASPECT_LOG2_2x1;
            texbuf.tex_width = tex_size;
            texbuf.tex_height = tex_size >> 1;
        }
        else
        {
            texbuf.info.aspectRatioLog2 = GFX_ASPECT_LOG2_1x1;
            texbuf.tex_width = texbuf.tex_height = tex_size;
        }
    }
    else
    {
        if ((texbuf.scr_height / texbuf.scr_width) >= 2)
        {
            texbuf.info.aspectRatioLog2 = GFX_ASPECT_LOG2_1x2;
            texbuf.tex_width = tex_size >> 1;
            texbuf.tex_height = tex_size;
        }
        else
        {
            texbuf.info.aspectRatioLog2 = GFX_ASPECT_LOG2_1x1;
            texbuf.tex_width = texbuf.tex_height = tex_size;
        }
    }
    if ((cimage.format != 0))// && (cimage.width <= 64))
        texbuf.info.format = GFX_TEXFMT_ALPHA_INTENSITY_88;
    else
        texbuf.info.format = GFX_TEXFMT_RGB_565;

    texbuf.lr_u = 256.0f * texbuf.scr_width / (float)tex_size;// + 1.0f;
    texbuf.lr_v = 256.0f * texbuf.scr_height / (float)tex_size;// + 1.0f;
    texbuf.tile = 0;
    texbuf.tile_uls = 0;
    texbuf.tile_ult = 0;
    texbuf.u_shift = 0;
    texbuf.v_shift = 0;
    texbuf.drawn = FALSE;
    texbuf.u_scale = texbuf.lr_u / (float)(texbuf.width);
    texbuf.v_scale = texbuf.lr_v / (float)(texbuf.height);
    texbuf.cache = 0;
    texbuf.crc = 0;
    texbuf.t_mem = 0;

    WriteTrace(TraceRDP, TraceDebug, "\nAllocateTextureBuffer. width: %d, height: %d, scr_width: %f, scr_height: %f, vi_width: %f, vi_height:%f, scale_x: %f, scale_y: %f, lr_u: %f, lr_v: %f, u_scale: %f, v_scale: %f", texbuf.width, texbuf.height, texbuf.scr_width, texbuf.scr_height, rdp.vi_width, rdp.vi_height, rdp.scale_x, rdp.scale_y, texbuf.lr_u, texbuf.lr_v, texbuf.u_scale, texbuf.v_scale);

    uint32_t required = gfxTexCalcMemRequired(texbuf.info.smallLodLog2, texbuf.info.largeLodLog2,
        texbuf.info.aspectRatioLog2, texbuf.info.format);
    //find free space
    for (int i = 0; i < (nbTextureUnits > 2 ? 2 : 1); i++)
    {
        uint32_t available = 0;
        uint32_t top = 0;
        if (rdp.texbufs[i].count)
        {
            TBUFF_COLOR_IMAGE & t = rdp.texbufs[i].images[rdp.texbufs[i].count - 1];
            if (rdp.read_whole_frame || rdp.motionblur)
            {
                if ((cimage.status == ci_aux) && (rdp.cur_tex_buf == i))
                {
                    top = t.tex_addr + t.tex_width * (int)(t.scr_height + 1) * 2;
                    if (rdp.texbufs[i].end - top < required)
                        return 0;
                }
                else
                    top = rdp.texbufs[i].end;
            }
            else
                top = t.tex_addr + t.tex_width * t.tex_height * 2;
            available = rdp.texbufs[i].end - top;
        }
        else
        {
            available = rdp.texbufs[i].end - rdp.texbufs[i].begin;
            top = rdp.texbufs[i].begin;
        }
        if (available >= required)
        {
            rdp.texbufs[i].count++;
            rdp.texbufs[i].clear_allowed = FALSE;
            texbuf.tex_addr = top;
            rdp.cur_tex_buf = i;
            texbuf.tmu = rdp.texbufs[i].tmu;
            rdp.texbufs[i].images[rdp.texbufs[i].count - 1] = texbuf;
            return &(rdp.texbufs[i].images[rdp.texbufs[i].count - 1]);
        }
    }
    //not found. keep recently accessed bank, clear second one
    if (!rdp.texbufs[rdp.cur_tex_buf ^ 1].clear_allowed) //can't clear => can't allocate
        return 0;
    rdp.cur_tex_buf ^= 1;
    rdp.texbufs[rdp.cur_tex_buf].count = 1;
    rdp.texbufs[rdp.cur_tex_buf].clear_allowed = FALSE;
    texbuf.tmu = rdp.texbufs[rdp.cur_tex_buf].tmu;
    texbuf.tex_addr = rdp.texbufs[rdp.cur_tex_buf].begin;
    rdp.texbufs[rdp.cur_tex_buf].images[0] = texbuf;
    return &(rdp.texbufs[rdp.cur_tex_buf].images[0]);
}

int OpenTextureBuffer(COLOR_IMAGE & cimage)
{
    WriteTrace(TraceRDP, TraceDebug, "OpenTextureBuffer. cur_tex_buf: %d, addr: %08lx, width: %d, height: %d", rdp.cur_tex_buf, cimage.addr, cimage.width, cimage.height);

    int found = FALSE, search = TRUE;
    TBUFF_COLOR_IMAGE *texbuf = 0;
    uint32_t addr = cimage.addr;
    if (g_settings->hacks(CSettings::hack_Banjo2) && cimage.status == ci_copy_self)
        addr = rdp.frame_buffers[rdp.copy_ci_index].addr;
    uint32_t end_addr = addr + ((cimage.width*cimage.height) << cimage.size >> 1);
    if (rdp.motionblur)
    {
        //    if (cimage.format != 0)
        //      return FALSE;
        search = FALSE;
    }
    if (rdp.read_whole_frame)
    {
        if (g_settings->hacks(CSettings::hack_PMario)) //motion blur effects in Paper Mario
        {
            rdp.cur_tex_buf = rdp.acc_tex_buf;
            WriteTrace(TraceRDP, TraceDebug, "\nread_whole_frame. last allocated bank: %d", rdp.acc_tex_buf);
        }
        else
        {
            if (!rdp.texbufs[0].clear_allowed || !rdp.texbufs[1].clear_allowed)
            {
                if (cimage.status == ci_main)
                {
                    texbuf = &(rdp.texbufs[rdp.cur_tex_buf].images[0]);
                    found = TRUE;
                }
                else
                {
                    for (int t = 0; (t < rdp.texbufs[rdp.cur_tex_buf].count) && !found; t++)
                    {
                        texbuf = &(rdp.texbufs[rdp.cur_tex_buf].images[t]);
                        if (addr == texbuf->addr && cimage.width == texbuf->width)
                        {
                            texbuf->drawn = FALSE;
                            found = TRUE;
                        }
                    }
                }
            }
            search = FALSE;
        }
    }
    if (search)
    {
        for (int i = 0; (i < (nbTextureUnits > 2 ? 2 : 1)) && !found; i++)
        {
            for (int j = 0; (j < rdp.texbufs[i].count) && !found; j++)
            {
                texbuf = &(rdp.texbufs[i].images[j]);
                if (addr == texbuf->addr && cimage.width == texbuf->width)
                {
                    //texbuf->height = cimage.height;
                    //texbuf->end_addr = end_addr;
                    texbuf->drawn = FALSE;
                    texbuf->format = (uint16_t)cimage.format;
                    if ((cimage.format != 0))
                        texbuf->info.format = GFX_TEXFMT_ALPHA_INTENSITY_88;
                    else
                        texbuf->info.format = GFX_TEXFMT_RGB_565;
                    texbuf->crc = 0;
                    texbuf->t_mem = 0;
                    texbuf->tile = 0;
                    found = TRUE;
                    rdp.cur_tex_buf = i;
                    rdp.texbufs[i].clear_allowed = FALSE;
                }
                else //check intersection
                {
                    if (!((end_addr <= texbuf->addr) || (addr >= texbuf->end_addr))) //intersected, remove
                    {
                        gfxRenderBuffer(GFX_BUFFER_TEXTUREBUFFER_EXT);
                        gfxTextureBufferExt(texbuf->tmu, texbuf->tex_addr, texbuf->info.smallLodLog2, texbuf->info.largeLodLog2,
                            texbuf->info.aspectRatioLog2, texbuf->info.format, GFX_MIPMAPLEVELMASK_BOTH);
                        gfxDepthMask(false);
                        gfxBufferClear(0, 0, 0xFFFF);
                        gfxDepthMask(true);
                        gfxRenderBuffer(GFX_BUFFER_BACKBUFFER);
                        rdp.texbufs[i].count--;
                        if (j < rdp.texbufs[i].count)
                            memmove(&(rdp.texbufs[i].images[j]), &(rdp.texbufs[i].images[j + 1]), sizeof(TBUFF_COLOR_IMAGE)*(rdp.texbufs[i].count - j));
                    }
                }
            }
        }
    }
    else
    {
        WriteTrace(TraceRDP, TraceDebug, "  not searched");
    }

    if (!found)
    {
        WriteTrace(TraceRDP, TraceDebug, "  not found");
        texbuf = AllocateTextureBuffer(cimage);
    }
    else
    {
        WriteTrace(TraceRDP, TraceDebug, "  found");
    }

    if (!texbuf)
    {
        WriteTrace(TraceRDP, TraceDebug, "  KO");
        return FALSE;
    }

    rdp.acc_tex_buf = rdp.cur_tex_buf;
    rdp.cur_image = texbuf;
    gfxRenderBuffer(GFX_BUFFER_TEXTUREBUFFER_EXT);
    gfxTextureBufferExt(rdp.cur_image->tmu, rdp.cur_image->tex_addr, rdp.cur_image->info.smallLodLog2, rdp.cur_image->info.largeLodLog2,
        rdp.cur_image->info.aspectRatioLog2, rdp.cur_image->info.format, GFX_MIPMAPLEVELMASK_BOTH);

    if (rdp.cur_image->clear && g_settings->fb_hwfbe_buf_clear_enabled() && cimage.changed)
    {
        rdp.cur_image->clear = FALSE;
        gfxDepthMask(false);
        gfxBufferClear(0, 0, 0xFFFF);
        gfxDepthMask(true);
    }
    //*/
    //  memset(gfx.RDRAM+cimage.addr, 0, cimage.width*cimage.height*cimage.size);
    WriteTrace(TraceRDP, TraceDebug, "  texaddr: %08lx, tex_width: %d, tex_height: %d, cur_tex_buf: %d, texformat: %d, motionblur: %d", rdp.cur_image->tex_addr, rdp.cur_image->tex_width, rdp.cur_image->tex_height, rdp.cur_tex_buf, rdp.cur_image->info.format, rdp.motionblur);
    if (!rdp.offset_x_bak)
    {
        rdp.offset_x_bak = rdp.offset_x;
        rdp.offset_x = 0;
    }
    if (!rdp.offset_y_bak)
    {
        rdp.offset_y_bak = rdp.offset_y;
        rdp.offset_y = 0;
    }
    rdp.update |= UPDATE_VIEWPORT | UPDATE_SCISSOR;
    return TRUE;
}

static gfxTextureFormat_t TexBufSetupCombiner(int force_rgb = FALSE)
{
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
    //  gfxConstantColorValue (0xFFFFFFFF);
    gfxAlphaBlendFunction(GFX_BLEND_ONE,	// use alpha compare, but not T0 alpha
        GFX_BLEND_ZERO,
        GFX_BLEND_ONE,
        GFX_BLEND_ZERO);
    gfxClipWindow(0, 0, g_scr_res_x, g_scr_res_y);
    gfxDepthBufferFunction(GFX_CMP_ALWAYS);
    gfxDepthMask(false);
    gfxCullMode(GFX_CULL_DISABLE);
    gfxFogMode(GFX_FOG_DISABLE);
    gfxTextureFormat_t buf_format = (rdp.tbuff_tex) ? rdp.tbuff_tex->info.format : GFX_TEXFMT_RGB_565;
    gfxCombineFunction_t color_source = GFX_COMBINE_FUNCTION_LOCAL;
    if (!force_rgb && rdp.black_ci_index > 0 && rdp.black_ci_index <= rdp.copy_ci_index)
    {
        color_source = GFX_COMBINE_FUNCTION_LOCAL_ALPHA;
        buf_format = GFX_TEXFMT_ALPHA_INTENSITY_88;
    }
    if (rdp.tbuff_tex->tmu == GFX_TMU0)
    {
        gfxTexCombine(GFX_TMU1,
            GFX_COMBINE_FUNCTION_NONE,
            GFX_COMBINE_FACTOR_NONE,
            GFX_COMBINE_FUNCTION_NONE,
            GFX_COMBINE_FACTOR_NONE,
            false,
            false);
        gfxTexCombine(GFX_TMU0,
            color_source,
            GFX_COMBINE_FACTOR_NONE,
            GFX_COMBINE_FUNCTION_ZERO,
            GFX_COMBINE_FACTOR_NONE,
            false,
            true);
    }
    else
    {
        gfxTexCombine(GFX_TMU1,
            color_source,
            GFX_COMBINE_FACTOR_NONE,
            GFX_COMBINE_FUNCTION_ZERO,
            GFX_COMBINE_FACTOR_NONE,
            false,
            true);
        gfxTexCombine(GFX_TMU0,
            GFX_COMBINE_FUNCTION_SCALE_OTHER,
            GFX_COMBINE_FACTOR_ONE,
            GFX_COMBINE_FUNCTION_SCALE_OTHER,
            GFX_COMBINE_FACTOR_ONE,
            false,
            false);
    }
    return buf_format;
}

int CloseTextureBuffer(int draw)
{
    if (!rdp.cur_image)
    {
        WriteTrace(TraceRDP, TraceDebug, "CloseTextureBuffer KO");
        return FALSE;
    }
    gfxRenderBuffer(GFX_BUFFER_BACKBUFFER);
    rdp.offset_x = rdp.offset_x_bak;
    rdp.offset_y = rdp.offset_y_bak;
    rdp.offset_x_bak = rdp.offset_y_bak = 0;
    rdp.update |= UPDATE_VIEWPORT | UPDATE_SCISSOR;
    if (!draw)
    {
        WriteTrace(TraceRDP, TraceDebug, "CloseTextureBuffer no draw, OK");
        rdp.cur_image = 0;
        return TRUE;
    }
    rdp.tbuff_tex = rdp.cur_image;
    rdp.cur_image = 0;
    rdp.tbuff_tex->info.format = TexBufSetupCombiner();
    float zero = 0.0f;
    float ul_x = rdp.offset_x;
    float ul_y = rdp.offset_y;
    float lr_x = rdp.tbuff_tex->scr_width + rdp.offset_x;
    float lr_y = rdp.tbuff_tex->scr_height + rdp.offset_y;
    float lr_u = rdp.tbuff_tex->lr_u;
    float lr_v = rdp.tbuff_tex->lr_v;
    WriteTrace(TraceRDP, TraceDebug, "lr_x: %f, lr_y: %f, lr_u: %f, lr_v: %f", lr_x, lr_y, lr_u, lr_v);

    // Make the vertices
    VERTEX v[4] = {
        { ul_x, ul_y, 1, 1, zero, zero, zero, zero, { zero, zero, zero, zero } },
        { lr_x, ul_y, 1, 1, lr_u, zero, lr_u, zero, { lr_u, zero, lr_u, zero } },
        { ul_x, lr_y, 1, 1, zero, lr_v, zero, lr_v, { zero, lr_v, zero, lr_v } },
        { lr_x, lr_y, 1, 1, lr_u, lr_v, lr_u, lr_v, { lr_u, lr_v, lr_u, lr_v } }
    };

    gfxTexSource(rdp.tbuff_tex->tmu, rdp.tbuff_tex->tex_addr, GFX_MIPMAPLEVELMASK_BOTH, &(rdp.tbuff_tex->info));
    gfxClipWindow(0, 0, g_res_x, g_res_y);
    gfxDrawTriangle(&v[0], &v[2], &v[1]);
    gfxDrawTriangle(&v[2], &v[3], &v[1]);
    rdp.update |= UPDATE_ZBUF_ENABLED | UPDATE_COMBINE | UPDATE_TEXTURE | UPDATE_ALPHA_COMPARE;
    if (g_settings->fog() && (rdp.flags & FOG_ENABLED))
    {
        gfxFogMode(GFX_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
    }
    WriteTrace(TraceRDP, TraceDebug, "CloseTextureBuffer draw, OK");
    rdp.tbuff_tex = 0;
    return TRUE;
}

int CopyTextureBuffer(COLOR_IMAGE & fb_from, COLOR_IMAGE & fb_to)
{
    WriteTrace(TraceRDP, TraceDebug, "CopyTextureBuffer from %08x to %08x", fb_from.addr, fb_to.addr);
    if (rdp.cur_image)
    {
        rdp.cur_image->crc = 0;
        if (rdp.cur_image->addr == fb_to.addr)
            return CloseTextureBuffer(TRUE);
        rdp.tbuff_tex = rdp.cur_image;
    }
    else if (!FindTextureBuffer(fb_from.addr, (uint16_t)fb_from.width))
    {
        WriteTrace(TraceRDP, TraceDebug, "Can't find 'from' buffer.");
        return FALSE;
    }
    if (!OpenTextureBuffer(fb_to))
    {
        WriteTrace(TraceRDP, TraceDebug, "Can't open new buffer.");
        return CloseTextureBuffer(TRUE);
    }
    rdp.tbuff_tex->crc = 0;
    gfxTextureFormat_t buf_format = rdp.tbuff_tex->info.format;
    rdp.tbuff_tex->info.format = GFX_TEXFMT_RGB_565;
    TexBufSetupCombiner(TRUE);
    float ul_x = 0.0f;
    float ul_y = 0.0f;
    float lr_x = rdp.tbuff_tex->scr_width;
    float lr_y = rdp.tbuff_tex->scr_height;
    float zero = 0.0f;
    float lr_u = rdp.tbuff_tex->lr_u;
    float lr_v = rdp.tbuff_tex->lr_v;
    WriteTrace(TraceRDP, TraceDebug, "lr_x: %f, lr_y: %f", lr_x, lr_y);

    // Make the vertices
    VERTEX v[4] = {
        { ul_x, ul_y, 1, 1, zero, zero, zero, zero, { zero, zero, zero, zero } },
        { lr_x, ul_y, 1, 1, lr_u, zero, lr_u, zero, { lr_u, zero, lr_u, zero } },
        { ul_x, lr_y, 1, 1, zero, lr_v, zero, lr_v, { zero, lr_v, zero, lr_v } },
        { lr_x, lr_y, 1, 1, lr_u, lr_v, lr_u, lr_v, { lr_u, lr_v, lr_u, lr_v } }
    };

    gfxTexSource(rdp.tbuff_tex->tmu, rdp.tbuff_tex->tex_addr, GFX_MIPMAPLEVELMASK_BOTH, &(rdp.tbuff_tex->info));
    gfxDrawTriangle(&v[0], &v[2], &v[1]);
    gfxDrawTriangle(&v[2], &v[3], &v[1]);
    gfxRenderBuffer(GFX_BUFFER_BACKBUFFER);
    rdp.offset_x = rdp.offset_x_bak;
    rdp.offset_y = rdp.offset_y_bak;
    rdp.offset_x_bak = rdp.offset_y_bak = 0;
    AddOffset(v, 4);
    gfxClipWindow(0, 0, g_res_x, g_res_y);
    gfxDrawTriangle(&v[0], &v[2], &v[1]);
    gfxDrawTriangle(&v[2], &v[3], &v[1]);
    rdp.tbuff_tex->info.format = buf_format;

    rdp.update |= UPDATE_ZBUF_ENABLED | UPDATE_COMBINE | UPDATE_TEXTURE | UPDATE_ALPHA_COMPARE;
    rdp.update |= UPDATE_VIEWPORT | UPDATE_SCISSOR;
    if (g_settings->fog() && (rdp.flags & FOG_ENABLED))
        gfxFogMode(GFX_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
    WriteTrace(TraceRDP, TraceDebug, "CopyTextureBuffer draw, OK");
    rdp.tbuff_tex = 0;
    rdp.cur_image = 0;
    return TRUE;
}

int CopyDepthBuffer()
{
    WriteTrace(TraceRDP, TraceDebug, "CopyDepthBuffer. ");
    float bound = 1024.0f;
    gfxLOD_t LOD = GFX_LOD_LOG2_1024;
    if (g_scr_res_x > 1024)
    {
        bound = 2048.0f;
        LOD = GFX_LOD_LOG2_2048;
    }
    rdp.tbuff_tex = &(rdp.texbufs[0].images[0]);
    rdp.tbuff_tex->tmu = rdp.texbufs[0].tmu;
    rdp.tbuff_tex->info.format = GFX_TEXFMT_RGB_565;
    rdp.tbuff_tex->info.smallLodLog2 = rdp.tbuff_tex->info.largeLodLog2 = LOD;
    rdp.tbuff_tex->info.aspectRatioLog2 = GFX_ASPECT_LOG2_1x1;
    TexBufSetupCombiner(TRUE);
    float ul_x = 0.0f;
    float ul_y = 0.0f;
    float lr_x = bound;
    float lr_y = bound;
    float zero = 0.0f;
    float lr_u = 255.5f;
    float lr_v = 255.5f;
    WriteTrace(TraceRDP, TraceDebug, "lr_x: %f, lr_y: %f", lr_x, lr_y);

    // Make the vertices
    VERTEX v[4] = {
        { ul_x, ul_y, 1, 1, zero, zero, zero, zero, { zero, zero, zero, zero } },
        { lr_x, ul_y, 1, 1, lr_u, zero, lr_u, zero, { lr_u, zero, lr_u, zero } },
        { ul_x, lr_y, 1, 1, zero, lr_v, zero, lr_v, { zero, lr_v, zero, lr_v } },
        { lr_x, lr_y, 1, 1, lr_u, lr_v, lr_u, lr_v, { lr_u, lr_v, lr_u, lr_v } }
    };

    gfxAuxBufferExt(GFX_BUFFER_AUXBUFFER);
    gfxTexSource(rdp.texbufs[0].tmu, rdp.texbufs[0].begin, GFX_MIPMAPLEVELMASK_BOTH, &(rdp.tbuff_tex->info));
    gfxRenderBuffer(GFX_BUFFER_TEXTUREBUFFER_EXT);
    gfxTextureBufferExt(rdp.texbufs[1].tmu, rdp.texbufs[1].begin, LOD, LOD,
        GFX_ASPECT_LOG2_1x1, GFX_TEXFMT_RGB_565, GFX_MIPMAPLEVELMASK_BOTH);
    gfxDrawTriangle(&v[0], &v[2], &v[1]);
    gfxDrawTriangle(&v[2], &v[3], &v[1]);
    gfxRenderBuffer(GFX_BUFFER_BACKBUFFER);
    gfxAuxBufferExt(GFX_BUFFER_TEXTUREAUXBUFFER_EXT);

    rdp.update |= UPDATE_ZBUF_ENABLED | UPDATE_COMBINE | UPDATE_TEXTURE | UPDATE_ALPHA_COMPARE;
    if (g_settings->fog() && (rdp.flags & FOG_ENABLED))
        gfxFogMode(GFX_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
    WriteTrace(TraceRDP, TraceDebug, "CopyDepthBuffer draw, OK");
    rdp.tbuff_tex = 0;
    return TRUE;
}

int SwapTextureBuffer()
{
    if (!rdp.tbuff_tex)
        return FALSE;
    WriteTrace(TraceRDP, TraceDebug, "SwapTextureBuffer.");
    COLOR_IMAGE ci;
    ci.addr = rdp.tbuff_tex->addr;
    ci.format = rdp.tbuff_tex->format;
    ci.width = rdp.tbuff_tex->width;
    ci.height = rdp.tbuff_tex->height;
    ci.size = 2;
    ci.status = ci_main;
    ci.changed = FALSE;
    TBUFF_COLOR_IMAGE * texbuf = AllocateTextureBuffer(ci);
    if (!texbuf)
    {
        WriteTrace(TraceRDP, TraceDebug, "Failed!");
        return FALSE;
    }
    TexBufSetupCombiner();

    float ul_x = 0.0f;
    float ul_y = 0.0f;
    float lr_x = rdp.tbuff_tex->scr_width;
    float lr_y = rdp.tbuff_tex->scr_height;
    float zero = 0.0f;
    float lr_u = rdp.tbuff_tex->lr_u;
    float lr_v = rdp.tbuff_tex->lr_v;

    // Make the vertices
    VERTEX v[4] = {
        { ul_x, ul_y, 1, 1, zero, zero, zero, zero, { zero, zero, zero, zero } },
        { lr_x, ul_y, 1, 1, lr_u, zero, lr_u, zero, { lr_u, zero, lr_u, zero } },
        { ul_x, lr_y, 1, 1, zero, lr_v, zero, lr_v, { zero, lr_v, zero, lr_v } },
        { lr_x, lr_y, 1, 1, lr_u, lr_v, lr_u, lr_v, { lr_u, lr_v, lr_u, lr_v } }
    };

    gfxTexSource(rdp.tbuff_tex->tmu, rdp.tbuff_tex->tex_addr, GFX_MIPMAPLEVELMASK_BOTH, &(rdp.tbuff_tex->info));
    texbuf->tile_uls = rdp.tbuff_tex->tile_uls;
    texbuf->tile_ult = rdp.tbuff_tex->tile_ult;
    texbuf->v_shift = rdp.tbuff_tex->v_shift;
    gfxRenderBuffer(GFX_BUFFER_TEXTUREBUFFER_EXT);
    gfxTextureBufferExt(texbuf->tmu, texbuf->tex_addr, texbuf->info.smallLodLog2, texbuf->info.largeLodLog2,
        texbuf->info.aspectRatioLog2, texbuf->info.format, GFX_MIPMAPLEVELMASK_BOTH);
    gfxDrawTriangle(&v[0], &v[2], &v[1]);
    gfxDrawTriangle(&v[2], &v[3], &v[1]);
    rdp.texbufs[rdp.tbuff_tex->tmu].clear_allowed = TRUE;
    rdp.texbufs[rdp.tbuff_tex->tmu].count = 0;
    texbuf->tile_uls = rdp.tbuff_tex->tile_uls;
    texbuf->tile_ult = rdp.tbuff_tex->tile_ult;
    texbuf->u_shift = rdp.tbuff_tex->u_shift;
    texbuf->v_shift = rdp.tbuff_tex->v_shift;
    rdp.tbuff_tex = texbuf;
    if (rdp.cur_image)
    {
        gfxTextureBufferExt(rdp.cur_image->tmu, rdp.cur_image->tex_addr, rdp.cur_image->info.smallLodLog2, rdp.cur_image->info.largeLodLog2,
            rdp.cur_image->info.aspectRatioLog2, rdp.cur_image->info.format, GFX_MIPMAPLEVELMASK_BOTH);
    }
    else
    {
        gfxRenderBuffer(GFX_BUFFER_BACKBUFFER);
        rdp.offset_x = rdp.offset_x_bak;
        rdp.offset_y = rdp.offset_y_bak;
        rdp.offset_x_bak = rdp.offset_y_bak = 0;
        rdp.update |= UPDATE_VIEWPORT | UPDATE_SCISSOR;
    }
    rdp.update |= UPDATE_ZBUF_ENABLED | UPDATE_COMBINE | UPDATE_TEXTURE | UPDATE_ALPHA_COMPARE;
    if (g_settings->fog() && (rdp.flags & FOG_ENABLED))
    {
        gfxFogMode(GFX_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
    }
    WriteTrace(TraceRDP, TraceDebug, "SwapTextureBuffer draw, OK");
    return TRUE;
}

static uint32_t CalcCRC(TBUFF_COLOR_IMAGE * pTCI)
{
    uint32_t result = 0;
    if (g_settings->fb_ref_enabled())
    {
        pTCI->crc = 0; //Since fb content changes each frame, crc check is meaningless.
    }
    else if (g_settings->fb_crc_mode() == CSettings::fbcrcFast)
    {
        result = *((uint32_t*)(gfx.RDRAM + pTCI->addr + (pTCI->end_addr - pTCI->addr) / 2));
    }
    else if (g_settings->fb_crc_mode() == CSettings::fbcrcSafe)
    {
        uint8_t * pSrc = gfx.RDRAM + pTCI->addr;
        const uint32_t nSize = pTCI->end_addr - pTCI->addr;
        result = CRC32(0xFFFFFFFF, pSrc, 32);
        result = CRC32(result, pSrc + (nSize >> 1), 32);
        result = CRC32(result, pSrc + nSize - 32, 32);
    }
    return result;
}

int FindTextureBuffer(uint32_t addr, uint16_t width)
{
    if (rdp.skip_drawing)
        return FALSE;
    WriteTrace(TraceRDP, TraceDebug, "FindTextureBuffer. addr: %08lx, width: %d, scale_x: %f", addr, width, rdp.scale_x);
    int found = FALSE;
    uint32_t shift = 0;
    for (int i = 0; i < (nbTextureUnits > 2 ? 2 : 1) && !found; i++)
    {
        uint8_t index = rdp.cur_tex_buf^i;
        for (int j = 0; j < rdp.texbufs[index].count && !found; j++)
        {
            rdp.tbuff_tex = &(rdp.texbufs[index].images[j]);
            if (addr >= rdp.tbuff_tex->addr && addr < rdp.tbuff_tex->end_addr)// && rdp.timg.format == 0)
            {
                bool bCorrect;
                if (rdp.tbuff_tex->crc == 0)
                {
                    rdp.tbuff_tex->crc = CalcCRC(rdp.tbuff_tex);
                    bCorrect = width == 1 || rdp.tbuff_tex->width == width || (rdp.tbuff_tex->width > 320 && rdp.tbuff_tex->width == width * 2);
                }
                else
                    bCorrect = rdp.tbuff_tex->crc == CalcCRC(rdp.tbuff_tex);
                if (bCorrect)
                {
                    shift = addr - rdp.tbuff_tex->addr;
                    // if (!rdp.motionblur)
                    if (!rdp.cur_image)
                        rdp.cur_tex_buf = index;
                    found = TRUE;
                    //    WriteTrace(TraceRDP, TraceDebug, "FindTextureBuffer, found in TMU%d buffer: %d", rdp.tbuff_tex->tmu, j);
                }
                else //new texture is loaded into this place, texture buffer is not valid anymore
                {
                    rdp.texbufs[index].count--;
                    if (j < rdp.texbufs[index].count)
                        memmove(&(rdp.texbufs[index].images[j]), &(rdp.texbufs[index].images[j + 1]), sizeof(TBUFF_COLOR_IMAGE)*(rdp.texbufs[index].count - j));
                }
            }
        }
    }
    if (found)
    {
        rdp.tbuff_tex->tile_uls = 0;
        rdp.tbuff_tex->tile_ult = 0;
        if (shift > 0)
        {
            shift >>= 1;
            rdp.tbuff_tex->v_shift = shift / rdp.tbuff_tex->width;
            rdp.tbuff_tex->u_shift = shift % rdp.tbuff_tex->width;
        }
        else
        {
            rdp.tbuff_tex->v_shift = 0;
            rdp.tbuff_tex->u_shift = 0;
        }
        WriteTrace(TraceRDP, TraceDebug, "FindTextureBuffer, found, u_shift: %d,  v_shift: %d, format: %s", rdp.tbuff_tex->u_shift, rdp.tbuff_tex->v_shift, str_format[rdp.tbuff_tex->format]);
        //WriteTrace(TraceRDP, TraceDebug, "Buffer, addr=%08lx, end_addr=%08lx, width: %d, height: %d", rdp.tbuff_tex->addr, rdp.tbuff_tex->end_addr, rdp.tbuff_tex->width, rdp.tbuff_tex->height);
        return TRUE;
    }
    rdp.tbuff_tex = 0;
    WriteTrace(TraceRDP, TraceDebug, "FindTextureBuffer, not found");
    return FALSE;
}