#pragma once
#include <Project64/Settings/UISettings.h>

enum MainMenuID
{
    // File menu
    ID_FILE_OPEN_ROM = 4000, ID_FILE_OPEN_COMBO, ID_FILE_ROM_INFO, ID_FILE_STARTEMULATION, ID_FILE_ENDEMULATION,
    ID_FILE_ROMDIRECTORY, ID_FILE_REFRESHROMLIST, ID_FILE_EXIT,

    // Language
    ID_LANG_START, ID_LANG_END = ID_LANG_START + 100,

    // Recent files
    ID_RECENT_ROM_START, ID_RECENT_ROM_END = ID_RECENT_ROM_START + 20,

    // Recent directory
    ID_RECENT_DIR_START, ID_RECENT_DIR_END = ID_RECENT_DIR_START + 20,

    // System menu
    ID_SYSTEM_RESET_SOFT, ID_SYSTEM_RESET_HARD, ID_SYSTEM_PAUSE, ID_SYSTEM_BITMAP,
    ID_SYSTEM_LIMITFPS, ID_SYSTEM_SWAPDISK, ID_SYSTEM_RESTORE, ID_SYSTEM_LOAD, ID_SYSTEM_SAVE,
    ID_SYSTEM_SAVEAS, ID_SYSTEM_ENHANCEMENT, ID_SYSTEM_CHEAT, ID_SYSTEM_GSBUTTON,

    //Current save slot
    ID_CURRENT_SAVE_1, ID_CURRENT_SAVE_2, ID_CURRENT_SAVE_3, ID_CURRENT_SAVE_4, ID_CURRENT_SAVE_5,
    ID_CURRENT_SAVE_6, ID_CURRENT_SAVE_7, ID_CURRENT_SAVE_8, ID_CURRENT_SAVE_9, ID_CURRENT_SAVE_10,
    ID_CURRENT_SAVE_DEFAULT,

    // Option menu
    ID_OPTIONS_FULLSCREEN, ID_OPTIONS_FULLSCREEN2, ID_OPTIONS_ALWAYSONTOP, ID_OPTIONS_CONFIG_GFX,
    ID_OPTIONS_CONFIG_AUDIO, ID_OPTIONS_CONFIG_CONT, ID_OPTIONS_CONFIG_RSP, ID_OPTIONS_CPU_USAGE,
    ID_OPTIONS_SETTINGS, ID_OPTIONS_DISPLAY_FR, ID_OPTIONS_CHANGE_FR, ID_OPTIONS_INCREASE_SPEED,
    ID_OPTIONS_DECREASE_SPEED,

    // Debugger menu
    ID_DEBUG_SHOW_TLB_MISSES, ID_DEBUG_SHOW_UNHANDLED_MEM, ID_DEBUG_SHOW_PIF_ERRORS,
    ID_DEBUG_SHOW_DLIST_COUNT, ID_DEBUG_SHOW_RECOMP_MEM_SIZE, ID_DEBUG_SHOW_DIV_BY_ZERO,
    ID_DEBUG_RECORD_RECOMPILER_ASM, ID_DEBUG_DISABLE_GAMEFIX, ID_DEBUG_LANGUAGE,
    ID_DEBUGGER_LOGOPTIONS, ID_DEBUGGER_GENERATELOG, ID_DEBUGGER_DUMPMEMORY, ID_DEBUGGER_SEARCHMEMORY,
    ID_DEBUGGER_TLBENTRIES, ID_DEBUGGER_BREAKPOINTS, ID_DEBUGGER_MEMORY, ID_DEBUGGER_R4300REGISTERS,
    ID_DEBUGGER_INTERRUPT_SP, ID_DEBUGGER_INTERRUPT_SI, ID_DEBUGGER_INTERRUPT_AI, ID_DEBUGGER_INTERRUPT_VI,
    ID_DEBUGGER_INTERRUPT_PI, ID_DEBUGGER_INTERRUPT_DP, ID_DEBUGGER_SCRIPTS, ID_DEBUGGER_SYMBOLS, ID_DEBUGGER_DMALOG,
    ID_DEBUGGER_EXCBREAKPOINTS, ID_DEBUGGER_CPULOG, ID_DEBUGGER_STACKTRACE, ID_DEBUGGER_STACKVIEW,

    // App logging
    ID_DEBUGGER_APPLOG_FLUSH, ID_DEBUGGER_TRACE_MD5, ID_DEBUGGER_TRACE_SETTINGS, ID_DEBUGGER_TRACE_UNKNOWN, ID_DEBUGGER_TRACE_APPINIT,
    ID_DEBUGGER_TRACE_APPCLEANUP, ID_DEBUGGER_TRACE_N64SYSTEM, ID_DEBUGGER_TRACE_PLUGINS, ID_DEBUGGER_TRACE_GFXPLUGIN,
    ID_DEBUGGER_TRACE_AUDIOPLUGIN, ID_DEBUGGER_TRACE_CONTROLLERPLUGIN, ID_DEBUGGER_TRACE_RSPPLUGIN, ID_DEBUGGER_TRACE_RSP,
    ID_DEBUGGER_TRACE_AUDIO, ID_DEBUGGER_TRACE_REGISTERCACHE, ID_DEBUGGER_TRACE_RECOMPILER, ID_DEBUGGER_TRACE_TLB,
    ID_DEBUGGER_TRACE_PROTECTEDMEM, ID_DEBUGGER_TRACE_USERINTERFACE,

    // Profile menu
    ID_PROFILE_PROFILE, ID_PROFILE_RESETCOUNTER, ID_PROFILE_GENERATELOG,

    // Help menu
    ID_HELP_SUPPORT_PROJECT64, ID_HELP_DISCORD, ID_HELP_WEBSITE, ID_HELP_ABOUT,
};

class CMainMenu :
    public CBaseMenu,
    private CDebugSettings
{
public:
    CMainMenu(CMainGui * Window);
    ~CMainMenu();

    int       ProcessAccelerator(HWND hWnd, void * lpMsg);
    bool      ProcessMessage(HWND hWnd, DWORD wNotifyCode, DWORD wID);
    bool      OnMenuSelect(HWND hWnd, DWORD wNotifyCode, DWORD MenuID);
    void      ResetMenu(void);
    void      ResetAccelerators(void) { m_ResetAccelerators = true; }

private:
    CMainMenu();
    CMainMenu(const CMainMenu&);
    CMainMenu& operator=(const CMainMenu&);

    void OnOpenRom(HWND hWnd);
    void OnOpenCombo(HWND hWnd);
    void OnRomInfo(HWND hWnd);
    void OnEndEmulation(void);
    void OnScreenShot(void);
    void OnSaveAs(HWND hWnd);
    void OnLodState(HWND hWnd);
    void OnEnhancements(HWND hWnd);
    void OnCheats(HWND hWnd);
    void OnSettings(HWND hWnd);
    void OnSupportProject64(HWND hWnd);

    void FillOutMenu(HMENU hMenu);
    std::wstring GetSaveSlotString(int Slot);
    stdstr GetFileLastMod(const CPath & FileName);
    void RebuildAccelerators(void);
    std::string ChooseFileToOpen(HWND hParent);
    std::string ChooseROMFileToOpen(HWND hParent);
    std::string ChooseDiskFileToOpen(HWND hParent);
    void SetTraceModuleSetttings(SettingID Type);
    void ShortCutsChanged(void);

    static void SettingsChanged(CMainMenu * _this);
    static void stShortCutsChanged(CMainMenu * _this) { return _this->ShortCutsChanged(); }

    typedef std::list<SettingID> SettingList;
    typedef std::list<UISettingID> UISettingList;

    CMainGui * m_Gui;

    void * m_AccelTable;
    bool m_ResetAccelerators;
    CShortCuts m_ShortCuts;
    SettingList m_ChangeSettingList;
    UISettingList m_ChangeUISettingList;
    CriticalSection m_CS;
};
