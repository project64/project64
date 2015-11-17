/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once
#include "Plugin Base.h"

class CRSP_Plugin : public CPlugin
{
    typedef struct {
        /* Menu */
        /* Items should have an ID between 5001 and 5100 */
        void * hRSPMenu;
        void(__cdecl *ProcessMenuItem) (int32_t ID);

        /* Break Points */
        int32_t UseBPoints;
        char BPPanelName[20];
        void(__cdecl *Add_BPoint)      (void);
        void(__cdecl *CreateBPPanel)   (void);
        void(__cdecl *HideBPPanel)     (void);
        void(__cdecl *PaintBPPanel)    (void);
        void(__cdecl *ShowBPPanel)     (void);
        void(__cdecl *RefreshBpoints)  (void * hList);
        void(__cdecl *RemoveBpoint)    (void * hList, int32_t index);
        void(__cdecl *RemoveAllBpoint) (void);
        /* RSP command Window */
        void(__cdecl *Enter_RSP_Commands_Window) (void);
    } RSPDEBUG_INFO;

    typedef struct {
        void(__cdecl *UpdateBreakPoints)(void);
        void(__cdecl *UpdateMemory)(void);
        void(__cdecl *UpdateR4300iRegisters)(void);
        void(__cdecl *Enter_BPoint_Window)(void);
        void(__cdecl *Enter_R4300i_Commands_Window)(void);
        void(__cdecl *Enter_R4300i_Register_Window)(void);
        void(__cdecl *Enter_RSP_Commands_Window) (void);
        void(__cdecl *Enter_Memory_Window)(void);
    } DEBUG_INFO;

public:
    CRSP_Plugin(void);
    ~CRSP_Plugin();

    bool Initiate(CPlugins * Plugins, CN64System * System);

    uint32_t(__cdecl *DoRspCycles)	(uint32_t);
    void(__cdecl *EnableDebugging)(int32_t Enable);

    void * GetDebugMenu(void) { return m_RSPDebug.hRSPMenu; }
    void ProcessMenuItem(int32_t id);

private:
    CRSP_Plugin(const CRSP_Plugin&);			// Disable copy constructor
    CRSP_Plugin& operator=(const CRSP_Plugin&);	// Disable assignment

    PLUGIN_TYPE type() { return PLUGIN_TYPE_RSP; }
    virtual int32_t GetDefaultSettingStartRange() const { return FirstRSPDefaultSet; }
    virtual int32_t GetSettingStartRange() const { return FirstRSPSettings; }

    bool LoadFunctions(void);
    void UnloadPluginDetails(void);

    RSPDEBUG_INFO m_RSPDebug;
    uint32_t      m_CycleCount;

    void(__cdecl *GetDebugInfo)     (RSPDEBUG_INFO * GFXDebugInfo);
    void(__cdecl *InitiateDebugger) (DEBUG_INFO DebugInfo);
};
