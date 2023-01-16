#include "jniBridgeSettings.h"
#include "UISettings.h"
#include <Project64-core/Settings.h>
#include <Project64-video/SettingsID.h>
#ifdef ANDROID
#include <android/log.h>
#endif

int CJniBridegSettings::m_RefCount = 0;
bool CJniBridegSettings::m_bCPURunning;

CJniBridegSettings::CJniBridegSettings()
{
    m_RefCount += 1;
    if (m_RefCount == 1)
    {
        g_Settings->RegisterChangeCB(GameRunning_CPU_Running,NULL,RefreshSettings);
        RefreshSettings(NULL);
    }

#define ADD_SETTING(ID) m_SettingNameList.insert(SettingNameList::value_type("Core." #ID,ID));
#define ADD_UI_SETTING(ID) m_SettingNameList.insert(SettingNameList::value_type("UISettingID." #ID,(SettingID)ID));
#define ADD_VIDEO_SETTING(ID) m_SettingNameList.insert(SettingNameList::value_type("VideoSettingID." #ID,(SettingID)(FirstGfxSettings + ID)));

    ADD_SETTING(Cmd_BaseDirectory);
    ADD_SETTING(Cmd_RomFile);
    ADD_SETTING(Cmd_ComboDiskFile);
    ADD_SETTING(Cmd_ShowHelp);

    // Support files
    ADD_SETTING(SupportFile_Settings);
    ADD_SETTING(SupportFile_SettingsDefault);
    ADD_SETTING(SupportFile_RomDatabase);
    ADD_SETTING(SupportFile_RomDatabaseDefault);
    ADD_SETTING(SupportFile_VideoRDB);
    ADD_SETTING(SupportFile_VideoRDBDefault);
    ADD_SETTING(SupportFile_AudioRDB);
    ADD_SETTING(SupportFile_AudioRDBDefault);
    ADD_SETTING(SupportFile_CheatDir);
    ADD_SETTING(SupportFile_CheatDirDefault);
    ADD_SETTING(SupportFile_UserCheatDir);
    ADD_SETTING(SupportFile_UserCheatDirDefault);
    ADD_SETTING(SupportFile_EnhancementDir);
    ADD_SETTING(SupportFile_EnhancementDirDefault);
    ADD_SETTING(SupportFile_UserEnhancementDir);
    ADD_SETTING(SupportFile_UserEnhancementDirDefault);
    ADD_SETTING(SupportFile_Notes);
    ADD_SETTING(SupportFile_NotesDefault);
    ADD_SETTING(SupportFile_ExtInfo);
    ADD_SETTING(SupportFile_ExtInfoDefault);

    // Settings
    ADD_SETTING(Setting_ApplicationName);
    ADD_SETTING(Setting_UseFromRegistry);
    ADD_SETTING(Setting_RdbEditor);
    ADD_SETTING(Setting_CN64TimeCritical);
    ADD_SETTING(Setting_AutoStart);
    ADD_SETTING(Setting_CheckEmuRunning);
    ADD_SETTING(Setting_EraseGameDefaults);
    ADD_SETTING(Setting_ForceInterpreterCPU);
    ADD_SETTING(Setting_FixedRdramAddress);

    ADD_SETTING(Setting_AutoZipInstantSave);
    ADD_SETTING(Setting_RememberCheats);
    ADD_SETTING(Setting_UniqueSaveDir);
    ADD_SETTING(Setting_LanguageDir);
    ADD_SETTING(Setting_LanguageDirDefault);
    ADD_SETTING(Setting_CurrentLanguage);
    ADD_SETTING(Setting_EnableDisk);
    ADD_SETTING(Setting_SyncViaAudioEnabled);
    ADD_SETTING(Setting_Enhancement);
    ADD_SETTING(Setting_DiskSaveType);

    // Default settings
    ADD_SETTING(Default_RDRamSizeUnknown);
    ADD_SETTING(Default_RDRamSizeKnown);
    ADD_SETTING(Default_UseHleGfx);
    ADD_SETTING(Default_ViRefreshRate);
    ADD_SETTING(Default_AiCountPerBytes);
    ADD_SETTING(Default_CounterFactor);
    ADD_SETTING(Default_SyncViaAudio);
    ADD_SETTING(Default_FixedAudio);
    ADD_SETTING(Default_RandomizeSIPIInterrupts);
    ADD_SETTING(Default_SMM_Protect_Memory);
    ADD_SETTING(Default_DiskSeekTiming);

    // RDB settings
    ADD_SETTING(Rdb_GoodName);
    ADD_SETTING(Rdb_SaveChip);
    ADD_SETTING(Rdb_CpuType);
    ADD_SETTING(Rdb_RDRamSize);
    ADD_SETTING(Rdb_CounterFactor);
    ADD_SETTING(Rdb_DelayDP);
    ADD_SETTING(Rdb_DelaySi);
    ADD_SETTING(Rdb_32Bit);
    ADD_SETTING(Rdb_FastSP);
    ADD_SETTING(Rdb_FixedAudio);
    ADD_SETTING(Rdb_SyncViaAudio);
    ADD_SETTING(Rdb_RspAudioSignal);
    ADD_SETTING(Rdb_TLB_VAddrStart);
    ADD_SETTING(Rdb_TLB_VAddrLen);
    ADD_SETTING(Rdb_TLB_PAddrStart);
    ADD_SETTING(Rdb_UseHleGfx);
    ADD_SETTING(Rdb_UseHleAudio);
    ADD_SETTING(Rdb_ScreenHertz);
    ADD_SETTING(Rdb_FuncLookupMode);
    ADD_SETTING(Rdb_RegCache);
    ADD_SETTING(Rdb_BlockLinking);
    ADD_SETTING(Rdb_SMM_StoreInstruc);
    ADD_SETTING(Rdb_SMM_Cache);
    ADD_SETTING(Rdb_SMM_PIDMA);
    ADD_SETTING(Rdb_SMM_TLB);
    ADD_SETTING(Rdb_SMM_Protect);
    ADD_SETTING(Rdb_SMM_ValidFunc);
    ADD_SETTING(Rdb_ViRefreshRate);
    ADD_SETTING(Rdb_AiCountPerBytes);
    ADD_SETTING(Rdb_AudioResetOnLoad);
    ADD_SETTING(Rdb_AllowROMWrites);
    ADD_SETTING(Rdb_CRC_Recalc);
    ADD_SETTING(Rdb_RandomizeSIPIInterrupts);
    ADD_SETTING(Rdb_RPCKey);
    ADD_SETTING(Rdb_DiskSeekTiming);

    // Individual game settings
    ADD_SETTING(Game_IniKey);
    ADD_SETTING(Game_File);
    ADD_SETTING(Game_UniqueSaveDir);
    ADD_SETTING(Game_GameName);
    ADD_SETTING(Cfg_GoodName);
    ADD_SETTING(Game_TempLoaded);
    ADD_SETTING(Game_SystemType);
    ADD_SETTING(Game_EditPlugin_Gfx);
    ADD_SETTING(Game_EditPlugin_Audio);
    ADD_SETTING(Game_EditPlugin_Contr);
    ADD_SETTING(Game_EditPlugin_RSP);
    ADD_SETTING(Game_Plugin_Gfx);
    ADD_SETTING(Game_Plugin_Audio);
    ADD_SETTING(Game_Plugin_Controller);
    ADD_SETTING(Game_Plugin_RSP);
    ADD_SETTING(Game_SaveChip);
    ADD_SETTING(Game_CpuType);
    ADD_SETTING(Game_LastSaveSlot);
    ADD_SETTING(Game_FixedAudio);
    ADD_SETTING(Game_SyncViaAudio);
    ADD_SETTING(Game_32Bit);
    ADD_SETTING(Game_SMM_Cache);
    ADD_SETTING(Game_SMM_Protect);
    ADD_SETTING(Game_SMM_ValidFunc);
    ADD_SETTING(Game_SMM_PIDMA);
    ADD_SETTING(Game_SMM_TLB);
    ADD_SETTING(Game_SMM_StoreInstruc);
    ADD_SETTING(Game_CurrentSaveState);
    ADD_SETTING(Game_LastSaveTime);
    ADD_SETTING(Game_RDRamSize);
    ADD_SETTING(Game_CounterFactor);
    ADD_SETTING(Game_DelayDP);
    ADD_SETTING(Game_DelaySI);
    ADD_SETTING(Game_FastSP);
    ADD_SETTING(Game_FuncLookupMode);
    ADD_SETTING(Game_RegCache);
    ADD_SETTING(Game_BlockLinking);
    ADD_SETTING(Game_ScreenHertz);
    ADD_SETTING(Game_RspAudioSignal);
    ADD_SETTING(Game_UseHleGfx);
    ADD_SETTING(Game_UseHleAudio);
    ADD_SETTING(Game_ViRefreshRate);
    ADD_SETTING(Game_AiCountPerBytes);
    ADD_SETTING(Game_AudioResetOnLoad);
    ADD_SETTING(Game_AllowROMWrites);
    ADD_SETTING(Game_CRC_Recalc);
    ADD_SETTING(Game_Transferpak_ROM);
    ADD_SETTING(Game_Transferpak_Sav);
    ADD_SETTING(Game_LoadSaveAtStart);
    ADD_SETTING(Game_OverClockModifier);
    ADD_SETTING(Game_FullSpeed);
    ADD_SETTING(Game_RandomizeSIPIInterrupts);
    ADD_SETTING(Game_RPCKey);
    ADD_SETTING(Game_DiskSeekTiming);

    // General game running info
    ADD_SETTING(GameRunning_LoadingInProgress);
    ADD_SETTING(GameRunning_CPU_Running);
    ADD_SETTING(GameRunning_CPU_Paused);
    ADD_SETTING(GameRunning_CPU_PausedType);
    ADD_SETTING(GameRunning_InstantSaveFile);
    ADD_SETTING(GameRunning_LimitFPS);
    ADD_SETTING(GameRunning_ScreenHertz);
    ADD_SETTING(GameRunning_InReset);

    // User interface
    ADD_SETTING(UserInterface_BasicMode);
    ADD_SETTING(UserInterface_ShowCPUPer);
    ADD_SETTING(UserInterface_DisplayFrameRate);
    ADD_SETTING(UserInterface_FrameDisplayType);

    // Directory info
    ADD_SETTING(Directory_Plugin);
    ADD_SETTING(Directory_PluginInitial);
    ADD_SETTING(Directory_PluginSelected);
    ADD_SETTING(Directory_PluginUseSelected);
    ADD_SETTING(Directory_PluginSync);
    ADD_SETTING(Directory_PluginSyncInitial);
    ADD_SETTING(Directory_PluginSyncSelected);
    ADD_SETTING(Directory_PluginSyncUseSelected);
    ADD_SETTING(Directory_SnapShot);
    ADD_SETTING(Directory_SnapShotInitial);
    ADD_SETTING(Directory_SnapShotSelected);
    ADD_SETTING(Directory_SnapShotUseSelected);
    ADD_SETTING(Directory_NativeSave);
    ADD_SETTING(Directory_NativeSaveInitial);
    ADD_SETTING(Directory_NativeSaveSelected);
    ADD_SETTING(Directory_NativeSaveUseSelected);
    ADD_SETTING(Directory_InstantSave);
    ADD_SETTING(Directory_InstantSaveInitial);
    ADD_SETTING(Directory_InstantSaveSelected);
    ADD_SETTING(Directory_InstantSaveUseSelected);
    ADD_SETTING(Directory_Texture);
    ADD_SETTING(Directory_TextureInitial);
    ADD_SETTING(Directory_TextureSelected);
    ADD_SETTING(Directory_TextureUseSelected);
    ADD_SETTING(Directory_Log);
    ADD_SETTING(Directory_LogInitial);
    ADD_SETTING(Directory_LogSelected);
    ADD_SETTING(Directory_LogUseSelected);

    // ROM list
    ADD_SETTING(RomList_RomListCache);
    ADD_SETTING(RomList_RomListCacheDefault);
    ADD_SETTING(RomList_GameDir);
    ADD_SETTING(RomList_GameDirInitial);
    ADD_SETTING(RomList_GameDirSelected);
    ADD_SETTING(RomList_GameDirUseSelected);
    ADD_SETTING(RomList_GameDirRecursive);
    ADD_SETTING(RomList_ShowFileExtensions);
    ADD_SETTING(RomList_7zipCache);
    ADD_SETTING(RomList_7zipCacheDefault);

    // File info
    ADD_SETTING(File_DiskIPLPath);
    ADD_SETTING(File_DiskIPLUSAPath);
    ADD_SETTING(File_DiskIPLTOOLPath);

    // Debugger
    ADD_SETTING(Debugger_Enabled);
    ADD_SETTING(Debugger_EndOnPermLoop);
    ADD_SETTING(Debugger_BreakOnUnhandledMemory);
    ADD_SETTING(Debugger_BreakOnAddressError);
    ADD_SETTING(Debugger_StepOnBreakOpCode);
    ADD_SETTING(Debugger_ShowPifErrors);
    ADD_SETTING(Debugger_RecordRecompilerAsm);
    ADD_SETTING(Debugger_DisableGameFixes);
    ADD_SETTING(Debugger_AppLogLevel);
    ADD_SETTING(Debugger_AppLogFlush);
    ADD_SETTING(Debugger_ShowDListAListCount);
    ADD_SETTING(Debugger_ShowRecompMemSize);
    ADD_SETTING(Debugger_DebugLanguage);
    ADD_SETTING(Debugger_RecordExecutionTimes);
    ADD_SETTING(Debugger_SteppingOps);
    ADD_SETTING(Debugger_SkipOp);
    ADD_SETTING(Debugger_HaveExecutionBP);
    ADD_SETTING(Debugger_WriteBPExists);
    ADD_SETTING(Debugger_ReadBPExists);
    ADD_SETTING(Debugger_WaitingForStep);
    ADD_SETTING(Debugger_CPULoggingEnabled);
    ADD_SETTING(Debugger_CPULogBufferSize);
    ADD_SETTING(Debugger_ExceptionBreakpoints);
    ADD_SETTING(Debugger_FpExceptionBreakpoints);
    ADD_SETTING(Debugger_IntrBreakpoints);
    ADD_SETTING(Debugger_RcpIntrBreakpoints);

    // Trace
    ADD_SETTING(Debugger_TraceMD5);
    ADD_SETTING(Debugger_TraceThread);
    ADD_SETTING(Debugger_TracePath);
    ADD_SETTING(Debugger_TraceSettings);
    ADD_SETTING(Debugger_TraceUnknown);
    ADD_SETTING(Debugger_TraceAppInit);
    ADD_SETTING(Debugger_TraceAppCleanup);
    ADD_SETTING(Debugger_TraceN64System);
    ADD_SETTING(Debugger_TracePlugins);
    ADD_SETTING(Debugger_TraceVideoPlugin);
    ADD_SETTING(Debugger_TraceAudioPlugin);
    ADD_SETTING(Debugger_TraceControllerPlugin);
    ADD_SETTING(Debugger_TraceRSPPlugin);
    ADD_SETTING(Debugger_TraceRSP);
    ADD_SETTING(Debugger_TraceAudio);
    ADD_SETTING(Debugger_TraceRegisterCache);
    ADD_SETTING(Debugger_TraceRecompiler);
    ADD_SETTING(Debugger_TraceTLB);
    ADD_SETTING(Debugger_TraceProtectedMEM);
    ADD_SETTING(Debugger_TraceUserInterface);
    ADD_SETTING(Debugger_TraceRomList);
    ADD_SETTING(Debugger_TraceExceptionHandler);

    // Plugins
    ADD_SETTING(Plugin_RSP_Current);
    ADD_SETTING(Plugin_RSP_CurVer);
    ADD_SETTING(Plugin_GFX_Default);
    ADD_SETTING(Plugin_GFX_Current);
    ADD_SETTING(Plugin_GFX_CurVer);
    ADD_SETTING(Plugin_AUDIO_Current);
    ADD_SETTING(Plugin_AUDIO_CurVer);
    ADD_SETTING(Plugin_CONT_Current);
    ADD_SETTING(Plugin_CONT_CurVer);
    ADD_SETTING(Plugin_UseHleGfx);
    ADD_SETTING(Plugin_UseHleAudio);
    ADD_SETTING(Plugin_EnableAudio);

    ADD_SETTING(Logging_GenerateLog);
    ADD_SETTING(Logging_LogRDRamRegisters);
    ADD_SETTING(Logging_LogSPRegisters);
    ADD_SETTING(Logging_LogDPCRegisters);
    ADD_SETTING(Logging_LogDPSRegisters);
    ADD_SETTING(Logging_LogMIPSInterface);
    ADD_SETTING(Logging_LogVideoInterface);
    ADD_SETTING(Logging_LogAudioInterface);
    ADD_SETTING(Logging_LogPerInterface);
    ADD_SETTING(Logging_LogRDRAMInterface);
    ADD_SETTING(Logging_LogSerialInterface);
    ADD_SETTING(Logging_LogPRDMAOperations);
    ADD_SETTING(Logging_LogPRDirectMemLoads);
    ADD_SETTING(Logging_LogPRDMAMemLoads);
    ADD_SETTING(Logging_LogPRDirectMemStores);
    ADD_SETTING(Logging_LogPRDMAMemStores);
    ADD_SETTING(Logging_LogControllerPak);
    ADD_SETTING(Logging_LogCP0changes);
    ADD_SETTING(Logging_LogCP0reads);
    ADD_SETTING(Logging_LogTLB);
    ADD_SETTING(Logging_LogExceptions);
    ADD_SETTING(Logging_NoInterrupts);
    ADD_SETTING(Logging_LogCache);
    ADD_SETTING(Logging_LogRomHeader);
    ADD_SETTING(Logging_LogUnknown);

    ADD_UI_SETTING(AssertsVersion);
    ADD_UI_SETTING(BuildVersion);
    ADD_UI_SETTING(ScreenOrientation);

    //Recent Game
    ADD_UI_SETTING(FileRecentGameFileCount);
    ADD_UI_SETTING(FileRecentGameFileIndex);

    //Touch Screen
    ADD_UI_SETTING(TouchScreenButtonScale);
    ADD_UI_SETTING(TouchScreenLayout);

    //Controller Config
    ADD_UI_SETTING(ControllerConfigFile);
    ADD_UI_SETTING(ControllerCurrentProfile);
    ADD_UI_SETTING(ControllerDeadzone);
    ADD_UI_SETTING(ControllerSensitivity);

    //App Info
    ADD_UI_SETTING(AppInfoRunCount);

    // General Settings
    ADD_VIDEO_SETTING(Set_vsync);
    ADD_VIDEO_SETTING(Set_Rotate);
    ADD_VIDEO_SETTING(Set_texenh_options);
    ADD_VIDEO_SETTING(Set_wrpVRAM);
    ADD_VIDEO_SETTING(Set_wrpFBO);
    ADD_VIDEO_SETTING(Set_wrpAnisotropic);
    ADD_VIDEO_SETTING(Set_autodetect_ucode);
    ADD_VIDEO_SETTING(Set_ucode);
    ADD_VIDEO_SETTING(Set_wireframe);
    ADD_VIDEO_SETTING(Set_wfmode);
    ADD_VIDEO_SETTING(Set_ghq_fltr);
    ADD_VIDEO_SETTING(Set_ghq_cmpr);
    ADD_VIDEO_SETTING(Set_ghq_enht);
    ADD_VIDEO_SETTING(Set_ghq_hirs);
    ADD_VIDEO_SETTING(Set_ghq_enht_cmpr);
    ADD_VIDEO_SETTING(Set_ghq_enht_f16bpp);
    ADD_VIDEO_SETTING(Set_ghq_enht_gz);
    ADD_VIDEO_SETTING(Set_ghq_enht_nobg);
    ADD_VIDEO_SETTING(Set_ghq_hirs_cmpr);
    ADD_VIDEO_SETTING(Set_ghq_hirs_tile);
    ADD_VIDEO_SETTING(Set_ghq_hirs_f16bpp);
    ADD_VIDEO_SETTING(Set_ghq_hirs_gz);
    ADD_VIDEO_SETTING(Set_ghq_hirs_altcrc);
    ADD_VIDEO_SETTING(Set_ghq_cache_save);
    ADD_VIDEO_SETTING(Set_ghq_cache_size);
    ADD_VIDEO_SETTING(Set_ghq_hirs_let_texartists_fly);
    ADD_VIDEO_SETTING(Set_ghq_hirs_dump);
    ADD_VIDEO_SETTING(Set_Resolution);

    // Default Game Settings
    ADD_VIDEO_SETTING(Set_optimize_texrect_default);
    ADD_VIDEO_SETTING(Set_filtering_default);
    ADD_VIDEO_SETTING(Set_lodmode_default);
    ADD_VIDEO_SETTING(Set_fog_default);
    ADD_VIDEO_SETTING(Set_buff_clear_default);
    ADD_VIDEO_SETTING(Set_swapmode_default);
    ADD_VIDEO_SETTING(Set_aspect_default);
    ADD_VIDEO_SETTING(Set_fb_smart_default);
    ADD_VIDEO_SETTING(Set_fb_hires_default);
    ADD_VIDEO_SETTING(Set_fb_read_always_default);
    ADD_VIDEO_SETTING(Set_read_back_to_screen_default);
    ADD_VIDEO_SETTING(Set_detect_cpu_write_default);
    ADD_VIDEO_SETTING(Set_fb_get_info_default);
    ADD_VIDEO_SETTING(Set_fb_render_default);

    //Game Settings
    ADD_VIDEO_SETTING(Set_alt_tex_size);
    ADD_VIDEO_SETTING(Set_use_sts1_only);
    ADD_VIDEO_SETTING(Set_force_calc_sphere);
    ADD_VIDEO_SETTING(Set_correct_viewport);
    ADD_VIDEO_SETTING(Set_increase_texrect_edge);
    ADD_VIDEO_SETTING(Set_decrease_fillrect_edge);
    ADD_VIDEO_SETTING(Set_texture_correction);
    ADD_VIDEO_SETTING(Set_pal230);
    ADD_VIDEO_SETTING(Set_stipple_mode);
    ADD_VIDEO_SETTING(Set_stipple_pattern);
    ADD_VIDEO_SETTING(Set_force_microcheck);
    ADD_VIDEO_SETTING(Set_force_quad3d);
    ADD_VIDEO_SETTING(Set_clip_zmin);
    ADD_VIDEO_SETTING(Set_clip_zmax);
    ADD_VIDEO_SETTING(Set_fast_crc);
    ADD_VIDEO_SETTING(Set_adjust_aspect);
    ADD_VIDEO_SETTING(Set_zmode_compare_less);
    ADD_VIDEO_SETTING(Set_old_style_adither);
    ADD_VIDEO_SETTING(Set_n64_z_scale);
    ADD_VIDEO_SETTING(Set_optimize_texrect);
    ADD_VIDEO_SETTING(Set_ignore_aux_copy);
    ADD_VIDEO_SETTING(Set_hires_buf_clear);
    ADD_VIDEO_SETTING(Set_fb_read_alpha);
    ADD_VIDEO_SETTING(Set_useless_is_useless);
    ADD_VIDEO_SETTING(Set_fb_crc_mode);
    ADD_VIDEO_SETTING(Set_filtering);
    ADD_VIDEO_SETTING(Set_fog);
    ADD_VIDEO_SETTING(Set_buff_clear);
    ADD_VIDEO_SETTING(Set_swapmode);
    ADD_VIDEO_SETTING(Set_aspect);
    ADD_VIDEO_SETTING(Set_lodmode);
    ADD_VIDEO_SETTING(Set_fb_smart);
    ADD_VIDEO_SETTING(Set_fb_hires);
    ADD_VIDEO_SETTING(Set_fb_read_always);
    ADD_VIDEO_SETTING(Set_read_back_to_screen);
    ADD_VIDEO_SETTING(Set_detect_cpu_write);
    ADD_VIDEO_SETTING(Set_fb_get_info);
    ADD_VIDEO_SETTING(Set_fb_render);

    //RDB Setting
    ADD_VIDEO_SETTING(Set_ucodeLookup);

    //Logging Settings
    ADD_VIDEO_SETTING(Set_Logging_MD5);
    ADD_VIDEO_SETTING(Set_Logging_Thread);
    ADD_VIDEO_SETTING(Set_Logging_Path);
    ADD_VIDEO_SETTING(Set_Logging_Settings);
    ADD_VIDEO_SETTING(Set_Logging_Unknown);
    ADD_VIDEO_SETTING(Set_Logging_Glide64);
    ADD_VIDEO_SETTING(Set_Logging_Interface);
    ADD_VIDEO_SETTING(Set_Logging_Resolution);
    ADD_VIDEO_SETTING(Set_Logging_Glitch);
    ADD_VIDEO_SETTING(Set_Logging_VideoRDP);
    ADD_VIDEO_SETTING(Set_Logging_TLUT);
    ADD_VIDEO_SETTING(Set_Logging_PNG);
    ADD_VIDEO_SETTING(Set_Logging_OGLWrapper);
    ADD_VIDEO_SETTING(Set_Logging_RDPCommands);
}

CJniBridegSettings::~CJniBridegSettings()
{
    m_RefCount -= 1;
    if (m_RefCount == 0)
    {
        g_Settings->UnregisterChangeCB(GameRunning_CPU_Running,NULL,RefreshSettings);
    }
}

void CJniBridegSettings::RefreshSettings(void *)
{
    m_bCPURunning  = g_Settings->LoadBool(GameRunning_CPU_Running);
}

SettingID CJniBridegSettings::TranslateSettingID(const char * SettingName)
{
    SettingNameList::iterator itr = m_SettingNameList.find(SettingName);
    if (itr != m_SettingNameList.end())
    {
        return itr->second;
    }
    return Default_None;
}
