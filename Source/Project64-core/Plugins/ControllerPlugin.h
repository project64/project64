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
#include <Project64-core/Plugins/PluginBase.h>

#pragma warning(push)
#pragma warning(disable : 4201) // warning C4201: nonstandard extension used : nameless struct/union

typedef union
{
    uint32_t Value;
    struct
    {
        unsigned R_DPAD : 1;
        unsigned L_DPAD : 1;
        unsigned D_DPAD : 1;
        unsigned U_DPAD : 1;
        unsigned START_BUTTON : 1;
        unsigned Z_TRIG : 1;
        unsigned B_BUTTON : 1;
        unsigned A_BUTTON : 1;

        unsigned R_CBUTTON : 1;
        unsigned L_CBUTTON : 1;
        unsigned D_CBUTTON : 1;
        unsigned U_CBUTTON : 1;
        unsigned R_TRIG : 1;
        unsigned L_TRIG : 1;
        unsigned Reserved1 : 1;
        unsigned Reserved2 : 1;

        signed   Y_AXIS : 8;

        signed   X_AXIS : 8;
    };
} BUTTONS;
#pragma warning(pop)

typedef struct
{
    int32_t Present;
    int32_t RawData;
    int32_t Plugin;
} CONTROL;

typedef struct
{
    void * hMainWindow;
    void * hinst;

    int32_t MemoryBswaped;  // memory in client- or server-native endian
    uint8_t * HEADER;   // the ROM header (first 40h bytes of the ROM)
    CONTROL * Controls; // pointer to array of 4 controllers, i.e.:  CONTROL Controls[4];
} CONTROL_INFO;

enum PluginType
{
    PLUGIN_NONE = 1,
    PLUGIN_MEMPAK = 2,
    PLUGIN_RUMBLE_PAK = 3,
    PLUGIN_TANSFER_PAK = 4, // not implemeted for non raw data
    PLUGIN_RAW = 5, // the controller plugin is passed in raw data
};

class CControl_Plugin;

class CCONTROL
{
public:
    CCONTROL(int32_t &Present, int32_t &RawData, int32_t &PlugType);
    inline bool  Present(void) const { return m_Present != 0; }
    inline uint32_t Buttons(void) const { return m_Buttons.Value; }
    inline PluginType Plugin(void) const { return static_cast<PluginType>(m_PlugType); }
private:
    friend CControl_Plugin; //controller plugin class has full access

    int32_t & m_Present;
    int32_t & m_RawData;
    int32_t      & m_PlugType;
    BUTTONS    m_Buttons;

    CCONTROL(void);                         // Disable default constructor
    CCONTROL(const CCONTROL&);              // Disable copy constructor
    CCONTROL& operator=(const CCONTROL&);   // Disable assignment
};

class CControl_Plugin : public CPlugin
{
public:
    CControl_Plugin(void);
    ~CControl_Plugin();

    bool Initiate(CN64System * System, RenderWindow * Window);
    void SetControl(CControl_Plugin const * const Plugin);
    void UpdateKeys(void);

    void(CALL *WM_KeyDown)          (uint32_t wParam, uint32_t lParam);
    void(CALL *WM_KeyUp)            (uint32_t wParam, uint32_t lParam);
    void(CALL *RumbleCommand)       (int32_t Control, int32_t bRumble);
    void(CALL *GetKeys)             (int32_t Control, BUTTONS * Keys);
    void(CALL *ReadController)      (int32_t Control, uint8_t * Command);
    void(CALL *ControllerCommand)   (int32_t Control, uint8_t * Command);

    inline CCONTROL const * Controller(int32_t control) { return m_Controllers[control]; }
    inline CONTROL * PluginControllers(void) { return m_PluginControllers; }

private:
    CControl_Plugin(const CControl_Plugin&);			// Disable copy constructor
    CControl_Plugin& operator=(const CControl_Plugin&);	// Disable assignment

    virtual int32_t GetDefaultSettingStartRange() const { return FirstCtrlDefaultSet; }
    virtual int32_t GetSettingStartRange() const { return FirstCtrlSettings; }
    PLUGIN_TYPE type() { return PLUGIN_TYPE_CONTROLLER; }
    bool LoadFunctions(void);
    void UnloadPluginDetails(void);

    bool   m_AllocatedControllers;

    // What the different controls are set up as
    CONTROL m_PluginControllers[4];
    CCONTROL * m_Controllers[4];
};
