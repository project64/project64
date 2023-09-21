#pragma once
#include <Common/SyncEvent.h>
#include <Common/Thread.h>
#include <Project64-core/Plugins/PluginBase.h>
#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/Settings/GameSettings.h>
#include <Project64-core/Settings/N64SystemSettings.h>

class CRSP_Plugin :
    public CPlugin,
    protected CN64SystemSettings,
    protected CGameSettings
{
    typedef struct
    {
        // Menu
        // Items should have an ID between 5001 and 5100
        void * hRSPMenu;
        void(CALL * ProcessMenuItem)(int32_t ID);

        // Breakpoints
        int32_t UseBPoints;
        char BPPanelName[20];
        void(CALL * Add_BPoint)(void);
        void(CALL * CreateBPPanel)(void);
        void(CALL * HideBPPanel)(void);
        void(CALL * PaintBPPanel)(void);
        void(CALL * ShowBPPanel)(void);
        void(CALL * RefreshBpoints)(void * hList);
        void(CALL * RemoveBpoint)(void * hList, int32_t index);
        void(CALL * RemoveAllBpoint)(void);

        // RSP command window
        void(CALL * Enter_RSP_Commands_Window)(void);
    } RSPDEBUG_INFO;

    typedef struct
    {
        void(CALL * UpdateBreakPoints)(void);
        void(CALL * UpdateMemory)(void);
        void(CALL * UpdateR4300iRegisters)(void);
        void(CALL * Enter_BPoint_Window)(void);
        void(CALL * Enter_R4300i_Commands_Window)(void);
        void(CALL * Enter_R4300i_Register_Window)(void);
        void(CALL * Enter_RSP_Commands_Window)(void);
        void(CALL * Enter_Memory_Window)(void);
    } DEBUG_INFO;

public:
    CRSP_Plugin(void);
    ~CRSP_Plugin();

    void RomOpened(RenderWindow * Render);
    void RomClose(RenderWindow * Render);
    bool Initiate(CPlugins * Plugins, CN64System * System);
    void EnableDebugging(int32_t Enable);
    void RunRSP(void);

    void * GetDebugMenu(void);
    void ProcessMenuItem(int32_t id);

private:
    CRSP_Plugin(const CRSP_Plugin &);
    CRSP_Plugin & operator=(const CRSP_Plugin &);

    PLUGIN_TYPE type();
    int32_t GetDefaultSettingStartRange() const;
    int32_t GetSettingStartRange() const;
    uint32_t RspThread(void);

    static uint32_t stRspThread(void * lpThreadParameter);

    bool LoadFunctions(void);
    void UnloadPluginDetails(void);

    void(CALL * m_EnableDebugging)(int32_t Enable);
    void(CALL * m_GetDebugInfo)(RSPDEBUG_INFO * GFXDebugInfo);
    void(CALL * m_InitiateDebugger)(DEBUG_INFO DebugInfo);
    uint32_t(CALL * m_DoRspCycles)(uint32_t);

    CPlugins * m_Plugins;
    CN64System * m_System;
    uint32_t m_AlistCount, m_DlistCount, m_UnknownCount;
    SyncEvent m_RunEvent;
    CThread m_Thread;
    RSPDEBUG_INFO m_RSPDebug;
    uint32_t m_CycleCount;
    bool m_RomOpened;
};
