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
