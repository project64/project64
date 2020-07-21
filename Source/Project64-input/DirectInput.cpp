#include "DirectInput.h"
#include <Common\StdString.h>
#include <Common\SyncEvent.h>

CDirectInput::CDirectInput(HINSTANCE hinst) :
    m_hDirectInputDLL(nullptr),
    m_pDIHandle(nullptr),
    m_hinst(hinst),
    m_hWnd(nullptr)
{
    if (m_hDirectInputDLL == nullptr)
    {
        m_hDirectInputDLL = LoadLibrary(L"dinput8.dll");
    }
    if (m_hDirectInputDLL != nullptr)
    {
        typedef HRESULT(WINAPI *tylpGetDIHandle)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
        tylpGetDIHandle lpGetDIHandle = (tylpGetDIHandle)GetProcAddress(m_hDirectInputDLL, "DirectInput8Create");

        if (lpGetDIHandle != NULL)
        {
            HRESULT hr = lpGetDIHandle(m_hinst, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&m_pDIHandle, NULL);
            if (FAILED(hr))
            {
                return;
            }
        }

        RefreshDeviceList();
    }
}

CDirectInput::~CDirectInput()
{
    for (DEVICE_MAP::iterator itr = m_Devices.begin(); itr != m_Devices.end(); itr++)
    {
        if (itr->second.didHandle != nullptr)
        {
            itr->second.didHandle->Release();
            itr->second.didHandle = NULL;
        }
    }
}

void CDirectInput::Initiate(CONTROL_INFO * ControlInfo)
{
    m_hWnd = (HWND)ControlInfo->hwnd;
}

void CDirectInput::MapControllerDevice(N64CONTROLLER & Controller)
{
    BUTTON * Buttons[] =
    {
        &Controller.U_DPAD,
        &Controller.D_DPAD,
        &Controller.L_DPAD,
        &Controller.R_DPAD,
        &Controller.A_BUTTON,
        &Controller.B_BUTTON,
        &Controller.U_CBUTTON,
        &Controller.D_CBUTTON,
        &Controller.L_CBUTTON,
        &Controller.R_CBUTTON,
        &Controller.START_BUTTON,
        &Controller.Z_TRIG,
        &Controller.R_TRIG,
        &Controller.L_TRIG,
        &Controller.U_ANALOG,
        &Controller.D_ANALOG,
        &Controller.L_ANALOG,
        &Controller.R_ANALOG,
    };

    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        DEVICE_MAP::iterator itr = m_Devices.find(Buttons[i]->DeviceGuid);
        if (itr != m_Devices.end())
        {
            Buttons[i]->Device = &itr->second;
        }
        else
        {
            Buttons[i]->Device = nullptr;
        }
    }
}

BOOL CDirectInput::stEnumMakeDeviceList(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
    return ((CDirectInput *)pvRef)->EnumMakeDeviceList(lpddi);
}

BOOL CDirectInput::EnumMakeDeviceList(LPCDIDEVICEINSTANCE lpddi)
{
    uint32_t DeviceType = GET_DIDEVICE_TYPE(lpddi->dwDevType);
    if (DeviceType == DI8DEVTYPE_DEVICE)
    {
        // ignore generic devices
        return DIENUM_CONTINUE;
    }

    DEVICE_MAP::iterator itr = m_Devices.find(lpddi->guidInstance);
    if (itr != m_Devices.end())
    {
        return DIENUM_CONTINUE;
    }

    DEVICE Device = { 0 };
    Device.didHandle = nullptr;
    Device.dwDevType = lpddi->dwDevType;
    Device.ProductName = stdstr().FromUTF16(lpddi->tszProductName);
    Device.InstanceName = stdstr().FromUTF16(lpddi->tszInstanceName);
    HRESULT hResult = m_pDIHandle->CreateDevice(lpddi->guidInstance, &Device.didHandle, NULL);
    if (!SUCCEEDED(hResult))
    {
        return DIENUM_CONTINUE;
    }

    LPCDIDATAFORMAT ppDiDataFormat = NULL;
    if (DeviceType == DI8DEVTYPE_KEYBOARD)
    {
        ppDiDataFormat = &c_dfDIKeyboard;
    }
    else if (DeviceType == DI8DEVTYPE_MOUSE)
    {
        ppDiDataFormat = &c_dfDIMouse2;
    }
    else
    {
        ppDiDataFormat = &c_dfDIJoystick;
    }
    hResult = Device.didHandle->SetDataFormat(ppDiDataFormat);
    if (!SUCCEEDED(hResult))
    {
        Device.didHandle->Release();
        return DIENUM_CONTINUE;
    }
    hResult = Device.didHandle->SetCooperativeLevel(m_hWnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    if (!SUCCEEDED(hResult))
    {
        Device.didHandle->Release();
        return DIENUM_CONTINUE;
    }

    std::pair<DEVICE_MAP::iterator, bool> res = m_Devices.insert(DEVICE_MAP::value_type(lpddi->guidInstance, Device));
    if (!res.second)
    {
        Device.didHandle->Release();
    }
    return DIENUM_CONTINUE;
}

CDirectInput::ScanResult CDirectInput::ScanDevices(BUTTON & Button)
{
    ScanResult Result = SCAN_FAILED;

    for (DEVICE_MAP::iterator itr = m_Devices.begin(); itr != m_Devices.end(); itr++)
    {
        DEVICE &device = itr->second;
        if (device.didHandle == nullptr)
        {
            continue;
        }
        if (FAILED(device.didHandle->Poll()))
        {
            device.didHandle->Acquire();
        }
    }
    for (DEVICE_MAP::iterator itr = m_Devices.begin(); itr != m_Devices.end(); itr++)
    {
        DEVICE &device = itr->second;
        if (device.didHandle == nullptr)
        {
            continue;
        }
        uint8_t DeviceType = LOBYTE(device.dwDevType);
        if (DeviceType == DI8DEVTYPE_KEYBOARD)
        {
            Result = ScanKeyboard(itr->first, device.didHandle, device.State.Keyboard, Button);
        }
        else if (DeviceType != DI8DEVTYPE_MOUSE)
        {
            Result = ScanGamePad(itr->first, device.didHandle, device.State.Joy, Button);
        }

        if (Result != SCAN_FAILED)
        {
            return Result;
        }
    }
    return Result;
}

std::wstring CDirectInput::ButtonAssignment(BUTTON & Button)
{
    static const char * iGamepad[] = 
    {
        "X-axis",
        "Y-axis",
        "Z-axis",
        "X-rotation",
        "Y-rotation",
        "Z-rotation",
        "Slider",
        "Slider",
        "PoV",
        "PoV",
        "PoV",
        "PoV",
        "Button" 
    };
    static const char * AxeID[] = 
    { 
        " +",
        " -",
        " /\\",
        " >",
        " \\/",
        " <" 
    };

    if (Button.BtnType == BTNTYPE_JOYBUTTON)
    {
        return stdstr_f("Button %u", Button.Offset).ToUTF16();
    }
    if (Button.BtnType == BTNTYPE_JOYAXE)
    {
        stdstr_f Offset("%u", Button.Offset);
        if (Button.Offset < (sizeof(iGamepad) / sizeof(iGamepad[0])))
        {
            Offset = iGamepad[Button.Offset];
        }
        stdstr_f AxisId(" %u", Button.AxisID);
        if (Button.AxisID < (sizeof(AxeID) / sizeof(AxeID[0])))
        {
            AxisId = AxeID[Button.AxisID];
        }
        return stdstr_f("%s%s", Offset.c_str(), AxisId.c_str()).ToUTF16();
    }
    if (Button.BtnType == BTNTYPE_JOYPOV)
    {
        stdstr_f Offset("%u", Button.Offset);
        if (Button.Offset < (sizeof(iGamepad) / sizeof(iGamepad[0])))
        {
            Offset = iGamepad[Button.Offset];
        }
        stdstr_f AxisId(" %u", Button.AxisID);
        if ((Button.AxisID + 2) < (sizeof(AxeID) / sizeof(AxeID[0])))
        {
            AxisId = AxeID[Button.AxisID + 2];
        }
        return stdstr_f("%s%s", Offset.c_str(), AxisId.c_str()).ToUTF16();
    }
    if (Button.BtnType == BTNTYPE_KEYBUTTON)
    {
        DEVICE_MAP::iterator itr = m_Devices.find(GUID_SysKeyboard);
        if (itr != m_Devices.end())
        {
            DIDEVICEOBJECTINSTANCE didoi;
            didoi.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);
            if (itr->second.didHandle->GetObjectInfo(&didoi, Button.Offset, DIPH_BYOFFSET) == DI_OK)
            {
                return didoi.tszName;
            }
            return L"Keyboard: ???";
        }
    }
    if (Button.BtnType == BTNTYPE_UNASSIGNED)
    {
        return L"";
    }
    return L"Unknown";
}

bool CDirectInput::IsButtonPressed(BUTTON & Button)
{
    if (Button.Device == nullptr)
    {
        return false;
    }
    DEVICE & Device = *(DEVICE *)Button.Device;
    switch (Button.BtnType)
    {
    case BTNTYPE_KEYBUTTON:
        return (Device.State.Keyboard[Button.Offset] & 0x80) != 0;
    case BTNTYPE_JOYBUTTON:
        return (Device.State.Joy.rgbButtons[Button.Offset] & 0x80) != 0;
    case BTNTYPE_JOYPOV:
        return JoyPadPovPressed((AI_POV)Button.AxisID, ((uint32_t *)&Device.State.Joy)[Button.Offset]);
    }
    return false;
}

void CDirectInput::GetAxis(N64CONTROLLER & Controller, BUTTONS * Keys)
{
    enum
    {
        N64DIVIDER = 258,
    };

    Keys->X_AXIS = 0;
    Keys->Y_AXIS = 0;

    bool b_Value;
    long l_Value = 0;

    long lAxisValueX = 0;
    long lAxisValueY = 0;

    uint8_t bPadDeadZone = Controller.DeadZone;
    long lDeadZoneValue = bPadDeadZone * RANGE_RELATIVE / 100;
    float fDeadZoneRelation = (float)RANGE_RELATIVE / (float)(RANGE_RELATIVE - lDeadZoneValue);

    struct
    {
        BUTTON & Button;
        bool Negative;
    }
    Buttons[] =
    {
        { Controller.R_ANALOG, false },
        { Controller.L_ANALOG, true },
        { Controller.D_ANALOG, true },
        { Controller.U_ANALOG, false },
    };

    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        bool fNegInput = Buttons[i].Negative;
        BUTTON & btnButton = Buttons[i].Button;
        if (btnButton.Device == nullptr)
        {
            continue;
        }
        DEVICE & Device = *(DEVICE *)btnButton.Device;
        LPLONG plRawState = (LPLONG)&Device.State.Joy;

        switch (btnButton.BtnType)
        {
        case BTNTYPE_JOYSLIDER:
        case BTNTYPE_JOYAXE:
            l_Value = (plRawState[btnButton.Offset] - MAX_AXIS_VALUE) * -1;

            if (btnButton.AxisID == AI_AXE_NEGATIVE)
            {
                fNegInput = !fNegInput;

                b_Value = (l_Value <= -lDeadZoneValue);
                if (b_Value)
                    l_Value = (long)((float)(l_Value + lDeadZoneValue) * fDeadZoneRelation);
            }
            else
            {
                b_Value = (l_Value >= lDeadZoneValue);
                if (b_Value)
                    l_Value = (long)((float)(l_Value - lDeadZoneValue) * fDeadZoneRelation);
            }
            break;
        case BTNTYPE_KEYBUTTON:
            if ((Device.State.Keyboard[btnButton.Offset] & 0x80) != 0)
            {
                b_Value = true;
                l_Value = MAX_AXIS_VALUE;
            }
            else
            {
                b_Value = false;
            }
            break;
        default:
            b_Value = false;
        }

        if (b_Value)
        {
            if (fNegInput)
                l_Value = -l_Value;

            if (i < 2)
                lAxisValueX += l_Value;
            else
                lAxisValueY += l_Value;
        }
    }
    if (lAxisValueX > MAX_AXIS_VALUE) { lAxisValueX = MAX_AXIS_VALUE; }
    if (lAxisValueY > MAX_AXIS_VALUE) { lAxisValueY = MAX_AXIS_VALUE; }
    Keys->X_AXIS = lAxisValueX / N64DIVIDER;
    Keys->Y_AXIS = lAxisValueY / N64DIVIDER;
}

void CDirectInput::UpdateDeviceData(void)
{
    for (DEVICE_MAP::iterator itr = m_Devices.begin(); itr != m_Devices.end(); itr++)
    {
        DEVICE & device = itr->second;
        LPDIRECTINPUTDEVICE8 & didHandle = device.didHandle;
        if (didHandle == nullptr)
        {
            continue;
        }
        if (FAILED(didHandle->Poll()) && !AcquireDevice(didHandle))
        {
            continue;
        }
        
        switch (LOBYTE(device.dwDevType))
        {
        case DI8DEVTYPE_KEYBOARD:
            didHandle->GetDeviceState(sizeof(device.State.Keyboard), &device.State.Keyboard);
            break;
        case DI8DEVTYPE_MOUSE:
            didHandle->GetDeviceState(sizeof(device.State.Mouse), &device.State.Mouse);
            break;
        default:
            didHandle->GetDeviceState(sizeof(device.State.Joy), &device.State.Joy);
        }
    }
}

void CDirectInput::DevicesChanged(void)
{
    RefreshDeviceList();
}

void CDirectInput::RefreshDeviceList(void)
{
    if (m_pDIHandle != nullptr)
    {
        m_pDIHandle->EnumDevices(DI8DEVCLASS_ALL, stEnumMakeDeviceList, this, DIEDFL_ATTACHEDONLY);
    }
}

CDirectInput::ScanResult CDirectInput::ScanKeyboard(const GUID & DeviceGuid, LPDIRECTINPUTDEVICE8 didHandle, uint8_t * KeyboardState, BUTTON & pButton)
{
    if (didHandle == nullptr)
    {
        return SCAN_FAILED;
    }
    uint8_t cKeys[256];
    HRESULT hr = didHandle->GetDeviceState(sizeof(cKeys), cKeys);
    if (FAILED(hr))
    {
        didHandle->Acquire();
        return SCAN_FAILED;
    }

    for (size_t i = 0, n = sizeof(cKeys) / sizeof(cKeys[0]); i < n; i++)
    {
        if (KeyboardState[i] == cKeys[i])
        {
            continue;
        }
        KeyboardState[i] = cKeys[i];
        if ((cKeys[i] & 0x80) == 0)
        {
            continue;
        }
        if (i == DIK_ESCAPE)
        {
            return SCAN_ESCAPE;
        }
        pButton.Offset = (uint8_t)i;
        pButton.AxisID = 0;
        pButton.BtnType = BTNTYPE_KEYBUTTON;
        pButton.DeviceGuid = DeviceGuid;
        pButton.Device = nullptr;
        return SCAN_SUCCEED;
    }
    return SCAN_FAILED;
}

CDirectInput::ScanResult CDirectInput::ScanGamePad(const GUID & DeviceGuid, LPDIRECTINPUTDEVICE8 didHandle, DIJOYSTATE & BaseState, BUTTON & pButton)
{
    DIJOYSTATE JoyState = { 0 };
    HRESULT hr = didHandle->GetDeviceState(sizeof(DIJOYSTATE), &JoyState);
    if (FAILED(hr))
    {
        didHandle->Acquire();
        return SCAN_FAILED;
    }

    uint32_t JoyPad[][2] = 
    { 
        { FIELD_OFFSET(DIJOYSTATE, lX) / sizeof(uint32_t), BTNTYPE_JOYAXE },
        { FIELD_OFFSET(DIJOYSTATE, lY) / sizeof(uint32_t), BTNTYPE_JOYAXE },
        { FIELD_OFFSET(DIJOYSTATE, lZ) / sizeof(uint32_t), BTNTYPE_JOYAXE },
        { FIELD_OFFSET(DIJOYSTATE, lRx) / sizeof(uint32_t), BTNTYPE_JOYAXE },
        { FIELD_OFFSET(DIJOYSTATE, lRy) / sizeof(uint32_t), BTNTYPE_JOYAXE },
        { FIELD_OFFSET(DIJOYSTATE, lRz) / sizeof(uint32_t), BTNTYPE_JOYAXE },
        { FIELD_OFFSET(DIJOYSTATE, rglSlider[0]) / sizeof(uint32_t), BTNTYPE_JOYSLIDER },
        { FIELD_OFFSET(DIJOYSTATE, rglSlider[1]) / sizeof(uint32_t), BTNTYPE_JOYSLIDER },
        { FIELD_OFFSET(DIJOYSTATE, rgdwPOV[0]) / sizeof(uint32_t), BTNTYPE_JOYPOV },
        { FIELD_OFFSET(DIJOYSTATE, rgdwPOV[1]) / sizeof(uint32_t), BTNTYPE_JOYPOV },
        { FIELD_OFFSET(DIJOYSTATE, rgdwPOV[2]) / sizeof(uint32_t), BTNTYPE_JOYPOV },
        { FIELD_OFFSET(DIJOYSTATE, rgdwPOV[3]) / sizeof(uint32_t), BTNTYPE_JOYPOV }
    };

    uint8_t bAxeDirection = 0;
    int32_t foundJoyPad = -1;
    for (int32_t i = 0, n = sizeof(JoyPad) / sizeof(JoyPad[0]); i < n; i++)
    {
        uint32_t lValue = ((int32_t*)&JoyState)[JoyPad[i][0]];
        uint32_t BaseValue = ((int32_t*)&BaseState)[JoyPad[i][0]];
        if (lValue == BaseValue)
        {
            continue;
        }
        ((int32_t*)&(BaseState))[JoyPad[i][0]] = lValue;

        if ((JoyPad[i][1] == BTNTYPE_JOYAXE) || (JoyPad[i][1] == BTNTYPE_JOYSLIDER))
        {
            enum
            {
                AXIS_TOP_VALUE = MAX_AXIS_VALUE / 2,
                AXIS_BOTTOM_VALUE = MAX_AXIS_VALUE + AXIS_TOP_VALUE,
            };
            if (lValue < AXIS_TOP_VALUE && BaseValue >= AXIS_TOP_VALUE)
            {
                bAxeDirection = AI_AXE_POSITIVE;
                foundJoyPad = i;
                break;
            }
            else if (lValue > AXIS_BOTTOM_VALUE && BaseValue <= AXIS_BOTTOM_VALUE)
            {
                bAxeDirection = AI_AXE_NEGATIVE;
                foundJoyPad = i;
                break;
            }
        }
        if (JoyPad[i][1] == BTNTYPE_JOYPOV)
        {            
            AI_POV pov[] =
            {
                AI_POV_UP,
                AI_POV_DOWN,
                AI_POV_LEFT,
                AI_POV_RIGHT,
            };

            for (size_t p = 0; p < (sizeof(pov) / sizeof(pov[0])); p++)
            {
                if (JoyPadPovPressed(pov[p], lValue) && !JoyPadPovPressed(pov[p], BaseValue))
                {
                    bAxeDirection = (uint8_t)pov[p];
                    foundJoyPad = i;
                    break;
                }
            }
            if (foundJoyPad >= 0)
            {
                break;
            }
        }
    }

    if (foundJoyPad >= 0)
    {    
        pButton.Offset = (uint8_t)JoyPad[foundJoyPad][0];
        pButton.AxisID = (uint8_t)bAxeDirection;
        pButton.BtnType = (BtnType)JoyPad[foundJoyPad][1];
        pButton.DeviceGuid = DeviceGuid;
        pButton.Device = nullptr;
        return SCAN_SUCCEED;
    }

    for (uint8_t i = 0, n = sizeof(JoyState.rgbButtons) / sizeof(JoyState.rgbButtons[0]); i < n; i++)
    {
        if (BaseState.rgbButtons[i] == JoyState.rgbButtons[i])
        {
            continue;
        }
        BaseState.rgbButtons[i] = JoyState.rgbButtons[i];

        if ((JoyState.rgbButtons[i] & 0x80) == 0)
        {
            continue;
        }
        pButton.Offset = i;
        pButton.AxisID = 0;
        pButton.BtnType = BTNTYPE_JOYBUTTON;
        pButton.DeviceGuid = DeviceGuid;
        pButton.Device = nullptr;
        return SCAN_SUCCEED;
    }
    return SCAN_FAILED;
}

bool CDirectInput::AcquireDevice(LPDIRECTINPUTDEVICE8 lpDirectInputDevice)
{
    HRESULT hResult = lpDirectInputDevice->Acquire();
    if (hResult == DIERR_INPUTLOST)
    {
        for (uint32_t i = 0; i < 10; i++)
        {
            hResult = lpDirectInputDevice->Acquire();
            if (hResult != DIERR_INPUTLOST)
            {
                break;
            }
        }
    }        
    if (SUCCEEDED(hResult))
    {
        lpDirectInputDevice->Poll();
        return true;
    }
    return false;
}

bool CDirectInput::JoyPadPovPressed(AI_POV Pov, int32_t Angle)
{
    enum
    {
        POV_ANGLE_THRESH = 5675
    };

    if (LOWORD(Angle) == 0xFFFF)
    {
        return false;
    }

    switch (Pov)
    {
    case AI_POV_UP:
        return ((Angle >= 36000 - POV_ANGLE_THRESH) || (Angle <= 0 + POV_ANGLE_THRESH));
    case AI_POV_RIGHT:
        return ((Angle >= 9000 - POV_ANGLE_THRESH) && (Angle <= 9000 + POV_ANGLE_THRESH));
    case AI_POV_DOWN:
        return ((Angle >= 18000 - POV_ANGLE_THRESH) && (Angle <= 18000 + POV_ANGLE_THRESH));
    case AI_POV_LEFT:
        return ((Angle >= 27000 - POV_ANGLE_THRESH) && (Angle <= 27000 + POV_ANGLE_THRESH));
    }
    return false;
}
