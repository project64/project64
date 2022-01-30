#pragma once
#include <Project64\UserInterface\WTLControls\wtl-BitmapPicture.h>
#include "resource.h"

class WelcomeScreen :
    public CDialogImpl<WelcomeScreen>
{
public:
    BEGIN_MSG_MAP_EX(WelcomeScreen)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic)
        MSG_WM_ERASEBKGND(OnEraseBackground)
        COMMAND_ID_HANDLER_EX(IDC_SELECT_GAME_DIR, SelectGameDir)
        COMMAND_ID_HANDLER(IDOK, OnOkCmd)
    END_MSG_MAP()

    enum { IDD = IDD_Welcome };
    
    WelcomeScreen(void);

private:
    WelcomeScreen(const WelcomeScreen&);
    WelcomeScreen& operator=(const WelcomeScreen&);

    void SelectGameDir(UINT Code, int id, HWND ctl);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    HBRUSH OnCtlColorStatic(CDCHandle dc, CStatic wndStatic);
    BOOL OnEraseBackground(CDCHandle dc);
    LRESULT OnOkCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled);

    static int CALLBACK SelectDirCallBack(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM lpData);

    CBitmapPicture m_Logo;
};
