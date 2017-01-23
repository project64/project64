#include "Gfx_1.3.h"
#include "SettingsID.h"

#ifdef _WIN32
int GetCurrentResIndex(void);
#endif

short Set_basic_mode = 0, Set_texture_dir = 0, Set_log_dir = 0, Set_log_flush = 0;

CSettings::CSettings() :
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
wrpAnisotropic(0),
m_FlushLogs(false)
{
    memset(m_log_dir, 0, sizeof(m_log_dir));
    RegisterSettings();
    ReadSettings();
}

void CSettings::RegisterSettings(void)
{
    SetModuleName("default");
    Set_basic_mode = FindSystemSettingId("Basic Mode");
    Set_texture_dir = FindSystemSettingId("Dir:Texture");
    Set_log_flush = FindSystemSettingId("Log Auto Flush");
    Set_log_dir = FindSystemSettingId("Dir:Log");

    SetModuleName("Glide64");
#ifdef _WIN32
    general_setting(Set_Resolution, "resolution", 7);
    general_setting(Set_wrpResolution, "wrpResolution", GetCurrentResIndex());
#endif
    general_setting(Set_vsync, "vsync", 1);
    general_setting(Set_ssformat, "ssformat", 1);
    general_setting(Set_clock, "clock", 0);
    general_setting(Set_clock_24_hr, "clock_24_hr", 0);
    general_setting(Set_texenh_options, "texenh_options", 0);
    general_setting(Set_hotkeys, "hotkeys", 1);
    general_setting(Set_wrpVRAM, "wrpVRAM", 0);
#ifndef ANDROID
    general_setting(Set_wrpFBO, "wrpFBO", 0);
#else
    general_setting(Set_wrpFBO, "wrpFBO", 1);
#endif
    general_setting(Set_Rotate, "rotate", 0);
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

    general_setting(Set_optimize_texrect_default, "optimize_texrect", 1);
    general_setting(Set_filtering_default, "filtering", 0);
    general_setting(Set_lodmode_default, "lodmode", 0);
    general_setting(Set_fog_default, "fog", 1);
    general_setting(Set_buff_clear_default, "buff_clear", 1);
    general_setting(Set_swapmode_default, "swapmode", 1);
    general_setting(Set_aspect_default, "aspect", 0);

    general_setting(Set_fb_smart_default, "fb_smart", 1);
    general_setting(Set_fb_hires_default, "fb_hires", 1);
    general_setting(Set_fb_read_always_default, "fb_read_always", 0);
    general_setting(Set_read_back_to_screen_default, "read_back_to_screen", 0);
    general_setting(Set_detect_cpu_write_default, "detect_cpu_write", 0);
    general_setting(Set_fb_get_info_default, "fb_get_info", 0);
    general_setting(Set_fb_render_default, "fb_render", 0);

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
    game_setting_default(Set_optimize_texrect, "optimize_texrect", Set_optimize_texrect_default);
    game_setting(Set_ignore_aux_copy, "ignore_aux_copy", (unsigned int)-1);
    game_setting(Set_hires_buf_clear, "hires_buf_clear", 1);
    game_setting(Set_fb_read_alpha, "fb_read_alpha", 0);
    game_setting(Set_useless_is_useless, "useless_is_useless", (unsigned int)-1);
    game_setting(Set_fb_crc_mode, "fb_crc_mode", 1);
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
}

void CSettings::ReadSettings()
{
#ifdef ANDROID
    this->scr_res_x = this->res_x = g_width;
    this->scr_res_y = this->res_y = g_height;
#else
    this->res_data = (uint32_t)GetSetting(Set_Resolution);
    if (this->res_data >= 24) this->res_data = 12;
    this->scr_res_x = this->res_x = resolutions[this->res_data][0];
    this->scr_res_y = this->res_y = resolutions[this->res_data][1];
    this->wrpResolution = GetSetting(Set_wrpResolution);
#endif
    this->vsync = GetSetting(Set_vsync);
    this->ssformat = (uint8_t)GetSetting(Set_ssformat);
    this->clock = GetSetting(Set_clock);
    this->clock_24_hr = GetSetting(Set_clock_24_hr);
    this->rotate = GetSetting(Set_Rotate);
    this->advanced_options = Set_basic_mode ? !GetSystemSetting(Set_basic_mode) : 0;
    this->texenh_options = GetSetting(Set_texenh_options);
    this->use_hotkeys = GetSetting(Set_hotkeys);

    this->wrpVRAM = GetSetting(Set_wrpVRAM);
    this->wrpFBO = GetSetting(Set_wrpFBO);
    this->wrpAnisotropic = GetSetting(Set_wrpAnisotropic);

#ifndef _ENDUSER_RELEASE_
    this->autodetect_ucode = GetSetting(Set_autodetect_ucode);
    this->ucode = GetSetting(Set_ucode);
    this->wireframe = GetSetting(Set_wireframe);
    this->wfmode = GetSetting(Set_wfmode);
    this->logging = GetSetting(Set_logging);
    this->log_clear = GetSetting(Set_log_clear);
    this->run_in_window = GetSetting(Set_run_in_window);
    this->elogging = GetSetting(Set_elogging);
    this->filter_cache = GetSetting(Set_filter_cache);
    this->unk_as_red = GetSetting(Set_unk_as_red);
    this->log_unk = GetSetting(Set_log_unk);
    this->unk_clear = GetSetting(Set_unk_clear);
#else
    this->autodetect_ucode = TRUE;
    this->ucode = 2;
    this->wireframe = FALSE;
    this->wfmode = 0;
    this->logging = FALSE;
    this->log_clear = FALSE;
    this->run_in_window = FALSE;
    this->elogging = FALSE;
    this->filter_cache = FALSE;
    this->unk_as_red = FALSE;
    this->log_unk = FALSE;
    this->unk_clear = FALSE;
#endif

    char texture_dir[260];
    memset(texture_dir, 0, sizeof(texture_dir));
    GetSystemSettingSz(Set_texture_dir, texture_dir, sizeof(texture_dir));
    this->texture_dir = texture_dir;
    this->ghq_fltr = (uint8_t)GetSetting(Set_ghq_fltr);
    this->ghq_cmpr = (uint8_t)GetSetting(Set_ghq_cmpr);
    this->ghq_enht = (uint8_t)GetSetting(Set_ghq_enht);
    this->ghq_hirs = (uint8_t)GetSetting(Set_ghq_hirs);
    this->ghq_enht_cmpr = GetSetting(Set_ghq_enht_cmpr);
    this->ghq_enht_tile = GetSetting(Set_ghq_enht_tile);
    this->ghq_enht_f16bpp = GetSetting(Set_ghq_enht_f16bpp);
    this->ghq_enht_gz = GetSetting(Set_ghq_enht_gz);
    this->ghq_enht_nobg = GetSetting(Set_ghq_enht_nobg);
    this->ghq_hirs_cmpr = GetSetting(Set_ghq_hirs_cmpr);
    this->ghq_hirs_tile = GetSetting(Set_ghq_hirs_tile);
    this->ghq_hirs_f16bpp = GetSetting(Set_ghq_hirs_f16bpp);
    this->ghq_hirs_gz = GetSetting(Set_ghq_hirs_gz);
    this->ghq_hirs_altcrc = GetSetting(Set_ghq_hirs_altcrc);
    this->ghq_cache_save = GetSetting(Set_ghq_cache_save);
    this->ghq_cache_size = GetSetting(Set_ghq_cache_size);
    this->ghq_hirs_let_texartists_fly = GetSetting(Set_ghq_hirs_let_texartists_fly);
    this->ghq_hirs_dump = GetSetting(Set_ghq_hirs_dump);

    if (Set_log_dir != 0)
    {
        GetSystemSettingSz(Set_log_dir, m_log_dir, sizeof(m_log_dir));
    }
    m_FlushLogs = Set_log_flush != 0 ? GetSystemSetting(Set_log_flush) != 0 : false;
}

void ReadSpecialSettings(const char * name)
{
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
	else if (strstr(name, (const char *)"WIN BACK") || strstr(name, (const char *)"OPERATION WINBACK"))
		g_settings->hacks |= hack_Winback;

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

    //frame buffer
    int optimize_texrect = GetSetting(g_romopen ? Set_optimize_texrect : Set_optimize_texrect_default);
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

    g_settings->filtering = GetSetting(g_romopen ? Set_filtering : Set_filtering_default);
    g_settings->fog = GetSetting(g_romopen ? Set_fog : Set_fog_default);
    g_settings->buff_clear = GetSetting(g_romopen ? Set_buff_clear : Set_buff_clear_default);
    g_settings->swapmode = GetSetting(g_romopen ? Set_swapmode : Set_swapmode_default);
    g_settings->aspectmode = GetSetting(g_romopen ? Set_aspect : Set_aspect_default);
    g_settings->lodmode = GetSetting(g_romopen ? Set_lodmode : Set_lodmode_default);
#ifdef _WIN32
    g_settings->res_data = GetSetting(Set_Resolution);
    if (g_settings->res_data < 0 || g_settings->res_data >= 0x18) g_settings->res_data = 12;
    g_settings->scr_res_x = g_settings->res_x = resolutions[g_settings->res_data][0];
    g_settings->scr_res_y = g_settings->res_y = resolutions[g_settings->res_data][1];
#endif

    //frame buffer
    int smart_read = GetSetting(g_romopen ? Set_fb_smart : Set_fb_smart_default);
    int hires = GetSetting(g_romopen ? Set_fb_hires : Set_fb_hires_default);
    int read_always = GetSetting(g_romopen ? Set_fb_read_always : Set_fb_read_always_default);
    int read_back_to_screen = GetSetting(g_romopen ? Set_read_back_to_screen : Set_read_back_to_screen_default);
    int cpu_write_hack = GetSetting(g_romopen ? Set_detect_cpu_write : Set_detect_cpu_write_default);
    int get_fbinfo = GetSetting(g_romopen ? Set_fb_get_info : Set_fb_get_info_default);
    int depth_render = GetSetting(g_romopen ? Set_fb_render : Set_fb_render_default);

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
    g_settings->flame_corona = (g_settings->hacks & hack_Zelda) && !fb_depth_render_enabled;
}

void WriteSettings(void)
{
#ifdef _WIN32
    SetSetting(Set_Resolution, (int)g_settings->res_data);
    SetSetting(Set_wrpResolution, g_settings->wrpResolution);
#endif
    SetSetting(Set_ssformat, g_settings->ssformat);
    SetSetting(Set_vsync, g_settings->vsync);
    SetSetting(Set_clock, g_settings->clock);
    SetSetting(Set_clock_24_hr, g_settings->clock_24_hr);
    SetSetting(Set_Rotate, g_settings->rotate);
    //SetSetting(Set_advanced_options,g_settings->advanced_options);
    SetSetting(Set_texenh_options, g_settings->texenh_options);

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

    SetSetting(g_romopen ? Set_filtering : Set_filtering_default, g_settings->filtering);
    SetSetting(g_romopen ? Set_fog : Set_fog_default, g_settings->fog);
    SetSetting(g_romopen ? Set_buff_clear : Set_buff_clear_default, g_settings->buff_clear);
    SetSetting(g_romopen ? Set_swapmode : Set_swapmode_default, g_settings->swapmode);
    SetSetting(g_romopen ? Set_lodmode : Set_lodmode_default, g_settings->lodmode);
    SetSetting(g_romopen ? Set_aspect : Set_aspect_default, g_settings->aspectmode);

    SetSetting(g_romopen ? Set_fb_read_always : Set_fb_read_always_default, g_settings->frame_buffer&fb_ref ? 1 : 0);
    SetSetting(g_romopen ? Set_fb_smart : Set_fb_smart_default, g_settings->frame_buffer & fb_emulation ? 1 : 0);
    SetSetting(g_romopen ? Set_fb_hires : Set_fb_hires_default, g_settings->frame_buffer & fb_hwfbe ? 1 : 0);
    SetSetting(g_romopen ? Set_fb_get_info : Set_fb_get_info_default, g_settings->frame_buffer & fb_get_info ? 1 : 0);
    SetSetting(g_romopen ? Set_fb_render : Set_fb_render_default, g_settings->frame_buffer & fb_depth_render ? 1 : 0);
    SetSetting(g_romopen ? Set_detect_cpu_write : Set_detect_cpu_write_default, g_settings->frame_buffer & fb_cpu_write_hack ? 1 : 0);
    if (g_settings->frame_buffer & fb_read_back_to_screen)
    {
        SetSetting(g_romopen ? Set_read_back_to_screen : Set_read_back_to_screen_default, 1);
    }
    else if (g_settings->frame_buffer & fb_read_back_to_screen2)
    {
        SetSetting(g_romopen ? Set_read_back_to_screen : Set_read_back_to_screen_default, 2);
    }
    else
    {
        SetSetting(g_romopen ? Set_read_back_to_screen : Set_read_back_to_screen_default, 0);
    }

    FlushSettings();
}

