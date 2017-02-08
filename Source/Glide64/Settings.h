#pragma once
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
        hack_ASB = (1<<0),         //All-Star Baseball games
        hack_Banjo2 = (1<<1),      //Banjo Tooie
        hack_BAR = (1<<2),         //Beetle Adventure Racing
        hack_Chopper = (1<<3),     //Chopper Attack
        hack_Diddy = (1<<4),       //diddy kong racing
        hack_Fifa98 = (1<<5),      //FIFA - Road to World Cup 98
        hack_Fzero = (1<<6),       //F-Zero
        hack_GoldenEye = (1<<7),   //Golden Eye
        hack_Hyperbike = (1<<8),   //Top Gear Hyper Bike
        hack_ISS64 = (1<<9),       //International Superstar Soccer 64
        hack_KI = (1<<10),         //Killer Instinct
        hack_Knockout = (1<<11),   //Knockout Kings 2000
        hack_Lego = (1<<12),       //LEGO Racers
        hack_MK64 = (1<<13),       //Mario Kart
        hack_Megaman = (1<<14),    //Megaman64
        hack_Makers = (1<<15),     //Mischief-makers
        hack_WCWnitro = (1<<16),   //WCW Nitro
        hack_Ogre64 = (1<<17),     //Ogre Battle 64
        hack_Pilotwings = (1<<18), //Pilotwings
        hack_PMario = (1<<19),     //Paper Mario
        hack_PPL = (1<<20),        //pokemon puzzle league requires many special fixes
        hack_RE2 = (1<<21),        //Resident Evil 2
        hack_Starcraft = (1<<22),  //StarCraft64
        hack_Supercross = (1<<23), //Supercross 2000
        hack_TGR = (1<<24),        //Top Gear Rally
        hack_TGR2 = (1<<25),       //Top Gear Rally 2
        hack_Tonic = (1<<26),      //tonic trouble
        hack_Winback = (1<<27),    //WinBack - Covert Operations
        hack_Yoshi = (1<<28),      //Yoshi Story
        hack_Zelda = (1<<29),      //zeldas hacks
        hack_OoT = (1<<30),        //zelda OoT hacks
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

    enum StippleMode_t
    {
        STIPPLE_Disable = 0x0,
        STIPPLE_Pattern = 0x1,
        STIPPLE_Rotate = 0x2,
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
        ucode_Turbo3d = 21,
    };


    int advanced_options;
    int texenh_options;
    int vsync;


    int fog;
    int buff_clear;


    enum FBCRCMODE_t
    {
        fbcrcNone = 0,
        fbcrcFast = 1,
        fbcrcSafe = 2
    };

    inline bool fb_emulation_enabled(void) const { return ((m_frame_buffer&fb_emulation) != 0); }
    inline bool fb_ref_enabled(void) const { return ((m_frame_buffer&fb_ref) != 0); }
    inline bool fb_hwfbe_enabled(void) const { return ((m_frame_buffer&(fb_emulation |fb_hwfbe)) == (fb_emulation | fb_hwfbe)); }
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
    inline uint32_t res_x(void) const { return m_res_x; }
    inline uint32_t res_y(void) const { return m_res_y; }
    inline uint32_t scr_res_x(void) const { return m_scr_res_x; }
    inline uint32_t scr_res_y(void) const { return m_scr_res_y; }
    inline uint32_t ScreenRes(void) const { return m_ScreenRes; }
    inline bool FlushLogs(void) const { return m_FlushLogs; }
    inline ScreenRotate_t rotate(void) const { return m_rotate; }
    inline Filtering_t filtering(void) const { return m_filtering; }

    inline SwapMode_t swapmode(void) const { return m_swapmode; }
    inline PixelLevelOfDetail_t lodmode(void) const { return m_lodmode; }
    inline AspectMode_t aspectmode(void) const { return m_aspectmode; }

    inline FBCRCMODE_t fb_crc_mode(void) const { return m_fb_crc_mode; }

    //Texture filtering options
    std::string texture_dir;
    inline TextureFilter_t ghq_fltr(void) const { return m_ghq_fltr; }
    inline TextureEnhancement_t ghq_enht(void) const { return m_ghq_enht; }
    int ghq_cmpr;
    int ghq_hirs;
    int ghq_use;
    int ghq_enht_cmpr;
    int ghq_enht_tile;
    int ghq_enht_f16bpp;
    int ghq_enht_gz;
    int ghq_enht_nobg;
    int ghq_hirs_cmpr;
    int ghq_hirs_tile;
    int ghq_hirs_f16bpp;
    int ghq_hirs_gz;
    int ghq_hirs_altcrc;
    int ghq_cache_save;
    int ghq_cache_size;
    int ghq_hirs_let_texartists_fly;
    int ghq_hirs_dump;

    //Debug
    int autodetect_ucode;
    inline ucode_t ucode(void) const { return m_ucode; }
    int unk_as_red;
    int unk_clear;
    int wireframe;
    int wfmode;

    // Special fixes
    int offset_x, offset_y;
    int scale_x, scale_y;
    int fast_crc;
    int alt_tex_size;
    int use_sts1_only;
    int flame_corona; //hack for zeldas flame's corona
    int increase_texrect_edge; // add 1 to lower right corner coordinates of texrect
    int decrease_fillrect_edge; // sub 1 from lower right corner coordinates of fillrect
    int texture_correction; // enable perspective texture correction emulation. is on by default
    inline StippleMode_t stipple_mode(void) const { return m_stipple_mode; } //used for dithered alpha emulation
    uint32_t stipple_pattern; //used for dithered alpha emulation
    int force_microcheck; //check microcode each frame, for mixed F3DEX-S2DEX games
    int force_quad3d; //force 0xb5 command to be quad, not line 3d
    int clip_zmin; //enable near z clipping
    int clip_zmax; //enable far plane clipping;
    int adjust_aspect; //adjust screen aspect for wide screen mode
    int force_calc_sphere; //use spheric mapping only, Ridge Racer 64
    int pal230;    //set special scale for PAL games
    int correct_viewport; //correct viewport values
    int zmode_compare_less; //force GR_CMP_LESS for zmode=0 (opaque)and zmode=1 (interpenetrating)
    int old_style_adither; //apply alpha dither regardless of alpha_dither_mode
    int n64_z_scale; //scale vertex z value before writing to depth buffer, as N64 does.
    
    inline bool hacks(hacks_t hack) const { return (m_hacks & hack) == hack; } //Special game hacks
    
    //wrapper settings
#ifndef ANDROID
    int wrpResolution;
#endif
    int wrpVRAM;
    int wrpFBO;
    int wrpAnisotropic;
    void SetScreenRes(uint32_t value);
    void SetAspectmode(AspectMode_t value);
    void SetLODmode(PixelLevelOfDetail_t value);
    void SetFiltering(Filtering_t value);
    void SetSwapMode(SwapMode_t value);
    void SetGhqFltr(TextureFilter_t value);
    void SetGhqEnht(TextureEnhancement_t value);
    void UpdateFrameBufferBits(uint32_t BitsToAdd, uint32_t BitsToRemove);
    ucode_t DetectUCode(uint32_t uc_crc);
    void SetUcode(ucode_t value);

    void ReadGameSettings(const char * name);
    void WriteSettings(void);
    void UpdateAspectRatio(void);
    void UpdateScreenSize(bool fullscreen);

private:
    void ReadSettings();
    void RegisterSettings(void);
    void SettingsChanged(void);
    
    static void stSettingsChanged(void * _this)
    {
        ((CSettings *)_this)->SettingsChanged();
    }

    bool m_dirty;
    bool m_FlushLogs;
    char m_log_dir[260];
    uint32_t m_ScreenRes;
    uint32_t m_res_x, m_scr_res_x;
    uint32_t m_res_y, m_scr_res_y;
    AspectMode_t m_aspectmode;
    uint32_t m_frame_buffer;
    FBCRCMODE_t m_fb_crc_mode;
    ScreenRotate_t m_rotate;
    Filtering_t m_filtering;
    SwapMode_t m_swapmode;
    PixelLevelOfDetail_t m_lodmode;
    TextureFilter_t m_ghq_fltr;
    TextureEnhancement_t m_ghq_enht;
    ucode_t m_ucode;
    StippleMode_t m_stipple_mode;
    hacks_t m_hacks;
};

extern CSettings * g_settings;
