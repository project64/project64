#include "stdafx.h"

#include "DebuggerUI.h"

#include <Project64-core/N64System/Mips/OpCodeName.h>

CDumpMemory::CDumpMemory(CDebuggerUI * debugger) :
CDebugDialog<CDumpMemory>(debugger)
{
}

CDumpMemory::~CDumpMemory()
{
}

LRESULT CDumpMemory::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    DlgSavePos_Init(DebuggerUI_MemoryDumpPos);

    m_StartAddress.Attach(GetDlgItem(IDC_E_START_ADDR));
    m_EndAddress.Attach(GetDlgItem(IDC_E_END_ADDR));
    m_PC.Attach(GetDlgItem(IDC_E_ALT_PC));
    m_FormatList.Attach(GetDlgItem(IDC_FORMAT));
    m_FileName.Attach(GetDlgItem(IDC_FILENAME));

    m_StartAddress.SetDisplayType(CEditNumber32::DisplayHex);
    m_EndAddress.SetDisplayType(CEditNumber32::DisplayHex);
    m_PC.SetDisplayType(CEditNumber32::DisplayHex);

    uint32_t startAddress = 0x80000000;
    uint32_t endAddress = startAddress + (g_MMU ? g_MMU->RdramSize() : 0x400000);

    m_StartAddress.SetValue(startAddress, DisplayMode::AllHex);
    m_EndAddress.SetValue(endAddress, DisplayMode::AllHex);
    m_PC.SetValue(startAddress);
    
    int nIndex = m_FormatList.AddString(L"TEXT - Disassembly + PC");
    m_FormatList.SetItemData(nIndex, (DWORD_PTR)DisassemblyWithPC);

    nIndex = m_FormatList.AddString(L"RAW - Big Endian (N64)");
    m_FormatList.SetItemData(nIndex, (LPARAM)RawBigEndian);

    m_FormatList.SetCurSel(0);

    LoadWindowPos();
    WindowCreated();
    return TRUE;
}

void CDumpMemory::OnExitSizeMove(void)
{
    SaveWindowPos(0);
}

LRESULT    CDumpMemory::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDC_BTN_CHOOSE_FILE:
    {
        g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_DumpMemory);

        int CurrentFormatSel = m_FormatList.GetCurSel();
        DumpFormat Format = (DumpFormat)m_FormatList.GetItemData(CurrentFormatSel);

        const char* FileFilter = "All files (*.*)\0*.*\0";

        if (Format == RawBigEndian)
        {
            FileFilter = "Binary file (*.bin)\0*.bin;\0All files (*.*)\0*.*\0";
        }
        else if (Format == DisassemblyWithPC)
        {
            FileFilter = "Text file (*.txt)\0*.txt;\0All files (*.*)\0*.*\0";
        }

        CPath FileName;
        
        if (FileName.SelectFile(m_hWnd, CPath(CPath::MODULE_DIRECTORY), FileFilter, false))
        {
            if (FileName.GetExtension().length() == 0)
            {
                FileName.SetExtension(Format == RawBigEndian ? "bin" : "txt");
                m_FileName.SetWindowText(stdstr((const char *)FileName).ToUTF16().c_str());
            }
        }
        g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_DumpMemory);
    }
    break;
    case IDOK:
    {
        TCHAR FileName[MAX_PATH];
        int CurrentFormatSel = m_FormatList.GetCurSel();
        DumpFormat Format = (DumpFormat) m_FormatList.GetItemData(CurrentFormatSel);
        
        DWORD StartPC = m_StartAddress.GetValue();
        DWORD EndPC = m_EndAddress.GetValue();
        DWORD DumpPC = m_PC.GetValue();
        GetDlgItemText(IDC_FILENAME, FileName, sizeof(FileName));
        if (wcslen(FileName) == 0)
        {
            g_Notify->DisplayWarning("Please choose target file");
            ::SetFocus(GetDlgItem(IDC_FILENAME));
            return false;
        }
        if (SendDlgItemMessage(IDC_USE_ALT_PC, BM_GETSTATE, 0, 0) != BST_CHECKED)
        {
            DumpPC = StartPC;
        }
        // Disable buttons
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
            // Enable buttons
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

bool CDumpMemory::DumpMemory(LPCTSTR FileName, DumpFormat Format, DWORD StartPC, DWORD EndPC, DWORD DumpPC)
{
    stdstr strFile = stdstr().FromUTF16(FileName);

    if (Format == DisassemblyWithPC)
    {
        CLog LogFile;
        if (!LogFile.Open(strFile.c_str()))
        {
            g_Notify->DisplayError(stdstr_f("Failed to open\n%s", strFile.c_str()).c_str());
            return false;
        }
        LogFile.SetFlush(false);
        LogFile.SetTruncateFile(false);

        for (uint32_t pc = StartPC; pc < EndPC; pc += 4, DumpPC += 4)
        {
            OPCODE opcode;
            m_Debugger->DebugLoad_VAddr(pc, opcode.Hex);

            const char* command = R4300iOpcodeName(opcode.Hex, DumpPC);

            char* cmdName = strtok((char*)command, "\t");
            char* cmdArgs = strtok(nullptr, "\t");
            cmdArgs = cmdArgs ? cmdArgs : "";

            LogFile.LogF("%X: %-15s%s\r\n", DumpPC, cmdName, cmdArgs);
        }

        m_StartAddress.SetValue(StartPC, DisplayMode::AllHex);
        m_EndAddress.SetValue(EndPC, DisplayMode::AllHex);
        return true;
    }

    if (Format == RawBigEndian)
    {
        CFile dumpFile;

        if (!dumpFile.Open(strFile.c_str(), CFile::modeCreate | CFile::modeWrite))
        {
            g_Notify->DisplayError(stdstr_f("Failed to open\n%s", strFile.c_str()).c_str());
            return false;
        }

        uint32_t dumpLen = EndPC - StartPC;
        uint8_t* dumpBuf = (uint8_t*)malloc(dumpLen);
        uint32_t dumpIdx = 0;

        for (uint32_t pc = StartPC; pc < EndPC; pc++, dumpIdx++)
        {
            bool bReadable = m_Debugger->DebugLoad_VAddr(pc, dumpBuf[dumpIdx]);

            if (!bReadable)
            {
                g_Notify->DisplayError(stdstr_f("Address error\n%s", strFile.c_str()).c_str());
                dumpFile.Close();
                free(dumpBuf);
                return false;
            }
        }

        dumpFile.SeekToBegin();
        dumpFile.Write(dumpBuf, dumpLen);
        dumpFile.Close();
        free(dumpBuf);
        return true;
    }

    return false;
}
