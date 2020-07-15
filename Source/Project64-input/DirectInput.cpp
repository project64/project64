#include "DirectInput.h"
#include <Common\StdString.h>

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

        if (m_pDIHandle != nullptr)
        {
            m_pDIHandle->EnumDevices(DI8DEVCLASS_ALL, stEnumMakeDeviceList, this, DIEDFL_ATTACHEDONLY);
        }
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
        else if (DeviceType == DI8DEVTYPE_MOUSE)
        {
            //dwReturn = ScanMouse(&g_devList[i], lpdwCounter, pButton);
        }
        else
        {
           // dwReturn = ScanGamePad(&g_devList[i], lpdwCounter, pButton, i);
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
    else if (Button.BtnType == BTNTYPE_UNASSIGNED)
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
    }
    return false;
}

int8_t CDirectInput::AxisPos(BUTTON & PosBtn, BUTTON & NegBtn, uint8_t Range)
{
    int8_t Pos = 0;
    if (PosBtn.Device != nullptr)
    {
        DEVICE & Device = *(DEVICE *)PosBtn.Device;
        switch (PosBtn.BtnType)
        {
        case BTNTYPE_KEYBUTTON:
            Pos += (Device.State.Keyboard[PosBtn.Offset] & 0x80) != 0 ? 127 : 0;
        }
    }
    if (NegBtn.Device != nullptr)
    {
        DEVICE & Device = *(DEVICE *)NegBtn.Device;
        switch (NegBtn.BtnType)
        {
        case BTNTYPE_KEYBUTTON:
            Pos -= (Device.State.Keyboard[NegBtn.Offset] & 0x80) != 0 ? 127 : 0;
        }
    }

    if (Pos != 0)
    {
        Pos = (int8_t)(Pos * (Range / 100.0));
    }
    return Pos;
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
