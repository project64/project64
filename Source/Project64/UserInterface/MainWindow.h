#pragma once

#include "../Settings/GuiSettings.h"
#include <Project64/UserInterface/Debugger/debugger.h>
#include <Project64-core/Plugins/Plugin.h>
#include <Project64\UserInterface\CheatUI.h>
#include <Project64\UserInterface\EnhancementUI.h>
#include <Project64\UserInterface\ProjectSupport.h>

class CGfxPlugin;      // Plugin that controls the rendering
class CAudioPlugin;    // Plugin for audio, need the hwnd
class CControl_Plugin; // Controller needs hwnd to see if it is the focused window
class CBaseMenu;           // Menu for the GUI
class CN64System;
class CriticalSection;

enum
{
    WM_HIDE_CUROSR = WM_USER + 10,
    WM_MAKE_FOCUS = WM_USER + 17,
    WM_RESET_PLUGIN = WM_USER + 18,
    WM_GAME_CLOSED = WM_USER + 19,
    WM_BROWSER_TOP = WM_USER + 40,
    WM_JSAPI_ACTION = WM_USER + 41,
    WM_EXTERNALAPI = WM_APP + 1 // Can be used by other processes
};

enum
{
    JSAPI_ACT_OPEN_ROM,
    JSAPI_ACT_CLOSE_ROM,
    JSAPI_ACT_RESET,
    JSAPI_ACT_PAUSE,
    JSAPI_ACT_RESUME
};

enum 
{
    EXAPI_QUERY_SIMPLE = 1 // Query simple states/properties
};

enum
{
    COPYDATAOPERATION_RUNIMAGE = 1
};

class CMainGui :
    public RenderWindow,
    public CRomBrowser,
    private CGuiSettings
{
    enum { StatusBarID = 400 };

    enum { Timer_SetWindowPos = 1 };

    struct RESET_PLUGIN
    {
        CN64System * system;
        CPlugins * plugins;
        HANDLE hEvent;
        bool res;
    };
public:
    CMainGui(bool bMainWindow, const char * WindowTitle = "");
    ~CMainGui(void);

    WPARAM ProcessAllMessages(void);
    bool ProcessGuiMessages(void);
    void EnterLogOptions(void);
    int Height(void); 
    int Width(void);
    float DPIScale(HWND hWnd);

    void SetPos(int X, int Y);
    void Show(bool ShowWindow);
    void MakeWindowOnTop(bool OnTop);
    void BringToTop(void);
    void Caption(LPCWSTR Caption);
    void SaveWindowLoc(void);

    void SetWindowMenu(CBaseMenu * Menu);
    void RefreshMenu(void);
    CBaseMenu * GetMenuClass(void) { return m_Menu; }

    void SetStatusText(int Panel, const wchar_t * Text);
    void ShowStatusBar(bool ShowBar);

    bool ResetPluginsInUiThread(CPlugins * plugins, CN64System * System);

    void DisplayCheatsUI(bool BlockExecution);
    void DisplayEnhancements(bool BlockExecution);

    void * GetWindowHandle(void) const { return m_hMainWindow; }
    void * GetStatusBar(void) const;
    void * GetModuleInstance(void) const;
    static std::wstring GetWindowClassName();

    inline CProjectSupport & Support(void) { return m_Support; }

private:
    CMainGui(void);
    CMainGui(const CMainGui&);
    CMainGui& operator=(const CMainGui&);

    friend class CGfxPlugin;
    friend class CAudioPlugin;
    friend class CControl_Plugin;

    bool RegisterWinClass(void);
    void ChangeWinSize(long width, long height);
    void Create(const char * WindowTitle);
    void CreateStatusBar(void);
    void Resize(DWORD fwSizeType, WORD nWidth, WORD nHeight); // Responding to WM_SIZE
    void AddRecentRom(const char * ImagePath);
    void SetWindowCaption(const wchar_t * Caption);
    void ShowRomBrowser(void);

    static LRESULT CALLBACK MainGui_Proc(HWND, UINT, WPARAM, LPARAM);

    friend void RomBowserEnabledChanged(CMainGui * Gui);
    friend void RomBowserColoumnsChanged(CMainGui * Gui);
    friend void RomBrowserListChanged(CMainGui * Gui);
	friend void DiscordRPCChanged(CMainGui * Gui);
    static void LoadingInProgressChanged(CMainGui * Gui);
    static void GameLoaded(CMainGui * Gui);
    static void GamePaused(CMainGui * Gui);
    static void GameCpuRunning(CMainGui * Gui);
    static void ShowStatusBarChanged(CMainGui * Gui);
    
    CBaseMenu * m_Menu;

    HWND m_hMainWindow, m_hStatusWnd;
    DWORD m_ThreadId;
    CCheatsUI m_CheatsUI;
    CEnhancementUI m_EnhancementUI;
    CProjectSupport m_Support;

    const bool m_bMainWindow;
    bool m_Created;
    bool m_AttachingMenu;
    bool m_MakingVisible;
    bool m_ResetPlugins;
    RESET_PLUGIN * m_ResetInfo;

    CriticalSection m_CS;

    bool m_SaveMainWindowPos;
    LONG m_SaveMainWindowTop;
    LONG m_SaveMainWindowLeft;

    bool m_SaveRomBrowserPos;
    LONG m_SaveRomBrowserTop;
    LONG m_SaveRomBrowserLeft;
};
