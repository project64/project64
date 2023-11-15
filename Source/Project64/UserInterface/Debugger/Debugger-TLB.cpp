#include "stdafx.h"

#include "DebuggerUI.h"

CDebugTlb::CDebugTlb(CDebuggerUI * debugger) :
    CDebugDialog<CDebugTlb>(debugger)
{
}

CDebugTlb::~CDebugTlb()
{
}

LRESULT CDebugTlb::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    DlgSavePos_Init(DebuggerUI_TLBPos);

    LV_COLUMN col;
    float DPIScale = CClientDC(m_hWnd).GetDeviceCaps(LOGPIXELSX) / 96.0f;

    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    col.fmt = LVCFMT_LEFT;

    col.pszText = L"Index";
    col.cx = (int)(40 * DPIScale);
    col.iSubItem = 0;
    ListView_InsertColumn(GetDlgItem(IDC_LIST), 0, &col);

    col.pszText = L"Page mask";
    col.cx = (int)(90 * DPIScale);
    col.iSubItem = 1;
    ListView_InsertColumn(GetDlgItem(IDC_LIST), 1, &col);

    col.pszText = L"Entry Hi";
    col.cx = (int)(90 * DPIScale);
    col.iSubItem = 2;
    ListView_InsertColumn(GetDlgItem(IDC_LIST), 2, &col);

    col.pszText = L"Entry Lo0";
    col.cx = (int)(90 * DPIScale);
    col.iSubItem = 3;
    ListView_InsertColumn(GetDlgItem(IDC_LIST), 3, &col);

    col.pszText = L"Entry Lo1";
    col.cx = (int)(90 * DPIScale);
    col.iSubItem = 4;
    ListView_InsertColumn(GetDlgItem(IDC_LIST), 4, &col);

    col.pszText = L"Index";
    col.cx = (int)(40 * DPIScale);
    col.iSubItem = 0;
    ListView_InsertColumn(GetDlgItem(IDC_LIST2), 0, &col);

    col.pszText = L"Valid";
    col.cx = (int)(40 * DPIScale);
    col.iSubItem = 1;
    ListView_InsertColumn(GetDlgItem(IDC_LIST2), 1, &col);

    col.pszText = L"Dirty";
    col.cx = (int)(40 * DPIScale);
    col.iSubItem = 2;
    ListView_InsertColumn(GetDlgItem(IDC_LIST2), 2, &col);

    col.pszText = L"Rule";
    col.cx = (int)(270 * DPIScale);
    col.iSubItem = 3;
    ListView_InsertColumn(GetDlgItem(IDC_LIST2), 3, &col);

    RefreshTLBWindow();
    SendMessage(GetDlgItem(IDC_TLB_ENTRIES), BM_SETCHECK, BST_CHECKED, 0);

    LoadWindowPos();
    WindowCreated();
    return TRUE;
}

void CDebugTlb::OnExitSizeMove(void)
{
    SaveWindowPos(0);
}

LRESULT CDebugTlb::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND, BOOL & /*bHandled*/)
{
    switch (wID)
    {
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDC_TLB_ENTRIES:
        ::ShowWindow(GetDlgItem(IDC_LIST), SW_SHOW);
        ::ShowWindow(GetDlgItem(IDC_LIST2), SW_HIDE);
        break;
    case IDC_TLB_RULES:
        ::ShowWindow(GetDlgItem(IDC_LIST), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_LIST2), SW_SHOW);
        break;
    }
    return FALSE;
}

void CDebugTlb::RefreshTLBWindow(void)
{
    if (m_hWnd == nullptr)
    {
        return;
    }

    HWND hList = GetDlgItem(IDC_LIST);
    wchar_t Output[100], OldText[100];
    LV_ITEM item, OldItem;
    int count;

    TLB_ENTRY * tlb = g_TLB->m_tlb;
    for (count = 0; count < 32; count++)
    {
        swprintf(Output, sizeof(Output), L"0x%02X", count);
        item.mask = LVIF_TEXT;
        item.iItem = count;
        item.pszText = Output;
        item.iSubItem = 0;

        OldItem.mask = LVIF_TEXT;
        OldItem.iItem = count;
        OldItem.pszText = OldText;
        OldItem.cchTextMax = sizeof(OldText) - 1;
        OldItem.iSubItem = 0;

        if (ListView_GetItemCount(hList) <= count)
        {
            ListView_InsertItem(hList, &item);
        }
        else
        {
            ListView_GetItem(hList, &OldItem);
            if (wcscmp(item.pszText, OldItem.pszText) != 0)
            {
                ListView_SetItem(hList, &item);
            }
        }
        if (tlb[count].EntryDefined)
        {
            swprintf(Output, sizeof(Output), L"0x%llX", tlb[count].PageMask.Value);
        }
        else
        {
            wcscpy(Output, L"................");
        }
        item.iSubItem = 1;
        OldItem.iSubItem = 1;
        ListView_GetItem(hList, &OldItem);
        if (wcscmp(item.pszText, OldItem.pszText) != 0)
        {
            ListView_SetItem(hList, &item);
        }

        if (tlb[count].EntryDefined)
        {
            swprintf(Output, sizeof(Output), L"0x%llX", tlb[count].EntryHi.Value);
        }
        else
        {
            wcscpy(Output, L"................");
        }
        item.iSubItem = 2;
        OldItem.iSubItem = 2;
        ListView_GetItem(hList, &OldItem);
        if (wcscmp(item.pszText, OldItem.pszText) != 0)
        {
            ListView_SetItem(hList, &item);
        }

        if (tlb[count].EntryDefined)
        {
            swprintf(Output, sizeof(Output), L"0x%llX", tlb[count].EntryLo0.Value);
        }
        else
        {
            wcscpy(Output, L"................");
        }
        item.iSubItem = 3;
        OldItem.iSubItem = 3;
        ListView_GetItem(hList, &OldItem);
        if (wcscmp(item.pszText, OldItem.pszText) != 0)
        {
            ListView_SetItem(hList, &item);
        }

        if (tlb[count].EntryDefined)
        {
            swprintf(Output, sizeof(Output), L"0x%llX", tlb[count].EntryLo1.Value);
        }
        else
        {
            wcscpy(Output, L"................");
        }
        item.iSubItem = 4;
        OldItem.iSubItem = 4;
        ListView_GetItem(hList, &OldItem);
        if (wcscmp(item.pszText, OldItem.pszText) != 0)
        {
            ListView_SetItem(hList, &item);
        }
    }

    CTLB::FASTTLB * FastTlb = g_TLB->m_FastTlb;
    hList = GetDlgItem(IDC_LIST2);
    for (count = 0; count < 64; count++)
    {
        swprintf(Output, sizeof(Output), L"0x%02X", count);
        item.mask = LVIF_TEXT;
        item.iItem = count;
        item.pszText = Output;
        item.iSubItem = 0;

        OldItem.mask = LVIF_TEXT;
        OldItem.iItem = count;
        OldItem.pszText = OldText;
        OldItem.cchTextMax = sizeof(OldText) - 1;
        OldItem.iSubItem = 0;

        if (ListView_GetItemCount(hList) <= count)
        {
            item.iItem = ListView_InsertItem(hList, &item);
        }
        else
        {
            ListView_GetItem(hList, &OldItem);
            if (wcscmp(item.pszText, OldItem.pszText) != 0)
            {
                ListView_SetItem(hList, &item);
            }
        }

        if (FastTlb[count].ValidEntry)
        {
            swprintf(Output, sizeof(Output), L"%s", FastTlb[count].VALID ? L"Yes" : L"No");
        }
        else
        {
            wcscpy(Output, L"................");
        }
        item.iSubItem = 1;
        OldItem.iSubItem = 1;
        ListView_GetItem(hList, &OldItem);
        if (wcscmp(item.pszText, OldItem.pszText) != 0)
        {
            ListView_SetItem(hList, &item);
        }

        if (FastTlb[count].ValidEntry && FastTlb[count].VALID)
        {
            swprintf(Output, sizeof(Output), L"%s", FastTlb[count].DIRTY ? L"Yes" : L"No");
        }
        else
        {
            wcscpy(Output, L"................");
        }
        item.iSubItem = 2;
        OldItem.iSubItem = 2;
        ListView_GetItem(hList, &OldItem);
        if (wcscmp(item.pszText, OldItem.pszText) != 0)
        {
            ListView_SetItem(hList, &item);
        }

        if (FastTlb[count].ValidEntry && FastTlb[count].VALID)
        {
            swprintf(Output, sizeof(Output), L"%llX:%llX -> %08X:%08X", FastTlb[count].VSTART, FastTlb[count].VEND,
                     FastTlb[count].PHYSSTART, FastTlb[count].PHYSEND);
        }
        else
        {
            wcscpy(Output, L"................");
        }
        item.iSubItem = 3;
        OldItem.iSubItem = 3;
        ListView_GetItem(hList, &OldItem);
        if (wcscmp(item.pszText, OldItem.pszText) != 0)
        {
            ListView_SetItem(hList, &item);
        }
    }
}
