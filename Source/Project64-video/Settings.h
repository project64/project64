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
#include <Project64-video/Renderer/types.h>
#include <string>

class CSettings
{
public:
    CSettings();
    ~CSettings();

    //Frame buffer emulation options
    enum fb_bits_t
    {
        fb_emulation = (1 << 0),              //frame buffer emulation
        fb_hwfbe = (1 << 1),                  //hardware frame buffer emualtion
        fb_motionblur = (1 << 2),             //emulate motion blur
        fb_ref = (1 << 3),                    //read every frame
        fb_read_alpha = (1 << 4),             //read alpha
        fb_hwfbe_buf_clear = (1 << 5),        //clear auxiliary texture frame buffers
        fb_depth_render = (1 << 6),           //enable software depth render
        fb_optimize_texrect = (1 << 7),       //fast texrect rendering with hwfbe
        fb_ignore_aux_copy = (1 << 8),        //do not copy auxiliary frame buffers
        fb_useless_is_useless = (1 << 10),    //
        fb_get_info = (1 << 11),              //get frame buffer info
        fb_read_back_to_screen = (1 << 12),   //render N64 frame buffer to screen
        fb_read_back_to_screen2 = (1 << 13),  //render N64 frame buffer to screen
        fb_cpu_write_hack = (1 << 14),        //show images writed directly by CPU
    };

    enum hacks_t
    {
        hack_ASB = (1 << 0),         //All-Star Baseball games
        hack_Banjo2 = (1 << 1),      //Banjo Tooie
        hack_BAR = (1 << 2),         //Beetle Adventure Racing
        hack_Chopper = (1 << 3),     //Chopper Attack
        hack_Diddy = (1 << 4),       //diddy kong racing
        hack_Fifa98 = (1 << 5),      //FIFA - Road to World Cup 98
        hack_Fzero = (1 << 6),       //F-Zero
        hack_GoldenEye = (1 << 7),   //Golden Eye
        hack_Hyperbike = (1 << 8),   //Top Gear Hyper Bike
        hack_ISS64 = (1 << 9),       //International Superstar Soccer 64
        hack_KI = (1 << 10),         //Killer Instinct
        hack_Knockout = (1 << 11),   //Knockout Kings 2000
        hack_Lego = (1 << 12),       //LEGO Racers
        hack_MK64 = (1 << 13),       //Mario Kart
        hack_Megaman = (1 << 14),    //Megaman64
        hack_Makers = (1 << 15),     //Mischief-makers
        hack_WCWnitro = (1 << 16),   //WCW Nitro
        hack_Ogre64 = (1 << 17),     //Ogre Battle 64
        hack_Pilotwings = (1 << 18), //Pilotwings
        hack_PMario = (1 << 19),     //Paper Mario
        hack_PPL = (1 << 20),        //pokemon puzzle league requires many special fixes
        hack_RE2 = (1 << 21),        //Resident Evil 2
        hack_Starcraft = (1 << 22),  //StarCraft64
        hack_Supercross = (1 << 23), //Supercross 2000
        hack_TGR = (1 << 24),        //Top Gear Rally
        hack_TGR2 = (1 << 25),       //Top Gear Rally 2
        hack_Tonic = (1 << 26),      //tonic trouble
        hack_Winback = (1 << 27),    //WinBack - Covert Operations
        hack_Yoshi = (1 << 28),      //Yoshi Story
        hack_Zelda = (1 << 29),      //zeldas hacks
        hack_OoT = (1 << 30),        //zelda OoT hacks
    };

    enum AspectMode_t
    {
        Aspect_4x3 = 0,
        Aspect_16x9 = 1,
        Aspect_Stretch = 2,
        Aspect_Original = 3,
    };

    enum ScreenRotate_t
    {
        Rotate_None = 0,
        Rotate_90 = 1,
        Rotate_180 = 2,
        Rotate_270 = 3,
    };

    enum Filtering_t
    {
        Filter_Automatic = 0,
        Filter_ForceBilinear = 1,
        Filter_ForcePointSampled = 2,
    };

    enum TextureFilter_t
    {
        TextureFilter_None = 0x00,
        TextureFilter_SmoothFiltering = 0x01,
        TextureFilter_SmoothFiltering2 = 0x02,
        TextureFilter_SmoothFiltering3 = 0x03,
        TextureFilter_SmoothFiltering4 = 0x04,
        TextureFilter_SharpFiltering1 = 0x10,
        TextureFilter_SharpFiltering2 = 0x20,
    };

    enum TextureEnhancement_t
    {
        TextureEnht_None = 0x00,
        TextureEnht_X2 = 0x100,
        TextureEnht_X2SAI = 0x200,
        TextureEnht_HQ2X = 0x300,
        TextureEnht_HQ2XS = 0x600,
        TextureEnht_LQ2X = 0x400,
        TextureEnht_LQ2XS = 0x700,
        TextureEnht_HQ4X = 0x500,
    };

    enum TextureCompression_t
    {
        TextureCompression_S3TC = 0x3000,
        TextureCompression_FXT1 = 0x1000,
    };

    enum HiResPackFormat_t
    {
        HiResPackFormat_None = 0,
        HiResPackFormat_Riceformat = 0x00020000,
    };

    enum SwapMode_t
    {
        SwapMode_Old = 0,
        SwapMode_New = 1,
        SwapMode_Hybrid = 2,
    };

    enum PixelLevelOfDetail_t
    {
        LOD_Off = 0,
        LOD_Fast = 1,
        LOD_Precise = 2,
    };

    enum ucode_t
    {
        uCode_NotFound = -2,
        uCode_Unsupported = -1,
        ucode_Fast3D = 0,
        ucode_F3DEX = 1,
        ucode_F3DEX2 = 2,
        ucode_WaveRace = 3,
        ucode_StarWars = 4,
        ucode_DiddyKong = 5,
        ucode_S2DEX = 6,
        ucode_PerfectDark = 7,
        ucode_CBFD = 8,
        ucode_zSort = 9,
        ucode_F3DTEXA = 10,
        ucode_Turbo3d = 21,
    };

    enum wfmode_t
    {
        wfmode_NormalColors = 0,
        wfmode_VertexColors = 1,
        wfmode_RedOnly = 2,
    };

    enum FBCRCMODE_t
    {
        fbcrcNone = 0,
        fbcrcFast = 1,
        fbcrcSafe = 2
    };

    inline bool fb_emulation_enabled(void) const { return ((m_frame_buffer&fb_emulation) != 0); }
    inline bool fb_ref_enabled(void) const { return ((m_frame_buffer&fb_ref) != 0); }
    inline bool fb_hwfbe_enabled(void) const { return ((m_frame_buffer&(fb_emulation | fb_hwfbe)) == (fb_emulation | fb_hwfbe)); }
    inline bool fb_hwfbe_set(void) const { return ((m_frame_buffer&fb_hwfbe) != 0); }
    inline bool fb_depth_render_enabled(void) const { return ((m_frame_buffer&fb_depth_render) != 0); }
    inline bool fb_get_info_enabled(void) const { return ((m_frame_buffer&fb_get_info) != 0); }
    inline bool fb_read_back_to_screen_enabled(void) const { return ((m_frame_buffer&fb_read_back_to_screen) != 0); }
    inline bool fb_read_back_to_screen2_enabled(void) const { return ((m_frame_buffer&fb_read_back_to_screen2) != 0); }
    inline bool fb_cpu_write_hack_enabled(void) const { return ((m_frame_buffer&fb_cpu_write_hack) != 0); }
    inline bool fb_ignore_aux_copy_enabled(void) const { return ((m_frame_buffer&fb_ignore_aux_copy) != 0); }
    inline bool fb_hwfbe_buf_clear_enabled(void) const { return ((m_frame_buffer&fb_hwfbe_buf_clear) != 0); }
    inline bool fb_useless_is_useless_enabled(void) const { return ((m_frame_buffer&fb_useless_is_useless) != 0); }
    inline bool fb_motionblur_enabled(void) const { return ((m_frame_buffer&fb_motionblur) != 0); }
    inline bool fb_read_alpha_enabled(void) const { return ((m_frame_buffer&fb_read_alpha) != 0); }
    inline bool fb_optimize_texrect_enabled(void) const { return ((m_frame_buffer&fb_optimize_texrect) != 0); }

    inline const char * log_dir(void) const { return m_log_dir; }
    inline uint32_t ScreenRes(void) const { return m_ScreenRes; }
    inline bool advanced_options(void) const { return m_advanced_options; }
    inline bool debugger_enabled(void) const { return m_debugger_enabled; }
    inline bool texenh_options(void) const { return m_texenh_options; }
    inline bool vsync(void) const { return m_vsync; }
    inline ScreenRotate_t rotate(void) const { return m_rotate; }
    inline Filtering_t filtering(void) const { return m_filtering; }

    inline bool fog(void) const { return m_fog; }
    inline bool buff_clear(void) const { return m_buff_clear; }
    inline SwapMode_t swapmode(void) const { return m_swapmode; }
    inline PixelLevelOfDetail_t lodmode(void) const { return m_lodmode; }
    inline AspectMode_t aspectmode(void) const { return m_aspectmode; }

    inline FBCRCMODE_t fb_crc_mode(void) const { return m_fb_crc_mode; }

    //Texture filtering options
    inline const char * texture_dir(void) const { return m_texture_dir.c_str(); }
    inline TextureFilter_t ghq_fltr(void) const { return m_ghq_fltr; }
    inline TextureEnhancement_t ghq_enht(void) const { return m_ghq_enht; }
    inline TextureCompression_t ghq_cmpr(void) const { return m_ghq_cmpr; }
    inline HiResPackFormat_t ghq_hirs(void) const { return m_ghq_hirs; }
    inline bool ghq_enht_cmpr(void) const { return m_ghq_enht_cmpr; }
    inline bool ghq_enht_f16bpp(void) const { return m_ghq_enht_f16bpp; }
    inline bool ghq_enht_gz(void) const { return m_ghq_enht_gz; }
    inline bool ghq_enht_nobg(void) const { return m_ghq_enht_nobg; }
    inline bool ghq_hirs_cmpr(void) const { return m_ghq_hirs_cmpr; }
    inline bool ghq_hirs_tile(void) const { return m_ghq_hirs_tile; }
    inline bool ghq_hirs_f16bpp(void) const { return m_ghq_hirs_f16bpp; }
    inline bool ghq_hirs_gz(void) const { return m_ghq_hirs_gz; }
    inline bool ghq_hirs_altcrc(void) const { return m_ghq_hirs_altcrc; }
    inline bool ghq_cache_save(void) const { return m_ghq_cache_save; }
    inline int ghq_cache_size(void) const { return m_ghq_cache_size; }
    inline bool ghq_hirs_let_texartists_fly(void) const { return m_ghq_hirs_let_texartists_fly; }
    inline bool ghq_hirs_dump(void) const { return m_ghq_hirs_dump; }

    //Debug
    inline bool autodetect_ucode(void) const { return m_autodetect_ucode; }
    inline ucode_t ucode(void) const { return m_ucode; }
    inline bool unk_as_red(void) const { return m_unk_as_red; }
    inline bool wireframe(void) const { return m_wireframe; }
    inline wfmode_t wfmode(void) const { return m_wfmode; }

    // Special fixes
    inline bool fast_crc(void) const { return m_fast_crc; }
    inline bool alt_tex_size(void) const { return m_alt_tex_size; }
    inline bool use_sts1_only(void) const { return m_use_sts1_only; }
    inline bool flame_corona(void) const { return m_flame_corona; } //hack for zeldas flame's corona
    inline bool increase_texrect_edge(void) const { return m_increase_texrect_edge; }  // add 1 to lower right corner coordinates of texrect
    inline bool decrease_fillrect_edge(void) const { return m_decrease_fillrect_edge; }; // sub 1 from lower right corner coordinates of fillrect
    inline bool texture_correction(void) const { return m_texture_correction; } // enable perspective texture correction emulation. is on by default
    inline gfxStippleMode_t stipple_mode(void) const { return m_stipple_mode; } //used for dithered alpha emulation
    inline uint32_t stipple_pattern(void) const { return m_stipple_pattern; } //used for dithered alpha emulation
    inline bool force_microcheck(void) const { return m_force_microcheck; } //check microcode each frame, for mixed F3DEX-S2DEX games
    inline bool force_quad3d(void) const { return m_force_quad3d; } //force 0xb5 command to be quad, not line 3d
    inline bool clip_zmin(void) const { return m_clip_zmin; } //enable near z clipping
    inline bool clip_zmax(void) const { return m_clip_zmax; } //enable far plane clipping
    inline bool adjust_aspect(void) const { return m_adjust_aspect; } //adjust screen aspect for wide screen mode
    inline bool force_calc_sphere(void) const { return m_force_calc_sphere; } //use spheric mapping only, Ridge Racer 64
    inline bool pal230(void) const { return m_pal230; } //use spheric mapping only, Ridge Racer 64
    inline bool correct_viewport(void) const { return m_correct_viewport; } //correct viewport values
    inline bool zmode_compare_less(void) const { return m_zmode_compare_less; } //force GFX_CMP_LESS for zmode=0 (opaque)and zmode=1 (interpenetrating)
    inline bool old_style_adither(void) const { return m_old_style_adither; } //apply alpha dither regardless of alpha_dither_mode
    inline bool n64_z_scale(void) const { return m_n64_z_scale; } //scale vertex z value before writing to depth buffer, as N64 does.

    inline bool hacks(hacks_t hack) const { return (m_hacks & hack) == hack; } //Special game hacks

    inline bool dirty(void) { return m_dirty; }

    //wrapper settings
#ifndef ANDROID
    inline uint32_t FullScreenRes(void) const { return m_FullScreenRes; }
#endif
    inline uint32_t RdramSize(void) const { return m_RdramSize; }
    inline int wrpVRAM(void) const { return m_wrpVRAM; }
    inline bool wrpFBO(void) const { return m_wrpFBO; }
    inline bool wrpAnisotropic(void) const { return m_wrpAnisotropic; }
    inline bool FlushLogs(void) const { return m_FlushLogs; }

    void SetTexenhOptions(bool value);
    void SetScreenRes(uint32_t value);
    void SetAspectmode(AspectMode_t value);
    void SetLODmode(PixelLevelOfDetail_t value);
    void SetVsync(bool value);
    void SetFiltering(Filtering_t value);
    void SetSwapMode(SwapMode_t value);
    void SetFog(bool value);
    void SetBuffClear(bool value);
    void SetWrpAnisotropic(bool value);
    void SetWrpVRAM(int value);
    void SetWrpFBO(bool value);
    void SetGhqFltr(TextureFilter_t value);
    void SetGhqEnht(TextureEnhancement_t value);
    void SetGhqCmpr(TextureCompression_t value);
    void SetGhqHirs(HiResPackFormat_t value);
    void SetGhqEnhtGz(bool value);
    void SetGhqHirsTile(bool value);
    void SetGhqHirsF16bpp(bool value);
    void SetGhqHirsDump(bool value);
    void SetGhqEnhtNobg(bool value);
    void SetGhqEnhtCmpr(bool value);
    void SetGhqHirsAltcrc(bool value);
    void SetGhqHirsCmpr(bool value);
    void SetGhqHirsGz(bool value);
    void SetGhqCacheSave(bool value);
    void SetGhqHirsLetTexartistsFly(bool value);
    void SetGhqCacheSize(int value);
    void UpdateFrameBufferBits(uint32_t BitsToAdd, uint32_t BitsToRemove);
    ucode_t DetectUCode(uint32_t uc_crc);
    void SetUcode(ucode_t value);
#ifndef ANDROID
    void SetFullScreenRes(uint32_t value);
#endif
    void ReadSettings();
    void ReadGameSettings(const char * name);
    void WriteSettings(void);

private:
    static void general_setting(short setting_ID, const char * name, unsigned int value);
    static void game_setting(short setting_ID, const char * name, unsigned int value);
    static void game_setting_default(short setting_ID, const char * name, short default_setting);

    void RegisterSettings(void);
    void SettingsChanged(void);
    void LogLevelChanged(void);

    static void stSettingsChanged(void * _this)
    {
        ((CSettings *)_this)->SettingsChanged();
    }

    static void stLogLevelChanged(void * _this)
    {
        ((CSettings *)_this)->LogLevelChanged();
    }

    short m_Set_basic_mode;
    short m_Set_debugger;
    short m_Set_RDRamSize;
    short m_Set_texture_dir;
    short m_Set_log_dir;
    short m_Set_log_flush;

    bool m_dirty;
#ifndef ANDROID
    uint32_t m_FullScreenRes;
#endif
    uint32_t m_RdramSize;
    int m_wrpVRAM;
    bool m_wrpFBO;
    bool m_wrpAnisotropic;
    bool m_FlushLogs;
    char m_log_dir[260];
    uint32_t m_ScreenRes;
    AspectMode_t m_aspectmode;
    uint32_t m_frame_buffer;
    FBCRCMODE_t m_fb_crc_mode;
    ScreenRotate_t m_rotate;
    Filtering_t m_filtering;
    bool m_fog;
    bool m_buff_clear;
    SwapMode_t m_swapmode;
    PixelLevelOfDetail_t m_lodmode;
    bool m_advanced_options;
    bool m_debugger_enabled;
    bool m_texenh_options;
    bool m_vsync;
    std::string m_texture_dir;
    TextureFilter_t m_ghq_fltr;
    TextureEnhancement_t m_ghq_enht;
    TextureCompression_t m_ghq_cmpr;
    HiResPackFormat_t m_ghq_hirs;
    bool m_ghq_enht_cmpr;
    bool m_ghq_enht_f16bpp;
    bool m_ghq_enht_gz;
    bool m_ghq_enht_nobg;
    bool m_ghq_hirs_cmpr;
    bool m_ghq_hirs_tile;
    bool m_ghq_hirs_f16bpp;
    bool m_ghq_hirs_gz;
    bool m_ghq_hirs_altcrc;
    bool m_ghq_cache_save;
    int m_ghq_cache_size;
    bool m_ghq_hirs_let_texartists_fly;
    bool m_ghq_hirs_dump;
    bool m_autodetect_ucode;
    bool m_unk_as_red;
    bool m_wireframe;
    wfmode_t m_wfmode;
    ucode_t m_ucode;
    bool m_fast_crc;
    bool m_alt_tex_size;
    bool m_use_sts1_only;
    bool m_flame_corona;
    bool m_increase_texrect_edge;
    bool m_decrease_fillrect_edge;
    bool m_texture_correction;
    gfxStippleMode_t m_stipple_mode;
    uint32_t m_stipple_pattern;
    bool m_force_microcheck;
    bool m_force_quad3d;
    bool m_clip_zmin;
    bool m_clip_zmax;
    bool m_adjust_aspect;
    bool m_force_calc_sphere;
    bool m_pal230;
    bool m_correct_viewport;
    bool m_zmode_compare_less;
    bool m_old_style_adither;
    bool m_n64_z_scale;
    hacks_t m_hacks;
    bool m_InWriteSettings;
};

extern CSettings * g_settings;
