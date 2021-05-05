#pragma once
#include <Project64-core/Plugins/PluginBase.h>

class CGfxPlugin : public CPlugin
{
    typedef struct
    {
        // Menu
        // Items should have an ID between 5101 and 5200
        void * hGFXMenu;
        void(CALL *ProcessMenuItem)(int32_t ID);

        // Breakpoints
        int32_t UseBPoints;
        char BPPanelName[20];
        void(CALL *Add_BPoint)      (void);
        void(CALL *CreateBPPanel)   (void * hDlg, void * rcBox);
        void(CALL *HideBPPanel)     (void);
        void(CALL *PaintBPPanel)    (void * ps);
        void(CALL *ShowBPPanel)     (void);
        void(CALL *RefreshBpoints)  (void * hList);
        void(CALL *RemoveBpoint)    (void * hList, int32_t index);
        void(CALL *RemoveAllBpoint) (void);

        // GFX command window
        void(CALL *Enter_GFX_Commands_Window)(void);
    } GFXDEBUG_INFO;

    typedef struct
    {
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
    CGfxPlugin(void);
    ~CGfxPlugin();

    bool LoadFunctions(void);
    bool Initiate(CN64System * System, RenderWindow * Window);

    void(CALL *CaptureScreen)   (const char *);
    void(CALL *ChangeWindow)    (void);
    void(CALL *DrawScreen)      (void);
    void(CALL *DrawStatus)      (const char * lpString, int32_t RightAlign);
    void(CALL *MoveScreen)      (int32_t xpos, int32_t ypos);
    void(CALL *ProcessDList)    (void);
    void(CALL *ProcessRDPList)  (void);
    void(CALL *ShowCFB)         (void);
    void(CALL *UpdateScreen)    (void);
    void(CALL *ViStatusChanged) (void);
    void(CALL *ViWidthChanged)  (void);
    void(CALL *SoftReset)       (void);
#ifdef ANDROID
    void(CALL *SurfaceCreated)  (void);
    void(CALL *SurfaceChanged)  (int w, int h);
#endif

    // ROM browser
    void *(CALL * GetRomBrowserMenu)(void); // Items should have an ID between 4101 and 4200
    void(CALL * OnRomBrowserMenuItem)(int32_t MenuID, void * hParent, uint8_t * HEADER);

    void * GetDebugMenu(void) { return m_GFXDebug.hGFXMenu; }
    void ProcessMenuItem(int32_t id);

private:
    CGfxPlugin(const CGfxPlugin&);
    CGfxPlugin& operator=(const CGfxPlugin&);

    virtual int32_t GetDefaultSettingStartRange() const { return FirstGfxDefaultSet; }
    virtual int32_t GetSettingStartRange() const { return FirstGfxSettings; }
    PLUGIN_TYPE type() { return PLUGIN_TYPE_GFX; }

    void UnloadPluginDetails(void);

    GFXDEBUG_INFO m_GFXDebug;

    void(CALL *GetDebugInfo)    (GFXDEBUG_INFO * GFXDebugInfo);
    void(CALL *InitiateDebugger)(DEBUG_INFO DebugInfo);

#ifdef ANDROID
    static void SwapBuffers(void);
#endif
    static void CALL DummyDrawScreen(void) {}
    static void CALL DummyMoveScreen(int32_t /*xpos*/, int32_t /*ypos*/) {}
    static void CALL DummyViStatusChanged(void) {}
    static void CALL DummyViWidthChanged(void) {}
    static void CALL DummySoftReset(void) {}
};
