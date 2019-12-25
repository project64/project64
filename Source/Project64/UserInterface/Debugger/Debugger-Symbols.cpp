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

#include <stdio.h>
#include <Common/path.h>

#include "Symbols.h"

CDebugSymbols::CDebugSymbols(CDebuggerUI * debugger) :
    CDebugDialog<CDebugSymbols>(debugger)
{
}

LRESULT CDebugSymbols::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    DlgResize_Init(false, true);
    DlgSavePos_Init(DebuggerUI_SymbolsPos);

    m_SymbolsListView.Attach(GetDlgItem(IDC_SYMBOLS_LIST));
    m_SymbolsListView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    m_SymbolsListView.AddColumn("Address", 0);
    m_SymbolsListView.AddColumn("Type", 1);
    m_SymbolsListView.AddColumn("Name", 2);
    m_SymbolsListView.AddColumn("Value", 3);
    m_SymbolsListView.AddColumn("Description", 4);

    m_SymbolsListView.SetColumnWidth(0, 70);
    m_SymbolsListView.SetColumnWidth(1, 40);
    m_SymbolsListView.SetColumnWidth(2, 120);
    m_SymbolsListView.SetColumnWidth(3, 100);
    m_SymbolsListView.SetColumnWidth(4, 120);

    Refresh();

    SetTimer(TIMER_ID_AUTO_REFRESH, 100, NULL);

    LoadWindowPos();
    WindowCreated();
    return 0;
}

void CDebugSymbols::OnExitSizeMove(void)
{
    SaveWindowPos(true);
}

LRESULT CDebugSymbols::OnDestroy(void)
{
    KillTimer(TIMER_ID_AUTO_REFRESH);
    m_SymbolsListView.Detach();
    return 0;
}

void CDebugSymbols::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_ID_AUTO_REFRESH)
    {
        RefreshValues();
    }
}

LRESULT CDebugSymbols::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDC_ADDSYMBOL_BTN:
        m_AddSymbolDlg.DoModal(m_Debugger);
        break;
    case IDC_REMOVESYMBOL_BTN:
    {
        int nItem = m_SymbolsListView.GetSelectedIndex();
        if (nItem != -1)
        {
            int id = m_SymbolsListView.GetItemData(nItem);
            m_Debugger->SymbolTable()->RemoveSymbolById(id);
            m_Debugger->SymbolTable()->Save();
            Refresh();
        }
        break;
    }
    }
    return FALSE;
}

LRESULT    CDebugSymbols::OnListDblClicked(NMHDR* pNMHDR)
{
    // Open it in memory viewer/commands viewer
    NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int nItem = pIA->iItem;

    int id = m_SymbolsListView.GetItemData(nItem);

    CSymbol symbol;
    if (!m_Debugger->SymbolTable()->GetSymbolById(id, &symbol))
    {
        return 0;
    }

    if (symbol.m_Type == SYM_CODE) // code
    {
        m_Debugger->Debug_ShowCommandsLocation(symbol.m_Address, true);
    }
    else // data/number
    {
        m_Debugger->Debug_ShowMemoryLocation(symbol.m_Address, true);
    }

    return 0;
}

void CDebugSymbols::Refresh()
{
    if (m_SymbolsListView.m_hWnd == NULL)
    {
        return;
    }
    m_SymbolsListView.SetRedraw(FALSE);
    m_SymbolsListView.DeleteAllItems();

    CSymbol symbol;
    int nItem = 0;

    while (m_Debugger->SymbolTable()->GetSymbolByIndex(nItem, &symbol))
    {
        char szValue[64];
        m_Debugger->SymbolTable()->GetValueString(szValue, &symbol);

        stdstr strAddr = stdstr_f("%08X", symbol.m_Address);

        m_SymbolsListView.AddItem(nItem, 0, strAddr.c_str());
        m_SymbolsListView.AddItem(nItem, 1, symbol.TypeName());
        m_SymbolsListView.AddItem(nItem, 2, symbol.m_Name);
        m_SymbolsListView.AddItem(nItem, 4, symbol.m_Description);
        m_SymbolsListView.AddItem(nItem, 5, szValue);

        m_SymbolsListView.SetItemData(nItem, symbol.m_Id);
        nItem++;
    }

    m_SymbolsListView.SetRedraw(TRUE);
}

void CDebugSymbols::RefreshValues()
{
    if (g_MMU == NULL)
    {
        return;
    }

    int count = m_SymbolsListView.GetItemCount();
    
    CSymbol symbol;

    for (int i = 0; i < count; i++)
    {
        int symbolId = m_SymbolsListView.GetItemData(i);

        if (!m_Debugger->SymbolTable()->GetSymbolById(symbolId, &symbol))
        {
            break;
        }

        char szValue[64];
        m_Debugger->SymbolTable()->GetValueString(szValue, &symbol);

        m_SymbolsListView.SetItemText(i, 3, szValue);
    }
}
