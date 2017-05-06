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
#include "stdafx.h"

#include "DebuggerUI.h"

CDumpMemory::CDumpMemory(CDebuggerUI * debugger) :
CDebugDialog<CDumpMemory>(debugger)
{
}

CDumpMemory::~CDumpMemory()
{
}

LRESULT	CDumpMemory::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_StartAddress.Attach(GetDlgItem(IDC_E_START_ADDR));
    m_EndAddress.Attach(GetDlgItem(IDC_E_END_ADDR));
    m_PC.Attach(GetDlgItem(IDC_E_ALT_PC));

    m_StartAddress.SetDisplayType(CEditNumber::DisplayHex);
    m_EndAddress.SetDisplayType(CEditNumber::DisplayHex);
    m_PC.SetDisplayType(CEditNumber::DisplayHex);

    m_StartAddress.SetValue(0x80000000, true, true);
    m_EndAddress.SetValue(0x803FFFF0, true, true);
    m_PC.SetValue(0x80000000);
    HWND hFormatList = GetDlgItem(IDC_FORMAT);
    int pos = ::SendMessage(hFormatList, CB_ADDSTRING, (WPARAM)0, (LPARAM)"TEXT - Disassembly + PC");
    ::SendMessage(hFormatList, CB_SETITEMDATA, (WPARAM)pos, (LPARAM)DisassemblyWithPC);
    ::SendMessage(hFormatList, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    WindowCreated();
    return TRUE;
}

LRESULT	CDumpMemory::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDC_BTN_CHOOSE_FILE:
    {
        g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_DumpMemory);

        CPath FileName;
        if (FileName.SelectFile(m_hWnd, CPath(CPath::MODULE_DIRECTORY), "Text file (*.txt)\0*.txt;\0All files (*.*)\0*.*\0", false))
        {
            if (FileName.GetExtension().length() == 0)
            {
                FileName.SetExtension("txt");
                SetDlgItemText(IDC_FILENAME, FileName);
            }
        }
        g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_DumpMemory);
    }
    break;
    case IDOK:
    {
        TCHAR FileName[MAX_PATH];
        int CurrentFormatSel = SendDlgItemMessage(IDC_FORMAT, CB_GETCURSEL, 0, 0);
        DumpFormat Format = (DumpFormat)SendDlgItemMessage(IDC_FORMAT, CB_GETITEMDATA, CurrentFormatSel, 0);
        DWORD StartPC = m_StartAddress.GetValue();
        DWORD EndPC = m_EndAddress.GetValue();
        DWORD DumpPC = m_PC.GetValue();
        GetDlgItemText(IDC_FILENAME, FileName, sizeof(FileName));
        if (strlen(FileName) == 0)
        {
            g_Notify->DisplayError("Please Choose target file");
            ::SetFocus(GetDlgItem(IDC_FILENAME));
            return false;
        }
        if (SendDlgItemMessage(IDC_USE_ALT_PC, BM_GETSTATE, 0, 0) != BST_CHECKED)
        {
            DumpPC = g_Reg->m_PROGRAM_COUNTER;
        }
        //disable buttons
        ::EnableWindow(GetDlgItem(IDC_E_START_ADDR), FALSE);
        ::EnableWindow(GetDlgItem(IDC_E_END_ADDR), FALSE);
        ::EnableWindow(GetDlgItem(IDC_E_ALT_PC), FALSE);
        ::EnableWindow(GetDlgItem(IDC_USE_ALT_PC), FALSE);
        ::EnableWindow(GetDlgItem(IDC_FILENAME), FALSE);
        ::EnableWindow(GetDlgItem(IDC_BTN_CHOOSE_FILE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_FORMAT), FALSE);
        ::EnableWindow(GetDlgItem(IDOK), FALSE);
        ::EnableWindow(GetDlgItem(IDCANCEL), FALSE);
        g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_DumpMemory);
        if (!DumpMemory(FileName, Format, StartPC, EndPC, DumpPC))
        {
            //enable buttons
            g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_DumpMemory);
            return false;
        }
        g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_DumpMemory);
    }
    EndDialog(0);
    break;
    }
    return FALSE;
}

bool CDumpMemory::DumpMemory(LPCSTR FileName, DumpFormat Format, DWORD StartPC, DWORD EndPC, DWORD /*DumpPC*/)
{
    switch (Format)
    {
    case DisassemblyWithPC:
    {
        CLog LogFile;
        if (!LogFile.Open(FileName))
        {
            g_Notify->DisplayError(stdstr_f("Failed to open\n%s", FileName).c_str());
            return false;
        }
        LogFile.SetFlush(false);
        LogFile.SetTruncateFile(false);
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef legacycode
        char Command[200];
        for (COpcode OpCode(StartPC);  OpCode.PC() < EndPC; OpCode.Next())
        {
            const char * szOpName = OpCode.OpcodeName();
            OpCode.OpcodeParam(Command);
            LogFile.LogF("%X: %-15s%s\r\n",OpCode.PC(),szOpName,Command);
        }
#endif
        m_StartAddress.SetValue(StartPC, true, true);
        m_EndAddress.SetValue(EndPC, true, true);
        return true;
    }
    break;
    }
    return false;
}