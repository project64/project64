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
uint32_t g_NativeWidth, g_NativeHeight;
#endif

GFX_INFO gfx;

int to_fullscreen = FALSE;
int GfxInitDone = FALSE;
bool g_romopen = false;
GrContext_t gfx_context = 0;
int exception = FALSE;

int evoodoo = 0;
int ev_fullscreen = 0;

extern int g_viewport_offset;
extern int g_width, g_height;

#ifdef _WIN32
HINSTANCE hinstDLL = NULL;
#endif

#ifdef PERFORMANCE
int64 perf_cur;
int64 perf_next;
#endif

uint32_t   region = 0;

unsigned int BMASK = 0x7FFFFF;
// Reality display processor structure
RDP rdp;

CSettings * g_settings = NULL;

VOODOO voodoo = { 0, 0, 0, 0,
0, 0, 0, 0,
0, 0, 0, 0
};

GrTexInfo fontTex;
GrTexInfo cursorTex;
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

void _ChangeSize()
{
    rdp.scale_1024 = g_settings->scr_res_x() / 1024.0f;
    rdp.scale_768 = g_settings->scr_res_y() / 768.0f;

    //  float res_scl_x = (float)g_settings->res_x / 320.0f;
    float res_scl_y = (float)g_settings->res_y() / 240.0f;

    uint32_t scale_x = *gfx.VI_X_SCALE_REG & 0xFFF;
    if (!scale_x) return;
    uint32_t scale_y = *gfx.VI_Y_SCALE_REG & 0xFFF;
    if (!scale_y) return;

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

    rdp.scale_x = (float)g_settings->res_x() / rdp.vi_width;
    if (region > 0 && g_settings->pal230())
    {
        // odd... but pal games seem to want 230 as height...
        rdp.scale_y = res_scl_y * (230.0f / rdp.vi_height)  * aspect;
    }
    else
    {
        rdp.scale_y = (float)g_settings->res_y() / rdp.vi_height * aspect;
    }
    //  rdp.offset_x = g_settings->offset_x * res_scl_x;
    //  rdp.offset_y = g_settings->offset_y * res_scl_y;
    //rdp.offset_x = 0;
    //  rdp.offset_y = 0;
    rdp.offset_y = ((float)g_settings->res_y() - rdp.vi_height * rdp.scale_y) * 0.5f;
    if (((uint32_t)rdp.vi_width <= (*gfx.VI_WIDTH_REG) / 2) && (rdp.vi_width > rdp.vi_height))
        rdp.scale_y *= 0.5f;

    rdp.scissor_o.ul_x = 0;
    rdp.scissor_o.ul_y = 0;
    rdp.scissor_o.lr_x = (uint32_t)rdp.vi_width;
    rdp.scissor_o.lr_y = (uint32_t)rdp.vi_height;

    rdp.update |= UPDATE_VIEWPORT | UPDATE_SCISSOR;
}

void ChangeSize()
{
    g_settings->UpdateScreenSize(ev_fullscreen);
    _ChangeSize();
    rdp.offset_x = (g_settings->scr_res_x() - g_settings->res_x()) / 2.0f;
    float offset_y = (g_settings->scr_res_y() - g_settings->res_y()) / 2.0f;
    rdp.offset_y += offset_y;
    if (g_settings->aspectmode() == CSettings::Aspect_Original)
    {
        rdp.scale_x = rdp.scale_y = 1.0f;
        rdp.offset_x = (g_settings->scr_res_x() - rdp.vi_width) / 2.0f;
        rdp.offset_y = (g_settings->scr_res_y() - rdp.vi_height) / 2.0f;
    }
}

void ConfigWrapper()
{
    grConfigWrapperExt(g_settings->wrpVRAM() * 1024 * 1024, g_settings->wrpFBO(), g_settings->wrpAnisotropic());
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
GETTEXADDR GetTexAddr = GetTexAddrNonUMA;

// guLoadTextures - used to load the cursor and font textures
void guLoadTextures()
{
    int tbuf_size = 0;
    if (voodoo.max_tex_size <= 256)
    {
        grTextureBufferExt(GR_TMU1, voodoo.tex_min_addr[GR_TMU1], GR_LOD_LOG2_256, GR_LOD_LOG2_256,
            GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565, GR_MIPMAPLEVELMASK_BOTH);
        tbuf_size = 8 * grTexCalcMemRequired(GR_LOD_LOG2_256, GR_LOD_LOG2_256,
            GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565);
    }
    else if (g_settings->scr_res_x() <= 1024)
    {
        grTextureBufferExt(GR_TMU0, voodoo.tex_min_addr[GR_TMU0], GR_LOD_LOG2_1024, GR_LOD_LOG2_1024,
            GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565, GR_MIPMAPLEVELMASK_BOTH);
        tbuf_size = grTexCalcMemRequired(GR_LOD_LOG2_1024, GR_LOD_LOG2_1024,
            GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565);
        grRenderBuffer(GR_BUFFER_TEXTUREBUFFER_EXT);
        grBufferClear(0, 0, 0xFFFF);
        grRenderBuffer(GR_BUFFER_BACKBUFFER);
    }
    else
    {
        grTextureBufferExt(GR_TMU0, voodoo.tex_min_addr[GR_TMU0], GR_LOD_LOG2_2048, GR_LOD_LOG2_2048,
            GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565, GR_MIPMAPLEVELMASK_BOTH);
        tbuf_size = grTexCalcMemRequired(GR_LOD_LOG2_2048, GR_LOD_LOG2_2048,
            GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565);
        grRenderBuffer(GR_BUFFER_TEXTUREBUFFER_EXT);
        grBufferClear(0, 0, 0xFFFF);
        grRenderBuffer(GR_BUFFER_BACKBUFFER);
    }

    rdp.texbufs[0].tmu = GR_TMU0;
    rdp.texbufs[0].begin = voodoo.tex_min_addr[GR_TMU0];
    rdp.texbufs[0].end = rdp.texbufs[0].begin + tbuf_size;
    rdp.texbufs[0].count = 0;
    rdp.texbufs[0].clear_allowed = TRUE;
    offset_font = tbuf_size;
    if (voodoo.num_tmu > 1)
    {
        rdp.texbufs[1].tmu = GR_TMU1;
        rdp.texbufs[1].begin = voodoo.tex_UMA ? rdp.texbufs[0].end : voodoo.tex_min_addr[GR_TMU1];
        rdp.texbufs[1].end = rdp.texbufs[1].begin + tbuf_size;
        rdp.texbufs[1].count = 0;
        rdp.texbufs[1].clear_allowed = TRUE;
        if (voodoo.tex_UMA)
            offset_font += tbuf_size;
        else
            offset_texbuf1 = tbuf_size;
    }

#include "font.h"
    uint32_t *data = (uint32_t*)font;
    uint32_t cur;

    // ** Font texture **
    uint8_t *tex8 = (uint8_t*)malloc(256 * 64);

    fontTex.smallLodLog2 = fontTex.largeLodLog2 = GR_LOD_LOG2_256;
    fontTex.aspectRatioLog2 = GR_ASPECT_LOG2_4x1;
    fontTex.format = GR_TEXFMT_ALPHA_8;
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

    grTexDownloadMipMap(GR_TMU0,
        voodoo.tex_min_addr[GR_TMU0] + offset_font,
        GR_MIPMAPLEVELMASK_BOTH,
        &fontTex);

    offset_cursor = offset_font + grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &fontTex);

    free(fontTex.data);

    // ** Cursor texture **
#include "cursor.h"
    data = (uint32_t*)cursor;

    uint16_t *tex16 = (uint16_t*)malloc(32 * 32 * 2);

    cursorTex.smallLodLog2 = cursorTex.largeLodLog2 = GR_LOD_LOG2_32;
    cursorTex.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
    cursorTex.format = GR_TEXFMT_ARGB_1555;
    cursorTex.data = tex16;

    // Conversion: [16-bit 1555 (swapped) --> 16-bit 1555]
    for (i = 0; i < 0x200; i++)
    {
        cur = *(data++);
        *(tex16++) = (uint16_t)(((cur & 0x000000FF) << 8) | ((cur & 0x0000FF00) >> 8));
        *(tex16++) = (uint16_t)(((cur & 0x00FF0000) >> 8) | ((cur & 0xFF000000) >> 24));
    }

    grTexDownloadMipMap(GR_TMU0,
        voodoo.tex_min_addr[GR_TMU0] + offset_cursor,
        GR_MIPMAPLEVELMASK_BOTH,
        &cursorTex);

    // Round to higher 16
    offset_textures = ((offset_cursor + grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &cursorTex))
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
    grBufferSwap(0);
    grColorMask(FXTRUE, FXTRUE);
    grBufferClear(0, 0, 0xFFFF);
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

    rdp_reset();

    // Initialize Glide
    grGlideInit();

    // Is mirroring allowed?
    const char *extensions = grGetString(GR_EXTENSION);

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
    const char *hardware = grGetString(GR_HARDWARE);
    unsigned int SST_type = GR_SSTTYPE_VOODOO;
    if (strstr(hardware, "Rush")) {
        SST_type = GR_SSTTYPE_SST96;
    }
    else if (strstr(hardware, "Voodoo2")) {
        SST_type = GR_SSTTYPE_Voodoo2;
    }
    else if (strstr(hardware, "Voodoo Banshee")) {
        SST_type = GR_SSTTYPE_Banshee;
    }
    else if (strstr(hardware, "Voodoo3")) {
        SST_type = GR_SSTTYPE_Voodoo3;
    }
    else if (strstr(hardware, "Voodoo4")) {
        SST_type = GR_SSTTYPE_Voodoo4;
    }
    else if (strstr(hardware, "Voodoo5")) {
        SST_type = GR_SSTTYPE_Voodoo5;
    }
    // 2Mb Texture boundary
    voodoo.has_2mb_tex_boundary = (SST_type < GR_SSTTYPE_Banshee) && !evoodoo;
    // use UMA if available
    voodoo.tex_UMA = FALSE;
    if (strstr(extensions, " TEXUMA "))
    {
        // we get better texture cache hits with UMA on
        grEnable(GR_TEXTURE_UMA_EXT);
        voodoo.tex_UMA = TRUE;
        WriteTrace(TraceGlide64, TraceDebug, "Using TEXUMA extension");
    }

    g_settings->UpdateScreenSize(ev_fullscreen);
#ifndef ANDROID
    SetWindowDisplaySize((HWND)gfx.hWnd);
#endif
    gfx_context = grSstWinOpen(GR_COLORFORMAT_RGBA, GR_ORIGIN_UPPER_LEFT, 2, 1);
    if (!gfx_context)
    {
        g_Notify->DisplayError("Error setting display mode");
        grGlideShutdown();
        return FALSE;
    }

    GfxInitDone = TRUE;
    to_fullscreen = FALSE;

    // get the # of TMUs available
    grGet(GR_NUM_TMU, 4, (FxI32*)&voodoo.num_tmu);
    // get maximal texture size
    grGet(GR_MAX_TEXTURE_SIZE, 4, (FxI32*)&voodoo.max_tex_size);
    voodoo.sup_large_tex = (voodoo.max_tex_size > 256 && !g_settings->hacks(CSettings::hack_PPL));

    //num_tmu = 1;
    if (voodoo.tex_UMA)
    {
        GetTexAddr = GetTexAddrUMA;
        voodoo.tex_min_addr[0] = voodoo.tex_min_addr[1] = grTexMinAddress(GR_TMU0);
        voodoo.tex_max_addr[0] = voodoo.tex_max_addr[1] = grTexMaxAddress(GR_TMU0);
    }
    else
    {
        GetTexAddr = GetTexAddrNonUMA;
        voodoo.tex_min_addr[0] = grTexMinAddress(GR_TMU0);
        voodoo.tex_min_addr[1] = grTexMinAddress(GR_TMU1);
        voodoo.tex_max_addr[0] = grTexMaxAddress(GR_TMU0);
        voodoo.tex_max_addr[1] = grTexMaxAddress(GR_TMU1);
    }

    if (strstr(extensions, "TEXMIRROR") && !g_settings->hacks(CSettings::hack_Zelda)) //zelda's trees suffer from hardware mirroring
        voodoo.sup_mirroring = 1;
    else
        voodoo.sup_mirroring = 0;

    if (strstr(extensions, "TEXFMT"))  //VSA100 texture format extension
        voodoo.sup_32bit_tex = TRUE;
    else
        voodoo.sup_32bit_tex = FALSE;

    voodoo.gamma_correction = 0;
    if (strstr(extensions, "GETGAMMA"))
        grGet(GR_GAMMA_TABLE_ENTRIES, sizeof(voodoo.gamma_table_size), &voodoo.gamma_table_size);

    srand(g_settings->stipple_pattern());
    setPattern();

    InitCombine();

#ifdef SIMULATE_VOODOO1
    voodoo.num_tmu = 1;
    voodoo.sup_mirroring = 0;
#endif

#ifdef SIMULATE_BANSHEE
    voodoo.num_tmu = 1;
    voodoo.sup_mirroring = 1;
#endif

    grCoordinateSpace(GR_WINDOW_COORDS);
    grVertexLayout(GR_PARAM_XY, offsetof(VERTEX, x), GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_Q, offsetof(VERTEX, q), GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_Z, offsetof(VERTEX, z), GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_ST0, offsetof(VERTEX, coord[0]), GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_ST1, offsetof(VERTEX, coord[2]), GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_PARGB, offsetof(VERTEX, b), GR_PARAM_ENABLE);

    grCullMode(GR_CULL_NEGATIVE);

    if (g_settings->fog()) //"FOGCOORD" extension
    {
        if (strstr(extensions, "FOGCOORD"))
        {
            GrFog_t fog_t[64];
            guFogGenerateLinear(fog_t, 0.0f, 255.0f);//(float)rdp.fog_multiplier + (float)rdp.fog_offset);//256.0f);

            for (int i = 63; i > 0; i--)
            {
                if (fog_t[i] - fog_t[i - 1] > 63)
                {
                    fog_t[i - 1] = fog_t[i] - 63;
                }
            }
            fog_t[0] = 0;
            //      for (int f = 0; f < 64; f++)
            //      {
            //        WriteTrace(TraceRDP, TraceDebug, "fog[%d]=%d->%f", f, fog_t[f], guFogTableIndexToW(f));
            //      }
            grFogTable(fog_t);
            grVertexLayout(GR_PARAM_FOG_EXT, offsetof(VERTEX, f), GR_PARAM_ENABLE);
        }
        else //not supported
        {
            g_settings->SetFog(FALSE);
        }
    }

    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
    grDepthBufferFunction(GR_CMP_LESS);
    grDepthMask(FXTRUE);

    ChangeSize();

    guLoadTextures();
    ClearCache();

    grCullMode(GR_CULL_DISABLE);
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
    grDepthBufferFunction(GR_CMP_ALWAYS);
    grRenderBuffer(GR_BUFFER_BACKBUFFER);
    grColorMask(FXTRUE, FXTRUE);
    grDepthMask(FXTRUE);
    grBufferClear(0, 0, 0xFFFF);
    grBufferSwap(0);
    grBufferClear(0, 0, 0xFFFF);
    grDepthMask(FXFALSE);
    grTexFilterMode(0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
    grTexFilterMode(1, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
    grTexClampMode(0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
    grTexClampMode(1, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
    grClipWindow(0, 0, g_settings->scr_res_x(), g_settings->scr_res_y());
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
    if (g_ghq_use && strstr(extensions, "TEXMIRROR"))
    {
        voodoo.sup_mirroring = 1;
    }
    return TRUE;
}

void ReleaseGfx()
{
    WriteTrace(TraceGlide64, TraceDebug, "-");

    // Restore gamma settings
    if (voodoo.gamma_correction)
    {
        if (voodoo.gamma_table_r)
            grLoadGammaTable(voodoo.gamma_table_size, voodoo.gamma_table_r, voodoo.gamma_table_g, voodoo.gamma_table_b);
        else
            guGammaCorrectionRGB(1.3f, 1.3f, 1.3f); //1.3f is default 3dfx gamma for everything but desktop
        voodoo.gamma_correction = 0;
    }

    // Release graphics
    grSstWinClose(gfx_context);

    // Shutdown glide
    grGlideShutdown();

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

void CALL ReadScreen(void **dest, int *width, int *height)
{
    *width = g_settings->res_x();
    *height = g_settings->res_y();
    uint8_t * buff = (uint8_t*)malloc(g_settings->res_x() * g_settings->res_y() * 3);
    uint8_t * line = buff;
    *dest = (void*)buff;

    GrLfbInfo_t info;
    info.size = sizeof(GrLfbInfo_t);
    if (grLfbLock(GR_LFB_READ_ONLY,
        GR_BUFFER_FRONTBUFFER,
        GR_LFBWRITEMODE_565,
        GR_ORIGIN_UPPER_LEFT,
        FXFALSE,
        &info))
    {
        uint32_t offset_src = info.strideInBytes*(g_settings->scr_res_y() - 1);

        // Copy the screen
        uint8_t r, g, b;
        if (info.writeMode == GR_LFBWRITEMODE_8888)
        {
            uint32_t col;
            for (uint32_t y = 0; y < g_settings->res_y(); y++)
            {
                uint32_t *ptr = (uint32_t*)((uint8_t*)info.lfbPtr + offset_src);
                for (uint32_t x = 0; x < g_settings->res_x(); x++)
                {
                    col = *(ptr++);
                    r = (uint8_t)((col >> 16) & 0xFF);
                    g = (uint8_t)((col >> 8) & 0xFF);
                    b = (uint8_t)(col & 0xFF);
                    line[x * 3] = b;
                    line[x * 3 + 1] = g;
                    line[x * 3 + 2] = r;
                }
                line += g_settings->res_x() * 3;
                offset_src -= info.strideInBytes;
            }
        }
        else
        {
            uint16_t col;
            for (uint32_t y = 0; y < g_settings->res_y(); y++)
            {
                uint16_t *ptr = (uint16_t*)((uint8_t*)info.lfbPtr + offset_src);
                for (uint32_t x = 0; x < g_settings->res_x(); x++)
                {
                    col = *(ptr++);
                    r = (uint8_t)((float)(col >> 11) / 31.0f * 255.0f);
                    g = (uint8_t)((float)((col >> 5) & 0x3F) / 63.0f * 255.0f);
                    b = (uint8_t)((float)(col & 0x1F) / 31.0f * 255.0f);
                    line[x * 3] = b;
                    line[x * 3 + 1] = g;
                    line[x * 3 + 2] = r;
                }
                line += g_settings->res_x() * 3;
                offset_src -= info.strideInBytes;
            }
        }
        // Unlock the frontbuffer
        grLfbUnlock(GR_LFB_READ_ONLY, GR_BUFFER_FRONTBUFFER);
    }
    WriteTrace(TraceGlide64, TraceDebug, "Success");
}

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
void CALL DllTest(HWND /*hParent*/)
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
    sprintf(PluginInfo->Name, "Glide64 For PJ64 (Debug): %s", VER_FILE_VERSION_STR);
#else
    sprintf(PluginInfo->Name, "Glide64 For PJ64: %s", VER_FILE_VERSION_STR);
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
    voodoo.num_tmu = 2;

    // Assume scale of 1 for debug purposes
    rdp.scale_x = 1.0f;
    rdp.scale_y = 1.0f;

    char name[21] = "DEFAULT";
    g_settings->ReadGameSettings(name);
    ZLUT_init();
    ConfigWrapper();

    gfx = Gfx_Info;

    util_init();
    math_init();
    TexCacheInit();
    CRC_BuildTable();
    CountCombine();
    ZLUT_init();

    grConfigWrapperExt(g_settings->wrpVRAM() * 1024 * 1024, g_settings->wrpFBO(), g_settings->wrpAnisotropic());
    grGlideInit();
    const char *extensions = grGetString(GR_EXTENSION);
    grGlideShutdown();
    if (strstr(extensions, "EVOODOO"))
    {
        evoodoo = 1;
        voodoo.has_2mb_tex_boundary = 0;
    }
    else
    {
        evoodoo = 0;
        voodoo.has_2mb_tex_boundary = 1;
    }
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
    if (evoodoo)
    {
        ReleaseGfx();
    }
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
    rdp_reset();

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
    if (!GfxInitDone)
    {
        grGlideInit();
    }
    const char *extensions = grGetString(GR_EXTENSION);
    grGlideShutdown();

    if (strstr(extensions, "EVOODOO"))
        evoodoo = 1;
    else
        evoodoo = 0;

    if (evoodoo)
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

    grDepthMask(FXTRUE);
    grColorMask(FXTRUE, FXTRUE);
    grBufferClear(0, 0, 0xFFFF);
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
    char strGetGammaTableExt[] = "grGetGammaTableExt";
    void (FX_CALL *grGetGammaTableExt)(FxU32, FxU32*, FxU32*, FxU32*) =
        (void (FX_CALL *)(FxU32, FxU32*, FxU32*, FxU32*))grGetProcAddress(strGetGammaTableExt);
    if (grGetGammaTableExt)
    {
        voodoo.gamma_table_r = new FxU32[voodoo.gamma_table_size];
        voodoo.gamma_table_g = new FxU32[voodoo.gamma_table_size];
        voodoo.gamma_table_b = new FxU32[voodoo.gamma_table_size];
        grGetGammaTableExt(voodoo.gamma_table_size, voodoo.gamma_table_r, voodoo.gamma_table_g, voodoo.gamma_table_b);
    }
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
    grClipWindow(0, 0, g_settings->scr_res_x(), g_settings->scr_res_y());
    grDepthBufferFunction(GR_CMP_ALWAYS);
    grDepthMask(FXFALSE);
    grCullMode(GR_CULL_DISABLE);

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
        const uint32_t image_width = g_settings->scr_res_x() - offset_x * 2;
        const uint32_t image_height = g_settings->scr_res_y() - offset_y * 2;

        GrLfbInfo_t info;
        info.size = sizeof(GrLfbInfo_t);
        if (grLfbLock(GR_LFB_READ_ONLY, GR_BUFFER_BACKBUFFER, GR_LFBWRITEMODE_565, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info))
        {
            AUTO_PTR<uint8_t> ssimg_buffer(new uint8_t[image_width * image_height * 3]);
            uint8_t * ssimg = ssimg_buffer.get();
            int sspos = 0;
            uint32_t offset_src = info.strideInBytes * offset_y;

            // Copy the screen
            if (info.writeMode == GR_LFBWRITEMODE_8888)
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
            grLfbUnlock(GR_LFB_READ_ONLY, GR_BUFFER_BACKBUFFER);
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
        grAuxBufferExt(GR_BUFFER_AUXBUFFER);
    }
    WriteTrace(TraceGlide64, TraceDebug, "BUFFER SWAPPED");
    grBufferSwap(g_settings->vsync());
    if (*gfx.VI_STATUS_REG & 0x08) //gamma correction is used
    {
        if (!voodoo.gamma_correction)
        {
            if (voodoo.gamma_table_size && !voodoo.gamma_table_r)
                GetGammaTable(); //save initial gamma tables
            guGammaCorrectionRGB(2.0f, 2.0f, 2.0f); //with gamma=2.0 gamma table is the same, as in N64
            voodoo.gamma_correction = 1;
        }
    }
    else
    {
        if (voodoo.gamma_correction)
        {
            if (voodoo.gamma_table_r)
                grLoadGammaTable(voodoo.gamma_table_size, voodoo.gamma_table_r, voodoo.gamma_table_g, voodoo.gamma_table_b);
            else
                guGammaCorrectionRGB(1.3f, 1.3f, 1.3f); //1.3f is default 3dfx gamma for everything but desktop
            voodoo.gamma_correction = 0;
        }
    }

    if (g_settings->wireframe() || g_settings->buff_clear() || (g_settings->hacks(CSettings::hack_PPL) && g_settings->ucode() == CSettings::ucode_S2DEX))
    {
        grDepthMask((g_settings->hacks(CSettings::hack_RE2) && g_settings->fb_depth_render_enabled()) ? FXFALSE : FXTRUE);
        grBufferClear(0, 0, 0xFFFF);
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
    g_NativeWidth = width;
    g_NativeHeight = height;
}

void Android_JNI_SwapWindow()
{
    gfx.SwapBuffers();
}
#endif