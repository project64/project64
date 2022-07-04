#include "CProject64Input.h"
#include "InputSettings.h"
#include "InputConfigUI.h"

CProject64Input * g_InputPlugin = nullptr;

CProject64Input::CProject64Input(HINSTANCE hinst) :
    m_hinst(hinst),
    m_Scanning(false),
    m_DisplayCtrlId(0),
    m_iFirstController(-1),
    m_MouseLock(false)
{
    memset(m_Controllers, 0, sizeof(m_Controllers));
}

CProject64Input::~CProject64Input() = default;

void CProject64Input::DevicesChanged(void)
{
    CGuard guard(m_CS);
    if (m_DirectInput.get() != nullptr)
    {
        m_DirectInput->DevicesChanged();
    }
}

void CProject64Input::DeviceAdded(void)
{
    ConfigUIDeviceAdded();
}

void CProject64Input::InitiateControllers(CONTROL_INFO * ControlInfo)
{
    CGuard guard(m_CS);
    m_ControlInfo = *ControlInfo;
    if (m_DirectInput.get() == nullptr)
    {
        m_DirectInput.reset(new CDirectInput(m_hinst));
    }
    m_DirectInput->Initiate(ControlInfo);
    m_iFirstController = -1;
    for (uint32_t i = 0, n = sizeof(m_Controllers) / sizeof(m_Controllers[0]); i < n; i++)
    {
        g_Settings->LoadController(i, m_ControlInfo.Controls[i], m_Controllers[i]);
        m_DirectInput->MapControllerDevice(m_Controllers[i]);
        if (m_ControlInfo.Controls[i].Present != PRESENT_NONE && m_iFirstController < 0)
        {
            m_iFirstController = i;
        }
    }

    g_Settings->GetControllerMouse(m_N64Mouse);
    m_DirectInput->MapControllerDevice(m_N64Mouse);

    g_Settings->LoadShortcuts(m_Shortcuts);
    m_DirectInput->MapShortcutDevice(m_Shortcuts);
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
        CheckShortcuts();
    }
    if (m_ControlInfo.Controls[Control].Present == PRESENT_MOUSE)
    {
        //Mouse
        if (m_MouseLock)
        {
            LockCursor();
            m_N64Mouse.Sensitivity = m_Controllers[Control].Sensitivity;
            Keys->R_DPAD = m_DirectInput->IsButtonPressed(m_N64Mouse.R_DPAD);
            Keys->L_DPAD = m_DirectInput->IsButtonPressed(m_N64Mouse.L_DPAD);
            Keys->D_DPAD = m_DirectInput->IsButtonPressed(m_N64Mouse.D_DPAD);
            Keys->U_DPAD = m_DirectInput->IsButtonPressed(m_N64Mouse.U_DPAD);
            Keys->START_BUTTON = m_DirectInput->IsButtonPressed(m_N64Mouse.START_BUTTON);
            Keys->Z_TRIG = m_DirectInput->IsButtonPressed(m_N64Mouse.Z_TRIG);
            Keys->B_BUTTON = m_DirectInput->IsButtonPressed(m_N64Mouse.B_BUTTON);
            Keys->A_BUTTON = m_DirectInput->IsButtonPressed(m_N64Mouse.A_BUTTON);
            Keys->R_CBUTTON = m_DirectInput->IsButtonPressed(m_N64Mouse.R_CBUTTON);
            Keys->L_CBUTTON = m_DirectInput->IsButtonPressed(m_N64Mouse.L_CBUTTON);
            Keys->D_CBUTTON = m_DirectInput->IsButtonPressed(m_N64Mouse.D_CBUTTON);
            Keys->U_CBUTTON = m_DirectInput->IsButtonPressed(m_N64Mouse.U_CBUTTON);
            Keys->R_TRIG = m_DirectInput->IsButtonPressed(m_N64Mouse.R_TRIG);
            Keys->L_TRIG = m_DirectInput->IsButtonPressed(m_N64Mouse.L_TRIG);
            m_DirectInput->GetAxis(m_N64Mouse, Keys);
        }
    }
    else
    {
        //Controller
        N64CONTROLLER& Controller = m_Controllers[Control];
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
    if (m_DirectInput.get() != nullptr)
    {
        Result = m_DirectInput->ScanDevices(Button);
    }
    return Result;
}

std::wstring CProject64Input::ButtonAssignment(BUTTON & Button)
{
    if (m_DirectInput.get() != nullptr)
    {
        return m_DirectInput->ButtonAssignment(Button);
    }
    return L"Unknown";
}

std::wstring CProject64Input::ControllerDevices(const N64CONTROLLER & Controller)
{
    if (m_DirectInput.get() != nullptr)
    {
        return m_DirectInput->ControllerDevices(Controller);
    }
    return L"";
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

bool CProject64Input::ResetController(uint32_t ControlIndex, CONTROL & ControlInfo, N64CONTROLLER & Controller)
{
    g_Settings->ResetController(ControlIndex, ControlInfo, Controller);
    m_DirectInput->MapControllerDevice(Controller);
    return true;
}

void CProject64Input::CheckShortcuts()
{
    bool isPressed = m_DirectInput->IsButtonPressed(m_Shortcuts.LOCKMOUSE);
    if ((isPressed == true) && (m_Shortcuts.LOCKMOUSE_PRESSED == false))
    {
        LockMouseSwitch();
    }
    m_Shortcuts.LOCKMOUSE_PRESSED = isPressed;
}

bool CProject64Input::SaveShortcuts()
{
    CGuard guard(m_CS);

    g_Settings->SaveShortcuts(m_Shortcuts);
    m_DirectInput->MapShortcutDevice(m_Shortcuts);
    return true;
}

bool CProject64Input::ResetShortcuts(SHORTCUTS& Shortcuts)
{
    g_Settings->ResetShortcuts(Shortcuts);
    m_DirectInput->MapShortcutDevice(Shortcuts);
    return true;
}

void CProject64Input::LockMouse()
{
    if (IsMouseUsed() == false) return UnlockMouse();
    if (m_MouseLock == true) return;
    PostMessage((HWND)m_ControlInfo.hWnd, WM_HIDE_CUROSR, false, 0);
    m_MouseLock = true;
}

void CProject64Input::UnlockMouse()
{
    if (m_MouseLock == false) return;
    PostMessage((HWND)m_ControlInfo.hWnd, WM_HIDE_CUROSR, true, 0);
    m_MouseLock = false;
}

void CProject64Input::LockMouseSwitch()
{
    if (m_MouseLock == true)
    {
        UnlockMouse();
    }
    else
    {
        LockMouse();
    }
}

bool CProject64Input::IsMouseUsed()
{
    for (uint32_t i = 0, n = sizeof(m_Controllers) / sizeof(m_Controllers[0]); i < n; i++)
    {
        if (m_ControlInfo.Controls[i].Present == PRESENT_MOUSE)
        {
            return true;
        }
    }
    return false;
}

void CProject64Input::LockCursor()
{
    RECT rect;
    GetWindowRect((HWND)m_ControlInfo.hWnd, &rect);
    SetCursorPos((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
}
