#include "Gfx_1.3.h"

CSettings::CSettings() :
card_id(0),
res_x(640),
scr_res_x(640),
res_y(480),
scr_res_y(480),
#ifndef ANDROID
res_data(GR_RESOLUTION_640x480),
res_data_org(GR_RESOLUTION_640x480),
#endif
advanced_options(0),
texenh_options(0),
ssformat(0),
vsync(0),

clock(0),
clock_24_hr(0),
rotate(0),

filtering(0),
fog(0),
buff_clear(0),
swapmode(0),
lodmode(0),
aspectmode(0),
use_hotkeys(0),
#ifdef TEXTURE_FILTER
//Texture filtering options
texture_dir(""),
ghq_fltr(0),
ghq_enht(0),
ghq_cmpr(0),
ghq_hirs(0),
ghq_use(0),
ghq_enht_cmpr(0),
ghq_enht_tile(0),
ghq_enht_f16bpp(0),
ghq_enht_gz(0),
ghq_enht_nobg(0),
ghq_hirs_cmpr(0),
ghq_hirs_tile(0),
ghq_hirs_f16bpp(0),
ghq_hirs_gz(0),
ghq_hirs_altcrc(0),
ghq_cache_save(0),
ghq_cache_size(0),
ghq_hirs_let_texartists_fly(0),
ghq_hirs_dump(0),
#endif
autodetect_ucode(0),
ucode(0),
logging(0),
elogging(0),
log_clear(0),
run_in_window(0),
filter_cache(0),
unk_as_red(0),
log_unk(0),
unk_clear(0),
wireframe(0),
wfmode(0),

// Special fixes
offset_x(0),
offset_y(0),
scale_x(0),
scale_y(0),
fast_crc(0),
alt_tex_size(0),
use_sts1_only(0),
flame_corona(0), //hack for zeldas flame's corona
increase_texrect_edge(0), // add 1 to lower right corner coordinates of texrect
decrease_fillrect_edge(0), // sub 1 from lower right corner coordinates of fillrect
texture_correction(0), // enable perspective texture correction emulation. is on by default
stipple_mode(0), //used for dithered alpha emulation
stipple_pattern(0), //used for dithered alpha emulation
force_microcheck(0), //check microcode each frame, for mixed F3DEX-S2DEX games
force_quad3d(0), //force 0xb5 command to be quad, not line 3d
clip_zmin(0), //enable near z clipping
clip_zmax(0), //enable far plane clipping;
adjust_aspect(0), //adjust screen aspect for wide screen mode
force_calc_sphere(0), //use spheric mapping only, Ridge Racer 64
pal230(0),    //set special scale for PAL games
correct_viewport(0), //correct viewport values
zmode_compare_less(0), //force GR_CMP_LESS for zmode=0 (opaque)and zmode=1 (interpenetrating)
old_style_adither(0), //apply alpha dither regardless of alpha_dither_mode
n64_z_scale(0), //scale vertex z value before writing to depth buffer, as N64 does.

hacks(0),

//wrapper settings
#ifndef ANDROID
wrpResolution(0),
#endif
wrpVRAM(0),
wrpFBO(0),
wrpAnisotropic(0)
{
}

void ReadSettings()
{
    g_settings->card_id = GetSetting(Set_CardId);
#ifdef ANDROID
    g_settings->scr_res_x = g_settings->res_x = g_width;
    g_settings->scr_res_y = g_settings->res_y = g_height;
#else
    g_settings->res_data = (uint32_t)GetSetting(Set_Resolution);
    if (g_settings->res_data >= 24) g_settings->res_data = 12;
    g_settings->scr_res_x = g_settings->res_x = resolutions[g_settings->res_data][0];
    g_settings->scr_res_y = g_settings->res_y = resolutions[g_settings->res_data][1];
    g_settings->wrpResolution = GetSetting(Set_wrpResolution);
#endif
    g_settings->vsync = GetSetting(Set_vsync);
    g_settings->ssformat = (uint8_t)GetSetting(Set_ssformat);
    g_settings->clock = GetSetting(Set_clock);
    g_settings->clock_24_hr = GetSetting(Set_clock_24_hr);
    g_settings->rotate = GetSetting(Set_Rotate);
    g_settings->advanced_options = Set_basic_mode ? !GetSystemSetting(Set_basic_mode) : 0;
    g_settings->texenh_options = GetSetting(Set_texenh_options);
    g_settings->use_hotkeys = GetSetting(Set_hotkeys);

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
}
