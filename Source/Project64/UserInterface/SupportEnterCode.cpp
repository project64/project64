#include "stdafx.h"

#include "SupportEnterCode.h"
#include "resource.h"
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>

class CRequestCode :
    public CDialogImpl<CRequestCode>
{
public:
    BEGIN_MSG_MAP_EX(CRequestCode)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        COMMAND_ID_HANDLER(IDOK, OnOkCmd)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

    enum { IDD = IDD_Support_RequestCode };

    CRequestCode(CProjectSupport & Support);
    void ShowOldCodeMsg();

private:
    CRequestCode(void);
    CRequestCode(const CRequestCode&);
    CRequestCode& operator=(const CRequestCode&);

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
    LRESULT OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    CProjectSupport & m_Support;
    bool m_ShowOldCodeMsg;
};

CSupportEnterCode::CSupportEnterCode(CProjectSupport & Support) :
    m_Support(Support)
{
}

LRESULT CSupportEnterCode::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SetWindowText(wGS(MSG_SUPPORT_ENTER_CODE_TITLE).c_str());
    CWindow hDescription = GetDlgItem(IDC_DESCRIPTION);
    CWindow MachineId = GetDlgItem(IDC_MACHINE_ID);
    CWindow OkBtn = GetDlgItem(IDOK);
    CWindow CancelBtn = GetDlgItem(IDCANCEL);

    std::wstring DescriptionText = wGS(MSG_SUPPORT_ENTER_CODE_DESC);
    hDescription.SetWindowText(DescriptionText.c_str());
    MachineId.SetWindowText(stdstr(m_Support.MachineID()).ToUTF16().c_str());
    OkBtn.SetWindowText(wGS(MSG_SUPPORT_OK).c_str());
    CancelBtn.SetWindowText(wGS(MSG_SUPPORT_CANCEL).c_str());

    m_RequestLink.SubclassWindow(GetDlgItem(IDC_REQUEST_LINK));
    m_RequestLink.SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON, HLINK_COMMANDBUTTON);

    CRect rcWin = { 0 };
    hDescription.GetClientRect(&rcWin);

    CDC hDC = hDescription.GetDC();
    HFONT hFont = hDescription.GetFont();
    if (hFont == nullptr)
    {
        hFont = (HFONT)::GetStockObject(SYSTEM_FONT);
    }
    hDC.SelectFont(hFont);
    if (hDC.DrawText(DescriptionText.c_str(), DescriptionText.length(), &rcWin, DT_LEFT | DT_CALCRECT | DT_WORDBREAK | DT_NOCLIP) > 0)
    {
        hDescription.SetWindowPos(nullptr, 0, 0, rcWin.right, rcWin.bottom, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER);
    }
    hDescription.GetWindowRect(&rcWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&rcWin, 2);

    MachineId.SetWindowPos(nullptr, rcWin.left, rcWin.bottom + 4, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);
    MachineId.GetWindowRect(&rcWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&rcWin, 2);

    CWindow Code = GetDlgItem(IDC_CODE);
    Code.SetWindowPos(nullptr, rcWin.left, rcWin.bottom + 4, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);
    Code.GetWindowRect(&rcWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&rcWin, 2);

    CWindow RequestDescption = GetDlgItem(IDC_REQUEST_DESCPTION);
    RequestDescption.ShowWindow(SWP_HIDEWINDOW);
    RequestDescption.SetWindowPos(nullptr, rcWin.left, rcWin.bottom + 10, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);
    RequestDescption.GetWindowRect(&rcWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&rcWin, 2);

    CWindow RequestLink = GetDlgItem(IDC_REQUEST_LINK);
    RequestLink.ShowWindow(SWP_HIDEWINDOW);
    RequestLink.SetWindowPos(nullptr, rcWin.left, rcWin.bottom + 4, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);
    RequestLink.GetWindowRect(&rcWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&rcWin, 2);

    RECT CancelBtnWin = { 0 };
    CancelBtn.GetWindowRect(&CancelBtnWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&CancelBtnWin, 2);
    CancelBtn.SetWindowPos(nullptr, CancelBtnWin.left, rcWin.bottom + 40, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);

    RECT OkBtnWin = { 0 };
    OkBtn.GetWindowRect(&OkBtnWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&OkBtnWin, 2);
    OkBtn.SetWindowPos(nullptr, OkBtnWin.left, rcWin.bottom + 40, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);
    OkBtn.GetWindowRect(&OkBtnWin);
    ::MapWindowPoints(nullptr, m_hWnd, (LPPOINT)&OkBtnWin, 2);

    GetWindowRect(&rcWin);
    SetRect(&rcWin, 0, 0, rcWin.Width(), OkBtnWin.bottom + 30);
    AdjustWindowRectEx(&rcWin, GetStyle(), GetMenu() != nullptr, GetExStyle());
    int32_t Left = (GetSystemMetrics(SM_CXSCREEN) - rcWin.Width()) / 2;
    int32_t	Top = (GetSystemMetrics(SM_CYSCREEN) - rcWin.Height()) / 2;
    MoveWindow(Left, Top, rcWin.Width(), rcWin.Height(), TRUE);
    return TRUE;
}

LRESULT CSupportEnterCode::OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CDCHandle hdcStatic = (HDC)wParam;
    hdcStatic.SetTextColor(RGB(0, 0, 0));
    hdcStatic.SetBkMode(TRANSPARENT);
    return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
}

LRESULT CSupportEnterCode::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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

LRESULT CSupportEnterCode::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return TRUE;
}

LRESULT CSupportEnterCode::OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    wchar_t code[50];
    if (!GetDlgItemText(IDC_CODE,code,sizeof(code) /sizeof(code[0])))
    {
        MessageBox(wGS(MSG_SUPPORT_ENTER_SUPPORT_CODE).c_str(), wGS(MSG_SUPPORT_PROJECT64).c_str(), MB_OK);
        return false;
    }
    GetDlgItem(IDOK).EnableWindow(false);
    GetDlgItem(IDCANCEL).EnableWindow(false);

    bool ValidCode = false;
    if (_wcsicmp(code,L"thank you from project64") == 0)
    {
        SetDlgItemText(IDC_CODE, L"");
        CRequestCode RequestWindow(m_Support);
        RequestWindow.ShowOldCodeMsg();
        RequestWindow.DoModal(m_hWnd);
        GetDlgItem(IDOK).EnableWindow(TRUE);
        GetDlgItem(IDCANCEL).EnableWindow(TRUE);
        return TRUE;
    }
    else if (m_Support.ValidateCode(stdstr().FromUTF16(code).c_str()))
    {
        ValidCode = true;
    }
    if (ValidCode)
    {
        MessageBox(wGS(MSG_SUPPORT_COMPLETE).c_str(), wGS(MSG_SUPPORT_PROJECT64).c_str(), MB_OK);
        EndDialog(wID);
    }
    else
    {
        MessageBox(wGS(MSG_SUPPORT_INCORRECT_CODE).c_str(), wGS(MSG_SUPPORT_PROJECT64).c_str(), MB_OK);
        GetDlgItem(IDOK).EnableWindow(TRUE);
        GetDlgItem(IDCANCEL).EnableWindow(TRUE);
    }
    return TRUE;
}

CRequestCode::CRequestCode(CProjectSupport & Support) :
    m_Support(Support),
    m_ShowOldCodeMsg(false)
{
}

void CRequestCode::ShowOldCodeMsg()
{
    m_ShowOldCodeMsg = true;
}

LRESULT CSupportEnterCode::OnRequestCode(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CRequestCode RequestWindow(m_Support);
    RequestWindow.DoModal(m_hWnd);
    return 0;
}

LRESULT CRequestCode::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (m_ShowOldCodeMsg)
    {
        SetDlgItemText(IDC_DESCRIPTION, L"We have changed the code to be unique to a machine, please enter the email you used to support Project64 with.");
    }
    return TRUE;
}

LRESULT CRequestCode::OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CDCHandle hdcStatic = (HDC)wParam;
    hdcStatic.SetTextColor(RGB(0, 0, 0));
    hdcStatic.SetBkMode(TRANSPARENT);
    return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
}

LRESULT CRequestCode::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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

LRESULT CRequestCode::OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    GetDlgItem(IDOK).EnableWindow(false);
    GetDlgItem(IDCANCEL).EnableWindow(false);
    if (m_Support.RequestCode(GetCWindowText(GetDlgItem(IDC_EMAIL)).c_str()))
    {
        MessageBox(wGS(MSG_SUPPORT_REQUESTCODE_SUCCESS).c_str(), wGS(MSG_SUPPORT_REQUESTCODE_TITLE).c_str(), MB_OK);
        EndDialog(wID);
    }
    else
    {
        MessageBox(wGS(MSG_SUPPORT_REQUESTCODE_FAIL).c_str(), wGS(MSG_SUPPORT_REQUESTCODE_TITLE).c_str(), MB_OK);
        GetDlgItem(IDOK).EnableWindow(true);
        GetDlgItem(IDCANCEL).EnableWindow(true);
    }
    return TRUE;
}

LRESULT CRequestCode::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return TRUE;
}
