/****************************************************************************
*                                                                           *
* Project64-video - A Nintendo 64 gfx plugin.                               *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

enum
{
    // General Settings
    Set_vsync, Set_Rotate, Set_texenh_options, Set_wrpVRAM,
    Set_wrpFBO, Set_wrpAnisotropic, Set_autodetect_ucode, Set_ucode, Set_wireframe,
    Set_wfmode, Set_ghq_fltr, Set_ghq_cmpr, Set_ghq_enht, Set_ghq_hirs, Set_ghq_enht_cmpr,
    Set_ghq_enht_f16bpp, Set_ghq_enht_gz, Set_ghq_enht_nobg, Set_ghq_hirs_cmpr,
    Set_ghq_hirs_tile, Set_ghq_hirs_f16bpp, Set_ghq_hirs_gz, Set_ghq_hirs_altcrc,
    Set_ghq_cache_save, Set_ghq_cache_size, Set_ghq_hirs_let_texartists_fly,
    Set_ghq_hirs_dump, Set_Resolution,

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

    //Logging Settings
    Set_Logging_MD5, Set_Logging_Thread, Set_Logging_Path, Set_Logging_Settings, 
    Set_Logging_Unknown, Set_Logging_Glide64, Set_Logging_Interface, Set_Logging_Resolution, 
    Set_Logging_Glitch, Set_Logging_VideoRDP, Set_Logging_TLUT, Set_Logging_PNG, 
    Set_Logging_OGLWrapper, Set_Logging_RDPCommands,

#ifdef _WIN32
    Set_FullScreenRes,
#endif
};
