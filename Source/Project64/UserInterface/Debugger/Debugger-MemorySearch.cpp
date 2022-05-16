#include "stdafx.h"
#include "DebuggerUI.h"
#include "MemoryScanner.h"

CDebugMemorySearch* CDebugMemorySearch::_this = nullptr;
HHOOK CDebugMemorySearch::hWinMessageHook = nullptr;

const CSetValueDlg::ComboItem CDebugMemorySearch::ModalChangeTypeItems[] = {
    { "uint8",  ValueType_uint8 },
    { "int8",   ValueType_int8 },
    { "uint16", ValueType_uint16 },
    { "int16",  ValueType_int16 },
    { "uint32", ValueType_uint32 },
    { "int32",  ValueType_int32 },
    { "uint64", ValueType_uint64 },
    { "int64",  ValueType_int64 },
    { "float",  ValueType_float },
    { "double", ValueType_double },
    { "string", ValueType_string },
    { nullptr, 0 }
};

CDebugMemorySearch::CDebugMemorySearch(CDebuggerUI * debugger) :
    CDebugDialog<CDebugMemorySearch>(debugger)
{
}

CDebugMemorySearch::~CDebugMemorySearch()
{
}

LRESULT CDebugMemorySearch::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    DlgSavePos_Init(DebuggerUI_MemorySearchPos);
    DlgResize_Init(false, true);

    m_HexCheckbox.Attach(GetDlgItem(IDC_CHK_HEX));
    m_UnsignedCheckbox.Attach(GetDlgItem(IDC_CHK_UNSIGNED));
    m_IgnoreCaseCheckbox.Attach(GetDlgItem(IDC_CHK_IGNORECASE));
    m_UnkEncodingCheckbox.Attach(GetDlgItem(IDC_CHK_UNKENCODING));
    m_SearchValue.Attach(GetDlgItem(IDC_SEARCH_VALUE));
    m_SearchTypeOptions.Attach(GetDlgItem(IDC_CMB_SCANTYPE));
    m_ValueTypeOptions.Attach(GetDlgItem(IDC_CMB_VALUETYPE));
    m_AddrStart.Attach(GetDlgItem(IDC_ADDR_START));
    m_AddrEnd.Attach(GetDlgItem(IDC_ADDR_END));
    m_PhysicalCheckbox.Attach(GetDlgItem(IDC_CHK_PHYSICAL));
    m_ResultsListCtrl.Attach(GetDlgItem(IDC_LST_RESULTS));
    m_ResultsScrollbar.Attach(GetDlgItem(IDC_SCRL_RESULTS));
    m_WatchListCtrl.Attach(GetDlgItem(IDC_LST_WATCHLIST));
    m_WatchListScrollbar.Attach(GetDlgItem(IDC_SCRL_WATCHLIST));

    m_SearchValue.SetDisplayFormat(DisplayDefault);
    m_AddrEnd.SetDisplayType(CEditNumber32::DisplayHex);
    m_AddrStart.SetDisplayType(CEditNumber32::DisplayHex);

    UpdateOptions(); // Setup search type combo box

    CComboBox & vtcb = m_ValueTypeOptions;
    vtcb.SetItemData(vtcb.AddString(L"int8"), ValueType_int8);
    vtcb.SetItemData(vtcb.AddString(L"int16"), ValueType_int16);
    vtcb.SetItemData(vtcb.AddString(L"int32"), ValueType_int32);
    vtcb.SetItemData(vtcb.AddString(L"int64"), ValueType_int64);
    vtcb.SetItemData(vtcb.AddString(L"float"), ValueType_float);
    vtcb.SetItemData(vtcb.AddString(L"double"), ValueType_double);
    vtcb.SetItemData(vtcb.AddString(L"string"), ValueType_string);
    vtcb.SetCurSel(0);

    m_ResultsListCtrl.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_ResultsListCtrl.ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0);
    m_ResultsListCtrl.AddColumn(L"Address", ResultsListCtrl_Col_Address);
    m_ResultsListCtrl.AddColumn(L"Value", ResultsListCtrl_Col_Value);
    m_ResultsListCtrl.AddColumn(L"Previous", ResultsListCtrl_Col_Previous);
    m_ResultsListCtrl.SetColumnWidth(ResultsListCtrl_Col_Address, 80);
    m_ResultsListCtrl.SetColumnWidth(ResultsListCtrl_Col_Value, 80);
    m_ResultsListCtrl.SetColumnWidth(ResultsListCtrl_Col_Previous, 80);

    m_WatchListCtrl.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_WatchListCtrl.ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0);
    m_WatchListCtrl.AddColumn(L"Lock", WatchListCtrl_Col_Lock);
    m_WatchListCtrl.AddColumn(L"BP", WatchListCtrl_Col_BP);
    m_WatchListCtrl.AddColumn(L"Address", WatchListCtrl_Col_Address);
    m_WatchListCtrl.AddColumn(L"Description", WatchListCtrl_Col_Description);
    m_WatchListCtrl.AddColumn(L"Type", WatchListCtrl_Col_Type);
    m_WatchListCtrl.AddColumn(L"Value", WatchListCtrl_Col_Value);
    m_WatchListCtrl.SetColumnWidth(WatchListCtrl_Col_Lock, 35);
    m_WatchListCtrl.SetColumnWidth(WatchListCtrl_Col_BP, 35);
    m_WatchListCtrl.SetColumnWidth(WatchListCtrl_Col_Address, 80);
    m_WatchListCtrl.SetColumnWidth(WatchListCtrl_Col_Description, 160);
    m_WatchListCtrl.SetColumnWidth(WatchListCtrl_Col_Type, 50);
    m_WatchListCtrl.SetColumnWidth(WatchListCtrl_Col_Value, 90);

    m_bJalSelected = false;
    m_bJalHexWasChecked = false;
    m_bJalUnsignedWasChecked = false;

    m_bDraggingSeparator = false;
    ::GetWindowRect(GetDlgItem(IDC_SEPARATOR), &m_InitialSeparatorRect);
    ScreenToClient(&m_InitialSeparatorRect);

    uint32_t ramSize = (g_MMU != nullptr) ? g_MMU->RdramSize() : 0x00400000;

    m_AddrStart.SetValue(0x80000000, DisplayMode::AllHex);
    m_AddrEnd.SetValue(0x80000000 + ramSize - 1, DisplayMode::AllHex);

    FixListHeader(m_WatchListCtrl);
    FixListHeader(m_ResultsListCtrl);

    UpdateResultsList(true);
    UpdateWatchList();

    LoadWindowPos();
    WindowCreated();

    m_hCursorSizeNS = LoadCursor(nullptr, IDC_SIZENS);

    SetTimer(TIMER_ID_AUTO_REFRESH, 100, nullptr);

    _this = this;
    m_ThreadId = ::GetCurrentThreadId();
    hWinMessageHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)HookProc, nullptr, m_ThreadId);

    GameReset();

    return TRUE;
}

LRESULT CDebugMemorySearch::OnDestroy(void)
{
    UnhookWindowsHookEx(hWinMessageHook);
    KillTimer(TIMER_ID_AUTO_REFRESH);

    m_UnsignedCheckbox.Detach();
    m_IgnoreCaseCheckbox.Detach();
    m_UnkEncodingCheckbox.Detach();
    m_HexCheckbox.Detach();
    m_SearchValue.Detach();
    m_SearchTypeOptions.Detach();
    m_ValueTypeOptions.Detach();
    m_AddrStart.Detach();
    m_AddrEnd.Detach();
    m_PhysicalCheckbox.Detach();
    m_ResultsListCtrl.Detach();
    m_ResultsScrollbar.Detach();
    m_WatchListCtrl.Detach();
    m_WatchListScrollbar.Detach();
    return 0;
}

void CDebugMemorySearch::GameReset(void)
{
    if (m_hWnd == nullptr)
    {
        return;
    }

    SendMessage(WM_GAMERESET);
}

LRESULT CDebugMemorySearch::OnGameReset(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    ReloadWatchList();
    return FALSE;
}

void CDebugMemorySearch::ReloadWatchList(void)
{
    stdstr strGame = g_Settings->LoadStringVal(Game_GameName);

    FlushWatchList();

    if (m_StrGame != strGame)
    {
        m_StrGame = std::move(strGame);
        LoadWatchList();
    }
}

void CDebugMemorySearch::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_ID_AUTO_REFRESH)
    {
        RefreshResultsListValues();
        RefreshWatchListValues();
    }
}

LRESULT CALLBACK CDebugMemorySearch::HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    MSG *pMsg = (MSG*)lParam;

    switch (pMsg->message)
    {
    case WM_MOUSEWHEEL:
        _this->OnInterceptMouseWheel(pMsg->wParam, pMsg->lParam);
        break;
    case WM_MOUSEMOVE:
        _this->OnInterceptMouseMove(pMsg->wParam, pMsg->lParam);
        break;
    }

    if (nCode < 0)
    {
        return CallNextHookEx(hWinMessageHook, nCode, wParam, lParam);
    }

    return 0;
}

void CDebugMemorySearch::OnExitSizeMove(void)
{
    UpdateWatchList(true);
    SaveWindowPos(true);
}

void CDebugMemorySearch::OnSizing(UINT /*fwSide*/, LPRECT /*pRect*/)
{
    FixListHeader(m_WatchListCtrl);
    FixListHeader(m_ResultsListCtrl);
    SetMsgHandled(FALSE);
}

LRESULT CDebugMemorySearch::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    EndDialog(0);
    return FALSE;
}

LRESULT CDebugMemorySearch::OnHexCheckbox(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    bool bChecked = (m_HexCheckbox.GetCheck() == BST_CHECKED);
    m_SearchValue.SetDisplayFormat(bChecked ? DisplayHex : DisplayDefault);
    return FALSE;
}

LRESULT CDebugMemorySearch::OnSearchButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_SearchMemory);
    Search();
    // Emulator won't resume sometimes unless there's a sleep() here
    Sleep(50); // TODO: fix?
    g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_SearchMemory);
    return FALSE;
}

LRESULT CDebugMemorySearch::OnResetButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    ResetResults();
    return FALSE;
}

LRESULT CDebugMemorySearch::OnScanTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    SearchType searchType = (SearchType)m_SearchTypeOptions.GetItemData(m_SearchTypeOptions.GetCurSel());

    bool bDidFirstScan = m_MemoryScanner.DidFirstScan();

    if (searchType == SearchType_JalTo)
    {
        m_bJalSelected = true;
        m_SearchValue.EnableWindow(TRUE);

        SetComboBoxSelByData(m_ValueTypeOptions, ValueType_int32);
        m_ValueTypeOptions.EnableWindow(FALSE);

        // Remember checkbox states
        m_bJalHexWasChecked = (m_HexCheckbox.GetCheck() == BST_CHECKED);
        m_bJalUnsignedWasChecked = (m_UnsignedCheckbox.GetCheck() == BST_CHECKED);

        m_SearchValue.SetDisplayFormat(DisplayHex);

        m_HexCheckbox.SetCheck(BST_CHECKED);
        m_HexCheckbox.EnableWindow(FALSE);
        m_UnsignedCheckbox.SetCheck(BST_CHECKED);
        m_UnsignedCheckbox.EnableWindow(FALSE);
    }
    else if(m_bJalSelected)
    {
        m_bJalSelected = false;
        m_SearchValue.SetDisplayFormat(m_bJalHexWasChecked ? DisplayHex : DisplayDefault);
        m_HexCheckbox.SetCheck(m_bJalHexWasChecked ? BST_CHECKED : BST_UNCHECKED);
        m_HexCheckbox.EnableWindow(TRUE);
        m_UnsignedCheckbox.SetCheck(m_bJalUnsignedWasChecked ? BST_CHECKED : BST_UNCHECKED);
        m_UnsignedCheckbox.EnableWindow(TRUE);
    }

    switch (searchType)
    {
    case SearchType_JalTo:
        break;
    case SearchType_UnknownValue:
    case SearchType_IncreasedValue:
    case SearchType_DecreasedValue:
    case SearchType_ChangedValue:
    case SearchType_UnchangedValue:
        m_ValueTypeOptions.EnableWindow(!bDidFirstScan);
        m_SearchValue.EnableWindow(FALSE);
        break;
    default:
        m_ValueTypeOptions.EnableWindow(!bDidFirstScan);
        m_SearchValue.EnableWindow(TRUE);
        break;
    }

    return FALSE;
}

LRESULT CDebugMemorySearch::OnValueTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    ValueType valueType = (ValueType)m_ValueTypeOptions.GetItemData(m_ValueTypeOptions.GetCurSel());

    if (valueType == ValueType_string || valueType == ValueType_istring || valueType == ValueType_unkstring)
    {
        m_SearchValue.EnableWindow(TRUE);
        SetComboBoxSelByData(m_SearchTypeOptions, SearchType_ExactValue);
        m_SearchTypeOptions.EnableWindow(FALSE);

        m_UnkEncodingCheckbox.ShowWindow(SW_SHOW);
        m_IgnoreCaseCheckbox.ShowWindow(SW_SHOW);
        m_UnsignedCheckbox.ShowWindow(SW_HIDE);
    }
    else
    {
        bool bShowUnsigned = (valueType != ValueType_float && valueType != ValueType_double);
        m_UnsignedCheckbox.ShowWindow(bShowUnsigned ? SW_SHOW : SW_HIDE);

        m_UnkEncodingCheckbox.ShowWindow(SW_HIDE);
        m_IgnoreCaseCheckbox.ShowWindow(SW_HIDE);

        m_SearchTypeOptions.EnableWindow(TRUE);
    }

    return FALSE;
}

LRESULT CDebugMemorySearch::OnRdramButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    bool bPhysicalChecked = (m_PhysicalCheckbox.GetCheck() == BST_CHECKED);
    uint32_t addrStart = bPhysicalChecked ? 0 : 0x80000000;
    uint32_t ramSize = (g_MMU != nullptr) ? g_MMU->RdramSize() : 0x00400000;
    m_AddrStart.SetValue(addrStart, DisplayMode::AllHex);
    m_AddrEnd.SetValue(addrStart + ramSize - 1, DisplayMode::AllHex);
    return FALSE;
}

LRESULT CDebugMemorySearch::OnRomButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    bool bPhysicalChecked = (m_PhysicalCheckbox.GetCheck() == BST_CHECKED);
    uint32_t addrStart = bPhysicalChecked ? 0x10000000 : 0xB0000000;
    m_AddrStart.SetValue(addrStart, DisplayMode::AllHex);
    m_AddrEnd.SetValue(addrStart + g_Rom->GetRomSize() - 1, DisplayMode::AllHex);
    return FALSE;
}

LRESULT CDebugMemorySearch::OnSpmemButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    bool bPhysicalChecked = (m_PhysicalCheckbox.GetCheck() == BST_CHECKED);
    uint32_t addrStart = bPhysicalChecked ? 0x04000000 : 0xA4000000;
    m_AddrStart.SetValue(addrStart, DisplayMode::AllHex);
    m_AddrEnd.SetValue(addrStart + 0x1FFF, DisplayMode::AllHex);
    return FALSE;
}

LRESULT CDebugMemorySearch::OnResultsCustomDraw(LPNMHDR lpnmh)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(lpnmh);
    DWORD drawStage = pLVCD->nmcd.dwDrawStage;

    switch (drawStage)
    {
    case CDDS_PREPAINT: return (CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT);
    case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW;
    case (CDDS_ITEMPREPAINT | CDDS_SUBITEM): break;
    default: return CDRF_DODEFAULT;
    }

    uint32_t nItem = (uint32_t)pLVCD->nmcd.dwItemSpec;
    uint32_t nSubItem = pLVCD->iSubItem;
    size_t index = m_ResultsListCtrl.GetItemData(nItem);
    CScanResult *presult = m_MemoryScanner.GetResult(index);

    switch (nSubItem)
    {
    case ResultsListCtrl_Col_Address:
        if (presult->m_AddressType == AddressType_Physical)
        {
            // Green if address is physical
            pLVCD->clrText = RGB(0x44, 0x88, 0x44);
        }
        break;
    case ResultsListCtrl_Col_Value:
        {
            pLVCD->clrText = RGB(0, 0, 0);
            char szCurrentValue[1024], szOldValue[1024];
            presult->GetMemoryValueString(szCurrentValue, 1024);
            presult->GetValueString(szOldValue, 1024);

            if (presult->IsStringType())
            {
                pLVCD->clrText = RGB(0, 0, 0);
                if (presult->m_DisplayFormat == DisplayHex)
                {
                    // Blue if hex string
                    pLVCD->clrText = RGB(0, 0, 255);
                }
            }
            else if (strcmp(szCurrentValue, szOldValue) != 0)
            {
                // Red if value has changed
                pLVCD->clrText = RGB(255, 0, 0);
            }
        }
        break;
    case ResultsListCtrl_Col_Previous:
        pLVCD->clrText = RGB(0, 0, 0);
        break;
    }

    return CDRF_DODEFAULT;
}

LRESULT CDebugMemorySearch::OnResultsDblClick(LPNMHDR)
{
    LONG iItem = m_ResultsListCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iItem == -1)
    {
        return true;
    }

    // Copy result to watch list
    int index = m_ResultsListCtrl.GetItemData(iItem);
    AddResultToWatchList(index);

    UpdateWatchList(true);
    return true;
}

LRESULT CDebugMemorySearch::OnResultsRClick(LPNMHDR /*lpnmh*/)
{
    LONG iItem = m_ResultsListCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iItem == -1)
    {
        return true;
    }

    // Load the menu
    HMENU hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_MEM_SEARCH));
    HMENU hPopupMenu = GetSubMenu(hMenu, 0);

    // Get the current mouse location
    POINT Mouse;
    GetCursorPos(&Mouse);

    // Show the menu
    TrackPopupMenu(hPopupMenu, 0, Mouse.x, Mouse.y, 0, m_hWnd, nullptr);
    DestroyMenu(hMenu);
    return true;
}

LRESULT CDebugMemorySearch::OnResultsPopupViewMemory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* presult = GetFirstSelectedScanResult();

    if (presult == nullptr)
    {
        return FALSE;
    }

    bool bUseVaddr = (presult->m_AddressType == AddressType_Virtual);
    m_Debugger->Debug_ShowMemoryLocation(presult->m_Address, bUseVaddr);
    return FALSE;
}

LRESULT CDebugMemorySearch::OnResultsPopupAddToWatchList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    LONG iItem = -1;

    while ((iItem = m_ResultsListCtrl.GetNextItem(iItem, LVNI_SELECTED)) != -1)
    {
        int index = m_ResultsListCtrl.GetItemData(iItem);
        AddResultToWatchList(index);
    }

    UpdateWatchList(true);

    return FALSE;
}

LRESULT CDebugMemorySearch::OnResultsPopupAddAllToWatchList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    size_t numResults = m_MemoryScanner.GetNumResults();

    for (size_t i = 0; i < numResults; i++)
    {
        AddResultToWatchList(i);
    }

    UpdateWatchList(true);

    return FALSE;
}

LRESULT CDebugMemorySearch::OnResultsPopupSetValue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* pFirstResult = GetFirstSelectedScanResult();

    if (pFirstResult == nullptr)
    {
        return FALSE;
    }

    char szInitialValue[1024];
    pFirstResult->GetMemoryValueString(szInitialValue, 1024);

    if (m_SetValueDlg.DoModal("Change value", "New value:", szInitialValue))
    {
        int nItems = m_ResultsListCtrl.GetItemCount();

        for (int iItem = 0; iItem < nItems; iItem++)
        {
            bool bSelected = (m_ResultsListCtrl.GetItemState(iItem, LVNI_SELECTED) != 0);

            if (bSelected)
            {
                int index = m_ResultsListCtrl.GetItemData(iItem);
                CScanResult* presult = m_MemoryScanner.GetResult(index);
                const std::string & enteredString = m_SetValueDlg.GetEnteredString();
                presult->SetMemoryValueFromString(enteredString.c_str());
                m_ResultsListCtrl.SetItemText(iItem, ResultsListCtrl_Col_Value, stdstr(enteredString).ToUTF16().c_str());
            }
        }
    }
    return FALSE;
}

LRESULT CDebugMemorySearch::OnResultsPopupRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    int nItems = m_ResultsListCtrl.GetItemCount();

    for (int iItem = nItems - 1; iItem >= 0; iItem--)
    {
        bool bSelected = (m_ResultsListCtrl.GetItemState(iItem, LVNI_SELECTED) != 0);

        if (bSelected)
        {
            size_t index = m_ResultsListCtrl.GetItemData(iItem);
            m_MemoryScanner.RemoveResult(index);
        }
    }

    UpdateResultsList(true);

    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListItemChanged(LPNMHDR /*lpnmh*/)
{
    //LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lpnmh;
    //
    //bool bSelected = (pnmv->uNewState & LVNI_SELECTED) != 0;
    //
    //if (bSelected)
    //{
    //    //for (size_t i = 0; i < m_WatchList.size(); i++)
    //    //{
    //    //    m_WatchList[i].SetSelected(false);
    //    //}
    //
    //    int iItem = pnmv->iItem;
    //    int index = m_WatchListCtrl.GetItemData(iItem);
    //    CScanResult *presult = &m_WatchList[index];
    //    //presult->SetSelected(bSelected);
    //}

    return true;
}

LRESULT CDebugMemorySearch::OnWatchListCustomDraw(LPNMHDR lpnmh)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(lpnmh);
    DWORD drawStage = pLVCD->nmcd.dwDrawStage;

    switch (drawStage)
    {
    case CDDS_PREPAINT: return (CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT);
    case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW;
    case (CDDS_ITEMPREPAINT | CDDS_SUBITEM): break;
    default: return CDRF_DODEFAULT;
    }

    uint32_t nItem = (uint32_t)pLVCD->nmcd.dwItemSpec;
    uint32_t nSubItem = pLVCD->iSubItem;
    size_t index = m_WatchListCtrl.GetItemData(nItem);
    CScanResult *presult = &m_WatchList[index];

    switch (nSubItem)
    {
    case WatchListCtrl_Col_Address:
        if (presult->m_AddressType == AddressType_Physical)
        {
            // Green if address is physical
            pLVCD->clrText = RGB(0x44, 0x88, 0x44);
        }
        break;
    case WatchListCtrl_Col_Value:
        if (presult->IsStringType())
        {
            pLVCD->clrText = RGB(0, 0, 0);
            if (presult->m_DisplayFormat == DisplayHex)
            {
                // Blue if hex string
                pLVCD->clrText = RGB(0, 0, 255);
            }
        }
        break;
    default:
        pLVCD->clrText = RGB(0, 0, 0);
        break;
    }

    return CDRF_DODEFAULT;
}

LRESULT CDebugMemorySearch::OnWatchListDblClick(LPNMHDR /*lpnmh*/)
{
    LONG iItem = m_WatchListCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iItem == -1)
    {
        return true;
    }

    int index = m_WatchListCtrl.GetItemData(iItem);
    CScanResult *presult = &m_WatchList[index];

    int nSelectedCol = -1;

    // Hit test for column

    POINT mousePt;
    RECT listRect;
    GetCursorPos(&mousePt);
    m_WatchListCtrl.GetWindowRect(&listRect);

    int mouseX = mousePt.x - listRect.left;

    for (int nCol = 0, colX = 0; nCol < WatchListCtrl_Num_Columns; nCol++)
    {
        int colWidth = m_WatchListCtrl.GetColumnWidth(nCol);
        if (mouseX >= colX && mouseX <= colX + colWidth)
        {
            nSelectedCol = nCol;
            break;
        }
        colX += colWidth;
    }

    switch (nSelectedCol)
    {
    case WatchListCtrl_Col_Lock:
        m_Debugger->Breakpoints()->ToggleMemLock(presult->m_Address);
        break;
    case WatchListCtrl_Col_Address:
        {
            if (m_SetValueDlg.DoModal("Change address", "New address:", stdstr_f("0x%08X", presult->m_Address).c_str()))
            {
                uint32_t newAddr = strtoul(m_SetValueDlg.GetEnteredString().c_str(), nullptr, 0);
                if (presult->SetAddressSafe(newAddr))
                {
                    m_WatchListCtrl.SetItemText(iItem, WatchListCtrl_Col_Address, stdstr_f("0x%08X", newAddr).ToUTF16().c_str());
                }
            }
        }
        break;
    case WatchListCtrl_Col_Value:
        {
            char szInitialValue[1024];
            presult->GetMemoryValueString(szInitialValue, 1024);
            if (m_SetValueDlg.DoModal("Change value", "New value:", szInitialValue))
            {
                presult->SetMemoryValueFromString(m_SetValueDlg.GetEnteredString().c_str());
            }
        }
        break;
    case WatchListCtrl_Col_Description:
        {
            if (m_SetValueDlg.DoModal("Set description", "New description:", presult->GetDescription()))
            {
                stdstr EnteredString = m_SetValueDlg.GetEnteredString();
                presult->SetDescription(EnteredString.c_str());
                m_WatchListCtrl.SetItemText(iItem, WatchListCtrl_Col_Description, EnteredString.ToUTF16().c_str());
            }
        }
        break;
    case WatchListCtrl_Col_Type:
        {
            if (m_SetValueDlg.DoModal("Change type", "New type:", presult->GetType(), ModalChangeTypeItems))
            {
                ValueType t = (ValueType) m_SetValueDlg.GetEnteredData();
                presult->SetType(t);

                if (t == ValueType_string)
                {
                    if (m_SetValueDlg.DoModal("String length", "New string length:", stdstr_f("%d", presult->GetStrLength()).c_str()))
                    {
                        int length = atoi(m_SetValueDlg.GetEnteredString().c_str());
                        presult->SetStrLengthSafe(length);
                    }

                    m_WatchListCtrl.SetItemText(iItem, WatchListCtrl_Col_Type, stdstr_f("char[%d]", presult->GetStrLength()).ToUTF16().c_str());
                }
                else
                {
                    m_WatchListCtrl.SetItemText(iItem, WatchListCtrl_Col_Type, stdstr(presult->GetTypeName()).ToUTF16().c_str());
                }
            }
        }
        break;
    }

    return true;
}

LRESULT CDebugMemorySearch::OnWatchListRClick(LPNMHDR /*lpnmh*/)
{
    LONG iItem = m_WatchListCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iItem == -1)
    {
        return true;
    }

    int index = m_WatchListCtrl.GetItemData(iItem);
    CScanResult *presult = &m_WatchList[index];

    // Load the menu
    HMENU hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_MEM_WATCHLIST));
    HMENU hPopupMenu = GetSubMenu(hMenu, 0);

    // Get the current mouse location
    POINT Mouse;
    GetCursorPos(&Mouse);

    CBreakpoints* breakpoints = m_Debugger->Breakpoints();

    bool bHaveLock = breakpoints->MemLockExists(presult->m_Address, 1);
    bool bHaveReadBP = (breakpoints->ReadBPExists8(presult->m_Address) != CBreakpoints::BPSTATE::BP_NOT_SET);
    bool bHaveWriteBP = (breakpoints->WriteBPExists8(presult->m_Address) != CBreakpoints::BPSTATE::BP_NOT_SET);
    bool bHex = (presult->m_DisplayFormat == DisplayHex);

    CheckMenuItem(hPopupMenu, ID_WATCHLIST_LOCKVALUE, bHaveLock ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hPopupMenu, ID_WATCHLIST_READBP, bHaveReadBP ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hPopupMenu, ID_WATCHLIST_WRITEBP, bHaveWriteBP ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hPopupMenu, ID_WATCHLIST_HEXADECIMAL, bHex ? MF_CHECKED : MF_UNCHECKED);

    // Show the menu
    TrackPopupMenu(hPopupMenu, 0, Mouse.x, Mouse.y, 0, m_hWnd, nullptr);
    DestroyMenu(hMenu);
    return true;
}

LRESULT CDebugMemorySearch::OnWatchListPopupViewMemory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* presult = GetFirstSelectedWatchListResult();

    if (presult == nullptr)
    {
        return FALSE;
    }

    bool bUseVaddr = (presult->m_AddressType == AddressType_Virtual);
    m_Debugger->Debug_ShowMemoryLocation(presult->m_Address, bUseVaddr);
    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupLock(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    int nItems = m_WatchListCtrl.GetItemCount();
    for (int iItem = nItems - 1; iItem >= 0; iItem--)
    {
        bool bSelected = (m_WatchListCtrl.GetItemState(iItem, LVNI_SELECTED) != 0);

        if (bSelected)
        {
            int index = m_WatchListCtrl.GetItemData(iItem);
            CScanResult* presult = &m_WatchList[index];
            m_Debugger->Breakpoints()->ToggleMemLock(presult->m_Address);
        }
    }

    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupReadBP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* presult = GetFirstSelectedWatchListResult();

    if (presult == nullptr)
    {
        return FALSE;
    }

    m_Debugger->Breakpoints()->RBPToggle(presult->m_Address);
    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupWriteBP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* presult = GetFirstSelectedWatchListResult();

    if (presult == nullptr)
    {
        return FALSE;
    }

    m_Debugger->Breakpoints()->WBPToggle(presult->m_Address);
    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupAddSymbol(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* presult = GetFirstSelectedWatchListResult();

    if (presult == nullptr)
    {
        return FALSE;
    }

    // TODO: fix magic numbers
    int nSymType = 1;

    switch (presult->GetType())
    {
    case ValueType_uint8: nSymType = 2; break;
    case ValueType_uint16: nSymType = 3; break;
    case ValueType_uint32: nSymType = 4; break;
    case ValueType_uint64: nSymType = 5; break;
    case ValueType_int8: nSymType = 6; break;
    case ValueType_int16: nSymType = 7; break;
    case ValueType_int32: nSymType = 8; break;
    case ValueType_int64: nSymType = 9; break;
    case ValueType_float: nSymType = 10; break;
    case ValueType_double: nSymType = 11; break;
    }

    m_AddSymbolDlg.DoModal(m_Debugger, presult->GetVirtualAddress(), nSymType);

    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupHexadecimal(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    int nItems = m_WatchListCtrl.GetItemCount();
    for (int iItem = 0; iItem < nItems; iItem++)
    {
        bool bSelected = (m_WatchListCtrl.GetItemState(iItem, LVNI_SELECTED) != 0);

        if (bSelected)
        {
            int index = m_WatchListCtrl.GetItemData(iItem);
            CScanResult* presult = &m_WatchList[index];

            if (presult->m_DisplayFormat == DisplayDefault)
            {
                presult->m_DisplayFormat = DisplayHex;
            }
            else
            {
                presult->m_DisplayFormat = DisplayDefault;
            }
        }
    }

    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupChangeValue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* pFirstResult = GetFirstSelectedWatchListResult();

    if (pFirstResult == nullptr)
    {
        return FALSE;
    }

    char szPlaceholder[1024];
    pFirstResult->GetMemoryValueString(szPlaceholder, 1024);

    if (m_SetValueDlg.DoModal("Change value", "New value:", szPlaceholder))
    {
        int nItems = m_WatchListCtrl.GetItemCount();
        for (int iItem = nItems - 1; iItem >= 0; iItem--)
        {
            bool bSelected = (m_WatchListCtrl.GetItemState(iItem, LVNI_SELECTED) != 0);

            if (bSelected)
            {
                int index = m_WatchListCtrl.GetItemData(iItem);
                CScanResult* presult = &m_WatchList[index];
                // TODO: prompt for size change if string is too long
                presult->SetMemoryValueFromString(m_SetValueDlg.GetEnteredString().c_str());
                m_WatchListCtrl.SetItemText(iItem, WatchListCtrl_Col_Value, stdstr(m_SetValueDlg.GetEnteredString()).ToUTF16().c_str());
            }
        }
    }

    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupChangeDescription(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* pFirstResult = GetFirstSelectedWatchListResult();

    if (pFirstResult == nullptr)
    {
        return FALSE;
    }

    if (m_SetValueDlg.DoModal("Change description", "New description:", pFirstResult->GetDescription()))
    {
        int nItems = m_WatchListCtrl.GetItemCount();
        for (int iItem = 0; iItem < nItems; iItem++)
        {
            bool bSelected = (m_WatchListCtrl.GetItemState(iItem, LVNI_SELECTED) != 0);

            if (bSelected)
            {
                int index = m_WatchListCtrl.GetItemData(iItem);
                CScanResult* presult = &m_WatchList[index];
                stdstr description = m_SetValueDlg.GetEnteredString();
                presult->SetDescription(description.c_str());
                m_WatchListCtrl.SetItemText(iItem, WatchListCtrl_Col_Description, description.ToUTF16().c_str());
            }
        }
    }

    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupChangeType(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* pFirstResult = GetFirstSelectedWatchListResult();

    if (pFirstResult == nullptr)
    {
        return FALSE;
    }

    if (m_SetValueDlg.DoModal("Change type", "New type:", pFirstResult->GetType(), ModalChangeTypeItems))
    {
        ValueType t = (ValueType)m_SetValueDlg.GetEnteredData();
        int length = 0;

        if (t == ValueType_string)
        {
            if (m_SetValueDlg.DoModal("String length", "New string length:",
                stdstr_f("%d", pFirstResult->GetStrLength()).c_str()))
            {
                length = atoi(m_SetValueDlg.GetEnteredString().c_str());

                if (length <= 0)
                {
                    length = 1;
                }
            }
        }

        int nItems = m_WatchListCtrl.GetItemCount();
        for (int iItem = 0; iItem < nItems; iItem++)
        {
            bool bSelected = (m_WatchListCtrl.GetItemState(iItem, LVNI_SELECTED) != 0);

            if (bSelected)
            {
                int index = m_WatchListCtrl.GetItemData(iItem);
                CScanResult* presult = &m_WatchList[index];

                if (presult->IsStringType())
                {
                    presult->SetStrLengthSafe(length);

                    m_WatchListCtrl.SetItemText(iItem, WatchListCtrl_Col_Type,
                        stdstr_f("char[%d]", presult->GetStrLength()).ToUTF16().c_str());
                }
                else
                {
                    m_WatchListCtrl.SetItemText(iItem, WatchListCtrl_Col_Type, stdstr(presult->GetTypeName()).ToUTF16().c_str());
                }
            }
        }
    }

    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupChangeAddress(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* pFirstResult = GetFirstSelectedWatchListResult();

    if (pFirstResult == nullptr)
    {
        return FALSE;
    }

    if (m_SetValueDlg.DoModal("Change address", "New address:", stdstr_f("0x%08X", pFirstResult->m_Address).c_str()))
    {
        uint32_t newAddr = strtoul(m_SetValueDlg.GetEnteredString().c_str(), nullptr, 0);
        stdstr newAddrStr = stdstr_f("0x%08X", newAddr);

        int nItems = m_WatchListCtrl.GetItemCount();
        for (int iItem = 0; iItem < nItems; iItem++)
        {
            bool bSelected = (m_WatchListCtrl.GetItemState(iItem, LVNI_SELECTED) != 0);

            if (bSelected)
            {
                int index = m_WatchListCtrl.GetItemData(iItem);
                CScanResult* presult = &m_WatchList[index];
                presult->SetAddressSafe(newAddr);
                m_WatchListCtrl.SetItemText(iItem, WatchListCtrl_Col_Address, newAddrStr.ToUTF16().c_str());
            }
        }
    }

    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupChangeAddressBy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    CScanResult* pFirstResult = GetFirstSelectedWatchListResult();

    if (pFirstResult == nullptr)
    {
        return FALSE;
    }

    if (m_SetValueDlg.DoModal("Adjust address", "Address offset (+/-):", "0"))
    {
        int offset = atoi(m_SetValueDlg.GetEnteredString().c_str());

        int nItems = m_WatchListCtrl.GetItemCount();
        for (int iItem = 0; iItem < nItems; iItem++)
        {
            bool bSelected = (m_WatchListCtrl.GetItemState(iItem, LVNI_SELECTED) != 0);

            if (bSelected)
            {
                int index = m_WatchListCtrl.GetItemData(iItem);
                CScanResult* presult = &m_WatchList[index];
                uint32_t newAddr = presult->m_Address + offset;
                if (presult->SetAddressSafe(newAddr))
                {
                    stdstr newAddrStr = stdstr_f("0x%08X", presult->m_Address);
                    m_WatchListCtrl.SetItemText(iItem, WatchListCtrl_Col_Address, newAddrStr.ToUTF16().c_str());
                }
            }
        }
    }

    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    RemoveSelectedWatchListItems();
    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    ClearWatchList();
    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupCopyGamesharkCode(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    int numWatchListItems = m_WatchListCtrl.GetItemCount();

    stdstr strGSCode = "";

    for (int i = 0; i < numWatchListItems; i++)
    {
        if (m_WatchListCtrl.GetItemState(i, LVNI_SELECTED) == 0)
        {
            continue;
        }

        size_t index = m_WatchListCtrl.GetItemData(i);
        CScanResult* presult = &m_WatchList[index];

        uint32_t vaddr = presult->GetVirtualAddress();

        if (presult->IsStringType())
        {
            int length = presult->GetStrLength();

            char str[1024];
            presult->GetMemoryValueString(str, 1024, true);

            bool haveOddLength = (length & 1) != 0;
            int evenLength = length & ~1;

            if (length >= 2)
            {
                for (int j = 0; j < evenLength; j += 2)
                {
                    strGSCode += stdstr_f("%08X %02X%02X\n", (vaddr + j) | GS_TWOBYTE, (uint8_t)str[j], (uint8_t)str[j + 1]);
                }
            }

            if (haveOddLength)
            {
                strGSCode += stdstr_f("%08X 00%02X\n", (vaddr + length - 1), (uint8_t)str[length - 1]);
            }
        }
        else
        {
            CMixed value;
            presult->GetMemoryValue(&value);

            switch (presult->GetType())
            {
            case ValueType_uint8:
            case ValueType_int8:
                strGSCode += stdstr_f("%08X 00%02X\n", vaddr, value.m_Value._uint8);
                break;
            case ValueType_uint16:
            case ValueType_int16:
                strGSCode += stdstr_f("%08X %04X\n", vaddr | GS_TWOBYTE, value.m_Value._uint16);
                break;
            case ValueType_uint32:
            case ValueType_int32:
            case ValueType_float:
                strGSCode += stdstr_f("%08X %04X\n", (vaddr + 0) | GS_TWOBYTE, (uint16_t)(value.m_Value._uint32 >> 16));
                strGSCode += stdstr_f("%08X %04X\n", (vaddr + 2) | GS_TWOBYTE, (uint16_t)(value.m_Value._uint32));
                break;
            case ValueType_uint64:
            case ValueType_int64:
            case ValueType_double:
                strGSCode += stdstr_f("%08X %04X\n", (vaddr + 0) | GS_TWOBYTE, (uint16_t)(value.m_Value._uint64 >> 48));
                strGSCode += stdstr_f("%08X %04X\n", (vaddr + 2) | GS_TWOBYTE, (uint16_t)(value.m_Value._uint64 >> 32));
                strGSCode += stdstr_f("%08X %04X\n", (vaddr + 4) | GS_TWOBYTE, (uint16_t)(value.m_Value._uint64 >> 16));
                strGSCode += stdstr_f("%08X %04X\n", (vaddr + 6) | GS_TWOBYTE, (uint16_t)(value.m_Value._uint64));
                break;
            default:
                g_Notify->BreakPoint(__FILE__, __LINE__);
                break;
            }
        }
    }

    if (strGSCode.length() == 0)
    {
        return FALSE;
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strGSCode.length());
    strncpy((char*)GlobalLock(hMem), strGSCode.c_str(), strGSCode.length() - 1);
    GlobalUnlock(hMem);
    OpenClipboard();
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    return FALSE;
}

LRESULT CDebugMemorySearch::OnWatchListPopupCopyAddressAndDescription(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    int numWatchListItems = m_WatchListCtrl.GetItemCount();

    stdstr str = "";

    for (int i = 0; i < numWatchListItems; i++)
    {
        if (m_WatchListCtrl.GetItemState(i, LVNI_SELECTED) == 0)
        {
            continue;
        }

        size_t index = m_WatchListCtrl.GetItemData(i);
        CScanResult* presult = &m_WatchList[index];

        str += stdstr_f("%08X %s\n", presult->m_Address, presult->GetDescription());
    }

    if (str.length() == 0)
    {
        return FALSE;
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, str.length());

    strncpy((char*)GlobalLock(hMem), str.c_str(), str.length() - 1);
    GlobalUnlock(hMem);
    OpenClipboard();
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    return FALSE;
}

LRESULT CDebugMemorySearch::OnSetFont(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Set row height for the results list and watch list
    CClientDC dc(m_hWnd);
    dc.SelectFont((HFONT)wParam);
    TEXTMETRIC tm;
    dc.GetTextMetrics(&tm);
    m_ListCtrlRowHeight = tm.tmHeight + tm.tmExternalLeading;
    return FALSE;
}

LRESULT CDebugMemorySearch::OnMeasureItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    if (wParam == IDC_LST_RESULTS || wParam == IDC_LST_WATCHLIST)
    {
        MEASUREITEMSTRUCT* lpMeasureItem = (MEASUREITEMSTRUCT*)lParam;
        lpMeasureItem->itemHeight = m_ListCtrlRowHeight;
    }
    return FALSE;
}

LRESULT CDebugMemorySearch::OnScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    WORD type = LOWORD(wParam);
    HWND hScrollbar = (HWND)lParam;
    WORD scrlId = (WORD)::GetDlgCtrlID(hScrollbar);

    SCROLLINFO scrollInfo;
    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_ALL;

    ::GetScrollInfo(hScrollbar, SB_CTL, &scrollInfo);

    int newPos;

    switch (type)
    {
    case SB_LINEUP:
        newPos = scrollInfo.nPos - 1;
        break;
    case SB_LINEDOWN:
        newPos = scrollInfo.nPos + 1;
        break;
    case SB_THUMBTRACK:
        newPos = scrollInfo.nTrackPos;
        break;
    default:
        return 0;
    }

    ::SetScrollPos(hScrollbar, SB_CTL, newPos, TRUE);

    if (scrlId == IDC_SCRL_RESULTS)
    {
        UpdateResultsList();
    }
    else if (scrlId == IDC_SCRL_WATCHLIST)
    {
        UpdateWatchList();
    }

    return 0;
}

LRESULT CDebugMemorySearch::OnMouseDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (MouseHovering(IDC_SEPARATOR, 0, 1))
    {
        ::GetWindowRect(GetDlgItem(IDC_SEPARATOR), &m_LastSeparatorRect);
        ScreenToClient(&m_LastSeparatorRect);
        m_bDraggingSeparator = true;
    }
    return 0;
}

LRESULT CDebugMemorySearch::OnMouseUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (m_bDraggingSeparator)
    {
        UpdateResultsList(true, false);
    }

    m_bDraggingSeparator = false;
    return 0;
}

void CDebugMemorySearch::OnInterceptMouseMove(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (MouseHovering(IDC_SEPARATOR, 0, 1) || m_bDraggingSeparator)
    {
        SetCursor(m_hCursorSizeNS);
    }

    CPoint cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(&cursorPos);

    if (m_bDraggingSeparator && cursorPos.y >= m_InitialSeparatorRect.top)
    {
        CRect sepRect, windowRect;
        int yChange = cursorPos.y - m_LastSeparatorRect.top;

        // Move separator
        HWND hSeparator = GetDlgItem(IDC_SEPARATOR);
        ::GetWindowRect(hSeparator, &sepRect);
        ScreenToClient(&sepRect);
        ::SetWindowPos(hSeparator, nullptr, sepRect.left, cursorPos.y, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER);
        ::InvalidateRect(hSeparator, nullptr, true);

        // Move and resize controls
        SeparatorMoveCtrl(IDC_LST_WATCHLIST, yChange, false);
        SeparatorMoveCtrl(IDC_SCRL_WATCHLIST, yChange, false);
        SeparatorMoveCtrl(IDC_NUM_RESULTS, yChange, true);
        SeparatorMoveCtrl(IDC_LST_RESULTS, yChange, true);
        SeparatorMoveCtrl(IDC_SCRL_RESULTS, yChange, true);

        // Adjust window height
        GetWindowRect(&windowRect);
        windowRect.bottom += yChange;
        SetWindowPos(HWND_TOP, &windowRect, SWP_NOMOVE);

        // Save separator position
        ::GetWindowRect(hSeparator, &m_LastSeparatorRect);
        ScreenToClient(&m_LastSeparatorRect);
    }
}

void CDebugMemorySearch::OnInterceptMouseWheel(WPARAM wParam, LPARAM /*lParam*/)
{
    int nScroll = -((short)HIWORD(wParam) / WHEEL_DELTA);

    if (MouseHovering(IDC_LST_RESULTS) || MouseHovering(IDC_SCRL_RESULTS))
    {
        // Scroll results list
        int scrollPos = m_ResultsScrollbar.GetScrollPos();
        m_ResultsScrollbar.SetScrollPos(scrollPos + nScroll);
        UpdateResultsList();
    }
    else if (MouseHovering(IDC_LST_WATCHLIST) || MouseHovering(IDC_SCRL_WATCHLIST))
    {
        // Scroll watch list
        int scrollPos = m_WatchListScrollbar.GetScrollPos();
        m_WatchListScrollbar.SetScrollPos(scrollPos + nScroll);
        UpdateWatchList();
    }
}

// Utility

void CDebugMemorySearch::ClearWatchList(void)
{
    for (size_t i = 0; i < m_WatchList.size(); i++)
    {
        m_WatchList[i].DeleteDescription();
    }

    m_WatchList.clear();
    UpdateWatchList(true);
}

void CDebugMemorySearch::RemoveWatchListItem(int index)
{
    m_WatchList[index].DeleteDescription();
    m_WatchList.erase(m_WatchList.begin() + index);
}

void CDebugMemorySearch::RemoveSelectedWatchListItems(void)
{
    int nItems = m_WatchListCtrl.GetItemCount();
    for (int iItem = nItems - 1; iItem >= 0; iItem--)
    {
        bool bSelected = (m_WatchListCtrl.GetItemState(iItem, LVNI_SELECTED) != 0);

        if (bSelected)
        {
            int index = m_WatchListCtrl.GetItemData(iItem);
            RemoveWatchListItem(index);
        }
    }

    UpdateWatchList();
}

void CDebugMemorySearch::AddResultToWatchList(int resultIndex)
{
    CScanResult result = *m_MemoryScanner.GetResult(resultIndex);
    m_WatchList.push_back(result);
}

CScanResult* CDebugMemorySearch::GetFirstSelectedScanResult(void)
{
    LONG iItem = m_ResultsListCtrl.GetNextItem(-1, LVNI_SELECTED);

    if (iItem == -1)
    {
        return nullptr;
    }

    int index = m_ResultsListCtrl.GetItemData(iItem);
    CScanResult *presult = m_MemoryScanner.GetResult(index);
    return presult;
}

CScanResult* CDebugMemorySearch::GetFirstSelectedWatchListResult(void)
{
    LONG iItem = m_WatchListCtrl.GetNextItem(-1, LVNI_SELECTED);

    if (iItem == -1)
    {
        return nullptr;
    }

    int index = m_WatchListCtrl.GetItemData(iItem);
    CScanResult *presult = &m_WatchList[index];
    return presult;
}

void CDebugMemorySearch::SetComboBoxSelByData(CComboBox& cb, DWORD_PTR data)
{
    int numOptions = cb.GetCount();

    for (int i = 0; i < numOptions; i++)
    {
        if (cb.GetItemData(i) == data)
        {
            cb.SetCurSel(i);
            break;
        }
    }
}

bool CDebugMemorySearch::MouseHovering(WORD ctrlId, int hMargin, int vMargin)
{
    CRect ctrlRect;
    POINT pointerPos;

    ::GetCursorPos(&pointerPos);

    HWND hWnd = WindowFromPoint(pointerPos);

    if (hWnd != m_hWnd && ::GetDlgCtrlID(hWnd) != ctrlId)
    {
        return false;
    }

    ::GetWindowRect(GetDlgItem(ctrlId), &ctrlRect);

    return (
        pointerPos.x >= ctrlRect.left - hMargin &&
        pointerPos.x <= ctrlRect.right + hMargin &&
        pointerPos.y >= ctrlRect.top - vMargin &&
        pointerPos.y <= ctrlRect.bottom + vMargin);
}

void CDebugMemorySearch::SeparatorMoveCtrl(WORD ctrlId, int yChange, bool bResize)
{
    CRect rect;
    HWND hControl = GetDlgItem(ctrlId);
    ::GetWindowRect(hControl, &rect);
    ScreenToClient(&rect);

    if (bResize) // Resize control
    {
        ::SetWindowPos(hControl, nullptr, 0, 0, rect.Width(), rect.Height() + yChange,
            SWP_NOMOVE | SWP_NOZORDER);
    }
    else // Move control
    {
        ::SetWindowPos(hControl, nullptr, rect.left, rect.top + yChange, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER);
    }

    ::InvalidateRect(hControl, nullptr, true);
}

int CDebugMemorySearch::GetNumVisibleRows(CListViewCtrl& list)
{
    CHeaderCtrl header = list.GetHeader();
    CRect listRect, headRect;
    list.GetWindowRect(&listRect);
    header.GetWindowRect(&headRect);
    int innerHeight = listRect.Height() - headRect.Height();
    return (innerHeight / m_ListCtrlRowHeight);
}

void CDebugMemorySearch::ResetResults(void)
{
    ::SetWindowTextA(GetDlgItem(IDC_NUM_RESULTS), "Results");
    m_MemoryScanner.Reset();
    UpdateResultsList(true);
    UpdateOptions();
}

void CDebugMemorySearch::Search(void)
{
    ValueType valueType = (ValueType) m_ValueTypeOptions.GetItemData(m_ValueTypeOptions.GetCurSel());
    SearchType searchType = (SearchType) m_SearchTypeOptions.GetItemData(m_SearchTypeOptions.GetCurSel());
    DWORD startAddress = m_AddrStart.GetValue();
    DWORD endAddress = m_AddrEnd.GetValue();

    bool bHexChecked = (m_HexCheckbox.GetCheck() == BST_CHECKED);
    bool bPhysicalChecked = (m_PhysicalCheckbox.GetCheck() == BST_CHECKED);

    MixedValue value;
    int stringValueLength;
    bool bUseSearchValue;

    if (valueType == ValueType_string)
    {
        if (m_UnkEncodingCheckbox.GetCheck() == BST_CHECKED)
        {
            valueType = ValueType_unkstring;
        }
        else if (m_IgnoreCaseCheckbox.GetCheck() == BST_CHECKED)
        {
            valueType = ValueType_istring;
        }
    }
    else if (m_UnsignedCheckbox.GetCheck() == BST_CHECKED)
    {
        switch (valueType)
        {
        case ValueType_int8:  valueType = ValueType_uint8; break;
        case ValueType_int16: valueType = ValueType_uint16; break;
        case ValueType_int32: valueType = ValueType_uint32; break;
        case ValueType_int64: valueType = ValueType_uint64; break;
        }
    }

    switch (searchType)
    {
    case SearchType_UnknownValue:
    case SearchType_ChangedValue:
    case SearchType_UnchangedValue:
    case SearchType_IncreasedValue:
    case SearchType_DecreasedValue:
        bUseSearchValue = false;
        break;
    default:
        bUseSearchValue = true;
        m_SearchValue.SetType(valueType);
        break;
    }

    m_MemoryScanner.SetSearchType(searchType);
    m_MemoryScanner.SetValueType(valueType);

    if (bUseSearchValue)
    {
        switch (valueType)
        {
        case ValueType_uint8:
            if (!m_SearchValue.GetValue(value._uint8))
            {
                goto value_parse_error;
            }
            m_MemoryScanner.SetValue<uint8_t>(value._uint8);
            break;
        case ValueType_int8:
            if (!m_SearchValue.GetValue(value._sint8))
            {
                goto value_parse_error;
            }
            m_MemoryScanner.SetValue<int8_t>(value._sint8);
            break;
        case ValueType_uint16:
            if (!m_SearchValue.GetValue(value._uint16))
            {
                goto value_parse_error;
            }
            m_MemoryScanner.SetValue<uint16_t>(value._uint16);
            break;
        case ValueType_int16:
            if (!m_SearchValue.GetValue(value._sint16))
            {
                goto value_parse_error;
            }
            m_MemoryScanner.SetValue<int16_t>(value._sint16);
            break;
        case ValueType_uint32:
            if (!m_SearchValue.GetValue(value._uint32))
            {
                goto value_parse_error;
            }
            m_MemoryScanner.SetValue<uint32_t>(value._uint32);
            break;
        case ValueType_int32:
            if (!m_SearchValue.GetValue(value._sint32))
            {
                goto value_parse_error;
            }
            m_MemoryScanner.SetValue<int32_t>(value._sint32);
            break;
        case ValueType_uint64:
            if (!m_SearchValue.GetValue(value._uint64))
            {
                goto value_parse_error;
            }
            m_MemoryScanner.SetValue<uint64_t>(value._uint64);
            break;
        case ValueType_int64:
            if (!m_SearchValue.GetValue(value._sint64))
            {
                goto value_parse_error;
            }
            m_MemoryScanner.SetValue<int64_t>(value._sint64);
            break;
        case ValueType_float:
            if (!m_SearchValue.GetValue(value._float))
            {
                goto value_parse_error;
            }
            m_MemoryScanner.SetValue<float>(value._float);
            break;
        case ValueType_double:
            if (!m_SearchValue.GetValue(value._double))
            {
                goto value_parse_error;
            }
            m_MemoryScanner.SetValue<double>(value._double);
            break;
        case ValueType_string:
            if (bHexChecked)
            {
                if (!m_SearchValue.GetValueHexString(value._string, stringValueLength))
                {
                    goto value_parse_error;
                }
            }
            else
            {
                if (!m_SearchValue.GetValueString(value._string, stringValueLength))
                {
                    goto value_parse_error;
                }
            }
            m_MemoryScanner.SetValue<const wchar_t*>(value._string);
            m_MemoryScanner.SetStringValueLength(stringValueLength);
            break;
        case ValueType_unkstring:
        case ValueType_istring:
            if (!m_SearchValue.GetValueString(value._string, stringValueLength))
            {
                goto value_parse_error;
            }

            m_MemoryScanner.SetValue<const wchar_t*>(value._string);
            m_MemoryScanner.SetStringValueLength(stringValueLength);
            break;
        default:
            MessageBox(L"Unimplemented value type", L"Unimplemented", MB_OK);
            return;
        }
    }

    if (!m_MemoryScanner.DidFirstScan())
    {
        m_MemoryScanner.SetAddressType(bPhysicalChecked ? AddressType_Physical : AddressType_Virtual);
        bool bAddressRangeValid = m_MemoryScanner.SetAddressRange(startAddress, endAddress);

        if (!bAddressRangeValid)
        {
            MessageBox(L"Invalid address range", L"Invalid address range");
            return;
        }

        m_MemoryScanner.FirstScan(m_SearchValue.GetDisplayFormat());
        UpdateOptions();
    }
    else
    {
        m_MemoryScanner.NextScan();

        if (m_MemoryScanner.GetNumResults() == 0)
        {
            UpdateOptions();
        }
    }

    UpdateResultsList(true);
    return;

value_parse_error:
    MessageBox(L"Invalid value", L"Invalid value", MB_OK);
    return;
}

void CDebugMemorySearch::UpdateOptions(void)
{
    bool bDidFirstScan = m_MemoryScanner.DidFirstScan();
    bool bHaveResults = (m_MemoryScanner.GetNumResults() > 0);

    m_ValueTypeOptions.EnableWindow(!bDidFirstScan);
    ::EnableWindow(GetDlgItem(IDC_BTN_RESET), bDidFirstScan);
    ::EnableWindow(GetDlgItem(IDC_BTN_RDRAM), !bDidFirstScan);
    ::EnableWindow(GetDlgItem(IDC_BTN_ROM), !bDidFirstScan);
    ::EnableWindow(GetDlgItem(IDC_BTN_SPMEM), !bDidFirstScan);
    m_PhysicalCheckbox.EnableWindow(!bDidFirstScan);
    m_AddrStart.EnableWindow(!bDidFirstScan);
    m_AddrEnd.EnableWindow(!bDidFirstScan);

    m_UnsignedCheckbox.EnableWindow(!bDidFirstScan);
    m_IgnoreCaseCheckbox.EnableWindow(!bDidFirstScan);
    m_UnkEncodingCheckbox.EnableWindow(!bDidFirstScan);

    SearchType searchType = (SearchType)m_SearchTypeOptions.GetItemData(m_SearchTypeOptions.GetCurSel());
    ValueType valueType = (ValueType)m_ValueTypeOptions.GetItemData(m_ValueTypeOptions.GetCurSel());

    if (!bDidFirstScan)
    {
        m_SearchTypeOptions.EnableWindow(TRUE);

        CComboBox & cb = m_SearchTypeOptions;
        cb.ResetContent();
        cb.SetItemData(cb.AddString(L"Exact value"), SearchType_ExactValue);
        cb.SetItemData(cb.AddString(L"Unknown initial value"), SearchType_UnknownValue);
        cb.SetItemData(cb.AddString(L"Greater than..."), SearchType_GreaterThanValue);
        cb.SetItemData(cb.AddString(L"Less than..."), SearchType_LessThanValue);
        cb.SetItemData(cb.AddString(L"JAL to..."), SearchType_JalTo);
        cb.SetCurSel(0);

        if (valueType == ValueType_string)
        {
            m_SearchTypeOptions.EnableWindow(false);
        }

        m_SearchValue.EnableWindow(TRUE);

        ::EnableWindow(GetDlgItem(IDC_BTN_SEARCH), true);
    }
    else
    {
        CComboBox & cb = m_SearchTypeOptions;
        cb.ResetContent();
        cb.SetItemData(cb.AddString(L"Exact value"), SearchType_ExactValue);
        cb.SetItemData(cb.AddString(L"Changed value"), SearchType_ChangedValue);
        cb.SetItemData(cb.AddString(L"Unchanged value"), SearchType_UnchangedValue);
        cb.SetItemData(cb.AddString(L"Greater than..."), SearchType_GreaterThanValue);
        cb.SetItemData(cb.AddString(L"Less than..."), SearchType_LessThanValue);
        cb.SetItemData(cb.AddString(L"Increased value"), SearchType_IncreasedValue);
        cb.SetItemData(cb.AddString(L"Decreased value"), SearchType_DecreasedValue);
        cb.SetCurSel(0);

        if (m_bJalSelected)
        {
            m_HexCheckbox.SetCheck(m_bJalHexWasChecked ? BST_CHECKED : BST_UNCHECKED);
            m_UnsignedCheckbox.SetCheck(m_bJalUnsignedWasChecked ? BST_CHECKED : BST_UNCHECKED);
            m_bJalSelected = false;
            m_bJalHexWasChecked = false;
            m_bJalUnsignedWasChecked = false;
        }

        m_HexCheckbox.EnableWindow(TRUE);
        m_UnsignedCheckbox.EnableWindow(TRUE);

        m_SearchValue.EnableWindow(TRUE);

        if (searchType == SearchType_JalTo ||
            valueType == ValueType_string ||
            valueType == ValueType_istring ||
            valueType == ValueType_unkstring)
        {
            // Complex search types, disable next search
            ::EnableWindow(GetDlgItem(IDC_BTN_SEARCH), false);
            m_SearchTypeOptions.EnableWindow(FALSE);
        }
        else
        {
            m_UnsignedCheckbox.ShowWindow(SW_SHOW);
            ::EnableWindow(GetDlgItem(IDC_BTN_SEARCH), bHaveResults);
            m_SearchTypeOptions.EnableWindow(bHaveResults);
        }
    }
}

void CDebugMemorySearch::UpdateResultsList(bool bUpdateScrollbar, bool bResetScrollPos)
{
    size_t numResults = m_MemoryScanner.GetNumResults();
    size_t numVisibleRows = GetNumVisibleRows(m_ResultsListCtrl);

    if (bUpdateScrollbar)
    {
        bool bCanDisplayAll = (numVisibleRows >= numResults);
        int scrollRangeMax = bCanDisplayAll ? 0 : numResults - numVisibleRows;

        m_ResultsScrollbar.EnableWindow(!bCanDisplayAll);
        m_ResultsScrollbar.SetScrollRange(0, scrollRangeMax, false);

        if (bResetScrollPos)
        {
            m_ResultsScrollbar.SetScrollPos(0, true);
        }
    }

    size_t start = m_ResultsScrollbar.GetScrollPos();
    size_t end = start + numVisibleRows;

    if (end > numResults)
    {
        end = numResults;
    }

    m_ResultsListCtrl.SetRedraw(FALSE);
    m_ResultsListCtrl.DeleteAllItems();

    int nItem = 0;

    for (size_t index = start; index < end; index++)
    {
        char szAddress[32];
        char szCurrentValue[1024];
        char szValue[1024];

        CScanResult *presult = m_MemoryScanner.GetResult(index);

        presult->GetAddressString(szAddress);
        presult->GetMemoryValueString(szCurrentValue, 1024);
        presult->GetValueString(szValue, 1024);

        m_ResultsListCtrl.AddItem(nItem, ResultsListCtrl_Col_Address, stdstr(szAddress).ToUTF16().c_str());
        m_ResultsListCtrl.SetItemText(nItem, ResultsListCtrl_Col_Value, stdstr(szCurrentValue).ToUTF16().c_str());
        m_ResultsListCtrl.SetItemText(nItem, ResultsListCtrl_Col_Previous, stdstr(szValue).ToUTF16().c_str());
        m_ResultsListCtrl.SetItemData(nItem, index);

        nItem++;
    }

    m_ResultsListCtrl.SetRedraw(TRUE);

    char szNumResults[32];
    sprintf(szNumResults, "Results (%d)", numResults);
    ::SetWindowTextA(GetDlgItem(IDC_NUM_RESULTS), szNumResults);
}

void CDebugMemorySearch::UpdateWatchList(bool bUpdateScrollbar)
{
    size_t numEntries = m_WatchList.size();
    size_t numVisibleRows = GetNumVisibleRows(m_WatchListCtrl);

    if (bUpdateScrollbar)
    {
        bool bCanDisplayAll = (numVisibleRows >= numEntries);
        int scrollRangeMax = bCanDisplayAll ? 0 : numEntries - numVisibleRows;

        m_WatchListScrollbar.SetScrollRange(0, scrollRangeMax, false);
        m_WatchListScrollbar.SetScrollPos(0, true);
        m_WatchListScrollbar.EnableWindow(!bCanDisplayAll);
    }

    size_t start = m_WatchListScrollbar.GetScrollPos();
    size_t end = start + numVisibleRows;

    if (end > numEntries)
    {
        end = numEntries;
    }

    m_WatchListCtrl.SetRedraw(FALSE);
    m_WatchListCtrl.DeleteAllItems();

    int nItem = 0;

    for (size_t index = start; index < end; index++)
    {
        CScanResult *presult = &m_WatchList[index];
        char szAddress[16];
        char szValue[1024];
        char szValueType[32];
        const char* pSzValueType;
        const char* pSzDescription;

        presult->GetAddressString(szAddress);
        presult->GetMemoryValueString(szValue, 1024);
        pSzValueType = presult->GetTypeName();
        pSzDescription = presult->GetDescription();

        switch(presult->GetType())
        {
        case ValueType_string:
        case ValueType_istring:
        case ValueType_unkstring:
            sprintf(szValueType, "%s[%d]", pSzValueType, presult->GetStrLength());
            pSzValueType = szValueType;
            break;
        }

        m_WatchListCtrl.AddItem(nItem, WatchListCtrl_Col_Lock, L"");
        m_WatchListCtrl.SetItemText(nItem, WatchListCtrl_Col_BP, L"");
        m_WatchListCtrl.SetItemText(nItem, WatchListCtrl_Col_Address, stdstr(szAddress).ToUTF16().c_str());
        m_WatchListCtrl.SetItemText(nItem, WatchListCtrl_Col_Description, stdstr(pSzDescription).ToUTF16().c_str());
        m_WatchListCtrl.SetItemText(nItem, WatchListCtrl_Col_Type, stdstr(pSzValueType).ToUTF16().c_str());
        m_WatchListCtrl.SetItemText(nItem, WatchListCtrl_Col_Value, stdstr(szValue).ToUTF16().c_str());
        m_WatchListCtrl.SetItemData(nItem, index);

        //if (presult->IsSelected())
        //{
        //    m_WatchListCtrl.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
        //}

        nItem++;
    }

    m_WatchListCtrl.SetRedraw(TRUE);
}

void CDebugMemorySearch::RefreshResultsListValues(void)
{
    if (!g_MMU || m_bDraggingSeparator)
    {
        return;
    }

    int numShownResults = m_ResultsListCtrl.GetItemCount();

    if (numShownResults > 0)
    {
        m_ResultsListCtrl.SetRedraw(FALSE);

        for (int nItem = 0; nItem < numShownResults; nItem++)
        {
            size_t index = m_ResultsListCtrl.GetItemData(nItem);
            CScanResult *presult = m_MemoryScanner.GetResult(index);

            if (presult == nullptr)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                break;
            }

            char szCurrentValue[1024];
            presult->GetMemoryValueString(szCurrentValue, 1024);

            m_ResultsListCtrl.SetItemText(nItem, ResultsListCtrl_Col_Value, stdstr(szCurrentValue).ToUTF16().c_str());
        }

        m_ResultsListCtrl.SetRedraw(TRUE);
    }
}

void CDebugMemorySearch::RefreshWatchListValues(void)
{
    if (!g_MMU || m_bDraggingSeparator)
    {
        return;
    }

    int numShownWatchListEntries = m_WatchListCtrl.GetItemCount();

    if (numShownWatchListEntries > 0)
    {
        m_WatchListCtrl.SetRedraw(FALSE);

        for (int nItem = 0; nItem < numShownWatchListEntries; nItem++)
        {
            size_t index = m_WatchListCtrl.GetItemData(nItem);
            CScanResult *presult = &m_WatchList[index];

            CBreakpoints *breakpoints = m_Debugger->Breakpoints();

            bool bHaveLock = breakpoints->MemLockExists(presult->m_Address, 1);
            bool bHaveReadBP = (breakpoints->ReadBPExists8(presult->m_Address) != CBreakpoints::BPSTATE::BP_NOT_SET);
            bool bHaveWriteBP = (breakpoints->WriteBPExists8(presult->m_Address) != CBreakpoints::BPSTATE::BP_NOT_SET);
            bool bHaveExecBP = (breakpoints->ExecutionBPExists(presult->m_Address) != CBreakpoints::BPSTATE::BP_NOT_SET);

            char szBPStates[4];
            sprintf(szBPStates, "%s%s%s",
                bHaveReadBP ? "R" : "",
                bHaveWriteBP ? "W" : "",
                bHaveExecBP ? "E" : "");

            char szCurrentValue[1024];
            presult->GetMemoryValueString(szCurrentValue, 1024);

            m_WatchListCtrl.SetItemText(nItem, WatchListCtrl_Col_Lock, (bHaveLock ? L"X" : L""));
            m_WatchListCtrl.SetItemText(nItem, WatchListCtrl_Col_BP, stdstr(szBPStates).ToUTF16().c_str());
            m_WatchListCtrl.SetItemText(nItem, WatchListCtrl_Col_Value, stdstr(szCurrentValue).ToUTF16().c_str());
        }

        m_WatchListCtrl.SetRedraw(TRUE);
    }
}

void CDebugMemorySearch::FlushWatchList(void)
{
    if (m_WatchList.size() == 0)
    {
        return;
    }

    CPath wlPath = GetWatchListPath();
    m_WatchListFile.Open(wlPath, CFileBase::modeCreate | CFile::modeWrite);

    if (!m_WatchListFile.IsOpen())
    {
        return;
    }

    size_t numWatchListEntries = m_WatchList.size();

    for (size_t i = 0; i < numWatchListEntries; i++)
    {
        CScanResult* presult = &m_WatchList[i];

        char szValueType[32];
        const char* pSzValueType;

        pSzValueType = presult->GetTypeName();

        switch (presult->GetType())
        {
        case ValueType_string:
        case ValueType_istring:
        case ValueType_unkstring:
            sprintf(szValueType, "%s[%d]", pSzValueType, presult->GetStrLength());
            pSzValueType = szValueType;
            break;
        }

        char cAddrType = (presult->m_AddressType == AddressType_Physical) ? 'p' : 'v';
        const char* szDisplayFormat = (presult->m_DisplayFormat == DisplayDefault) ? "def" : "hex";

        stdstr line = stdstr_f("%c,%08X,%s,%s,%s\n",
            cAddrType, presult->m_Address, pSzValueType, szDisplayFormat, presult->GetDescription());

        m_WatchListFile.Write(line.c_str(), line.length());
    }

    m_WatchListFile.Close();

    return;
}

void CDebugMemorySearch::LoadWatchList(void)
{
    if (m_hWnd)
    {
        ClearWatchList();
    }

    CPath wlPath = GetWatchListPath();
    m_WatchListFile.Open(wlPath, CFileBase::modeRead);

    if (!m_WatchListFile.IsOpen())
    {
        return;
    }

    uint32_t length = m_WatchListFile.GetLength();
    char* szWlFile = new char[length + 1];

    m_WatchListFile.SeekToBegin();
    m_WatchListFile.Read(szWlFile, length);
    szWlFile[length] = '\0';

    m_WatchListFile.Close();

    char* p = szWlFile;

    while (*p)
    {
        CScanResult result(AddressType_Virtual, DisplayDefault);

        char* szAddrType, *szAddress, *szValueType, *szDisplayFormat, *szDescription;

        szAddrType = p; p = strchr(p, ','); *p++ = '\0';
        szAddress = p; p = strchr(p, ','); *p++ = '\0';
        szValueType = p; p = strchr(p, ','); *p++ = '\0';
        szDisplayFormat = p; p = strchr(p, ','); *p++ = '\0';
        szDescription = p; p = strchr(p, '\n'); *p++ = '\0';

        switch (szAddrType[0])
        {
        case 'v': result.m_AddressType = AddressType_Virtual; break;
        case 'p': result.m_AddressType = AddressType_Physical; break;
        default: goto parse_error;
        }

        uint32_t address = strtoul(szAddress, nullptr, 16);
        result.m_Address = address;

        ValueType type;
        int charArrayLength = 0;
        type = CMixed::GetTypeFromString(szValueType, &charArrayLength);

        if (type == ValueType_invalid)
        {
            goto parse_error;
        }

        result.SetType(type);

        if (result.IsStringType())
        {
            // g_MMU is null here, can't use SetStrLengthSafe
            // TODO: fix
            result.SetStrLength(charArrayLength);
        }

        if (strcmp(szDisplayFormat, "hex") == 0)
        {
            result.m_DisplayFormat = DisplayHex;
        }
        else if (strcmp(szDisplayFormat, "def") == 0)
        {
            result.m_DisplayFormat = DisplayDefault;
        }
        else
        {
            goto parse_error;
        }

        if (szDescription[0] != '\0')
        {
            result.SetDescription(szDescription);
        }

        m_WatchList.push_back(result);
    }

    UpdateWatchList();

    parse_error:
    delete[] szWlFile;

}

void CDebugMemorySearch::FixListHeader(CListViewCtrl& listCtrl)
{
    CRect listRect, headRect;
    CHeaderCtrl listHead = listCtrl.GetHeader();

    listCtrl.GetWindowRect(&listRect);
    listHead.GetWindowRect(&headRect);

    listHead.ResizeClient(listRect.Width(), headRect.Height());
}

CPath CDebugMemorySearch::GetWatchListPath(void)
{
    stdstr strSaveDir = g_Settings->LoadStringVal(Directory_NativeSave);

    stdstr wlFileName = stdstr_f("%s.wlst", m_StrGame.c_str());

    CPath wlFilePath(strSaveDir.c_str(), wlFileName.c_str());

    if (g_Settings->LoadBool(Setting_UniqueSaveDir))
    {
        stdstr strUniqueSaveDir = g_Settings->LoadStringVal(Game_UniqueSaveDir);
        wlFilePath.AppendDirectory(strUniqueSaveDir.c_str());
    }

    wlFilePath.NormalizePath(CPath(CPath::MODULE_DIRECTORY));

    if (!wlFilePath.DirectoryExists())
    {
        wlFilePath.DirectoryCreate();
    }

    return wlFilePath;
}

INT_PTR CSetValueDlg::DoModal(const char* caption, const char* label, const char* initialText)
{
    m_Mode = Mode_TextBox;
    m_Caption = caption;
    m_Label = label;
    m_InitialText = initialText;
    return CDialogImpl<CSetValueDlg>::DoModal();
}

INT_PTR CSetValueDlg::DoModal(const char* caption, const char* label, DWORD_PTR initialData, const ComboItem items[])
{
    m_Mode = Mode_ComboBox;
    m_Caption = caption;
    m_Label = label;
    m_InitialData = initialData;
    m_ComboItems = items;
    return CDialogImpl<CSetValueDlg>::DoModal();
}

const std::string & CSetValueDlg::GetEnteredString(void)
{
    return m_EnteredString;
}

DWORD_PTR CSetValueDlg::GetEnteredData(void)
{
    if (m_Mode != Mode_ComboBox)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    return m_EnteredData;
}

LRESULT CSetValueDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SetWindowText(stdstr(m_Caption).ToUTF16().c_str());
    CenterWindow();
    m_Value.Attach(GetDlgItem(IDC_EDIT_VALUE));
    m_CmbValue.Attach(GetDlgItem(IDC_CMB_VALUE));
    m_Prompt.Attach(GetDlgItem(IDC_LBL_PROMPT));

    m_Prompt.SetWindowText(stdstr(m_Label).ToUTF16().c_str());

    if (m_Mode == Mode_TextBox)
    {
        m_CmbValue.ShowWindow(SW_HIDE);
        m_Value.SetWindowText(stdstr(m_InitialText).ToUTF16().c_str());
        m_Value.SetFocus();
        m_Value.SetSelAll();
    }
    else if (m_Mode == Mode_ComboBox)
    {
        m_Value.ShowWindow(SW_HIDE);

        for (int i = 0; m_ComboItems[i].str != nullptr; i++)
        {
            int idx = m_CmbValue.AddString(stdstr(m_ComboItems[i].str).ToUTF16().c_str());
            m_CmbValue.SetItemData(idx, m_ComboItems[i].data);
            if (m_ComboItems[i].data == m_InitialData)
            {
                m_CmbValue.SetCurSel(idx);
            }
        }
    }

    return FALSE;
}

LRESULT CSetValueDlg::OnDestroy(void)
{
    m_Value.Detach();
    m_CmbValue.Detach();
    m_Prompt.Detach();
    return 0;
}

LRESULT CSetValueDlg::OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    m_EnteredString.clear();
    if (m_Mode == Mode_TextBox)
    {
        m_EnteredString = GetCWindowText(m_Value);
    }
    else if (m_Mode == Mode_ComboBox)
    {
        m_EnteredString = GetCWindowText(m_CmbValue);
        m_EnteredData = m_CmbValue.GetItemData(m_CmbValue.GetCurSel());
    }

    EndDialog(TRUE);
    return FALSE;
}

LRESULT CSetValueDlg::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hwnd*/, BOOL& /*bHandled*/)
{
    EndDialog(FALSE);
    return FALSE;
}

CSetValueDlg::CSetValueDlg(void)
{
}

CSetValueDlg::~CSetValueDlg(void)
{
}

CEditMixed::CEditMixed(void) :
    m_String(nullptr)
{
}

CEditMixed::~CEditMixed(void)
{
}

DisplayFormat CEditMixed::GetDisplayFormat(void)
{
    return m_DisplayFormat;
}

void CEditMixed::SetDisplayFormat(DisplayFormat fmt)
{
    m_DisplayFormat = fmt;
}

void CEditMixed::ReloadString(void)
{
    if (m_String != nullptr)
    {
        delete[] m_String;
    }

    m_StringLength = GetWindowTextLength();
    m_String = new wchar_t[m_StringLength + 1];
    GetWindowText(m_String, m_StringLength + 1);
}

BOOL CEditMixed::Attach(HWND hWndNew)
{
    return SubclassWindow(hWndNew);
}

bool CEditMixed::GetValue(uint8_t& value)
{
    uint64_t valueU64;
    bool bValid = GetValue(valueU64);

    if (!bValid || valueU64 > UINT8_MAX)
    {
        return false;
    }

    value = (uint8_t)(valueU64 & 0xFF);
    return true;
}

bool CEditMixed::GetValue(int8_t& value)
{
    if (m_DisplayFormat == DisplayHex)
    {
        return GetValue((uint8_t&)value);
    }

    int64_t valueS64;
    bool bValid = GetValue(valueS64);

    if (!bValid || valueS64 > INT8_MAX || valueS64 < INT8_MIN)
    {
        return false;
    }

    value = (int8_t)(valueS64 & 0xFF);
    return true;
}

bool CEditMixed::GetValue(uint16_t& value)
{
    uint64_t valueU64;
    bool bValid = GetValue(valueU64);

    if (!bValid || valueU64 > UINT16_MAX)
    {
        return false;
    }

    value = (uint16_t)(valueU64 & 0xFFFF);
    return true;
}

bool CEditMixed::GetValue(int16_t& value)
{
    if (m_DisplayFormat == DisplayHex)
    {
        return GetValue((uint16_t&)value);
    }

    int64_t valueS64;
    bool bValid = GetValue(valueS64);

    if (!bValid || valueS64 > INT16_MAX || valueS64 < INT16_MIN)
    {
        return false;
    }

    value = (int16_t)(valueS64 & 0xFFFF);
    return true;
}

bool CEditMixed::GetValue(uint32_t& value)
{
    uint64_t valueU64;
    bool bValid = GetValue(valueU64);

    if (!bValid || valueU64 > UINT32_MAX)
    {
        return false;
    }

    value = (uint32_t)(valueU64 & 0xFFFFFFFF);
    return true;
}

bool CEditMixed::GetValue(int32_t& value)
{
    if (m_DisplayFormat == DisplayHex)
    {
        return GetValue((uint32_t&)value);
    }

    int64_t valueS64;
    bool bValid = GetValue(valueS64);

    if (!bValid || (valueS64 > INT32_MAX) || (valueS64 < INT32_MIN))
    {
        return false;
    }

    value = (int32_t)(valueS64 & 0xFFFFFFFF);
    return true;
}

bool CEditMixed::GetValue(uint64_t& value)
{
    ReloadString();

    char *end;
    uint64_t res = strtoull(stdstr().FromUTF16(m_String).c_str(), &end, m_DisplayFormat == DisplayHex ? 16 : 10);

    if (*end != '\0')
    {
        return false; // Parse failure
    }

    value = res;
    return true;
}

bool CEditMixed::GetValue(int64_t& value)
{
    if (m_DisplayFormat == DisplayHex)
    {
        return GetValue((uint64_t&)value);
    }

    ReloadString();

    char *end;
    uint64_t res = strtoll(stdstr().FromUTF16(m_String).c_str(), &end, m_DisplayFormat == DisplayHex ? 16 : 10);

    if (*end != '\0')
    {
        return false; // Parse failure
    }

    value = res;
    return true;
}

bool CEditMixed::GetValue(float& value)
{
    if (m_DisplayFormat == DisplayHex)
    {
        return GetValue((uint32_t&)value);
    }

    ReloadString();

    float valueF32;
    char *end;

    valueF32 = strtof(stdstr().FromUTF16(m_String).c_str(), &end);

    if (*end != '\0')
    {
        return false;
    }

    value = valueF32;
    return true;
}

bool CEditMixed::GetValue(double& value)
{
    if (m_DisplayFormat == DisplayHex)
    {
        return GetValue((uint64_t&)value);
    }

    ReloadString();

    double valueF64;
    char *end;

    valueF64 = strtod(stdstr().FromUTF16(m_String).c_str(), &end);

    if (*end != '\0')
    {
        return false;
    }

    value = valueF64;
    return true;
}

bool CEditMixed::GetValueString(const wchar_t*& value, int& length)
{
    ReloadString();

    if (m_StringLength == 0)
    {
        return false;
    }

    value = m_String;
    length = m_StringLength;
    return true;
}

bool CEditMixed::GetValueHexString(const wchar_t*& value, int& length)
{
    ReloadString();

    if (m_StringLength == 0)
    {
        return false;
    }

    stdstr string = stdstr().FromUTF16(m_String);
    int numBytes = CMemoryScanner::ParseHexString(nullptr, string.c_str());

    if (numBytes == 0)
    {
        return false;
    }

    
    char *hexString = new char[numBytes];
    wchar_t *wchexString = new wchar_t[numBytes];

    CMemoryScanner::ParseHexString(hexString, string.c_str());
    wcscpy(wchexString, stdstr(hexString).ToUTF16().c_str());

    delete[] hexString;
    delete[] m_String;

    m_String = wchexString;
    m_StringLength = numBytes;

    value = m_String;
    length = m_StringLength;
    return true;
}
