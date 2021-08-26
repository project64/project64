#include "stdafx.h"
#include <Project64-core/ExceptionHandler.h>
#include <Common/MemoryManagement.h>

#include <UserInterface/WTLControls/HexEditCtrl.h>

#include "DebuggerUI.h"
#include "Symbols.h"
#include "DMALog.h"

CDebugMemoryView::jump_item_t CDebugMemoryView::JumpItems[] = {
    { 0x80000000, 0x00000000, 0x0800000, "RDRAM" },
    { 0xA3F00000, 0x03F00000, 0x0000028, "RDRAM registers" },
    { 0xA4000000, 0x04000000, 0x0001000, "SP DMEM" },
    { 0xA4001000, 0x04001000, 0x0001000, "SP IMEM" },
    { 0xA4040000, 0x04040000, 0x0000020, "SP registers" },
    { 0xA4080000, 0x04080000, 0x0000004, "SP PC register" },
    { 0xA4100000, 0x04100000, 0x0000020, "DP control registers" },
    { 0xA4300000, 0x04300000, 0x0000010, "MI registers" },
    { 0xA4400000, 0x04400000, 0x0000038, "VI registers" },
    { 0xA4500000, 0x04500000, 0x0000018, "AI registers" },
    { 0xA4600000, 0x04600000, 0x0000034, "PI registers" },
    { 0xA4700000, 0x04700000, 0x0000020, "RI registers" },
    { 0xA4800000, 0x04800000, 0x0000010, "SI registers" },
    { 0xA5000500, 0x05000500, 0x000004C, "DD registers" },
    { 0xA8000000, 0x08000000, 0x1000000, "Cartridge save data" },
    { 0xB0000000, 0x10000000, 0xFC00000, "Cartridge ROM" },
    { 0xBFC00000, 0x1FC00000, 0x00007C0, "PIF ROM" },
    { 0xBFC007C0, 0x1FC007C0, 0x0000040, "PIF RAM" },
    { 0, NULL}
};

CDebugMemoryView::CDebugMemoryView(CDebuggerUI * debugger) :
    CDebugDialog<CDebugMemoryView>(debugger),
    CDialogResize<CDebugMemoryView>(),
    CToolTipDialog<CDebugMemoryView>(),
    m_Breakpoints(nullptr),
    m_WriteTargetColorStride(0),
    m_ReadTargetColorStride(0),
    m_SymbolColorStride(0),
    m_SymbolColorPhase(0),
    m_bIgnoreAddressInput(false),
    m_HotAddress(0),
    m_bVirtualMemory(true),
    m_ContextMenuAddress(0),
    m_bSafeEditMode(false)
{
    m_Breakpoints = m_Debugger->Breakpoints();
}

CDebugMemoryView::~CDebugMemoryView()
{
}

void CDebugMemoryView::ShowAddress(uint32_t address, bool bVirtual)
{
    if (m_hWnd == nullptr)
    {
        return;
    }

    SendMessage(WM_SHOWADDRESS, (WPARAM)address, (LPARAM)bVirtual);
}

bool CDebugMemoryView::GetByte(uint32_t address, uint8_t* value)
{
    if (m_bVirtualMemory)
    {
        return m_Debugger->DebugLoad_VAddr(address, *value);
    }
    else
    {
        return m_Debugger->DebugLoad_PAddr(address, *value);
    }
}

bool CDebugMemoryView::SetByte(uint32_t address, uint8_t value)
{
    if (m_bVirtualMemory)
    {
        return m_Debugger->DebugStore_VAddr(address, value);
    }
    else
    {
        return m_Debugger->DebugStore_PAddr(address, value);
    }
}

void CDebugMemoryView::CopyTextToClipboard(const char* text)
{
    size_t length = strlen(text);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, length + 1);
    strcpy((char*)GlobalLock(hMem), text);
    GlobalUnlock(hMem);
    OpenClipboard();
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

void CDebugMemoryView::CopyBytesToClipboard(uint32_t startAddress, uint32_t endAddress, bool bHex, bool bIncludeAddresses, bool bRowAddresses)
{
    uint32_t baseAddress = m_HexEditCtrl.GetBaseAddress();
    int groupSize = m_HexEditCtrl.GetNumBytesPerGroup();
    int rowSize = m_HexEditCtrl.GetNumBytesPerRow();

    stdstr str = "";

    for (uint32_t address = startAddress; address <= endAddress; address++)
    {
        int offsetFromBase = address - baseAddress;
        int offsetFromSelStart = address - startAddress;

        uint8_t value;
        GetByte(address, &value);
        
        if (bIncludeAddresses)
        {
            if ((bRowAddresses && offsetFromBase % rowSize == 0) ||
                (!bRowAddresses && offsetFromBase % groupSize == 0) ||
                (offsetFromSelStart == 0))
            {
                str += stdstr_f("%08X: ", address);
            }
        }

        if (bHex)
        {
            str += stdstr_f("%02X", value);
        }
        else
        {
            str += CHexEditCtrl::ByteAscii(value);
        }

        if ((offsetFromBase + 1) % rowSize == 0 || (bIncludeAddresses && !bRowAddresses && (offsetFromBase + 1) % groupSize == 0))
        {
            str += "\r\n";
        }
        else if (bHex && (offsetFromBase + 1) % groupSize == 0)
        {
            str += " ";
        }
    }

    CopyTextToClipboard(str.Trim(" \r\n").c_str());
}

void CDebugMemoryView::CopyGameSharkCodeToClipboard(uint32_t startAddress, uint32_t endAddress)
{
    stdstr str = "";

    if (startAddress & 1)
    {
        uint8_t value = 0;
        GetByte(startAddress, &value);
        str += stdstr_f("%08X %04X\r\n", startAddress, value);
        startAddress++;
    }
    
    for (uint32_t address = startAddress; address < endAddress; address += 2)
    {
        uint8_t value0 = 0, value1 = 0;
        GetByte(address + 0, &value0);
        GetByte(address + 1, &value1);
        str += stdstr_f("%08X %02X%02X\r\n", address | 0x01000000, value0, value1);
    }

    if (!(endAddress & 1))
    {
        uint8_t value = 0;
        GetByte(endAddress, &value);
        str += stdstr_f("%08X %04X\r\n", endAddress, value);
    }

    CopyTextToClipboard(str.Trim("\n").c_str());
}

void CDebugMemoryView::FillRange(uint32_t startAddress, uint32_t endAddress, uint8_t value)
{
    for (uint32_t address = startAddress; address <= endAddress; address++)
    {
        SetByte(address, value);
    }
}

void CDebugMemoryView::FollowPointer(bool bContextMenuAddress)
{
    uint32_t address;

    if (bContextMenuAddress)
    {
        address = m_ContextMenuAddress & (~3);
    }
    else
    {
        uint32_t selStartAddress, selEndAddress;
        m_HexEditCtrl.GetSelectionRange(&selStartAddress, &selEndAddress);
        address = selStartAddress & (~3);
    }

    uint32_t pointer;
    bool bValid;

    if (m_bVirtualMemory)
    {
        bValid = m_Debugger->DebugLoad_VAddr(address, pointer);
    }
    else
    {
        bValid = m_Debugger->DebugLoad_VAddr(address, pointer);
    }

    if (bValid)
    {
        OpenNewTab(pointer, m_bVirtualMemory, 4, true, true);
    }
}


void CDebugMemoryView::JumpToSelection(void)
{
    uint32_t startAddress, endAddress;
    bool bHaveSelection = m_HexEditCtrl.GetSelectionRange(&startAddress, &endAddress);
    uint32_t targetAddress = bHaveSelection ? startAddress : m_HexEditCtrl.GetCaretAddress();
    m_MemAddr.SetValue(targetAddress, DisplayMode::ZeroExtend);
}

bool CDebugMemoryView::GetSafeEditValue(uint32_t address, uint8_t* value)
{
    if (m_SafeEditQueue.size() == 0)
    {
        return false;
    }

    for(size_t i = m_SafeEditQueue.size(); i-- > 0;)
    {
        edit_t edit = m_SafeEditQueue[i];
        if (address >= edit.startAddress && address <= edit.endAddress)
        {
            *value = edit.value;
            return true;
        }
    }
    return false;
}

void CDebugMemoryView::ApplySafeEdits(void)
{
    for (size_t i = 0; i < m_SafeEditQueue.size(); i++)
    {
        edit_t edit = m_SafeEditQueue[i];
        if (edit.type == SE_FILL)
        {
            FillRange(edit.startAddress, edit.endAddress, edit.value);
        }
    }

    m_SafeEditQueue.clear();
}

void CDebugMemoryView::SetupJumpMenu(bool bVirtual)
{
    m_CmbJump.SetRedraw(FALSE);
    m_CmbJump.ResetContent();

    for (int i = 0;; i++)
    {
        jump_item_t* item = &JumpItems[i];

        if (item->caption == nullptr)
        {
            break;
        }

        m_CmbJump.AddString(stdstr_f("%08X %s", bVirtual ? item->vaddr : item->paddr, item->caption).ToUTF16().c_str());
    }

    m_CmbJump.SetRedraw(TRUE);
}

int CDebugMemoryView::GetJumpItemIndex(uint32_t address, bool bVirtual)
{
    for (int nItem = 0;; nItem++)
    {
        if (JumpItems[nItem].caption == nullptr)
        {
            break;
        }

        uint32_t start = bVirtual ? JumpItems[nItem].vaddr : JumpItems[nItem].paddr;
        uint32_t end = start + JumpItems[nItem].size - 1;

        if (address >= start && address <= end)
        {
            return nItem;
        }
    }
    return -1;
}

LRESULT CDebugMemoryView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    DlgResize_Init(false, true);
    DlgSavePos_Init(DebuggerUI_MemoryPos);
    DlgToolTip_Init();

    m_HexEditCtrl.Attach(GetDlgItem(IDC_HEXEDIT));
    m_TabCtrl.Attach(GetDlgItem(IDC_MEMTABS));
    m_MemAddr.Attach(GetDlgItem(IDC_ADDR_EDIT));
    m_VirtualCheckbox.Attach(GetDlgItem(IDC_CHK_VADDR));
    m_StatusBar.Attach(GetDlgItem(IDC_STATUSBAR));
    m_CmbJump.Attach(GetDlgItem(IDC_CMB_JUMP));

    m_SymbolColorStride = 0;
    m_SymbolColorPhase = 0;

    HWND hScrlBar = GetDlgItem(IDC_SCRL_BAR);
    if (hScrlBar)
    {
        SCROLLINFO si;

        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
        si.nMin = 0;
        si.nMax = 0xFFFF;
        si.nPos = 0x8000;
        si.nPage = 100;
        ::SetScrollInfo(hScrlBar, SB_CTL, &si, TRUE);
    }

    m_MemAddr.SetDisplayType(CEditNumber32::DisplayHex);
    m_MemAddr.SetValue(0x80000000, DisplayMode::ZeroExtend);

    m_VirtualCheckbox.SetCheck(BST_CHECKED);

    float dpiScale = CClientDC(m_hWnd).GetDeviceCaps(LOGPIXELSX) / 96.0f;

    int statusPaneWidths[MEMSB_NUM_PANES] = {
        (int)(MEMSB_HOTADDR_W * dpiScale),
        (int)(MEMSB_BLOCK_W * dpiScale),
        (int)(MEMSB_BLOCKLEN_W * dpiScale),
        (int)(MEMSB_DMAINFO_W * dpiScale),
        (int)(MEMSB_SAFEMODE_W * dpiScale)
    };

    m_StatusBar.SetParts(MEMSB_NUM_PANES, statusPaneWidths);

    SetupJumpMenu(true);
    m_CmbJump.SetCurSel(0);
    
    m_TabData.clear();
    AddTab(0x80000000, true, 4);

    m_HexEditCtrl.Draw();

    LoadWindowPos();
    WindowCreated();

    return TRUE;
}

LRESULT CDebugMemoryView::OnDestroy(void)
{
    m_HexEditCtrl.Detach();
    m_MemAddr.Detach();
    m_VirtualCheckbox.Detach();
    m_TabCtrl.Detach();
    m_StatusBar.Detach();
    m_CmbJump.Detach();
    return 0;
}

LRESULT CDebugMemoryView::OnShowAddress(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    uint32_t address = (uint32_t)wParam;
    bool bVirtual = (lParam != 0);
    m_CmbJump.SetCurSel(GetJumpItemIndex(address, bVirtual));
    OpenNewTab(address, bVirtual, 4, true, true);
    return FALSE;
}

void CDebugMemoryView::OnExitSizeMove()
{
    SaveWindowPos(true);
}

LRESULT CDebugMemoryView::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND, BOOL& /*bHandled*/)
{
    switch (wID)
    {
    case IDC_CHK_VADDR:
        m_bVirtualMemory = (m_VirtualCheckbox.GetCheck() == BST_CHECKED);
        SetupJumpMenu(m_bVirtualMemory);
        m_CmbJump.SetCurSel(GetJumpItemIndex(m_MemAddr.GetValue(), m_bVirtualMemory));
        break;
    case IDC_SYMBOLS_BTN:
        m_Debugger->OpenSymbolsWindow();
        break;
    case IDCANCEL:
        EndDialog(0);
        break;
    case ID_POPUPMENU_TOGGLERBP:
        m_Breakpoints->RBPToggle(m_ContextMenuAddress);
        break;
    case ID_POPUPMENU_TOGGLEWBP:
        m_Breakpoints->WBPToggle(m_ContextMenuAddress);
        break;
    case ID_POPUPMENU_CLEARALLBPS:
        m_Breakpoints->RBPClear();
        m_Breakpoints->WBPClear();
        break;
    case ID_POPUPMENU_TOGGLELOCK:
        m_Breakpoints->ToggleMemLock(m_ContextMenuAddress);
        break;
    case ID_POPUPMENU_CLEARLOCKS:
        m_Breakpoints->ClearMemLocks();
        break;
    case ID_POPUPMENU_JUMPHERE:
        JumpToSelection();
        break;
    case ID_POPUPMENU_FOLLOWPOINTER:
        FollowPointer();
        break;
    case ID_POPUPMENU_VIEWDISASM:
        m_Debugger->Debug_ShowCommandsLocation(m_ContextMenuAddress, true);
        break;
    case ID_POPUPMENU_ADDSYMBOL:
        m_AddSymbolDlg.DoModal(m_Debugger, m_ContextMenuAddress);
        break;
    case ID_POPUPMENU_COPY:
        m_HexEditCtrl.Copy();
        break;
    case ID_POPUPMENU_COPYGAMESHARKCODE:
        {
        uint32_t startAddress, endAddress;
        m_HexEditCtrl.GetSelectionRange(&startAddress, &endAddress);
        CopyGameSharkCodeToClipboard(startAddress, endAddress);
        }
        break;
    case ID_POPUPMENU_COPYDATAWITHGROUPADDRESSES:
        {
        uint32_t startAddress, endAddress;
        m_HexEditCtrl.GetSelectionRange(&startAddress, &endAddress);
        CopyBytesToClipboard(startAddress, endAddress, m_HexEditCtrl.GetFocusedColumn() == HX_COL_HEXDATA, true, false);
        }
        break;
    case ID_POPUPMENU_COPYDATAWITHROWADDRESSES:
        {
        uint32_t startAddress, endAddress;
        m_HexEditCtrl.GetSelectionRange(&startAddress, &endAddress);
        CopyBytesToClipboard(startAddress, endAddress, m_HexEditCtrl.GetFocusedColumn() == HX_COL_HEXDATA, true, true);
        }
        break;
    case ID_POPUPMENU_PASTE:
        m_HexEditCtrl.Paste();
        break;
    case ID_POPUPMENU_SAFEMODE:
        m_HexEditCtrl.SendMessage(WM_KEYDOWN, VK_INSERT, 0);
        break;
    case ID_POPUPMENU_ZEROFILL:
        m_HexEditCtrl.SendMessage(WM_KEYDOWN, VK_DELETE, 0);
        break;
    case ID_POPUPMENU_BYTEGROUPSIZE_1:
        m_HexEditCtrl.SetByteGroupSize(1);
        break;
    case ID_POPUPMENU_BYTEGROUPSIZE_2:
        m_HexEditCtrl.SetByteGroupSize(2);
        break;
    case ID_POPUPMENU_BYTEGROUPSIZE_4:
        m_HexEditCtrl.SetByteGroupSize(4);
        break;
    case ID_POPUPMENU_BYTEGROUPSIZE_8:
        m_HexEditCtrl.SetByteGroupSize(8);
        break;
    case ID_POPUPMENU_DUMP:
        m_Debugger->OpenMemoryDump();
        break;
    case ID_POPUPMENU_SEARCH:
        m_Debugger->OpenMemorySearch();
        break;
    }
    return FALSE;
}

void CDebugMemoryView::OnAddrChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    if (m_bIgnoreAddressInput)
    {
        m_bIgnoreAddressInput = false;
        return;
    }

    uint32_t address = m_MemAddr.GetValue();
    m_HexEditCtrl.SetBaseAddress(address);
    UpdateCurrentTab(address);
    m_CmbJump.SetCurSel(GetJumpItemIndex(address, m_bVirtualMemory));
}

void CDebugMemoryView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
    if (pScrollBar != GetDlgItem(IDC_SCRL_BAR))
    {
        return;
    }

    uint32_t address = m_MemAddr.GetValue();
    int numBytesPerRow = m_HexEditCtrl.GetNumBytesPerRow();
    int numVisibleBytes = m_HexEditCtrl.GetNumVisibleBytes();

    switch (nSBCode)
    {
    case SB_LINEDOWN:
        m_MemAddr.SetValue(address < 0xFFFFFFEF ? address + numBytesPerRow : 0xFFFFFFFF, DisplayMode::ZeroExtend);
        break;
    case SB_LINEUP:
        m_MemAddr.SetValue(address > (uint32_t)numBytesPerRow ? address - numBytesPerRow : 0, DisplayMode::ZeroExtend);
        break;
    case SB_PAGEDOWN:
        m_MemAddr.SetValue(address < 0xFFFFFEFF ? address + numVisibleBytes : 0xFFFFFFFF, DisplayMode::ZeroExtend);
        break;
    case SB_PAGEUP:
        m_MemAddr.SetValue(address >(uint32_t)numVisibleBytes ? address - numVisibleBytes : 0, DisplayMode::ZeroExtend);
        break;
    case SB_THUMBPOSITION:
        m_MemAddr.SetValue((DWORD)nPos << 0x10, DisplayMode::ZeroExtend);
        break;
    default:
        break;
    }

    m_CmbJump.SetCurSel(GetJumpItemIndex(address, m_bVirtualMemory));
}

LRESULT CDebugMemoryView::OnHxCtrlKeyPressed(LPNMHDR lpNMHDR)
{
    NMHXCTRLKEYPRESSED* nmck = reinterpret_cast<NMHXCTRLKEYPRESSED*>(lpNMHDR);
    uint32_t address = m_HexEditCtrl.GetCaretAddress();

    if (nmck->nChar >= '1' && nmck->nChar <= '9')
    {
        int nBytes = nmck->nChar - '0';
        m_HexEditCtrl.SetByteGroupSize(nBytes);
        return FALSE;
    }

    switch (nmck->nChar)
    {
    case 'G':
        JumpToSelection();
        break;
    case 'W':
        m_Breakpoints->WBPToggle(address);
        break;
    case 'N':
        AddTab(0x80000000, true, 4);
        break;
    case 'R':
        m_Breakpoints->RBPToggle(address);
        break;
    case 'E':
        m_Breakpoints->ToggleMemLock(address);
        break;
    case 'Q':
        m_Breakpoints->WBPClear();
        m_Breakpoints->RBPClear();
        m_Breakpoints->ClearMemLocks();
        break;
    case 'F':
        // TODO: put selection in the textbox
        m_Debugger->OpenMemorySearch();
        break;
    case 'S':
        // TODO: set start and end address to selection
        m_Debugger->OpenMemoryDump();
        break;
    case 'T':
        OpenDuplicateTab();
        break;
    case 'Z':
        if (m_SafeEditQueue.size() != 0)
        {
            m_SafeEditQueue.pop_back();
        }
        break;
    case VK_F4:
        CloseCurrentTab();
        break;
    case VK_SPACE:
        FollowPointer(false);
        break;
    case VK_TAB:
        {
            int curSel = m_TabCtrl.GetCurSel();
            if (m_TabCtrl.SetCurSel(curSel + 1) == -1)
            {
                m_TabCtrl.SetCurSel(0);
            }
            TabSelChanged();
        }
        break;
    }

    return FALSE;
}

LRESULT CDebugMemoryView::OnHxSetNibble(LPNMHDR lpNMHDR)
{
    if (g_MMU == nullptr)
    {
        return FALSE;
    }

    NMHXSETNIBBLE* nmsn = reinterpret_cast<NMHXSETNIBBLE*>(lpNMHDR);

    uint8_t curValue;
    bool bValid = GetByte(nmsn->address, &curValue);

    if (!bValid)
    {
        return false;
    }

    uint8_t mask = (nmsn->bLoNibble ? 0xF0 : 0x0F);
    uint8_t newValue = (curValue & mask) | (nmsn->value << (nmsn->bLoNibble ? 0 : 4));

    if (nmsn->bInsert)
    {
        if (GetSafeEditValue(nmsn->address, &curValue))
        {
            newValue = (curValue & mask) | (nmsn->value << (nmsn->bLoNibble ? 0 : 4));
        }

        m_SafeEditQueue.push_back({ SE_FILL, nmsn->address, nmsn->address, newValue });
    }
    else
    {
        SetByte(nmsn->address, newValue);
    }

    return FALSE;
}

LRESULT CDebugMemoryView::OnHxSetByte(LPNMHDR lpNMHDR)
{
    NMHXSETBYTE* nmsb = reinterpret_cast<NMHXSETBYTE*>(lpNMHDR);

    if (g_MMU == nullptr)
    {
        return FALSE;
    }

    if (nmsb->bInsert)
    {
        m_SafeEditQueue.push_back({ SE_FILL, nmsb->address, nmsb->address, nmsb->value });
    }
    else
    {
        SetByte(nmsb->address, nmsb->value);
    }
    return FALSE;
}

LRESULT CDebugMemoryView::OnHxFillRange(LPNMHDR lpNMHDR)
{
    NMHXFILLRANGE* nmfr = reinterpret_cast<NMHXFILLRANGE*>(lpNMHDR);

    if (nmfr->bInsert)
    {
        m_SafeEditQueue.push_back({ SE_FILL, nmfr->startAddress, nmfr->endAddress, nmfr->value });
        return FALSE;
    }

    FillRange(nmfr->startAddress, nmfr->endAddress, nmfr->value);
    return FALSE;
}

LRESULT CDebugMemoryView::OnHxInsertModeChanged(LPNMHDR /*lpNMHDR*/)
{
    m_SafeEditQueue.clear();
    m_bSafeEditMode = m_HexEditCtrl.GetInsertMode();
    m_StatusBar.SetText(MEMSB_SAFEMODE, m_bSafeEditMode ? L"Safe mode" : L"");
    return FALSE;
}

LRESULT CDebugMemoryView::OnHxSelectionChanged(LPNMHDR /*lpNMHDR*/)
{
    uint32_t startAddress, endAddress;
    bool bHaveSelection = m_HexEditCtrl.GetSelectionRange(&startAddress, &endAddress);

    stdstr strBlock, strLength;

    if (bHaveSelection)
    {
        strBlock = stdstr_f("%08X:%08X", startAddress, endAddress);
        strLength = stdstr_f("%X", endAddress - startAddress + 1);
        m_StatusBar.SetText(MEMSB_BLOCK, strBlock.ToUTF16().c_str());
        m_StatusBar.SetText(MEMSB_BLOCKLEN, strLength.ToUTF16().c_str());
    }
    else
    {
        strBlock = stdstr_f("%08X", startAddress);
        m_StatusBar.SetText(MEMSB_BLOCK, strBlock.ToUTF16().c_str());
        m_StatusBar.SetText(MEMSB_BLOCKLEN, L"");
    }

    uint32_t romAddr, offset;
    DMALOGENTRY* entry = m_Debugger->DMALog()->GetEntryByRamAddress(startAddress, &romAddr, &offset);
    m_StatusBar.SetText(MEMSB_DMAINFO, entry != nullptr ? L"Have DMA" : L"");

    return FALSE;
}

LRESULT CDebugMemoryView::OnHxGroupSizeChanged(LPNMHDR /*lpNMHDR*/)
{
    int groupSize = m_HexEditCtrl.GetNumBytesPerGroup();

    int nItem = m_TabCtrl.GetCurSel();

    if (nItem == -1)
    {
        return FALSE;
    }

    m_TabData[nItem].numBytesPerGroup = groupSize;
    return FALSE;
}

LRESULT CDebugMemoryView::OnHxEnterPressed(LPNMHDR /*lpNMHDR*/)
{
    ApplySafeEdits();
    return FALSE;
}

LRESULT CDebugMemoryView::OnHxRedrawStarted(LPNMHDR /*lpNMHDR*/)
{
    m_SymbolColorPhase = 0;
    return FALSE;
}

LRESULT CDebugMemoryView::OnHxGetByteInfo(LPNMHDR lpNMHDR)
{
    NMHXGETBYTEINFO *nmgbi = reinterpret_cast<NMHXGETBYTEINFO*>(lpNMHDR);

    bool bHaveWriteTarget = false, bHaveReadTarget = false;
    uint32_t cpuReadWriteAddress = 0;
    int cpuReadWriteNumBytes = 0;

    if (g_Settings->LoadBool(Debugger_SteppingOps))
    {
        COpInfo opInfo(R4300iOp::m_Opcode);
        if (opInfo.IsStoreCommand())
        {
            cpuReadWriteAddress = opInfo.GetLoadStoreAddress();
            cpuReadWriteNumBytes = opInfo.NumBytesToStore();
            bHaveWriteTarget = true;
        }
        else if (opInfo.IsLoadCommand())
        {
            cpuReadWriteAddress = opInfo.GetLoadStoreAddress();
            cpuReadWriteNumBytes = opInfo.NumBytesToLoad();
            bHaveReadTarget = true;
        }
    }

    for (uint32_t i = 0; i < nmgbi->numBytes; i++)
    {
        uint32_t address = nmgbi->address + i;
        uint32_t paddress = address;
        HXBYTEINFO* oldByte = &nmgbi->oldBytes[i];
        HXBYTEINFO* newByte = &nmgbi->newBytes[i];

        newByte->bkColor = BKCOLOR_DEFAULT;
        newByte->color = COLOR_DEFAULT;

        if (m_bVirtualMemory && (g_MMU == nullptr || !g_MMU->TranslateVaddr(address, paddress)))
        {
            newByte->bValid = false;
            continue;
        }

        newByte->bValid = GetByte(address, &newByte->value);

        if (!newByte->bValid)
        {
            continue;
        }

        // Always use virtual addresses for breakpoint and symbol info
        // TODO: should be the other way around
        uint32_t vaddress = m_bVirtualMemory ? address : address + 0x80000000;

        CSymbol symbol;
        if (m_Debugger->SymbolTable()->GetSymbolByAddress(vaddress, &symbol))
        {
            m_SymbolColorStride = symbol.TypeSize();
            m_SymbolColorPhase = m_SymbolColorPhase ? 0 : 1;
        }

        if (bHaveWriteTarget && address == cpuReadWriteAddress)
        {
            m_WriteTargetColorStride = cpuReadWriteNumBytes;
        }
        else if (bHaveReadTarget && address == cpuReadWriteAddress)
        {
            m_ReadTargetColorStride = cpuReadWriteNumBytes;
        }

        bool bLocked = m_Breakpoints->MemLockExists(vaddress, 1);
        bool bReadBP = m_Breakpoints->ReadBPExists8(vaddress) == CBreakpoints::BP_SET;
        bool bWriteBP = m_Breakpoints->WriteBPExists8(vaddress) == CBreakpoints::BP_SET;

        if (bLocked)
        {
            newByte->bkColor = BKCOLOR_LOCKED;
            newByte->color = COLOR_BP;
        }
        else if (bReadBP && bWriteBP)
        {
            newByte->bkColor = BKCOLOR_RWBP;
            newByte->color = COLOR_BP;
        }
        else if (bReadBP)
        {
            newByte->bkColor = BKCOLOR_RBP;
            newByte->color = COLOR_BP;
        }
        else if (bWriteBP)
        {
            newByte->bkColor = BKCOLOR_WBP;
            newByte->color = COLOR_BP;
        }
        else if (m_ReadTargetColorStride > 0)
        {
            newByte->bkColor = BKCOLOR_CPUREAD;
        }
        else if (m_WriteTargetColorStride > 0)
        {
            newByte->bkColor = BKCOLOR_CPUWRITE;
        }
        else if (m_SymbolColorStride > 0)
        {
            newByte->bkColor = m_SymbolColorPhase ? BKCOLOR_SYMBOL0 : BKCOLOR_SYMBOL1;
        }

        if (g_Rom != nullptr && paddress >= 0x10000000 && paddress < 0x10000000 + g_Rom->GetRomSize())
        {
            newByte->color = COLOR_READONLY;
        }

        if (!nmgbi->bIgnoreDiff && oldByte->value != newByte->value)
        {
            newByte->color = COLOR_CHANGED;
        }

        if (m_SymbolColorStride > 0)
        {
            m_SymbolColorStride--;
        }

        if (m_ReadTargetColorStride > 0)
        {
            m_ReadTargetColorStride--;
        }

        if (m_WriteTargetColorStride > 0)
        {
            m_WriteTargetColorStride--;
        }

        uint8_t safeEditValue;
        if (GetSafeEditValue(address, &safeEditValue))
        {
            newByte->bValid = true;
            newByte->value = safeEditValue;
            newByte->bkColor = RGB(0xFF, 0xCC, 0xFF);
            newByte->color = RGB(0xFF, 0x00, 0xFF);
        }

        newByte->bHidden = false;
    }

    return FALSE;
}

LRESULT CDebugMemoryView::OnHxRightClick(LPNMHDR lpNMHDR)
{
    NMHXRCLICK *nmrc = reinterpret_cast<NMHXRCLICK*>(lpNMHDR);

    m_ContextMenuAddress = nmrc->address;

    HMENU hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_MEM_BP_POPUP));
    HMENU hPopupMenu = GetSubMenu(hMenu, 0);

    bool bHaveLock = m_Breakpoints->MemLockExists(m_ContextMenuAddress, 1);
    bool bHaveReadBP = (m_Breakpoints->ReadBPExists8(m_ContextMenuAddress) != CBreakpoints::BPSTATE::BP_NOT_SET);
    bool bHaveWriteBP = (m_Breakpoints->WriteBPExists8(m_ContextMenuAddress) != CBreakpoints::BPSTATE::BP_NOT_SET);

    if (m_Breakpoints->ReadMem().size() == 0 && m_Breakpoints->WriteMem().size() == 0)
    {
        EnableMenuItem(hPopupMenu, ID_POPUPMENU_CLEARALLBPS, MF_DISABLED | MF_GRAYED);
    }
    if (m_Breakpoints->NumMemLocks() == 0)
    {
        EnableMenuItem(hPopupMenu, ID_POPUPMENU_CLEARLOCKS, MF_DISABLED | MF_GRAYED);
    }

    CheckMenuItem(hPopupMenu, ID_POPUPMENU_TOGGLELOCK, bHaveLock ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hPopupMenu, ID_POPUPMENU_TOGGLERBP, bHaveReadBP ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hPopupMenu, ID_POPUPMENU_TOGGLEWBP, bHaveWriteBP ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hPopupMenu, ID_POPUPMENU_SAFEMODE, m_bSafeEditMode ? MF_CHECKED : MF_UNCHECKED);

    POINT mouse;
    GetCursorPos(&mouse);
    TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN, mouse.x, mouse.y, 0, m_hWnd, nullptr);
    DestroyMenu(hMenu);
    return FALSE;
}

LRESULT CDebugMemoryView::OnHxHotAddrChanged(LPNMHDR /*lpNMHDR*/)
{
    m_HotAddress = m_HexEditCtrl.GetHotAddress();
    stdstr strAddrInfo = "";

    CSymbol symbol;
    if (m_Debugger->SymbolTable()->GetSymbolByAddress(m_HotAddress, &symbol))
    {
        strAddrInfo += stdstr_f("%08X %s %s", symbol.m_Address, symbol.TypeName(), symbol.m_Name);
    }
    else
    {
        strAddrInfo += stdstr_f("%08X\n", m_HotAddress);
    }

    m_StatusBar.SetText(MEMSB_HOTADDR, strAddrInfo.ToUTF16().c_str());
    return FALSE;
}

LRESULT CDebugMemoryView::OnHxBaseAddrChanged(LPNMHDR /*lpNMHDR*/)
{
    // Address was updated from the control
    uint32_t address = m_HexEditCtrl.GetBaseAddress();
    m_bIgnoreAddressInput = true;
    m_MemAddr.SetValue(address, DisplayMode::ZeroExtend);
    m_CmbJump.SetCurSel(GetJumpItemIndex(address, m_bVirtualMemory));
    UpdateCurrentTab(address);
    return FALSE;
}

LRESULT CDebugMemoryView::OnHxCopy(LPNMHDR /*lpNMHDR*/)
{
    HXCOLUMN column = m_HexEditCtrl.GetFocusedColumn();
    uint32_t startAddress, endAddress;
    m_HexEditCtrl.GetSelectionRange(&startAddress, &endAddress);
    CopyBytesToClipboard(startAddress, endAddress, column == HX_COL_HEXDATA);
    return FALSE;
}

LRESULT CDebugMemoryView::OnHxPaste(LPNMHDR lpNMHDR)
{
    NMHXPASTE *nmp = reinterpret_cast<NMHXPASTE*>(lpNMHDR);

    if (g_MMU == nullptr)
    {
        return FALSE;
    }

    OpenClipboard();
    HANDLE hData = GetClipboardData(CF_TEXT);
    char* text = (char*)GlobalLock(hData);
    int retDataLength = 0;

    if (nmp->column == HX_COL_HEXDATA)
    {
        // TODO: move this function to some utility class
        int length = CMemoryScanner::ParseHexString(nullptr, text);

        if (length != 0)
        {
            std::unique_ptr<char[]> data = std::make_unique<char[]>(length);
            CMemoryScanner::ParseHexString(data.get(), text);

            for (int i = 0; i < length; i++)
            {
                SetByte(nmp->address + i, data.get()[i]);
            }
        }

        retDataLength = length;
    }
    else if (nmp->column == HX_COL_ASCII)
    {
        size_t length = strlen(text);
        for (size_t i = 0; i < length; i++)
        {
            SetByte(nmp->address + i, text[i]);
        }

        retDataLength = length;
    }

    GlobalUnlock(hData);
    CloseClipboard();

    return retDataLength;
}


LRESULT CDebugMemoryView::OnTabSelChange(LPNMHDR /*lpNMHDR*/)
{
    TabSelChanged();
    return FALSE;
}

void CDebugMemoryView::TabSelChanged(void)
{
    TCITEM item = { 0 };
    item.mask = TCIF_PARAM;
    
    int nItem = m_TabCtrl.GetCurSel();
    
    if (m_TabCtrl.GetItem(nItem, &item))
    {
        tab_info_t tabInfo = m_TabData[nItem];
        uint32_t address = tabInfo.address;
        
        m_MemAddr.SetValue(address, DisplayMode::ZeroExtend);

        if (m_bVirtualMemory != tabInfo.bVirtual)
        {
            m_bVirtualMemory = tabInfo.bVirtual;
            m_VirtualCheckbox.SetCheck(m_bVirtualMemory ? BST_CHECKED : BST_UNCHECKED);
            SetupJumpMenu(m_bVirtualMemory);
        }

        m_CmbJump.SetCurSel(GetJumpItemIndex(address, m_bVirtualMemory));
        m_HexEditCtrl.SetByteGroupSize(tabInfo.numBytesPerGroup);
    }
}

int CDebugMemoryView::AddTab(uint32_t address, bool bVirtual, int numBytesPerGroup)
{
    stdstr szAddress;
    szAddress.Format("%08X", address);
    m_TabData.push_back({ address, bVirtual, numBytesPerGroup });
    return m_TabCtrl.AddItem(TCIF_TEXT | TCIF_PARAM, szAddress.ToUTF16().c_str(), 0, (LPARAM)address);
}

int CDebugMemoryView::InsertTab(int nItem, uint32_t address, bool bVirtual, int numBytesPerGroup)
{
    m_TabData.insert(m_TabData.begin() + nItem + 1, { address, bVirtual, numBytesPerGroup });
    m_TabCtrl.SetRedraw(FALSE);
    m_TabCtrl.DeleteAllItems();
    for (size_t i = 0; i < m_TabData.size(); i++)
    {
        m_TabCtrl.AddItem(TCIF_TEXT, stdstr_f("%08X", m_TabData[i].address).ToUTF16().c_str(), 0, 0);
    }
    m_TabCtrl.SetRedraw(TRUE);
    return nItem + 1;
}

void CDebugMemoryView::DeleteTab(int nItem)
{
    m_TabData.erase(m_TabData.begin() + nItem);
    m_TabCtrl.DeleteItem(nItem);
}

void CDebugMemoryView::UpdateCurrentTab(uint32_t address)
{
    std::wstring szAddress = stdstr_f("%08X", address).ToUTF16();
    int nItem = m_TabCtrl.GetCurSel();
    
    if (nItem == -1)
    {
        return;
    }

    TCITEM item = { 0 };
    item.mask = TCIF_TEXT;
    item.pszText = (LPWSTR)szAddress.c_str();

    m_TabCtrl.SetRedraw(FALSE);
    m_TabCtrl.SetItem(nItem, &item);
    m_TabData[nItem].address = address;
    m_TabData[nItem].bVirtual = m_bVirtualMemory;
    m_TabCtrl.SetRedraw(TRUE);
}

void CDebugMemoryView::OpenNewTab(uint32_t address, bool bVirtual, int numBytesPerGroup, bool bInsert, bool bOpenExisting)
{
    int nItem;

    if (bOpenExisting)
    {
        for (size_t i = 0; i < m_TabData.size(); i++)
        {
            if (m_TabData[i].address == address && m_TabData[i].bVirtual == bVirtual)
            {
                m_TabCtrl.SetCurSel(i);
                TabSelChanged();
                return;
            }
        }
    }
    
    if (bInsert)
    {
        int nCurSelItem = m_TabCtrl.GetCurSel();
        nItem = InsertTab(nCurSelItem, address, bVirtual, numBytesPerGroup);
    }
    else
    {
        nItem = AddTab(address, bVirtual, numBytesPerGroup);
    }

    m_TabCtrl.SetCurSel(nItem);
    TabSelChanged();
}

void CDebugMemoryView::OpenDuplicateTab(void)
{
    int nItem = m_TabCtrl.GetCurSel();    
    tab_info_t tabInfo = m_TabData[nItem];
    int nItemNew = InsertTab(nItem, tabInfo.address, tabInfo.bVirtual, tabInfo.numBytesPerGroup);
    m_TabCtrl.SetCurSel(nItemNew);
    TabSelChanged();
}

void CDebugMemoryView::CloseTab(int nItem)
{
    int itemCount = m_TabCtrl.GetItemCount();
    int nSelItem = m_TabCtrl.GetCurSel();
    
    if (itemCount < 2 || nItem == -1)
    {
        return;
    }
    
    if (nItem == nSelItem)
    {
        if (nItem == m_TabCtrl.GetItemCount() - 1)
        {
            // Last tab
            m_TabCtrl.SetCurSel(nItem - 1);
        }
        else if (nItem == nSelItem)
        {
            m_TabCtrl.SetCurSel(nItem + 1);
        }

        TabSelChanged();
    }

    DeleteTab(nItem);    
}

void CDebugMemoryView::CloseCurrentTab(void)
{
    CloseTab(m_TabCtrl.GetCurSel());
}

LRESULT CDebugMemoryView::OnTabDblClick(LPNMHDR /*lpNMHDR*/)
{
    OpenDuplicateTab();
    return FALSE;
}

LRESULT CDebugMemoryView::OnTabRClick(LPNMHDR lpNMHDR)
{
    NMMTRCLICK *nmrc = reinterpret_cast<NMMTRCLICK*>(lpNMHDR);
    CloseTab(nmrc->nItem);
    return FALSE;
}

LRESULT CDebugMemoryView::OnStatusBarClick(LPNMHDR lpNMHDR)
{
    NMMOUSE *nmm = reinterpret_cast<NMMOUSE*>(lpNMHDR);

    uint32_t startAddress, endAddress;
    bool bHaveSelection = m_HexEditCtrl.GetSelectionRange(&startAddress, &endAddress);

    if (nmm->dwItemSpec == MEMSB_DMAINFO)
    {
        uint32_t romAddress, blockOffset;
        DMALOGENTRY* entry = m_Debugger->DMALog()->GetEntryByRamAddress(startAddress, &romAddress, &blockOffset);

        if (entry == nullptr)
        {
            return FALSE;
        }

        stdstr strDmaTitle = stdstr_f("DMA information for 0x%08X", startAddress);
        stdstr strDmaInfo = stdstr_f("Block:\nROM 0x%08X -> RAM 0x%08X ( 0x%X bytes )\n\nROM address of byte:\n0x%08X ( 0x%08X + 0x%08X )",
            entry->romAddr, entry->ramAddr, entry->length, romAddress, entry->romAddr, blockOffset);
        MessageBox(strDmaInfo.ToUTF16().c_str(), strDmaTitle.ToUTF16().c_str(), MB_OK);
    }
    else if (nmm->dwItemSpec == MEMSB_BLOCK)
    {
        stdstr strAddrRange;

        if (bHaveSelection)
        {
            strAddrRange = stdstr_f("%08X:%08X", startAddress, endAddress);
        }
        else
        {
            strAddrRange = stdstr_f("%08X", startAddress);
        }

        CopyTextToClipboard(strAddrRange.c_str());
    }

    return FALSE;
}

void CDebugMemoryView::OnJumpComboSelChange(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    int nItem = m_CmbJump.GetCurSel();
    uint32_t address;

    if (m_bVirtualMemory)
    {
        address = JumpItems[nItem].vaddr;
    }
    else
    {
        address = JumpItems[nItem].paddr;
    }
    
    m_MemAddr.SetValue(address, DisplayMode::ZeroExtend);
    m_HexEditCtrl.SetFocus();
}
