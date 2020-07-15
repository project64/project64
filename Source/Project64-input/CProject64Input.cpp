#include "CProject64Input.h"
#include "InputSettings.h"

CProject64Input * g_InputPlugin = nullptr;

CProject64Input::CProject64Input(HINSTANCE hinst) :
    m_hinst(hinst),
    m_Scanning(false),
    m_DisplayCtrlId(0),
    m_iFirstController(-1)
{
    memset(m_Controllers, 0, sizeof(m_Controllers));
}

CProject64Input::~CProject64Input()
{
}

void CProject64Input::DevicesChanged(void)
{
    CGuard guard(m_CS);
    if (m_DirectInput.get() != nullptr)
    {
        m_DirectInput->DevicesChanged();
    }
}

void CProject64Input::InitiateControllers(CONTROL_INFO * ControlInfo)
{
    CGuard guard(m_CS);
    m_ControlInfo = *ControlInfo;
    if (m_DirectInput.get() == NULL)
    {
        m_DirectInput.reset(new CDirectInput(m_hinst));
    }
    m_DirectInput->Initiate(ControlInfo);
    m_iFirstController = -1;
    for (size_t i = 0, n = sizeof(m_Controllers) / sizeof(m_Controllers[0]); i < n; i++)
    {
        g_Settings->LoadController(i, m_ControlInfo.Controls[i], m_Controllers[i]);
        m_DirectInput->MapControllerDevice(m_Controllers[i]);
        if (m_ControlInfo.Controls[i].Present != 0 && m_iFirstController < 0)
        {
            m_iFirstController = i;
        }
    }
}

void CProject64Input::GetKeys(int32_t Control, BUTTONS * Keys)
{
    if (Control >= sizeof(m_Controllers) / sizeof(m_Controllers[0]))
    {
        return;
    }
    CGuard guard(m_CS);
    if (Control == m_iFirstController)
    {
        m_DirectInput->UpdateDeviceData();
    }
    N64CONTROLLER & Controller = m_Controllers[Control];
    Keys->R_DPAD = m_DirectInput->IsButtonPressed(Controller.R_DPAD);
    Keys->L_DPAD = m_DirectInput->IsButtonPressed(Controller.L_DPAD);
    Keys->D_DPAD = m_DirectInput->IsButtonPressed(Controller.D_DPAD);
    Keys->U_DPAD = m_DirectInput->IsButtonPressed(Controller.U_DPAD);
    Keys->START_BUTTON = m_DirectInput->IsButtonPressed(Controller.START_BUTTON);
    Keys->Z_TRIG = m_DirectInput->IsButtonPressed(Controller.Z_TRIG);
    Keys->B_BUTTON = m_DirectInput->IsButtonPressed(Controller.B_BUTTON);
    Keys->A_BUTTON = m_DirectInput->IsButtonPressed(Controller.A_BUTTON);
    Keys->R_CBUTTON = m_DirectInput->IsButtonPressed(Controller.R_CBUTTON);
    Keys->L_CBUTTON = m_DirectInput->IsButtonPressed(Controller.L_CBUTTON);
    Keys->D_CBUTTON = m_DirectInput->IsButtonPressed(Controller.D_CBUTTON);
    Keys->U_CBUTTON = m_DirectInput->IsButtonPressed(Controller.U_CBUTTON);
    Keys->R_TRIG = m_DirectInput->IsButtonPressed(Controller.R_TRIG);
    Keys->L_TRIG = m_DirectInput->IsButtonPressed(Controller.L_TRIG);
    m_DirectInput->GetAxis(Controller, Keys);
}

void CProject64Input::StartScanDevices(int32_t DisplayCtrlId)
{
    m_Scanning = true;
    m_DisplayCtrlId = DisplayCtrlId;
    m_DirectInput->UpdateDeviceData();
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
    CGuard guard(m_CS);

    if (ControlIndex >= sizeof(m_Controllers) / sizeof(m_Controllers[0]))
    {
        return false;
    }
    g_Settings->SaveController(ControlIndex, m_ControlInfo.Controls[ControlIndex], m_Controllers[ControlIndex]);
    m_DirectInput->MapControllerDevice(m_Controllers[ControlIndex]);
    return true;
}
