#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "RSP Command.h"
#include "Rsp.h"
#include "breakpoint.h"
#include "memory.h"
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPInstruction.h>
#include <Project64-rsp-core/cpu/RSPOpcode.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspTypes.h>

#define RSP_MaxCommandLines 30

#define RSP_Status_PC 1
#define RSP_Status_BP 2

#define IDC_LIST 1000
#define IDC_ADDRESS 1001
#define IDC_FUNCTION_COMBO 1002
#define IDC_GO_BUTTON 1003
#define IDC_BREAK_BUTTON 1004
#define IDC_STEP_BUTTON 1005
#define IDC_SKIP_BUTTON 1006
#define IDC_BP_BUTTON 1007
#define IDC_R4300I_REGISTERS_BUTTON 1008
#define IDC_R4300I_DEBUGGER_BUTTON 1009
#define IDC_RSP_REGISTERS_BUTTON 1010
#define IDC_MEMORY_BUTTON 1011
#define IDC_SCRL_BAR 1012

void Paint_RSP_Commands(HWND hDlg);
void RSP_Commands_Setup(HWND hDlg);
LRESULT CALLBACK RSP_Commands_Proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct
{
    DWORD Location;
    DWORD opcode;
    char String[150];
    DWORD status;
} RSPCOMMANDLINE;

RSPCOMMANDLINE RSPCommandLine[30];
HWND RSPCommandshWnd, hList, hAddress, hFunctionlist, hGoButton, hBreakButton,
    hStepButton, hSkipButton, hBPButton, hR4300iRegisters, hR4300iDebugger, hRSPRegisters,
    hMemory, hScrlBar;
bool InRSPCommandsWindow;
char CommandName[100];
bool Stepping_Commands, WaitingForStep;

void Create_RSP_Commands_Window(int Child)
{
    DWORD ThreadID;

    if (Child)
    {
        InRSPCommandsWindow = true;
        DialogBoxA((HINSTANCE)hinstDLL, "RSPCOMMAND", NULL, (DLGPROC)RSP_Commands_Proc);

        InRSPCommandsWindow = false;
        memset(RSPCommandLine, 0, sizeof(RSPCommandLine));
        SetRSPCommandToRunning();
    }
    else
    {
        Stepping_Commands = true;
        if (!InRSPCommandsWindow)
        {
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Create_RSP_Commands_Window,
                         (LPVOID) true, 0, &ThreadID);
        }
        else
        {
            if (IsIconic((HWND)RSPCommandshWnd))
            {
                SendMessage(RSPCommandshWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            }
            SetForegroundWindow(RSPCommandshWnd);
        }
    }
}

void Disable_RSP_Commands_Window(void)
{
    SCROLLINFO si;

    if (!InRSPCommandsWindow)
    {
        return;
    }
    EnableWindow(hList, false);
    EnableWindow(hAddress, false);
    EnableWindow(hScrlBar, false);
    EnableWindow(hGoButton, false);
    EnableWindow(hStepButton, false);
    EnableWindow(hSkipButton, false);
    EnableWindow(hR4300iRegisters, false);
    EnableWindow(hRSPRegisters, false);
    EnableWindow(hR4300iDebugger, false);
    EnableWindow(hMemory, false);

    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
    si.nMin = 0;
    si.nMax = 0;
    si.nPos = 1;
    si.nPage = 1;
    SetScrollInfo(hScrlBar, SB_CTL, &si, true);
}

int DisplayRSPCommand(DWORD location, int InsertPos)
{
    uint32_t OpCode;
    DWORD LinesUsed = 1, status;
    bool Redraw = false;

    RSP_LW_IMEM(location, &OpCode);

    status = 0;
    if (location == *PrgCount)
    {
        status = RSP_Status_PC;
    }
    if (CheckForRSPBPoint(location))
    {
        status |= RSP_Status_BP;
    }
    if (RSPCommandLine[InsertPos].opcode != OpCode)
    {
        Redraw = true;
    }
    if (RSPCommandLine[InsertPos].Location != location)
    {
        Redraw = true;
    }
    if (RSPCommandLine[InsertPos].status != status)
    {
        Redraw = true;
    }
    if (Redraw)
    {
        RSPInstruction Instruction(0x1000 | location, OpCode);

        RSPCommandLine[InsertPos].Location = location;
        RSPCommandLine[InsertPos].status = status;
        RSPCommandLine[InsertPos].opcode = OpCode;
        sprintf(RSPCommandLine[InsertPos].String, " 0x%04X\t%s\t%s", 0x1000 | location, Instruction.Name(), Instruction.Param());
        if (SendMessage(hList, LB_GETCOUNT, 0, 0) <= InsertPos)
        {
            SendMessage(hList, LB_INSERTSTRING, (WPARAM)InsertPos, (LPARAM)location);
        }
        else
        {
            RECT ItemRC;
            SendMessage(hList, LB_GETITEMRECT, (WPARAM)InsertPos, (LPARAM)&ItemRC);
            RedrawWindow(hList, &ItemRC, NULL, RDW_INVALIDATE);
        }
    }
    return LinesUsed;
}

void DumpRSPCode(void)
{
    char string[100], LogFileName[255], *p;
    uint32_t OpCode;
    DWORD location, dwWritten;
    HANDLE hLogFile = NULL;

    strcpy(LogFileName, GetCommandLineA() + 1);

    if (strchr(LogFileName, '\"'))
    {
        p = strchr(LogFileName, '\"');
        *p = '\0';
    }

    if (strchr(LogFileName, '\\'))
    {
        p = LogFileName;
        while (strchr(p, '\\'))
        {
            p = strchr(p, '\\');
            p++;
        }
        p -= 1;
        *p = '\0';
    }

    strcat(LogFileName, "\\RSP code.txt");

    hLogFile = CreateFileA(LogFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    SetFilePointer(hLogFile, 0, NULL, FILE_BEGIN);

    for (location = 0; location < 0x1000; location += 4)
    {
        unsigned int characters_to_write;
        int characters_converted;

        RSP_LW_IMEM(location, &OpCode);
        characters_converted = sprintf(
            &string[0],
            " 0x%03X\t%s\r\n",
            location,
            RSPInstruction(location, OpCode).NameAndParam().c_str());

        if (characters_converted < 0)
        {
            DisplayError("Failed to sprintf IMEM from 0x%03X.", location);
            break;
        }
        characters_to_write = (unsigned)characters_converted;
        WriteFile(hLogFile, string, characters_to_write, &dwWritten, NULL);
    }
    CloseHandle(hLogFile);
}

void DumpRSPData(void)
{
    char string[100], LogFileName[255], *p;
    DWORD location, dwWritten;
    HANDLE hLogFile = NULL;

    strcpy(LogFileName, GetCommandLineA() + 1);

    if (strchr(LogFileName, '\"'))
    {
        p = strchr(LogFileName, '\"');
        *p = '\0';
    }

    if (strchr(LogFileName, '\\'))
    {
        p = LogFileName;
        while (strchr(p, '\\'))
        {
            p = strchr(p, '\\');
            p++;
        }
        p -= 1;
        *p = '\0';
    }

    strcat(LogFileName, "\\RSP data.txt");

    hLogFile = CreateFileA(LogFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    SetFilePointer(hLogFile, 0, NULL, FILE_BEGIN);

    for (location = 0; location < 0x1000; location += 4)
    {
        unsigned int characters_to_write;
        int characters_converted;

        uint32_t Value = *(uint32_t *)(RSPInfo.DMEM + location);
        characters_converted = sprintf(
            &string[0],
            " 0x%03X\t0x%08X\r\n",
            location,
            Value);

        if (characters_converted < 0)
        {
            DisplayError("Failed to sprintf DMEM from 0x%03X.", location);
            break;
        }
        characters_to_write = (unsigned)characters_converted;
        WriteFile(hLogFile, string, characters_to_write, &dwWritten, NULL);
    }
    CloseHandle(hLogFile);
}

void DrawRSPCommand(LPARAM lParam)
{
    char Command[150], Offset[30], Instruction[30], Arguments[40];
    int printed_offset, printed_instruction, printed_arguments;
    LPDRAWITEMSTRUCT ditem;
    COLORREF oldColor = {0};
    int ResetColor;
    HBRUSH hBrush;
    RECT TextRect;
    char *p1, *p2;

    ditem = (LPDRAWITEMSTRUCT)lParam;
    strcpy(Command, RSPCommandLine[ditem->itemID].String);

    if (strchr(Command, '\t'))
    {
        p1 = strchr(Command, '\t');
        printed_offset = sprintf(Offset, "%.*s", p1 - Command, Command);
        p1++;
        if (strchr(p1, '\t'))
        {
            p2 = strchr(p1, '\t');
            printed_instruction = sprintf(Instruction, "%.*s", p2 - p1, p1);
            printed_arguments = sprintf(Arguments, "%s", p2 + 1);
        }
        else
        {
            printed_instruction = sprintf(Instruction, "%s", p1);
            printed_arguments = sprintf(Arguments, "\0");
        }
        Command[0] = '\0';
    }
    else
    {
        printed_offset = sprintf(Offset, "\0");
        printed_instruction = sprintf(Instruction, "\0");
        printed_arguments = sprintf(Arguments, "\0");
    }

    if (printed_offset < 0 || printed_instruction < 0 || printed_arguments < 0)
    {
        DisplayError("Failed to sprintf from item %u.", ditem->itemID);
    }

    if (*PrgCount == RSPCommandLine[ditem->itemID].Location)
    {
        ResetColor = true;
        hBrush = (HBRUSH)(COLOR_HIGHLIGHT + 1);
        oldColor = SetTextColor(ditem->hDC, RGB(255, 255, 255));
    }
    else
    {
        ResetColor = false;
        hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    }

    if (CheckForRSPBPoint(RSPCommandLine[ditem->itemID].Location))
    {
        ResetColor = true;
        if (*PrgCount == RSPCommandLine[ditem->itemID].Location)
        {
            SetTextColor(ditem->hDC, RGB(255, 0, 0));
        }
        else
        {
            oldColor = SetTextColor(ditem->hDC, RGB(255, 0, 0));
        }
    }

    FillRect(ditem->hDC, &ditem->rcItem, hBrush);
    SetBkMode(ditem->hDC, TRANSPARENT);

    if (Command[0] == '\0')
    {
        SetRect(&TextRect, ditem->rcItem.left, ditem->rcItem.top, ditem->rcItem.left + 83,
                ditem->rcItem.bottom);
        DrawTextA(
            ditem->hDC,
            &Offset[0], printed_offset,
            &TextRect,
            DT_SINGLELINE | DT_VCENTER);

        SetRect(&TextRect, ditem->rcItem.left + 83, ditem->rcItem.top, ditem->rcItem.left + 165,
                ditem->rcItem.bottom);
        DrawTextA(
            ditem->hDC,
            &Instruction[0], printed_instruction,
            &TextRect,
            DT_SINGLELINE | DT_VCENTER);

        SetRect(&TextRect, ditem->rcItem.left + 165, ditem->rcItem.top, ditem->rcItem.right,
                ditem->rcItem.bottom);
        DrawTextA(
            ditem->hDC,
            &Arguments[0], printed_arguments,
            &TextRect,
            DT_SINGLELINE | DT_VCENTER);
    }
    else
    {
        DrawTextA(
            ditem->hDC,
            &Command[0], (signed int)strlen(Command),
            &ditem->rcItem,
            DT_SINGLELINE | DT_VCENTER);
    }

    if (ResetColor != 0)
    {
        SetTextColor(ditem->hDC, oldColor);
    }
}

void Enable_RSP_Commands_Window(void)
{
    SCROLLINFO si;

    if (!InRSPCommandsWindow)
    {
        return;
    }
    EnableWindow(hList, true);
    EnableWindow(hAddress, true);
    EnableWindow(hScrlBar, true);
    EnableWindow(hGoButton, true);
    EnableWindow(hStepButton, true);
    EnableWindow(hSkipButton, false);
    EnableWindow(hR4300iRegisters, true);
    EnableWindow(hRSPRegisters, true);
    EnableWindow(hR4300iDebugger, true);
    EnableWindow(hMemory, true);
    SendMessage(hBPButton, BM_SETSTYLE, BS_PUSHBUTTON, true);
    SendMessage(hStepButton, BM_SETSTYLE, BS_DEFPUSHBUTTON, true);
    SendMessage(RSPCommandshWnd, DM_SETDEFID, IDC_STEP_BUTTON, 0);

    if (Stepping_Commands)
    {
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
        si.nMin = 0;
        si.nMax = (0x1000 >> 2) - 1;
        si.nPos = (*PrgCount >> 2);
        si.nPage = 30;
        SetScrollInfo(hScrlBar, SB_CTL, &si, true);

        SetRSPCommandViewto(*PrgCount);
        SetForegroundWindow(RSPCommandshWnd);
    }
}

void Enter_RSP_Commands_Window(void)
{
    Create_RSP_Commands_Window(false);
}

void Paint_RSP_Commands(HWND hDlg)
{
    PAINTSTRUCT ps;
    RECT rcBox;
    HFONT hOldFont;
    int OldBkMode;

    BeginPaint(hDlg, &ps);

    rcBox.left = 5;
    rcBox.top = 5;
    rcBox.right = 343;
    rcBox.bottom = 463;
    DrawEdge(ps.hdc, &rcBox, EDGE_RAISED, BF_RECT);

    rcBox.left = 8;
    rcBox.top = 8;
    rcBox.right = 340;
    rcBox.bottom = 460;
    DrawEdge(ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT);

    rcBox.left = 347;
    rcBox.top = 7;
    rcBox.right = 446;
    rcBox.bottom = 42;
    DrawEdge(ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT);

    rcBox.left = 352;
    rcBox.top = 2;
    rcBox.right = 400;
    rcBox.bottom = 15;
    FillRect(ps.hdc, &rcBox, (HBRUSH)COLOR_WINDOW);

    rcBox.left = 14;
    rcBox.top = 14;
    rcBox.right = 88;
    rcBox.bottom = 32;
    DrawEdge(ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT);

    rcBox.left = 86;
    rcBox.top = 14;
    rcBox.right = 173;
    rcBox.bottom = 32;
    DrawEdge(ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT);

    rcBox.left = 171;
    rcBox.top = 14;
    rcBox.right = 320;
    rcBox.bottom = 32;
    DrawEdge(ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT);

    hOldFont = (HFONT)SelectObject(ps.hdc, GetStockObject(DEFAULT_GUI_FONT));
    OldBkMode = SetBkMode(ps.hdc, TRANSPARENT);

    TextOutA(ps.hdc, 23, 16, "Offset", 6);
    TextOutA(ps.hdc, 97, 16, "Instruction", 11);
    TextOutA(ps.hdc, 180, 16, "Arguments", 9);
    TextOutA(ps.hdc, 354, 2, " Address ", 9);
    TextOutA(ps.hdc, 358, 19, "0x1", 3);

    SelectObject(ps.hdc, hOldFont);
    SetBkMode(ps.hdc, OldBkMode);

    EndPaint(hDlg, &ps);
}

void RefreshRSPCommands(void)
{
    DWORD location, LinesUsed;
    char AsciiAddress[20];
    int count;

    if (InRSPCommandsWindow == false)
    {
        return;
    }

    GetWindowTextA(hAddress, AsciiAddress, sizeof(AsciiAddress));
    location = AsciiToHex(AsciiAddress) & ~3;

    if (location > 0xF88)
    {
        location = 0xF88;
    }
    for (count = 0; count < RSP_MaxCommandLines; count += LinesUsed)
    {
        LinesUsed = DisplayRSPCommand(location, count);
        location += 4;
    }
}

LRESULT CALLBACK RSP_Commands_Proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        RSPCommandshWnd = hDlg;
        RSP_Commands_Setup(hDlg);
        break;
    case WM_MOVE:
        //StoreCurrentWinPos("RSP Commands",hDlg);
        break;
    case WM_DRAWITEM:
        if (wParam == IDC_LIST)
        {
            DrawRSPCommand(lParam);
        }
        break;
    case WM_PAINT:
        Paint_RSP_Commands(hDlg);
        RedrawWindow(hScrlBar, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
        return true;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_LIST:
            if (HIWORD(wParam) == LBN_DBLCLK)
            {
                LRESULT Selected;
                DWORD Location;

                Selected = SendMessage(hList, LB_GETCURSEL, 0, 0);
                Location = RSPCommandLine[Selected].Location;
                if (Location != (DWORD)-1)
                {
                    if (CheckForRSPBPoint(Location))
                    {
                        RemoveRSPBreakPoint(Location);
                    }
                    else
                    {
                        AddRSP_BPoint(Location, false);
                    }
                    RefreshRSPCommands();
                }
            }
            break;
        case IDC_ADDRESS:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                RefreshRSPCommands();
            }
            break;
        case IDC_GO_BUTTON:
            SetRSPCommandToRunning();
            break;
        case IDC_BREAK_BUTTON:
            SetRSPCommandToStepping();
            break;
        case IDC_STEP_BUTTON:
            WaitingForStep = false;
            break;
        /*case IDC_SKIP_BUTTON:
			SkipNextRSPOpCode = true;
			WaitingFor_RSPStep   = false;
			break;*/
        case IDC_BP_BUTTON:
            if (DebugInfo.Enter_BPoint_Window != NULL)
            {
                DebugInfo.Enter_BPoint_Window();
            }
            break;
        case IDC_RSP_REGISTERS_BUTTON:
            Enter_RSP_Register_Window();
            break;
        case IDC_R4300I_DEBUGGER_BUTTON:
            if (DebugInfo.Enter_R4300i_Commands_Window != NULL)
            {
                DebugInfo.Enter_R4300i_Commands_Window();
            }
            break;
        case IDC_R4300I_REGISTERS_BUTTON:
            if (DebugInfo.Enter_R4300i_Register_Window != NULL)
            {
                DebugInfo.Enter_R4300i_Register_Window();
            }
            break;
        case IDC_MEMORY_BUTTON:
            if (DebugInfo.Enter_Memory_Window != NULL)
            {
                DebugInfo.Enter_Memory_Window();
            }
            break;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        break;
    case WM_VSCROLL:
        if ((HWND)lParam == hScrlBar)
        {
            DWORD location;
            char Value[20];
            SCROLLINFO si;

            GetWindowTextA(hAddress, Value, sizeof(Value));
            location = AsciiToHex(Value) & ~3;

            switch (LOWORD(wParam))
            {
            case SB_THUMBTRACK:
                sprintf(Value, "%03X", ((short int)HIWORD(wParam) << 2));
                SetWindowTextA(hAddress, Value);
                si.cbSize = sizeof(si);
                si.fMask = SIF_POS;
                si.nPos = (short int)HIWORD(wParam);
                SetScrollInfo(hScrlBar, SB_CTL, &si, true);
                break;
            case SB_LINEDOWN:
                if (location < 0xF88)
                {
                    sprintf(Value, "%03X", location + 0x4);
                    SetWindowTextA(hAddress, Value);
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_POS;
                    si.nPos = ((location + 0x4) >> 2);
                    SetScrollInfo(hScrlBar, SB_CTL, &si, true);
                }
                else
                {
                    sprintf(Value, "%03X", 0xF88);
                    SetWindowTextA(hAddress, Value);
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_POS;
                    si.nPos = (0xFFC >> 2);
                    SetScrollInfo(hScrlBar, SB_CTL, &si, true);
                }
                break;
            case SB_LINEUP:
                if (location > 0x4)
                {
                    sprintf(Value, "%03X", location - 0x4);
                    SetWindowTextA(hAddress, Value);
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_POS;
                    si.nPos = ((location - 0x4) >> 2);
                    SetScrollInfo(hScrlBar, SB_CTL, &si, true);
                }
                else
                {
                    sprintf(Value, "%03X", 0);
                    SetWindowTextA(hAddress, Value);
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_POS;
                    si.nPos = 0;
                    SetScrollInfo(hScrlBar, SB_CTL, &si, true);
                }
                break;
            case SB_PAGEDOWN:
                if ((location + 0x74) < 0xF88)
                {
                    sprintf(Value, "%03X", location + 0x74);
                    SetWindowTextA(hAddress, Value);
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_POS;
                    si.nPos = ((location + 0x74) >> 2);
                    SetScrollInfo(hScrlBar, SB_CTL, &si, true);
                }
                else
                {
                    sprintf(Value, "%03X", 0xF88);
                    SetWindowTextA(hAddress, Value);
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_POS;
                    si.nPos = (0xF8F >> 2);
                    SetScrollInfo(hScrlBar, SB_CTL, &si, true);
                }
                break;
            case SB_PAGEUP:
                if ((location - 0x74) > 0x74)
                {
                    sprintf(Value, "%03X", location - 0x74);
                    SetWindowTextA(hAddress, Value);
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_POS;
                    si.nPos = ((location - 0x74) >> 2);
                    SetScrollInfo(hScrlBar, SB_CTL, &si, true);
                }
                else
                {
                    sprintf(Value, "%03X", 0);
                    SetWindowTextA(hAddress, Value);
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_POS;
                    si.nPos = 0;
                    SetScrollInfo(hScrlBar, SB_CTL, &si, true);
                }
                break;
            }
        }
        break;
    default:
        return false;
    }
    return true;
}

void RSP_Commands_Setup(HWND hDlg)
{
#define WindowWidth 457
#define WindowHeight 494
    char Location[10];
    DWORD X, Y, WndPos;

    hList = CreateWindowExA(WS_EX_STATICEDGE, "LISTBOX", "", WS_CHILD | WS_VISIBLE | LBS_OWNERDRAWFIXED | LBS_NOTIFY, 14, 30, 303, 445, hDlg,
                            (HMENU)IDC_LIST, (HINSTANCE)hinstDLL, NULL);
    if (hList)
    {
        SendMessage(hList, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        SendMessage(hList, LB_SETITEMHEIGHT, (WPARAM)0, (LPARAM)MAKELPARAM(14, 0));
    }

    sprintf(Location, "%03X", PrgCount ? *PrgCount : 0);
    hAddress = CreateWindowExA(0, "EDIT", Location, WS_CHILD | ES_UPPERCASE | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 375, 17, 36, 18, hDlg, (HMENU)IDC_ADDRESS, (HINSTANCE)hinstDLL, NULL);
    if (hAddress)
    {
        SendMessage(hAddress, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        SendMessage(hAddress, EM_SETLIMITTEXT, (WPARAM)3, (LPARAM)0);
    }

    hFunctionlist = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_SORT | WS_TABSTOP, 352, 56, 89, 150, hDlg,
                                    (HMENU)IDC_FUNCTION_COMBO, (HINSTANCE)hinstDLL, NULL);
    if (hFunctionlist)
    {
        SendMessage(hFunctionlist, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
    }

    hGoButton = CreateWindowExA(WS_EX_STATICEDGE, "BUTTON", "&Go", WS_CHILD | BS_DEFPUSHBUTTON | WS_VISIBLE | WS_TABSTOP, 347, 56, 100, 24, hDlg, (HMENU)IDC_GO_BUTTON,
                                (HINSTANCE)hinstDLL, NULL);
    if (hGoButton)
    {
        SendMessage(hGoButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
    }

    hBreakButton = CreateWindowExA(WS_EX_STATICEDGE, "BUTTON", "&Break", WS_DISABLED | WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347, 85, 100, 24, hDlg,
                                   (HMENU)IDC_BREAK_BUTTON, (HINSTANCE)hinstDLL, NULL);
    if (hBreakButton)
    {
        SendMessage(hBreakButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
    }

    hStepButton = CreateWindowExA(WS_EX_STATICEDGE, "BUTTON", "&Step", WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347, 114, 100, 24, hDlg,
                                  (HMENU)IDC_STEP_BUTTON, (HINSTANCE)hinstDLL, NULL);
    if (hStepButton)
    {
        SendMessage(hStepButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
    }

    hSkipButton = CreateWindowExA(WS_EX_STATICEDGE, "BUTTON", "&Skip", WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347, 143, 100, 24, hDlg,
                                  (HMENU)IDC_SKIP_BUTTON, (HINSTANCE)hinstDLL, NULL);
    if (hSkipButton)
    {
        SendMessage(hSkipButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
    }

    WndPos = 324;
    if (DebugInfo.Enter_BPoint_Window == NULL)
    {
        WndPos += 29;
    }
    if (DebugInfo.Enter_R4300i_Commands_Window == NULL)
    {
        WndPos += 29;
    }
    if (DebugInfo.Enter_R4300i_Register_Window == NULL)
    {
        WndPos += 29;
    }
    if (DebugInfo.Enter_Memory_Window == NULL)
    {
        WndPos += 29;
    }

    if (DebugInfo.Enter_BPoint_Window != NULL)
    {
        hBPButton = CreateWindowExA(WS_EX_STATICEDGE, "BUTTON", "&Break Points", WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347, WndPos, 100, 24, hDlg,
                                    (HMENU)IDC_BP_BUTTON, (HINSTANCE)hinstDLL, NULL);
        if (hBPButton)
        {
            SendMessage(hBPButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        }
    }

    WndPos += 29;
    hRSPRegisters = CreateWindowExA(WS_EX_STATICEDGE, "BUTTON", "RSP &Registers...",
                                    WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347, WndPos, 100, 24, hDlg,
                                    (HMENU)IDC_RSP_REGISTERS_BUTTON, (HINSTANCE)hinstDLL, NULL);
    if (hRSPRegisters)
    {
        SendMessage(hRSPRegisters, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
    }

    WndPos += 29;
    if (DebugInfo.Enter_R4300i_Commands_Window != NULL)
    {
        hR4300iDebugger = CreateWindowExA(WS_EX_STATICEDGE, "BUTTON", "R4300i &Debugger...",
                                          WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347, WndPos, 100, 24, hDlg,
                                          (HMENU)IDC_R4300I_DEBUGGER_BUTTON, (HINSTANCE)hinstDLL, NULL);
        if (hR4300iDebugger)
        {
            SendMessage(hR4300iDebugger, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        }
    }

    WndPos += 29;
    if (DebugInfo.Enter_R4300i_Register_Window != NULL)
    {
        hR4300iRegisters = CreateWindowExA(WS_EX_STATICEDGE, "BUTTON", "R4300i R&egisters...",
                                           WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347, WndPos, 100, 24, hDlg,
                                           (HMENU)IDC_R4300I_REGISTERS_BUTTON, (HINSTANCE)hinstDLL, NULL);
        if (hR4300iRegisters)
        {
            SendMessage(hR4300iRegisters, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        }
    }

    WndPos += 29;
    if (DebugInfo.Enter_Memory_Window != NULL)
    {
        hMemory = CreateWindowExA(WS_EX_STATICEDGE, "BUTTON", "&Memory...", WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347, WndPos, 100, 24, hDlg,
                                  (HMENU)IDC_MEMORY_BUTTON, (HINSTANCE)hinstDLL, NULL);
        if (hMemory)
        {
            SendMessage(hMemory, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        }
    }

    hScrlBar = CreateWindowExA(WS_EX_STATICEDGE, "SCROLLBAR", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | SBS_VERT, 318, 14, 18, 439, hDlg, (HMENU)IDC_SCRL_BAR, (HINSTANCE)hinstDLL, NULL);

    if (RSP_Running)
    {
        Enable_RSP_Commands_Window();
    }
    else
    {
        Disable_RSP_Commands_Window();
    }

    //if ( !GetStoredWinPos("RSP Commands", &X, &Y ) ) {
    X = (GetSystemMetrics(SM_CXSCREEN) - WindowWidth) / 2;
    Y = (GetSystemMetrics(SM_CYSCREEN) - WindowHeight) / 2;
    //}
    SetWindowTextA(hDlg, "RSP Commands");

    SetWindowPos(hDlg, NULL, X, Y, WindowWidth, WindowHeight, SWP_NOZORDER | SWP_SHOWWINDOW);
}

void SetRSPCommandToRunning(void)
{
    Stepping_Commands = false;
    if (InRSPCommandsWindow == false)
    {
        return;
    }
    EnableWindow(hGoButton, false);
    EnableWindow(hBreakButton, true);
    EnableWindow(hStepButton, false);
    EnableWindow(hSkipButton, false);
    SendMessage(RSPCommandshWnd, DM_SETDEFID, IDC_BREAK_BUTTON, 0);
    SendMessage(hGoButton, BM_SETSTYLE, BS_PUSHBUTTON, true);
    SendMessage(hBreakButton, BM_SETSTYLE, BS_DEFPUSHBUTTON, true);
    SetFocus(hBreakButton);
}

void SetRSPCommandToStepping(void)
{
    if (InRSPCommandsWindow == false)
    {
        return;
    }
    EnableWindow(hGoButton, true);
    EnableWindow(hBreakButton, false);
    EnableWindow(hStepButton, true);
    EnableWindow(hSkipButton, true);
    SendMessage(hBreakButton, BM_SETSTYLE, BS_PUSHBUTTON, true);
    SendMessage(hStepButton, BM_SETSTYLE, BS_DEFPUSHBUTTON, true);
    SendMessage(RSPCommandshWnd, DM_SETDEFID, IDC_STEP_BUTTON, 0);
    SetFocus(hStepButton);
    Stepping_Commands = true;
}

void SetRSPCommandViewto(UINT NewLocation)
{
    unsigned int location;
    char Value[20];

    if (InRSPCommandsWindow == false)
    {
        return;
    }

    GetWindowTextA(hAddress, Value, sizeof(Value));
    location = AsciiToHex(Value) & ~3;

    if (NewLocation < location || NewLocation >= location + 120)
    {
        sprintf(Value, "%03X", NewLocation);
        SetWindowTextA(hAddress, Value);
    }
    else
    {
        RefreshRSPCommands();
    }
}
