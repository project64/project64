#include "DirectInput.h"
#include <Common\StdString.h>

CDirectInput::CDirectInput(HINSTANCE hinst) :
    m_hDirectInputDLL(nullptr),
    m_pDIHandle(nullptr),
    m_hinst(hinst),
    m_hWnd(nullptr)
{
    LoadConfig();
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

    DEVICE Device;
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
            Result = ScanKeyboard(device.didHandle, Button);
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

CDirectInput::ScanResult CDirectInput::ScanKeyboard(LPDIRECTINPUTDEVICE8 didHandle, BUTTON & pButton)
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
        return SCAN_SUCCEED;
    }
    return SCAN_FAILED;
}

void CDirectInput::LoadConfig(void)
{

}
