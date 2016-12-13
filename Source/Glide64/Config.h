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
// Glide64 - Glide Plugin for Nintendo 64 emulators (tested mostly with Project64)
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
//
// Glide64 dialogs
// Created by Gonetz, 2008
//
//****************************************************************
#pragma once

#ifdef _WIN32
void ConfigInit(HINSTANCE hinst);
void ConfigCleanup(void);
#endif

enum
{
    // General Settings
    Set_CardId, Set_vsync, Set_ssformat, Set_clock,
    Set_clock_24_hr, Set_Rotate, Set_texenh_options, Set_hotkeys, Set_wrpVRAM,
	Set_wrpFBO, Set_wrpAnisotropic, Set_autodetect_ucode, Set_ucode, Set_wireframe,
    Set_wfmode, Set_logging, Set_log_clear, Set_elogging, Set_run_in_window,
    Set_filter_cache, Set_unk_as_red, Set_log_unk, Set_unk_clear, Set_ghq_fltr,
    Set_ghq_cmpr, Set_ghq_enht, Set_ghq_hirs, Set_ghq_enht_cmpr, Set_ghq_enht_tile,
    Set_ghq_enht_f16bpp, Set_ghq_enht_gz, Set_ghq_enht_nobg, Set_ghq_hirs_cmpr,
    Set_ghq_hirs_tile, Set_ghq_hirs_f16bpp, Set_ghq_hirs_gz, Set_ghq_hirs_altcrc,
    Set_ghq_cache_save, Set_ghq_cache_size, Set_ghq_hirs_let_texartists_fly,
    Set_ghq_hirs_dump,

#ifndef ANDROID
    Set_Resolution, Set_wrpResolution,
#endif

    // Default Game Settings
    Set_optimize_texrect_default, Set_filtering_default, Set_lodmode_default,
    Set_fog_default, Set_buff_clear_default, Set_swapmode_default,
    Set_aspect_default, Set_fb_smart_default, Set_fb_hires_default,
    Set_fb_read_always_default, Set_read_back_to_screen_default, Set_detect_cpu_write_default,
    Set_fb_get_info_default, Set_fb_render_default,

    //Game Settings
    Set_alt_tex_size, Set_use_sts1_only, Set_force_calc_sphere, Set_correct_viewport,
    Set_increase_texrect_edge, Set_decrease_fillrect_edge, Set_texture_correction,
    Set_pal230, Set_stipple_mode, Set_stipple_pattern, Set_force_microcheck, Set_force_quad3d,
    Set_clip_zmin, Set_clip_zmax, Set_fast_crc, Set_adjust_aspect, Set_zmode_compare_less,
    Set_old_style_adither, Set_n64_z_scale, Set_optimize_texrect, Set_ignore_aux_copy,
    Set_hires_buf_clear, Set_fb_read_alpha, Set_useless_is_useless, Set_fb_crc_mode,
    Set_filtering, Set_fog, Set_buff_clear, Set_swapmode, Set_aspect, Set_lodmode,
    Set_fb_smart, Set_fb_hires, Set_fb_read_always, Set_read_back_to_screen,
    Set_detect_cpu_write, Set_fb_get_info, Set_fb_render,

    //RDB Setting
    Set_ucodeLookup,
};

extern short Set_basic_mode, Set_texture_dir, Set_log_dir, Set_log_flush;

extern void general_setting(short setting_ID, const char * name, unsigned int value);
extern void game_setting(short setting_ID, const char * name, unsigned int value);
extern void game_setting_default(short setting_ID, const char * name, short default_setting);
