/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/

#include "stdafx.h"
#include "DebuggerUI.h"

CDebugExcBreakpoints::ExcCheckboxMeta CDebugExcBreakpoints::ExcCheckboxMap[] = {
    { IDC_CHK_INT,      0 },
    { IDC_CHK_MOD,      1 },
    { IDC_CHK_RMISS,    2 },
    { IDC_CHK_WMISS,    3 },
    { IDC_CHK_RADE,     4 },
    { IDC_CHK_WADE,     5 },
    { IDC_CHK_IBE,      6 },
    { IDC_CHK_DBE,      7 },
    { IDC_CHK_SYSCALL,  8 },
    { IDC_CHK_BREAK,    9 },
    { IDC_CHK_II,      10 },
    { IDC_CHK_CPU,     11 },
    { IDC_CHK_OV,      12 },
    { IDC_CHK_TRAP,    13 },
    { IDC_CHK_VCEI,    14 },
    { IDC_CHK_FPE,     15 },
    { IDC_CHK_WATCH,   23 },
    { IDC_CHK_VCED,    31 },
    { 0, 0 }
};

CDebugExcBreakpoints::ExcCheckboxMeta CDebugExcBreakpoints::FpExcCheckboxMap[] = {
    { IDC_CHK_FP_CI, (1 << 0) },
    { IDC_CHK_FP_CU, (1 << 1) },
    { IDC_CHK_FP_CO, (1 << 2) },
    { IDC_CHK_FP_CZ, (1 << 3) },
    { IDC_CHK_FP_CV, (1 << 4) },
    { IDC_CHK_FP_CE, (1 << 5) },
    { 0, 0 }
};

CDebugExcBreakpoints::ExcCheckboxMeta CDebugExcBreakpoints::IntrCheckboxMap[] = {
    { IDC_CHK_INTR_IP0, (1 << 0) },
    { IDC_CHK_INTR_IP1, (1 << 1) },
    { IDC_CHK_INTR_IP2, (1 << 2) },
    { IDC_CHK_INTR_IP3, (1 << 3) },
    { IDC_CHK_INTR_IP4, (1 << 4) },
    { IDC_CHK_INTR_IP5, (1 << 5) },
    { IDC_CHK_INTR_IP6, (1 << 6) },
    { IDC_CHK_INTR_IP7, (1 << 7) },
    { 0, 0 }
};

CDebugExcBreakpoints::ExcCheckboxMeta CDebugExcBreakpoints::RcpIntrCheckboxMap[] = {
    { IDC_CHK_INTR_SP, (1 << 0) },
    { IDC_CHK_INTR_SI, (1 << 1) },
    { IDC_CHK_INTR_AI, (1 << 2) },
    { IDC_CHK_INTR_VI, (1 << 3) },
    { IDC_CHK_INTR_PI, (1 << 4) },
    { IDC_CHK_INTR_DP, (1 << 5) },
    { 0, 0 }
};

CDebugExcBreakpoints::CDebugExcBreakpoints(CDebuggerUI* debugger) :
    CDebugDialog<CDebugExcBreakpoints>(debugger)
{
}

CDebugExcBreakpoints::~CDebugExcBreakpoints()
{
}

LRESULT CDebugExcBreakpoints::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    DlgSavePos_Init(DebuggerUI_ExceptionBPPos);

    InitCheckboxes(ExcCheckboxMap, Debugger_ExceptionBreakpoints, true);
    InitCheckboxes(FpExcCheckboxMap, Debugger_FpExceptionBreakpoints);
    InitCheckboxes(IntrCheckboxMap, Debugger_IntrBreakpoints);
    InitCheckboxes(RcpIntrCheckboxMap, Debugger_RcpIntrBreakpoints);

    bool intrEnabled = g_Settings->LoadDword(Debugger_ExceptionBreakpoints) & 0x01;
    bool rcpIntrEnabled = g_Settings->LoadDword(Debugger_IntrBreakpoints) & 0x04;
    bool fpExcEnabled = g_Settings->LoadDword(Debugger_ExceptionBreakpoints) & (1 << 15);

    EnableCheckboxes(IntrCheckboxMap, intrEnabled);
    EnableCheckboxes(RcpIntrCheckboxMap, intrEnabled && rcpIntrEnabled);
    EnableCheckboxes(FpExcCheckboxMap, fpExcEnabled);

    LoadWindowPos();
    WindowCreated();
    return TRUE;
}

LRESULT CDebugExcBreakpoints::OnDestroy(void)
{
    return 0;
}

LRESULT CDebugExcBreakpoints::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDOK:
    case IDCANCEL:
        EndDialog(0);
        return FALSE;
    }

    bool bChecked = (SendMessage(GetDlgItem(wID), BM_GETSTATE, 0, 0) & BST_CHECKED) != 0;

    if (wID == IDC_CHK_INT)
    {
        EnableCheckboxes(IntrCheckboxMap, bChecked);
        bool toggleRcpIntr = bChecked && (g_Settings->LoadDword(Debugger_IntrBreakpoints) & 0x04);
        EnableCheckboxes(RcpIntrCheckboxMap, toggleRcpIntr);
    }
    
    if (wID == IDC_CHK_FPE)
    {
        EnableCheckboxes(FpExcCheckboxMap, bChecked);
    }

    if (wID == IDC_CHK_INTR_IP2)
    {
        EnableCheckboxes(RcpIntrCheckboxMap, bChecked);
    }

    UpdateBpSetting(ExcCheckboxMap, Debugger_ExceptionBreakpoints, wID, bChecked, true);
    UpdateBpSetting(FpExcCheckboxMap, Debugger_FpExceptionBreakpoints, wID, bChecked);
    UpdateBpSetting(IntrCheckboxMap, Debugger_IntrBreakpoints, wID, bChecked);
    UpdateBpSetting(RcpIntrCheckboxMap, Debugger_RcpIntrBreakpoints, wID, bChecked);

    return FALSE;
}

void CDebugExcBreakpoints::OnExitSizeMove(void)
{
    SaveWindowPos(0);
}

void CDebugExcBreakpoints::InitCheckboxes(ExcCheckboxMeta* checkboxMap, SettingID settingID, bool bShift)
{
    uint32_t excBits = g_Settings->LoadDword(settingID);

    for (int i = 0; checkboxMap[i].ctrlId != 0; i++)
    {
        uint32_t excBit = bShift ? (1 << checkboxMap[i].exc) : checkboxMap[i].exc;

        if (excBits & excBit)
        {
            SendDlgItemMessage(checkboxMap[i].ctrlId, BM_SETCHECK, BST_CHECKED, 0);
        }
    }
}

void CDebugExcBreakpoints::UpdateBpSetting(ExcCheckboxMeta* checkboxMap, SettingID settingID, WORD wID, bool bChecked, bool bShift)
{
    for (int i = 0; checkboxMap[i].ctrlId != 0; i++)
    {
        if (checkboxMap[i].ctrlId == wID)
        {
            uint32_t excBit = bShift ? (1 << checkboxMap[i].exc) : checkboxMap[i].exc;
            uint32_t bits = g_Settings->LoadDword(settingID);

            if (bChecked)
            {
                bits |= excBit;
            }
            else
            {
                bits &= ~excBit;
            }

            g_Settings->SaveDword(settingID, bits);
            return;
        }
    }
}

void CDebugExcBreakpoints::EnableCheckboxes(ExcCheckboxMeta* checkboxMap, bool bEnable)
{
    for (int i = 0; checkboxMap[i].ctrlId != 0; i++)
    {
        ::EnableWindow(GetDlgItem(checkboxMap[i].ctrlId), bEnable);
    }
}