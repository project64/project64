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

    uint32_t excBreakpoints = g_Settings->LoadDword(Debugger_ExceptionBreakpoints);

    for (int i = 0; ExcCheckboxMap[i].ctrlId != 0; i++)
    {
        uint32_t excBit = (1 << ExcCheckboxMap[i].exc);

        if (excBreakpoints & excBit)
        {
            SendDlgItemMessage(ExcCheckboxMap[i].ctrlId, BM_SETCHECK, BST_CHECKED, 0);
        }
    }

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

    for (int i = 0; ExcCheckboxMap[i].ctrlId != 0; i++)
    {
        if (ExcCheckboxMap[i].ctrlId == wID)
        {
            uint32_t excBit = (1 << ExcCheckboxMap[i].exc);
            bool bChecked = (SendMessage(GetDlgItem(wID), BM_GETSTATE, 0, 0) & BST_CHECKED) != 0;
            uint32_t excBreakpoints = g_Settings->LoadDword(Debugger_ExceptionBreakpoints);

            if (bChecked)
            {
                excBreakpoints |= excBit;
            }
            else
            {
                excBreakpoints &= ~excBit;
            }

            g_Settings->SaveDword(Debugger_ExceptionBreakpoints, excBreakpoints);
            break;
        }
    }

    return FALSE;
}

void CDebugExcBreakpoints::OnExitSizeMove(void)
{
    SaveWindowPos();
}
