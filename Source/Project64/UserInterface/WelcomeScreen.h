/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once
#include <Project64\UserInterface\WTLControls\wtl-BitmapPicture.h>
#include "resource.h"

class WelcomeScreen :
    public CDialogImpl<WelcomeScreen>
{
public:
    BEGIN_MSG_MAP_EX(WelcomeScreen)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
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
    LRESULT OnColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnOkCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL & bHandled);

    static int CALLBACK SelectDirCallBack(HWND hwnd, DWORD uMsg, DWORD lp, DWORD lpData);

    CBitmapPicture m_Logo;
};