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

    BEGIN_MSG_MAP(CDebugSettings)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic)
        CHAIN_MSG_MAP(CPropertyPageImpl<CControllerSettings>)
    END_MSG_MAP()

    CControllerSettings(uint32_t ControllerNumber);
    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
    HBRUSH OnCtlColorStatic(CDCHandle dc, CWindow wndStatic);
    bool OnApply();

private:
    void ButtonChannged(void);
    static void stButtonChanged(size_t data) { ((CControllerSettings *)data)->ButtonChannged(); }

    std::wstring m_Title;
    uint32_t m_ControllerNumber;
    uint32_t m_ScanCount;
    CBitmapPicture m_ControllerImg;
    CScanButton ButtonUDPad, ButtonDDPad, ButtonLDPad, ButtonRDPad;
    CScanButton ButtonCUp, ButtonCDown, ButtonCLeft, ButtonCRight;
    CScanButton ButtonA, ButtonB, ButtonStart;
    CScanButton ButtonZtrigger, ButtonRTrigger, ButtonLTrigger;
    CScanButton ButtonAnalogU, ButtonAnalogD, ButtonAnalogL, ButtonAnalogR;
};

CControllerSettings::CControllerSettings(uint32_t ControllerNumber) :
    m_ControllerNumber(ControllerNumber),
    m_ScanCount(0),
    ButtonUDPad(g_InputPlugin->Controllers(ControllerNumber).U_DPAD, IDC_EDIT_DIGITIAL_UP, IDC_BTN_DIGITIAL_UP),
    ButtonDDPad(g_InputPlugin->Controllers(ControllerNumber).D_DPAD, IDC_EDIT_DIGITIAL_DOWN, IDC_BTN_DIGITIAL_DOWN),
    ButtonLDPad(g_InputPlugin->Controllers(ControllerNumber).L_DPAD, IDC_EDIT_DIGITIAL_LEFT, IDC_BTN_DIGITIAL_LEFT),
    ButtonRDPad(g_InputPlugin->Controllers(ControllerNumber).R_DPAD, IDC_EDIT_DIGITIAL_RIGHT, IDC_BTN_DIGITIAL_RIGHT),
    ButtonA(g_InputPlugin->Controllers(ControllerNumber).A_BUTTON, IDC_EDIT_BUTTON_A, IDC_BTN_BUTTON_A),
    ButtonB(g_InputPlugin->Controllers(ControllerNumber).B_BUTTON, IDC_EDIT_BUTTON_B, IDC_BTN_BUTTON_B),
    ButtonCUp(g_InputPlugin->Controllers(ControllerNumber).U_CBUTTON, IDC_EDIT_CBUTTON_UP, IDC_BTN_CBUTTON_UP),
    ButtonCDown(g_InputPlugin->Controllers(ControllerNumber).D_CBUTTON, IDC_EDIT_CBUTTON_DOWN, IDC_BTN_CBUTTON_DOWN),
    ButtonCLeft(g_InputPlugin->Controllers(ControllerNumber).L_CBUTTON, IDC_EDIT_CBUTTON_LEFT, IDC_BTN_CBUTTON_LEFT),
    ButtonCRight(g_InputPlugin->Controllers(ControllerNumber).R_CBUTTON, IDC_EDIT_CBUTTON_RIGHT, IDC_BTN_CBUTTON_RIGHT),
    ButtonStart(g_InputPlugin->Controllers(ControllerNumber).START_BUTTON, IDC_EDIT_BUTTON_START, IDC_BTN_BUTTON_START),
    ButtonZtrigger(g_InputPlugin->Controllers(ControllerNumber).Z_TRIG, IDC_EDIT_BUTTON_Z, IDC_BTN_BUTTON_Z),
    ButtonRTrigger(g_InputPlugin->Controllers(ControllerNumber).R_TRIG, IDC_EDIT_RTRIGGER, IDC_BTN_RTRIGGER),
    ButtonLTrigger(g_InputPlugin->Controllers(ControllerNumber).L_TRIG, IDC_EDIT_LTRIGGER, IDC_BTN_LTRIGGER),
    ButtonAnalogU(g_InputPlugin->Controllers(ControllerNumber).U_ANALOG, IDC_EDIT_ANALOG_UP, IDC_BTN_ANALOG_UP),
    ButtonAnalogD(g_InputPlugin->Controllers(ControllerNumber).D_ANALOG, IDC_EDIT_ANALOG_DOWN, IDC_BTN_ANALOG_DOWN),
    ButtonAnalogL(g_InputPlugin->Controllers(ControllerNumber).L_ANALOG, IDC_EDIT_ANALOG_LEFT, IDC_BTN_ANALOG_LEFT),
    ButtonAnalogR(g_InputPlugin->Controllers(ControllerNumber).R_ANALOG, IDC_EDIT_ANALOG_RIGHT, IDC_BTN_ANALOG_RIGHT)
{
    m_Title = stdstr_f("Player %d", ControllerNumber + 1).ToUTF16();
    SetTitle(m_Title.c_str());
}

BOOL CControllerSettings::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    GetDlgItem(IDC_BTN_SETUP).EnableWindow(false);
    GetDlgItem(IDC_BTN_DEFAULTS).EnableWindow(false);
    GetDlgItem(IDC_BTN_LOAD).EnableWindow(false);
    GetDlgItem(IDC_BTN_SAVE).EnableWindow(false);
    GetDlgItem(IDC_TACK_RANGE).EnableWindow(false);

    m_ControllerImg.SubclassWindow(GetDlgItem(IDC_BMP_CONTROLLER));
    m_ControllerImg.SetBitmap(MAKEINTRESOURCE(IDB_CONTROLLER));
    CScanButton * Buttons[] = {
        &ButtonUDPad, &ButtonDDPad, &ButtonLDPad, &ButtonRDPad, &ButtonA, &ButtonB,
        &ButtonCUp, &ButtonCDown, &ButtonCLeft, &ButtonCRight, &ButtonStart,
        &ButtonZtrigger, &ButtonRTrigger, &ButtonLTrigger,
        &ButtonAnalogU, &ButtonAnalogD, &ButtonAnalogL, &ButtonAnalogR
    };

    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        Buttons[i]->SubclassWindow(m_hWnd);
        Buttons[i]->SetChangeCallback(stButtonChanged, (size_t)this);
    }
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
    return g_InputPlugin->SaveController(m_ControllerNumber);
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
