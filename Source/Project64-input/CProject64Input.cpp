#include "CProject64Input.h"
#include "InputSettings.h"

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
    CGuard guard(m_CS);
    if (m_DirectInput.get() == NULL)
    {
        m_DirectInput.reset(new CDirectInput(m_hinst));
    }
    m_DirectInput->Initiate(ControlInfo);
    for (size_t i = 0, n = sizeof(m_Controllers) / sizeof(m_Controllers[0]); i < n; i++)
    {
        g_Settings->LoadController(0, m_Controllers[i]);
        m_DirectInput->MapControllerDevice(m_Controllers[i]);
    }
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

bool CProject64Input::SaveController(uint32_t ControlIndex)
{
    if (ControlIndex >= sizeof(m_Controllers) / sizeof(m_Controllers[0]))
    {
        return false;
    }
    g_Settings->SaveController(ControlIndex, m_Controllers[ControlIndex]);
    return true;
}
