#include "InputConfigUI.h"
#include "CProject64Input.h"
#include "wtl.h"
#include "wtl-BitmapPicture.h"
#include "wtl-ScanButton.h"
#include "OptionsUI.h"
#include <stdint.h>
#include <Common/StdString.h>
#include "resource.h"

class CInputConfigUI;

CInputConfigUI * g_ConfigUI = nullptr;

class CControllerSettings :
    public CPropertyPageImpl<CControllerSettings>
{
    friend CInputConfigUI;

    enum
    {
        DETECT_KEY_TIMER = 1
    };
public:
    enum { IDD = IDD_Controller };

    BEGIN_MSG_MAP(CControllerSettings)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic)
        COMMAND_HANDLER_EX(IDC_BTN_DEFAULTS, BN_CLICKED, DefaultBtnClicked)
        COMMAND_HANDLER_EX(IDC_BTN_SETUP, BN_CLICKED, SetupBtnClicked)
        COMMAND_HANDLER_EX(IDC_BTN_OPTIONS, BN_CLICKED, OptionsBtnClicked)
        COMMAND_HANDLER_EX(IDC_CHK_PLUGGED_IN, BN_CLICKED, PluggedInChanged)
        NOTIFY_HANDLER_EX(IDC_TACK_RANGE, NM_RELEASEDCAPTURE, ItemChangedNotify);
        MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
        MESSAGE_HANDLER(CScanButton::WM_SCAN_SUCCESS, OnScanSuccess)
        MESSAGE_HANDLER(CScanButton::WM_SCAN_CANCELED, OnScanCanceled)
        CHAIN_MSG_MAP(CPropertyPageImpl<CControllerSettings>)
    END_MSG_MAP()

    CControllerSettings(uint32_t ControllerNumber);
    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
    HBRUSH OnCtlColorStatic(CDCHandle dc, CWindow wndStatic);
    void RemoveMapping(const BUTTON & Button);
    LRESULT OnApply();

private:
    LRESULT OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnScanSuccess(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnScanCanceled(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    void DefaultBtnClicked(UINT Code, int id, HWND ctl);
    void SetupBtnClicked(UINT Code, int id, HWND ctl);
    void OptionsBtnClicked(UINT Code, int id, HWND ctl);
    void PluggedInChanged(UINT Code, int id, HWND ctl);
    LRESULT	ItemChangedNotify(NMHDR* /*pNMHDR*/);
    void DisplayController(void);
    void ButtonChannged(const BUTTON & Button);
    void EnablePage(bool Enable);
    static void stButtonChanged(size_t data, const BUTTON & Button) { ((CControllerSettings *)data)->ButtonChannged(Button); }

    std::wstring m_Title;
    uint32_t m_ControllerNumber;
    uint32_t m_ScanCount;
    int32_t m_SetupIndex;
    N64CONTROLLER m_Controller;
    CONTROL m_ControlInfo;
    CBitmapPicture m_ControllerImg;
    CButton m_PluggedIn;
    CTrackBarCtrl m_Range;
    CTrackBarCtrl m_DeadZone;
    CScanButton m_ButtonUDPad, m_ButtonDDPad, m_ButtonLDPad, m_ButtonRDPad;
    CScanButton m_ButtonCUp, m_ButtonCDown, m_ButtonCLeft, m_ButtonCRight;
    CScanButton m_ButtonA, m_ButtonB, m_ButtonStart;
    CScanButton m_ButtonZtrigger, m_ButtonRTrigger, m_ButtonLTrigger;
    CScanButton m_ButtonAnalogU, m_ButtonAnalogD, m_ButtonAnalogL, m_ButtonAnalogR;
};

class CInputConfigUI :
    public CPropertySheetImpl<CInputConfigUI>
{
public:
    CInputConfigUI();
    ~CInputConfigUI();

    void UpdateDeviceMapping(void);
    void RemoveMapping(const BUTTON & Button);
    void OnSheetInitialized();

private:
    CControllerSettings m_pgController0, m_pgController1, m_pgController2, m_pgController3;
};

CControllerSettings::CControllerSettings(uint32_t ControllerNumber) :
    m_ControllerNumber(ControllerNumber),
    m_ScanCount(0),
    m_SetupIndex(-1),
    m_Controller(g_InputPlugin->Controllers(ControllerNumber)),
    m_ControlInfo(g_InputPlugin->ControlInfo(ControllerNumber)),
    m_ButtonUDPad(m_Controller.U_DPAD, IDC_EDIT_DIGITIAL_UP, IDC_BTN_DIGITIAL_UP),
    m_ButtonDDPad(m_Controller.D_DPAD, IDC_EDIT_DIGITIAL_DOWN, IDC_BTN_DIGITIAL_DOWN),
    m_ButtonLDPad(m_Controller.L_DPAD, IDC_EDIT_DIGITIAL_LEFT, IDC_BTN_DIGITIAL_LEFT),
    m_ButtonRDPad(m_Controller.R_DPAD, IDC_EDIT_DIGITIAL_RIGHT, IDC_BTN_DIGITIAL_RIGHT),
    m_ButtonA(m_Controller.A_BUTTON, IDC_EDIT_BUTTON_A, IDC_BTN_BUTTON_A),
    m_ButtonB(m_Controller.B_BUTTON, IDC_EDIT_BUTTON_B, IDC_BTN_BUTTON_B),
    m_ButtonCUp(m_Controller.U_CBUTTON, IDC_EDIT_CBUTTON_UP, IDC_BTN_CBUTTON_UP),
    m_ButtonCDown(m_Controller.D_CBUTTON, IDC_EDIT_CBUTTON_DOWN, IDC_BTN_CBUTTON_DOWN),
    m_ButtonCLeft(m_Controller.L_CBUTTON, IDC_EDIT_CBUTTON_LEFT, IDC_BTN_CBUTTON_LEFT),
    m_ButtonCRight(m_Controller.R_CBUTTON, IDC_EDIT_CBUTTON_RIGHT, IDC_BTN_CBUTTON_RIGHT),
    m_ButtonStart(m_Controller.START_BUTTON, IDC_EDIT_BUTTON_START, IDC_BTN_BUTTON_START),
    m_ButtonZtrigger(m_Controller.Z_TRIG, IDC_EDIT_BUTTON_Z, IDC_BTN_BUTTON_Z),
    m_ButtonRTrigger(m_Controller.R_TRIG, IDC_EDIT_RTRIGGER, IDC_BTN_RTRIGGER),
    m_ButtonLTrigger(m_Controller.L_TRIG, IDC_EDIT_LTRIGGER, IDC_BTN_LTRIGGER),
    m_ButtonAnalogU(m_Controller.U_ANALOG, IDC_EDIT_ANALOG_UP, IDC_BTN_ANALOG_UP),
    m_ButtonAnalogD(m_Controller.D_ANALOG, IDC_EDIT_ANALOG_DOWN, IDC_BTN_ANALOG_DOWN),
    m_ButtonAnalogL(m_Controller.L_ANALOG, IDC_EDIT_ANALOG_LEFT, IDC_BTN_ANALOG_LEFT),
    m_ButtonAnalogR(m_Controller.R_ANALOG, IDC_EDIT_ANALOG_RIGHT, IDC_BTN_ANALOG_RIGHT)
{
    m_Title = stdstr_f("Player %d", ControllerNumber + 1).ToUTF16();
    SetTitle(m_Title.c_str());
}

BOOL CControllerSettings::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    m_DeadZone.Attach(GetDlgItem(IDC_SLIDE_DEADZONE));
    m_DeadZone.SetTicFreq(1);
    m_DeadZone.SetRangeMin(1);
    m_DeadZone.SetRangeMax(100);
    m_Range.Attach(GetDlgItem(IDC_SLIDER_RANGE));
    m_Range.SetTicFreq(1);
    m_Range.SetRangeMin(1);
    m_Range.SetRangeMax(100);
    m_PluggedIn.Attach(GetDlgItem(IDC_CHK_PLUGGED_IN));

    m_ControllerImg.SubclassWindow(GetDlgItem(IDC_BMP_CONTROLLER));
    m_ControllerImg.SetBitmap(MAKEINTRESOURCE(IDB_CONTROLLER));
    CScanButton * Buttons[] = {
        &m_ButtonUDPad, &m_ButtonDDPad, &m_ButtonLDPad, &m_ButtonRDPad, &m_ButtonA, &m_ButtonB,
        &m_ButtonCUp, &m_ButtonCDown, &m_ButtonCLeft, &m_ButtonCRight, &m_ButtonStart,
        &m_ButtonZtrigger, &m_ButtonRTrigger, &m_ButtonLTrigger,
        &m_ButtonAnalogU, &m_ButtonAnalogD, &m_ButtonAnalogL, &m_ButtonAnalogR
    };

    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        Buttons[i]->SubclassWindow(m_hWnd);
        Buttons[i]->SetChangeCallback(stButtonChanged, (size_t)this);
    }
    DisplayController();
    EnablePage(m_PluggedIn.GetCheck() == BST_CHECKED);
    return TRUE;
}

HBRUSH CControllerSettings::OnCtlColorStatic(CDCHandle dc, CWindow wndStatic)
{
    if (g_InputPlugin->IsScanning() && wndStatic.GetDlgCtrlID() == g_InputPlugin->DisplayCtrlId())
    {   
        dc.SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
        dc.SetBkColor(GetSysColor(COLOR_HIGHLIGHT));
        return ::GetSysColorBrush(COLOR_HIGHLIGHT);
    }
    dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
    dc.SetBkColor(GetSysColor(COLOR_WINDOW));
    return ::GetSysColorBrush(COLOR_WINDOW);
}

LRESULT CControllerSettings::OnApply()
{
    N64CONTROLLER & Controller = g_InputPlugin->Controllers(m_ControllerNumber);
    Controller = m_Controller;
    Controller.Range = (uint8_t)m_Range.GetPos();
    Controller.DeadZone = (uint8_t)m_DeadZone.GetPos();
    CONTROL & ControlInfo = g_InputPlugin->ControlInfo(m_ControllerNumber);
    ControlInfo.Present = (m_PluggedIn.GetCheck() == BST_CHECKED) ? 1 : 0;
    return g_InputPlugin->SaveController(m_ControllerNumber) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
}

LRESULT CControllerSettings::OnScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    LONG SliderId = CWindow((HWND)lParam).GetWindowLong(GWL_ID);
    if (SliderId == IDC_SLIDER_RANGE)
    {
        CWindow(GetDlgItem(IDC_LABEL_RANGE)).SetWindowText(stdstr_f("%d%%", m_Range.GetPos()).ToUTF16().c_str());
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }
    if (SliderId == IDC_SLIDE_DEADZONE)
    {
        CWindow(GetDlgItem(IDC_GROUP_DEADZONE)).SetWindowText(stdstr_f("Deadzone: %d%%", m_DeadZone.GetPos()).ToUTF16().c_str());
        SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }
    return 0;
}

LRESULT CControllerSettings::OnScanSuccess(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (m_SetupIndex < 0)
    {
        return 0;
    }

    CScanButton * Buttons[] = {
        &m_ButtonUDPad, &m_ButtonDDPad, &m_ButtonLDPad, &m_ButtonRDPad, 
        &m_ButtonAnalogU, &m_ButtonAnalogD, &m_ButtonAnalogL, &m_ButtonAnalogR,
        &m_ButtonCUp, &m_ButtonCDown, &m_ButtonCLeft, &m_ButtonCRight, 
        &m_ButtonB, &m_ButtonA, &m_ButtonStart, &m_ButtonZtrigger,
        &m_ButtonLTrigger, &m_ButtonRTrigger
    };

    m_SetupIndex += 1;
    if (m_SetupIndex < (sizeof(Buttons) / sizeof(Buttons[0])))
    {
        Buttons[m_SetupIndex]->DetectKey();
    }
    return 0;
}

LRESULT CControllerSettings::OnScanCanceled(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_SetupIndex = -1;
    return 0;
}

void CControllerSettings::DefaultBtnClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    g_InputPlugin->ResetController(m_ControllerNumber, m_ControlInfo, m_Controller);
    DisplayController();
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void CControllerSettings::SetupBtnClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    m_SetupIndex = 0;
    m_ButtonUDPad.DetectKey();
}

void CControllerSettings::OptionsBtnClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    ConfigOption(m_ControllerNumber, m_ControlInfo, m_Controller);
}

void CControllerSettings::PluggedInChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    EnablePage(m_PluggedIn.GetCheck() == BST_CHECKED);
}

LRESULT	CControllerSettings::ItemChangedNotify(NMHDR* /*pNMHDR*/)
{
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    return 0;
}

void CControllerSettings::DisplayController(void)
{
    m_PluggedIn.SetCheck(m_ControlInfo.Present != 0 ? BST_CHECKED : BST_UNCHECKED);
    m_Range.SetPos(m_Controller.Range);
    m_DeadZone.SetPos(m_Controller.DeadZone);
    CWindow(GetDlgItem(IDC_LABEL_RANGE)).SetWindowText(stdstr_f("%d%%", m_Range.GetPos()).ToUTF16().c_str());
    CWindow(GetDlgItem(IDC_GROUP_DEADZONE)).SetWindowText(stdstr_f("Deadzone: %d%%", m_DeadZone.GetPos()).ToUTF16().c_str());
    CScanButton * Buttons[] = {
        &m_ButtonUDPad, &m_ButtonDDPad, &m_ButtonLDPad, &m_ButtonRDPad, &m_ButtonA, &m_ButtonB,
        &m_ButtonCUp, &m_ButtonCDown, &m_ButtonCLeft, &m_ButtonCRight, &m_ButtonStart,
        &m_ButtonZtrigger, &m_ButtonRTrigger, &m_ButtonLTrigger,
        &m_ButtonAnalogU, &m_ButtonAnalogD, &m_ButtonAnalogL, &m_ButtonAnalogR
    };

    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        Buttons[i]->DisplayButton();
    }
    GetDlgItem(IDC_BOUND_DEVICE).SetWindowText(g_InputPlugin->ControllerDevices(m_Controller).c_str());
}

void CControllerSettings::ButtonChannged(const BUTTON & Button)
{
    if (g_ConfigUI != nullptr)
    {
        g_ConfigUI->RemoveMapping(Button);
    }
    GetDlgItem(IDC_BOUND_DEVICE).SetWindowText(g_InputPlugin->ControllerDevices(m_Controller).c_str());
    CPropertySheetWindow(GetParent()).SetModified(m_hWnd);
}

void CControllerSettings::EnablePage(bool Enable)
{
    GetDlgItem(IDC_SLIDE_DEADZONE).EnableWindow(Enable);
    GetDlgItem(IDC_SLIDER_RANGE).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_LTRIGGER).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_RTRIGGER).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_DIGITIAL_UP).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_DIGITIAL_DOWN).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_DIGITIAL_LEFT).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_DIGITIAL_RIGHT).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_ANALOG_UP).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_ANALOG_DOWN).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_ANALOG_LEFT).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_ANALOG_RIGHT).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_CBUTTON_UP).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_CBUTTON_DOWN).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_CBUTTON_LEFT).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_CBUTTON_RIGHT).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_BUTTON_B).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_BUTTON_A).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_BUTTON_START).EnableWindow(Enable);
    GetDlgItem(IDC_EDIT_BUTTON_Z).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_LTRIGGER).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_RTRIGGER).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_DIGITIAL_UP).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_DIGITIAL_DOWN).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_DIGITIAL_LEFT).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_DIGITIAL_RIGHT).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_ANALOG_UP).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_ANALOG_DOWN).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_ANALOG_LEFT).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_ANALOG_RIGHT).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_CBUTTON_UP).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_CBUTTON_DOWN).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_CBUTTON_LEFT).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_CBUTTON_RIGHT).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_BUTTON_B).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_BUTTON_A).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_BUTTON_START).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_BUTTON_Z).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_SETUP).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_DEFAULTS).EnableWindow(Enable);
    GetDlgItem(IDC_BTN_OPTIONS).EnableWindow(Enable);
}

void CControllerSettings::RemoveMapping(const BUTTON & Button)
{
    BUTTON EmptyButton = { 0 };
    if (!m_Controller.RemoveDuplicate || memcmp(&Button, &EmptyButton, sizeof(Button)) == 0)
    {
        return;
    }
    BUTTON * buttons[] = 
    {
        &m_Controller.U_DPAD, &m_Controller.D_DPAD, &m_Controller.L_DPAD, &m_Controller.R_DPAD,
        &m_Controller.A_BUTTON, &m_Controller.B_BUTTON, &m_Controller.START_BUTTON, &m_Controller.Z_TRIG,
        &m_Controller.U_CBUTTON, &m_Controller.D_CBUTTON, &m_Controller.L_CBUTTON, &m_Controller.R_CBUTTON,
        &m_Controller.U_ANALOG, &m_Controller.D_ANALOG, &m_Controller.L_ANALOG, &m_Controller.R_ANALOG,
        &m_Controller.R_TRIG, &m_Controller.L_TRIG,
    };

    bool Changed = false;
    for (size_t b = 0; b < (sizeof(buttons) / sizeof(buttons[0])); b++)
    {
        if (buttons[b]->Offset == Button.Offset &&
            buttons[b]->AxisID == Button.AxisID &&
            buttons[b]->BtnType == Button.BtnType &&
            memcmp(&buttons[b]->DeviceGuid, &Button.DeviceGuid, sizeof(Button.DeviceGuid)) == 0)
        {
            *buttons[b] = EmptyButton;
            Changed = true;
        }
    }

    if (Changed)
    {
        CScanButton * ScanButtons[] = {
            &m_ButtonUDPad, &m_ButtonDDPad, &m_ButtonLDPad, &m_ButtonRDPad, &m_ButtonA, &m_ButtonB,
            &m_ButtonCUp, &m_ButtonCDown, &m_ButtonCLeft, &m_ButtonCRight, &m_ButtonStart,
            &m_ButtonZtrigger, &m_ButtonRTrigger, &m_ButtonLTrigger,
            &m_ButtonAnalogU, &m_ButtonAnalogD, &m_ButtonAnalogL, &m_ButtonAnalogR
        };

        for (size_t i = 0, n = sizeof(ScanButtons) / sizeof(ScanButtons[0]); i < n; i++)
        {
            ScanButtons[i]->DisplayButton();
        }
    }
}

void ConfigInput(void * hParent)
{
    CInputConfigUI ConfigUI;
    g_ConfigUI = &ConfigUI;
    ConfigUI.DoModal((HWND)hParent);
    g_ConfigUI = nullptr;
}

CInputConfigUI::CInputConfigUI() :
    m_pgController0(0),
    m_pgController1(1),
    m_pgController2(2),
    m_pgController3(3)
{
    m_psh.pszCaption = L"Configure input";
    AddPage(&m_pgController0.m_psp);
    AddPage(&m_pgController1.m_psp);
    AddPage(&m_pgController2.m_psp);
    AddPage(&m_pgController3.m_psp);
}

CInputConfigUI::~CInputConfigUI()
{
}

void CInputConfigUI::OnSheetInitialized()
{
    ModifyStyleEx(WS_EX_CONTEXTHELP,0);
}

void CInputConfigUI::UpdateDeviceMapping(void)
{
    CControllerSettings * Pages[] = {
        &m_pgController0,
        &m_pgController1,
        &m_pgController2,
        &m_pgController3
    };

    for (uint32_t i = 0, n = GetPageCount(); i < n; i++)
    {
        HWND hPage = IndexToHwnd(i);
        if (hPage != nullptr && i < (sizeof(Pages) / sizeof(Pages[0])))
        {
            N64CONTROLLER & Controller = Pages[i]->m_Controller;
            CWindow(::GetDlgItem(hPage, IDC_BOUND_DEVICE)).SetWindowText(g_InputPlugin->ControllerDevices(Controller).c_str());
        }
    }
}

void CInputConfigUI::RemoveMapping(const BUTTON & Button)
{
    CControllerSettings * Pages[] = {
        &m_pgController0,
        &m_pgController1,
        &m_pgController2,
        &m_pgController3
    };

    for (size_t i = 0, n = (sizeof(Pages) / sizeof(Pages[0])); i < n; i++)
    {
        Pages[i]->RemoveMapping(Button);
    }
}

void ConfigUIDeviceAdded(void)
{
    if (g_ConfigUI != nullptr)
    {
        g_ConfigUI->UpdateDeviceMapping();
    }
}
