#pragma once
#include "ControllerSpec1.1.h"
#include "Button.h"
#include "N64Controller.h"
#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <dinput.h>
#include <string>
#include <map>

class CDirectInput 
{
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
    ScanResult ScanDevices(BUTTON & Button);
    std::wstring ButtonAssignment(BUTTON & Button);
    bool IsButtonPressed(BUTTON & Button);
    int8_t AxisPos(BUTTON & PosBtn, BUTTON & NegBtn, uint8_t Range);
    void UpdateDeviceData(void);

private:
    CDirectInput();
    CDirectInput(const CDirectInput&);
    CDirectInput& operator=(const CDirectInput&);

    static BOOL CALLBACK stEnumMakeDeviceList(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);
    BOOL EnumMakeDeviceList(LPCDIDEVICEINSTANCE lpddi);
    ScanResult ScanKeyboard(const GUID & DeviceGuid, LPDIRECTINPUTDEVICE8 didHandle, uint8_t * KeyboardState, BUTTON & pButton);
    bool AcquireDevice(LPDIRECTINPUTDEVICE8 lpDirectInputDevice);
    void LoadConfig(void);

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

    DEVICE_MAP m_Devices;
    HMODULE m_hDirectInputDLL;
    LPDIRECTINPUT8 m_pDIHandle;
    HINSTANCE m_hinst;
    HWND m_hWnd;
};