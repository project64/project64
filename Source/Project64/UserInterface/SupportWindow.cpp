#include "stdafx.h"

#include "SupportEnterCode.h"
#include <time.h>

CSupportWindow * CSupportWindow::m_this = nullptr;

CSupportWindow::CSupportWindow(CProjectSupport & Support) :
    m_Support(Support),
    m_TimeOutTime(30),
    m_hParent(nullptr),
    m_Delay(false)
{
}

CSupportWindow::~CSupportWindow(void)
{
}

void CALLBACK CSupportWindow::TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
{
    ::KillTimer(nullptr, idEvent);
    m_this->DoModal(m_this->m_hParent);
}

void CSupportWindow::Show(HWND hParent, bool Delay)
{
    m_Delay = Delay;
    if (Delay)
    {
        if (m_Support.Validated())
        {
            return;
        }

        m_Support.IncrementRunCount();
        if (m_Support.RunCount() < 7 || !m_Support.ShowSuppotWindow())
        {
            return;
        }
        m_hParent = hParent;
        m_this = this;
        UISettingsSaveBool(UserInterface_ShowingNagWindow, true);
        ::SetTimer(nullptr, 0, 2500, TimerProc);
    }
    else
    {
        UISettingsSaveBool(UserInterface_ShowingNagWindow, true);
        DoModal(hParent);
    }
}

void CSupportWindow::EnableContinue()
{
    GetSystemMenu(true);
    SetWindowLong(GWL_STYLE, GetWindowLong(GWL_STYLE) & ~CS_NOCLOSE);
    ::EnableWindow(GetDlgItem(IDCANCEL), true);
}

LRESULT CSupportWindow::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    m_Logo.SubclassWindow(GetDlgItem(IDC_BMP_LOGO));
    m_Logo.SetBitmap(MAKEINTRESOURCE(IDB_ABOUT_LOGO));

    std::wstring InfoText = wGS(MSG_SUPPORT_INFO);
    SetWindowText(wGS(MSG_SUPPORT_TITLE).c_str());
    GetDlgItem(IDC_ENTER_CODE).SetWindowText(wGS(MSG_SUPPORT_ENTER_CODE).c_str());
    GetDlgItem(ID_SUPPORT_PJ64).SetWindowText(wGS(MSG_SUPPORT_PROJECT64).c_str());
    GetDlgItem(IDCANCEL).SetWindowText(wGS(MSG_SUPPORT_CONTINUE).c_str());

    m_EnterLink.SubclassWindow(GetDlgItem(IDC_ENTER_CODE));
    m_EnterLink.SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON, HLINK_COMMANDBUTTON);
    m_EnterLink.EnableWindow(!m_Support.Validated());

    CWindow hInfo = GetDlgItem(IDC_INFO);
    CRect rcWin = {0};
    hInfo.GetClientRect(&rcWin);

    CDC hDC = hInfo.GetDC();
    HFONT hFont = hInfo.GetFont();
    if (hFont == nullptr)
    {
        hFont = (HFONT)::GetStockObject(SYSTEM_FONT);
    }
    hDC.SelectFont(hFont);
    if (hDC.DrawText(InfoText.c_str(), (int)((INT_PTR)InfoText.length()), &rcWin, DT_LEFT | DT_CALCRECT | DT_WORDBREAK | DT_NOCLIP) > 0)
    {
        hInfo.SetWindowPos(nullptr, 0, 0, rcWin.right, rcWin.bottom, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER);
    }
    hInfo.SetWindowText(InfoText.c_str());
    hInfo.GetWindowRect(&rcWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&rcWin, 2);

    CWindow EnterCode = GetDlgItem(IDC_ENTER_CODE);
    EnterCode.SetWindowPos(nullptr, rcWin.left, rcWin.bottom + 4, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);
    EnterCode.GetWindowRect(&rcWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&rcWin, 2);

    CWindow SupportBtn = GetDlgItem(ID_SUPPORT_PJ64);
    RECT SupportBtnWin = {0};
    SupportBtn.GetWindowRect(&SupportBtnWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&SupportBtnWin, 2);
    SupportBtn.SetWindowPos(nullptr, SupportBtnWin.left, rcWin.bottom + 40, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);

    CWindow CancelBtn = GetDlgItem(IDCANCEL);
    RECT CancelBtnWin = {0};
    CancelBtn.GetWindowRect(&CancelBtnWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&CancelBtnWin, 2);
    CancelBtn.SetWindowPos(nullptr, CancelBtnWin.left, rcWin.bottom + 40, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);

    GetWindowRect(&rcWin);
    SupportBtn.GetWindowRect(&SupportBtnWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&SupportBtnWin, 2);
    SetRect(&rcWin, 0, 0, rcWin.Width(), SupportBtnWin.bottom + 30);
    AdjustWindowRectEx(&rcWin, GetStyle(), GetMenu() != nullptr, GetExStyle());

    int32_t Left = (GetSystemMetrics(SM_CXSCREEN) - rcWin.Width()) / 2;
    int32_t Top = (GetSystemMetrics(SM_CYSCREEN) - rcWin.Height()) / 2;

    MoveWindow(Left, Top, rcWin.Width(), rcWin.Height(), TRUE);

    if (m_Delay && m_Support.RunCount() >= 15)
    {
        CMenuHandle menu = GetSystemMenu(false);
        menu.RemoveMenu(SC_CLOSE, MF_BYCOMMAND);
        DWORD dwStyle = GetWindowLong(GWL_STYLE);
        dwStyle |= CS_NOCLOSE;
        SetWindowLong(GWL_STYLE, dwStyle);

        GetDlgItem(IDCANCEL).EnableWindow(false);
        srand((uint32_t)time(nullptr));
        SetTimer(0, 1000, nullptr);
    }
    return TRUE;
}

LRESULT CSupportWindow::OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    CDCHandle hdcStatic = (HDC)wParam;
    hdcStatic.SetTextColor(RGB(0, 0, 0));
    hdcStatic.SetBkMode(TRANSPARENT);
    return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
}

LRESULT CSupportWindow::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    static HPEN Outline = CreatePen(PS_SOLID, 1, 0x00FFFFFF);
    static HBRUSH Fill = CreateSolidBrush(0x00FFFFFF);

    CDCHandle hdc = (HDC)wParam;
    hdc.SelectPen(Outline);
    hdc.SelectBrush(Fill);

    RECT rect;
    GetClientRect(&rect);
    hdc.Rectangle(&rect);
    return TRUE;
}

LRESULT CSupportWindow::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    m_TimeOutTime -= 1;
    if (m_TimeOutTime == 0)
    {
        KillTimer(wParam);
        EnableContinue();
    }
    stdstr_f Continue_txt(m_TimeOutTime > 0 ? "%s (%d)" : "%s", GS(MSG_SUPPORT_CONTINUE), m_TimeOutTime);
    GetDlgItem(IDCANCEL).SetWindowText(Continue_txt.ToUTF16().c_str());
    return true;
}

LRESULT CSupportWindow::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    UISettingsSaveBool(UserInterface_ShowingNagWindow, false);
    EndDialog(wID);
    return TRUE;
}

LRESULT CSupportWindow::OnSupportProject64(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    stdstr SupportURL = stdstr_f("https://www.pj64-emu.com/support-project64.html?ver=%s&machine=%s", VER_FILE_VERSION_STR, m_Support.MachineID());
    ShellExecute(nullptr, L"open", SupportURL.ToUTF16().c_str(), nullptr, nullptr, SW_SHOWMAXIMIZED);
    return TRUE;
}

LRESULT CSupportWindow::OnEnterCode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    CSupportEnterCode EnterCodeWindow(m_Support);
    EnterCodeWindow.DoModal(m_hWnd);
    if (m_Support.Validated())
    {
        UISettingsSaveBool(UserInterface_ShowingNagWindow, false);
        EndDialog(wID);
    }
    return TRUE;
}
