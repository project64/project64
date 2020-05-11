#include "stdafx.h"
#include "SupportEnterCode.h"
#include <time.h>

HWND CSupportWindow::m_hParent = NULL;
CSupportWindow * CSupportWindow::m_this = NULL;
uint32_t CSupportWindow::m_RunCount = 0;
uint32_t CSupportWindow::m_TimeOutTime = 30;

CSupportWindow::CSupportWindow(void)
{
}

CSupportWindow::~CSupportWindow(void)
{
}

void CALLBACK CSupportWindow::TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
{
    ::KillTimer(NULL, idEvent);
    CSupportWindow SupportWindow;
    SupportWindow.DoModal(m_hParent);
}

void CSupportWindow::Show(HWND hParent)
{
    m_RunCount = UISettingsLoadDword(SupportWindows_RunCount);
    if (m_RunCount == -1)
    {
        return;
    }
    UISettingsSaveDword(SupportWindows_RunCount, m_RunCount + 1);

    if (m_RunCount < 3)
    {
        return;
    }

    m_hParent = hParent;
    m_this = this;
    ::SetTimer(NULL, 0, 2500, TimerProc);
}

void CSupportWindow::EnableContinue()
{
    GetSystemMenu(true);
    SetWindowLong(GWL_STYLE, GetWindowLong(GWL_STYLE) & ~CS_NOCLOSE);
    ::EnableWindow(GetDlgItem(IDCANCEL), true);
}

LRESULT CSupportWindow::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SetWindowTextW(m_hWnd, wGS(MSG_SUPPORT_TITLE).c_str());
    SetWindowTextW(GetDlgItem(IDC_ENTER_CODE), wGS(MSG_SUPPORT_ENTER_CODE).c_str());
    SetWindowTextW(GetDlgItem(ID_SUPPORT_PJ64), wGS(MSG_SUPPORT_PROJECT64).c_str());
    SetWindowTextW(GetDlgItem(IDCANCEL), wGS(MSG_SUPPORT_CONTINUE).c_str());

    {
        HWND hInfo = GetDlgItem(IDC_INFO);
        std::wstring InfoText = wGS(MSG_SUPPORT_INFO);
        RECT rcWin = { 0 };
        ::GetClientRect(hInfo,&rcWin);

        HDC hDC = ::GetDC(hInfo);
        HFONT hFont = (HFONT)::SendMessage(hInfo, WM_GETFONT, 0, 0L);
        if(hFont == NULL)
        {
            hFont = (HFONT)::GetStockObject(SYSTEM_FONT);
        }
        SelectObject(hDC, hFont);

        if (DrawTextW(hDC,InfoText.c_str(),InfoText.length(),&rcWin,DT_LEFT | DT_CALCRECT | DT_WORDBREAK | DT_NOCLIP) > 0)
        {
            ::SetWindowPos(hInfo,NULL,0,0,rcWin.right, rcWin.bottom,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER);
        }
        SetWindowTextW(hInfo, InfoText.c_str());

        ::GetWindowRect(hInfo,&rcWin);
        ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rcWin, 2);
        ::SetWindowPos(GetDlgItem(IDC_ENTER_CODE),NULL,rcWin.left,rcWin.bottom + 4,0,0,SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOOWNERZORDER);

        m_EnterLink.SubclassWindow(GetDlgItem(IDC_ENTER_CODE));
        m_EnterLink.SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON,HLINK_COMMANDBUTTON);
    }
    if (m_RunCount >= 10)
    {
        HMENU menu = GetSystemMenu(false);
        RemoveMenu(menu, SC_CLOSE, MF_BYCOMMAND);
        DWORD dwStyle = GetWindowLong(GWL_STYLE);
        dwStyle |= CS_NOCLOSE;
        SetWindowLong(GWL_STYLE, dwStyle);

        ::EnableWindow(GetDlgItem(IDCANCEL), false);
        srand ((uint32_t)time(NULL));
        SetTimer(0, 1000, NULL);
    }
    return TRUE;
}

LRESULT CSupportWindow::OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    HDC hdcStatic = (HDC)wParam;
    SetTextColor(hdcStatic, RGB(0, 0, 0));
    SetBkMode(hdcStatic, TRANSPARENT);
    return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
}

LRESULT CSupportWindow::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    static HPEN outline = CreatePen(PS_SOLID, 1, 0x00FFFFFF);
    static HBRUSH fill = CreateSolidBrush(0x00FFFFFF);
    SelectObject((HDC)wParam, outline);
    SelectObject((HDC)wParam, fill);

    RECT rect;
    GetClientRect(&rect);

    Rectangle((HDC)wParam, rect.left, rect.top, rect.right, rect.bottom);
    return TRUE;
}

LRESULT CSupportWindow::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_TimeOutTime -= 1;
    if (m_TimeOutTime == 0)
    {
        KillTimer(wParam);
        EnableContinue();
    }
    stdstr_f continue_txt(m_TimeOutTime > 0 ? "%s (%d)" : "%s", GS(MSG_SUPPORT_CONTINUE), m_TimeOutTime);
    SetWindowTextW(GetDlgItem(IDCANCEL), continue_txt.ToUTF16().c_str());
    return true;
}

LRESULT CSupportWindow::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return TRUE;
}

LRESULT CSupportWindow::OnSupportProject64(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    std::string SupportURL = stdstr_f("http://www.pj64-emu.com/support-project64.html?ver=%s", VER_FILE_VERSION_STR);
    ShellExecute(NULL, "open", SupportURL.c_str(), NULL, NULL, SW_SHOWMAXIMIZED);
    return TRUE;
}

LRESULT CSupportWindow::OnEnterCode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CSupportEnterCode EnterCodeWindow;
    EnterCodeWindow.DoModal(m_hWnd);
    if (UISettingsLoadDword(SupportWindows_RunCount) == -1)
    {
        EndDialog(wID);
    }
    return TRUE;
}