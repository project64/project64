#include <stdafx.h>
#include <Project64\UserInterface\About.h>

CAboutDlg::CAboutDlg(CProjectSupport & Support) :
    m_Support(Support)
{
}

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    m_Logo.SubclassWindow(GetDlgItem(IDC_BMP_LOGO));
    m_Logo.SetBitmap(MAKEINTRESOURCE(IDB_ABOUT_LOGO));

    stdstr AboutMsg;
    if (m_Support.Validated() && strlen(m_Support.Name()) > 0)
    {
        AboutMsg += stdstr_f("Thank you %s for the support!\n\n", m_Support.Name());
    }
    AboutMsg += "Project64 is a completely free and open-source emulator for the Nintendo 64 and 64DD written in C++.\n\nCapable of playing your favorite N64 games on your PC with high definition graphics, excellent compatibility, save states, built - in cheat codes, and more.";

    CDC hDC = GetDC();
    float DPIScale = hDC.GetDeviceCaps(LOGPIXELSX) / 96.0f;
    LOGFONT lf = { 0 };
    CFontHandle(GetDlgItem(IDC_VERSION).GetFont()).GetLogFont(&lf);
    lf.lfHeight = (int)(16 * DPIScale);
    m_TextFont.CreateFontIndirect(&lf);
    lf.lfHeight = (int)(18 * DPIScale);
    lf.lfWeight += 200;
    m_BoldFont.CreateFontIndirect(&lf);

    SetWindowDetais(IDC_VERSION, IDC_BMP_LOGO, stdstr_f("Version: %s", VER_FILE_VERSION_STR).ToUTF16().c_str(), m_BoldFont);
    SetWindowDetais(IDC_ABOUT_PROJECT, IDC_VERSION, AboutMsg.ToUTF16().c_str(), m_TextFont);
    SetWindowDetais(IDC_THANKS_CORE, IDC_ABOUT_PROJECT, L"Special Thanks to previous core members:", m_BoldFont);
    SetWindowDetais(IDC_CORE_THANK_LIST, IDC_THANKS_CORE, L"Jabo, Smiff, Gent", m_TextFont);
    SetWindowDetais(IDC_THANKYOU, IDC_CORE_THANK_LIST, L"Thanks also goes to:", m_BoldFont);
    SetWindowDetais(IDC_THANKYOU_LIST, IDC_THANKYOU, L"Jahra!n, Witten, RadeonUser, Azimer, Shygoo, Frank, LuigiBlood, theboy181, Gonetz, BlueToonYoshi, Kimbjo, Melchior, retroben, AIO, krom", m_TextFont);

    return TRUE;
}

void CAboutDlg::SetWindowDetais(int nIDDlgItem, int nAboveIDDlgItem, const wchar_t * Text, const HFONT &font)
{
    CWindow Wnd = GetDlgItem(nIDDlgItem);
    Wnd.SetWindowText(Text);
    Wnd.SetFont(font);

    CDC hDC = GetDC();
    float DPIScale = hDC.GetDeviceCaps(LOGPIXELSX) / 96.0f;
    hDC.SelectFont(font);

    CRect rcWin;
    Wnd.GetWindowRect(&rcWin);
    ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rcWin, 2);
    if (hDC.DrawText(Text, -1, &rcWin, DT_LEFT | DT_CALCRECT | DT_WORDBREAK | DT_NOCLIP) > 0)
    {
        Wnd.SetWindowPos(NULL, 0, 0, rcWin.Width(), rcWin.Height(), SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER);
    }
    
    CWindow AboveWnd = GetDlgItem(nAboveIDDlgItem);
    AboveWnd.GetWindowRect(&rcWin);
    ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rcWin, 2);
    LONG Top = rcWin.bottom + (LONG)(8 * DPIScale);

    Wnd.GetWindowRect(&rcWin);
    ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rcWin, 2);
    Wnd.SetWindowPos(NULL, rcWin.left, Top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);
}


LRESULT CAboutDlg::OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    HDC hdcStatic = (HDC)wParam;
    SetTextColor(hdcStatic, RGB(0, 0, 0));
    SetBkMode(hdcStatic, TRANSPARENT);
    return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
}

LRESULT CAboutDlg::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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

LRESULT CAboutDlg::OnOkCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    EndDialog(0);
    return TRUE;
}