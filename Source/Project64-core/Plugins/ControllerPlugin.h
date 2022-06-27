#pragma once
#include <Project64-core/Plugins/PluginBase.h>
#include <Project64-plugin-spec/Input.h>

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
