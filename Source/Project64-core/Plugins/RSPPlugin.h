#pragma once
#include <Project64-core/Plugins/PluginBase.h>

class CRSP_Plugin : public CPlugin
{
    typedef struct {
        // Menu
        // Items should have an ID between 5001 and 5100
        void * hRSPMenu;
        void(CALL *ProcessMenuItem) (int32_t ID);

        // Breakpoints
        int32_t UseBPoints;
        char BPPanelName[20];
        void(CALL *Add_BPoint)      (void);
        void(CALL *CreateBPPanel)   (void);
        void(CALL *HideBPPanel)     (void);
        void(CALL *PaintBPPanel)    (void);
        void(CALL *ShowBPPanel)     (void);
        void(CALL *RefreshBpoints)  (void * hList);
        void(CALL *RemoveBpoint)    (void * hList, int32_t index);
        void(CALL *RemoveAllBpoint) (void);

        // RSP command window
        void(CALL *Enter_RSP_Commands_Window)(void);
    } RSPDEBUG_INFO;

    typedef struct {
        void(CALL *UpdateBreakPoints)(void);
        void(CALL *UpdateMemory)(void);
        void(CALL *UpdateR4300iRegisters)(void);
        void(CALL *Enter_BPoint_Window)(void);
        void(CALL *Enter_R4300i_Commands_Window)(void);
        void(CALL *Enter_R4300i_Register_Window)(void);
        void(CALL *Enter_RSP_Commands_Window)(void);
        void(CALL *Enter_Memory_Window)(void);
    } DEBUG_INFO;

public:
    CRSP_Plugin(void);
    ~CRSP_Plugin();

    bool Initiate(CPlugins * Plugins, CN64System * System);

    uint32_t(CALL *DoRspCycles)(uint32_t);
    void(CALL *EnableDebugging)(int32_t Enable);

    void * GetDebugMenu(void) { return m_RSPDebug.hRSPMenu; }
    void ProcessMenuItem(int32_t id);

private:
    CRSP_Plugin(const CRSP_Plugin&);
    CRSP_Plugin& operator=(const CRSP_Plugin&);

    PLUGIN_TYPE type() { return PLUGIN_TYPE_RSP; }
    virtual int32_t GetDefaultSettingStartRange() const { return FirstRSPDefaultSet; }
    virtual int32_t GetSettingStartRange() const { return FirstRSPSettings; }

    bool LoadFunctions(void);
    void UnloadPluginDetails(void);

    RSPDEBUG_INFO m_RSPDebug;
    uint32_t      m_CycleCount;

    void(CALL *GetDebugInfo)    (RSPDEBUG_INFO * GFXDebugInfo);
    void(CALL *InitiateDebugger)(DEBUG_INFO DebugInfo);
};
