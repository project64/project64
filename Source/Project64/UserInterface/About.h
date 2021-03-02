#pragma once
#include <Project64\UserInterface\WTLControls\wtl-BitmapPicture.h>
#include "resource.h"

class CAboutDlg :
    public CDialogImpl<CAboutDlg>
{
public:
    BEGIN_MSG_MAP_EX(CAboutDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        COMMAND_ID_HANDLER(IDOK, OnOkCmd)
        COMMAND_ID_HANDLER(IDCANCEL, OnOkCmd)
    END_MSG_MAP()

    enum { IDD = IDD_About };

    CAboutDlg(CProjectSupport & Support);

private:
    CAboutDlg(void);
    CAboutDlg(const CAboutDlg&);
    CAboutDlg& operator=(const CAboutDlg&);

    void SetWindowDetais(int nIDDlgItem, int nAboveIDDlgItem, const wchar_t * Text, const HFONT &font);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnOkCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled);

    CProjectSupport & m_Support;
    CBitmapPicture m_Logo;
    CFont m_BoldFont;
    CFont m_TextFont;
};