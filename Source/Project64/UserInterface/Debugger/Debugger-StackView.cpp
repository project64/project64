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
#include "Symbols.h"

CDebugStackView::CDebugStackView(CDebuggerUI * debugger) :
    CDebugDialog<CDebugStackView>(debugger)
{
}

CDebugStackView::~CDebugStackView(void)
{
}

LRESULT CDebugStackView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    DlgResize_Init(false, true);

    CRect m_DefaultWindowRect;
    GetWindowRect(&m_DefaultWindowRect);

    //We find the middle position of the screen, we use this if theres no setting
    int32_t X = GetX(m_DefaultWindowRect);
    int32_t	Y = GetY(m_DefaultWindowRect);

    //Load the value from settings, if none is available, default to above
    UISettingsLoadDword(StackView_Top, (uint32_t &)Y);
    UISettingsLoadDword(StackView_Left, (uint32_t &)X);

    SetPos(X, Y);

    int32_t Width = UISettingsLoadDword(StackView_Width);
    int32_t Height = UISettingsLoadDword(StackView_Height);

    SetSize(Width, Height);

    m_StackList.Attach(GetDlgItem(IDC_STACK_LIST));
    m_StackList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_StackList.AddColumn("#", 0);
    m_StackList.AddColumn("00", 1);
    m_StackList.AddColumn("04", 2);
    m_StackList.AddColumn("08", 3);
    m_StackList.AddColumn("0C", 4);

    m_StackList.SetColumnWidth(0, 22);
    m_StackList.SetColumnWidth(1, 64);
    m_StackList.SetColumnWidth(2, 64);
    m_StackList.SetColumnWidth(3, 64);
    m_StackList.SetColumnWidth(4, 64);

    m_SPStatic.Attach(GetDlgItem(IDC_SP_STATIC));

    WindowCreated();

    return 0;
}

LRESULT CDebugStackView::OnDestroy(void)
{
    m_StackList.Detach();
    m_SPStatic.Detach();
    return 0;
}

LRESULT CDebugStackView::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDC_MEM_BTN:
        if (g_Reg != NULL)
        {
            m_Debugger->Debug_ShowMemoryLocation(g_Reg->m_GPR[29].UW[0], true);
        }
        break;
    case IDCANCEL:
        EndDialog(0);
        break;
    }
    return 0;
}

void CDebugStackView::Refresh()
{
    m_StackList.SetRedraw(FALSE);
    m_StackList.DeleteAllItems();

    uint32_t spBase;

    if (g_Reg != NULL)
    {
        spBase = g_Reg->m_GPR[29].UW[0];
        m_SPStatic.SetWindowTextA(stdstr_f("SP: %08X", spBase).c_str());
    }

    CSymbols::EnterCriticalSection();

    for (int i = 0; i < 0x10; i++)
    {
        char t[4];
        sprintf(t, "%02X", i * 0x10);
        m_StackList.AddItem(i, 0, t);

        for (int j = 0; j < 4; j++)
        {
            uint32_t vaddr = spBase + (i * 0x10) + (j * 4);
            uint32_t val;

            if (!m_Debugger->DebugLW_VAddr(vaddr, val))
            {
                m_StackList.AddItem(i, j + 1, "????????");
                continue;
            }

            char valStr[9];
            sprintf(valStr, "%08X", val);
            m_StackList.AddItem(i, j + 1, valStr);
        }
    }

    CSymbols::LeaveCriticalSection();

    m_StackList.SetRedraw(TRUE);
}