#include "DirectInput.h"
#include <Common/StdString.h>
#include <Common/SyncEvent.h>
#include <set>
#include "CProject64Input.h"

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

        if (lpGetDIHandle != nullptr)
        {
            HRESULT hr = lpGetDIHandle(m_hinst, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&m_pDIHandle, nullptr);
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
    CGuard Guard(m_DeviceCS);

    for (DEVICE_MAP::iterator itr = m_Devices.begin(); itr != m_Devices.end(); itr++)
    {
        if (itr->second.didHandle != nullptr)
        {
            itr->second.didHandle->Release();
            itr->second.didHandle = nullptr;
        }
    }
}

void CDirectInput::Initiate(CONTROL_INFO * ControlInfo)
{
    m_hWnd = (HWND)ControlInfo->hWnd;
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

    CGuard Guard(m_DeviceCS);
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

void CDirectInput::MapShortcutDevice(SHORTCUTS& Shortcuts)
{
    BUTTON* Buttons[] =
    {
        &Shortcuts.LOCKMOUSE,
    };

    CGuard Guard(m_DeviceCS);
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
        // Ignore generic devices
        return DIENUM_CONTINUE;
    }

    {
        CGuard Guard(m_DeviceCS);
        DEVICE_MAP::iterator itr = m_Devices.find(lpddi->guidInstance);
        if (itr != m_Devices.end())
        {
            return DIENUM_CONTINUE;
        }
    }

    DEVICE Device = { 0 };
    Device.didHandle = nullptr;
    Device.dwDevType = lpddi->dwDevType;
    Device.ProductName = stdstr().FromUTF16(lpddi->tszProductName);
    Device.InstanceName = stdstr().FromUTF16(lpddi->tszInstanceName);
    HRESULT hResult = m_pDIHandle->CreateDevice(lpddi->guidInstance, &Device.didHandle, nullptr);
    if (!SUCCEEDED(hResult))
    {
        return DIENUM_CONTINUE;
    }

    LPCDIDATAFORMAT ppDiDataFormat = nullptr;
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

    {
        CGuard Guard(m_DeviceCS);
        std::pair<DEVICE_MAP::iterator, bool> res = m_Devices.insert(DEVICE_MAP::value_type(lpddi->guidInstance, Device));
        if (!res.second)
        {
            Device.didHandle->Release();
        }
    }
    g_InputPlugin->DeviceAdded();
    return DIENUM_CONTINUE;
}

CDirectInput::ScanResult CDirectInput::ScanDevices(BUTTON & Button)
{
    ScanResult Result = SCAN_FAILED;

    CGuard Guard(m_DeviceCS);
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
    static const char* iMouse[] =
    {
        "X-axis",
        "Y-axis",
        "Z-axis",
        "Button"
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
    if (Button.BtnType == BTNTYPE_MOUSEAXE)
    {
        stdstr_f Offset("%u", Button.Offset);
        if (Button.Offset < (sizeof(iMouse) / sizeof(iMouse[0])))
        {
            Offset = iMouse[Button.Offset];
        }
        stdstr_f AxisId(" %u", Button.AxisID);
        if (Button.AxisID < (sizeof(AxeID) / sizeof(AxeID[0])))
        {
            AxisId = AxeID[Button.AxisID];
        }
        return stdstr_f("%s%s", Offset.c_str(), AxisId.c_str()).ToUTF16();
    }
    if (Button.BtnType == BTNTYPE_MOUSEBUTTON)
    {
        return stdstr_f("Button %u", Button.Offset).ToUTF16();
    }
    if (Button.BtnType == BTNTYPE_UNASSIGNED)
    {
        return L"";
    }
    return L"Unknown";
}

std::wstring CDirectInput::ControllerDevices(const N64CONTROLLER & Controller)
{
    const BUTTON * Buttons[] =
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
    typedef std::set<GUID, GUIDComparer> GUID_LIST;
    GUID_LIST DeviceGuids;
    GUID EmptyGuid = { 0 };

    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        GUID_LIST::iterator itr = DeviceGuids.find(Buttons[i]->DeviceGuid);
        if (itr != DeviceGuids.end())
        {
            continue;
        }
        if (memcmp(&Buttons[i]->DeviceGuid, &EmptyGuid, sizeof(EmptyGuid)) == 0)
        {
            continue;
        }
        DeviceGuids.insert(Buttons[i]->DeviceGuid);
    }

    std::wstring DeviceList;
    CGuard Guard(m_DeviceCS);
    bool UnknownDevice = false;
    for (GUID_LIST::iterator itr = DeviceGuids.begin(); itr != DeviceGuids.end(); itr++)
    {
        DEVICE_MAP::iterator DeviceItr = m_Devices.find(*itr);
        if (DeviceItr == m_Devices.end())
        {
            UnknownDevice = true;
            continue;
        }
        if (!DeviceList.empty()) { DeviceList += L", "; }
        DeviceList += stdstr(DeviceItr->second.ProductName).ToUTF16();
    }
    if (UnknownDevice)
    {
        if (!DeviceList.empty()) { DeviceList += L", "; }
        DeviceList += L"Unknown Device";
    }
    return DeviceList;
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
    case BTNTYPE_MOUSEBUTTON:
        return (Device.State.Mouse.rgbButtons[Button.Offset] & 0x80) != 0;
    case BTNTYPE_JOYPOV:
        return JoyPadPovPressed((AI_POV)Button.AxisID, ((uint32_t *)&Device.State.Joy)[Button.Offset]);
    case BTNTYPE_JOYSLIDER:
    case BTNTYPE_JOYAXE:
        return Button.AxisID ? ((uint32_t*)&Device.State.Joy)[Button.Offset] > AXIS_BOTTOM_VALUE : ((uint32_t *)&Device.State.Joy)[Button.Offset] < AXIS_TOP_VALUE;
    case BTNTYPE_MOUSEAXE:
        return Button.AxisID ? ((uint32_t*)&Device.State.Mouse)[Button.Offset] > AXIS_BOTTOM_VALUE : ((uint32_t*)&Device.State.Mouse)[Button.Offset] < AXIS_TOP_VALUE;
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
        BUTTON & Button = Buttons[i].Button;
        if (Button.Device == nullptr)
        {
            continue;
        }
        DEVICE & Device = *(DEVICE *)Button.Device;
        LPLONG plRawState = (LPLONG)&Device.State.Joy;
        LPLONG plRawStateMouse = (LPLONG)&Device.State.Mouse;

        switch (Button.BtnType)
        {
        case BTNTYPE_JOYSLIDER:
        case BTNTYPE_JOYAXE:
            l_Value = (plRawState[Button.Offset] - MAX_AXIS_VALUE) * -1;

            if (Button.AxisID == AI_AXE_NEGATIVE)
            {
                fNegInput = !fNegInput;
                b_Value = (l_Value < 0);
            }
            else
            {
                b_Value = (l_Value > 0);
            }
            break;
        case BTNTYPE_MOUSEAXE:
            l_Value = (plRawStateMouse[Button.Offset]) * -1;
            l_Value *= Controller.Sensitivity * MOUSESCALEVALUE;

            if (Button.AxisID == AI_AXE_NEGATIVE)
            {
                fNegInput = !fNegInput;
                b_Value = (l_Value < 0);
            }
            else
            {
                b_Value = (l_Value > 0);
            }
            break;
        case BTNTYPE_KEYBUTTON:
            b_Value = (Device.State.Keyboard[Button.Offset] & 0x80) != 0;
            if (b_Value)
            {
                l_Value = MAX_AXIS_VALUE;
            }
            break;
        case BTNTYPE_JOYBUTTON:
            b_Value = (Device.State.Joy.rgbButtons[Button.Offset] & 0x80) != 0;
            if (b_Value)
            {
                l_Value = MAX_AXIS_VALUE;
            }
            break;
        case BTNTYPE_MOUSEBUTTON:
            b_Value = (Device.State.Mouse.rgbButtons[Button.Offset] & 0x80) != 0;
            if (b_Value)
            {
                l_Value = MAX_AXIS_VALUE;
            }
            break;
        case BTNTYPE_JOYPOV:
            b_Value = JoyPadPovPressed((AI_POV)Button.AxisID, ((uint32_t *)&Device.State.Joy)[Button.Offset]);
            if (b_Value)
            {
                l_Value = MAX_AXIS_VALUE;
            }
            break;
        default:
            b_Value = false;
        }

        if (b_Value)
        {
            l_Value = fNegInput ? -l_Value : l_Value;
            if (i < 2)
            {
                lAxisValueX += l_Value;
            }
            else
            {
                lAxisValueY += l_Value;
            }
        }
    }

    long lAbsoluteX = (lAxisValueX > 0) ? lAxisValueX : -lAxisValueX;
    long lAbsoluteY = (lAxisValueY > 0) ? lAxisValueY : -lAxisValueY;


    if (lAbsoluteX * lAbsoluteX + lAbsoluteY * lAbsoluteY > lDeadZoneValue * lDeadZoneValue)
    {
        double dMagnitudeDiagonal = sqrt((double)lAbsoluteX * lAbsoluteX + (double)lAbsoluteY * lAbsoluteY);
        double dRel = ((dMagnitudeDiagonal - lDeadZoneValue) / dMagnitudeDiagonal * fDeadZoneRelation);
        lAxisValueX = (long)(lAxisValueX * dRel);
        lAxisValueY = (long)(lAxisValueY * dRel);
    }
    else
    {
        lAxisValueX = lAxisValueY = 0;
    }

    if (Controller.RealN64Range && (lAxisValueX || lAxisValueY))
    {
        long lRangeX = lAbsoluteX > lAbsoluteY ? MAX_AXIS_VALUE : MAX_AXIS_VALUE * lAbsoluteX / lAbsoluteY;
        long lRangeY = lAbsoluteX > lAbsoluteY ? MAX_AXIS_VALUE * lAbsoluteY / lAbsoluteX : MAX_AXIS_VALUE;

        double dRangeDiagonal = sqrt((double)(lRangeX * lRangeX + lRangeY * lRangeY));
        double dRel = MAX_AXIS_VALUE / dRangeDiagonal;
        lAxisValueX = (long)(lAxisValueX * dRel);
        lAxisValueY = (long)(lAxisValueY * dRel);
    }
    if (lAxisValueX > MAX_AXIS_VALUE) { lAxisValueX = MAX_AXIS_VALUE; }
    if (lAxisValueX < MIN_AXIS_VALUE) { lAxisValueX = MIN_AXIS_VALUE; }
    if (lAxisValueY > MAX_AXIS_VALUE) { lAxisValueY = MAX_AXIS_VALUE; }
    if (lAxisValueY < MIN_AXIS_VALUE) { lAxisValueY = MIN_AXIS_VALUE; }
    Keys->X_AXIS = lAxisValueX / N64DIVIDER;
    Keys->Y_AXIS = lAxisValueY / N64DIVIDER;
}

void CDirectInput::UpdateDeviceData(void)
{
    CGuard Guard(m_DeviceCS);
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

        if ((JoyPad[i][1] == BTNTYPE_JOYAXE) || (JoyPad[i][1] == BTNTYPE_JOYSLIDER))
        {
            if ((lValue < AXIS_TOP_VALUE && BaseValue < AXIS_TOP_VALUE) || (lValue > AXIS_BOTTOM_VALUE && BaseValue > AXIS_BOTTOM_VALUE))
            {
                continue;
            }
            ((int32_t*)&(BaseState))[JoyPad[i][0]] = lValue;
            if (lValue < AXIS_TOP_VALUE)
            {
                bAxeDirection = AI_AXE_POSITIVE;
                foundJoyPad = i;
                break;
            }
            else if (lValue > AXIS_BOTTOM_VALUE)
            {
                bAxeDirection = AI_AXE_NEGATIVE;
                foundJoyPad = i;
                break;
            }
        }
        else
        {
            if (lValue == BaseValue)
            {
                continue;
            }
            ((int32_t*)&(BaseState))[JoyPad[i][0]] = lValue;
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

CDirectInput::ScanResult CDirectInput::ScanMouse(const GUID& DeviceGuid, LPDIRECTINPUTDEVICE8 didHandle, DIMOUSESTATE2& BaseState, BUTTON& pButton)
{
    DIJOYSTATE MouseState = { 0 };
    HRESULT hr = didHandle->GetDeviceState(sizeof(DIMOUSESTATE2), &MouseState);
    if (FAILED(hr))
    {
        didHandle->Acquire();
        return SCAN_FAILED;
    }

    uint32_t Mouse[][2] =
    {
        { DIMOFS_X / sizeof(uint32_t), BTNTYPE_MOUSEAXE },
        { DIMOFS_Y / sizeof(uint32_t), BTNTYPE_MOUSEAXE },
        { DIMOFS_Z / sizeof(uint32_t), BTNTYPE_MOUSEAXE }
    };

    uint8_t bAxeDirection = 0;
    int32_t foundJoyPad = -1;

    for (int32_t i = 0, n = sizeof(Mouse) / sizeof(Mouse[0]); i < n; i++)
    {
        uint32_t lValue = ((int32_t*)&MouseState)[Mouse[i][0]];
        uint32_t BaseValue = ((int32_t*)&BaseState)[Mouse[i][0]];

        if (Mouse[i][1] == BTNTYPE_MOUSEAXE)
        {
            if ((lValue < AXIS_TOP_VALUE && BaseValue < AXIS_TOP_VALUE) || (lValue > AXIS_BOTTOM_VALUE && BaseValue > AXIS_BOTTOM_VALUE))
            {
                continue;
            }
            ((int32_t*)&(BaseState))[Mouse[i][0]] = lValue;
            if (lValue < AXIS_TOP_VALUE)
            {
                bAxeDirection = AI_AXE_POSITIVE;
                foundJoyPad = i;
                break;
            }
            else if (lValue > AXIS_BOTTOM_VALUE)
            {
                bAxeDirection = AI_AXE_NEGATIVE;
                foundJoyPad = i;
                break;
            }
        }
        else
        {
            if (lValue == BaseValue)
            {
                continue;
            }
            ((int32_t*)&(BaseState))[Mouse[i][0]] = lValue;
        }
    }

    if (foundJoyPad >= 0)
    {
        pButton.Offset = (uint8_t)Mouse[foundJoyPad][0];
        pButton.AxisID = (uint8_t)bAxeDirection;
        pButton.BtnType = (BtnType)Mouse[foundJoyPad][1];
        pButton.DeviceGuid = DeviceGuid;
        pButton.Device = nullptr;
        return SCAN_SUCCEED;
    }

    for (uint8_t i = 0, n = sizeof(MouseState.rgbButtons) / sizeof(MouseState.rgbButtons[0]); i < n; i++)
    {
        if (BaseState.rgbButtons[i] == MouseState.rgbButtons[i])
        {
            continue;
        }
        BaseState.rgbButtons[i] = MouseState.rgbButtons[i];

        if ((MouseState.rgbButtons[i] & 0x80) == 0)
        {
            continue;
        }
        pButton.Offset = i;
        pButton.AxisID = 0;
        pButton.BtnType = BTNTYPE_MOUSEBUTTON;
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
