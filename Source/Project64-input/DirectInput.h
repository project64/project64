#pragma once
#include "ControllerSpec1.1.h"
#include "Button.h"
#include "DeviceNotification.h"
#include "N64Controller.h"
#include "Shortcuts.h"
#include <Common/CriticalSection.h>
#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <dinput.h>
#include <string>
#include <map>

class CDirectInput 
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

    CDirectInput(HINSTANCE hinst);
    ~CDirectInput();

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
    void LockDevice(bool set, const N64CONTROLLER & Controller);

private:
    CDirectInput();
    CDirectInput(const CDirectInput&);
    CDirectInput& operator=(const CDirectInput&);

    static BOOL CALLBACK stEnumMakeDeviceList(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);
    BOOL EnumMakeDeviceList(LPCDIDEVICEINSTANCE lpddi);
    ScanResult ScanKeyboard(const GUID & DeviceGuid, LPDIRECTINPUTDEVICE8 didHandle, uint8_t * KeyboardState, BUTTON & pButton);
    ScanResult ScanGamePad(const GUID & DeviceGuid, LPDIRECTINPUTDEVICE8 didHandle, DIJOYSTATE & BaseState, BUTTON & pButton);
    ScanResult ScanMouse(const GUID& DeviceGuid, LPDIRECTINPUTDEVICE8 didHandle, DIMOUSESTATE2& BaseState, BUTTON& pButton);
    bool AcquireDevice(LPDIRECTINPUTDEVICE8 lpDirectInputDevice);
    void RefreshDeviceList(void);
    bool JoyPadPovPressed(AI_POV Pov, int32_t Angle);

    typedef struct
    {
        LPDIRECTINPUTDEVICE8 didHandle;
        uint32_t dwDevType;
        std::string InstanceName;
        std::string ProductName;
        union INPUTSTATE
        {
            DIJOYSTATE Joy;
            DIMOUSESTATE2 Mouse;
            uint8_t Keyboard[256];
        } State;
    } DEVICE;

    struct GUIDComparer
    {
        bool operator()(const GUID & Left, const GUID & Right) const
        {
            return memcmp(&Left, &Right, sizeof(Right)) < 0;
        }
    };
    typedef std::map<GUID, DEVICE, GUIDComparer> DEVICE_MAP;

    DeviceNotification m_DeviceNotification;
    DEVICE_MAP m_Devices;
    CriticalSection m_DeviceCS;
    HMODULE m_hDirectInputDLL;
    LPDIRECTINPUT8 m_pDIHandle;
    HINSTANCE m_hinst;
    HWND m_hWnd;
};
