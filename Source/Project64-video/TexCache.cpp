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

#include "TexCache.h"
#include "Combine.h"
#include "Util.h"
#include <Common/Util.h>
#include <Project64-video/trace.h>

void LoadTex(int id, gfxChipID_t tmu);

uint8_t tex1[1024 * 1024 * 4];		// temporary texture
uint8_t tex2[1024 * 1024 * 4];
uint8_t *texture;
uint8_t *texture_buffer = tex1;

#include "TexLoad.h"	// texture loading functions, ONLY INCLUDE IN THIS FILE!!!
#include "MiClWr32b.h"
#include "MiClWr16b.h"	// Mirror/Clamp/Wrap functions, ONLY INCLUDE IN THIS FILE!!!
#include "MiClWr8b.h"	// Mirror/Clamp/Wrap functions, ONLY INCLUDE IN THIS FILE!!!
#include "TexConv.h"	// texture conversions, ONLY INCLUDE IN THIS FILE!!!
#include "TexMod.h"
#include "TexModCI.h"
#include "CRC.h"

extern bool g_ghq_use;

typedef struct TEXINFO_t
{
    int real_image_width, real_image_height;	// FOR ALIGNMENT PURPOSES ONLY!!!
    int tile_width, tile_height;
    int mask_width, mask_height;
    int width, height;
    int wid_64, line;
    uint32_t crc;
    uint32_t flags;
    int splits, splitheight;
    uint64_t ricecrc;
} TEXINFO;

TEXINFO texinfo[2];
int tex_found[2][MAX_TMU];

typedef struct HIRESTEX_t
{
    int width, height;
    uint16_t format;
    uint8_t *data;
} HIRESTEX;

//****************************************************************
// List functions

typedef struct NODE_t {
    uint32_t	crc;
    uintptr_t	data;
    int		tmu;
    int		number;
    NODE_t	*pNext;
} NODE;

NODE *cachelut[65536];

void AddToList(NODE **list, uint32_t crc, uintptr_t data, int tmu, int number)
{
    NODE *node = new NODE;
    node->crc = crc;
    node->data = data;
    node->tmu = tmu;
    node->number = number;
    node->pNext = *list;
    *list = node;
    rdp.n_cached[tmu] ++;
    rdp.n_cached[tmu ^ 1] = rdp.n_cached[tmu];
}

void DeleteList(NODE **list)
{
    while (*list)
    {
        NODE *next = (*list)->pNext;
        delete (*list);
        *list = next;
    }
}

void TexCacheInit()
{
    for (int i = 0; i < 65536; i++)
    {
        cachelut[i] = NULL;
    }
}

//****************************************************************
// ClearCache - clear the texture cache for BOTH tmus

void ClearCache()
{
    voodoo.tmem_ptr[0] = offset_textures;
    rdp.n_cached[0] = 0;
    voodoo.tmem_ptr[1] = offset_textures;
    rdp.n_cached[1] = 0;

    for (int i = 0; i < 65536; i++)
    {
        DeleteList(&cachelut[i]);
    }
}

//****************************************************************
uint32_t textureCRC(uint8_t *addr, int width, int height, int line)
{
    uint32_t crc = 0;
    uint32_t *pixelpos;
    unsigned int i;
    uint64_t twopixel_crc;

    pixelpos = (uint32_t*)addr;
    for (; height; height--) {
        for (i = width; i; --i) {
            twopixel_crc = i * (uint64_t)(pixelpos[1] + pixelpos[0] + crc);
            crc = (uint32_t)(((twopixel_crc >> 32) + twopixel_crc) & 0xFFFFFFFF);
            pixelpos += 2;
        }
        crc = ((unsigned int)height * (uint64_t)crc >> 32) + height * crc;
        pixelpos = (uint32_t *)((char *)pixelpos + line);
    }

    return crc;
}
// GetTexInfo - gets information for either t0 or t1, checks if in cache & fills tex_found

void GetTexInfo(int id, int tile)
{
    WriteTrace(TraceRDP, TraceDebug, " | |-+ GetTexInfo (id: %d, tile: %d)", id, tile);

    // this is the NEW cache searching, searches only textures with similar crc's
    int t;
    for (t = 0; t < MAX_TMU; t++)
        tex_found[id][t] = -1;

    TBUFF_COLOR_IMAGE * pFBTex = 0;
    if (rdp.aTBuffTex[0] && rdp.aTBuffTex[0]->tile == id)
        pFBTex = rdp.aTBuffTex[0];
    else if (rdp.aTBuffTex[1] && rdp.aTBuffTex[1]->tile == id)
        pFBTex = rdp.aTBuffTex[1];
    if (pFBTex && pFBTex->cache)
        return;

    TEXINFO *info = &texinfo[id];

    int tile_width, tile_height;
    int mask_width, mask_height;
    int width, height;
    int wid_64, line, bpl;

    // Get width and height
    tile_width = rdp.tiles(tile).lr_s - rdp.tiles(tile).ul_s + 1;
    tile_height = rdp.tiles(tile).lr_t - rdp.tiles(tile).ul_t + 1;

    mask_width = (rdp.tiles(tile).mask_s == 0) ? (tile_width) : (1 << rdp.tiles(tile).mask_s);
    mask_height = (rdp.tiles(tile).mask_t == 0) ? (tile_height) : (1 << rdp.tiles(tile).mask_t);

    if (g_settings->alt_tex_size())
    {
        // ** ALTERNATE TEXTURE SIZE METHOD **
        // Helps speed in some games that loaded weird-sized textures, but could break other
        //  textures.

        // Get the width/height to load
        if ((rdp.tiles(tile).clamp_s && tile_width <= 256) || (mask_width > 256))
        {
            // loading width
            width = minval(mask_width, tile_width);
            // actual width
            rdp.tiles(tile).width = tile_width;
        }
        else
        {
            // wrap all the way
            width = minval(mask_width, tile_width);	// changed from mask_width only
            rdp.tiles(tile).width = width;
        }

        if ((rdp.tiles(tile).clamp_t && tile_height <= 256) || (mask_height > 256))
        {
            // loading height
            height = minval(mask_height, tile_height);
            // actual height
            rdp.tiles(tile).height = tile_height;
        }
        else
        {
            // wrap all the way
            height = minval(mask_height, tile_height);
            rdp.tiles(tile).height = height;
        }
    }
    else
    {
        // ** NORMAL TEXTURE SIZE METHOD **
        // This is the 'correct' method for determining texture size, but may cause certain
        //  textures to load too large & make the whole game go slow.

        if (mask_width > 256 && mask_height > 256)
        {
            mask_width = tile_width;
            mask_height = tile_height;
        }

        // Get the width/height to load
        if ((rdp.tiles(tile).clamp_s && tile_width <= 256))//|| (mask_width > 256))
        {
            // loading width
            width = minval(mask_width, tile_width);
            // actual width
            rdp.tiles(tile).width = tile_width;
        }
        else
        {
            // wrap all the way
            width = mask_width;
            rdp.tiles(tile).width = mask_width;
        }

        if ((rdp.tiles(tile).clamp_t && tile_height <= 256) || (mask_height > 256))
        {
            // loading height
            height = minval(mask_height, tile_height);
            // actual height
            rdp.tiles(tile).height = tile_height;
        }
        else
        {
            // wrap all the way
            height = mask_height;
            rdp.tiles(tile).height = mask_height;
        }
    }

    // without any large texture fixing-up; for alignment
    int real_image_width = rdp.tiles(tile).width;
    int real_image_height = rdp.tiles(tile).height;
    int crc_height = height;
    if (rdp.timg.set_by == 1)
        crc_height = tile_height;

    bpl = width << rdp.tiles(tile).size >> 1;

    // ** COMMENT THIS TO DISABLE LARGE TEXTURES
#ifdef LARGE_TEXTURE_HANDLING
    if (g_settings->hacks(CSettings::hack_PPL) && width > 256)
    {
        info->splits = ((width - 1) >> 8) + 1;
        info->splitheight = rdp.tiles(tile).height;
        rdp.tiles(tile).height *= info->splits;
        rdp.tiles(tile).width = 256;
        width = 256;
    }
    else
#endif
        // **
    {
        info->splits = 1;
    }

    WriteTrace(TraceRDP, TraceDebug, " | | |-+ Texture approved:");
    WriteTrace(TraceRDP, TraceDebug, " | | | |- tmem: %08lx", rdp.tiles(tile).t_mem);
    WriteTrace(TraceRDP, TraceDebug, " | | | |- load width: %d", width);
    WriteTrace(TraceRDP, TraceDebug, " | | | |- load height: %d", height);
    WriteTrace(TraceRDP, TraceDebug, " | | | |- actual width: %d", rdp.tiles(tile).width);
    WriteTrace(TraceRDP, TraceDebug, " | | | |- actual height: %d", rdp.tiles(tile).height);
    WriteTrace(TraceRDP, TraceDebug, " | | | |- size: %d", rdp.tiles(tile).size);
    WriteTrace(TraceRDP, TraceDebug, " | | | +- format: %d", rdp.tiles(tile).format);
    WriteTrace(TraceRDP, TraceDebug, " | | |- Calculating CRC... ");

    // ** CRC CHECK

    wid_64 = width << (rdp.tiles(tile).size) >> 1;
    if (rdp.tiles(tile).size == 3)
    {
        if (wid_64 & 15) wid_64 += 16;
        wid_64 &= 0xFFFFFFF0;
    }
    else
    {
        if (wid_64 & 7) wid_64 += 8;	// round up
    }
    wid_64 = wid_64 >> 3;

    // Texture too big for tmem & needs to wrap? (trees in mm)
    if (rdp.tiles(tile).t_mem + minval(height, tile_height) * (rdp.tiles(tile).line << 3) > 4096)
    {
        WriteTrace(TraceRDP, TraceDebug, "TEXTURE WRAPS TMEM!!! ");

        // calculate the y value that intersects at 4096 bytes
        int y = (4096 - rdp.tiles(tile).t_mem) / (rdp.tiles(tile).line << 3);

        rdp.tiles(tile).clamp_t = 0;
        rdp.tiles(tile).lr_t = rdp.tiles(tile).ul_t + y - 1;

        // calc mask
        int shift;
        for (shift = 0; (1 << shift) < y; shift++);
        rdp.tiles(tile).mask_t = shift;

        // restart the function
        WriteTrace(TraceRDP, TraceDebug, "restarting...");
        GetTexInfo(id, tile);
        return;
    }

    line = rdp.tiles(tile).line;
    if (rdp.tiles(tile).size == 3)
        line <<= 1;
    uint32_t crc = 0;
    if (g_settings->fast_crc())
    {
        line = (line - wid_64) << 3;
        if (wid_64 < 1) wid_64 = 1;
        uint8_t * addr = (((uint8_t*)rdp.tmem) + (rdp.tiles(tile).t_mem << 3));
        if (crc_height > 0) // Check the CRC
        {
            if (rdp.tiles(tile).size < 3)
                crc = textureCRC(addr, wid_64, crc_height, line);
            else //32b texture
            {
                int line_2 = line >> 1;
                int wid_64_2 = maxval(1, wid_64 >> 1);
                crc = textureCRC(addr, wid_64_2, crc_height, line_2);
                crc += textureCRC(addr + 0x800, wid_64_2, crc_height, line_2);
            }
        }
    }
    else
    {
        crc = 0xFFFFFFFF;
        uintptr_t addr = uintptr_t(rdp.tmem) + (rdp.tiles(tile).t_mem << 3);
        uint32_t line2 = maxval(line, 1);
        if (rdp.tiles(tile).size < 3)
        {
            line2 <<= 3;
            for (int y = 0; y < crc_height; y++)
            {
                crc = CRC32(crc, reinterpret_cast<void*>(addr), bpl);
                addr += line2;
            }
        }
        else //32b texture
        {
            line2 <<= 2;
            //32b texel is split in two 16b parts, so bpl/2 and line/2.
            //Min value for bpl is 4, because when width==1 first 2 bytes of tmem will not be used.
            bpl = maxval(bpl >> 1, 4);
            for (int y = 0; y < crc_height; y++)
            {
                crc = CRC32(crc, reinterpret_cast<void*>(addr), bpl);
                crc = CRC32(crc, reinterpret_cast<void*>(addr + 0x800), bpl);
                addr += line2;
            }
        }
        line = (line - wid_64) << 3;
        if (wid_64 < 1) wid_64 = 1;
    }
    if ((rdp.tiles(tile).size < 2) && (rdp.tlut_mode || rdp.tiles(tile).format == 2))
    {
        if (rdp.tiles(tile).size == 0)
            crc += rdp.pal_8_crc[rdp.tiles(tile).palette];
        else
            crc += rdp.pal_256_crc;
    }

    WriteTrace(TraceRDP, TraceDebug, "Done.  CRC is: %08lx.", crc);

    uint32_t flags = (rdp.tiles(tile).clamp_s << 23) | (rdp.tiles(tile).mirror_s << 22) |
        (rdp.tiles(tile).mask_s << 18) | (rdp.tiles(tile).clamp_t << 17) |
        (rdp.tiles(tile).mirror_t << 16) | (rdp.tiles(tile).mask_t << 12);

    info->real_image_width = real_image_width;
    info->real_image_height = real_image_height;
    info->tile_width = tile_width;
    info->tile_height = tile_height;
    info->mask_width = mask_width;
    info->mask_height = mask_height;
    info->width = width;
    info->height = height;
    info->wid_64 = wid_64;
    info->line = line;
    info->crc = crc;
    info->flags = flags;

    // Search the texture cache for this texture
    WriteTrace(TraceRDP, TraceDebug, " | | |-+ Checking cache...");

    CACHE_LUT *cache;

    if (rdp.noise == CRDP::noise_texture)
        return;

    uint32_t mod, modcolor, modcolor1, modcolor2, modfactor;
    if (id == 0)
    {
        mod = cmb.mod_0;
        modcolor = cmb.modcolor_0;
        modcolor1 = cmb.modcolor1_0;
        modcolor2 = cmb.modcolor2_0;
        modfactor = cmb.modfactor_0;
    }
    else
    {
        mod = cmb.mod_1;
        modcolor = cmb.modcolor_1;
        modcolor1 = cmb.modcolor1_1;
        modcolor2 = cmb.modcolor2_1;
        modfactor = cmb.modfactor_1;
    }

    NODE *node = cachelut[crc >> 16];
    uint32_t mod_mask = (rdp.tiles(tile).format == 2) ? 0xFFFFFFFF : 0xF0F0F0F0;
    while (node)
    {
        if (node->crc == crc)
        {
            cache = (CACHE_LUT*)node->data;
            if (/*tex_found[id][node->tmu] == -1 &&
                rdp.tiles(tile).palette == cache->palette &&
                rdp.tiles(tile).format == cache->format &&
                rdp.tiles(tile).size == cache->size &&*/
                rdp.tiles(tile).width == cache->width &&
                rdp.tiles(tile).height == cache->height &&
                flags == cache->flags)
            {
                if (!(mod + cache->mod) || (cache->mod == mod &&
                    (cache->mod_color&mod_mask) == (modcolor&mod_mask) &&
                    (cache->mod_color1&mod_mask) == (modcolor1&mod_mask) &&
                    (cache->mod_color2&mod_mask) == (modcolor2&mod_mask) &&
                    abs((int)(cache->mod_factor - modfactor)) < 8))
                {
                    WriteTrace(TraceRDP, TraceDebug, " | | | |- Texture found in cache (tmu=%d).", node->tmu);
                    tex_found[id][node->tmu] = node->number;
                    tex_found[id][node->tmu ^ 1] = node->number;
                    return;
                }
            }
        }
        node = node->pNext;
    }

    WriteTrace(TraceRDP, TraceDebug, " | | | +- Done.\n | | +- GetTexInfo end");
}

//****************************************************************
// SelectTBuffTex - select texture from texture buffer
static void SelectTBuffTex(TBUFF_COLOR_IMAGE * pTBuffTex)
{
    WriteTrace(TraceRDP, TraceDebug, "SelectTBuffTex: tex: %d, tmu: %d, tile: %d", rdp.tex, pTBuffTex->tmu, pTBuffTex->tile);
    gfxTexSource((gfxChipID_t)pTBuffTex->tile, pTBuffTex->tex_addr, GFX_MIPMAPLEVELMASK_BOTH, &(pTBuffTex->info));
}

//****************************************************************
// TexCache - does texture loading after combiner is set
int SwapTextureBuffer();
void TexCache()
{
    WriteTrace(TraceRDP, TraceDebug, " |-+ TexCache called");

    if (rdp.tex & 1)
        GetTexInfo(0, rdp.cur_tile);
    if (rdp.tex & 2)
        GetTexInfo(1, rdp.cur_tile + 1);

    TBUFF_COLOR_IMAGE * aTBuff[2] = { 0, 0 };
    if (rdp.aTBuffTex[0])
        aTBuff[rdp.aTBuffTex[0]->tile] = rdp.aTBuffTex[0];
    if (rdp.aTBuffTex[1])
        aTBuff[rdp.aTBuffTex[1]->tile] = rdp.aTBuffTex[1];

#define TMUMODE_NORMAL		0
#define TMUMODE_PASSTHRU	1
#define TMUMODE_NONE		2

    gfxChipID_t tmu_0, tmu_1;
    int tmu_0_mode = 0, tmu_1_mode = 0;

    // Select the best TMUs to use (removed 3 tmu support, unnecessary)
    if (rdp.tex == 3)	// T0 and T1
    {
        tmu_0 = GFX_TMU0;
        tmu_1 = GFX_TMU1;
    }
    else if (rdp.tex == 2)	// T1
    {
        if (tex_found[1][0] != -1)	// T1 found in tmu 0
            tmu_1 = GFX_TMU0;
        else if (tex_found[1][1] != -1)	// T1 found in tmu 1
            tmu_1 = GFX_TMU1;
        else	// T1 not found
            tmu_1 = GFX_TMU0;

        tmu_0 = tmu_1 == GFX_TMU0 ? GFX_TMU1 : GFX_TMU0;
        tmu_0_mode = (tmu_0 == 1) ? TMUMODE_NONE : TMUMODE_PASSTHRU;
    }
    else if (rdp.tex == 1)	// T0
    {
        if (tex_found[0][0] != -1)	// T0 found in tmu 0
            tmu_0 = GFX_TMU0;
        else if (tex_found[0][1] != -1)	// T0 found in tmu 1
            tmu_0 = GFX_TMU1;
        else	// T0 not found
            tmu_0 = GFX_TMU0;

        tmu_1 = tmu_0 == GFX_TMU0 ? GFX_TMU1 : GFX_TMU0;
        tmu_1_mode = (tmu_1 == 1) ? TMUMODE_NONE : TMUMODE_PASSTHRU;
    }
    else	// no texture
    {
        tmu_0 = GFX_TMU0;
        tmu_0_mode = TMUMODE_NONE;
        tmu_1 = GFX_TMU0;
        tmu_1_mode = TMUMODE_NONE;
    }

    WriteTrace(TraceRDP, TraceDebug, " | |-+ Modes set:\n | | |- tmu_0 = %d\n | | |- tmu_1 = %d", tmu_0, tmu_1);
    WriteTrace(TraceRDP, TraceDebug, " | | |- tmu_0_mode = %d\n | | |- tmu_1_mode = %d", tmu_0_mode, tmu_1_mode);

    if (tmu_0_mode == TMUMODE_PASSTHRU)
    {
        cmb.tmu0_func = cmb.tmu0_a_func = GFX_COMBINE_FUNCTION_SCALE_OTHER;
        cmb.tmu0_fac = cmb.tmu0_a_fac = GFX_COMBINE_FACTOR_ONE;
        if (cmb.tex_cmb_ext_use)
        {
            cmb.t0c_ext_a = GFX_CMBX_OTHER_TEXTURE_RGB;
            cmb.t0c_ext_a_mode = GFX_FUNC_MODE_X;
            cmb.t0c_ext_b = GFX_CMBX_LOCAL_TEXTURE_RGB;
            cmb.t0c_ext_b_mode = GFX_FUNC_MODE_ZERO;
            cmb.t0c_ext_c = GFX_CMBX_ZERO;
            cmb.t0c_ext_c_invert = 1;
            cmb.t0c_ext_d = GFX_CMBX_ZERO;
            cmb.t0c_ext_d_invert = 0;
            cmb.t0a_ext_a = GFX_CMBX_OTHER_TEXTURE_ALPHA;
            cmb.t0a_ext_a_mode = GFX_FUNC_MODE_X;
            cmb.t0a_ext_b = GFX_CMBX_LOCAL_TEXTURE_ALPHA;
            cmb.t0a_ext_b_mode = GFX_FUNC_MODE_ZERO;
            cmb.t0a_ext_c = GFX_CMBX_ZERO;
            cmb.t0a_ext_c_invert = 1;
            cmb.t0a_ext_d = GFX_CMBX_ZERO;
            cmb.t0a_ext_d_invert = 0;
        }
    }
    else if (tmu_0_mode == TMUMODE_NONE)
    {
        cmb.tmu0_func = cmb.tmu0_a_func = GFX_COMBINE_FUNCTION_NONE;
        cmb.tmu0_fac = cmb.tmu0_a_fac = GFX_COMBINE_FACTOR_NONE;
        if (cmb.tex_cmb_ext_use)
        {
            cmb.t0c_ext_a = GFX_CMBX_LOCAL_TEXTURE_RGB;
            cmb.t0c_ext_a_mode = GFX_FUNC_MODE_ZERO;
            cmb.t0c_ext_b = GFX_CMBX_LOCAL_TEXTURE_RGB;
            cmb.t0c_ext_b_mode = GFX_FUNC_MODE_ZERO;
            cmb.t0c_ext_c = GFX_CMBX_ZERO;
            cmb.t0c_ext_c_invert = 0;
            cmb.t0c_ext_d = GFX_CMBX_ZERO;
            cmb.t0c_ext_d_invert = 0;
            cmb.t0a_ext_a = GFX_CMBX_LOCAL_TEXTURE_ALPHA;
            cmb.t0a_ext_a_mode = GFX_FUNC_MODE_ZERO;
            cmb.t0a_ext_b = GFX_CMBX_LOCAL_TEXTURE_ALPHA;
            cmb.t0a_ext_b_mode = GFX_FUNC_MODE_ZERO;
            cmb.t0a_ext_c = GFX_CMBX_ZERO;
            cmb.t0a_ext_c_invert = 0;
            cmb.t0a_ext_d = GFX_CMBX_ZERO;
            cmb.t0a_ext_d_invert = 0;
        }
    }
    if (tmu_1_mode == TMUMODE_PASSTHRU)
    {
        cmb.tmu1_func = cmb.tmu1_a_func = GFX_COMBINE_FUNCTION_SCALE_OTHER;
        cmb.tmu1_fac = cmb.tmu1_a_fac = GFX_COMBINE_FACTOR_ONE;
        if (cmb.tex_cmb_ext_use)
        {
            cmb.t1c_ext_a = GFX_CMBX_OTHER_TEXTURE_RGB;
            cmb.t1c_ext_a_mode = GFX_FUNC_MODE_X;
            cmb.t1c_ext_b = GFX_CMBX_LOCAL_TEXTURE_RGB;
            cmb.t1c_ext_b_mode = GFX_FUNC_MODE_ZERO;
            cmb.t1c_ext_c = GFX_CMBX_ZERO;
            cmb.t1c_ext_c_invert = 1;
            cmb.t1c_ext_d = GFX_CMBX_ZERO;
            cmb.t1c_ext_d_invert = 0;
            cmb.t1a_ext_a = GFX_CMBX_OTHER_TEXTURE_ALPHA;
            cmb.t1a_ext_a_mode = GFX_FUNC_MODE_X;
            cmb.t1a_ext_b = GFX_CMBX_LOCAL_TEXTURE_ALPHA;
            cmb.t1a_ext_b_mode = GFX_FUNC_MODE_ZERO;
            cmb.t1a_ext_c = GFX_CMBX_ZERO;
            cmb.t1a_ext_c_invert = 1;
            cmb.t1a_ext_d = GFX_CMBX_ZERO;
            cmb.t1a_ext_d_invert = 0;
        }
    }
    else if (tmu_1_mode == TMUMODE_NONE)
    {
        cmb.tmu1_func = cmb.tmu1_a_func = GFX_COMBINE_FUNCTION_NONE;
        cmb.tmu1_fac = cmb.tmu1_a_fac = GFX_COMBINE_FACTOR_NONE;
        if (cmb.tex_cmb_ext_use)
        {
            cmb.t1c_ext_a = GFX_CMBX_LOCAL_TEXTURE_RGB;
            cmb.t1c_ext_a_mode = GFX_FUNC_MODE_ZERO;
            cmb.t1c_ext_b = GFX_CMBX_LOCAL_TEXTURE_RGB;
            cmb.t1c_ext_b_mode = GFX_FUNC_MODE_ZERO;
            cmb.t1c_ext_c = GFX_CMBX_ZERO;
            cmb.t1c_ext_c_invert = 0;
            cmb.t1c_ext_d = GFX_CMBX_ZERO;
            cmb.t1c_ext_d_invert = 0;
            cmb.t1a_ext_a = GFX_CMBX_LOCAL_TEXTURE_ALPHA;
            cmb.t1a_ext_a_mode = GFX_FUNC_MODE_ZERO;
            cmb.t1a_ext_b = GFX_CMBX_LOCAL_TEXTURE_ALPHA;
            cmb.t1a_ext_b_mode = GFX_FUNC_MODE_ZERO;
            cmb.t1a_ext_c = GFX_CMBX_ZERO;
            cmb.t1a_ext_c_invert = 0;
            cmb.t1a_ext_d = GFX_CMBX_ZERO;
            cmb.t1a_ext_d_invert = 0;
        }
    }

    // little change to make single-tmu cards look better, use first texture no matter what

    if ((nbTextureUnits > 2 ? 2 : 1) == 1)
    {
        if (rdp.best_tex == 0)
        {
            cmb.tmu0_func = cmb.tmu0_a_func = GFX_COMBINE_FUNCTION_LOCAL;
            cmb.tmu0_fac = cmb.tmu0_a_fac = GFX_COMBINE_FACTOR_NONE;
            tmu_0 = GFX_TMU0;
            tmu_1 = GFX_TMU1;
        }
        else
        {
            cmb.tmu1_func = cmb.tmu1_a_func = GFX_COMBINE_FUNCTION_LOCAL;
            cmb.tmu1_fac = cmb.tmu1_a_fac = GFX_COMBINE_FACTOR_NONE;
            tmu_1 = GFX_TMU0;
            tmu_0 = GFX_TMU1;
        }
    }

    rdp.t0 = tmu_0;
    rdp.t1 = tmu_1;

    // SET the combiner
    if (GfxInitDone)
    {
        if (rdp.allow_combine)
        {
            // Now actually combine
            if (cmb.cmb_ext_use)
            {
                WriteTrace(TraceRDP, TraceDebug, " | | | |- combiner extension");
                if (!(cmb.cmb_ext_use & COMBINE_EXT_COLOR))
                    ColorCombinerToExtension();
                if (!(cmb.cmb_ext_use & COMBINE_EXT_ALPHA))
                    AlphaCombinerToExtension();
                gfxColorCombineExt(cmb.c_ext_a, cmb.c_ext_a_mode, cmb.c_ext_b, cmb.c_ext_b_mode, cmb.c_ext_c, cmb.c_ext_c_invert, cmb.c_ext_d, cmb.c_ext_d_invert, 0, 0);
                gfxAlphaCombineExt(cmb.a_ext_a, cmb.a_ext_a_mode, cmb.a_ext_b, cmb.a_ext_b_mode, cmb.a_ext_c, cmb.a_ext_c_invert, cmb.a_ext_d, cmb.a_ext_d_invert, 0, 0);
            }
            else
            {
                gfxColorCombine(cmb.c_fnc, cmb.c_fac, cmb.c_loc, cmb.c_oth, false);
                gfxAlphaCombine(cmb.a_fnc, cmb.a_fac, cmb.a_loc, cmb.a_oth, false);
            }
            gfxConstantColorValue(cmb.ccolor);
            gfxAlphaBlendFunction(cmb.abf1, cmb.abf2, GFX_BLEND_ZERO, GFX_BLEND_ZERO);
            if (!rdp.tex) //nothing more to do
                return;
        }

        if (tmu_1 < (nbTextureUnits > 2 ? 2 : 1))
        {
            if (cmb.tex_cmb_ext_use)
            {
                WriteTrace(TraceRDP, TraceDebug, " | | | |- combiner extension tmu1");
                if (!(cmb.tex_cmb_ext_use & TEX_COMBINE_EXT_COLOR))
                    TexColorCombinerToExtension(GFX_TMU1);
                if (!(cmb.tex_cmb_ext_use & TEX_COMBINE_EXT_ALPHA))
                    TexAlphaCombinerToExtension(GFX_TMU1);
                gfxTexColorCombineExt(tmu_1, cmb.t1c_ext_a, cmb.t1c_ext_a_mode, cmb.t1c_ext_b, cmb.t1c_ext_b_mode, cmb.t1c_ext_c, cmb.t1c_ext_c_invert, cmb.t1c_ext_d, cmb.t1c_ext_d_invert, 0, 0);
                gfxTexAlphaCombineExt(tmu_1, cmb.t1a_ext_a, cmb.t1a_ext_a_mode, cmb.t1a_ext_b, cmb.t1a_ext_b_mode, cmb.t1a_ext_c, cmb.t1a_ext_c_invert, cmb.t1a_ext_d, cmb.t1a_ext_d_invert, 0, 0);
                gfxConstantColorValueExt(tmu_1, cmb.tex_ccolor);
            }
            else
            {
                gfxTexCombine(tmu_1, cmb.tmu1_func, cmb.tmu1_fac, cmb.tmu1_a_func, cmb.tmu1_a_fac, cmb.tmu1_invert, cmb.tmu1_a_invert);
                if (cmb.combine_ext)
                {
                    gfxConstantColorValueExt(tmu_1, 0);
                }
            }
            gfxTexDetailControl(tmu_1, cmb.dc1_lodbias, cmb.dc1_detailscale, cmb.dc1_detailmax);
        }
        if (tmu_0 < (nbTextureUnits > 2 ? 2 : 1))
        {
            if (cmb.tex_cmb_ext_use)
            {
                WriteTrace(TraceRDP, TraceDebug, " | | | |- combiner extension tmu0");
                if (!(cmb.tex_cmb_ext_use & TEX_COMBINE_EXT_COLOR))
                {
                    TexColorCombinerToExtension(GFX_TMU0);
                }
                if (!(cmb.tex_cmb_ext_use & TEX_COMBINE_EXT_ALPHA))
                {
                    TexAlphaCombinerToExtension(GFX_TMU0);
                }
                gfxTexColorCombineExt(tmu_0, cmb.t0c_ext_a, cmb.t0c_ext_a_mode, cmb.t0c_ext_b, cmb.t0c_ext_b_mode, cmb.t0c_ext_c, cmb.t0c_ext_c_invert, cmb.t0c_ext_d, cmb.t0c_ext_d_invert, 0, 0);
                gfxTexAlphaCombineExt(tmu_0, cmb.t0a_ext_a, cmb.t0a_ext_a_mode, cmb.t0a_ext_b, cmb.t0a_ext_b_mode, cmb.t0a_ext_c, cmb.t0a_ext_c_invert, cmb.t0a_ext_d, cmb.t0a_ext_d_invert, 0, 0);
                gfxConstantColorValueExt(tmu_0, cmb.tex_ccolor);
            }
            else
            {
                gfxTexCombine(tmu_0, cmb.tmu0_func, cmb.tmu0_fac, cmb.tmu0_a_func, cmb.tmu0_a_fac, cmb.tmu0_invert, cmb.tmu0_a_invert);
                if (cmb.combine_ext)
                {
                    gfxConstantColorValueExt(tmu_0, 0);
                }
            }
            gfxTexDetailControl(tmu_0, cmb.dc0_lodbias, cmb.dc0_detailscale, cmb.dc0_detailmax);
        }
    }

    if ((rdp.tex & 1) && tmu_0 < (nbTextureUnits > 2 ? 2 : 1))
    {
        if (aTBuff[0] && aTBuff[0]->cache)
        {
            WriteTrace(TraceRDP, TraceDebug, " | |- Hires tex T0 found in cache.");
            if (GfxInitDone)
            {
                rdp.cur_cache[0] = aTBuff[0]->cache;
                rdp.cur_cache[0]->last_used = frame_count;
                rdp.cur_cache[0]->uses = rdp.debug_n;
            }
        }
        else if (tex_found[0][tmu_0] != -1)
        {
            WriteTrace(TraceRDP, TraceDebug, " | |- T0 found in cache.");
            if (GfxInitDone)
            {
                CACHE_LUT *cache = &rdp.cache[0][tex_found[0][0]];
                rdp.cur_cache_n[0] = tex_found[0][tmu_0];
                rdp.cur_cache[0] = cache;
                rdp.cur_cache[0]->last_used = frame_count;
                rdp.cur_cache[0]->uses = rdp.debug_n;
                gfxTexSource(tmu_0,
                    (voodoo.tex_min_addr[tmu_0] + cache->tmem_addr),
                    GFX_MIPMAPLEVELMASK_BOTH,
                    &cache->t_info);
            }
        }
        else
            LoadTex(0, tmu_0);
    }
    if ((rdp.tex & 2) && tmu_1 < (nbTextureUnits > 2 ? 2 : 1))
    {
        if (aTBuff[1] && aTBuff[1]->cache)
        {
            WriteTrace(TraceRDP, TraceDebug, " | |- Hires tex T1 found in cache.");
            if (GfxInitDone)
            {
                rdp.cur_cache[1] = aTBuff[1]->cache;
                rdp.cur_cache[1]->last_used = frame_count;
                rdp.cur_cache[1]->uses = rdp.debug_n;
            }
        }
        else if (tex_found[1][tmu_1] != -1)
        {
            WriteTrace(TraceRDP, TraceDebug, " | |- T1 found in cache.");
            if (GfxInitDone)
            {
                CACHE_LUT *cache = &rdp.cache[0][tex_found[1][0]];
                rdp.cur_cache_n[1] = tex_found[1][tmu_1];
                rdp.cur_cache[1] = cache;
                rdp.cur_cache[1]->last_used = frame_count;
                rdp.cur_cache[1]->uses = rdp.debug_n;
                gfxTexSource(tmu_1,
                    (voodoo.tex_min_addr[tmu_1] + cache->tmem_addr),
                    GFX_MIPMAPLEVELMASK_BOTH,
                    &cache->t_info);
            }
        }
        else
            LoadTex(1, tmu_1);
    }

    if (GfxInitDone)
    {
        gfxChipID_t tmu_v[2];

        tmu_v[0] = tmu_0;
        tmu_v[1] = tmu_1;
        for (int i = 0; i < 2; i++)
        {
            const gfxChipID_t tmu = tmu_v[i];

            if (tmu >= (nbTextureUnits > 2 ? 2 : 1)) continue;

            int tile = rdp.cur_tile + i;

            if (g_settings->filtering() == CSettings::Filter_Automatic)
            {
                gfxTextureFilterMode_t filter = (rdp.filter_mode != 2) ? GFX_TEXTUREFILTER_POINT_SAMPLED : GFX_TEXTUREFILTER_BILINEAR;
                gfxTexFilterMode(tmu, filter, filter);
            }
            else
            {
                gfxTextureFilterMode_t filter = (g_settings->filtering() == CSettings::Filter_ForceBilinear) ? GFX_TEXTUREFILTER_BILINEAR : GFX_TEXTUREFILTER_POINT_SAMPLED;
                gfxTexFilterMode(tmu, filter, filter);
            }

            if (rdp.cur_cache[i])
            {
                gfxTextureClampMode_t mode_s, mode_t;
                int clamp_s, clamp_t;
                if (rdp.force_wrap && !rdp.texrecting)
                {
                    clamp_s = rdp.tiles(tile).clamp_s && rdp.tiles(tile).lr_s - rdp.tiles(tile).ul_s < 256;
                    clamp_t = rdp.tiles(tile).clamp_t && rdp.tiles(tile).lr_t - rdp.tiles(tile).ul_t < 256;
                }
                else
                {
                    clamp_s = (rdp.tiles(tile).clamp_s || rdp.tiles(tile).mask_s == 0) &&
                        rdp.tiles(tile).lr_s - rdp.tiles(tile).ul_s < 256;
                    clamp_t = (rdp.tiles(tile).clamp_t || rdp.tiles(tile).mask_t == 0) &&
                        rdp.tiles(tile).lr_t - rdp.tiles(tile).ul_t < 256;
                }

                if (rdp.cur_cache[i]->f_mirror_s)
                    mode_s = GFX_TEXTURECLAMP_MIRROR_EXT;
                else if (rdp.cur_cache[i]->f_wrap_s)
                    mode_s = GFX_TEXTURECLAMP_WRAP;
                else if (clamp_s)
                    mode_s = GFX_TEXTURECLAMP_CLAMP;
                else
                {
                    if (rdp.tiles(tile).mirror_s && !g_settings->hacks(CSettings::hack_Zelda))
                        mode_s = GFX_TEXTURECLAMP_MIRROR_EXT;
                    else
                        mode_s = GFX_TEXTURECLAMP_WRAP;
                }

                if (rdp.cur_cache[i]->f_mirror_t)
                    mode_t = GFX_TEXTURECLAMP_MIRROR_EXT;
                else if (rdp.cur_cache[i]->f_wrap_t)
                    mode_t = GFX_TEXTURECLAMP_WRAP;
                else if (clamp_t)
                    mode_t = GFX_TEXTURECLAMP_CLAMP;
                else
                {
                    if (rdp.tiles(tile).mirror_t && !g_settings->hacks(CSettings::hack_Zelda))
                        mode_t = GFX_TEXTURECLAMP_MIRROR_EXT;
                    else
                        mode_t = GFX_TEXTURECLAMP_WRAP;
                }

                gfxTexClampMode(tmu,
                    mode_s,
                    mode_t);
            }
            if (aTBuff[i] && (rdp.tex&(i + 1)))
                SelectTBuffTex(aTBuff[i]);
        }
    }

    WriteTrace(TraceRDP, TraceDebug, " | +- TexCache End");
}

/** cite from RiceVideo */
inline uint32_t CalculateDXT(uint32_t txl2words)
{
    if (txl2words == 0) return 1;
    else return (2048 + txl2words - 1) / txl2words;
}

uint32_t sizeBytes[4] = { 0, 1, 2, 4 };

inline uint32_t Txl2Words(uint32_t width, uint32_t size)
{
    return size == 0 ? maxval(1, width / 16) : maxval(1, width*sizeBytes[size] / 8);
}

inline uint32_t ReverseDXT(uint32_t val, uint32_t /*lrs*/, uint32_t width, uint32_t size)
{
    if (val == 0x800) return 1;

    int low = 2047 / val;
    if (CalculateDXT(low) > val)	low++;
    int high = 2047 / (val - 1);

    if (low == high)	return low;

    for (int i = low; i <= high; i++)
    {
        if (Txl2Words(width, size) == (uint32_t)i)
            return i;
    }

    return	(low + high) / 2;
}
/** end RiceVideo cite */

//****************************************************************
// LoadTex - does the actual texture loading after everything is prepared

void LoadTex(int id, gfxChipID_t tmu)
{
    WriteTrace(TraceRDP, TraceDebug, " | |-+ LoadTex (id: %d, tmu: %d)", id, tmu);

    int td = rdp.cur_tile + id;
    gfxLOD_t lod;
    gfxAspectRatio_t aspect;
    CACHE_LUT *cache;

    if (texinfo[id].width < 0 || texinfo[id].height < 0)
        return;

    // Clear the cache if it's full
    if (rdp.n_cached[tmu] >= MAX_CACHE)
    {
        WriteTrace(TraceRDP, TraceDebug, "Cache count reached, clearing...");
        ClearCache();
        if (id == 1 && rdp.tex == 3)
            LoadTex(0, rdp.t0);
    }

    // Get this cache object
    cache = &rdp.cache[0][rdp.n_cached[0]];
    memset(cache, 0, sizeof(*cache));
    rdp.cur_cache[id] = cache;
    rdp.cur_cache_n[id] = rdp.n_cached[tmu];

    //!Hackalert
    //GoldenEye water texture. It has CI format in fact, but the game set it to RGBA
    if (g_settings->hacks(CSettings::hack_GoldenEye) && rdp.tiles(td).format == 0 && rdp.tlut_mode == 2 && rdp.tiles(td).size == 2)
    {
        rdp.tiles(td).format = 2;
        rdp.tiles(td).size = 1;
    }

    // Set the data
    cache->line = rdp.tiles(td).line;
    cache->addr = rdp.addr[rdp.tiles(td).t_mem];
    cache->crc = texinfo[id].crc;
    cache->palette = rdp.tiles(td).palette;
    cache->width = rdp.tiles(td).width;
    cache->height = rdp.tiles(td).height;
    cache->format = rdp.tiles(td).format;
    cache->size = rdp.tiles(td).size;
    cache->tmem_addr = voodoo.tmem_ptr[tmu];
    cache->set_by = rdp.timg.set_by;
    cache->texrecting = rdp.texrecting;
    cache->last_used = frame_count;
    cache->uses = rdp.debug_n;
    cache->flags = texinfo[id].flags;
    cache->f_mirror_s = FALSE;
    cache->f_mirror_t = FALSE;
    cache->f_wrap_s = FALSE;
    cache->f_wrap_t = FALSE;
    cache->is_hires_tex = FALSE;
    cache->ricecrc = texinfo[id].ricecrc;

    // Add this cache to the list
    AddToList(&cachelut[cache->crc >> 16], cache->crc, uintptr_t(cache), tmu, rdp.n_cached[tmu]);

    // temporary
    cache->t_info.format = GFX_TEXFMT_ARGB_1555;

    // Calculate lod and aspect
    uint32_t size_x = rdp.tiles(td).width;
    uint32_t size_y = rdp.tiles(td).height;

    // make size_x and size_y both powers of two
    if (g_settings->hacks(CSettings::hack_PPL))
    {
        if (size_x > 256) size_x = 256;
        if (size_y > 256) size_y = 256;
    }

    int shift;
    for (shift = 0; (1 << shift) < (int)size_x; shift++);
    size_x = 1 << shift;
    for (shift = 0; (1 << shift) < (int)size_y; shift++);
    size_y = 1 << shift;

    if (g_settings->hacks(CSettings::hack_Zelda))
    {
        if (rdp.tiles(td).mirror_s && !rdp.tiles(td).clamp_s && (!g_settings->hacks(CSettings::hack_PPL) || size_x <= 128))
            size_x <<= 1;
        if (rdp.tiles(td).mirror_t && !rdp.tiles(td).clamp_t && (!g_settings->hacks(CSettings::hack_PPL) || size_y <= 128))
            size_y <<= 1;
    }

    // Calculate the maximum size
    int size_max = maxval(size_x, size_y);
    uint32_t real_x = size_max, real_y = size_max;
    switch (size_max)
    {
    case 1:
        lod = GFX_LOD_LOG2_1;
        cache->scale = 256.0f;
        break;
    case 2:
        lod = GFX_LOD_LOG2_2;
        cache->scale = 128.0f;
        break;
    case 4:
        lod = GFX_LOD_LOG2_4;
        cache->scale = 64.0f;
        break;
    case 8:
        lod = GFX_LOD_LOG2_8;
        cache->scale = 32.0f;
        break;
    case 16:
        lod = GFX_LOD_LOG2_16;
        cache->scale = 16.0f;
        break;
    case 32:
        lod = GFX_LOD_LOG2_32;
        cache->scale = 8.0f;
        break;
    case 64:
        lod = GFX_LOD_LOG2_64;
        cache->scale = 4.0f;
        break;
    case 128:
        lod = GFX_LOD_LOG2_128;
        cache->scale = 2.0f;
        break;
    case 256:
        lod = GFX_LOD_LOG2_256;
        cache->scale = 1.0f;
        break;
    case 512:
        lod = GFX_LOD_LOG2_512;
        cache->scale = 0.5f;
        break;
    default:
        lod = GFX_LOD_LOG2_1024;
        cache->scale = 0.25f;
        break;
    }

    // Calculate the aspect ratio
    if (size_x >= size_y)
    {
        int ratio = size_x / size_y;
        switch (ratio)
        {
        case 1:
            aspect = GFX_ASPECT_LOG2_1x1;
            cache->scale_x = 1.0f;
            cache->scale_y = 1.0f;
            break;
        case 2:
            aspect = GFX_ASPECT_LOG2_2x1;
            cache->scale_x = 1.0f;
            cache->scale_y = 0.5f;
            real_y >>= 1;
            break;
        case 4:
            aspect = GFX_ASPECT_LOG2_4x1;
            cache->scale_x = 1.0f;
            cache->scale_y = 0.25f;
            real_y >>= 2;
            break;
        default:
            aspect = GFX_ASPECT_LOG2_8x1;
            cache->scale_x = 1.0f;
            cache->scale_y = 0.125f;
            real_y >>= 3;
            break;
        }
    }
    else
    {
        int ratio = size_y / size_x;
        switch (ratio)
        {
        case 2:
            aspect = GFX_ASPECT_LOG2_1x2;
            cache->scale_x = 0.5f;
            cache->scale_y = 1.0f;
            real_x >>= 1;
            break;
        case 4:
            aspect = GFX_ASPECT_LOG2_1x4;
            cache->scale_x = 0.25f;
            cache->scale_y = 1.0f;
            real_x >>= 2;
            break;
        default:
            aspect = GFX_ASPECT_LOG2_1x8;
            cache->scale_x = 0.125f;
            cache->scale_y = 1.0f;
            real_x >>= 3;
            break;
        }
    }

    if (real_x != cache->width || real_y != cache->height)
    {
        cache->scale_x *= (float)cache->width / (float)real_x;
        cache->scale_y *= (float)cache->height / (float)real_y;
    }

    int splits = texinfo[id].splits;
    cache->splits = texinfo[id].splits;
    cache->splitheight = real_y / cache->splits;
    if (cache->splitheight < texinfo[id].splitheight)
        cache->splitheight = texinfo[id].splitheight;

    // ** Calculate alignment values
    int wid = cache->width;
    int hei = cache->height;

    if (splits > 1)
    {
        wid = texinfo[id].real_image_width;
        hei = texinfo[id].real_image_height;
    }

    cache->c_off = cache->scale * 0.5f;
    if (wid != 1) cache->c_scl_x = cache->scale;
    else cache->c_scl_x = 0.0f;
    if (hei != 1) cache->c_scl_y = cache->scale;
    else cache->c_scl_y = 0.0f;
    // **

    uint32_t mod, modcolor, modcolor1, modcolor2, modfactor;
    if (id == 0)
    {
        mod = cmb.mod_0;
        modcolor = cmb.modcolor_0;
        modcolor1 = cmb.modcolor1_0;
        modcolor2 = cmb.modcolor2_0;
        modfactor = cmb.modfactor_0;
    }
    else
    {
        mod = cmb.mod_1;
        modcolor = cmb.modcolor_1;
        modcolor1 = cmb.modcolor1_1;
        modcolor2 = cmb.modcolor2_1;
        modfactor = cmb.modfactor_1;
    }

    uint16_t tmp_pal[256];
    int modifyPalette = (mod && (cache->format == 2) && (rdp.tlut_mode == 2));

    if (modifyPalette)
    {
        memcpy(tmp_pal, rdp.pal_8, 512);
        ModifyPalette(mod, modcolor, modcolor1, modfactor);
    }

    cache->mod = mod;
    cache->mod_color = modcolor;
    cache->mod_color1 = modcolor1;
    cache->mod_factor = modfactor;

    for (int t = 0; t < 2; t++) {
        if (rdp.aTBuffTex[t] && rdp.aTBuffTex[t]->tile == id) //texture buffer will be used instead of frame buffer texture
        {
            rdp.aTBuffTex[t]->cache = cache;
            WriteTrace(TraceRDP, TraceDebug, "tbuff_tex selected: %d, tile=%d", t, id);
            return;
        }
    }

    uint32_t result = 0;	// keep =0 so it doesn't mess up on the first split

    texture = tex1;

    // Hiroshi Morii <koolsmoky@users.sourceforge.net>
    // NOTE: Loading Hi-res texture packs and filtering should be done
    // before the texture is modified with color palettes, etc.
    //
    // Since the internal texture identification needs Glide64CRC, (RiceCRC
    // doesn't always return unique values) it seems reasonable that the
    // extra CRC calculation for hires textures should be executed only
    // when we get passed the texture ram cache and texture buffers for
    // minimal calculation overhead.
    //
    GHQTexInfo ghqTexInfo;
    memset(&ghqTexInfo, 0, sizeof(GHQTexInfo));
    uint32_t g64_crc = cache->crc;
    if (g_ghq_use)
    {
        int bpl;
        uint8_t* addr = (uint8_t*)(gfx.RDRAM + rdp.addr[rdp.tiles(td).t_mem]);
        int tile_width = texinfo[id].width;
        int tile_height = texinfo[id].height;
        LOAD_TILE_INFO &info = rdp.load_info[rdp.tiles(td).t_mem];
        if (rdp.timg.set_by == 1)
        {
            bpl = info.tex_width << info.tex_size >> 1;
            addr += (info.tile_ul_t * bpl) + (((info.tile_ul_s << info.tex_size) + 1) >> 1);

            tile_width = minval(info.tile_width, info.tex_width);
            if (info.tex_size > rdp.tiles(td).size)
                tile_width <<= info.tex_size - rdp.tiles(td).size;

            if (rdp.tiles(td).lr_t > rdp.bg_image_height)
                tile_height = rdp.bg_image_height - rdp.tiles(td).ul_t;
            else
                tile_height = info.tile_height;
        }
        else
        {
            if (rdp.tiles(td).size == 3)
                bpl = rdp.tiles(td).line << 4;
            else if (info.dxt == 0)
                bpl = rdp.tiles(td).line << 3;
            else {
                uint32_t dxt = info.dxt;
                if (dxt > 1)
                    dxt = ReverseDXT(dxt, info.tile_width, texinfo[id].width, rdp.tiles(td).size);
                bpl = dxt << 3;
            }
        }

        //    uint8_t* addr = (uint8_t*)(gfx.RDRAM+rdp.addr[rdp.tiles(td).t_mem] + (rdp.tiles(td).ul_t * bpl) + (((rdp.tiles(td).ul_s<<rdp.tiles(td).size)+1)>>1));
        uint8_t * paladdr = 0;
        uint16_t * palette = 0;
        if ((rdp.tiles(td).size < 2) && (rdp.tlut_mode || rdp.tiles(td).format == 2))
        {
            if (rdp.tiles(td).size == 1)
                paladdr = (uint8_t*)(rdp.pal_8_rice);
            else if (g_settings->ghq_hirs_altcrc())
                paladdr = (uint8_t*)(rdp.pal_8_rice + (rdp.tiles(td).palette << 5));
            else
                paladdr = (uint8_t*)(rdp.pal_8_rice + (rdp.tiles(td).palette << 4));
            palette = (rdp.pal_8 + (rdp.tiles(td).palette << 4));
        }

        // XXX: Special combiner modes are ignored for hires textures
        // for now. Come back to this later!! The following is needed
        // for (2xSai, hq4x, etc) enhanced/filtered textures.
        g64_crc = CRC32(g64_crc, &cache->mod, 4);
        g64_crc = CRC32(g64_crc, &cache->mod_color, 4);
        g64_crc = CRC32(g64_crc, &cache->mod_color1, 4);
        //g64_crc = CRC32( g64_crc, &cache->mod_color2, 4 ); // not used?
        g64_crc = CRC32(g64_crc, &cache->mod_factor, 4);

        cache->ricecrc = ext_ghq_checksum(addr, tile_width, tile_height, (unsigned short)(rdp.tiles(td).format << 8 | rdp.tiles(td).size), bpl, paladdr);
        WriteTrace(TraceRDP, TraceDebug, "CI RICE CRC. format: %d, size: %d, CRC: %08lx, PalCRC: %08lx", rdp.tiles(td).format, rdp.tiles(td).size, (uint32_t)(cache->ricecrc & 0xFFFFFFFF), (uint32_t)(cache->ricecrc >> 32));
        if (ext_ghq_hirestex((uint64_t)g64_crc, cache->ricecrc, palette, &ghqTexInfo))
        {
            cache->is_hires_tex = ghqTexInfo.is_hires_tex;
            if (!ghqTexInfo.is_hires_tex && aspect != ghqTexInfo.aspectRatioLog2)
                ghqTexInfo.data = 0; //if aspects of current texture and found filtered texture are different, texture must be filtered again.
        }
    }

    // ** handle texture splitting **
    if (ghqTexInfo.data)
        ;//do nothing
    else
        if (splits > 1)
        {
            cache->scale_y = 0.125f;

            int i;
            for (i = 0; i < splits; i++)
            {
                int start_dst = i * cache->splitheight * 256;	// start lower
                start_dst <<= HIWORD(result);	// 1st time, result is set to 0, but start_dst is 0 anyway so it doesn't matter

                int start_src = i * 256;	// start 256 more to the right
                start_src = start_src << (rdp.tiles(td).size) >> 1;
                if (rdp.tiles(td).size == 3)
                    start_src >>= 1;

                result = load_table[rdp.tiles(td).size][rdp.tiles(td).format]
                (uintptr_t(texture) + start_dst, uintptr_t(rdp.tmem) + (rdp.tiles(td).t_mem << 3) + start_src,
                    texinfo[id].wid_64, texinfo[id].height, texinfo[id].line, real_x, td);

                uint32_t size = HIWORD(result);
                // clamp so that it looks somewhat ok when wrapping
                if (size == 1)
                    Clamp16bT((texture)+start_dst, texinfo[id].height, real_x, cache->splitheight);
                else if (size != 2)
                    Clamp8bT((texture)+start_dst, texinfo[id].height, real_x, cache->splitheight);
                else
                    Clamp32bT((texture)+start_dst, texinfo[id].height, real_x, cache->splitheight);
            }
        }
    // ** end texture splitting **
        else
        {
            result = load_table[rdp.tiles(td).size][rdp.tiles(td).format]
            (uintptr_t(texture), uintptr_t(rdp.tmem) + (rdp.tiles(td).t_mem << 3),
                texinfo[id].wid_64, texinfo[id].height, texinfo[id].line, real_x, td);

            uint32_t size = HIWORD(result);

            int min_x, min_y;
            if (rdp.tiles(td).mask_s != 0)
                min_x = minval((int)real_x, 1 << rdp.tiles(td).mask_s);
            else
                min_x = real_x;
            if (rdp.tiles(td).mask_t != 0)
                min_y = minval((int)real_y, 1 << rdp.tiles(td).mask_t);
            else
                min_y = real_y;

            // Load using mirroring/clamping
            if (min_x > texinfo[id].width)
            {
                if (size == 1)
                    Clamp16bS((texture), texinfo[id].width, min_x, real_x, texinfo[id].height);
                else if (size != 2)
                    Clamp8bS((texture), texinfo[id].width, min_x, real_x, texinfo[id].height);
                else
                    Clamp32bS((texture), texinfo[id].width, min_x, real_x, texinfo[id].height);
            }

            if (texinfo[id].width < (int)real_x)
            {
                if (rdp.tiles(td).mirror_s)
                {
                    if (size == 1)
                        Mirror16bS((texture), rdp.tiles(td).mask_s,
                            real_x, real_x, texinfo[id].height);
                    else if (size != 2)
                        Mirror8bS((texture), rdp.tiles(td).mask_s,
                            real_x, real_x, texinfo[id].height);
                    else
                        Mirror32bS((texture), rdp.tiles(td).mask_s,
                            real_x, real_x, texinfo[id].height);
                }
                else
                {
                    if (size == 1)
                        Wrap16bS((texture), rdp.tiles(td).mask_s,
                            real_x, real_x, texinfo[id].height);
                    else if (size != 2)
                        Wrap8bS((texture), rdp.tiles(td).mask_s,
                            real_x, real_x, texinfo[id].height);
                    else
                        Wrap32bS((texture), rdp.tiles(td).mask_s,
                            real_x, real_x, texinfo[id].height);
                }
            }

            if (min_y > texinfo[id].height)
            {
                if (size == 1)
                    Clamp16bT((texture), texinfo[id].height, real_x, min_y);
                else if (size != 2)
                    Clamp8bT((texture), texinfo[id].height, real_x, min_y);
                else
                    Clamp32bT((texture), texinfo[id].height, real_x, min_y);
            }

            if (texinfo[id].height < (int)real_y)
            {
                if (rdp.tiles(td).mirror_t)
                {
                    if (size == 1)
                        Mirror16bT((texture), rdp.tiles(td).mask_t,
                            real_y, real_x);
                    else if (size != 2)
                        Mirror8bT((texture), rdp.tiles(td).mask_t,
                            real_y, real_x);
                    else
                        Mirror32bT((texture), rdp.tiles(td).mask_t,
                            real_y, real_x);
                }
                else
                {
                    if (size == 1)
                        Wrap16bT((texture), rdp.tiles(td).mask_t,
                            real_y, real_x);
                    else if (size != 2)
                        Wrap8bT((texture), rdp.tiles(td).mask_t,
                            real_y, real_x);
                    else
                        Wrap32bT((texture), rdp.tiles(td).mask_t,
                            real_y, real_x);
                }
            }
        }

    if (modifyPalette)
    {
        memcpy(rdp.pal_8, tmp_pal, 512);
    }

    if (mod && !modifyPalette && !ghqTexInfo.data)
    {
        // Convert the texture to ARGB 4444
        if (LOWORD(result) == GFX_TEXFMT_ARGB_1555)
        {
            TexConv_ARGB1555_ARGB4444((texture), (tex2), real_x, real_y);
            texture = tex2;
        }
        else if (LOWORD(result) == GFX_TEXFMT_ALPHA_INTENSITY_88)
        {
            TexConv_AI88_ARGB4444((texture), (tex2), real_x, real_y);
            texture = tex2;
        }
        else if (LOWORD(result) == GFX_TEXFMT_ALPHA_INTENSITY_44)
        {
            TexConv_AI44_ARGB4444((texture), (tex2), real_x, real_y);
            texture = tex2;
        }
        else if (LOWORD(result) == GFX_TEXFMT_ALPHA_8)
        {
            TexConv_A8_ARGB4444((texture), (tex2), real_x, real_y);
            texture = tex2;
        }
        /*else if (LOWORD(result) == GFX_TEXFMT_ARGB_4444)
        {
        memcpy (tex2, texture, (real_x*real_y) << 1);
        texture = tex2;
        }*/ // we can skip memcpy since "texture" won't be swapped between "tex1" and "tex2" after this.
        // Hiroshi Morii <koolsmoky@users.sourceoforge.net>

        result = (1 << 16) | GFX_TEXFMT_ARGB_4444;

        // Now convert the color to the same
        modcolor = ((modcolor & 0xF0000000) >> 16) | ((modcolor & 0x00F00000) >> 12) |
            ((modcolor & 0x0000F000) >> 8) | ((modcolor & 0x000000F0) >> 4);
        modcolor1 = ((modcolor1 & 0xF0000000) >> 16) | ((modcolor1 & 0x00F00000) >> 12) |
            ((modcolor1 & 0x0000F000) >> 8) | ((modcolor1 & 0x000000F0) >> 4);
        modcolor2 = ((modcolor2 & 0xF0000000) >> 16) | ((modcolor2 & 0x00F00000) >> 12) |
            ((modcolor2 & 0x0000F000) >> 8) | ((modcolor2 & 0x000000F0) >> 4);

        int size = (real_x * real_y) << 1;

        switch (mod)
        {
        case TMOD_TEX_INTER_COLOR_USING_FACTOR:
            mod_tex_inter_color_using_factor((uint16_t*)texture, size, modcolor, modfactor);
            break;
        case TMOD_TEX_INTER_COL_USING_COL1:
            mod_tex_inter_col_using_col1((uint16_t*)texture, size, modcolor, modcolor1);
            break;
        case TMOD_FULL_COLOR_SUB_TEX:
            mod_full_color_sub_tex((uint16_t*)texture, size, modcolor);
            break;
        case TMOD_COL_INTER_COL1_USING_TEX:
            mod_col_inter_col1_using_tex((uint16_t*)texture, size, modcolor, modcolor1);
            break;
        case TMOD_COL_INTER_COL1_USING_TEXA:
            mod_col_inter_col1_using_texa((uint16_t*)texture, size, modcolor, modcolor1);
            break;
        case TMOD_COL_INTER_COL1_USING_TEXA__MUL_TEX:
            mod_col_inter_col1_using_texa__mul_tex((uint16_t*)texture, size, modcolor, modcolor1);
            break;
        case TMOD_COL_INTER_TEX_USING_TEXA:
            mod_col_inter_tex_using_texa((uint16_t*)texture, size, modcolor);
            break;
        case TMOD_COL2_INTER__COL_INTER_COL1_USING_TEX__USING_TEXA:
            mod_col2_inter__col_inter_col1_using_tex__using_texa((uint16_t*)texture, size, modcolor, modcolor1, modcolor2);
            break;
        case TMOD_TEX_SCALE_FAC_ADD_FAC:
            mod_tex_scale_fac_add_fac((uint16_t*)texture, size, modfactor);
            break;
        case TMOD_TEX_SUB_COL_MUL_FAC_ADD_TEX:
            mod_tex_sub_col_mul_fac_add_tex((uint16_t*)texture, size, modcolor, modfactor);
            break;
        case TMOD_TEX_SCALE_COL_ADD_COL:
            mod_tex_scale_col_add_col((uint16_t*)texture, size, modcolor, modcolor1);
            break;
        case TMOD_TEX_ADD_COL:
            mod_tex_add_col((uint16_t*)texture, size, modcolor);
            break;
        case TMOD_TEX_SUB_COL:
            mod_tex_sub_col((uint16_t*)texture, size, modcolor);
            break;
        case TMOD_TEX_SUB_COL_MUL_FAC:
            mod_tex_sub_col_mul_fac((uint16_t*)texture, size, modcolor, modfactor);
            break;
        case TMOD_COL_INTER_TEX_USING_COL1:
            mod_col_inter_tex_using_col1((uint16_t*)texture, size, modcolor, modcolor1);
            break;
        case TMOD_COL_MUL_TEXA_ADD_TEX:
            mod_col_mul_texa_add_tex((uint16_t*)texture, size, modcolor);
            break;
        case TMOD_COL_INTER_TEX_USING_TEX:
            mod_col_inter_tex_using_tex((uint16_t*)texture, size, modcolor);
            break;
        case TMOD_TEX_INTER_NOISE_USING_COL:
            mod_tex_inter_noise_using_col((uint16_t*)texture, size, modcolor);
            break;
        case TMOD_TEX_INTER_COL_USING_TEXA:
            mod_tex_inter_col_using_texa((uint16_t*)texture, size, modcolor);
            break;
        case TMOD_TEX_MUL_COL:
            mod_tex_mul_col((uint16_t*)texture, size, modcolor);
            break;
        case TMOD_TEX_SCALE_FAC_ADD_COL:
            mod_tex_scale_fac_add_col((uint16_t*)texture, size, modcolor, modfactor);
            break;
        default:
            ;
        }
    }

    cache->t_info.format = (gfxTextureFormat_t)LOWORD(result);

    cache->realwidth = real_x;
    cache->realheight = real_y;
    cache->lod = lod;
    cache->aspect = aspect;

    if (GfxInitDone)
    {
        if (g_ghq_use)
        {
            if (!ghqTexInfo.data)
                if (!g_settings->ghq_enht_nobg() || !rdp.texrecting || (texinfo[id].splits == 1 && texinfo[id].width <= 256))
                    ext_ghq_txfilter((unsigned char*)texture, (int)real_x, (int)real_y, LOWORD(result), (uint64_t)g64_crc, &ghqTexInfo);

            if (ghqTexInfo.data)
            {
                if (ghqTexInfo.aspectRatioLog2 < GFX_ASPECT_LOG2_1x8 ||
                    ghqTexInfo.aspectRatioLog2 > GFX_ASPECT_LOG2_8x1 ||
                    ghqTexInfo.largeLodLog2 > GFX_LOD_LOG2_2048 ||
                    ghqTexInfo.largeLodLog2 < GFX_LOD_LOG2_1)
                {
                    /* invalid dimensions */
                }
                else
                {
                    texture = (uint8_t *)ghqTexInfo.data;
                    lod = (gfxLOD_t)ghqTexInfo.largeLodLog2;
                    int splits = cache->splits;
                    if (ghqTexInfo.is_hires_tex)
                    {
                        if (ghqTexInfo.tiles)
                        {
                            cache->scale = 1.0f;
                            cache->c_off = 0.5f;
                            cache->splits = ghqTexInfo.tiles;//((hirestex.width-1)>>8)+1;
                            cache->splitheight = ghqTexInfo.untiled_height;
                            cache->scale_x = 1.0f;
                            cache->scale_y = float(ghqTexInfo.untiled_height*ghqTexInfo.tiles) / float(ghqTexInfo.width);//*sy;
                            if (splits == 1)
                            {
                                int shift;
                                for (shift = 9; (1 << shift) < ghqTexInfo.untiled_width; shift++);
                                float mult = float(1 << shift >> 8);
                                cache->c_scl_x *= mult;
                                cache->c_scl_y *= mult;
                            }
                            else
                            {
                                int tile_width = rdp.tiles(td).width;
                                if (rdp.timg.set_by == 1)
                                    tile_width = rdp.load_info[rdp.tiles(td).t_mem].tex_width;
                                float mult = float(ghqTexInfo.untiled_width / tile_width);
                                cache->c_scl_x *= mult;
                                cache->c_scl_y *= mult;
                            }
                        }
                        else
                        {
                            cache->scale = 256.0f / float(1 << lod);
                            cache->c_off = cache->scale * 0.5f;
                            cache->splits = 1;
                            if (aspect != ghqTexInfo.aspectRatioLog2)
                            {
                                float mscale = float(1 << abs(aspect - ghqTexInfo.aspectRatioLog2));
                                if (abs(aspect) > abs(ghqTexInfo.aspectRatioLog2))
                                {
                                    cache->c_scl_y *= mscale;
                                    cache->c_scl_x *= mscale;
                                }
                                if (ghqTexInfo.aspectRatioLog2 >= 0)
                                {
                                    cache->scale_x = 1.0f;
                                    cache->scale_y = 1.0f / float(1 << ghqTexInfo.aspectRatioLog2);
                                }
                                else
                                {
                                    cache->scale_y = 1.0f;
                                    cache->scale_x = 1.0f / float(1 << (-ghqTexInfo.aspectRatioLog2));
                                }
                            }
                            else if (splits > 1)
                            {
                                cache->c_scl_x /= splits;
                                cache->c_scl_y /= splits;
                            }
                        }
                        if (!g_settings->hacks(CSettings::hack_Zelda))
                        {
                            if (rdp.tiles(td).mirror_s && texinfo[id].tile_width == 2 * texinfo[id].width)
                                cache->f_mirror_s = TRUE;
                            else if (texinfo[id].tile_width >= 2 * texinfo[id].width)
                                cache->f_wrap_s = TRUE;
                            if (rdp.tiles(td).mirror_t && texinfo[id].tile_height == 2 * texinfo[id].height)
                                cache->f_mirror_t = TRUE;
                            else if (texinfo[id].tile_height >= 2 * texinfo[id].height)
                                cache->f_wrap_t = TRUE;
                            if (cache->f_mirror_s && cache->f_mirror_t)
                            {
                                cache->c_scl_x *= 2.0f;
                                cache->c_scl_y *= 2.0f;
                            }
                        }
                        aspect = (gfxAspectRatio_t)ghqTexInfo.aspectRatioLog2;
                        cache->lod = lod;
                        cache->aspect = aspect;
                    }
                    else
                    {
                        cache->c_off = 128.0f / float(1 << lod);
                    }
                    real_x = ghqTexInfo.width;
                    real_y = ghqTexInfo.height;
                    result = (1 << 16) | ghqTexInfo.format;
                    cache->t_info.format = (gfxTextureFormat_t)ghqTexInfo.format;
                    cache->realwidth = real_x;
                    cache->realheight = real_y;
                }
            }
        }

        // Load the texture into texture memory
        gfxTexInfo *t_info = &cache->t_info;
        t_info->data = texture;
        t_info->smallLodLog2 = lod;
        t_info->largeLodLog2 = lod;
        t_info->aspectRatioLog2 = aspect;

        uint32_t texture_size = gfxTexTextureMemRequired(GFX_MIPMAPLEVELMASK_BOTH, t_info);

        // Check for end of memory (too many textures to fit, clear cache)
        if (voodoo.tmem_ptr[tmu] + texture_size >= voodoo.tex_max_addr[tmu])
        {
            WriteTrace(TraceRDP, TraceDebug, "Cache size reached, clearing...");
            ClearCache();

            if (id == 1 && rdp.tex == 3)
                LoadTex(0, rdp.t0);

            LoadTex(id, tmu);
            return;
            // DON'T CONTINUE (already done)
        }

        uint32_t tex_addr = voodoo.tex_min_addr[0] + voodoo.tmem_ptr[0];
        voodoo.tmem_ptr[0] += texture_size;
        voodoo.tmem_ptr[1] = voodoo.tmem_ptr[0];
        gfxTexDownloadMipMap(tmu, tex_addr, GFX_MIPMAPLEVELMASK_BOTH, t_info);
        gfxTexSource(tmu, tex_addr, GFX_MIPMAPLEVELMASK_BOTH, t_info);
    }
    WriteTrace(TraceRDP, TraceDebug, " | | +- LoadTex end");
}