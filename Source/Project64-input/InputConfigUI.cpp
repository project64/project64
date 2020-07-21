#include "InputConfigUI.h"
#include "CProject64Input.h"
#include "wtl.h"
#include "wtl-BitmapPicture.h"
#include "wtl-ScanButton.h"
#include <Common\stdtypes.h>
#include <Common\StdString.h>
#include "resource.h"

class CControllerSettings :
    public CPropertyPageImpl<CControllerSettings>
{
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
        COMMAND_HANDLER_EX(IDC_CHK_PLUGGED_IN, BN_CLICKED, ItemChanged)
        NOTIFY_HANDLER_EX(IDC_TACK_RANGE, NM_RELEASEDCAPTURE, ItemChangedNotify);
        MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
        MESSAGE_HANDLER(CScanButton::WM_SCAN_SUCCESS, OnScanSuccess)
        MESSAGE_HANDLER(CScanButton::WM_SCAN_CANCELED, OnScanCanceled)
        CHAIN_MSG_MAP(CPropertyPageImpl<CControllerSettings>)
    END_MSG_MAP()

    CControllerSettings(uint32_t ControllerNumber);
    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
    HBRUSH OnCtlColorStatic(CDCHandle dc, CWindow wndStatic);
    bool OnApply();

private:
    LRESULT OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnScanSuccess(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnScanCanceled(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    void DefaultBtnClicked(UINT Code, int id, HWND ctl);
    void SetupBtnClicked(UINT Code, int id, HWND ctl);
    void ItemChanged(UINT Code, int id, HWND ctl);
    LRESULT	ItemChangedNotify(NMHDR* /*pNMHDR*/);
    void DisplayController(void);
    void ButtonChannged(void);
    static void stButtonChanged(size_t data) { ((CControllerSettings *)data)->ButtonChannged(); }

    std::wstring m_Title;
    uint32_t m_ControllerNumber;
    uint32_t m_ScanCount;
    int32_t m_SetupIndex;
    CBitmapPicture m_ControllerImg;
    CButton m_PluggedIn;
    CTrackBarCtrl m_Range;
    CScanButton m_ButtonUDPad, m_ButtonDDPad, m_ButtonLDPad, m_ButtonRDPad;
    CScanButton m_ButtonCUp, m_ButtonCDown, m_ButtonCLeft, m_ButtonCRight;
    CScanButton m_ButtonA, m_ButtonB, m_ButtonStart;
    CScanButton m_ButtonZtrigger, m_ButtonRTrigger, m_ButtonLTrigger;
    CScanButton m_ButtonAnalogU, m_ButtonAnalogD, m_ButtonAnalogL, m_ButtonAnalogR;
};

CControllerSettings::CControllerSettings(uint32_t ControllerNumber) :
    m_ControllerNumber(ControllerNumber),
    m_ScanCount(0),
    m_SetupIndex(-1),
    m_ButtonUDPad(g_InputPlugin->Controllers(ControllerNumber).U_DPAD, IDC_EDIT_DIGITIAL_UP, IDC_BTN_DIGITIAL_UP),
    m_ButtonDDPad(g_InputPlugin->Controllers(ControllerNumber).D_DPAD, IDC_EDIT_DIGITIAL_DOWN, IDC_BTN_DIGITIAL_DOWN),
    m_ButtonLDPad(g_InputPlugin->Controllers(ControllerNumber).L_DPAD, IDC_EDIT_DIGITIAL_LEFT, IDC_BTN_DIGITIAL_LEFT),
    m_ButtonRDPad(g_InputPlugin->Controllers(ControllerNumber).R_DPAD, IDC_EDIT_DIGITIAL_RIGHT, IDC_BTN_DIGITIAL_RIGHT),
    m_ButtonA(g_InputPlugin->Controllers(ControllerNumber).A_BUTTON, IDC_EDIT_BUTTON_A, IDC_BTN_BUTTON_A),
    m_ButtonB(g_InputPlugin->Controllers(ControllerNumber).B_BUTTON, IDC_EDIT_BUTTON_B, IDC_BTN_BUTTON_B),
    m_ButtonCUp(g_InputPlugin->Controllers(ControllerNumber).U_CBUTTON, IDC_EDIT_CBUTTON_UP, IDC_BTN_CBUTTON_UP),
    m_ButtonCDown(g_InputPlugin->Controllers(ControllerNumber).D_CBUTTON, IDC_EDIT_CBUTTON_DOWN, IDC_BTN_CBUTTON_DOWN),
    m_ButtonCLeft(g_InputPlugin->Controllers(ControllerNumber).L_CBUTTON, IDC_EDIT_CBUTTON_LEFT, IDC_BTN_CBUTTON_LEFT),
    m_ButtonCRight(g_InputPlugin->Controllers(ControllerNumber).R_CBUTTON, IDC_EDIT_CBUTTON_RIGHT, IDC_BTN_CBUTTON_RIGHT),
    m_ButtonStart(g_InputPlugin->Controllers(ControllerNumber).START_BUTTON, IDC_EDIT_BUTTON_START, IDC_BTN_BUTTON_START),
    m_ButtonZtrigger(g_InputPlugin->Controllers(ControllerNumber).Z_TRIG, IDC_EDIT_BUTTON_Z, IDC_BTN_BUTTON_Z),
    m_ButtonRTrigger(g_InputPlugin->Controllers(ControllerNumber).R_TRIG, IDC_EDIT_RTRIGGER, IDC_BTN_RTRIGGER),
    m_ButtonLTrigger(g_InputPlugin->Controllers(ControllerNumber).L_TRIG, IDC_EDIT_LTRIGGER, IDC_BTN_LTRIGGER),
    m_ButtonAnalogU(g_InputPlugin->Controllers(ControllerNumber).U_ANALOG, IDC_EDIT_ANALOG_UP, IDC_BTN_ANALOG_UP),
    m_ButtonAnalogD(g_InputPlugin->Controllers(ControllerNumber).D_ANALOG, IDC_EDIT_ANALOG_DOWN, IDC_BTN_ANALOG_DOWN),
    m_ButtonAnalogL(g_InputPlugin->Controllers(ControllerNumber).L_ANALOG, IDC_EDIT_ANALOG_LEFT, IDC_BTN_ANALOG_LEFT),
    m_ButtonAnalogR(g_InputPlugin->Controllers(ControllerNumber).R_ANALOG, IDC_EDIT_ANALOG_RIGHT, IDC_BTN_ANALOG_RIGHT)
{
    m_Title = stdstr_f("Player %d", ControllerNumber + 1).ToUTF16();
    SetTitle(m_Title.c_str());
}

BOOL CControllerSettings::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
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

bool CControllerSettings::OnApply()
{
    N64CONTROLLER & Controller = g_InputPlugin->Controllers(m_ControllerNumber);
    CONTROL & ControlInfo = g_InputPlugin->ControlInfo(m_ControllerNumber);
    Controller.Range = (uint8_t)m_Range.GetPos();
    ControlInfo.Present = (m_PluggedIn.GetCheck() == BST_CHECKED) ? 1 : 0;

    return g_InputPlugin->SaveController(m_ControllerNumber);
}

LRESULT CControllerSettings::OnScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    LONG SliderId = CWindow((HWND)lParam).GetWindowLong(GWL_ID);
    if (SliderId == IDC_SLIDER_RANGE)
    {
        CWindow(GetDlgItem(IDC_LABEL_RANGE)).SetWindowText(stdstr_f("%d%%", m_Range.GetPos()).ToUTF16().c_str());
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

void CControllerSettings::DefaultBtnClicked(UINT Code, int id, HWND ctl)
{
    g_InputPlugin->ResetController(m_ControllerNumber);
    DisplayController();
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void CControllerSettings::SetupBtnClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    m_SetupIndex = 0;
    m_ButtonUDPad.DetectKey();
}

void CControllerSettings::ItemChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

LRESULT	CControllerSettings::ItemChangedNotify(NMHDR* /*pNMHDR*/)
{
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    return 0;
}

void CControllerSettings::DisplayController(void)
{
    N64CONTROLLER & Controller = g_InputPlugin->Controllers(m_ControllerNumber);
    CONTROL & ControlInfo = g_InputPlugin->ControlInfo(m_ControllerNumber);
    m_PluggedIn.SetCheck(ControlInfo.Present != 0 ? BST_CHECKED : BST_UNCHECKED);
    m_Range.SetPos(Controller.Range);
    CWindow(GetDlgItem(IDC_LABEL_RANGE)).SetWindowText(stdstr_f("%d%%", m_Range.GetPos()).ToUTF16().c_str());
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
}

void CControllerSettings::ButtonChannged(void)
{
    CPropertySheetWindow(GetParent()).SetModified(m_hWnd);
}

class CInputConfigUI: 
    public CPropertySheetImpl<CInputConfigUI>
{
public:
    CInputConfigUI();
    ~CInputConfigUI();

    void OnSheetInitialized();

private:
    CControllerSettings m_pgController0, m_pgController1, m_pgController2, m_pgController3;
};

void ConfigInput(void * hParent)
{
    CInputConfigUI().DoModal((HWND)hParent);
}

CInputConfigUI::CInputConfigUI() :
    m_pgController0(0),
    m_pgController1(1),
    m_pgController2(2),
    m_pgController3(3)
{
    m_psh.pszCaption = L"Configure Input";
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
