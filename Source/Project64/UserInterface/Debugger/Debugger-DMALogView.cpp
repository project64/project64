#include "stdafx.h"
#include "DebuggerUI.h"
#include "DMALog.h"
#include <fstream>

CDebugDMALogView::CDebugDMALogView(CDebuggerUI* debugger) :
CDebugDialog<CDebugDMALogView>(debugger)
{
    m_DMALog = debugger->DMALog();
    m_bFilterChanged = false;
    m_bUniqueRomAddresses = true;
    m_bCustomDrawClrNext = false;
}

CDebugDMALogView::~CDebugDMALogView()
{
}

/*
bool CDebugDMALogView::FilterEntry(int dmaLogIndex)
{
    DMALogEntry entry = m_Debugger->DMALog()->at(dmaLogIndex);

    for (int i = 0; i < dmaLogIndex; i++)
    {
        DMALogEntry testEntry = m_Debugger->DMALog()->at(i);

        // Don't show if another entry has the same ROM address
        if (entry.romAddr == testEntry.romAddr)
        {
            return false;
        }
    }

    return true;
}
*/

void CDebugDMALogView::RefreshList()
{
    if (g_Rom == nullptr)
    {
        return;
    }
    
    uint8_t* rom = g_Rom->GetRomAddress();
    
    // Get scrollbar state
    SCROLLINFO scroll;
    scroll.cbSize = sizeof(SCROLLINFO);
    scroll.fMask = SIF_ALL;
    m_DMAList.GetScrollInfo(SB_VERT, &scroll);

    bool bScrolledDown = false;

    if ((scroll.nPage + scroll.nPos) - 1 == (uint32_t)scroll.nMax)
    {
        bScrolledDown = true;
    }

    int startIndex;
    int dmaLogSize = m_Debugger->DMALog()->GetNumEntries();
    
    HWND hWndExportBtn = GetDlgItem(IDC_EXPORT_BTN);

    if (dmaLogSize == 0)
    {
        // Reset
        m_DMAList.DeleteAllItems();
        startIndex = 0;
        m_bFilterChanged = false;
        ::EnableWindow(hWndExportBtn, FALSE);
    }
    else
    {
        // Continue from last index
        startIndex = m_nLastStartIndex;
        ::EnableWindow(hWndExportBtn, TRUE);
    }
    
    m_DMAList.SetRedraw(FALSE);

    int itemIndex = m_DMAList.GetItemCount();
    
    for (int i = startIndex; i < dmaLogSize; i++)
    {
        DMALOGENTRY* lpEntry = m_DMALog->GetEntryByIndex(i);

        //if (!FilterEntry(i))
        //{
        //    continue;
        //}
        
        m_DMAList.AddItem(itemIndex, 0, stdstr_f("%08X", lpEntry->romAddr).ToUTF16().c_str());
        m_DMAList.AddItem(itemIndex, 1, stdstr_f("%08X", lpEntry->ramAddr).ToUTF16().c_str());
        m_DMAList.AddItem(itemIndex, 2, stdstr_f("%08X (%d)", lpEntry->length, lpEntry->length).ToUTF16().c_str());
        
        union
        {
            uint32_t u32;
            uint8_t sz[5];
        } sig = { 0 };

        if (lpEntry->romAddr < g_Rom->GetRomSize())
        {
            sig.u32 = _byteswap_ulong(*(uint32_t*)&rom[lpEntry->romAddr]);
        }

        // TODO: checkbox to display all in hex
        if (isalnum(sig.sz[0]) && isalnum(sig.sz[1]) && isalnum(sig.sz[2]) && isalnum(sig.sz[3]))
        {
            m_DMAList.AddItem(itemIndex, 4, stdstr((char*)sig.sz).ToUTF16().c_str());
        }

        itemIndex++;
    }
    
    if (bScrolledDown)
    {
        m_DMAList.EnsureVisible(m_DMAList.GetItemCount() - 1, FALSE);
    }

    m_DMAList.SetRedraw(TRUE);
    
    m_nLastStartIndex = dmaLogSize;
}

void CDebugDMALogView::Export(void)
{
    OPENFILENAME openfilename;
    TCHAR filePath[_MAX_PATH];

    memset(&filePath, 0, sizeof(filePath));
    memset(&openfilename, 0, sizeof(openfilename));
    
    wsprintf(filePath, L"*.csv");

    const TCHAR* filters = (
        /*1*/ L"Comma separated values (*.csv)\0*.csv;\0"
        /*2*/ L"Plain text (*.txt)\0*.txt;\0"
    );

    const char *extensions[] = { "", ".csv", ".txt" };

    openfilename.lStructSize = sizeof(openfilename);
    openfilename.hwndOwner = (HWND)m_hWnd;
    openfilename.lpstrFilter = filters;
    openfilename.lpstrFile = filePath;
    openfilename.lpstrInitialDir = L".";
    openfilename.nMaxFile = MAX_PATH;
    openfilename.Flags = OFN_HIDEREADONLY;

    if (GetSaveFileName(&openfilename))
    {
        stdstr path;
        path.FromUTF16(filePath);

        if (openfilename.nFileExtension == 0)
        {
            path += extensions[openfilename.nFilterIndex];
        }

        std::ofstream file;
        file.open(path, std::ios::out | std::ios::binary);

        if (!file.is_open())
        {
            return;
        }

        file << "ROM Address,RAM Address,Length\r\n";

        size_t numEntries = m_DMALog->GetNumEntries();

        for (size_t nEntry = 0; nEntry < numEntries; nEntry++)
        {
            DMALOGENTRY* entry = m_DMALog->GetEntryByIndex(nEntry);

            file << stdstr_f("0x%08X,0x%08X,0x%08X\r\n",
                entry->romAddr, entry->ramAddr, entry->length);
        }

        file.close();
    }
}

LRESULT CDebugDMALogView::OnActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    //RefreshList();
    return FALSE;
}

LRESULT CDebugDMALogView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    DlgResize_Init(false, true);
    DlgSavePos_Init(DebuggerUI_DMALogPos);

    m_bConvertingAddress = false;
    m_nLastStartIndex = 0;

    m_DMAList.Attach(GetDlgItem(IDC_DMA_LIST));
    m_DMARamEdit.Attach(GetDlgItem(IDC_DMA_RAM_EDIT));
    m_DMARomEdit.Attach(GetDlgItem(IDC_DMA_ROM_EDIT));
    m_BlockInfo.Attach(GetDlgItem(IDC_BLOCK_INFO));

    m_DMAList.ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0);

    m_DMAList.AddColumn(L"ROM", 0);
    m_DMAList.AddColumn(L"RAM", 1);
    m_DMAList.AddColumn(L"Length", 2);
    m_DMAList.AddColumn(L"Symbol (RAM)", 3);
    m_DMAList.AddColumn(L"Signature", 4);

    m_DMAList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    m_DMAList.SetColumnWidth(0, 65);
    m_DMAList.SetColumnWidth(1, 65);
    m_DMAList.SetColumnWidth(2, 120);
    //m_DMAList.SetColumnWidth(3, 50);
    //m_DMAList.SetColumnWidth(4, 50);
    //m_DMAList.SetColumnWidth(5, 50);
    
    m_DMARamEdit.SetLimitText(8);
    m_DMARomEdit.SetLimitText(8);

    RefreshList();

    LoadWindowPos();
    WindowCreated();

    return TRUE;
}

void CDebugDMALogView::RefreshDMALogWindow(bool bReset)
{
    if (m_hWnd == nullptr || m_DMAList.m_hWnd == nullptr)
    {
        if (bReset)
        {
            m_DMALog->ClearEntries();
        }
        return;
    }

    PostMessage(WM_REFRESH, bReset);
}

LRESULT CDebugDMALogView::OnRefresh(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    bool bReset = (wParam != 0);

    if (bReset)
    {
        m_DMALog->ClearEntries();
    }

    RefreshList();
    return TRUE;
}

void CDebugDMALogView::OnExitSizeMove(void)
{
    SaveWindowPos(true);
}

LRESULT CDebugDMALogView::OnDestroy(void)
{
    m_DMAList.Detach();
    m_DMARamEdit.Detach();
    m_DMARomEdit.Detach();
    m_BlockInfo.Detach();

    return 0;
}

LRESULT CDebugDMALogView::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDOK:
        EndDialog(0);
        break;
    case IDCANCEL:
        EndDialog(0);
        break;
    case IDC_CLEAR_BTN:
        m_DMALog->ClearEntries();
        RefreshList();
        break;
    case IDC_EXPORT_BTN:
        Export();
        break;
    }
    return FALSE;
}

LRESULT CDebugDMALogView::OnRamAddrChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (m_bConvertingAddress)
    {
        return FALSE;
    }

    wchar_t szRamAddr[9];
    char szRomAddr[9];

    m_DMARamEdit.GetWindowText(szRamAddr, 9);
    uint32_t ramAddr = wcstoul(szRamAddr, nullptr, 16);
    uint32_t romAddr, offset;

    DMALOGENTRY* lpEntry = m_DMALog->GetEntryByRamAddress(ramAddr, &romAddr, &offset);

    if (lpEntry != nullptr)
    {
        sprintf(szRomAddr, "%08X", romAddr);
        stdstr_f blockInfo("Block: %08X -> %08X [%X] +%X", romAddr, ramAddr, lpEntry->length, offset);
        m_BlockInfo.SetWindowText(blockInfo.ToUTF16().c_str());
    }
    else
    {
        sprintf(szRomAddr, "????????");
        m_BlockInfo.SetWindowText(L"Block: ?");
    }
    
    m_bConvertingAddress = true;
    m_DMARomEdit.SetWindowText(stdstr(szRomAddr).ToUTF16().c_str());
    m_bConvertingAddress = false;
    return FALSE;
}

LRESULT CDebugDMALogView::OnRomAddrChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (m_bConvertingAddress)
    {
        return FALSE;
    }

    wchar_t szRamAddr[9];
    wchar_t szRomAddr[9];

    m_DMARomEdit.GetWindowText(szRomAddr, 9);
    uint32_t romAddr = wcstoul(szRomAddr, nullptr, 16);
    uint32_t ramAddr, offset;

    DMALOGENTRY* lpEntry = m_DMALog->GetEntryByRomAddress(romAddr, &ramAddr, &offset);

    if (lpEntry != nullptr)
    {
        wsprintf(szRamAddr, L"%08X", ramAddr);
        stdstr blockInfo = stdstr_f("Block: %08X -> %08X [%X] +%X", romAddr, ramAddr, lpEntry->length, offset);
        m_BlockInfo.SetWindowText(blockInfo.ToUTF16().c_str());
    }
    else
    {
        wsprintf(szRamAddr, L"????????");
        m_BlockInfo.SetWindowText(L"Block: ?");
    }

    m_bConvertingAddress = true;
    m_DMARamEdit.SetWindowText(szRamAddr);
    m_bConvertingAddress = false;
    return FALSE;
}


LRESULT CDebugDMALogView::OnCustomDrawList(NMHDR* pNMHDR)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    DWORD drawStage = pLVCD->nmcd.dwDrawStage;

    switch (drawStage)
    {
    case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW;
    case (CDDS_ITEMPREPAINT | CDDS_SUBITEM): break;
    default: return CDRF_DODEFAULT;
    }
    
    DWORD nItem = pLVCD->nmcd.dwItemSpec;
    DWORD nSubItem = pLVCD->iSubItem;

    if (nSubItem != 0)
    {
        return CDRF_DODEFAULT;
    }
    
    size_t nEntries = m_DMALog->GetNumEntries();

    if (nEntries == 0)
    {
        return CDRF_DODEFAULT;
    }
    
    DMALOGENTRY* lpEntry = m_DMALog->GetEntryByIndex(nItem);
    
    if (nItem >= 1) // Continuation
    {
        DMALOGENTRY* lpPrevEntry = m_DMALog->GetEntryByIndex(nItem - 1);

        if (lpEntry->romAddr == lpPrevEntry->romAddr + lpPrevEntry->length)
        {
            pLVCD->clrTextBk = RGB(0xFF, 0xFF, 0xCC);
            return CDRF_DODEFAULT;
        }
    }

    if (nEntries >= 2 && nItem <= nEntries - 2) // Head
    {
        DMALOGENTRY* lpNextEntry = m_DMALog->GetEntryByIndex(nItem + 1);

        if (lpNextEntry->romAddr == lpEntry->romAddr + lpEntry->length)
        {
            pLVCD->clrTextBk = RGB(0xFF, 0xFF, 0x88);
            return CDRF_DODEFAULT;
        }
    }
    
    return CDRF_DODEFAULT;
}
