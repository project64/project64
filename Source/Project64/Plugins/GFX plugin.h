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

class CGfxPlugin : public CPlugin
{
    typedef struct
    {
        /* Menu */
        /* Items should have an ID between 5101 and 5200 */
        void * hGFXMenu;
        void(__cdecl *ProcessMenuItem) (int32_t ID);

        /* Break Points */
        int32_t UseBPoints;
        char BPPanelName[20];
        void(__cdecl *Add_BPoint)      (void);
        void(__cdecl *CreateBPPanel)   (void * hDlg, void * rcBox);
        void(__cdecl *HideBPPanel)     (void);
        void(__cdecl *PaintBPPanel)    (void * ps);
        void(__cdecl *ShowBPPanel)     (void);
        void(__cdecl *RefreshBpoints)  (void * hList);
        void(__cdecl *RemoveBpoint)    (void * hList, int32_t index);
        void(__cdecl *RemoveAllBpoint) (void);

        /* GFX command Window */
        void(__cdecl *Enter_GFX_Commands_Window) (void);
    } GFXDEBUG_INFO;

    typedef struct
    {
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
    CGfxPlugin(void);
    ~CGfxPlugin();

    bool LoadFunctions(void);
    bool Initiate(CN64System * System, RenderWindow * Window);

    void(__cdecl *CaptureScreen)      (const char *);
    void(__cdecl *ChangeWindow)       (void);
    void(__cdecl *DrawScreen)         (void);
    void(__cdecl *DrawStatus)         (const char * lpString, int32_t RightAlign);
    void(__cdecl *MoveScreen)         (int32_t xpos, int32_t ypos);
    void(__cdecl *ProcessDList)       (void);
    void(__cdecl *ProcessRDPList)     (void);
    void(__cdecl *ShowCFB)			   (void);
    void(__cdecl *UpdateScreen)       (void);
    void(__cdecl *ViStatusChanged)    (void);
    void(__cdecl *ViWidthChanged)     (void);
    void(__cdecl *SoftReset)          (void);

    //Rom Browser
    void *(__cdecl * GetRomBrowserMenu)  (void); /* Items should have an ID between 4101 and 4200 */
    void(__cdecl * OnRomBrowserMenuItem) (int32_t MenuID, void * hParent, uint8_t * HEADER);

    void * GetDebugMenu(void) { return m_GFXDebug.hGFXMenu; }
    void ProcessMenuItem(int32_t id);

private:
    CGfxPlugin(const CGfxPlugin&);				// Disable copy constructor
    CGfxPlugin& operator=(const CGfxPlugin&);	// Disable assignment

    virtual int32_t GetDefaultSettingStartRange() const { return FirstGfxDefaultSet; }
    virtual int32_t GetSettingStartRange() const { return FirstGfxSettings; }
    PLUGIN_TYPE type() { return PLUGIN_TYPE_GFX; }

    void UnloadPluginDetails(void);

    GFXDEBUG_INFO m_GFXDebug;

    void(__cdecl *GetDebugInfo)	(GFXDEBUG_INFO * GFXDebugInfo);
    void(__cdecl *InitiateDebugger)(DEBUG_INFO DebugInfo);

    static void __cdecl DummyDrawScreen(void) {}
    static void __cdecl DummyMoveScreen(int32_t /*xpos*/, int32_t /*ypos*/) {}
    static void __cdecl DummyViStatusChanged(void) {}
    static void __cdecl DummyViWidthChanged(void) {}
    static void __cdecl DummySoftReset(void) {}
};