#include "stdafx.h"

#include "DebuggerUI.h"
#include "Symbols.h"

CDebugStackTrace::CDebugStackTrace(CDebuggerUI * debugger) :
    CDebugDialog<CDebugStackTrace>(debugger),
    m_EntriesIndex(0)
{
}

CDebugStackTrace::~CDebugStackTrace()
{
}

LRESULT CDebugStackTrace::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    DlgResize_Init();
    DlgSavePos_Init(DebuggerUI_StackTracePos);

    m_List.Attach(GetDlgItem(IDC_STACKTRACE_LIST));
    m_List.AddColumn(L"Caller", 0);
    m_List.AddColumn(L"Routine", 1);
    m_List.AddColumn(L"Name", 2);

    m_List.SetColumnWidth(0, 70);
    m_List.SetColumnWidth(1, 70);
    m_List.SetColumnWidth(2, 160);

    m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

    LoadWindowPos();
    WindowCreated();
    return TRUE;
}

void CDebugStackTrace::OnExitSizeMove(void)
{
    SaveWindowPos(true);
}

LRESULT CDebugStackTrace::OnActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    Refresh();
    return FALSE;
}

LRESULT CDebugStackTrace::OnDestroy(void)
{
    m_List.Detach();
    return FALSE;
}

LRESULT CDebugStackTrace::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    }
    return FALSE;
}

LRESULT CDebugStackTrace::OnListDblClicked(NMHDR * pNMHDR)
{
    NMITEMACTIVATE * pIA = reinterpret_cast<NMITEMACTIVATE *>(pNMHDR);
    int nItem = pIA->iItem;

    uint32_t address = m_List.GetItemData(nItem);

    m_Debugger->Debug_ShowCommandsLocation(address, true);

    return 0;
}

void CDebugStackTrace::Refresh()
{
    if (!isStepping())
    {
        return;
    }

    SetWindowText(stdstr_f("Stack trace (%d)", m_EntriesIndex).ToUTF16().c_str());

    m_List.SetRedraw(FALSE);
    m_List.DeleteAllItems();

    for (int i = 0; i < m_EntriesIndex; i++)
    {
        uint32_t routineAddress = m_Entries[i].routineAddress;
        uint32_t callingAddress = m_Entries[i].callingAddress;

        m_List.AddItem(i, 0, stdstr_f("%08X", callingAddress).ToUTF16().c_str());
        m_List.AddItem(i, 1, stdstr_f("%08X", routineAddress).ToUTF16().c_str());

        CSymbol symbol;
        if (m_Debugger->SymbolTable()->GetSymbolByAddress(routineAddress, &symbol))
        {
            m_List.AddItem(i, 2, stdstr(symbol.m_Name).ToUTF16().c_str());
        }
        else
        {
            m_List.AddItem(i, 2, L"");
        }

        m_List.SetItemData(i, routineAddress);
    }

    m_List.SetRedraw(TRUE);
}
