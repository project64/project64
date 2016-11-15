#pragma once

class CSupportEnterCode :
    public CDialogImpl<CSupportEnterCode>
{
public:
    BEGIN_MSG_MAP_EX(CSettingConfig)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        COMMAND_ID_HANDLER(IDOK, OnOkCmd)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
        END_MSG_MAP()

        enum { IDD = IDD_Support_EnterCode };

private:
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
    LRESULT OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};