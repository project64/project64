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

        signed   X_AXIS : 8;

        signed   Y_AXIS : 8;
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

    int32_t MemoryBswaped;  // Memory in client or server-native endian
    uint8_t * HEADER;   // The ROM header (first 40h bytes of the ROM)
    CONTROL * Controls; // Pointer to array of 4 controllers, i.e.:  CONTROL Controls[4];
} CONTROL_INFO;

enum PluginType
{
    PLUGIN_NONE = 1,
    PLUGIN_MEMPAK = 2,
    PLUGIN_RUMBLE_PAK = 3,
    PLUGIN_TANSFER_PAK = 4, // Not implemented for non-raw data
    PLUGIN_RAW = 5, // The controller plugin is passed in raw data
};

enum PresentType
{
    PRESENT_NONE = 0,
    PRESENT_CONT = 1,
    PRESENT_MOUSE = 2,
};

class CControl_Plugin;

class CCONTROL
{
public:
    CCONTROL(int32_t &Present, int32_t &RawData, int32_t &PlugType);
    inline bool Present(void) const { return m_Present != 0; }
    inline uint32_t Buttons(void) const { return m_Buttons.Value; }
    inline PluginType Plugin(void) const { return static_cast<PluginType>(m_PlugType); }
private:
    friend class CControl_Plugin;

    int32_t & m_Present;
    int32_t & m_RawData;
    int32_t & m_PlugType;
    BUTTONS m_Buttons;

    CCONTROL(void);
    CCONTROL(const CCONTROL&);
    CCONTROL& operator=(const CCONTROL&);
};

class CControl_Plugin : public CPlugin
{
public:
    typedef void(CALL * fnGetKeys) (int32_t Control, BUTTONS * Keys);

    CControl_Plugin(void);
    ~CControl_Plugin();

    bool Initiate(CN64System * System, RenderWindow * Window);
    void SetControl(CControl_Plugin const * const Plugin);
    void UpdateKeys(void);

    void(CALL *WM_KeyDown) (uint32_t wParam, uint32_t lParam);
    void(CALL *WM_KeyUp) (uint32_t wParam, uint32_t lParam);
    void(CALL *WM_KillFocus) (uint32_t wParam, uint32_t lParam);
    void(CALL *EmulationPaused) ();
    void(CALL *RumbleCommand) (int32_t Control, int32_t bRumble);
    fnGetKeys GetKeys;
    void(CALL *ReadController) (int32_t Control, uint8_t * Command);
    void(CALL *ControllerCommand) (int32_t Control, uint8_t * Command);

    inline CCONTROL const * Controller(int32_t control) { return m_Controllers[control]; }
    inline CONTROL * PluginControllers(void) { return m_PluginControllers; }

private:
    CControl_Plugin(const CControl_Plugin&);
    CControl_Plugin& operator=(const CControl_Plugin&);

    virtual int32_t GetDefaultSettingStartRange() const { return FirstCtrlDefaultSet; }
    virtual int32_t GetSettingStartRange() const { return FirstCtrlSettings; }
    PLUGIN_TYPE type() { return PLUGIN_TYPE_CONTROLLER; }
    bool LoadFunctions(void);
    void UnloadPluginDetails(void);

    bool m_AllocatedControllers;

    CONTROL m_PluginControllers[4];
    CCONTROL * m_Controllers[4];
};
