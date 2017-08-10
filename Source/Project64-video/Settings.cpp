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
#include <Common/StdString.h>
#include <Settings/Settings.h>
#include <Project64-video/trace.h>
#include "Gfx_1.3.h"
#include "ScreenResolution.h"
#include "SettingsID.h"
#include "trace.h"

CSettings::CSettings() :
    m_Set_basic_mode(0),
    m_Set_debugger(0),
    m_Set_texture_dir(0),
    m_Set_log_dir(0),
    m_Set_log_flush(0),
    m_dirty(false),
    m_ScreenRes(GetDefaultScreenRes()),
    m_advanced_options(false),
    m_debugger_enabled(false),
    m_texenh_options(false),
    m_vsync(false),
    m_rotate(Rotate_None),
    m_filtering(Filter_Automatic),
    m_fog(false),
    m_buff_clear(false),
    m_swapmode(SwapMode_Old),
    m_lodmode(LOD_Off),
    m_aspectmode(Aspect_4x3),
    m_frame_buffer(0),
    m_fb_crc_mode(fbcrcFast),
    //Texture filtering options
    m_texture_dir(""),
    m_ghq_fltr(TextureFilter_None),
    m_ghq_enht(TextureEnht_None),
    m_ghq_cmpr(TextureCompression_S3TC),
    m_ghq_hirs(HiResPackFormat_None),
    m_ghq_enht_cmpr(false),
    m_ghq_enht_f16bpp(false),
    m_ghq_enht_gz(false),
    m_ghq_enht_nobg(false),
    m_ghq_hirs_cmpr(false),
    m_ghq_hirs_tile(false),
    m_ghq_hirs_f16bpp(false),
    m_ghq_hirs_gz(false),
    m_ghq_hirs_altcrc(false),
    m_ghq_cache_save(false),
    m_ghq_cache_size(0),
    m_ghq_hirs_let_texartists_fly(false),
    m_ghq_hirs_dump(false),
    m_autodetect_ucode(true),
    m_ucode(ucode_Fast3D),
    m_unk_as_red(false),
    m_wireframe(false),
    m_wfmode(wfmode_VertexColors),

    // Special fixes
    m_fast_crc(false),
    m_alt_tex_size(false),
    m_use_sts1_only(false),
    m_flame_corona(false), //hack for zeldas flame's corona
    m_increase_texrect_edge(false), // add 1 to lower right corner coordinates of texrect
    m_decrease_fillrect_edge(false), // sub 1 from lower right corner coordinates of fillrect
    m_texture_correction(false), // enable perspective texture correction emulation. is on by default
    m_stipple_mode(GFX_STIPPLE_DISABLE), //used for dithered alpha emulation
    m_stipple_pattern(0), //used for dithered alpha emulation
    m_force_microcheck(false), //check microcode each frame, for mixed F3DEX-S2DEX games
    m_force_quad3d(false), //force 0xb5 command to be quad, not line 3d
    m_clip_zmin(false), //enable near z clipping
    m_clip_zmax(false), //enable far plane clipping;
    m_adjust_aspect(false), //adjust screen aspect for wide screen mode
    m_force_calc_sphere(false), //use spheric mapping only, Ridge Racer 64
    m_pal230(false),    //set special scale for PAL games
    m_correct_viewport(false), //correct viewport values
    m_zmode_compare_less(false), //force GFX_CMP_LESS for zmode=0 (opaque)and zmode=1 (interpenetrating)
    m_old_style_adither(false), //apply alpha dither regardless of alpha_dither_mode
    m_n64_z_scale(false), //scale vertex z value before writing to depth buffer, as N64 does.

    m_hacks((hacks_t)0),

    //wrapper settings
#ifndef ANDROID
    m_FullScreenRes(0),
#endif
    m_wrpVRAM(0),
    m_wrpFBO(false),
    m_wrpAnisotropic(false),
    m_FlushLogs(false),
    m_InWriteSettings(false)
{
    memset(m_log_dir, 0, sizeof(m_log_dir));
    RegisterSettings();
    ReadSettings();
}

CSettings::~CSettings()
{
    SettingsUnregisterChange(false, Set_Resolution, this, stSettingsChanged);
    SettingsUnregisterChange(false, Set_aspect, this, stSettingsChanged);
    SettingsUnregisterChange(false, Set_Logging_MD5, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Thread, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Path, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Settings, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Unknown, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Glide64, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Interface, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Resolution, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Glitch, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_VideoRDP, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_TLUT, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_OGLWrapper, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_RDPCommands, this, stLogLevelChanged);
}

void CSettings::RegisterSettings(void)
{
    SetModuleName("default");
    m_Set_basic_mode = FindSystemSettingId("Basic Mode");
    m_Set_debugger = FindSystemSettingId("Debugger");
    m_Set_texture_dir = FindSystemSettingId("Dir:Texture");
    m_Set_log_flush = FindSystemSettingId("Log Auto Flush");
    m_Set_log_dir = FindSystemSettingId("Dir:Log");

    SetModuleName("Glide64");
    general_setting(Set_Resolution, "resolution", GetDefaultScreenRes());
    general_setting(Set_vsync, "vsync", true);
    general_setting(Set_texenh_options, "texenh_options", false);
    general_setting(Set_wrpVRAM, "wrpVRAM", 0);
#ifndef ANDROID
    general_setting(Set_wrpFBO, "wrpFBO", false);
#else
    general_setting(Set_wrpFBO, "wrpFBO", true);
#endif
    general_setting(Set_Rotate, "rotate", Rotate_None);
    general_setting(Set_wrpAnisotropic, "wrpAnisotropic", false);
    general_setting(Set_autodetect_ucode, "autodetect_ucode", true);
    general_setting(Set_ucode, "ucode", ucode_F3DEX2);
    general_setting(Set_wireframe, "wireframe", false);
    general_setting(Set_wfmode, "wfmode", wfmode_VertexColors);
    general_setting(Set_ghq_fltr, "ghq_fltr", TextureFilter_None);
    general_setting(Set_ghq_cmpr, "ghq_cmpr", TextureCompression_S3TC);
    general_setting(Set_ghq_enht, "ghq_enht", TextureEnht_None);
    general_setting(Set_ghq_hirs, "ghq_hirs", HiResPackFormat_None);
    general_setting(Set_ghq_enht_cmpr, "ghq_enht_cmpr", false);
    general_setting(Set_ghq_enht_f16bpp, "ghq_enht_f16bpp", false);
    general_setting(Set_ghq_enht_gz, "ghq_enht_gz", true);
    general_setting(Set_ghq_enht_nobg, "ghq_enht_nobg", false);
    general_setting(Set_ghq_hirs_cmpr, "ghq_hirs_cmpr", false);
    general_setting(Set_ghq_hirs_tile, "ghq_hirs_tile", false);
    general_setting(Set_ghq_hirs_f16bpp, "ghq_hirs_f16bpp", false);
    general_setting(Set_ghq_hirs_gz, "ghq_hirs_gz", true);
    general_setting(Set_ghq_hirs_altcrc, "ghq_hirs_altcrc", true);
    general_setting(Set_ghq_cache_save, "ghq_cache_save", true);
    general_setting(Set_ghq_cache_size, "ghq_cache_size", 0);
    general_setting(Set_ghq_hirs_let_texartists_fly, "ghq_hirs_let_texartists_fly", false);
    general_setting(Set_ghq_hirs_dump, "ghq_hirs_dump", false);

    general_setting(Set_optimize_texrect_default, "optimize_texrect", true);
    general_setting(Set_filtering_default, "filtering", CSettings::Filter_Automatic);
    general_setting(Set_lodmode_default, "lodmode", CSettings::LOD_Off);
    general_setting(Set_fog_default, "fog", true);
    general_setting(Set_buff_clear_default, "buff_clear", true);
    general_setting(Set_swapmode_default, "swapmode", SwapMode_New);
    general_setting(Set_aspect_default, "aspect", Aspect_4x3);

    general_setting(Set_fb_smart_default, "fb_smart", true);
    general_setting(Set_fb_hires_default, "fb_hires", true);
    general_setting(Set_fb_read_always_default, "fb_read_always", false);
    general_setting(Set_read_back_to_screen_default, "read_back_to_screen", false);
    general_setting(Set_detect_cpu_write_default, "detect_cpu_write", false);
    general_setting(Set_fb_get_info_default, "fb_get_info", false);
    general_setting(Set_fb_render_default, "fb_render", false);

    RegisterSetting(Set_Logging_MD5, Data_DWORD_General, "MD5", "Logging", g_ModuleLogLevel[TraceMD5], NULL);
    RegisterSetting(Set_Logging_Thread, Data_DWORD_General, "Thread", "Logging", g_ModuleLogLevel[TraceThread], NULL);
    RegisterSetting(Set_Logging_Path, Data_DWORD_General, "Path", "Logging", g_ModuleLogLevel[TracePath], NULL);
    RegisterSetting(Set_Logging_Settings, Data_DWORD_General, "Settings", "Logging", g_ModuleLogLevel[TraceSettings], NULL);
    RegisterSetting(Set_Logging_Unknown, Data_DWORD_General, "Unknown", "Logging", g_ModuleLogLevel[TraceUnknown], NULL);
    RegisterSetting(Set_Logging_Glide64, Data_DWORD_General, "Glide64", "Logging", g_ModuleLogLevel[TraceGlide64], NULL);
    RegisterSetting(Set_Logging_Interface, Data_DWORD_General, "Interface", "Logging", g_ModuleLogLevel[TraceInterface], NULL);
    RegisterSetting(Set_Logging_Resolution, Data_DWORD_General, "Resolution", "Logging", g_ModuleLogLevel[TraceResolution], NULL);
    RegisterSetting(Set_Logging_Glitch, Data_DWORD_General, "Glitch", "Logging", g_ModuleLogLevel[TraceGlitch], NULL);
    RegisterSetting(Set_Logging_VideoRDP, Data_DWORD_General, "VideoRDP", "Logging", g_ModuleLogLevel[TraceRDP], NULL);
    RegisterSetting(Set_Logging_TLUT, Data_DWORD_General, "TLUT", "Logging", g_ModuleLogLevel[TraceTLUT], NULL);
    RegisterSetting(Set_Logging_PNG, Data_DWORD_General, "PNG", "Logging", g_ModuleLogLevel[TracePNG], NULL);
    RegisterSetting(Set_Logging_OGLWrapper, Data_DWORD_General, "OGLWrapper", "Logging", g_ModuleLogLevel[TraceOGLWrapper], NULL);
    RegisterSetting(Set_Logging_RDPCommands, Data_DWORD_General, "RDPCommands", "Logging", g_ModuleLogLevel[TraceRDPCommands], NULL);

#ifndef ANDROID
    general_setting(Set_FullScreenRes, "FullScreenRes", GetCurrentResIndex());
#endif

    game_setting(Set_alt_tex_size, "alt_tex_size", false);
    game_setting(Set_use_sts1_only, "use_sts1_only", false);
    game_setting(Set_force_calc_sphere, "force_calc_sphere", false);
    game_setting(Set_correct_viewport, "correct_viewport", false);
    game_setting(Set_increase_texrect_edge, "increase_texrect_edge", false);
    game_setting(Set_decrease_fillrect_edge, "decrease_fillrect_edge", false);
    game_setting(Set_texture_correction, "texture_correction", true);
    game_setting(Set_pal230, "pal230", false);
    game_setting(Set_stipple_mode, "stipple_mode", GFX_STIPPLE_ROTATE);

    game_setting(Set_stipple_pattern, "stipple_pattern", 0x3E0F83E0);
    game_setting(Set_force_microcheck, "force_microcheck", false);
    game_setting(Set_force_quad3d, "force_quad3d", false);
    game_setting(Set_clip_zmin, "clip_zmin", false);
    game_setting(Set_clip_zmax, "clip_zmax", true);
    game_setting(Set_fast_crc, "fast_crc", true);
    game_setting(Set_adjust_aspect, "adjust_aspect", true);
    game_setting(Set_zmode_compare_less, "zmode_compare_less", false);
    game_setting(Set_old_style_adither, "old_style_adither", false);
    game_setting(Set_n64_z_scale, "n64_z_scale", false);
    game_setting_default(Set_optimize_texrect, "optimize_texrect", Set_optimize_texrect_default);
    game_setting(Set_ignore_aux_copy, "ignore_aux_copy", false);
    game_setting(Set_hires_buf_clear, "hires_buf_clear", true);
    game_setting(Set_fb_read_alpha, "fb_read_alpha", false);
    game_setting(Set_useless_is_useless, "useless_is_useless", false);
    game_setting(Set_fb_crc_mode, "fb_crc_mode", fbcrcFast);
    game_setting_default(Set_filtering, "filtering", Set_filtering_default);
    game_setting_default(Set_fog, "fog", Set_fog_default);
    game_setting_default(Set_buff_clear, "buff_clear", Set_buff_clear_default);
    game_setting_default(Set_swapmode, "swapmode", Set_swapmode_default);
    game_setting_default(Set_aspect, "aspect", Set_aspect_default);
    game_setting_default(Set_lodmode, "lodmode", Set_lodmode_default);

    game_setting_default(Set_fb_smart, "fb_smart", Set_fb_smart_default);
    game_setting_default(Set_fb_hires, "fb_hires", Set_fb_hires_default);
    game_setting_default(Set_fb_read_always, "fb_read_always", Set_fb_read_always_default);
    game_setting_default(Set_read_back_to_screen, "read_back_to_screen", Set_read_back_to_screen_default);
    game_setting_default(Set_detect_cpu_write, "detect_cpu_write", Set_detect_cpu_write_default);
    game_setting_default(Set_fb_get_info, "fb_get_info", Set_fb_get_info_default);
    game_setting_default(Set_fb_render, "fb_render", Set_fb_render_default);

    SettingsRegisterChange(false, Set_Resolution, this, stSettingsChanged);
    SettingsRegisterChange(false, Set_aspect, this, stSettingsChanged);

    LogLevelChanged();
    SettingsRegisterChange(false, Set_Logging_MD5, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Thread, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Path, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Settings, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Unknown, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Glide64, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Interface, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Resolution, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Glitch, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_VideoRDP, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_TLUT, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_OGLWrapper, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_RDPCommands, this, stLogLevelChanged);
}

void CSettings::SetTexenhOptions(bool value)
{
    if (value != m_texenh_options)
    {
        m_texenh_options = value;
        m_dirty = true;
    }
}

void CSettings::SetScreenRes(uint32_t value)
{
    if (value >= GetScreenResolutionCount())
    {
        value = GetDefaultScreenRes();
    }

    if (value != m_ScreenRes)
    {
        m_ScreenRes = value;
        m_dirty = true;
    }
}

void CSettings::SetAspectmode(AspectMode_t value)
{
    if (value != m_aspectmode)
    {
        m_aspectmode = value;
        m_dirty = true;
    }
}

void CSettings::SetLODmode(PixelLevelOfDetail_t value)
{
    if (value != m_lodmode)
    {
        m_lodmode = value;
        m_dirty = true;
    }
}

void CSettings::SetVsync(bool value)
{
    if (value != m_vsync)
    {
        m_vsync = value;
        m_dirty = true;
    }
}

void CSettings::SetFiltering(Filtering_t value)
{
    if (value != m_filtering)
    {
        m_filtering = value;
        m_dirty = true;
    }
}

void CSettings::SetSwapMode(SwapMode_t value)
{
    if (value != m_swapmode)
    {
        m_swapmode = value;
        m_dirty = true;
    }
}

void CSettings::SetFog(bool value)
{
    if (value != m_fog)
    {
        m_fog = value;
        m_dirty = true;
    }
}

void CSettings::SetBuffClear(bool value)
{
    if (value != m_buff_clear)
    {
        m_buff_clear = value;
        m_dirty = true;
    }
}

void CSettings::SetWrpAnisotropic(bool value)
{
    if (value != m_wrpAnisotropic)
    {
        m_wrpAnisotropic = value;
        m_dirty = true;
    }
}

void CSettings::SetWrpVRAM(int value)
{
    if (value != m_wrpVRAM)
    {
        m_wrpVRAM = value;
        m_dirty = true;
    }
}

void CSettings::SetWrpFBO(bool value)
{
    if (value != m_wrpFBO)
    {
        m_wrpFBO = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqFltr(TextureFilter_t value)
{
    if (value != m_ghq_fltr)
    {
        m_ghq_fltr = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqEnht(TextureEnhancement_t value)
{
    if (value != m_ghq_enht)
    {
        m_ghq_enht = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqCmpr(TextureCompression_t value)
{
    if (value != m_ghq_cmpr)
    {
        m_ghq_cmpr = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqHirs(HiResPackFormat_t value)
{
    if (value != m_ghq_hirs)
    {
        m_ghq_hirs = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqEnhtGz(bool value)
{
    if (value != m_ghq_enht_gz)
    {
        m_ghq_enht_gz = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqHirsTile(bool value)
{
    if (value != m_ghq_hirs_tile)
    {
        m_ghq_hirs_tile = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqHirsF16bpp(bool value)
{
    if (value != m_ghq_hirs_f16bpp)
    {
        m_ghq_hirs_f16bpp = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqHirsDump(bool value)
{
    if (value != m_ghq_hirs_dump)
    {
        m_ghq_hirs_dump = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqEnhtNobg(bool value)
{
    if (value != m_ghq_enht_nobg)
    {
        m_ghq_enht_nobg = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqEnhtCmpr(bool value)
{
    if (value != m_ghq_enht_cmpr)
    {
        m_ghq_enht_cmpr = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqHirsAltcrc(bool value)
{
    if (value != m_ghq_hirs_altcrc)
    {
        m_ghq_hirs_altcrc = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqHirsCmpr(bool value)
{
    if (value != m_ghq_hirs_cmpr)
    {
        m_ghq_hirs_cmpr = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqHirsGz(bool value)
{
    if (value != m_ghq_hirs_gz)
    {
        m_ghq_hirs_gz = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqHirsLetTexartistsFly(bool value)
{
    if (value != m_ghq_hirs_let_texartists_fly)
    {
        m_ghq_hirs_let_texartists_fly = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqCacheSave(bool value)
{
    if (value != m_ghq_cache_save)
    {
        m_ghq_cache_save = value;
        m_dirty = true;
    }
}

void CSettings::SetGhqCacheSize(int value)
{
    if (value != m_ghq_cache_size)
    {
        m_ghq_cache_size = value;
        m_dirty = true;
    }
}

void CSettings::UpdateFrameBufferBits(uint32_t BitsToAdd, uint32_t BitsToRemove)
{
    uint32_t frame_buffer_original = m_frame_buffer;
    m_frame_buffer |= BitsToAdd;
    m_frame_buffer &= ~BitsToRemove;
    if (frame_buffer_original != m_frame_buffer)
    {
        m_dirty = true;
    }
}

CSettings::ucode_t CSettings::DetectUCode(uint32_t uc_crc)
{
    RegisterSetting(Set_ucodeLookup, Data_DWORD_RDB_Setting, stdstr_f("%08lx", uc_crc).c_str(), "ucode", (unsigned int)-2, NULL);
    CSettings::ucode_t uc = (CSettings::ucode_t)GetSetting(Set_ucodeLookup);
    if (uc == CSettings::uCode_NotFound || uc == CSettings::uCode_Unsupported)
    {
        m_ucode = (CSettings::ucode_t)GetSetting(Set_ucode);
    }
    else
    {
        m_ucode = uc;
    }
    return uc;
}

void CSettings::SetUcode(ucode_t value)
{
    m_ucode = value;
}

#ifndef ANDROID
void CSettings::SetFullScreenRes(uint32_t value)
{
    if (value != m_FullScreenRes)
    {
        m_FullScreenRes = value;
        m_dirty = true;
    }
}
#endif

void CSettings::ReadSettings()
{
    SetScreenRes(GetSetting(Set_Resolution));
#ifndef ANDROID
    SetFullScreenRes(GetSetting(Set_FullScreenRes));
#endif
    m_vsync = GetSetting(Set_vsync) != 0;
    m_rotate = (ScreenRotate_t)GetSetting(Set_Rotate);
    m_advanced_options = m_Set_basic_mode ? GetSystemSetting(m_Set_basic_mode) == 0 : false;
    m_debugger_enabled = m_advanced_options && m_Set_debugger ? GetSystemSetting(m_Set_debugger) == 1 : false;
    m_texenh_options = GetSetting(Set_texenh_options) != 0;
    m_aspectmode = (AspectMode_t)GetSetting(Set_aspect);

    m_wrpVRAM = GetSetting(Set_wrpVRAM);
    m_wrpFBO = GetSetting(Set_wrpFBO) != 0;
    m_wrpAnisotropic = GetSetting(Set_wrpAnisotropic) != 0;

    m_autodetect_ucode = GetSetting(Set_autodetect_ucode) != 0;
    m_wireframe = GetSetting(Set_wireframe) != 0;
    m_wfmode = (wfmode_t)GetSetting(Set_wfmode);
    m_ucode = ucode_F3DEX2;

    char texture_dir[260];
    memset(texture_dir, 0, sizeof(texture_dir));
    GetSystemSettingSz(m_Set_texture_dir, texture_dir, sizeof(texture_dir));
    m_texture_dir = texture_dir;
    m_ghq_fltr = (TextureFilter_t)GetSetting(Set_ghq_fltr);
    m_ghq_cmpr = (TextureCompression_t)GetSetting(Set_ghq_cmpr);
    m_ghq_enht = (TextureEnhancement_t)GetSetting(Set_ghq_enht);
    m_ghq_hirs = (HiResPackFormat_t)GetSetting(Set_ghq_hirs);
    m_ghq_enht_cmpr = GetSetting(Set_ghq_enht_cmpr) != 0;
    m_ghq_enht_f16bpp = GetSetting(Set_ghq_enht_f16bpp) != 0;
    m_ghq_enht_gz = GetSetting(Set_ghq_enht_gz) != 0;
    m_ghq_enht_nobg = GetSetting(Set_ghq_enht_nobg) != 0;
    m_ghq_hirs_cmpr = GetSetting(Set_ghq_hirs_cmpr) != 0;
    m_ghq_hirs_tile = GetSetting(Set_ghq_hirs_tile) != 0;
    m_ghq_hirs_f16bpp = GetSetting(Set_ghq_hirs_f16bpp) != 0;
    m_ghq_hirs_gz = GetSetting(Set_ghq_hirs_gz) != 0;
    m_ghq_hirs_altcrc = GetSetting(Set_ghq_hirs_altcrc) != 0;
    m_ghq_cache_save = GetSetting(Set_ghq_cache_save) != 0;
    m_ghq_cache_size = GetSetting(Set_ghq_cache_size);
    m_ghq_hirs_let_texartists_fly = GetSetting(Set_ghq_hirs_let_texartists_fly) != 0;
    m_ghq_hirs_dump = GetSetting(Set_ghq_hirs_dump) != 0;

    if (m_Set_log_dir != 0)
    {
        GetSystemSettingSz(m_Set_log_dir, m_log_dir, sizeof(m_log_dir));
    }
    m_FlushLogs = m_Set_log_flush != 0 ? GetSystemSetting(m_Set_log_flush) != 0 : false;
    m_dirty = false;
}

void CSettings::ReadGameSettings(const char * name)
{
    m_hacks = (hacks_t)0;

    //detect games which require special hacks
    if (strstr(name, (const char *)"ZELDA"))
    {
        m_hacks = (hacks_t)(m_hacks | (CSettings::hack_Zelda | CSettings::hack_OoT));
    }
    else if (strstr(name, (const char *)"MASK"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Zelda);
    }
    else if (strstr(name, (const char *)"ROADSTERS TROPHY"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Zelda);
    }
    else if (strstr(name, (const char *)"Diddy Kong Racing"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Diddy);
    }
    else if (strstr(name, (const char *)"Tonic Trouble"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Tonic);
    }
    else if (strstr(name, (const char *)"All") && strstr(name, (const char *)"Star") && strstr(name, (const char *)"Baseball"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_ASB);
    }
    else if (strstr(name, (const char *)"Beetle") || strstr(name, (const char *)"BEETLE") || strstr(name, (const char *)"HSV"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_BAR);
    }
    else if (strstr(name, (const char *)"I S S 64") || strstr(name, (const char *)"J WORLD SOCCER3") || strstr(name, (const char *)"PERFECT STRIKER") || strstr(name, (const char *)"RONALDINHO SOCCER"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_ISS64);
    }
    else if (strstr(name, (const char *)"MARIOKART64"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_MK64);
    }
    else if (strstr(name, (const char *)"NITRO64"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_WCWnitro);
    }
    else if (strstr(name, (const char *)"CHOPPER_ATTACK") || strstr(name, (const char *)"WILD CHOPPERS"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Chopper);
    }
    else if (strstr(name, (const char *)"Resident Evil II") || strstr(name, (const char *)"BioHazard II"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_RE2);
    }
    else if (strstr(name, (const char *)"YOSHI STORY"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Yoshi);
    }
    else if (strstr(name, (const char *)"F-Zero X") || strstr(name, (const char *)"F-ZERO X"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Fzero);
    }
    else if (strstr(name, (const char *)"PAPER MARIO") || strstr(name, (const char *)"MARIO STORY"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_PMario);
    }
    else if (strstr(name, (const char *)"TOP GEAR RALLY 2"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_TGR2);
    }
    else if (strstr(name, (const char *)"TOP GEAR RALLY"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_TGR);
    }
    else if (strstr(name, (const char *)"Top Gear Hyper Bike"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Hyperbike);
    }
    else if (strstr(name, (const char *)"Killer Instinct Gold") || strstr(name, (const char *)"KILLER INSTINCT GOLD"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_KI);
    }
    else if (strstr(name, (const char *)"Knockout Kings 2000"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Knockout);
    }
    else if (strstr(name, (const char *)"LEGORacers"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Lego);
    }
    else if (strstr(name, (const char *)"OgreBattle64"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Ogre64);
    }
    else if (strstr(name, (const char *)"Pilot Wings64"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Pilotwings);
    }
    else if (strstr(name, (const char *)"Supercross"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Supercross);
    }
    else if (strstr(name, (const char *)"STARCRAFT 64"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Starcraft);
    }
    else if (strstr(name, (const char *)"BANJO KAZOOIE 2") || strstr(name, (const char *)"BANJO TOOIE"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Banjo2);
    }
    else if (strstr(name, (const char *)"FIFA: RTWC 98") || strstr(name, (const char *)"RoadToWorldCup98"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Fifa98);
    }
    else if (strstr(name, (const char *)"Mega Man 64") || strstr(name, (const char *)"RockMan Dash"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Megaman);
    }
    else if (strstr(name, (const char *)"MISCHIEF MAKERS") || strstr(name, (const char *)"TROUBLE MAKERS"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Makers);
    }
    else if (strstr(name, (const char *)"GOLDENEYE"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_GoldenEye);
    }
    else if (strstr(name, (const char *)"PUZZLE LEAGUE"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_PPL);
    }
    else if (strstr(name, (const char *)"WIN BACK") || strstr(name, (const char *)"OPERATION WINBACK"))
    {
        m_hacks = (hacks_t)(m_hacks | CSettings::hack_Winback);
    }

    m_alt_tex_size = GetSetting(Set_alt_tex_size) != 0;
    m_use_sts1_only = GetSetting(Set_use_sts1_only) != 0;
    m_force_calc_sphere = GetSetting(Set_force_calc_sphere) != 0;
    m_correct_viewport = GetSetting(Set_correct_viewport);
    m_increase_texrect_edge = GetSetting(Set_increase_texrect_edge) != 0;
    m_decrease_fillrect_edge = GetSetting(Set_decrease_fillrect_edge) != 0;
    m_texture_correction = GetSetting(Set_texture_correction) != 0;
    m_pal230 = GetSetting(Set_pal230) != 0;
    m_stipple_mode = (gfxStippleMode_t)GetSetting(Set_stipple_mode);
    int stipple_pattern = GetSetting(Set_stipple_pattern);
    m_stipple_pattern = stipple_pattern > 0 ? (uint32_t)stipple_pattern : 0x3E0F83E0;
    m_force_microcheck = GetSetting(Set_force_microcheck) != 0;
    m_force_quad3d = GetSetting(Set_force_quad3d) != 0;
    m_clip_zmin = GetSetting(Set_clip_zmin) != 0;
    m_clip_zmax = GetSetting(Set_clip_zmax) != 0;
    m_fast_crc = GetSetting(Set_fast_crc) != 0;
    m_adjust_aspect = GetSetting(Set_adjust_aspect);
    m_zmode_compare_less = GetSetting(Set_zmode_compare_less);
    m_old_style_adither = GetSetting(Set_old_style_adither);
    m_n64_z_scale = GetSetting(Set_n64_z_scale) != 0;

    m_ScreenRes = GetSetting(Set_Resolution);
    if (m_ScreenRes >= GetScreenResolutionCount()) { m_ScreenRes = GetDefaultScreenRes(); }

    //frame buffer
    short fb_Settings[] =
    {
        g_romopen ? Set_optimize_texrect : Set_optimize_texrect_default,
        Set_ignore_aux_copy,
        Set_hires_buf_clear,
        Set_fb_read_alpha,
        Set_useless_is_useless,
        g_romopen ? Set_fb_smart : Set_fb_smart_default,
        g_romopen ? Set_fb_hires : Set_fb_hires_default,
        g_romopen ? Set_fb_read_always : Set_fb_read_always_default,
        g_romopen ? Set_detect_cpu_write : Set_detect_cpu_write_default,
        g_romopen ? Set_fb_get_info : Set_fb_get_info_default,
        g_romopen ? Set_fb_render : Set_fb_render_default
    };

    fb_bits_t bits[] =
    {
        fb_optimize_texrect,
        fb_ignore_aux_copy,
        fb_hwfbe_buf_clear,
        fb_read_alpha,
        fb_useless_is_useless,
        fb_emulation,
        fb_hwfbe,
        fb_ref,
        fb_cpu_write_hack,
        fb_get_info,
        fb_depth_render
    };

    uint32_t fb_add_bits = 0, fb_remove_bits = 0;
    for (int i = 0; i < (sizeof(fb_Settings) / sizeof(fb_Settings[0])); i++)
    {
        if (GetSetting(fb_Settings[i]) != 0)
        {
            fb_add_bits |= bits[i];
        }
        else
        {
            fb_remove_bits |= bits[i];
        }
    }
    fb_add_bits |= fb_motionblur;

    int read_back_to_screen = GetSetting(g_romopen ? Set_read_back_to_screen : Set_read_back_to_screen_default);
    if (read_back_to_screen == 1) { fb_add_bits |= fb_read_back_to_screen; }
    else if (read_back_to_screen == 2) { fb_add_bits |= fb_read_back_to_screen2; }
    else if (read_back_to_screen == 0) { fb_remove_bits |= fb_read_back_to_screen | fb_read_back_to_screen2; }

    UpdateFrameBufferBits(fb_add_bits, fb_remove_bits);
    m_fb_crc_mode = (FBCRCMODE_t)GetSetting(Set_fb_crc_mode);

    SetFiltering((Filtering_t)GetSetting(g_romopen ? Set_filtering : Set_filtering_default));
    SetFog(GetSetting(g_romopen ? Set_fog : Set_fog_default) != 0);
    SetBuffClear(GetSetting(g_romopen ? Set_buff_clear : Set_buff_clear_default) != 0);
    SetSwapMode((SwapMode_t)GetSetting(g_romopen ? Set_swapmode : Set_swapmode_default));
    SetAspectmode((AspectMode_t)GetSetting(g_romopen ? Set_aspect : Set_aspect_default));
    SetLODmode((PixelLevelOfDetail_t)GetSetting(g_romopen ? Set_lodmode : Set_lodmode_default));
    m_flame_corona = hacks(hack_Zelda) && !fb_depth_render_enabled();
    m_aspectmode = (AspectMode_t)GetSetting(g_romopen ? Set_aspect : Set_aspect_default);
}

void CSettings::WriteSettings(void)
{
    m_InWriteSettings = true;
    SetSetting(Set_Resolution, m_ScreenRes);
#ifndef ANDROID
    SetSetting(Set_FullScreenRes, m_FullScreenRes);
#endif
    SetSetting(Set_vsync, m_vsync ? 1 : 0);
    SetSetting(Set_Rotate, m_rotate);
    SetSetting(Set_texenh_options, m_texenh_options);

    SetSetting(Set_wrpVRAM, m_wrpVRAM);
    SetSetting(Set_wrpFBO, m_wrpFBO);
    SetSetting(Set_wrpAnisotropic, m_wrpAnisotropic);
    SetSetting(Set_autodetect_ucode, m_autodetect_ucode);

    SetSetting(Set_wireframe, m_wireframe);
    SetSetting(Set_wfmode, m_wfmode);
    SetSetting(Set_ucode, (int)m_ucode);

    SetSetting(Set_ghq_fltr, m_ghq_fltr);
    SetSetting(Set_ghq_cmpr, m_ghq_cmpr);
    SetSetting(Set_ghq_enht, m_ghq_enht);
    SetSetting(Set_ghq_hirs, m_ghq_hirs);
    SetSetting(Set_ghq_enht_cmpr, m_ghq_enht_cmpr);
    SetSetting(Set_ghq_enht_f16bpp, m_ghq_enht_f16bpp);
    SetSetting(Set_ghq_enht_gz, m_ghq_enht_gz);
    SetSetting(Set_ghq_enht_nobg, m_ghq_enht_nobg);
    SetSetting(Set_ghq_hirs_cmpr, m_ghq_hirs_cmpr);
    SetSetting(Set_ghq_hirs_tile, m_ghq_hirs_tile);
    SetSetting(Set_ghq_hirs_f16bpp, m_ghq_hirs_f16bpp);
    SetSetting(Set_ghq_hirs_gz, m_ghq_hirs_gz);
    SetSetting(Set_ghq_hirs_altcrc, m_ghq_hirs_altcrc);
    SetSetting(Set_ghq_cache_save, m_ghq_cache_save);
    SetSetting(Set_ghq_cache_size, m_ghq_cache_size);
    SetSetting(Set_ghq_hirs_let_texartists_fly, m_ghq_hirs_let_texartists_fly);
    SetSetting(Set_ghq_hirs_dump, m_ghq_hirs_dump);

    SetSetting(g_romopen ? Set_filtering : Set_filtering_default, m_filtering);
    SetSetting(g_romopen ? Set_fog : Set_fog_default, m_fog);
    SetSetting(g_romopen ? Set_buff_clear : Set_buff_clear_default, m_buff_clear);
    SetSetting(g_romopen ? Set_swapmode : Set_swapmode_default, m_swapmode);
    SetSetting(g_romopen ? Set_lodmode : Set_lodmode_default, m_lodmode);
    SetSetting(g_romopen ? Set_aspect : Set_aspect_default, m_aspectmode);

    SetSetting(g_romopen ? Set_fb_read_always : Set_fb_read_always_default, fb_ref_enabled() ? true : false);
    SetSetting(g_romopen ? Set_fb_smart : Set_fb_smart_default, fb_emulation_enabled() ? true : false);
    SetSetting(g_romopen ? Set_fb_hires : Set_fb_hires_default, fb_hwfbe_set() ? true : false);
    SetSetting(g_romopen ? Set_fb_get_info : Set_fb_get_info_default, fb_get_info_enabled() ? true : false);
    SetSetting(g_romopen ? Set_fb_render : Set_fb_render_default, fb_depth_render_enabled() ? true : false);
    SetSetting(g_romopen ? Set_detect_cpu_write : Set_detect_cpu_write_default, fb_cpu_write_hack_enabled() ? true : false);
    if (fb_read_back_to_screen_enabled())
    {
        SetSetting(g_romopen ? Set_read_back_to_screen : Set_read_back_to_screen_default, 1);
    }
    else if (fb_read_back_to_screen2_enabled())
    {
        SetSetting(g_romopen ? Set_read_back_to_screen : Set_read_back_to_screen_default, 2);
    }
    else
    {
        SetSetting(g_romopen ? Set_read_back_to_screen : Set_read_back_to_screen_default, 0);
    }

    FlushSettings();
    m_InWriteSettings = false;
}

void CSettings::general_setting(short setting_ID, const char * name, unsigned int value)
{
    RegisterSetting(setting_ID, Data_DWORD_General, name, NULL, value, NULL);
}

void CSettings::game_setting(short setting_ID, const char * name, unsigned int value)
{
    RegisterSetting(setting_ID, Data_DWORD_Game, name, NULL, value, NULL);
}

void CSettings::game_setting_default(short setting_ID, const char * name, short default_setting)
{
    RegisterSetting2(setting_ID, Data_DWORD_Game, name, NULL, default_setting);
}

void CSettings::SettingsChanged(void)
{
    if (m_InWriteSettings)
    {
        return;
    }
    m_ScreenRes = GetSetting(Set_Resolution);
    m_aspectmode = (AspectMode_t)GetSetting(Set_aspect);
}

void CSettings::LogLevelChanged(void)
{
    g_ModuleLogLevel[TraceMD5] = GetSetting(Set_Logging_MD5);
    g_ModuleLogLevel[TraceThread] = GetSetting(Set_Logging_Thread);
    g_ModuleLogLevel[TracePath] = GetSetting(Set_Logging_Path);
    g_ModuleLogLevel[TraceSettings] = GetSetting(Set_Logging_Settings);
    g_ModuleLogLevel[TraceUnknown] = GetSetting(Set_Logging_Unknown);
    g_ModuleLogLevel[TraceGlide64] = GetSetting(Set_Logging_Glide64);
    g_ModuleLogLevel[TraceInterface] = GetSetting(Set_Logging_Interface);
    g_ModuleLogLevel[TraceResolution] = GetSetting(Set_Logging_Resolution);
    g_ModuleLogLevel[TraceGlitch] = GetSetting(Set_Logging_Glitch);
    g_ModuleLogLevel[TraceRDP] = GetSetting(Set_Logging_VideoRDP);
    g_ModuleLogLevel[TraceTLUT] = GetSetting(Set_Logging_TLUT);
    g_ModuleLogLevel[TracePNG] = GetSetting(Set_Logging_PNG);
    g_ModuleLogLevel[TraceOGLWrapper] = GetSetting(Set_Logging_OGLWrapper);
    g_ModuleLogLevel[TraceRDPCommands] = GetSetting(Set_Logging_RDPCommands);
}

#ifdef _WIN32
#include <Windows.h>
#endif
void UseUnregisteredSetting(int /*SettingID*/)
{
#ifdef _WIN32
    DebugBreak();
#endif
}