#include "CProject64Input.h"

CProject64Input * g_InputPlugin = nullptr;

CProject64Input::CProject64Input(HINSTANCE hinst) :
    m_hinst(hinst),
    m_Scanning(false),
    m_DisplayCtrlId(0)
{
    memset(m_Controllers, 0, sizeof(m_Controllers));
}

CProject64Input::~CProject64Input()
{
}

void CProject64Input::InitiateControllers(CONTROL_INFO * ControlInfo)
{
    if (m_DirectInput.get() == NULL)
    {
        m_DirectInput.reset(new CDirectInput(m_hinst));
    }
    m_DirectInput->Initiate(ControlInfo);
}

void CProject64Input::StartScanDevices(int32_t DisplayCtrlId)
{
    m_Scanning = true;
    m_DisplayCtrlId = DisplayCtrlId;
}

void CProject64Input::EndScanDevices(void)
{
    m_Scanning = false;
    m_DisplayCtrlId = 0;
}

CDirectInput::ScanResult CProject64Input::ScanDevices(BUTTON & Button)
{
    CDirectInput::ScanResult Result = CDirectInput::SCAN_FAILED;
    if (m_DirectInput.get() != NULL)
    {
        Result = m_DirectInput->ScanDevices(Button);
    }
    return Result;
}

std::wstring CProject64Input::ButtonAssignment(BUTTON & Button)
{
    if (m_DirectInput.get() != NULL)
    {
        return m_DirectInput->ButtonAssignment(Button);
    }
    return L"Unknown";
}
