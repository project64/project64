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

class CDumpMemory :
    public CDebugDialog < CDumpMemory >
{
public:
    enum { IDD = IDD_Cheats_DumpMemory };

    CDumpMemory(CDebuggerUI * debugger);
    virtual ~CDumpMemory(void);

private:
    CDumpMemory(void);							// Disable default constructor
    CDumpMemory(const CDumpMemory&);			// Disable copy constructor
    CDumpMemory& operator=(const CDumpMemory&);	// Disable assignment

    enum DumpFormat
    {
        DisassemblyWithPC,
        RawBigEndian
    };

    BEGIN_MSG_MAP_EX(CDumpMemory)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
        END_MSG_MAP()

    LRESULT				OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT				OnClicked(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);

    bool DumpMemory(LPCSTR FileName, DumpFormat Format, DWORD StartPC, DWORD EndPC, DWORD DumpPC);

    CComboBox   m_FormatList;
    CEdit       m_FileName;
    CEditNumber m_StartAddress, m_EndAddress, m_PC;
};
