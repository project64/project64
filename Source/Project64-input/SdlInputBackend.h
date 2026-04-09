#pragma once
#include <Project64-plugin-spec/Input.h>
#include "Button.h"
#include "DeviceNotification.h"
#include "N64Controller.h"
#include "Shortcuts.h"
#include <Common/CriticalSection.h>
#include <Windows.h>
#include <SDL.h>
#include <string>
#include <map>

#pragma pack(push, 8)
typedef struct
{
    LONG lX;
    LONG lY;
    LONG lZ;
    LONG lRx;
    LONG lRy;
    LONG lRz;
    LONG rglSlider[2];
    DWORD rgdwPOV[4];
    BYTE rgbButtons[32];
} DIJOYSTATE_COMPAT;

typedef struct
{
    LONG lX;
    LONG lY;
    LONG lZ;
    BYTE rgbButtons[8];
} DIMOUSESTATE2_COMPAT;
#pragma pack(pop)

class CSdlInput
{
    enum
    {
        CONFIG_THRESHOLD = 50,
        MIN_AXIS_VALUE = -32767,
        MAX_AXIS_VALUE = 32767,
        AXIS_TOP_VALUE = MAX_AXIS_VALUE / 2,
        AXIS_BOTTOM_VALUE = MAX_AXIS_VALUE + AXIS_TOP_VALUE,
        RANGE_RELATIVE = 0x8000,
        AI_AXE_POSITIVE = 0,
        AI_AXE_NEGATIVE = 1,
        THRESHOLD = 50,
        MOUSESCALEVALUE = 10,
    };

    enum AI_POV
    {
        AI_POV_UP = 0,
        AI_POV_RIGHT = 1,
        AI_POV_DOWN = 2,
        AI_POV_LEFT = 3,
    };

public:
    enum ScanResult
    {
        SCAN_FAILED = 0x00,
        SCAN_SUCCEED = 0x01,
        SCAN_ESCAPE = 0x10,
    };

    CSdlInput(HINSTANCE hinst);
    ~CSdlInput();

    void Initiate(CONTROL_INFO * ControlInfo);
    void MapControllerDevice(N64CONTROLLER & Controller);
    void MapShortcutDevice(SHORTCUTS& Shortcuts);
    ScanResult ScanDevices(BUTTON & Button);
    std::wstring ButtonAssignment(BUTTON & Button);
    std::wstring ControllerDevices(const N64CONTROLLER & Controller);
    bool IsButtonPressed(BUTTON & Button);
    void GetAxis(N64CONTROLLER & Controller, BUTTONS * Keys);
    void UpdateDeviceData(void);
    void DevicesChanged(void);

    void NotifyConfigDialogOpen(bool open);
    void NotifyScanActive(bool active);
    void NotifyRomOpen(bool open);

private:
    CSdlInput();
    CSdlInput(const CSdlInput&);
    CSdlInput& operator=(const CSdlInput&);

    struct DEVICE_ENTRY
    {
        uint32_t dwDevType;
        std::string InstanceName;
        std::string ProductName;
        SDL_Joystick * joystick;
        SDL_GameController * gamecontroller;
        int sdlJoystickIndex;
        union INPUTSTATE
        {
            DIJOYSTATE_COMPAT Joy;
            DIMOUSESTATE2_COMPAT Mouse;
            uint8_t Keyboard[512];
        } State;
    };

    void RefreshDeviceList(void);
    void CloseJoystickDevices(void);
    void SyncJoystickState(DEVICE_ENTRY & dev);
    static void FillJoyState(const DEVICE_ENTRY & dev, DIJOYSTATE_COMPAT & j);

    ScanResult ScanKeyboard(const GUID & DeviceGuid, uint8_t * KeyboardState, BUTTON & pButton);
    ScanResult ScanGamePad(const GUID & DeviceGuid, DIJOYSTATE_COMPAT & BaseState, BUTTON & pButton);
    ScanResult ScanMouse(const GUID& DeviceGuid, DIMOUSESTATE2_COMPAT& BaseState, BUTTON& pButton);
    bool JoyPadPovPressed(AI_POV Pov, int32_t Angle);

    struct GUIDComparer
    {
        bool operator()(const GUID & Left, const GUID & Right) const
        {
            return memcmp(&Left, &Right, sizeof(Right)) < 0;
        }
    };
    typedef std::map<GUID, DEVICE_ENTRY, GUIDComparer> DEVICE_MAP;

    static GUID GuidFromSdlJoystick(SDL_JoystickID id);
    static void MakeKeyboardMouseGuids(GUID * outKeyboard, GUID * outMouse);

    void EnsurePumpWindow(void);
    void DestroyPumpWindow(void);
    void ApplyPumpTimerInterval(void);
    static LRESULT CALLBACK PumpWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    DeviceNotification m_DeviceNotification;
    DEVICE_MAP m_Devices;
    CriticalSection m_DeviceCS;
    HINSTANCE m_hinst;
    HWND m_hWnd;
    HWND m_PumpHwnd;
    bool m_SdlInited;
    bool m_ConfigDialogOpen;
    bool m_ScanActive;
    bool m_RomOpen;
    GUID m_GuidKeyboard;
    GUID m_GuidMouse;
    LONG m_MouseAccumX;
    LONG m_MouseAccumY;
    LONG m_MouseAccumZ;
};
