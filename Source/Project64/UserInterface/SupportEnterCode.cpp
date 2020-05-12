#include "stdafx.h"
#include "SupportEnterCode.h"
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
#include "resource.h"

LRESULT CSupportEnterCode::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    ::SetWindowTextW(m_hWnd, wGS(MSG_SUPPORT_ENTER_CODE_TITLE).c_str());
    ::SetWindowTextW(GetDlgItem(IDOK), wGS(MSG_SUPPORT_OK).c_str());
    ::SetWindowTextW(GetDlgItem(IDCANCEL), wGS(MSG_SUPPORT_CANCEL).c_str());
    ::SetWindowTextW(GetDlgItem(IDC_DESCRIPTION), wGS(MSG_SUPPORT_ENTER_CODE_DESC).c_str());
    return TRUE;
}

LRESULT CSupportEnterCode::OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    HDC hdcStatic = (HDC)wParam;
    SetTextColor(hdcStatic, RGB(0, 0, 0));
    SetBkMode(hdcStatic, TRANSPARENT);
    return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
}

LRESULT CSupportEnterCode::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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
    if (_wcsicmp(code,L"thank you from project64") != 0)
    {
        MessageBox(wGS(MSG_SUPPORT_INCORRECT_CODE).c_str(), wGS(MSG_SUPPORT_PROJECT64).c_str(), MB_OK);
        return false;
    }
    UISettingsSaveDword(SupportWindows_RunCount, (uint32_t) -1);
    CSettingTypeApplication::Flush();
    MessageBox(wGS(MSG_SUPPORT_COMPLETE).c_str(), wGS(MSG_SUPPORT_PROJECT64).c_str(), MB_OK);
    EndDialog(wID);
    return TRUE;
}