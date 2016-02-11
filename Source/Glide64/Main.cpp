/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

#include <string.h>
#include <Common/StdString.h>
#include "Gfx_1.3.h"
#include "Version.h"
#include <Settings/Settings.h>
#include <Common/CriticalSection.h>
#include <Common/path.h>

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

#ifdef TEXTURE_FILTER // Hiroshi Morii <koolsmoky@users.sourceforge.net>
#include <stdarg.h>
int  ghq_dmptex_toggle_key = 0;
#endif

GFX_INFO gfx;

int to_fullscreen = FALSE;
int GfxInitDone = FALSE;
int romopen = FALSE;
GrContext_t gfx_context = 0;
int debugging = FALSE;
int exception = FALSE;

int evoodoo = 0;
int ev_fullscreen = 0;

#ifdef _WIN32
#define WINPROC_OVERRIDE
HINSTANCE hinstDLL = NULL;
#endif

#ifdef WINPROC_OVERRIDE
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC oldWndProc = NULL;
WNDPROC myWndProc = NULL;
#endif

#ifdef ALTTAB_FIX
HHOOK hhkLowLevelKybd = NULL;
LRESULT CALLBACK LowLevelKeyboardProc(int nCode,
    WPARAM wParam, LPARAM lParam);
#endif

#ifdef PERFORMANCE
int64 perf_cur;
int64 perf_next;
#endif

#ifdef FPS
wxDateTime fps_last;
wxDateTime fps_next;
float      fps = 0.0f;
uint32_t   fps_count = 0;

uint32_t   vi_count = 0;
float      vi = 0.0f;

uint32_t   region = 0;

float      ntsc_percent = 0.0f;
float      pal_percent = 0.0f;

#endif

// Resolutions, MUST be in the correct order (SST1VID.H)
uint32_t resolutions[0x18][2] = {
    { 320, 200 },
    { 320, 240 },
    { 400, 256 },
    { 512, 384 },
    { 640, 200 },
    { 640, 350 },
    { 640, 400 },
    { 640, 480 },
    { 800, 600 },
    { 960, 720 },
    { 856, 480 },
    { 512, 256 },
    { 1024, 768 },
    { 1280, 1024 },
    { 1600, 1200 },
    { 400, 300 },

    // 0x10
    { 1152, 864 },
    { 1280, 960 },
    { 1600, 1024 },
    { 1792, 1344 },
    { 1856, 1392 },
    { 1920, 1440 },
    { 2048, 1536 },
    { 2048, 2048 }
};

// ref rate
// 60=0x0, 70=0x1, 72=0x2, 75=0x3, 80=0x4, 90=0x5, 100=0x6, 85=0x7, 120=0x8, none=0xff

unsigned int BMASK = 0x7FFFFF;
// Reality display processor structure
RDP rdp;

CSettings * g_settings;

HOTKEY_INFO hotkey_info;

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

int    capture_screen = 0;
std::string capture_path;

void _ChangeSize()
{
    rdp.scale_1024 = g_settings->scr_res_x / 1024.0f;
    rdp.scale_768 = g_settings->scr_res_y / 768.0f;

    //  float res_scl_x = (float)g_settings->res_x / 320.0f;
    float res_scl_y = (float)g_settings->res_y / 240.0f;

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
    float aspect = (g_settings->adjust_aspect && (fscale_y > fscale_x) && (rdp.vi_width > rdp.vi_height)) ? fscale_x / fscale_y : 1.0f;

    WriteTrace(TraceResolution, TraceDebug, "hstart: %d, hend: %d, vstart: %d, vend: %d", hstart, hend, vstart, vend);
    WriteTrace(TraceResolution, TraceDebug, "size: %d x %d", (int)rdp.vi_width, (int)rdp.vi_height);

    rdp.scale_x = (float)g_settings->res_x / rdp.vi_width;
    if (region > 0 && g_settings->pal230)
    {
        // odd... but pal games seem to want 230 as height...
        rdp.scale_y = res_scl_y * (230.0f / rdp.vi_height)  * aspect;
    }
    else
    {
        rdp.scale_y = (float)g_settings->res_y / rdp.vi_height * aspect;
    }
    //  rdp.offset_x = g_settings->offset_x * res_scl_x;
    //  rdp.offset_y = g_settings->offset_y * res_scl_y;
    //rdp.offset_x = 0;
    //  rdp.offset_y = 0;
    rdp.offset_y = ((float)g_settings->res_y - rdp.vi_height * rdp.scale_y) * 0.5f;
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
    if (debugging)
    {
        _ChangeSize();
        return;
    }
    switch (g_settings->aspectmode)
    {
    case 0: //4:3
        if (g_settings->scr_res_x >= g_settings->scr_res_y * 4.0f / 3.0f) {
            g_settings->res_y = g_settings->scr_res_y;
            g_settings->res_x = (uint32_t)(g_settings->res_y * 4.0f / 3.0f);
        }
        else {
            g_settings->res_x = g_settings->scr_res_x;
            g_settings->res_y = (uint32_t)(g_settings->res_x / 4.0f * 3.0f);
        }
        break;
    case 1: //16:9
        if (g_settings->scr_res_x >= g_settings->scr_res_y * 16.0f / 9.0f) {
            g_settings->res_y = g_settings->scr_res_y;
            g_settings->res_x = (uint32_t)(g_settings->res_y * 16.0f / 9.0f);
        }
        else {
            g_settings->res_x = g_settings->scr_res_x;
            g_settings->res_y = (uint32_t)(g_settings->res_x / 16.0f * 9.0f);
        }
        break;
    default: //stretch or original
        g_settings->res_x = g_settings->scr_res_x;
        g_settings->res_y = g_settings->scr_res_y;
    }
    _ChangeSize();
    rdp.offset_x = (g_settings->scr_res_x - g_settings->res_x) / 2.0f;
    float offset_y = (g_settings->scr_res_y - g_settings->res_y) / 2.0f;
    g_settings->res_x += (uint32_t)rdp.offset_x;
    g_settings->res_y += (uint32_t)offset_y;
    rdp.offset_y += offset_y;
    if (g_settings->aspectmode == 3) // original
    {
        rdp.scale_x = rdp.scale_y = 1.0f;
        rdp.offset_x = (g_settings->scr_res_x - rdp.vi_width) / 2.0f;
        rdp.offset_y = (g_settings->scr_res_y - rdp.vi_height) / 2.0f;
    }
    //	g_settings->res_x = g_settings->scr_res_x;
    //	g_settings->res_y = g_settings->scr_res_y;
}

void ConfigWrapper()
{
    grConfigWrapperExt(g_settings->wrpResolution, g_settings->wrpVRAM * 1024 * 1024, g_settings->wrpFBO, g_settings->wrpAnisotropic);
}

void UseUnregisteredSetting(int /*SettingID*/)
{
#ifdef _WIN32
    DebugBreak();
#endif
}

void ReadSettings()
{
    g_settings->card_id = GetSetting(Set_CardId);
    g_settings->res_data = (uint32_t)GetSetting(Set_Resolution);
    if (g_settings->res_data >= 24) g_settings->res_data = 12;
    g_settings->scr_res_x = g_settings->res_x = resolutions[g_settings->res_data][0];
    g_settings->scr_res_y = g_settings->res_y = resolutions[g_settings->res_data][1];
    g_settings->vsync = GetSetting(Set_vsync);
    g_settings->ssformat = (uint8_t)GetSetting(Set_ssformat);
    g_settings->show_fps = (uint8_t)GetSetting(Set_ShowFps);
    g_settings->clock = GetSetting(Set_clock);
    g_settings->clock_24_hr = GetSetting(Set_clock_24_hr);
    g_settings->advanced_options = Set_basic_mode ? !GetSystemSetting(Set_basic_mode) : 0;
    g_settings->texenh_options = GetSetting(Set_texenh_options);
    g_settings->use_hotkeys = GetSetting(Set_hotkeys);

    g_settings->wrpResolution = GetSetting(Set_wrpResolution);
    g_settings->wrpVRAM = GetSetting(Set_wrpVRAM);
    g_settings->wrpFBO = GetSetting(Set_wrpFBO);
    g_settings->wrpAnisotropic = GetSetting(Set_wrpAnisotropic);

#ifndef _ENDUSER_RELEASE_
    g_settings->autodetect_ucode = GetSetting(Set_autodetect_ucode);
    g_settings->ucode = GetSetting(Set_ucode);
    g_settings->wireframe = GetSetting(Set_wireframe);
    g_settings->wfmode = GetSetting(Set_wfmode);
    g_settings->logging = GetSetting(Set_logging);
    g_settings->log_clear = GetSetting(Set_log_clear);
    g_settings->run_in_window = GetSetting(Set_run_in_window);
    g_settings->elogging = GetSetting(Set_elogging);
    g_settings->filter_cache = GetSetting(Set_filter_cache);
    g_settings->unk_as_red = GetSetting(Set_unk_as_red);
    g_settings->log_unk = GetSetting(Set_log_unk);
    g_settings->unk_clear = GetSetting(Set_unk_clear);
#else
    g_settings->autodetect_ucode = TRUE;
    g_settings->ucode = 2;
    g_settings->wireframe = FALSE;
    g_settings->wfmode = 0;
    g_settings->logging = FALSE;
    g_settings->log_clear = FALSE;
    g_settings->run_in_window = FALSE;
    g_settings->elogging = FALSE;
    g_settings->filter_cache = FALSE;
    g_settings->unk_as_red = FALSE;
    g_settings->log_unk = FALSE;
    g_settings->unk_clear = FALSE;
#endif

#ifdef TEXTURE_FILTER
    char texture_dir[260];
    memset(texture_dir, 0, sizeof(texture_dir));
    GetSystemSettingSz(Set_texture_dir, texture_dir, sizeof(texture_dir));
    g_settings->texture_dir = texture_dir;
    g_settings->ghq_fltr = (uint8_t)GetSetting(Set_ghq_fltr);
    g_settings->ghq_cmpr = (uint8_t)GetSetting(Set_ghq_cmpr);
    g_settings->ghq_enht = (uint8_t)GetSetting(Set_ghq_enht);
    g_settings->ghq_hirs = (uint8_t)GetSetting(Set_ghq_hirs);
    g_settings->ghq_enht_cmpr = GetSetting(Set_ghq_enht_cmpr);
    g_settings->ghq_enht_tile = GetSetting(Set_ghq_enht_tile);
    g_settings->ghq_enht_f16bpp = GetSetting(Set_ghq_enht_f16bpp);
    g_settings->ghq_enht_gz = GetSetting(Set_ghq_enht_gz);
    g_settings->ghq_enht_nobg = GetSetting(Set_ghq_enht_nobg);
    g_settings->ghq_hirs_cmpr = GetSetting(Set_ghq_hirs_cmpr);
    g_settings->ghq_hirs_tile = GetSetting(Set_ghq_hirs_tile);
    g_settings->ghq_hirs_f16bpp = GetSetting(Set_ghq_hirs_f16bpp);
    g_settings->ghq_hirs_gz = GetSetting(Set_ghq_hirs_gz);
    g_settings->ghq_hirs_altcrc = GetSetting(Set_ghq_hirs_altcrc);
    g_settings->ghq_cache_save = GetSetting(Set_ghq_cache_save);
    g_settings->ghq_cache_size = GetSetting(Set_ghq_cache_size);
    g_settings->ghq_hirs_let_texartists_fly = GetSetting(Set_ghq_hirs_let_texartists_fly);
    g_settings->ghq_hirs_dump = GetSetting(Set_ghq_hirs_dump);
#endif

    ConfigWrapper();
}

void ReadSpecialSettings(const char * name)
{
    //  char buf [256];
    //  sprintf(buf, "ReadSpecialSettings. Name: %s", name);
    //  LOG(buf);
    g_settings->hacks = 0;

    //detect games which require special hacks
    if (strstr(name, (const char *)"ZELDA"))
        g_settings->hacks |= (hack_Zelda | hack_OoT);
    else if (strstr(name, (const char *)"MASK"))
        g_settings->hacks |= hack_Zelda;
    else if (strstr(name, (const char *)"ROADSTERS TROPHY"))
        g_settings->hacks |= hack_Zelda;
    else if (strstr(name, (const char *)"Diddy Kong Racing"))
        g_settings->hacks |= hack_Diddy;
    else if (strstr(name, (const char *)"Tonic Trouble"))
        g_settings->hacks |= hack_Tonic;
    else if (strstr(name, (const char *)"All") && strstr(name, (const char *)"Star") && strstr(name, (const char *)"Baseball"))
        g_settings->hacks |= hack_ASB;
    else if (strstr(name, (const char *)"Beetle") || strstr(name, (const char *)"BEETLE") || strstr(name, (const char *)"HSV"))
        g_settings->hacks |= hack_BAR;
    else if (strstr(name, (const char *)"I S S 64") || strstr(name, (const char *)"J WORLD SOCCER3") || strstr(name, (const char *)"PERFECT STRIKER") || strstr(name, (const char *)"RONALDINHO SOCCER"))
        g_settings->hacks |= hack_ISS64;
    else if (strstr(name, (const char *)"MARIOKART64"))
        g_settings->hacks |= hack_MK64;
    else if (strstr(name, (const char *)"NITRO64"))
        g_settings->hacks |= hack_WCWnitro;
    else if (strstr(name, (const char *)"CHOPPER_ATTACK") || strstr(name, (const char *)"WILD CHOPPERS"))
        g_settings->hacks |= hack_Chopper;
    else if (strstr(name, (const char *)"Resident Evil II") || strstr(name, (const char *)"BioHazard II"))
        g_settings->hacks |= hack_RE2;
    else if (strstr(name, (const char *)"YOSHI STORY"))
        g_settings->hacks |= hack_Yoshi;
    else if (strstr(name, (const char *)"F-Zero X") || strstr(name, (const char *)"F-ZERO X"))
        g_settings->hacks |= hack_Fzero;
    else if (strstr(name, (const char *)"PAPER MARIO") || strstr(name, (const char *)"MARIO STORY"))
        g_settings->hacks |= hack_PMario;
    else if (strstr(name, (const char *)"TOP GEAR RALLY 2"))
        g_settings->hacks |= hack_TGR2;
    else if (strstr(name, (const char *)"TOP GEAR RALLY"))
        g_settings->hacks |= hack_TGR;
    else if (strstr(name, (const char *)"Top Gear Hyper Bike"))
        g_settings->hacks |= hack_Hyperbike;
    else if (strstr(name, (const char *)"Killer Instinct Gold") || strstr(name, (const char *)"KILLER INSTINCT GOLD"))
        g_settings->hacks |= hack_KI;
    else if (strstr(name, (const char *)"Knockout Kings 2000"))
        g_settings->hacks |= hack_Knockout;
    else if (strstr(name, (const char *)"LEGORacers"))
        g_settings->hacks |= hack_Lego;
    else if (strstr(name, (const char *)"OgreBattle64"))
        g_settings->hacks |= hack_Ogre64;
    else if (strstr(name, (const char *)"Pilot Wings64"))
        g_settings->hacks |= hack_Pilotwings;
    else if (strstr(name, (const char *)"Supercross"))
        g_settings->hacks |= hack_Supercross;
    else if (strstr(name, (const char *)"STARCRAFT 64"))
        g_settings->hacks |= hack_Starcraft;
    else if (strstr(name, (const char *)"BANJO KAZOOIE 2") || strstr(name, (const char *)"BANJO TOOIE"))
        g_settings->hacks |= hack_Banjo2;
    else if (strstr(name, (const char *)"FIFA: RTWC 98") || strstr(name, (const char *)"RoadToWorldCup98"))
        g_settings->hacks |= hack_Fifa98;
    else if (strstr(name, (const char *)"Mega Man 64") || strstr(name, (const char *)"RockMan Dash"))
        g_settings->hacks |= hack_Megaman;
    else if (strstr(name, (const char *)"MISCHIEF MAKERS") || strstr(name, (const char *)"TROUBLE MAKERS"))
        g_settings->hacks |= hack_Makers;
    else if (strstr(name, (const char *)"GOLDENEYE"))
        g_settings->hacks |= hack_GoldenEye;
    else if (strstr(name, (const char *)"PUZZLE LEAGUE"))
        g_settings->hacks |= hack_PPL;

    g_settings->alt_tex_size = GetSetting(Set_alt_tex_size);
    g_settings->use_sts1_only = GetSetting(Set_use_sts1_only);
    g_settings->force_calc_sphere = GetSetting(Set_force_calc_sphere);
    g_settings->correct_viewport = GetSetting(Set_correct_viewport);
    g_settings->increase_texrect_edge = GetSetting(Set_increase_texrect_edge);
    g_settings->decrease_fillrect_edge = GetSetting(Set_decrease_fillrect_edge);
    g_settings->texture_correction = GetSetting(Set_texture_correction) == 0 ? 0 : 1;
    g_settings->pal230 = GetSetting(Set_pal230) == 1 ? 1 : 0;
    g_settings->stipple_mode = GetSetting(Set_stipple_mode);
    int stipple_pattern = GetSetting(Set_stipple_pattern);
    g_settings->stipple_pattern = stipple_pattern > 0 ? (uint32_t)stipple_pattern : 0x3E0F83E0;
    g_settings->force_microcheck = GetSetting(Set_force_microcheck);
    g_settings->force_quad3d = GetSetting(Set_force_quad3d);
    g_settings->clip_zmin = GetSetting(Set_clip_zmin);
    g_settings->clip_zmax = GetSetting(Set_clip_zmax);
    g_settings->fast_crc = GetSetting(Set_fast_crc);
    g_settings->adjust_aspect = GetSetting(Set_adjust_aspect);
    g_settings->zmode_compare_less = GetSetting(Set_zmode_compare_less);
    g_settings->old_style_adither = GetSetting(Set_old_style_adither);
    g_settings->n64_z_scale = GetSetting(Set_n64_z_scale);
    if (g_settings->n64_z_scale)
        ZLUT_init();

    //frame buffer
    int optimize_texrect = GetSetting(Set_optimize_texrect);
    int ignore_aux_copy = GetSetting(Set_ignore_aux_copy);
    int hires_buf_clear = GetSetting(Set_hires_buf_clear);
    int read_alpha = GetSetting(Set_fb_read_alpha);
    int useless_is_useless = GetSetting(Set_useless_is_useless);
    int fb_crc_mode = GetSetting(Set_fb_crc_mode);

    if (optimize_texrect > 0) g_settings->frame_buffer |= fb_optimize_texrect;
    else if (optimize_texrect == 0) g_settings->frame_buffer &= ~fb_optimize_texrect;
    if (ignore_aux_copy > 0) g_settings->frame_buffer |= fb_ignore_aux_copy;
    else if (ignore_aux_copy == 0) g_settings->frame_buffer &= ~fb_ignore_aux_copy;
    if (hires_buf_clear > 0) g_settings->frame_buffer |= fb_hwfbe_buf_clear;
    else if (hires_buf_clear == 0) g_settings->frame_buffer &= ~fb_hwfbe_buf_clear;
    if (read_alpha > 0) g_settings->frame_buffer |= fb_read_alpha;
    else if (read_alpha == 0) g_settings->frame_buffer &= ~fb_read_alpha;
    if (useless_is_useless > 0) g_settings->frame_buffer |= fb_useless_is_useless;
    else g_settings->frame_buffer &= ~fb_useless_is_useless;
    if (fb_crc_mode >= 0) g_settings->fb_crc_mode = (CSettings::FBCRCMODE)fb_crc_mode;

    //  if (g_settings->custom_ini)
    {
        g_settings->filtering = GetSetting(Set_filtering);
        g_settings->fog = GetSetting(Set_fog);
        g_settings->buff_clear = GetSetting(Set_buff_clear);
        g_settings->swapmode = GetSetting(Set_swapmode);
        g_settings->aspectmode = GetSetting(Set_aspect);
        g_settings->lodmode = GetSetting(Set_lodmode);
        int resolution = GetSetting(Set_Resolution);
        if (resolution >= 0)
        {
            g_settings->res_data = (uint32_t)resolution;
            if (g_settings->res_data >= 0x18) g_settings->res_data = 12;
            g_settings->scr_res_x = g_settings->res_x = resolutions[g_settings->res_data][0];
            g_settings->scr_res_y = g_settings->res_y = resolutions[g_settings->res_data][1];
        }

        //frame buffer
        int smart_read = GetSetting(Set_fb_smart);
        int hires = GetSetting(Set_fb_hires);
        int read_always = GetSetting(Set_fb_read_always);
        int read_back_to_screen = GetSetting(Set_read_back_to_screen);
        int cpu_write_hack = GetSetting(Set_detect_cpu_write);
        int get_fbinfo = GetSetting(Set_fb_get_info);
        int depth_render = GetSetting(Set_fb_render);

        if (smart_read > 0) g_settings->frame_buffer |= fb_emulation;
        else if (smart_read == 0) g_settings->frame_buffer &= ~fb_emulation;
        if (hires > 0) g_settings->frame_buffer |= fb_hwfbe;
        else if (hires == 0) g_settings->frame_buffer &= ~fb_hwfbe;
        if (read_always > 0) g_settings->frame_buffer |= fb_ref;
        else if (read_always == 0) g_settings->frame_buffer &= ~fb_ref;
        if (read_back_to_screen == 1) g_settings->frame_buffer |= fb_read_back_to_screen;
        else if (read_back_to_screen == 2) g_settings->frame_buffer |= fb_read_back_to_screen2;
        else if (read_back_to_screen == 0) g_settings->frame_buffer &= ~(fb_read_back_to_screen | fb_read_back_to_screen2);
        if (cpu_write_hack > 0) g_settings->frame_buffer |= fb_cpu_write_hack;
        else if (cpu_write_hack == 0) g_settings->frame_buffer &= ~fb_cpu_write_hack;
        if (get_fbinfo > 0) g_settings->frame_buffer |= fb_get_info;
        else if (get_fbinfo == 0) g_settings->frame_buffer &= ~fb_get_info;
        if (depth_render > 0) g_settings->frame_buffer |= fb_depth_render;
        else if (depth_render == 0) g_settings->frame_buffer &= ~fb_depth_render;
        g_settings->frame_buffer |= fb_motionblur;
    }
    g_settings->flame_corona = (g_settings->hacks & hack_Zelda) && !fb_depth_render_enabled;
}

void WriteSettings(bool saveEmulationSettings)
{
    SetSetting(Set_CardId, g_settings->card_id);
    SetSetting(Set_Resolution, (int)g_settings->res_data);
    SetSetting(Set_ssformat, g_settings->ssformat);
    SetSetting(Set_vsync, g_settings->vsync);
    SetSetting(Set_ShowFps, g_settings->show_fps);
    SetSetting(Set_clock, g_settings->clock);
    SetSetting(Set_clock_24_hr, g_settings->clock_24_hr);
    //SetSetting(Set_advanced_options,g_settings->advanced_options);
    SetSetting(Set_texenh_options, g_settings->texenh_options);

    SetSetting(Set_wrpResolution, g_settings->wrpResolution);
    SetSetting(Set_wrpVRAM, g_settings->wrpVRAM);
    SetSetting(Set_wrpFBO, g_settings->wrpFBO);
    SetSetting(Set_wrpAnisotropic, g_settings->wrpAnisotropic);

#ifndef _ENDUSER_RELEASE_
    SetSetting(Set_autodetect_ucode, g_settings->autodetect_ucode);
    SetSetting(Set_ucode, (int)g_settings->ucode);
    SetSetting(Set_wireframe, g_settings->wireframe);
    SetSetting(Set_wfmode, g_settings->wfmode);
    SetSetting(Set_logging, g_settings->logging);
    SetSetting(Set_log_clear, g_settings->log_clear);
    SetSetting(Set_run_in_window,g_settings->run_in_window);
    SetSetting(Set_elogging,g_settings->elogging);
    SetSetting(Set_filter_cache,g_settings->filter_cache);
    SetSetting(Set_unk_as_red,g_settings->unk_as_red);
    SetSetting(Set_log_unk,g_settings->log_unk);
    SetSetting(Set_unk_clear, g_settings->unk_clear);
#endif //_ENDUSER_RELEASE_

#ifdef TEXTURE_FILTER
    SetSetting(Set_ghq_fltr, g_settings->ghq_fltr);
    SetSetting(Set_ghq_cmpr, g_settings->ghq_cmpr);
    SetSetting(Set_ghq_enht, g_settings->ghq_enht);
    SetSetting(Set_ghq_hirs, g_settings->ghq_hirs);
    SetSetting(Set_ghq_enht_cmpr, g_settings->ghq_enht_cmpr);
    SetSetting(Set_ghq_enht_tile, g_settings->ghq_enht_tile);
    SetSetting(Set_ghq_enht_f16bpp, g_settings->ghq_enht_f16bpp);
    SetSetting(Set_ghq_enht_gz, g_settings->ghq_enht_gz);
    SetSetting(Set_ghq_enht_nobg, g_settings->ghq_enht_nobg);
    SetSetting(Set_ghq_hirs_cmpr, g_settings->ghq_hirs_cmpr);
    SetSetting(Set_ghq_hirs_tile, g_settings->ghq_hirs_tile);
    SetSetting(Set_ghq_hirs_f16bpp, g_settings->ghq_hirs_f16bpp);
    SetSetting(Set_ghq_hirs_gz, g_settings->ghq_hirs_gz);
    SetSetting(Set_ghq_hirs_altcrc, g_settings->ghq_hirs_altcrc);
    SetSetting(Set_ghq_cache_save, g_settings->ghq_cache_save);
    SetSetting(Set_ghq_cache_size, g_settings->ghq_cache_size);
    SetSetting(Set_ghq_hirs_let_texartists_fly, g_settings->ghq_hirs_let_texartists_fly);
    SetSetting(Set_ghq_hirs_dump, g_settings->ghq_hirs_dump);
#endif

    if (saveEmulationSettings)
    {
        SetSetting(Set_filtering, g_settings->filtering);
        SetSetting(Set_fog, g_settings->fog);
        SetSetting(Set_buff_clear, g_settings->buff_clear);
        SetSetting(Set_swapmode, g_settings->swapmode);
        SetSetting(Set_lodmode, g_settings->lodmode);
        SetSetting(Set_aspect, g_settings->aspectmode);

        SetSetting(Set_fb_read_always, g_settings->frame_buffer&fb_ref ? 1 : 0);
        SetSetting(Set_fb_smart, g_settings->frame_buffer & fb_emulation ? 1 : 0);
        SetSetting(Set_fb_hires, g_settings->frame_buffer & fb_hwfbe ? 1 : 0);
        SetSetting(Set_fb_get_info, g_settings->frame_buffer & fb_get_info ? 1 : 0);
        SetSetting(Set_fb_render, g_settings->frame_buffer & fb_depth_render ? 1 : 0);
        SetSetting(Set_detect_cpu_write, g_settings->frame_buffer & fb_cpu_write_hack ? 1 : 0);
        if (g_settings->frame_buffer & fb_read_back_to_screen)
            SetSetting(Set_read_back_to_screen, 1);
        else if (g_settings->frame_buffer & fb_read_back_to_screen2)
            SetSetting(Set_read_back_to_screen, 2);
        else
            SetSetting(Set_read_back_to_screen, 0);
    }

    FlushSettings();
}

GRSTIPPLE grStippleModeExt = NULL;
GRSTIPPLE grStipplePatternExt = NULL;
FxBool(FX_CALL *grKeyPressed)(FxU32) = NULL;

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
    else if (g_settings->scr_res_x <= 1024)
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

#ifdef TEXTURE_FILTER
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
#endif

int InitGfx()
{
    if (GfxInitDone)
    {
        ReleaseGfx();
    }

    WriteTrace(TraceGlide64, TraceDebug, "-");

    debugging = FALSE;
    rdp_reset();

    // Initialize Glide
    grGlideInit();

    // Select the Glide device
    grSstSelect(g_settings->card_id);

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
    //*
    if (strstr(extensions, " TEXUMA ")) {
        // we get better texture cache hits with UMA on
        grEnable(GR_TEXTURE_UMA_EXT);
        voodoo.tex_UMA = TRUE;
        WriteTrace(TraceGlide64, TraceDebug, "Using TEXUMA extension");
    }
    //*/

    uint32_t res_data = g_settings->res_data;
    if (ev_fullscreen)
    {
        uint32_t _width, _height = 0;
        g_settings->res_data = grWrapperFullScreenResolutionExt((FxU32*)&_width, (FxU32*)&_height);
        g_settings->scr_res_x = g_settings->res_x = _width;
        g_settings->scr_res_y = g_settings->res_y = _height;
        res_data = g_settings->res_data;
    }
    else if (evoodoo)
    {
        g_settings->res_data = g_settings->res_data_org;
        g_settings->scr_res_x = g_settings->res_x = resolutions[g_settings->res_data][0];
        g_settings->scr_res_y = g_settings->res_y = resolutions[g_settings->res_data][1];
        res_data = g_settings->res_data | 0x80000000;
    }

    gfx_context = 0;

    // Select the window

    /*if (fb_hwfbe_enabled)
    {
    gfx_context = grSstWinOpenExt (uintptr_t(gfx.hWnd),
    res_data,
    GR_REFRESH_60Hz,
    GR_COLORFORMAT_RGBA,
    GR_ORIGIN_UPPER_LEFT,
    fb_emulation_enabled?GR_PIXFMT_RGB_565:GR_PIXFMT_ARGB_8888, //32b color is not compatible with fb emulation
    2,    // Double-buffering
    1);   // 1 auxillary buffer
    }*/
    if (!gfx_context)
        gfx_context = grSstWinOpen(gfx.hWnd,
        res_data,
        GR_REFRESH_60Hz,
        GR_COLORFORMAT_RGBA,
        GR_ORIGIN_UPPER_LEFT,
        2,    // Double-buffering
        1);   // 1 auxillary buffer

    if (!gfx_context)
    {
#ifdef _WIN32
        MessageBox(gfx.hWnd, "Error setting display mode", "Error", MB_OK | MB_ICONEXCLAMATION);
#endif
        //    grSstWinClose (gfx_context);
        grGlideShutdown();
        return FALSE;
    }

    GfxInitDone = TRUE;
    to_fullscreen = FALSE;

#ifdef __WINDOWS__
    if (ev_fullscreen)
    {
        if (gfx.hStatusBar)
            ShowWindow(gfx.hStatusBar, SW_HIDE);
        ShowCursor(FALSE);
    }
#endif

    // get the # of TMUs available
    grGet(GR_NUM_TMU, 4, (FxI32*)&voodoo.num_tmu);
    // get maximal texture size
    grGet(GR_MAX_TEXTURE_SIZE, 4, (FxI32*)&voodoo.max_tex_size);
    voodoo.sup_large_tex = (voodoo.max_tex_size > 256 && !(g_settings->hacks & hack_PPL));

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

    if (strstr(extensions, "TEXMIRROR") && !(g_settings->hacks&hack_Zelda)) //zelda's trees suffer from hardware mirroring
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

    grStippleModeExt = (GRSTIPPLE)grStippleMode;
    grStipplePatternExt = (GRSTIPPLE)grStipplePattern;

    if (grStipplePatternExt)
        grStipplePatternExt(g_settings->stipple_pattern);

    char strKeyPressedExt[] = "grKeyPressedExt";
    grKeyPressed = (FxBool(FX_CALL *)(FxU32))grGetProcAddress(strKeyPressedExt);

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

    if (g_settings->fog) //"FOGCOORD" extension
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
            g_settings->fog = FALSE;
    }

    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
    grDepthBufferFunction(GR_CMP_LESS);
    grDepthMask(FXTRUE);

    g_settings->res_x = g_settings->scr_res_x;
    g_settings->res_y = g_settings->scr_res_y;
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
    grClipWindow(0, 0, g_settings->scr_res_x, g_settings->scr_res_y);
    rdp.update |= UPDATE_SCISSOR | UPDATE_COMBINE | UPDATE_ZBUF_ENABLED | UPDATE_CULL_MODE;

#ifdef TEXTURE_FILTER // Hiroshi Morii <koolsmoky@users.sourceforge.net>
    if (!g_settings->ghq_use)
    {
        g_settings->ghq_use = g_settings->ghq_fltr || g_settings->ghq_enht /*|| g_settings->ghq_cmpr*/ || g_settings->ghq_hirs;
        if (g_settings->ghq_use)
        {
            /* Plugin path */
            int options = texfltr[g_settings->ghq_fltr] | texenht[g_settings->ghq_enht] | texcmpr[g_settings->ghq_cmpr] | texhirs[g_settings->ghq_hirs];
            if (g_settings->ghq_enht_cmpr)
                options |= COMPRESS_TEX;
            if (g_settings->ghq_hirs_cmpr)
                options |= COMPRESS_HIRESTEX;
            //      if (g_settings->ghq_enht_tile)
            //        options |= TILE_TEX;
            if (g_settings->ghq_hirs_tile)
                options |= TILE_HIRESTEX;
            if (g_settings->ghq_enht_f16bpp)
                options |= FORCE16BPP_TEX;
            if (g_settings->ghq_hirs_f16bpp)
                options |= FORCE16BPP_HIRESTEX;
            if (g_settings->ghq_enht_gz)
                options |= GZ_TEXCACHE;
            if (g_settings->ghq_hirs_gz)
                options |= GZ_HIRESTEXCACHE;
            if (g_settings->ghq_cache_save)
                options |= (DUMP_TEXCACHE | DUMP_HIRESTEXCACHE);
            if (g_settings->ghq_hirs_let_texartists_fly)
                options |= LET_TEXARTISTS_FLY;
            if (g_settings->ghq_hirs_dump)
                options |= DUMP_TEX;

            ghq_dmptex_toggle_key = 0;

            g_settings->ghq_use = (int)ext_ghq_init(voodoo.max_tex_size, // max texture width supported by hardware
                voodoo.max_tex_size, // max texture height supported by hardware
                voodoo.sup_32bit_tex ? 32 : 16, // max texture bpp supported by hardware
                options,
                g_settings->ghq_cache_size * 1024 * 1024, // cache texture to system memory
                g_settings->texture_dir.c_str(),
                rdp.RomName, // name of ROM. must be no longer than 256 characters
                DisplayLoadProgress);
        }
    }
    if (g_settings->ghq_use && strstr(extensions, "TEXMIRROR"))
        voodoo.sup_mirroring = 1;
#endif

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

//
// DllMain - called when the DLL is loaded, use this to get the DLL's instance
//
class wxDLLApp : public wxApp
{
public:
    virtual bool OnInit();
    virtual void CleanUp();
};

IMPLEMENT_APP_NO_MAIN(wxDLLApp)

bool wxDLLApp::OnInit()
{
    wxImage::AddHandler(new wxPNGHandler);
    wxImage::AddHandler(new wxJPEGHandler);
    return true;
}

void wxDLLApp::CleanUp()
{
    wxApp::CleanUp();
}

#ifndef __WINDOWS__
int __attribute__((constructor)) DllLoad(void);
int __attribute__((destructor)) DllUnload(void);
#endif

// Called when the library is loaded and before dlopen() returns
int DllLoad(void)
{
    int argc = 0;
    char **argv = NULL;
    wxEntryStart(argc, argv);
    if (wxTheApp)
        return wxTheApp->CallOnInit() ? TRUE : FALSE;
    return 0;
}

// Called when the library is unloaded and before dlclose() returns
int DllUnload(void)
{
    if (wxTheApp)
        wxTheApp->OnExit();
    wxEntryCleanup();
    return TRUE;
}

#ifdef _WIN32
void wxSetInstance(HINSTANCE hInstance);
CriticalSection * g_ProcessDListCS = NULL;

extern "C" int WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID /*lpReserved*/)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        hinstDLL = hinst;
        SetupTrace();
        if (g_ProcessDListCS == NULL)
        {
            g_ProcessDListCS = new CriticalSection();
        }
        wxSetInstance(hinstDLL);
        return DllLoad();
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (g_ProcessDListCS)
        {
            delete g_ProcessDListCS;
        }
        return DllUnload();
    }
    return TRUE;
}
#endif

void CALL ReadScreen(void **dest, int *width, int *height)
{
    *width = g_settings->res_x;
    *height = g_settings->res_y;
    uint8_t * buff = (uint8_t*)malloc(g_settings->res_x * g_settings->res_y * 3);
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
        uint32_t offset_src = info.strideInBytes*(g_settings->scr_res_y - 1);

        // Copy the screen
        uint8_t r, g, b;
        if (info.writeMode == GR_LFBWRITEMODE_8888)
        {
            uint32_t col;
            for (uint32_t y = 0; y < g_settings->res_y; y++)
            {
                uint32_t *ptr = (uint32_t*)((uint8_t*)info.lfbPtr + offset_src);
                for (uint32_t x = 0; x < g_settings->res_x; x++)
                {
                    col = *(ptr++);
                    r = (uint8_t)((col >> 16) & 0xFF);
                    g = (uint8_t)((col >> 8) & 0xFF);
                    b = (uint8_t)(col & 0xFF);
                    line[x * 3] = b;
                    line[x * 3 + 1] = g;
                    line[x * 3 + 2] = r;
                }
                line += g_settings->res_x * 3;
                offset_src -= info.strideInBytes;
            }
        }
        else
        {
            uint16_t col;
            for (uint32_t y = 0; y < g_settings->res_y; y++)
            {
                uint16_t *ptr = (uint16_t*)((uint8_t*)info.lfbPtr + offset_src);
                for (uint32_t x = 0; x < g_settings->res_x; x++)
                {
                    col = *(ptr++);
                    r = (uint8_t)((float)(col >> 11) / 31.0f * 255.0f);
                    g = (uint8_t)((float)((col >> 5) & 0x3F) / 63.0f * 255.0f);
                    b = (uint8_t)((float)(col & 0x1F) / 31.0f * 255.0f);
                    line[x * 3] = b;
                    line[x * 3 + 1] = g;
                    line[x * 3 + 2] = r;
                }
                line += g_settings->res_x * 3;
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
    capture_screen = 1;
    capture_path = Directory;
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
#ifdef __WINDOWS__
            if (gfx.hStatusBar)
                ShowWindow(gfx.hStatusBar, SW_HIDE);
            ShowCursor(FALSE);
#endif
        }
        else
        {
            ev_fullscreen = FALSE;
            InitGfx();
#ifdef __WINDOWS__
            ShowCursor(TRUE);
            if (gfx.hStatusBar)
            {
                ShowWindow(gfx.hStatusBar, SW_SHOW);
            }
            SetWindowLongPtr(gfx.hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
#endif
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
#ifdef __WINDOWS__
            if (gfx.hStatusBar)
                ShowWindow(gfx.hStatusBar, SW_HIDE);
            ShowCursor(FALSE);
#endif
        }
        else
        {
            ReleaseGfx();
#ifdef __WINDOWS__
            ShowCursor(TRUE);
            if (gfx.hStatusBar)
                ShowWindow(gfx.hStatusBar, SW_SHOW);
            // SetWindowLong fixes the following Windows XP Banshee issues:
            // 1964 crash error when loading another rom.
            // All N64 emu's minimize, restore crashes.
            SetWindowLongPtr(gfx.hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
#endif
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

    // re-set the old window proc
#ifdef WINPROC_OVERRIDE
    SetWindowLongPtr(gfx.hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
#endif

#ifdef ALTTAB_FIX
    if (hhkLowLevelKybd)
    {
        UnhookWindowsHookEx(hhkLowLevelKybd);
        hhkLowLevelKybd = 0;
    }
#endif

    //CLOSELOG ();

#ifdef TEXTURE_FILTER // Hiroshi Morii <koolsmoky@users.sourceforge.net>
    if (g_settings->ghq_use)
    {
        ext_ghq_shutdown();
        g_settings->ghq_use = 0;
    }
#endif

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
    WriteTrace(TraceGlide64, TraceDebug, "-");
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

    g_settings = new CSettings;
    ReadSettings();
    char name[21] = "DEFAULT";
    ReadSpecialSettings(name);
    g_settings->res_data_org = g_settings->res_data;

#ifdef FPS
    fps_last = wxDateTime::UNow();
#endif

    debug_init();    // Initialize debugger

    gfx = Gfx_Info;

#ifdef WINPROC_OVERRIDE
    // [H.Morii] inject our own winproc so that "alt-enter to fullscreen"
    // message is shown when the emulator window is activated.
    WNDPROC curWndProc = (WNDPROC)GetWindowLongPtr(gfx.hWnd, GWLP_WNDPROC);
    if (curWndProc && curWndProc != (WNDPROC)WndProc) {
        oldWndProc = (WNDPROC)SetWindowLongPtr(gfx.hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
    }
#endif

    util_init();
    math_init();
    TexCacheInit();
    CRC_BuildTable();
    CountCombine();
    if (fb_depth_render_enabled)
        ZLUT_init();

    grConfigWrapperExt(g_settings->wrpResolution, g_settings->wrpVRAM * 1024 * 1024, g_settings->wrpFBO, g_settings->wrpAnisotropic);

    grGlideInit();
    grSstSelect(0);
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
    SetModuleName("default");
    Set_basic_mode = FindSystemSettingId("Basic Mode");
    Set_texture_dir = FindSystemSettingId("Dir:Texture");
    Set_log_flush = FindSystemSettingId("Log Auto Flush");
    Set_log_dir = FindSystemSettingId("Dir:Log");
    SetupTrace();

    WriteTrace(TraceInterface, TraceDebug, "Start");

    SetModuleName("Glide64");
    general_setting(Set_CardId, "card_id", 0);
    general_setting(Set_Resolution, "resolution", 7);
    general_setting(Set_vsync, "vsync", 1);
    general_setting(Set_ssformat, "ssformat", 1);
    general_setting(Set_ShowFps, "show_fps", 0);
    general_setting(Set_clock, "clock", 0);
    general_setting(Set_clock_24_hr, "clock_24_hr", 0);
    general_setting(Set_texenh_options, "texenh_options", 0);
    general_setting(Set_hotkeys, "hotkeys", 1);
    general_setting(Set_wrpResolution, "wrpResolution", 0);
    general_setting(Set_wrpVRAM, "wrpVRAM", 0);
    general_setting(Set_wrpFBO, "wrpFBO", 0);
    general_setting(Set_wrpAnisotropic, "wrpAnisotropic", 0);
    general_setting(Set_autodetect_ucode, "autodetect_ucode", 1);
    general_setting(Set_ucode, "ucode", 2);
    general_setting(Set_wireframe, "wireframe", 0);
    general_setting(Set_wfmode, "wfmode", 1);
    general_setting(Set_logging, "logging", 0);
    general_setting(Set_log_clear, "log_clear", 0);
    general_setting(Set_run_in_window, "run_in_window", 0);
    general_setting(Set_elogging, "elogging", 0);
    general_setting(Set_filter_cache, "filter_cache", 0);
    general_setting(Set_unk_as_red, "unk_as_red", 0);
    general_setting(Set_log_unk, "log_unk", 0);
    general_setting(Set_unk_clear, "unk_clear", 0);
    general_setting(Set_ghq_fltr, "ghq_fltr", 0);
    general_setting(Set_ghq_cmpr, "ghq_cmpr", 0);
    general_setting(Set_ghq_enht, "ghq_enht", 0);
    general_setting(Set_ghq_hirs, "ghq_hirs", 0);
    general_setting(Set_ghq_enht_cmpr, "ghq_enht_cmpr", 0);
    general_setting(Set_ghq_enht_tile, "ghq_enht_tile", 0);
    general_setting(Set_ghq_enht_f16bpp, "ghq_enht_f16bpp", 0);
    general_setting(Set_ghq_enht_gz, "ghq_enht_gz", 1);
    general_setting(Set_ghq_enht_nobg, "ghq_enht_nobg", 0);
    general_setting(Set_ghq_hirs_cmpr, "ghq_hirs_cmpr", 0);
    general_setting(Set_ghq_hirs_tile, "ghq_hirs_tile", 0);
    general_setting(Set_ghq_hirs_f16bpp, "ghq_hirs_f16bpp", 0);
    general_setting(Set_ghq_hirs_gz, "ghq_hirs_gz", 1);
    general_setting(Set_ghq_hirs_altcrc, "ghq_hirs_altcrc", 1);
    general_setting(Set_ghq_cache_save, "ghq_cache_save", 1);
    general_setting(Set_ghq_cache_size, "ghq_cache_size", 0);
    general_setting(Set_ghq_hirs_let_texartists_fly, "ghq_hirs_let_texartists_fly", 0);
    general_setting(Set_ghq_hirs_dump, "ghq_hirs_dump", 0);

    game_setting(Set_alt_tex_size, "alt_tex_size", 0);
    game_setting(Set_use_sts1_only, "use_sts1_only", 0);
    game_setting(Set_force_calc_sphere, "force_calc_sphere", 0);
    game_setting(Set_correct_viewport, "correct_viewport", 0);
    game_setting(Set_increase_texrect_edge, "increase_texrect_edge", 0);
    game_setting(Set_decrease_fillrect_edge, "decrease_fillrect_edge", 0);
    game_setting(Set_texture_correction, "texture_correction", 1);
    game_setting(Set_pal230, "pal230", 0);
    game_setting(Set_stipple_mode, "stipple_mode", 2);

    game_setting(Set_stipple_pattern, "stipple_pattern", 0x3E0F83E0);
    game_setting(Set_force_microcheck, "force_microcheck", 0);
    game_setting(Set_force_quad3d, "force_quad3d", 0);
    game_setting(Set_clip_zmin, "clip_zmin", 0);
    game_setting(Set_clip_zmax, "clip_zmax", 1);
    game_setting(Set_fast_crc, "fast_crc", 1);
    game_setting(Set_adjust_aspect, "adjust_aspect", 1);
    game_setting(Set_zmode_compare_less, "zmode_compare_less", 0);
    game_setting(Set_old_style_adither, "old_style_adither", 0);
    game_setting(Set_n64_z_scale, "n64_z_scale", 0);
    game_setting(Set_optimize_texrect, "optimize_texrect", 1);
    game_setting(Set_ignore_aux_copy, "ignore_aux_copy", (unsigned int)-1);
    game_setting(Set_hires_buf_clear, "hires_buf_clear", 1);
    game_setting(Set_fb_read_alpha, "fb_read_alpha", 0);
    game_setting(Set_useless_is_useless, "useless_is_useless", (unsigned int)-1);
    game_setting(Set_fb_crc_mode, "fb_crc_mode", 1);
    game_setting(Set_filtering, "filtering", 0);
    game_setting(Set_fog, "fog", 1);
    game_setting(Set_buff_clear, "buff_clear", 1);
    game_setting(Set_swapmode, "swapmode", 1);
    game_setting(Set_aspect, "aspect", 0);
    game_setting(Set_lodmode, "lodmode", 0);

    game_setting(Set_fb_smart, "fb_smart", 1);
    game_setting(Set_fb_hires, "fb_hires", 1);
    game_setting(Set_fb_read_always, "fb_read_always", 0);
    game_setting(Set_read_back_to_screen, "read_back_to_screen", 0);
    game_setting(Set_detect_cpu_write, "detect_cpu_write", 0);
    game_setting(Set_fb_get_info, "fb_get_info", 0);
    game_setting(Set_fb_render, "fb_render", 0);

    WriteTrace(TraceInterface, TraceDebug, "Done");
}

/******************************************************************
Function: RomClosed
Purpose:  This function is called when a rom is closed.
input:    none
output:   none
*******************************************************************/
void CALL RomClosed(void)
{
    WriteTrace(TraceGlide64, TraceDebug, "-");

    rdp.window_changed = TRUE;
    romopen = FALSE;
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
    romopen = TRUE;
    ucode_error_report = TRUE;	// allowed to report ucode errors
    rdp_reset();

    // Get the country code & translate to NTSC(0) or PAL(1)
    uint16_t code = ((uint16_t*)gfx.HEADER)[0x1F ^ 1];

    if (code == 0x4400) region = 1; // Germany (PAL)
    if (code == 0x4500) region = 0; // USA (NTSC)
    if (code == 0x4A00) region = 0; // Japan (NTSC)
    if (code == 0x5000) region = 1; // Europe (PAL)
    if (code == 0x5500) region = 0; // Australia (NTSC)

    char name[21] = "DEFAULT";
    ReadSpecialSettings(name);

    // get the name of the ROM
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

    if (g_settings->ghq_use && strcmp(rdp.RomName, name) != 0)
    {
        ext_ghq_shutdown();
        g_settings->ghq_use = 0;
    }
    strcpy(rdp.RomName, name);
    ReadSpecialSettings(name);
    ClearCache();

    CheckDRAMSize();

    // ** EVOODOO EXTENSIONS **
    if (!GfxInitDone)
    {
        grGlideInit();
        grSstSelect(0);
    }
    const char *extensions = grGetString(GR_EXTENSION);
    grGlideShutdown();

    if (strstr(extensions, "EVOODOO"))
        evoodoo = 1;
    else
        evoodoo = 0;

    if (evoodoo)
        InitGfx();

    if (strstr(extensions, "ROMNAME"))
    {
        char strSetRomName[] = "grSetRomName";
        void (FX_CALL *grSetRomName)(char*);
        grSetRomName = (void (FX_CALL *)(char*))grGetProcAddress(strSetRomName);
        grSetRomName(name);
    }
    // **
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
    WriteTrace(TraceGlide64, TraceDebug, "-");
    const uint32_t VIwidth = *gfx.VI_WIDTH_REG;
    FB_TO_SCREEN_INFO fb_info;
    fb_info.width = VIwidth;
    fb_info.height = (uint32_t)rdp.vi_height;
    if (fb_info.height == 0)
    {
        WriteTrace(TraceRDP, TraceDebug, "Image height = 0 - skipping");
        return;
    }
    fb_info.ul_x = 0;

    fb_info.lr_x = VIwidth - 1;
    //  fb_info.lr_x = (uint32_t)rdp.vi_width - 1;
    fb_info.ul_y = 0;
    fb_info.lr_y = fb_info.height - 1;
    fb_info.opaque = 1;
    fb_info.addr = *gfx.VI_ORIGIN_REG;
    fb_info.size = *gfx.VI_STATUS_REG & 3;
    rdp.last_bg = fb_info.addr;

    bool drawn = DrawFrameBufferToScreen(fb_info);
    if (g_settings->hacks&hack_Lego && drawn)
    {
        rdp.updatescreen = 1;
        newSwapBuffers();
        DrawFrameBufferToScreen(fb_info);
    }
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
#ifdef FPS
    // vertical interrupt has occurred, increment counter
    vi_count++;

    // Check frames per second
    fps_next = wxDateTime::UNow();
    wxTimeSpan difference = fps_next - fps_last;
    double diff_secs = difference.GetMilliseconds().ToDouble() / 1000.0;
    if (diff_secs > 0.5f)
    {
        fps = (float)(fps_count / diff_secs);
        vi = (float)(vi_count / diff_secs);
        ntsc_percent = vi / 0.6f;
        pal_percent = vi / 0.5f;
        fps_last = fps_next;
        fps_count = 0;
        vi_count = 0;
    }
#endif
    //*
    uint32_t limit = (g_settings->hacks&hack_Lego) ? 15 : 30;
    if ((g_settings->frame_buffer&fb_cpu_write_hack) && (update_screen_count > limit) && (rdp.last_bg == 0))
    {
        WriteTrace(TraceRDP, TraceDebug, "DirectCPUWrite hack!");
        update_screen_count = 0;
        no_dlist = true;
        ClearCache();
        UpdateScreen();
        return;
    }
    //*/
    //*
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
    //*/
    if (g_settings->swapmode == 0)
        newSwapBuffers();
}

static void DrawWholeFrameBufferToScreen()
{
    static uint32_t toScreenCI = 0;
    if (rdp.ci_width < 200)
        return;
    if (rdp.cimg == toScreenCI)
        return;
    toScreenCI = rdp.cimg;
    FB_TO_SCREEN_INFO fb_info;
    fb_info.addr = rdp.cimg;
    fb_info.size = rdp.ci_size;
    fb_info.width = rdp.ci_width;
    fb_info.height = rdp.ci_height;
    if (fb_info.height == 0)
        return;
    fb_info.ul_x = 0;
    fb_info.lr_x = rdp.ci_width - 1;
    fb_info.ul_y = 0;
    fb_info.lr_y = rdp.ci_height - 1;
    fb_info.opaque = 0;
    DrawFrameBufferToScreen(fb_info);
    if (!(g_settings->frame_buffer & fb_ref))
        memset(gfx.RDRAM + rdp.cimg, 0, (rdp.ci_width*rdp.ci_height) << rdp.ci_size >> 1);
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

uint32_t curframe = 0;
void newSwapBuffers()
{
    if (!rdp.updatescreen)
        return;

    rdp.updatescreen = 0;

    WriteTrace(TraceRDP, TraceDebug, "swapped");

    rdp.update |= UPDATE_SCISSOR | UPDATE_COMBINE | UPDATE_ZBUF_ENABLED | UPDATE_CULL_MODE;
    grClipWindow(0, 0, g_settings->scr_res_x, g_settings->scr_res_y);
    grDepthBufferFunction(GR_CMP_ALWAYS);
    grDepthMask(FXFALSE);
    grCullMode(GR_CULL_DISABLE);

    if ((g_settings->show_fps & 0xF) || g_settings->clock)
        set_message_combiner();
#ifdef FPS
    float y = 0;//(float)g_settings->res_y;
    if (g_settings->show_fps & 0x0F)
    {
        if (g_settings->show_fps & 4)
        {
            if (region)   // PAL
                output(0, y, 1, "%d%% ", (int)pal_percent);
            else
                output(0, y, 1, "%d%% ", (int)ntsc_percent);
            y += 16;
        }
        if (g_settings->show_fps & 2)
        {
            output(0, y, 1, "VI/s: %.02f ", vi);
            y += 16;
        }
        if (g_settings->show_fps & 1)
            output(0, y, 1, "FPS: %.02f ", fps);
    }
#endif

    if (g_settings->clock)
    {
        if (g_settings->clock_24_hr)
        {
            output(956.0f, 0, 1, (char*)wxDateTime::Now().Format("%H:%M:%S").char_str(), 0);
        }
        else
        {
            output(930.0f, 0, 1, (char*)wxDateTime::Now().Format("%I:%M:%S %p").char_str(), 0);
        }
    }
    //hotkeys
    //if (CheckKeyPressed(G64_VK_BACK, 0x0001))
    //{
    //hotkey_info.hk_filtering = 100;
    //if (g_settings->filtering < 2)
    //g_settings->filtering++;
    //else
    //g_settings->filtering = 0;
    //}
    if ((abs((int)(frame_count - curframe)) > 3) && CheckKeyPressed(G64_VK_ALT, 0x8000))  //alt +
    {
        if (CheckKeyPressed(G64_VK_B, 0x8000))  //b
        {
            hotkey_info.hk_motionblur = 100;
            hotkey_info.hk_ref = 0;
            curframe = frame_count;
            g_settings->frame_buffer ^= fb_motionblur;
        }
        else if (CheckKeyPressed(G64_VK_V, 0x8000))  //v
        {
            hotkey_info.hk_ref = 100;
            hotkey_info.hk_motionblur = 0;
            curframe = frame_count;
            g_settings->frame_buffer ^= fb_ref;
        }
    }
    if (g_settings->buff_clear && (hotkey_info.hk_ref || hotkey_info.hk_motionblur || hotkey_info.hk_filtering))
    {
        set_message_combiner();
        char buf[256];
        buf[0] = 0;
        char * message = 0;
        if (hotkey_info.hk_ref)
        {
            if (g_settings->frame_buffer & fb_ref)
                message = strcat(buf, "FB READ ALWAYS: ON");
            else
                message = strcat(buf, "FB READ ALWAYS: OFF");
            hotkey_info.hk_ref--;
        }
        if (hotkey_info.hk_motionblur)
        {
            if (g_settings->frame_buffer & fb_motionblur)
                message = strcat(buf, "  MOTION BLUR: ON");
            else
                message = strcat(buf, "  MOTION BLUR: OFF");
            hotkey_info.hk_motionblur--;
        }
        if (hotkey_info.hk_filtering)
        {
            switch (g_settings->filtering)
            {
            case 0:
                message = strcat(buf, "  FILTERING MODE: AUTOMATIC");
                break;
            case 1:
                message = strcat(buf, "  FILTERING MODE: FORCE BILINEAR");
                break;
            case 2:
                message = strcat(buf, "  FILTERING MODE: FORCE POINT-SAMPLED");
                break;
            }
            hotkey_info.hk_filtering--;
        }
        output(120.0f, 0.0f, 1, message, 0);
    }

    if (capture_screen)
    {
        CPath path(capture_path);
        if (!path.DirectoryExists())
        {
            path.DirectoryCreate();
        }
        stdstr romName = rdp.RomName;
        romName.Replace(" ", "_");
        romName.Replace(":", ";");

        if (g_settings->ssformat > NumOfFormats)
        {
            g_settings->ssformat = 0;
        }
        for (int i = 1;; i++)
        {
            stdstr_f filename("Glide64_%s_%s%d.%s", romName.c_str(), i < 10 ? "0" : "", i, ScreenShotFormats[g_settings->ssformat].extension);
            path.SetNameExtension(filename.c_str());
            if (!path.Exists())
            {
                break;
            }
        }

        const uint32_t offset_x = (uint32_t)rdp.offset_x;
        const uint32_t offset_y = (uint32_t)rdp.offset_y;
        const uint32_t image_width = g_settings->scr_res_x - offset_x * 2;
        const uint32_t image_height = g_settings->scr_res_y - offset_y * 2;

        GrLfbInfo_t info;
        info.size = sizeof(GrLfbInfo_t);
        if (grLfbLock(GR_LFB_READ_ONLY, GR_BUFFER_BACKBUFFER, GR_LFBWRITEMODE_565, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info))
        {
            uint8_t *ssimg = (uint8_t*)malloc(image_width * image_height * 3); // will be free in wxImage destructor
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
            wxImage screenshot(image_width, image_height, ssimg);
            wxString wxPath((const char *)path);
            screenshot.SaveFile(wxPath, ScreenShotFormats[g_settings->ssformat].type);
            capture_screen = 0;
        }
    }

    // Capture the screen if debug capture is set
    if (_debugger.capture)
    {
        // Allocate the screen
        _debugger.screen = new uint8_t[(g_settings->res_x*g_settings->res_y) << 1];

        // Lock the backbuffer (already rendered)
        GrLfbInfo_t info;
        info.size = sizeof(GrLfbInfo_t);
        while (!grLfbLock(GR_LFB_READ_ONLY,
            GR_BUFFER_BACKBUFFER,
            GR_LFBWRITEMODE_565,
            GR_ORIGIN_UPPER_LEFT,
            FXFALSE,
            &info));

        uint32_t offset_src = 0, offset_dst = 0;

        // Copy the screen
        for (uint32_t y = 0; y < g_settings->res_y; y++)
        {
            if (info.writeMode == GR_LFBWRITEMODE_8888)
            {
                uint32_t *src = (uint32_t*)((uint8_t*)info.lfbPtr + offset_src);
                uint16_t *dst = (uint16_t*)(_debugger.screen + offset_dst);
                uint8_t r, g, b;
                uint32_t col;
                for (unsigned int x = 0; x < g_settings->res_x; x++)
                {
                    col = src[x];
                    r = (uint8_t)((col >> 19) & 0x1F);
                    g = (uint8_t)((col >> 10) & 0x3F);
                    b = (uint8_t)((col >> 3) & 0x1F);
                    dst[x] = (r << 11) | (g << 5) | b;
                }
            }
            else
            {
                memcpy(_debugger.screen + offset_dst, (uint8_t*)info.lfbPtr + offset_src, g_settings->res_x << 1);
            }
            offset_dst += g_settings->res_x << 1;
            offset_src += info.strideInBytes;
        }

        // Unlock the backbuffer
        grLfbUnlock(GR_LFB_READ_ONLY, GR_BUFFER_BACKBUFFER);
    }

    if (debugging)
    {
        debug_keys();
        debug_cacheviewer();
        debug_mouse();
    }

    if (g_settings->frame_buffer & fb_read_back_to_screen)
        DrawWholeFrameBufferToScreen();

    if (fb_hwfbe_enabled && !(g_settings->hacks&hack_RE2) && !evoodoo)
        grAuxBufferExt(GR_BUFFER_AUXBUFFER);
    WriteTrace(TraceGlide64, TraceDebug, "BUFFER SWAPPED");
    grBufferSwap(g_settings->vsync);
    fps_count++;
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

    if (_debugger.capture)
        debug_capture();

    if (debugging || g_settings->wireframe || g_settings->buff_clear || (g_settings->hacks&hack_PPL && g_settings->ucode == 6))
    {
        if (g_settings->hacks&hack_RE2 && fb_depth_render_enabled)
            grDepthMask(FXFALSE);
        else
            grDepthMask(FXTRUE);
        grBufferClear(0, 0, 0xFFFF);
    }
    /* //let the game to clear the buffers
    else
    {
    grDepthMask (FXTRUE);
    grColorMask (FXFALSE, FXFALSE);
    grBufferClear (0, 0, 0xFFFF);
    grColorMask (FXTRUE, FXTRUE);
    }
    */

    if (g_settings->frame_buffer & fb_read_back_to_screen2)
    {
        DrawWholeFrameBufferToScreen();
    }
    frame_count++;

    // Open/close debugger?
    if (CheckKeyPressed(G64_VK_SCROLL, 0x0001))
    {
        if (!debugging)
        {
            //if (g_settings->scr_res_x == 1024 && g_settings->scr_res_y == 768)
            {
                debugging = 1;

                // Recalculate screen size, don't resize screen
                g_settings->res_x = (uint32_t)(g_settings->scr_res_x * 0.625f);
                g_settings->res_y = (uint32_t)(g_settings->scr_res_y * 0.625f);

                ChangeSize();
            }
        }
        else
        {
            debugging = 0;

            g_settings->res_x = g_settings->scr_res_x;
            g_settings->res_y = g_settings->scr_res_y;

            ChangeSize();
        }
    }

    // Debug capture?
    if (/*fullscreen && */debugging && CheckKeyPressed(G64_VK_INSERT, 0x0001))
    {
        _debugger.capture = 1;
    }
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

#ifdef WINPROC_OVERRIDE
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ACTIVATEAPP:
        if (wParam == TRUE && !GfxInitDone) rdp.window_changed = TRUE;
        break;
    case WM_PAINT:
        if (!GfxInitDone) rdp.window_changed = TRUE;
        break;

        /*    case WM_DESTROY:
        SetWindowLong (gfx.hWnd, GWL_WNDPROC, (long)oldWndProc);
        break;*/
    }

    return CallWindowProc(oldWndProc, hwnd, msg, wParam, lParam);
}
#endif

int CheckKeyPressed(int key, int mask)
{
    static Glide64Keys g64Keys;
    if (g_settings->use_hotkeys == 0)
        return 0;
#ifdef __WINDOWS__
    return (GetAsyncKeyState(g64Keys[key]) & mask);
#else
    if (grKeyPressed)
        return grKeyPressed(g64Keys[key]);
    return 0;
#endif
}

#ifdef ALTTAB_FIX
int k_ctl = 0, k_alt = 0, k_del = 0;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode,
    WPARAM wParam, LPARAM lParam)
{
    if (!fullscreen) return CallNextHookEx(NULL, nCode, wParam, lParam);

    int TabKey = FALSE;

    PKBDLLHOOKSTRUCT p;

    if (nCode == HC_ACTION)
    {
        switch (wParam) {
        case WM_KEYUP:    case WM_SYSKEYUP:
            p = (PKBDLLHOOKSTRUCT) lParam;
            if (p->vkCode == 162) k_ctl = 0;
            if (p->vkCode == 164) k_alt = 0;
            if (p->vkCode == 46) k_del = 0;
            goto do_it;

        case WM_KEYDOWN:  case WM_SYSKEYDOWN:
            p = (PKBDLLHOOKSTRUCT) lParam;
            if (p->vkCode == 162) k_ctl = 1;
            if (p->vkCode == 164) k_alt = 1;
            if (p->vkCode == 46) k_del = 1;
            goto do_it;

        do_it:
            TabKey =
                ((p->vkCode == VK_TAB) && ((p->flags & LLKHF_ALTDOWN) != 0)) ||
                ((p->vkCode == VK_ESCAPE) && ((p->flags & LLKHF_ALTDOWN) != 0)) ||
                ((p->vkCode == VK_ESCAPE) && ((GetKeyState(VK_CONTROL) & 0x8000) != 0)) ||
                (k_ctl && k_alt && k_del);

            break;
        }
    }

    if (TabKey)
    {
        k_ctl = 0;
        k_alt = 0;
        k_del = 0;
        ReleaseGfx ();
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
#endif