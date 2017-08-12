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
#include <Common/StdString.h>
#include "Gfx_1.3.h"
#include "Version.h"
#include <Common/CriticalSection.h>
#include <Common/DateTimeClass.h>
#include <Common/path.h>
#include <png/png.h>
#include <memory>
#include <Common/SmartPointer.h>
#include <Settings/Settings.h>

#include "Config.h"
#include "Util.h"
#include "3dmath.h"
#include "Debugger.h"
#include "Combine.h"
#include "TexCache.h"
#include "CRC.h"
#include "FBtoScreen.h"
#include "DepthBufferRender.h"
#include "trace.h"
#include "ScreenResolution.h"

#ifdef _WIN32
#include <commctrl.h>
#endif

#include <stdarg.h>

#ifdef ANDROID
uint32_t g_ScreenWidth, g_ScreenHeight;
#endif

GFX_INFO gfx;

int to_fullscreen = FALSE;
int GfxInitDone = FALSE;
bool g_romopen = false;
int exception = FALSE;

int evoodoo = 0;
int ev_fullscreen = 0;

extern int g_viewport_offset;
extern int g_width, g_height;

#ifdef _WIN32
HINSTANCE hinstDLL = NULL;
#endif

uint32_t   region = 0;

unsigned int BMASK = 0x7FFFFF;
// Reality display processor structure
CRDP rdp;

CSettings * g_settings = NULL;

VOODOO voodoo = { 0, 0, 0, 0,
0, 0, 0, 0,
0, 0, 0, 0
};

gfxTexInfo fontTex;
gfxTexInfo cursorTex;
uint32_t   offset_font = 0;
uint32_t   offset_cursor = 0;
uint32_t   offset_textures = 0;
uint32_t   offset_texbuf1 = 0;

bool g_ghq_use = false;
bool g_capture_screen = false;
std::string g_capture_path;

#ifdef _WIN32
HWND g_hwnd_win = NULL;
static RECT g_windowedRect = { 0 };
static HMENU g_windowedMenu = 0;
static unsigned long g_windowedExStyle, g_windowedStyle;
bool g_fullscreen;
#endif // _WIN32

void ChangeSize()
{
    WriteTrace(TraceResolution, TraceDebug, "Start");
#ifdef ANDROID
    g_width = g_ScreenWidth;
    g_height = g_ScreenHeight;
#else
    g_width = ev_fullscreen ? GetFullScreenResWidth(g_settings->FullScreenRes()) : GetScreenResWidth(g_settings->ScreenRes());
    g_height = ev_fullscreen ? GetFullScreenResHeight(g_settings->FullScreenRes()) : GetScreenResHeight(g_settings->ScreenRes());
#endif
    g_scr_res_x = g_res_x = g_width;
    g_scr_res_y = g_res_y = g_height;

    switch (g_settings->aspectmode())
    {
    case CSettings::Aspect_4x3:
        if (g_scr_res_x >= g_scr_res_y * 4.0f / 3.0f)
        {
            g_res_y = g_scr_res_y;
            g_res_x = (uint32_t)(g_res_y * 4.0f / 3.0f);
        }
        else
        {
            g_res_x = g_scr_res_x;
            g_res_y = (uint32_t)(g_res_x / 4.0f * 3.0f);
        }
        break;
    case CSettings::Aspect_16x9:
        if (g_scr_res_x >= g_scr_res_y * 16.0f / 9.0f)
        {
            g_res_y = g_scr_res_y;
            g_res_x = (uint32_t)(g_res_y * 16.0f / 9.0f);
        }
        else
        {
            g_res_x = g_scr_res_x;
            g_res_y = (uint32_t)(g_res_x / 16.0f * 9.0f);
        }
        break;
    default: //stretch or original
        g_res_x = g_scr_res_x;
        g_res_y = g_scr_res_y;
    }

    float res_scl_y = (float)g_res_y / 240.0f;

    uint32_t scale_x = *gfx.VI_X_SCALE_REG & 0xFFF;
    uint32_t scale_y = *gfx.VI_Y_SCALE_REG & 0xFFF;
    if (scale_x != 0 && scale_y != 0)
    {
        float fscale_x = (float)scale_x / 1024.0f;
        float fscale_y = (float)scale_y / 2048.0f;

        uint32_t dwHStartReg = *gfx.VI_H_START_REG;
        uint32_t dwVStartReg = *gfx.VI_V_START_REG;

        uint32_t hstart = dwHStartReg >> 16;
        uint32_t hend = dwHStartReg & 0xFFFF;

        // dunno... but sometimes this happens
        if (hend == hstart) hend = (int)(*gfx.VI_WIDTH_REG / fscale_x);

        uint32_t vstart = dwVStartReg >> 16;
        uint32_t vend = dwVStartReg & 0xFFFF;

        rdp.vi_width = (hend - hstart) * fscale_x;
        rdp.vi_height = (vend - vstart) * fscale_y * 1.0126582f;
        float aspect = (g_settings->adjust_aspect() && (fscale_y > fscale_x) && (rdp.vi_width > rdp.vi_height)) ? fscale_x / fscale_y : 1.0f;

        WriteTrace(TraceResolution, TraceDebug, "hstart: %d, hend: %d, vstart: %d, vend: %d", hstart, hend, vstart, vend);
        WriteTrace(TraceResolution, TraceDebug, "size: %d x %d", (int)rdp.vi_width, (int)rdp.vi_height);

        rdp.scale_x = (float)g_res_x / rdp.vi_width;
        if (region > 0 && g_settings->pal230())
        {
            // odd... but pal games seem to want 230 as height...
            rdp.scale_y = res_scl_y * (230.0f / rdp.vi_height)  * aspect;
        }
        else
        {
            rdp.scale_y = (float)g_res_y / rdp.vi_height * aspect;
        }
        rdp.offset_y = ((float)g_res_y - rdp.vi_height * rdp.scale_y) * 0.5f;
        if (((uint32_t)rdp.vi_width <= (*gfx.VI_WIDTH_REG) / 2) && (rdp.vi_width > rdp.vi_height))
            rdp.scale_y *= 0.5f;

        rdp.scissor_o.ul_x = 0;
        rdp.scissor_o.ul_y = 0;
        rdp.scissor_o.lr_x = (uint32_t)rdp.vi_width;
        rdp.scissor_o.lr_y = (uint32_t)rdp.vi_height;

        rdp.update |= UPDATE_VIEWPORT | UPDATE_SCISSOR;
    }
    rdp.offset_x = (g_scr_res_x - g_res_x) / 2.0f;
    float offset_y = (g_scr_res_y - g_res_y) / 2.0f;
    rdp.offset_y += offset_y;
    if (g_settings->aspectmode() == CSettings::Aspect_Original)
    {
        rdp.scale_x = rdp.scale_y = 1.0f;
        rdp.offset_x = (g_scr_res_x - rdp.vi_width) / 2.0f;
        rdp.offset_y = (g_scr_res_y - rdp.vi_height) / 2.0f;
    }
    WriteTrace(TraceResolution, TraceDebug, "rdp.offset_x = %f rdp.offset_y = %f rdp.scale_x = %f, rdp.scale_y = %f", rdp.offset_x, rdp.offset_y, rdp.scale_x, rdp.scale_y);
    WriteTrace(TraceResolution, TraceDebug, "Done");
}

extern int g_width, g_height;

int GetTexAddrUMA(int /*tmu*/, int texsize)
{
    int addr = voodoo.tex_min_addr[0] + voodoo.tmem_ptr[0];
    voodoo.tmem_ptr[0] += texsize;
    voodoo.tmem_ptr[1] = voodoo.tmem_ptr[0];
    return addr;
}
int GetTexAddrNonUMA(int tmu, int texsize)
{
    int addr = voodoo.tex_min_addr[tmu] + voodoo.tmem_ptr[tmu];
    voodoo.tmem_ptr[tmu] += texsize;
    return addr;
}

// guLoadTextures - used to load the cursor and font textures
void guLoadTextures()
{
    int tbuf_size = 0;
    if (voodoo.max_tex_size <= 256)
    {
        gfxTextureBufferExt(GFX_TMU1, voodoo.tex_min_addr[GFX_TMU1], GFX_LOD_LOG2_256, GFX_LOD_LOG2_256,
            GFX_ASPECT_LOG2_1x1, GFX_TEXFMT_RGB_565, GFX_MIPMAPLEVELMASK_BOTH);
        tbuf_size = 8 * gfxTexCalcMemRequired(GFX_LOD_LOG2_256, GFX_LOD_LOG2_256,
            GFX_ASPECT_LOG2_1x1, GFX_TEXFMT_RGB_565);
    }
    else if (g_scr_res_x <= 1024)
    {
        gfxTextureBufferExt(GFX_TMU0, voodoo.tex_min_addr[GFX_TMU0], GFX_LOD_LOG2_1024, GFX_LOD_LOG2_1024,
            GFX_ASPECT_LOG2_1x1, GFX_TEXFMT_RGB_565, GFX_MIPMAPLEVELMASK_BOTH);
        tbuf_size = gfxTexCalcMemRequired(GFX_LOD_LOG2_1024, GFX_LOD_LOG2_1024,
            GFX_ASPECT_LOG2_1x1, GFX_TEXFMT_RGB_565);
        gfxRenderBuffer(GFX_BUFFER_TEXTUREBUFFER_EXT);
        gfxBufferClear(0, 0, 0xFFFF);
        gfxRenderBuffer(GFX_BUFFER_BACKBUFFER);
    }
    else
    {
        gfxTextureBufferExt(GFX_TMU0, voodoo.tex_min_addr[GFX_TMU0], GFX_LOD_LOG2_2048, GFX_LOD_LOG2_2048,
            GFX_ASPECT_LOG2_1x1, GFX_TEXFMT_RGB_565, GFX_MIPMAPLEVELMASK_BOTH);
        tbuf_size = gfxTexCalcMemRequired(GFX_LOD_LOG2_2048, GFX_LOD_LOG2_2048,
            GFX_ASPECT_LOG2_1x1, GFX_TEXFMT_RGB_565);
        gfxRenderBuffer(GFX_BUFFER_TEXTUREBUFFER_EXT);
        gfxBufferClear(0, 0, 0xFFFF);
        gfxRenderBuffer(GFX_BUFFER_BACKBUFFER);
    }

    rdp.texbufs[0].tmu = GFX_TMU0;
    rdp.texbufs[0].begin = voodoo.tex_min_addr[GFX_TMU0];
    rdp.texbufs[0].end = rdp.texbufs[0].begin + tbuf_size;
    rdp.texbufs[0].count = 0;
    rdp.texbufs[0].clear_allowed = TRUE;
    offset_font = tbuf_size;
    if ((nbTextureUnits > 2 ? 2 : 1) > 1)
    {
        rdp.texbufs[1].tmu = GFX_TMU1;
        rdp.texbufs[1].begin = rdp.texbufs[0].end;
        rdp.texbufs[1].end = rdp.texbufs[1].begin + tbuf_size;
        rdp.texbufs[1].count = 0;
        rdp.texbufs[1].clear_allowed = TRUE;
        offset_font += tbuf_size;
    }

#include "font.h"
    uint32_t *data = (uint32_t*)font;
    uint32_t cur;

    // ** Font texture **
    uint8_t *tex8 = (uint8_t*)malloc(256 * 64);

    fontTex.smallLodLog2 = fontTex.largeLodLog2 = GFX_LOD_LOG2_256;
    fontTex.aspectRatioLog2 = GFX_ASPECT_LOG2_4x1;
    fontTex.format = GFX_TEXFMT_ALPHA_8;
    fontTex.data = tex8;

    // Decompression: [1-bit inverse alpha --> 8-bit alpha]
    uint32_t i, b;
    for (i = 0; i < 0x200; i++)
    {
        // cur = ~*(data++), byteswapped
#ifdef __VISUALC__
        cur = _byteswap_ulong(~*(data++));
#else
        cur = ~*(data++);
        cur = ((cur & 0xFF) << 24) | (((cur >> 8) & 0xFF) << 16) | (((cur >> 16) & 0xFF) << 8) | ((cur >> 24) & 0xFF);
#endif

        for (b = 0x80000000; b != 0; b >>= 1)
        {
            if (cur&b) *tex8 = 0xFF;
            else *tex8 = 0x00;
            tex8++;
        }
    }

    gfxTexDownloadMipMap(GFX_TMU0,
        voodoo.tex_min_addr[GFX_TMU0] + offset_font,
        GFX_MIPMAPLEVELMASK_BOTH,
        &fontTex);

    offset_cursor = offset_font + gfxTexTextureMemRequired(GFX_MIPMAPLEVELMASK_BOTH, &fontTex);

    free(fontTex.data);

    // ** Cursor texture **
#include "cursor.h"
    data = (uint32_t*)cursor;

    uint16_t *tex16 = (uint16_t*)malloc(32 * 32 * 2);

    cursorTex.smallLodLog2 = cursorTex.largeLodLog2 = GFX_LOD_LOG2_32;
    cursorTex.aspectRatioLog2 = GFX_ASPECT_LOG2_1x1;
    cursorTex.format = GFX_TEXFMT_ARGB_1555;
    cursorTex.data = tex16;

    // Conversion: [16-bit 1555 (swapped) --> 16-bit 1555]
    for (i = 0; i < 0x200; i++)
    {
        cur = *(data++);
        *(tex16++) = (uint16_t)(((cur & 0x000000FF) << 8) | ((cur & 0x0000FF00) >> 8));
        *(tex16++) = (uint16_t)(((cur & 0x00FF0000) >> 8) | ((cur & 0xFF000000) >> 24));
    }

    gfxTexDownloadMipMap(GFX_TMU0,
        voodoo.tex_min_addr[GFX_TMU0] + offset_cursor,
        GFX_MIPMAPLEVELMASK_BOTH,
        &cursorTex);

    // Round to higher 16
    offset_textures = ((offset_cursor + gfxTexTextureMemRequired(GFX_MIPMAPLEVELMASK_BOTH, &cursorTex))
        & 0xFFFFFFF0) + 16;
    free(cursorTex.data);
}

void DisplayLoadProgress(const wchar_t *format, ...)
{
    va_list args;
    wchar_t wbuf[INFO_BUF];
    char buf[INFO_BUF];

    // process input
    va_start(args, format);
    vswprintf(wbuf, INFO_BUF, format, args);
    va_end(args);

    // XXX: convert to multibyte
    wcstombs(buf, wbuf, INFO_BUF);

    float x;
    set_message_combiner();
    output(382, 380, 1, "LOADING TEXTURES. PLEASE WAIT...");
    int len = minval((int)strlen(buf) * 8, 1024);
    x = (1024 - len) / 2.0f;
    output(x, 360, 1, buf);
    gfxBufferSwap(0);
    gfxColorMask(true, true);
    gfxBufferClear(0, 0, 0xFFFF);
}

#ifdef _WIN32
void SetWindowDisplaySize(HWND hWnd)
{
    if (hWnd == NULL)
    {
        hWnd = GetActiveWindow();
    }
    g_hwnd_win = (HWND)hWnd;

    if (ev_fullscreen)
    {
        ZeroMemory(&g_windowedRect, sizeof(RECT));
        GetWindowRect(hWnd, &g_windowedRect);

        g_windowedExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        g_windowedStyle = GetWindowLong(hWnd, GWL_STYLE);

        // primary monitor only
        if (!EnterFullScreen(g_settings->FullScreenRes()))
        {
            WriteTrace(TraceGlitch, TraceWarning, "can't change to fullscreen mode");
        }

        g_windowedMenu = GetMenu(hWnd);
        if (g_windowedMenu) SetMenu(hWnd, NULL);

        HWND hStatusBar = FindWindowEx(hWnd, NULL, "msctls_statusbar32", NULL); // 1964
        if (hStatusBar) ShowWindow(hStatusBar, SW_HIDE);

        SetWindowLong(hWnd, GWL_STYLE, 0);
        SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
        SetWindowPos(hWnd, NULL, 0, 0, g_width, g_height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);

        g_viewport_offset = 0;
        g_fullscreen = true;
    }
    else
    {
        RECT clientRect = { 0 }, toolbarRect = { 0 }, statusbarRect = { 0 }, windowedRect = { 0 };
        HWND hToolBar = FindWindowEx(hWnd, NULL, REBARCLASSNAME, NULL);
        HWND hStatusBar = FindWindowEx(hWnd, NULL, STATUSCLASSNAME, NULL);
        if (hStatusBar == NULL)
        {
            hStatusBar = FindWindowEx(hWnd, NULL, "msctls_statusbar32", NULL);
        }
        if (hToolBar != NULL)
        {
            GetWindowRect(hToolBar, &toolbarRect);
        }
        if (hStatusBar != NULL)
        {
            GetWindowRect(hStatusBar, &statusbarRect);
        }
        g_viewport_offset = statusbarRect.bottom - statusbarRect.top;
        GetWindowRect(hWnd, &g_windowedRect);
        GetClientRect(hWnd, &clientRect);
        g_windowedRect.right += (g_width - (clientRect.right - clientRect.left));
        g_windowedRect.bottom += (g_height + (toolbarRect.bottom - toolbarRect.top) + (statusbarRect.bottom - statusbarRect.top) - (clientRect.bottom - clientRect.top));
        SetWindowPos(hWnd, NULL, 0, 0, g_windowedRect.right - g_windowedRect.left, g_windowedRect.bottom - g_windowedRect.top, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);

        g_fullscreen = false;
    }
}

void ExitFullScreen(void)
{
    if (g_fullscreen)
    {
        ChangeDisplaySettings(NULL, 0);
        SetWindowPos(g_hwnd_win, NULL, g_windowedRect.left, g_windowedRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowLong(g_hwnd_win, GWL_STYLE, g_windowedStyle);
        SetWindowLong(g_hwnd_win, GWL_EXSTYLE, g_windowedExStyle);
        if (g_windowedMenu) SetMenu(g_hwnd_win, g_windowedMenu);
        g_fullscreen = false;
    }
}

#endif

void setPattern();

int InitGfx()
{
    if (GfxInitDone)
    {
        ReleaseGfx();
    }

    WriteTrace(TraceGlide64, TraceDebug, "-");

    // Check which SST we are using and initialize stuff
    // Hiroshi Morii <koolsmoky@users.sourceforge.net>
    enum {
        GR_SSTTYPE_VOODOO = 0,
        GR_SSTTYPE_SST96 = 1,
        GR_SSTTYPE_AT3D = 2,
        GR_SSTTYPE_Voodoo2 = 3,
        GR_SSTTYPE_Banshee = 4,
        GR_SSTTYPE_Voodoo3 = 5,
        GR_SSTTYPE_Voodoo4 = 6,
        GR_SSTTYPE_Voodoo5 = 7
    };
    unsigned int SST_type = GR_SSTTYPE_Voodoo5;
    // 2Mb Texture boundary
    voodoo.has_2mb_tex_boundary = (SST_type < GR_SSTTYPE_Banshee) && !evoodoo;
    // we get better texture cache hits with UMA on
    WriteTrace(TraceGlide64, TraceDebug, "Using TEXUMA extension");

    ChangeSize();
#ifndef ANDROID
    SetWindowDisplaySize((HWND)gfx.hWnd);
#endif
    if (!gfxSstWinOpen(GFX_COLORFORMAT_RGBA, GFX_ORIGIN_UPPER_LEFT, 2, 1))
    {
        g_Notify->DisplayError("Error setting display mode");
        return FALSE;
    }
    rdp.init();
    util_init();

    GfxInitDone = TRUE;
    to_fullscreen = FALSE;

    // get maximal texture size
    voodoo.max_tex_size = 2048;
    voodoo.sup_large_tex = (voodoo.max_tex_size > 256 && !g_settings->hacks(CSettings::hack_PPL));

    voodoo.tex_min_addr[0] = voodoo.tex_min_addr[1] = gfxTexMinAddress(GFX_TMU0);
    voodoo.tex_max_addr[0] = voodoo.tex_max_addr[1] = gfxTexMaxAddress(GFX_TMU0);

    // Is mirroring allowed?
    if (!g_settings->hacks(CSettings::hack_Zelda)) //zelda's trees suffer from hardware mirroring
        voodoo.sup_mirroring = 1;
    else
        voodoo.sup_mirroring = 0;

    voodoo.sup_32bit_tex = TRUE;
    voodoo.gamma_correction = 0;
    voodoo.gamma_table_size = 256;

    srand(g_settings->stipple_pattern());
    setPattern();

    InitCombine();

#ifdef SIMULATE_VOODOO1
    voodoo.sup_mirroring = 0;
#endif

#ifdef SIMULATE_BANSHEE
    voodoo.sup_mirroring = 1;
#endif

    gfxCullMode(GFX_CULL_NEGATIVE);

    if (g_settings->fog()) //"FOGCOORD" extension
    {
        gfxFogGenerateLinear(0.0f, 255.0f);
    }

    gfxDepthBufferMode(GFX_DEPTHBUFFER_ZBUFFER);
    gfxDepthBufferFunction(GFX_CMP_LESS);
    gfxDepthMask(true);

    ChangeSize();

    guLoadTextures();
    ClearCache();

    gfxCullMode(GFX_CULL_DISABLE);
    gfxDepthBufferMode(GFX_DEPTHBUFFER_ZBUFFER);
    gfxDepthBufferFunction(GFX_CMP_ALWAYS);
    gfxRenderBuffer(GFX_BUFFER_BACKBUFFER);
    gfxColorMask(true, true);
    gfxDepthMask(true);
    gfxBufferClear(0, 0, 0xFFFF);
    gfxBufferSwap(0);
    gfxBufferClear(0, 0, 0xFFFF);
    gfxDepthMask(false);
    gfxTexFilterMode(GFX_TMU0, GFX_TEXTUREFILTER_BILINEAR, GFX_TEXTUREFILTER_BILINEAR);
    gfxTexFilterMode(GFX_TMU1, GFX_TEXTUREFILTER_BILINEAR, GFX_TEXTUREFILTER_BILINEAR);
    gfxTexClampMode(GFX_TMU0, GFX_TEXTURECLAMP_CLAMP, GFX_TEXTURECLAMP_CLAMP);
    gfxTexClampMode(GFX_TMU1, GFX_TEXTURECLAMP_CLAMP, GFX_TEXTURECLAMP_CLAMP);
    gfxClipWindow(0, 0, g_scr_res_x, g_scr_res_y);
    rdp.update |= UPDATE_SCISSOR | UPDATE_COMBINE | UPDATE_ZBUF_ENABLED | UPDATE_CULL_MODE;

    if (!g_ghq_use)
    {
        g_ghq_use = g_settings->ghq_fltr() != CSettings::TextureFilter_None || g_settings->ghq_enht() != CSettings::TextureEnht_None || g_settings->ghq_hirs() != CSettings::HiResPackFormat_None;
        if (g_ghq_use)
        {
            /* Plugin path */
            int options = g_settings->ghq_fltr() | g_settings->ghq_enht() | g_settings->ghq_cmpr() | g_settings->ghq_hirs();
            if (g_settings->ghq_enht_cmpr())
            {
                options |= COMPRESS_TEX;
            }
            if (g_settings->ghq_hirs_cmpr())
            {
                options |= COMPRESS_HIRESTEX;
            }
            if (g_settings->ghq_hirs_tile())
            {
                options |= TILE_HIRESTEX;
            }
            if (g_settings->ghq_enht_f16bpp())
            {
                options |= FORCE16BPP_TEX;
            }
            if (g_settings->ghq_hirs_f16bpp())
            {
                options |= FORCE16BPP_HIRESTEX;
            }
            if (g_settings->ghq_enht_gz())
            {
                options |= GZ_TEXCACHE;
            }
            if (g_settings->ghq_hirs_gz())
            {
                options |= GZ_HIRESTEXCACHE;
            }
            if (g_settings->ghq_cache_save())
            {
                options |= (DUMP_TEXCACHE | DUMP_HIRESTEXCACHE);
            }
            if (g_settings->ghq_hirs_let_texartists_fly())
            {
                options |= LET_TEXARTISTS_FLY;
            }
            if (g_settings->ghq_hirs_dump())
            {
                options |= DUMP_TEX;
            }

            g_ghq_use = (int)ext_ghq_init(voodoo.max_tex_size, // max texture width supported by hardware
                voodoo.max_tex_size, // max texture height supported by hardware
                voodoo.sup_32bit_tex ? 32 : 16, // max texture bpp supported by hardware
                options,
                g_settings->ghq_cache_size() * 1024 * 1024, // cache texture to system memory
                g_settings->texture_dir(),
                rdp.RomName, // name of ROM. must be no longer than 256 characters
                DisplayLoadProgress);
        }
    }
    if (g_ghq_use)
    {
        voodoo.sup_mirroring = 1;
    }
    return TRUE;
}

void ReleaseGfx()
{
    WriteTrace(TraceGlide64, TraceDebug, "-");

    rdp.free();

    // Restore gamma settings
    if (voodoo.gamma_correction)
    {
        if (voodoo.gamma_table_r)
            gfxLoadGammaTable(voodoo.gamma_table_size, voodoo.gamma_table_r, voodoo.gamma_table_g, voodoo.gamma_table_b);
        else
            gfxGammaCorrectionRGB(1.3f, 1.3f, 1.3f); //1.3f is default 3dfx gamma for everything but desktop
        voodoo.gamma_correction = 0;
    }

    // Release graphics
    gfxSstWinClose();

    GfxInitDone = FALSE;
    rdp.window_changed = TRUE;
}

#ifdef _WIN32
CriticalSection * g_ProcessDListCS = NULL;

extern "C" int WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID /*lpReserved*/)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        hinstDLL = hinst;
        if (g_ProcessDListCS == NULL)
        {
            g_ProcessDListCS = new CriticalSection();
        }
        ConfigInit(hinst);
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (g_ProcessDListCS)
        {
            delete g_ProcessDListCS;
        }
        ConfigCleanup();
    }
    return TRUE;
}
#endif

/******************************************************************
Function: CaptureScreen
Purpose:  This function dumps the current frame to a file
input:    pointer to the directory to save the file to
output:   none
*******************************************************************/
EXPORT void CALL CaptureScreen(char * Directory)
{
    g_capture_screen = true;
    g_capture_path = Directory;
}

/******************************************************************
Function: ChangeWindow
Purpose:  to change the window between fullscreen and window
mode. If the window was in fullscreen this should
change the screen to window mode and vice vesa.
input:    none
output:   none
*******************************************************************/
EXPORT void CALL ChangeWindow(void)
{
    WriteTrace(TraceGlide64, TraceDebug, "-");

    if (evoodoo)
    {
        if (!ev_fullscreen)
        {
            to_fullscreen = TRUE;
            ev_fullscreen = TRUE;
        }
        else
        {
            ev_fullscreen = FALSE;
            InitGfx();
        }
    }
    else
    {
        // Go to fullscreen at next dlist
        // This is for compatibility with 1964, which reloads the plugin
        //  when switching to fullscreen
        if (!GfxInitDone)
        {
            to_fullscreen = TRUE;
        }
        else
        {
            ReleaseGfx();
        }
    }
}

/******************************************************************
Function: CloseDLL
Purpose:  This function is called when the emulator is closing
down allowing the dll to de-initialise.
input:    none
output:   none
*******************************************************************/
void CALL CloseDLL(void)
{
    WriteTrace(TraceGlide64, TraceDebug, "-");

    if (g_ghq_use)
    {
        ext_ghq_shutdown();
        g_ghq_use = false;
    }

    if (g_settings)
    {
        delete g_settings;
        g_settings = NULL;
    }

    ReleaseGfx();
    ZLUT_release();
    ClearCache();
    delete[] voodoo.gamma_table_r;
    voodoo.gamma_table_r = 0;
    delete[] voodoo.gamma_table_g;
    voodoo.gamma_table_g = 0;
    delete[] voodoo.gamma_table_b;
    voodoo.gamma_table_b = 0;
}

/******************************************************************
Function: DllTest
Purpose:  This function is optional function that is provided
to allow the user to test the dll
input:    a handle to the window that calls this function
output:   none
*******************************************************************/
void CALL DllTest(void * /*hParent*/)
{
}

/******************************************************************
Function: DrawScreen
Purpose:  This function is called when the emulator receives a
WM_PAINT message. This allows the gfx to fit in when
it is being used in the desktop.
input:    none
output:   none
*******************************************************************/
void CALL DrawScreen(void)
{
    WriteTrace(TraceGlide64, TraceDebug, "-");
}

/******************************************************************
Function: GetDllInfo
Purpose:  This function allows the emulator to gather information
about the dll by filling in the PluginInfo structure.
input:    a pointer to a PLUGIN_INFO stucture that needs to be
filled by the function. (see def above)
output:   none
*******************************************************************/
void CALL GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = 0x0104;     // Set to 0x0104
    PluginInfo->Type = PLUGIN_TYPE_GFX;  // Set to PLUGIN_TYPE_GFX
#ifdef _DEBUG
    sprintf(PluginInfo->Name, "Project64 Video Plugin (Debug): %s", VER_FILE_VERSION_STR);
#else
    sprintf(PluginInfo->Name, "Project64 Video Plugin: %s", VER_FILE_VERSION_STR);
#endif

    // If DLL supports memory these memory options then set them to TRUE or FALSE
    //  if it does not support it
    PluginInfo->NormalMemory = FALSE;  // a normal uint8_t array
    PluginInfo->MemoryBswaped = TRUE; // a normal uint8_t array where the memory has been pre
    // bswap on a dword (32 bits) boundry
}

/******************************************************************
Function: InitiateGFX
Purpose:  This function is called when the DLL is started to give
information from the emulator that the n64 graphics
uses. This is not called from the emulation thread.
Input:    Gfx_Info is passed to this function which is defined
above.
Output:   TRUE on success
FALSE on failure to initialise

** note on interrupts **:
To generate an interrupt set the appropriate bit in MI_INTR_REG
and then call the function CheckInterrupts to tell the emulator
that there is a waiting interrupt.
*******************************************************************/

int CALL InitiateGFX(GFX_INFO Gfx_Info)
{
    WriteTrace(TraceInterface, TraceDebug, "Start");

    // Assume scale of 1 for debug purposes
    rdp.scale_x = 1.0f;
    rdp.scale_y = 1.0f;

    char name[21] = "DEFAULT";
    g_settings->ReadGameSettings(name);
    ZLUT_init();

    gfx = Gfx_Info;

    if (!rdp.init())
    {
        return false;
    }
    math_init();
    TexCacheInit();
    CRC_BuildTable();
    CountCombine();
    ZLUT_init();

    evoodoo = 1;
    voodoo.has_2mb_tex_boundary = 0;
    return TRUE;
}

/******************************************************************
Function: MoveScreen
Purpose:  This function is called in response to the emulator
receiving a WM_MOVE passing the xpos and ypos passed
from that message.
input:    xpos - the x-coordinate of the upper-left corner of the
client area of the window.
ypos - y-coordinate of the upper-left corner of the
client area of the window.
output:   none
*******************************************************************/
void CALL MoveScreen(int xpos, int ypos)
{
    xpos = xpos;
    ypos = ypos;
    WriteTrace(TraceGlide64, TraceDebug, "xpos: %d ypos: %d", xpos, ypos);
    rdp.window_changed = TRUE;
}

void CALL PluginLoaded(void)
{
    SetupTrace();
    if (g_settings == NULL)
    {
        g_settings = new CSettings;
    }
    StartTrace();

    WriteTrace(TraceInterface, TraceDebug, "Start");
    WriteTrace(TraceInterface, TraceDebug, "Done");
}

#ifdef ANDROID
void vbo_disable(void);
#endif

/******************************************************************
Function: RomClosed
Purpose:  This function is called when a rom is closed.
input:    none
output:   none
*******************************************************************/
void CALL RomClosed(void)
{
    WriteTrace(TraceGlide64, TraceDebug, "-");

#ifdef ANDROID
    vbo_disable();
#endif
    rdp.window_changed = TRUE;
    g_romopen = FALSE;
    ReleaseGfx();
}

static void CheckDRAMSize()
{
    uint32_t test;
    GLIDE64_TRY
    {
        test = gfx.RDRAM[0x007FFFFF] + 1;
    }
        GLIDE64_CATCH
    {
        test = 0;
    }
        if (test)
            BMASK = 0x7FFFFF;
        else
            BMASK = WMASK;
#ifdef LOGGING
    sprintf(out_buf, "Detected RDRAM size: %08lx", BMASK);
    LOG(out_buf);
#endif
}

/******************************************************************
Function: RomOpen
Purpose:  This function is called when a rom is open. (from the
emulation thread)
input:    none
output:   none
*******************************************************************/
void CALL RomOpen(void)
{
    WriteTrace(TraceGlide64, TraceDebug, "-");
    no_dlist = true;
    g_romopen = TRUE;
    g_ucode_error_report = TRUE;	// allowed to report ucode errors

    // Get the country code & translate to NTSC(0) or PAL(1)
    uint16_t code = ((uint16_t*)gfx.HEADER)[0x1F ^ 1];

    if (code == 0x4400) region = 1; // Germany (PAL)
    if (code == 0x4500) region = 0; // USA (NTSC)
    if (code == 0x4A00) region = 0; // Japan (NTSC)
    if (code == 0x5000) region = 1; // Europe (PAL)
    if (code == 0x5500) region = 0; // Australia (NTSC)

    // get the name of the ROM
    char name[21];
    for (int i = 0; i < 20; i++)
    {
        char ch;
        const char invalid_ch = '?'; /* Some Japanese games use wide chars. */

        ch = (char)gfx.HEADER[(32 + i) ^ 3];
        if (ch == '\0')
            ch = ' ';
        if (ch < ' ')
            ch = invalid_ch;
        if (ch > '~')
            ch = invalid_ch;
        name[i] = ch;
    }
    name[20] = '\0';

    // remove all trailing spaces
    while (name[strlen(name) - 1] == ' ')
    {
        name[strlen(name) - 1] = 0;
    }

    if (g_ghq_use && strcmp(rdp.RomName, name) != 0)
    {
        ext_ghq_shutdown();
        g_ghq_use = false;
    }
    strcpy(rdp.RomName, name);
    g_settings->ReadGameSettings(name);
    ClearCache();

    CheckDRAMSize();
    // ** EVOODOO EXTENSIONS **
    evoodoo = 1;
    InitGfx();
}

/******************************************************************
Function: ShowCFB
Purpose:  Useally once Dlists are started being displayed, cfb is
ignored. This function tells the dll to start displaying
them again.
input:    none
output:   none
*******************************************************************/
bool no_dlist = true;
void CALL ShowCFB(void)
{
    WriteTrace(TraceGlide64, TraceDebug, "-");
    no_dlist = true;
}

void drawViRegBG()
{
    WriteTrace(TraceGlide64, TraceDebug, "start");
    FB_TO_SCREEN_INFO fb_info;
    fb_info.width = *gfx.VI_WIDTH_REG;
    fb_info.height = (uint32_t)rdp.vi_height;
    if (fb_info.height == 0)
    {
        WriteTrace(TraceRDP, TraceDebug, "Image height = 0 - skipping");
        return;
    }
    fb_info.ul_x = 0;

    fb_info.lr_x = fb_info.width - 1;
    //  fb_info.lr_x = (uint32_t)rdp.vi_width - 1;
    fb_info.ul_y = 0;
    fb_info.lr_y = fb_info.height - 1;
    fb_info.opaque = 1;
    fb_info.addr = *gfx.VI_ORIGIN_REG;
    fb_info.size = *gfx.VI_STATUS_REG & 3;
    rdp.last_bg = fb_info.addr;

    bool drawn = DrawFrameBufferToScreen(fb_info);
    if (g_settings->hacks(CSettings::hack_Lego) && drawn)
    {
        rdp.updatescreen = 1;
        newSwapBuffers();
        DrawFrameBufferToScreen(fb_info);
    }
    WriteTrace(TraceGlide64, TraceDebug, "done");
}

static void DrawFrameBuffer()
{
    if (to_fullscreen)
        GoToFullScreen();

    gfxDepthMask(true);
    gfxColorMask(true, true);
    gfxBufferClear(0, 0, 0xFFFF);
    drawViRegBG();
}

/******************************************************************
Function: UpdateScreen
Purpose:  This function is called in response to a vsync of the
screen were the VI bit in MI_INTR_REG has already been
set
input:    none
output:   none
*******************************************************************/
uint32_t update_screen_count = 0;
void CALL UpdateScreen(void)
{
    WriteTrace(TraceGlide64, TraceDebug, "Origin: %08x, Old origin: %08x, width: %d", *gfx.VI_ORIGIN_REG, rdp.vi_org_reg, *gfx.VI_WIDTH_REG);

    uint32_t width = (*gfx.VI_WIDTH_REG) << 1;
    if (*gfx.VI_ORIGIN_REG > width)
    {
        update_screen_count++;
    }
    uint32_t limit = g_settings->hacks(CSettings::hack_Lego) ? 15 : 30;
    if (g_settings->fb_cpu_write_hack_enabled() && (update_screen_count > limit) && (rdp.last_bg == 0))
    {
        WriteTrace(TraceRDP, TraceDebug, "DirectCPUWrite hack!");
        update_screen_count = 0;
        no_dlist = true;
        ClearCache();
        UpdateScreen();
        return;
    }
    if (no_dlist)
    {
        if (*gfx.VI_ORIGIN_REG > width)
        {
            ChangeSize();
            WriteTrace(TraceRDP, TraceDebug, "ChangeSize done");
            DrawFrameBuffer();
            WriteTrace(TraceRDP, TraceDebug, "DrawFrameBuffer done");
            rdp.updatescreen = 1;
            newSwapBuffers();
        }
        return;
    }
    if (g_settings->swapmode() == CSettings::SwapMode_Old)
    {
        newSwapBuffers();
    }
}

static void DrawWholeFrameBufferToScreen()
{
    static uint32_t toScreenCI = 0;
    if (rdp.ci_width < 200)
    {
        return;
    }
    if (rdp.cimg == toScreenCI)
    {
        return;
    }
    toScreenCI = rdp.cimg;
    FB_TO_SCREEN_INFO fb_info;
    fb_info.addr = rdp.cimg;
    fb_info.size = rdp.ci_size;
    fb_info.width = rdp.ci_width;
    fb_info.height = rdp.ci_height;
    if (fb_info.height == 0)
    {
        return;
    }
    fb_info.ul_x = 0;
    fb_info.lr_x = rdp.ci_width - 1;
    fb_info.ul_y = 0;
    fb_info.lr_y = rdp.ci_height - 1;
    fb_info.opaque = 0;
    DrawFrameBufferToScreen(fb_info);
    if (!g_settings->fb_ref_enabled())
    {
        memset(gfx.RDRAM + rdp.cimg, 0, (rdp.ci_width*rdp.ci_height) << rdp.ci_size >> 1);
    }
}

static void GetGammaTable()
{
    voodoo.gamma_table_r = new uint32_t[voodoo.gamma_table_size];
    voodoo.gamma_table_g = new uint32_t[voodoo.gamma_table_size];
    voodoo.gamma_table_b = new uint32_t[voodoo.gamma_table_size];
    gfxGetGammaTableExt(voodoo.gamma_table_size, voodoo.gamma_table_r, voodoo.gamma_table_g, voodoo.gamma_table_b);
}

void write_png_file(const char* file_name, int width, int height, uint8_t *buffer)
{
    /* create file */
    FILE *fp = fopen(file_name, "wb");
    if (!fp)
    {
        WriteTrace(TracePNG, TraceError, "File %s could not be opened for writing", file_name);
        return;
    }

    /* initialize stuff */
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        WriteTrace(TracePNG, TraceError, "png_create_write_struct failed");
        fclose(fp);
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        WriteTrace(TracePNG, TraceError, "png_create_info_struct failed");
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        WriteTrace(TracePNG, TraceError, "Error during init_io");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }

    png_init_io(png_ptr, fp);

    /* write header */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        WriteTrace(TracePNG, TraceError, "Error during writing header");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }

    png_byte bit_depth = 8;
    png_byte color_type = PNG_COLOR_TYPE_RGB;
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        WriteTrace(TracePNG, TraceError, "Error during writing bytes");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }

    int pixel_size = 3;
    int p = 0;
    png_bytep * row_pointers = (png_bytep*)malloc(sizeof(png_bytep)* height);
    for (int y = 0; y < height; y++)
    {
        row_pointers[y] = (png_byte*)malloc(width*pixel_size);
        for (int x = 0; x < width; x++)
        {
            row_pointers[y][x*pixel_size + 0] = buffer[p++];
            row_pointers[y][x*pixel_size + 1] = buffer[p++];
            row_pointers[y][x*pixel_size + 2] = buffer[p++];
        }
    }
    png_write_image(png_ptr, row_pointers);

    // cleanup heap allocation
    for (int y = 0; y < height; y++)
    {
        free(row_pointers[y]);
    }
    free(row_pointers);

    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        WriteTrace(TracePNG, TraceError, "Error during end of write");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }
    png_write_end(png_ptr, NULL);
    fclose(fp);
}

uint32_t curframe = 0;
void newSwapBuffers()
{
    if (!rdp.updatescreen)
        return;

    rdp.updatescreen = 0;

    WriteTrace(TraceRDP, TraceDebug, "swapped");

    rdp.update |= UPDATE_SCISSOR | UPDATE_COMBINE | UPDATE_ZBUF_ENABLED | UPDATE_CULL_MODE;
    gfxClipWindow(0, 0, g_scr_res_x, g_scr_res_y);
    gfxDepthBufferFunction(GFX_CMP_ALWAYS);
    gfxDepthMask(false);
    gfxCullMode(GFX_CULL_DISABLE);

    if (g_capture_screen)
    {
        CPath path(g_capture_path);
        if (!path.DirectoryExists())
        {
            path.DirectoryCreate();
        }
        stdstr romName = rdp.RomName;
        romName.Replace(" ", "_");
        romName.Replace(":", ";");

        for (int i = 1;; i++)
        {
            stdstr_f filename("Glide64_%s_%s%d.png", romName.c_str(), i < 10 ? "0" : "", i);
            path.SetNameExtension(filename.c_str());
            if (!path.Exists())
            {
                break;
            }
        }

        const uint32_t offset_x = (uint32_t)rdp.offset_x;
        const uint32_t offset_y = (uint32_t)rdp.offset_y;
        const uint32_t image_width = g_scr_res_x - offset_x * 2;
        const uint32_t image_height = g_scr_res_y - offset_y * 2;

        gfxLfbInfo_t info;
        info.size = sizeof(gfxLfbInfo_t);
        if (gfxLfbLock(GFX_LFB_READ_ONLY, GFX_BUFFER_BACKBUFFER, GFX_LFBWRITEMODE_565, GFX_ORIGIN_UPPER_LEFT, false, &info))
        {
            AUTO_PTR<uint8_t> ssimg_buffer(new uint8_t[image_width * image_height * 3]);
            uint8_t * ssimg = ssimg_buffer.get();
            int sspos = 0;
            uint32_t offset_src = info.strideInBytes * offset_y;

            // Copy the screen
            if (info.writeMode == GFX_LFBWRITEMODE_8888)
            {
                uint32_t col;
                for (uint32_t y = 0; y < image_height; y++)
                {
                    uint32_t *ptr = (uint32_t*)((uint8_t*)info.lfbPtr + offset_src);
                    ptr += offset_x;
                    for (uint32_t x = 0; x < image_width; x++)
                    {
                        col = *(ptr++);
                        ssimg[sspos++] = (uint8_t)((col >> 16) & 0xFF);
                        ssimg[sspos++] = (uint8_t)((col >> 8) & 0xFF);
                        ssimg[sspos++] = (uint8_t)(col & 0xFF);
                    }
                    offset_src += info.strideInBytes;
                }
            }
            else
            {
                uint16_t col;
                for (uint32_t y = 0; y < image_height; y++)
                {
                    uint16_t *ptr = (uint16_t*)((uint8_t*)info.lfbPtr + offset_src);
                    ptr += offset_x;
                    for (uint32_t x = 0; x < image_width; x++)
                    {
                        col = *(ptr++);
                        ssimg[sspos++] = (uint8_t)((float)(col >> 11) / 31.0f * 255.0f);
                        ssimg[sspos++] = (uint8_t)((float)((col >> 5) & 0x3F) / 63.0f * 255.0f);
                        ssimg[sspos++] = (uint8_t)((float)(col & 0x1F) / 31.0f * 255.0f);
                    }
                    offset_src += info.strideInBytes;
                }
            }
            // Unlock the backbuffer
            gfxLfbUnlock(GFX_LFB_READ_ONLY, GFX_BUFFER_BACKBUFFER);
            write_png_file(path, image_width, image_height, ssimg);
            g_capture_screen = false;
        }
    }

    if (g_settings->fb_read_back_to_screen_enabled())
    {
        DrawWholeFrameBufferToScreen();
    }

    if (g_settings->fb_hwfbe_enabled() && !g_settings->hacks(CSettings::hack_RE2) && !evoodoo)
    {
        gfxAuxBufferExt(GFX_BUFFER_AUXBUFFER);
    }
    WriteTrace(TraceGlide64, TraceDebug, "BUFFER SWAPPED");
    gfxBufferSwap(g_settings->vsync());
    if (*gfx.VI_STATUS_REG & 0x08) //gamma correction is used
    {
        if (!voodoo.gamma_correction)
        {
            if (voodoo.gamma_table_size && !voodoo.gamma_table_r)
                GetGammaTable(); //save initial gamma tables
            gfxGammaCorrectionRGB(2.0f, 2.0f, 2.0f); //with gamma=2.0 gamma table is the same, as in N64
            voodoo.gamma_correction = 1;
        }
    }
    else
    {
        if (voodoo.gamma_correction)
        {
            if (voodoo.gamma_table_r)
                gfxLoadGammaTable(voodoo.gamma_table_size, voodoo.gamma_table_r, voodoo.gamma_table_g, voodoo.gamma_table_b);
            else
                gfxGammaCorrectionRGB(1.3f, 1.3f, 1.3f); //1.3f is default 3dfx gamma for everything but desktop
            voodoo.gamma_correction = 0;
        }
    }

    if (g_settings->wireframe() || g_settings->buff_clear() || (g_settings->hacks(CSettings::hack_PPL) && g_settings->ucode() == CSettings::ucode_S2DEX))
    {
        gfxDepthMask((g_settings->hacks(CSettings::hack_RE2) && g_settings->fb_depth_render_enabled()) ? false : true);
        gfxBufferClear(0, 0, 0xFFFF);
    }

    if (g_settings->fb_read_back_to_screen2_enabled())
    {
        DrawWholeFrameBufferToScreen();
    }
    frame_count++;
}

/******************************************************************
Function: ViStatusChanged
Purpose:  This function is called to notify the dll that the
ViStatus registers value has been changed.
input:    none
output:   none
*******************************************************************/
void CALL ViStatusChanged(void)
{
}

/******************************************************************
Function: ViWidthChanged
Purpose:  This function is called to notify the dll that the
ViWidth registers value has been changed.
input:    none
output:   none
*******************************************************************/
void CALL ViWidthChanged(void)
{
}

#ifdef ANDROID
/******************************************************************
Function: SurfaceCreated
Purpose:  this function is called when the surface is created.
input:    none
output:   none
*******************************************************************/
void CALL SurfaceCreated(void)
{
}

/******************************************************************
Function: SurfaceChanged
Purpose:  this function is called when the surface is has changed.
input:    none
output:   none
*******************************************************************/
void CALL SurfaceChanged(int width, int height)
{
    g_ScreenWidth = width;
    g_ScreenHeight = height;
}

void Android_JNI_SwapWindow()
{
    gfx.SwapBuffers();
}
#endif