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

    m_AutoRefreshThread = CreateThread(NULL, 0, AutoRefreshProc, (void*)this, 0, NULL);

    WindowCreated();
    return 0;
}

LRESULT CDebugSymbols::OnDestroy(void)
{
    m_SymbolsListView.Detach();
    if (m_AutoRefreshThread != NULL)
    {
        TerminateThread(m_AutoRefreshThread, 0);
        CloseHandle(m_AutoRefreshThread);
    }
    return 0;
}

DWORD WINAPI CDebugSymbols::AutoRefreshProc(void* _this)
{
    CDebugSymbols* self = (CDebugSymbols*)_this;
    while (true)
    {
        self->RefreshValues();
        Sleep(100);
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
        int id = m_SymbolsListView.GetItemData(m_SymbolsListView.GetSelectedIndex());
        CSymbols::EnterCriticalSection();
        CSymbols::RemoveEntryById(id);
        CSymbols::Save();
        CSymbols::LeaveCriticalSection();
        Refresh();
        break;
    }
    }
    return FALSE;
}

LRESULT	CDebugSymbols::OnListDblClicked(NMHDR* pNMHDR)
{
    // Open it in memory viewer/commands viewer
    NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int nItem = pIA->iItem;

    int id = m_SymbolsListView.GetItemData(nItem);
    CSymbolEntry* symbol = CSymbols::GetEntryById(id);

    if (symbol->m_Type == 0) // code
    {
        m_Debugger->Debug_ShowCommandsLocation(symbol->m_Address, true);
    }
    else // data/number
    {
        m_Debugger->Debug_ShowMemoryLocation(symbol->m_Address, true);
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

    CSymbols::EnterCriticalSection();

    int count = CSymbols::GetCount();

    for (int i = 0; i < count; i++)
    {
        CSymbolEntry* lpSymbol = CSymbols::GetEntryByIndex(i);

        stdstr addrStr = stdstr_f("%08X", lpSymbol->m_Address);

        m_SymbolsListView.AddItem(i, 0, addrStr.c_str());
        m_SymbolsListView.AddItem(i, 1, lpSymbol->TypeName());
        m_SymbolsListView.AddItem(i, 2, lpSymbol->m_Name);
        m_SymbolsListView.AddItem(i, 4, lpSymbol->m_Description);

        m_SymbolsListView.SetItemData(i, lpSymbol->m_Id);

        if (g_MMU)
        {
            char szValue[64];
            CSymbols::GetValueString(szValue, lpSymbol);
            m_SymbolsListView.AddItem(i, 3, szValue);
        }
    }

    CSymbols::LeaveCriticalSection();

    m_SymbolsListView.SetRedraw(TRUE);
}

void CDebugSymbols::RefreshValues()
{
    if (g_MMU == NULL)
    {
        return;
    }

    int count = m_SymbolsListView.GetItemCount();

    CSymbols::EnterCriticalSection();

    for (int i = 0; i < count; i++)
    {
        int symbolId = m_SymbolsListView.GetItemData(i);

        CSymbolEntry* lpSymbol = CSymbols::GetEntryById(symbolId);

        char szValue[64];
        CSymbols::GetValueString(szValue, lpSymbol);

        m_SymbolsListView.SetItemText(i, 3, szValue);
    }

    CSymbols::LeaveCriticalSection();
}