#ifdef _WIN32
#include <Windows.h>
#include <commctrl.h>
#include <memory>
#include <windowsx.h>
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1910
#include <intrin.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Settings/Settings.h"
#include <Common/StdString.h>
#include <stdint.h>

#include "Debugger/RSPDebuggerUI.h"
#include "Debugger/RSPRegistersUI.h"
#include "RSP Command.h"
#include "Rsp.h"
#include "breakpoint.h"
#include "resource.h"
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/Recompiler/RspProfiling.h>
#include <Project64-rsp-core/Recompiler/RspRecompilerCPU.h>
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Project64-rsp-core/Settings/RspSettingsID.h>
#include <Project64-rsp-core/Version.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspLog.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Project64-rsp-core/cpu/RspTypes.h>

void ProcessMenuItem(int32_t ID);
#ifdef _WIN32
BOOL CALLBACK CompilerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
HMENU hRSPMenu = NULL;
#endif

DEBUG_INFO DebugInfo;
void * hinstDLL;
std::unique_ptr<RSPDebuggerUI> g_RSPDebuggerUI;

extern uint8_t * pLastSecondary;

// DLL info
const char * AppName(void)
{
    static stdstr_f Name("RSP %s", VER_FILE_VERSION_STR);
    return Name.c_str();
}
const char * AboutMsg(void)
{
    static stdstr_f Msg("RSP emulation plugin\nMade for Project64 (c)\nVersion %s\n\nby Jabo and Zilmar", VER_FILE_VERSION_STR);
    return Msg.c_str();
}

// Functions

uint32_t AsciiToHex(char * HexValue)
{
    size_t Finish, Count;
    uint32_t Value = 0;

    Finish = strlen(HexValue);
    if (Finish > 8)
    {
        Finish = 8;
    }

    for (Count = 0; Count < Finish; Count++)
    {
        Value = (Value << 4);
        switch (HexValue[Count])
        {
        case '0': break;
        case '1': Value += 1; break;
        case '2': Value += 2; break;
        case '3': Value += 3; break;
        case '4': Value += 4; break;
        case '5': Value += 5; break;
        case '6': Value += 6; break;
        case '7': Value += 7; break;
        case '8': Value += 8; break;
        case '9': Value += 9; break;
        case 'A': Value += 10; break;
        case 'a': Value += 10; break;
        case 'B': Value += 11; break;
        case 'b': Value += 11; break;
        case 'C': Value += 12; break;
        case 'c': Value += 12; break;
        case 'D': Value += 13; break;
        case 'd': Value += 13; break;
        case 'E': Value += 14; break;
        case 'e': Value += 14; break;
        case 'F': Value += 15; break;
        case 'f': Value += 15; break;
        default:
            Value = (Value >> 4);
            Count = Finish;
        }
    }
    return Value;
}

void DisplayError(char * Message, ...)
{
    char Msg[400];
    va_list ap;

    va_start(ap, Message);
    vsprintf(Msg, Message, ap);
    va_end(ap);
#ifdef _WIN32
    MessageBoxA(NULL, Msg, "Error", MB_OK | MB_ICONERROR);
#else
    fputs(&Msg[0], stderr);
#endif
}

/*
Function: CloseDLL
Purpose: This function is called when the emulator is closing
down allowing the DLL to de-initialize.
Input: None
Output: None
*/

EXPORT void CloseDLL(void)
{
    FreeRSP();
}

/*
Function: DllAbout
Purpose: This function is optional function that is provided
to give further information about the DLL.
Input: A handle to the window that calls this function
Output: None
*/

EXPORT void DllAbout(void * hParent)
{
#ifdef _WIN32
    MessageBoxA((HWND)hParent, AboutMsg(), "About", MB_OK | MB_ICONINFORMATION);
#else
    puts(AboutMsg());
#endif
}

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID /*lpvReserved*/)
{
    hinstDLL = hinst;
    if (fdwReason == DLL_PROCESS_DETACH)
    {
        StopCPULog();
    }
    return true;
}

void FixMenuState(void)
{
    short Set_MultiThreadedDefault = FindSystemSettingId("Rsp Multi Threaded Default");
    bool MultiThreadedDefault = Set_MultiThreadedDefault != 0 ? GetSystemSetting(Set_MultiThreadedDefault) != 0 : false;

    EnableMenuItem(hRSPMenu, ID_RSPCOMMANDS, MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
    EnableMenuItem(hRSPMenu, ID_RSPREGISTERS, MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
    EnableMenuItem(hRSPMenu, ID_PROFILING_RESETSTATS, MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
    EnableMenuItem(hRSPMenu, ID_PROFILING_GENERATELOG, MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
    EnableMenuItem(hRSPMenu, ID_DUMP_RSPCODE, MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
    EnableMenuItem(hRSPMenu, ID_DUMP_DMEM, MF_BYCOMMAND | (DebuggingEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));

    CheckMenuItem(hRSPMenu, ID_CPUMETHOD_RECOMPILER, MF_BYCOMMAND | (g_CPUCore == RecompilerCPU ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_CPUMETHOD_INTERPT, MF_BYCOMMAND | (g_CPUCore == InterpreterCPU ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_BREAKONSTARTOFTASK, MF_BYCOMMAND | (BreakOnStart ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_LOGRDPCOMMANDS, MF_BYCOMMAND | (LogRDP ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_SETTINGS_HLEALISTTASK, MF_BYCOMMAND | (HleAlistTask ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_SETTINGS_LOGX86CODE, MF_BYCOMMAND | (LogX86Code ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_SETTINGS_MULTITHREADED, MF_BYCOMMAND | (MultiThreadedDefault ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_PROFILING_ON, MF_BYCOMMAND | (Profiling ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_PROFILING_OFF, MF_BYCOMMAND | (Profiling ? MFS_UNCHECKED : MF_CHECKED));
    CheckMenuItem(hRSPMenu, ID_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND | (IndvidualBlock ? MFS_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hRSPMenu, ID_SHOWCOMPILERERRORS, MF_BYCOMMAND | (ShowErrors ? MFS_CHECKED : MF_UNCHECKED));
}
#endif

/*
Function: GetDllInfo
Purpose: This function allows the emulator to gather information
about the DLL by filling in the PluginInfo structure.
Input: A pointer to a PLUGIN_INFO structure that needs to be
filled by the function. (see def above)
Output: None
*/

EXPORT void GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = 0x0103;
    PluginInfo->Type = PLUGIN_TYPE_RSP;
#ifdef _DEBUG
    sprintf(PluginInfo->Name, "RSP debug plugin %s", VER_FILE_VERSION_STR);
#else
    sprintf(PluginInfo->Name, "RSP plugin %s", VER_FILE_VERSION_STR);
#endif
    PluginInfo->Reserved2 = false;
    PluginInfo->Reserved1 = true;
}

/*
Function: GetRspDebugInfo
Purpose: This function allows the emulator to gather information
about the debug capabilities of the DLL by filling in
the DebugInfo structure.
Input: A pointer to a RSPDEBUG_INFO structure that needs to be
filled by the function. (see def above)
Output: None
*/

EXPORT void GetRspDebugInfo(RSPDEBUG_INFO * _DebugInfo)
{
#ifdef _WIN32
    if (hRSPMenu == NULL)
    {
        hRSPMenu = LoadMenu((HINSTANCE)hinstDLL, MAKEINTRESOURCE(RspMenu));
        FixMenuState();
    }
    _DebugInfo->hRSPMenu = hRSPMenu;
#endif
    _DebugInfo->ProcessMenuItem = ProcessMenuItem;

    _DebugInfo->UseBPoints = true;
    sprintf(_DebugInfo->BPPanelName, " RSP ");
    _DebugInfo->Add_BPoint = Add_BPoint;
    _DebugInfo->CreateBPPanel = CreateBPPanel;
    _DebugInfo->HideBPPanel = HideBPPanel;
    _DebugInfo->PaintBPPanel = PaintBPPanel;
    _DebugInfo->RefreshBpoints = RefreshBpoints;
    _DebugInfo->RemoveAllBpoint = RemoveAllBpoint;
    _DebugInfo->RemoveBpoint = RemoveBpoint;
    _DebugInfo->ShowBPPanel = ShowBPPanel;

    _DebugInfo->Enter_RSP_Commands_Window = Enter_RSP_Commands_Window;
}

/*
Function: InitiateRSP
Purpose: This function is called when the DLL is started to give
information from the emulator that the N64 RSP interface needs.
Input: Rsp_Info is passed to this function which is defined
above.
CycleCount is the number of cycles between switching
control between the RSP and r4300i core.
Output: None
*/
EXPORT void InitiateRSP(RSP_INFO Rsp_Info, uint32_t * CycleCount)
{
    g_RSPDebuggerUI.reset(new RSPDebuggerUI(RSPSystem));
    g_RSPDebugger = g_RSPDebuggerUI.get();
    InitilizeRSP(Rsp_Info);
    *CycleCount = 0;
}

/*
Function: InitiateRSPDebugger
Purpose: This function is called when the DLL is started to give
information from the emulator that the N64 RSP
interface needs to integrate the debugger with the
rest of the emulator.
Input: DebugInfo is passed to this function which is defined
above.
Output: None
*/

EXPORT void InitiateRSPDebugger(DEBUG_INFO Debug_Info)
{
    DebugInfo = Debug_Info;
}

#ifdef _WIN32
void ProcessMenuItem(int32_t ID)
{
    switch (ID)
    {
    case ID_RSPCOMMANDS: Enter_RSP_Commands_Window(); break;
    case ID_RSPREGISTERS: Enter_RSP_Register_Window(); break;
    case ID_DUMP_RSPCODE: DumpRSPCode(); break;
    case ID_DUMP_DMEM: DumpRSPData(); break;
    case ID_PROFILING_ON:
    case ID_PROFILING_OFF:
    {
        bool ProfilingOn = ID == ID_PROFILING_ON;
        CheckMenuItem(hRSPMenu, ID_PROFILING_ON, MF_BYCOMMAND | (ProfilingOn ? MFS_CHECKED : MFS_UNCHECKED));
        CheckMenuItem(hRSPMenu, ID_PROFILING_OFF, MF_BYCOMMAND | (ProfilingOn ? MFS_UNCHECKED : MFS_CHECKED));
        SetSetting(Set_Profiling, ProfilingOn);
        if (DebuggingEnabled)
        {
            Profiling = ProfilingOn;
        }
        break;
    }
    case ID_PROFILING_RESETSTATS: ResetTimerList(); break;
    case ID_PROFILING_GENERATELOG: GenerateTimerResults(); break;
    case ID_PROFILING_LOGINDIVIDUALBLOCKS:
    {
        bool Checked = (GetMenuState(hRSPMenu, ID_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND) & MFS_CHECKED) != 0;
        CheckMenuItem(hRSPMenu, ID_PROFILING_LOGINDIVIDUALBLOCKS, MF_BYCOMMAND | (Checked ? MFS_UNCHECKED : MFS_CHECKED));
        SetSetting(Set_IndvidualBlock, !Checked);
        if (DebuggingEnabled)
        {
            IndvidualBlock = !Checked;
        }
        break;
    }
    case ID_SHOWCOMPILERERRORS:
    {
        bool Checked = (GetMenuState(hRSPMenu, ID_SHOWCOMPILERERRORS, MF_BYCOMMAND) & MFS_CHECKED) != 0;
        CheckMenuItem(hRSPMenu, ID_SHOWCOMPILERERRORS, MF_BYCOMMAND | (Checked ? MFS_UNCHECKED : MFS_CHECKED));
        SetSetting(Set_ShowErrors, !Checked);
        if (DebuggingEnabled)
        {
            ShowErrors = !Checked;
        }
        break;
    }
    break;
    case ID_COMPILER:
        DialogBoxA((HINSTANCE)hinstDLL, "RSPCOMPILER", HWND_DESKTOP, (DLGPROC)CompilerDlgProc);
        break;
    case ID_BREAKONSTARTOFTASK:
    {
        bool Checked = (GetMenuState(hRSPMenu, ID_BREAKONSTARTOFTASK, MF_BYCOMMAND) & MFS_CHECKED) != 0;
        CheckMenuItem(hRSPMenu, ID_BREAKONSTARTOFTASK, MF_BYCOMMAND | (Checked ? MFS_UNCHECKED : MFS_CHECKED));
        SetSetting(Set_BreakOnStart, !Checked);
        if (DebuggingEnabled)
        {
            BreakOnStart = !Checked;
        }
        break;
    }
    case ID_LOGRDPCOMMANDS:
    {
        bool Checked = (GetMenuState(hRSPMenu, ID_LOGRDPCOMMANDS, MF_BYCOMMAND) & MFS_CHECKED) != 0;
        CheckMenuItem(hRSPMenu, ID_LOGRDPCOMMANDS, MF_BYCOMMAND | (Checked ? MFS_UNCHECKED : MFS_CHECKED));
        SetSetting(Set_LogRDP, !Checked);
        if (DebuggingEnabled)
        {
            LogRDP = !Checked;
            if (LogRDP)
            {
                RDPLog.StartLog();
            }
            else
            {
                RDPLog.StopLog();
            }
        }
        break;
    }
    case ID_SETTINGS_LOGX86CODE:
    {
        bool Checked = (GetMenuState(hRSPMenu, ID_SETTINGS_LOGX86CODE, MF_BYCOMMAND) & MFS_CHECKED) != 0;
        CheckMenuItem(hRSPMenu, ID_SETTINGS_LOGX86CODE, MF_BYCOMMAND | (Checked ? MFS_UNCHECKED : MFS_CHECKED));
        SetSetting(Set_LogX86Code, !Checked);
        if (DebuggingEnabled)
        {
            LogX86Code = !Checked;
            if (LogX86Code)
            {
                StartCPULog();
            }
            else
            {
                StopCPULog();
            }
        }
        break;
    }
    case ID_SETTINGS_HLEALISTTASK:
    {
        bool Checked = (GetMenuState(hRSPMenu, ID_SETTINGS_HLEALISTTASK, MF_BYCOMMAND) & MFS_CHECKED) != 0;
        CheckMenuItem(hRSPMenu, ID_SETTINGS_HLEALISTTASK, MF_BYCOMMAND | (Checked ? MFS_UNCHECKED : MFS_CHECKED));
        SetSetting(Set_HleAlistTask, !Checked);
        break;
    }
    case ID_SETTINGS_MULTITHREADED:
    {
        bool Checked = (GetMenuState(hRSPMenu, ID_SETTINGS_MULTITHREADED, MF_BYCOMMAND) & MFS_CHECKED) != 0;
        CheckMenuItem(hRSPMenu, ID_SETTINGS_MULTITHREADED, MF_BYCOMMAND | (Checked ? MFS_UNCHECKED : MFS_CHECKED));
        short Set_MultiThreadedDefault = FindSystemSettingId("Rsp Multi Threaded Default");
        if (Set_MultiThreadedDefault != 0)
        {
            SetSystemSetting(Set_MultiThreadedDefault, !Checked);
        }
        break;
    }
    case ID_CPUMETHOD_RECOMPILER:
        SetSetting(Set_CPUCore, RecompilerCPU);
        g_CPUCore = RecompilerCPU;
        FixMenuState();
        SetCPU(RecompilerCPU);
        break;
    case ID_CPUMETHOD_INTERPT:
        SetSetting(Set_CPUCore, InterpreterCPU);
        g_CPUCore = InterpreterCPU;
        FixMenuState();
        SetCPU(InterpreterCPU);
        break;
    }
}
#endif

/*
Function: RomOpen
Purpose: This function is called when a ROM is opened.
Input: None
Output: None
*/

EXPORT void RomOpen(void)
{
    RspRomOpened();
    if (DebuggingEnabled)
    {
        EnableDebugging(true);
    }
}

/*
Function: RomClosed
Purpose: This function is called when a ROM is closed.
Input: None
Output: None
*/

EXPORT void RomClosed(void)
{
    RspRomClosed();
}

#ifdef _WIN32
static bool GetBooleanCheck(HWND hDlg, DWORD DialogID)
{
    return (IsDlgButtonChecked(hDlg, DialogID) == BST_CHECKED) ? true : false;
}

BOOL CALLBACK CompilerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    char Buffer[256];

    switch (uMsg)
    {
    case WM_INITDIALOG:
        if (Compiler.bDest == true)
            CheckDlgButton(hDlg, IDC_COMPILER_DEST, BST_CHECKED);
        if (Compiler.mmx == true)
            CheckDlgButton(hDlg, IDC_CHECK_MMX, BST_CHECKED);
        if (Compiler.mmx2 == true)
            CheckDlgButton(hDlg, IDC_CHECK_MMX2, BST_CHECKED);
        if (Compiler.sse == true)
            CheckDlgButton(hDlg, IDC_CHECK_SSE, BST_CHECKED);

        if (Compiler.bAlignVector == true)
            CheckDlgButton(hDlg, IDC_COMPILER_ALIGNVEC, BST_CHECKED);

        if (Compiler.bSections == true)
            CheckDlgButton(hDlg, IDC_COMPILER_SECTIONS, BST_CHECKED);
        if (Compiler.bGPRConstants == true)
            CheckDlgButton(hDlg, IDC_COMPILER_GPRCONSTANTS, BST_CHECKED);
        if (Compiler.bReOrdering == true)
            CheckDlgButton(hDlg, IDC_COMPILER_REORDER, BST_CHECKED);
        if (Compiler.bFlags == true)
            CheckDlgButton(hDlg, IDC_COMPILER_FLAGS, BST_CHECKED);
        if (Compiler.bAccum == true)
            CheckDlgButton(hDlg, IDC_COMPILER_ACCUM, BST_CHECKED);

        SetTimer(hDlg, 1, 250, NULL);
        break;
    case WM_TIMER:
        sprintf(Buffer, "x86: %2.2f KB / %2.2f KB", (float)(RecompPos - RecompCode) / 1024.0F,
                pLastSecondary ? (float)((pLastSecondary - RecompCodeSecondary) / 1024.0F) : 0);

        SetDlgItemTextA(hDlg, IDC_COMPILER_BUFFERS, Buffer);
        break;
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            Compiler.bDest = GetBooleanCheck(hDlg, IDC_COMPILER_DEST);
            Compiler.bAccum = GetBooleanCheck(hDlg, IDC_COMPILER_ACCUM);
            Compiler.mmx = GetBooleanCheck(hDlg, IDC_CHECK_MMX);
            Compiler.mmx2 = GetBooleanCheck(hDlg, IDC_CHECK_MMX2);
            Compiler.sse = GetBooleanCheck(hDlg, IDC_CHECK_SSE);
            Compiler.bSections = GetBooleanCheck(hDlg, IDC_COMPILER_SECTIONS);
            Compiler.bReOrdering = GetBooleanCheck(hDlg, IDC_COMPILER_REORDER);
            Compiler.bGPRConstants = GetBooleanCheck(hDlg, IDC_COMPILER_GPRCONSTANTS);
            Compiler.bFlags = GetBooleanCheck(hDlg, IDC_COMPILER_FLAGS);
            Compiler.bAlignVector = GetBooleanCheck(hDlg, IDC_COMPILER_ALIGNVEC);
            SetSetting(Set_CheckDest, Compiler.bDest);
            SetSetting(Set_Accum, Compiler.bAccum);
            SetSetting(Set_Mmx, Compiler.mmx);
            SetSetting(Set_Mmx2, Compiler.mmx2);
            SetSetting(Set_Sse, Compiler.sse);
            SetSetting(Set_Sections, Compiler.bSections);
            SetSetting(Set_ReOrdering, Compiler.bReOrdering);
            SetSetting(Set_GPRConstants, Compiler.bGPRConstants);
            SetSetting(Set_Flags, Compiler.bFlags);
            SetSetting(Set_AlignVector, Compiler.bAlignVector);

            KillTimer(hDlg, 1);
            EndDialog(hDlg, true);
            break;
        case IDCANCEL:
            KillTimer(hDlg, 1);
            EndDialog(hDlg, true);
            break;
        }
        break;
    default:
        return false;
    }
    return true;
}

BOOL CALLBACK ConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    HWND hWndItem;
    DWORD value;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        if (AudioHle == true)
        {
            CheckDlgButton(hDlg, IDC_AUDIOHLE, BST_CHECKED);
        }
        if (GraphicsHle == true)
        {
            CheckDlgButton(hDlg, IDC_GRAPHICSHLE, BST_CHECKED);
        }

        hWndItem = GetDlgItem(hDlg, IDC_COMPILER_SELECT);
        ComboBox_AddString(hWndItem, "Interpreter");
        ComboBox_AddString(hWndItem, "Recompiler");
        ComboBox_SetCurSel(hWndItem, g_CPUCore);
        break;
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            hWndItem = GetDlgItem(hDlg, IDC_COMPILER_SELECT);
            value = ComboBox_GetCurSel(hWndItem);
            SetCPU((RSPCpuType)value);

            AudioHle = GetBooleanCheck(hDlg, IDC_AUDIOHLE);
            GraphicsHle = GetBooleanCheck(hDlg, IDC_GRAPHICSHLE);

            EndDialog(hDlg, true);
            break;
        case IDCANCEL:
            EndDialog(hDlg, true);
            break;
        }
        break;
    default:
        return false;
    }
    return true;
}
#endif

/*EXPORT void DllConfig(HWND hWnd)
{
	// DialogBox(hinstDLL, "RSPCONFIG", hWnd, ConfigDlgProc);
	DialogBox(hinstDLL, "RSPCONFIG", GetForegroundWindow(), ConfigDlgProc);
}*/

EXPORT void EnableDebugging(int Enabled)
{
    DebuggingEnabled = Enabled != 0;
    if (DebuggingEnabled)
    {
        BreakOnStart = GetSetting(Set_BreakOnStart) != 0;
        g_CPUCore = (RSPCpuType)GetSetting(Set_CPUCore);
        LogRDP = GetSetting(Set_LogRDP) != 0;
        LogX86Code = GetSetting(Set_LogX86Code) != 0;
        Profiling = GetSetting(Set_Profiling) != 0;
        IndvidualBlock = GetSetting(Set_IndvidualBlock) != 0;
        ShowErrors = GetSetting(Set_ShowErrors) != 0;
        HleAlistTask = GetSetting(Set_HleAlistTask) != 0;

        Compiler.bDest = GetSetting(Set_CheckDest) != 0;
        Compiler.bAccum = GetSetting(Set_Accum) != 0;
        Compiler.mmx = GetSetting(Set_Mmx) != 0;
        Compiler.mmx2 = GetSetting(Set_Mmx2) != 0;
        Compiler.sse = GetSetting(Set_Sse) != 0;
        Compiler.bSections = GetSetting(Set_Sections) != 0;
        Compiler.bReOrdering = GetSetting(Set_ReOrdering) != 0;
        Compiler.bGPRConstants = GetSetting(Set_GPRConstants) != 0;
        Compiler.bFlags = GetSetting(Set_Flags) != 0;
        Compiler.bAlignVector = GetSetting(Set_AlignVector) != 0;
        SetCPU(g_CPUCore);
    }
#ifdef _WIN32
    FixMenuState();
#else
    fputs("FixMenuState()\n", stderr);
#endif
    if (LogRDP)
    {
        RDPLog.StartLog();
    }
    if (LogX86Code)
    {
        StartCPULog();
    }
}

EXPORT void PluginLoaded(void)
{
    RspPluginLoaded();
}

#ifdef _WIN32
void UseUnregisteredSetting(int /*SettingID*/)
{
    DebugBreak();
}
#endif
