#include "stdafx.h"
#include <Project64-core/N64System/Enhancement/Enhancements.h>

CCheatsUI::CCheatsUI(void) :
    m_EditCheat(m_Cheats, m_SelectCheat),
    m_SelectCheat(m_Cheats, m_EditCheat),
    m_bModal(false)
{
}

CCheatsUI::~CCheatsUI()
{
}

void CCheatsUI::Display(HWND hParent, bool BlockExecution)
{
    m_Cheats = g_Enhancements->Cheats();
    if (g_BaseSystem)
    {
        g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_Cheats);
    }
    if (BlockExecution)
    {
        m_bModal = true;
        DoModal(hParent);
    }
    else if (m_hWnd != NULL)
    {
        SetFocus();
    }
    else
    {
        m_bModal = false;
        Create(hParent);
    }
}

LRESULT CCheatsUI::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    bool inBasicMode = g_Settings->LoadBool(UserInterface_BasicMode);
    m_StateBtn.Attach(GetDlgItem(IDC_STATE));

    WINDOWPLACEMENT WndPlac = {0};
    WndPlac.length = sizeof(WndPlac);
    GetWindowPlacement(&WndPlac);

    SetWindowText(wGS(CHEAT_TITLE).c_str());
    m_SelectCheat.Create(m_hWnd, (LPARAM)this);
    m_SelectCheat.SetWindowPos(HWND_TOP, 5, 8, 0, 0, SWP_NOSIZE);
    m_SelectCheat.ShowWindow(SW_SHOW);

    if (inBasicMode)
    {
        CRect rcList;
        m_SelectCheat.GetWindowRect(&rcList);
        m_MinSizeDlg = rcList.right - rcList.left + 16;
        m_MaxSizeDlg = m_MinSizeDlg;
        m_DialogState = CONTRACTED;
        WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + m_MinSizeDlg;
        SetWindowPlacement(&WndPlac);

        m_StateBtn.ShowWindow(SW_HIDE);
    }
    else
    {
        m_EditCheat.Create(m_hWnd);
        m_EditCheat.SetWindowPos(HWND_TOP, (WndPlac.rcNormalPosition.right - WndPlac.rcNormalPosition.left) / 2, 8, 0, 0, SWP_NOSIZE);
        m_EditCheat.ShowWindow(SW_HIDE);

        CRect rcList, rcEditCheat;
        m_SelectCheat.GetWindowRect(&rcList);
        m_EditCheat.GetWindowRect(&rcEditCheat);
        m_MinSizeDlg = rcList.Width() + 32;
        m_MaxSizeDlg = rcEditCheat.right - rcList.left + 32;
        m_DialogState = CONTRACTED;
        WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + m_MinSizeDlg;
        SetWindowPlacement(&WndPlac);

        CRect rc;
        GetClientRect(&rc);
        m_StateBtn.SetWindowPos(HWND_TOP, rc.Width() - 16, 0, 16, rc.Height(), 0);
        m_StateBtn.SetIcon(AtlLoadIconImage(MAKEINTRESOURCE(IDI_RIGHT), LR_DEFAULTCOLOR));
    }
    CRect rcDlg, rcParent;
    GetWindowRect(&rcDlg);
    GetParent().GetWindowRect(&rcParent);

    int32_t DlgWidth = rcDlg.Width();
    int32_t DlgHeight = rcDlg.Height();
    int32_t X = (((rcParent.Width()) - DlgWidth) / 2) + rcParent.left;
    int32_t Y = (((rcParent.Height()) - DlgHeight) / 2) + rcParent.top;
    SetWindowPos(NULL, X, Y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    m_SelectCheat.RefreshItems();
    ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CCheatsUI::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_StateBtn.Detach();
    return 0;
}

LRESULT CCheatsUI::OnCloseCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (m_EditCheat.ValuesChanged())
    {
        return 0;
    }

    if (g_BaseSystem)
    {
        g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_Cheats);
    }
    if (m_bModal)
    {
        EndDialog(0);
    }
    else
    {
        DestroyWindow();
    }
    return 0;
}

LRESULT CCheatsUI::OnStateChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    WINDOWPLACEMENT WndPlac;
    WndPlac.length = sizeof(WndPlac);
    GetWindowPlacement(&WndPlac);

    if (m_DialogState == CONTRACTED)
    {
        m_DialogState = EXPANDED;
        WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + m_MaxSizeDlg;
        SetWindowPlacement(&WndPlac);

        RECT clientrect;
        GetClientRect(&clientrect);
        m_StateBtn.SetIcon(AtlLoadIconImage(MAKEINTRESOURCE(IDI_LEFT), LR_DEFAULTCOLOR));
        m_StateBtn.SetWindowPos(HWND_TOP, (clientrect.right - clientrect.left) - 16, 0, 16, clientrect.bottom - clientrect.top, 0);
        m_EditCheat.ShowWindow(SW_SHOW);
    }
    else
    {
        m_DialogState = CONTRACTED;
        WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + m_MinSizeDlg;
        SetWindowPlacement(&WndPlac);

        RECT clientrect;
        GetClientRect(&clientrect);
        m_StateBtn.SetIcon(AtlLoadIconImage(MAKEINTRESOURCE(IDI_RIGHT), LR_DEFAULTCOLOR));
        m_StateBtn.SetWindowPos(HWND_TOP, (clientrect.right - clientrect.left) - 16, 0, 16, clientrect.bottom - clientrect.top, 0);
        m_EditCheat.ShowWindow(SW_HIDE);
    }
    return 0;
}

CCheatList::CCheatList(CEnhancementList & Cheats, CEditCheat & EditCheat) :
    m_Cheats(Cheats),
    m_EditCheat(EditCheat),
    m_hSelectedItem(NULL),
    m_DeleteingEntries(false)
{
}

CCheatList::~CCheatList()
{
}

LRESULT CCheatList::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    GetDlgItem(IDC_NOTESFRAME).SetWindowText(wGS(CHEAT_NOTES_FRAME).c_str());
    GetDlgItem(IDC_UNMARK).SetWindowText(wGS(CHEAT_MARK_NONE).c_str());

    RECT rcButton, rcList;
    GetDlgItem(IDC_UNMARK).GetWindowRect(&rcButton);
    GetWindowRect(&rcList);

    CRect hCheatTreeRect(6, 4, rcList.right - rcList.left - 13, rcButton.top - rcList.top - 8);
    m_hCheatTree.Create(m_hWnd, &hCheatTreeRect, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_DISABLEDRAGDROP | WS_TABSTOP | TVS_FULLROWSELECT,
        WS_EX_CLIENTEDGE, (HMENU)IDC_MYTREE);
    m_hCheatTree.SetWindowLong(GWL_STYLE, TVS_CHECKBOXES | TVS_SHOWSELALWAYS | m_hCheatTree.GetWindowLong(GWL_STYLE));

    CImageList hImageList;
    hImageList.Create(16, 16, ILC_COLOR | ILC_MASK, 40, 40);
    CBitmap bmp;
    bmp.LoadBitmap(MAKEINTRESOURCE(IDB_TRI_STATE));
    hImageList.Add(bmp, RGB(255, 0, 255));
    m_hCheatTree.SetImageList(hImageList, TVSIL_STATE);
    return true;
}

LRESULT CCheatList::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_hCheatTree.Detach();
    return 0;
}

LRESULT CCheatList::OnChangeCodeExtension(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    m_hSelectedItem = (HTREEITEM)lParam;

    TVITEM item = { 0 };
    item.mask = TVIF_PARAM;
    item.hItem = m_hSelectedItem;
    if (!m_hCheatTree.GetItem(&item) || item.lParam == NULL)
    {
        return 0;
    }

    CEnhancement * Enhancement = (CEnhancement *)item.lParam;
    if (Enhancement->CodeOptionSize() == 0 || Enhancement->GetOptions().empty())
    {
        return 0;
    }

    CEnhancementCodeEx(Enhancement).DoModal();
    stdstr Name = Enhancement->GetNameAndExtension();
    const char * ChildName = strrchr(Name.c_str(), '\\');
    if (ChildName != nullptr)
    {
        Name = &ChildName[1];
    }
    std::wstring wName = Name.ToUTF16();
    item.mask = TVIF_TEXT;
    item.pszText = (LPWSTR)wName.c_str();
    item.cchTextMax = wName.length();
    m_hCheatTree.SetItem(&item);
    return 0;
}

LRESULT CCheatList::OnPopupDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int Response = MessageBox(wGS(MSG_DEL_SURE).c_str(), wGS(MSG_DEL_TITLE).c_str(), MB_YESNO | MB_ICONQUESTION);
    if (Response != IDYES)
    {
        return 0;
    }

    TVITEM item = { 0 };
    item.hItem = m_hSelectedItem;
    item.mask = TVIF_PARAM;
    m_hCheatTree.GetItem(&item);

    ChangeChildrenStatus(TVI_ROOT, false);
    DeleteCheat(item.lParam);
    RefreshItems();
    return 0;
}

LRESULT CCheatList::OnUnmark(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ChangeChildrenStatus(TVI_ROOT, false);
    if (g_Enhancements != nullptr)
    {
        g_Enhancements->UpdateCheats();
    }
    return 0;
}

LRESULT CCheatList::OnTreeClicked(NMHDR* lpnmh)
{
    uint32_t dwpos = GetMessagePos();
    TVHITTESTINFO ht = { 0 };
    ht.pt.x = GET_X_LPARAM(dwpos);
    ht.pt.y = GET_Y_LPARAM(dwpos);
    ::MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

    TreeView_HitTest(lpnmh->hwndFrom, &ht);

    if (TVHT_ONITEMSTATEICON & ht.flags)
    {
        switch (TV_GetCheckState(ht.hItem))
        {
        case TV_STATE_CLEAR:
        case TV_STATE_INDETERMINATE:
            if (m_hCheatTree.GetChildItem(ht.hItem) == NULL)
            {
                TVITEM item = { 0 };
                item.mask = TVIF_PARAM;
                item.hItem = ht.hItem;
                m_hCheatTree.GetItem(&item);        
                if (item.lParam != NULL)
                {
                    CEnhancement * Enhancement = (CEnhancement *)item.lParam;
                    if (Enhancement->CodeOptionSize() != 0 && Enhancement->GetOptions().size() > 0 && !Enhancement->OptionSelected())
                    {
                        SendMessage(UM_CHANGECODEEXTENSION, 0, (LPARAM)ht.hItem);
                        TV_SetCheckState(ht.hItem, TV_STATE_CLEAR);
                        break;
                    }
                }
            }
            TV_SetCheckState(ht.hItem, TV_STATE_CHECKED);
            ChangeChildrenStatus(ht.hItem, true);
            CheckParentStatus(m_hCheatTree.GetParentItem(ht.hItem));
            break;
        case TV_STATE_CHECKED:
            TV_SetCheckState(ht.hItem, TV_STATE_CLEAR);
            ChangeChildrenStatus(ht.hItem, false);
            CheckParentStatus(m_hCheatTree.GetParentItem(ht.hItem));
            break;
        }
        switch (TV_GetCheckState(ht.hItem))
        {
        case TV_STATE_CHECKED: TV_SetCheckState(ht.hItem, TV_STATE_INDETERMINATE); break;
        case TV_STATE_CLEAR: TV_SetCheckState(ht.hItem, TV_STATE_CHECKED); break;
        case TV_STATE_INDETERMINATE: TV_SetCheckState(ht.hItem, TV_STATE_CLEAR); break;
        }
    
        if (g_Enhancements != nullptr)
        {
            g_Enhancements->UpdateCheats();
        }
    }
    return 0;
}

LRESULT CCheatList::OnTreeRClicked(NMHDR* lpnmh)
{
    if (g_Settings->LoadBool(UserInterface_BasicMode)) { return true; }

    //Work out what item is selected
    TVHITTESTINFO ht = { 0 };
    uint32_t dwpos = GetMessagePos();

    // include <windowsx.h> and <windows.h> header files
    ht.pt.x = GET_X_LPARAM(dwpos);
    ht.pt.y = GET_Y_LPARAM(dwpos);
    ::MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

    TreeView_HitTest(lpnmh->hwndFrom, &ht);
    m_hSelectedItem = ht.hItem;

    //Show Menu
    HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_CHEAT_MENU));
    HMENU hPopupMenu = GetSubMenu(hMenu, 0);
    POINT Mouse;

    GetCursorPos(&Mouse);

    MenuSetText(hPopupMenu, 0, wGS(CHEAT_ADDNEW).c_str(), NULL);
    MenuSetText(hPopupMenu, 1, wGS(CHEAT_EDIT).c_str(), NULL);
    MenuSetText(hPopupMenu, 3, wGS(CHEAT_DELETE).c_str(), NULL);

    if (m_hSelectedItem != NULL && m_hCheatTree.GetChildItem(m_hSelectedItem) == NULL)
    {
        TrackPopupMenu(hPopupMenu, 0, Mouse.x, Mouse.y, 0, m_hWnd, NULL);
    }
    DestroyMenu(hMenu);
    return true;
}

LRESULT CCheatList::OnTreeDClicked(NMHDR* lpnmh)
{
    uint32_t dwpos = GetMessagePos();
    TVHITTESTINFO ht = { 0 };
    ht.pt.x = GET_X_LPARAM(dwpos);
    ht.pt.y = GET_Y_LPARAM(dwpos);
    ::MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

    TreeView_HitTest(lpnmh->hwndFrom, &ht);

    if (TVHT_ONITEMLABEL & ht.flags)
    {
        PostMessage(UM_CHANGECODEEXTENSION, 0, (LPARAM)ht.hItem);
    }
    return 0;
}

LRESULT CCheatList::OnTreeSelChanged(NMHDR * /*lpnmh*/)
{
    HTREEITEM hItem = m_hCheatTree.GetSelectedItem();
    GetDlgItem(IDC_NOTES).SetWindowText(L"");
    if (m_hCheatTree.GetChildItem(hItem) == NULL)
    {
        TVITEM item = { 0 };
        item.mask = TVIF_PARAM;
        item.hItem = hItem;
        m_hCheatTree.GetItem(&item);
        if (item.lParam != NULL)
        {
            CEnhancement * Enhancement = (CEnhancement *)item.lParam;
            GetDlgItem(IDC_NOTES).SetWindowText(stdstr(Enhancement->GetNote()).ToUTF16().c_str());
            if (m_EditCheat.m_hWnd != NULL)
            {
                m_EditCheat.SendMessage(CEditCheat::WM_EDITCHEAT, item.lParam, 0);
            }
        }
    }
    return 0;
}

void CCheatList::RefreshItems()
{
    if (m_hWnd == NULL) { return; }

    m_DeleteingEntries = true;
    m_hCheatTree.DeleteAllItems();
    m_DeleteingEntries = false;

    for (CEnhancementList::iterator itr = m_Cheats.begin(); itr != m_Cheats.end(); itr++)
    {
        std::string Name = itr->second.GetNameAndExtension();
        if (Name.length() == 0) 
        { 
            continue; 
        }
        AddCodeLayers((LPARAM)&itr->second, stdstr(Name).ToUTF16(), TVI_ROOT, itr->second.Active());
    }
}

void CCheatList::AddCodeLayers(LPARAM Enhancement, const std::wstring &Name, HTREEITEM hParent, bool CheatActive)
{
    TV_INSERTSTRUCT tv;

    wchar_t Text[500], Item[500];
    if (Name.length() > ((sizeof(Text) / sizeof(Text[0])) - 5)) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    wcscpy(Text, Name.c_str());
    if (wcschr(Text, L'\\') > 0) { *wcschr(Text, L'\\') = 0; }

    tv.item.mask = TVIF_TEXT;
    tv.item.pszText = Item;
    tv.item.cchTextMax = sizeof(Item);
    tv.item.hItem = TreeView_GetChild(m_hCheatTree, hParent);
    while (tv.item.hItem)
    {
        TreeView_GetItem(m_hCheatTree, &tv.item);
        if (wcscmp(Text, Item) == 0)
        {
            TV_CHECK_STATE State = TV_GetCheckState(tv.item.hItem);
            if ((CheatActive && State == TV_STATE_CLEAR) || (!CheatActive && State == TV_STATE_CHECKED))
            {
                TV_SetCheckState(tv.item.hItem, TV_STATE_INDETERMINATE);
            }
            size_t StartPos = wcslen(Text) + 1;
            std::wstring TempCheatName;
            if (StartPos < Name.length())
            {
                TempCheatName = Name.substr(StartPos);
            }
            AddCodeLayers(Enhancement, TempCheatName, tv.item.hItem, CheatActive);
            return;
        }
        tv.item.hItem = TreeView_GetNextSibling(m_hCheatTree, tv.item.hItem);
    }

    tv.hInsertAfter = TVI_SORT;
    tv.item.mask = TVIF_TEXT | TVIF_PARAM;
    tv.item.pszText = Text;
    tv.item.lParam = Enhancement;
    tv.hParent = hParent;
    hParent = TreeView_InsertItem(m_hCheatTree, &tv);
    TV_SetCheckState(hParent, CheatActive ? TV_STATE_CHECKED : TV_STATE_CLEAR);

    if (wcscmp(Text, Name.c_str()) == 0) { return; }
    AddCodeLayers(Enhancement, Name.substr(wcslen(Text) + 1), hParent, CheatActive);
}

void CCheatList::ChangeChildrenStatus(HTREEITEM hParent, bool Checked)
{
    HTREEITEM hItem = m_hCheatTree.GetChildItem(hParent);;
    if (hItem == NULL)
    {
        if (hParent == TVI_ROOT) { return; }

        TVITEM item = { 0 };
        item.mask = TVIF_PARAM;
        item.hItem = hParent;
        m_hCheatTree.GetItem(&item);
        if (item.lParam == NULL)
        {
            return;
        }
        CEnhancement * Enhancement = (CEnhancement *)item.lParam;
        if (Checked && Enhancement->CodeOptionSize() != 0 && Enhancement->GetOptions().size() > 0 && !Enhancement->OptionSelected())
        {
            return;
        }

        TV_SetCheckState(hParent, Checked ? TV_STATE_CHECKED : TV_STATE_CLEAR);
        Enhancement->SetActive(Checked);
        if (g_Enhancements != nullptr)
        {
            g_Enhancements->UpdateCheats();
        }
        return;
    }
    TV_CHECK_STATE state = TV_STATE_UNKNOWN;
    while (hItem != NULL)
    {
        TV_CHECK_STATE ChildState = TV_GetCheckState(hItem);
        if ((ChildState != TV_STATE_CHECKED || !Checked) &&
            (ChildState != TV_STATE_CLEAR || Checked))
        {
            ChangeChildrenStatus(hItem, Checked);
        }
        ChildState = TV_GetCheckState(hItem);
        if (state == TV_STATE_UNKNOWN) { state = ChildState; }
        if (state != ChildState) { state = TV_STATE_INDETERMINATE; }
        hItem = m_hCheatTree.GetNextSiblingItem(hItem);
    }
    if (state != TV_STATE_UNKNOWN)
    {
        TV_SetCheckState(hParent, state);
    }
}

void CCheatList::CheckParentStatus(HTREEITEM hParent)
{
    if (hParent == NULL)
    {
        return;
    }
    HTREEITEM hItem = m_hCheatTree.GetChildItem(hParent);
    TV_CHECK_STATE InitialState = TV_GetCheckState(hParent);
    TV_CHECK_STATE CurrentState = TV_GetCheckState(hItem);
    while (hItem != NULL)
    {
        if (TV_GetCheckState(hItem) != CurrentState)
        {
            CurrentState = TV_STATE_INDETERMINATE;
            break;
        }
        hItem = m_hCheatTree.GetNextSiblingItem(hItem);
    }
    TV_SetCheckState(hParent, CurrentState);
    if (InitialState != CurrentState)
    {
        CheckParentStatus(m_hCheatTree.GetParentItem(hParent));
    }
}

void CCheatList::DeleteCheat(LPARAM Enhancement)
{
    for (CEnhancementList::iterator itr = m_Cheats.begin(); itr != m_Cheats.end(); itr++)
    {
        if (Enhancement != (LPARAM)&itr->second)
        {
            continue;
        }
        m_Cheats.erase(itr);
        g_Enhancements->UpdateCheats(m_Cheats);
        RefreshItems();
        break;
    }
}

CCheatList::TV_CHECK_STATE CCheatList::TV_GetCheckState(HTREEITEM hItem)
{
    TVITEM tvItem = { 0 };
    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;

    m_hCheatTree.GetItem(&tvItem);

    switch (tvItem.state >> 12)
    {
    case 1: return TV_STATE_CHECKED;
    case 2: return TV_STATE_CLEAR;
    case 3: return TV_STATE_INDETERMINATE;
    }
    return TV_STATE_UNKNOWN;
}

bool CCheatList::TV_SetCheckState(HTREEITEM hItem, TV_CHECK_STATE state)
{
    TVITEM tvItem = { 0 };
    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = (HTREEITEM)hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;

    switch (state)
    {
    case TV_STATE_CHECKED: tvItem.state = INDEXTOSTATEIMAGEMASK(1); break;
    case TV_STATE_CLEAR: tvItem.state = INDEXTOSTATEIMAGEMASK(2); break;
    case TV_STATE_INDETERMINATE: tvItem.state = INDEXTOSTATEIMAGEMASK(3); break;
    default: tvItem.state = INDEXTOSTATEIMAGEMASK(0); break;
    }
    return m_hCheatTree.SetItem(&tvItem) != 0;
}

void CCheatList::MenuSetText(HMENU hMenu, int MenuPos, const wchar_t * Title, const wchar_t * ShortCut)
{
    if (Title == NULL || wcslen(Title) == 0)
    {
        return;
    }

    MENUITEMINFO MenuInfo = { 0 };
    wchar_t String[256];
    MenuInfo.cbSize = sizeof(MENUITEMINFO);
    MenuInfo.fMask = MIIM_TYPE;
    MenuInfo.fType = MFT_STRING;
    MenuInfo.fState = MFS_ENABLED;
    MenuInfo.dwTypeData = String;
    MenuInfo.cch = 256;

    GetMenuItemInfo(hMenu, MenuPos, true, &MenuInfo);
    wcscpy(String, Title);
    if (wcschr(String, '\t') != NULL) { *(wcschr(String, '\t')) = '\0'; }
    if (ShortCut) { _swprintf(String, L"%s\t%s", String, ShortCut); }
    SetMenuItemInfo(hMenu, MenuPos, true, &MenuInfo);
}

CEditCheat::CEditCheat(CEnhancementList & Cheats, CCheatList & CheatList) :
    m_Cheats(Cheats),
    m_CheatList(CheatList),
    m_EditEnhancement(nullptr)
{
}

CEditCheat::~CEditCheat()
{
}

LRESULT CEditCheat::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    GetDlgItem(IDC_NAME).SetWindowText(wGS(CHEAT_ADDCHEAT_NAME).c_str());
    GetDlgItem(IDC_CODE).SetWindowText(wGS(CHEAT_ADDCHEAT_CODE).c_str());
    GetDlgItem(IDC_LABEL_OPTIONS).SetWindowText(wGS(CHEAT_ADDCHEAT_OPT).c_str());
    GetDlgItem(IDC_CODE_DES).SetWindowText(wGS(CHEAT_ADDCHEAT_CODEDES).c_str());
    GetDlgItem(IDC_LABEL_OPTIONS_FORMAT).SetWindowText(wGS(CHEAT_ADDCHEAT_OPTDES).c_str());
    GetDlgItem(IDC_CHEATNOTES).SetWindowText(wGS(CHEAT_ADDCHEAT_NOTES).c_str());
    GetDlgItem(IDC_NEWCHEAT).SetWindowText(wGS(CHEAT_ADDCHEAT_NEW).c_str());
    GetDlgItem(IDC_ADD).SetWindowText(wGS(CHEAT_ADDCHEAT_ADD).c_str());
    RecordCurrentValues();
    return 0;
}

LRESULT CEditCheat::OnEditCheat(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (ValuesChanged())
    {
        return 0;
    }

    m_EditEnhancement = (CEnhancement *)wParam;
    if (m_EditEnhancement == NULL)
    {
        return 0;
    }
    GetDlgItem(IDC_CODE_NAME).SetWindowText(stdstr(m_EditEnhancement->GetName()).ToUTF16().c_str());
    stdstr Buffer;
    const CEnhancement::CodeEntries & Entries = m_EditEnhancement->GetEntries();
    for (size_t i = 0, n = Entries.size(); i < n; i++)
    {
        const CEnhancement::CodeEntry Entry = Entries[i];
        if (!Buffer.empty())
        {
            Buffer.append("\r\n");
        }
        Buffer += stdstr_f("%08X %s", Entry.Command, Entry.Value.c_str());
    }
    GetDlgItem(IDC_CHEAT_CODES).SetWindowText(Buffer.ToUTF16().c_str());

    const CEnhancement::CodeOptions & Options = m_EditEnhancement->GetOptions();
    Buffer.clear();
    for (size_t i = 0, n = Options.size(); i < n; i++)
    {
        const CEnhancement::CodeOption Option = Options[i];
        if (!Buffer.empty())
        {
            Buffer.append("\r\n");
        }
        Buffer += stdstr_f(m_EditEnhancement->CodeOptionSize() == 2 ? "%02X %s" : "%04X %s", Option.Value, Option.Name.c_str());
    }
    GetDlgItem(IDC_CHEAT_OPTIONS).SetWindowText(Buffer.ToUTF16().c_str());
    GetDlgItem(IDC_NOTES).SetWindowText(stdstr(m_EditEnhancement->GetNote()).ToUTF16().c_str());
    GetDlgItem(IDC_ADD).SetWindowText(wGS(CHEAT_EDITCHEAT_UPDATE).c_str());

    SendMessage(WM_COMMAND, MAKELPARAM(IDC_CHEAT_CODES, EN_CHANGE), (LPARAM)(HWND)GetDlgItem(IDC_CHEAT_CODES));
    RecordCurrentValues();
    return 0;
}

LRESULT CEditCheat::OnAddCheat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    std::string NewCheatName = GetItemText(IDC_CODE_NAME);
    for (CEnhancementList::const_iterator itr = m_Cheats.begin(); itr != m_Cheats.end(); itr++)
    {
        const CEnhancement & Enhancement = itr->second;
        if (_stricmp(Enhancement.GetName().c_str(), NewCheatName.c_str()) != 0)
        {
            continue;
        }
        if (m_EditEnhancement == &Enhancement)
        {
            continue;
        }
        g_Notify->DisplayWarning(GS(MSG_CHEAT_NAME_IN_USE));
        GetDlgItem(IDC_CODE_NAME).SetFocus();
        return 0;
    }

    CEnhancement Enhancement(CEnhancement::CheatIdent);
    if (!ReadEnhancement(Enhancement))
    {
        return 0;
    }

    if (m_EditEnhancement != nullptr)
    {
        m_EditEnhancement->SetName(NewCheatName.c_str());
        m_EditEnhancement->SetEntries(Enhancement.GetEntries());
        m_EditEnhancement->SetOptions(Enhancement.GetOptions());
        m_EditEnhancement->SetNote(GetItemText(IDC_NOTES).c_str());
    }
    else
    {
        m_Cheats.AddItem(Enhancement);
    }
    g_Enhancements->UpdateCheats(m_Cheats);
    RecordCurrentValues();
    m_CheatList.RefreshItems();
    return 0;
}

LRESULT CEditCheat::OnNewCheat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (ValuesChanged())
    {
        return 0;
    }
    m_EditEnhancement = nullptr;
    SetDlgItemText(IDC_CODE_NAME, L"");
    SetDlgItemText(IDC_CHEAT_CODES, L"");
    SetDlgItemText(IDC_CHEAT_OPTIONS, L"");
    SetDlgItemText(IDC_NOTES, L"");
    GetDlgItem(IDC_ADD).SetWindowText(wGS(CHEAT_ADDCHEAT_ADD).c_str());
    GetDlgItem(IDC_ADD).EnableWindow(false);
    GetDlgItem(IDC_CHEAT_OPTIONS).EnableWindow(false);
    RecordCurrentValues();
    return 0;
}

LRESULT CEditCheat::OnCodeNameChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    DetailsChanged();
    return 0;
}

LRESULT CEditCheat::OnCheatCodeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    DetailsChanged();
    return 0;
}

LRESULT CEditCheat::OnCheatOptionsChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    DetailsChanged();
    return 0;
}

bool CEditCheat::ReadEnhancement(CEnhancement & Enhancement)
{
    CEnhancement TestEnhancement(CEnhancement::CheatIdent);
    TestEnhancement.SetName(GetItemText(IDC_CODE_NAME).c_str());
    TestEnhancement.SetNote(GetItemText(IDC_NOTES).c_str());

    CEnhancement::CodeEntries Entries;
    CEdit CheatCodes(GetDlgItem(IDC_CHEAT_CODES));
    int NumLines = CheatCodes.GetLineCount();
    for (int i = 0; i < NumLines; i++)
    {
        wchar_t wc_str[128];
        int len = CheatCodes.GetLine(i, wc_str, sizeof(wc_str) / sizeof(wc_str[0]));
        if (len <= 0)
        {
            continue;
        }
        wc_str[len] = 0;
        std::string str = stdstr().FromUTF16(wc_str);
        CEnhancement::CodeEntry Entry;
        Entry.Command = strtoul(str.c_str(), 0, 16);
        const char * ReadPos = strchr(str.c_str(), ' ');
        if (ReadPos != nullptr)
        {
            Entry.Value = ReadPos + 1;
            Entries.push_back(Entry);
        }
        else
        {
            return false;
        }
    }
    TestEnhancement.SetEntries(Entries);

    CEdit CheatOptions(GetDlgItem(IDC_CHEAT_OPTIONS));
    NumLines = CheatOptions.GetLineCount();
    std::string OptionsStr;
    CEnhancement::CodeOptions Options;
    for (int i = 0; i < NumLines; i++)
    {
        wchar_t wc_str[128];
        int len = CheatOptions.GetLine(i, wc_str, sizeof(wc_str) / sizeof(wc_str[0]));
        if (len <= 0)
        {
            continue;
        }
        wc_str[len] = 0;

        std::string str = stdstr().FromUTF16(wc_str);
        CEnhancement::CodeOption Option;
        Option.Value = (uint16_t)strtoul(str.c_str(), 0, 16);
        if ((uint32_t)len > TestEnhancement.CodeOptionSize() && str[TestEnhancement.CodeOptionSize()] != ' ')
        {
            return false;
        }
        const char * ReadPos = strchr(str.c_str(), ' ');
        if (ReadPos != nullptr)
        {
            Option.Name = ReadPos + 1;
            Options.push_back(Option);
        }
        else
        {
            return false;
        }
    }
    TestEnhancement.SetOptions(Options);
    if (!TestEnhancement.Valid())
    {
        return false;
    }
    Enhancement = TestEnhancement;
    return true;
}

void CEditCheat::DetailsChanged(void)
{
    CEnhancement Enhancement(CEnhancement::CheatIdent);
    if (!ReadEnhancement(Enhancement))
    {
        GetDlgItem(IDC_ADD).EnableWindow(false);
    }
    else
    {
        bool HasOptions = Enhancement.CodeOptionSize() > 0;

        GetDlgItem(IDC_LABEL_OPTIONS).EnableWindow(HasOptions);
        GetDlgItem(IDC_LABEL_OPTIONS_FORMAT).EnableWindow(HasOptions);
        GetDlgItem(IDC_CHEAT_OPTIONS).EnableWindow(HasOptions);
        GetDlgItem(IDC_ADD).EnableWindow(Enhancement.CodeOptionSize() == 0 || Enhancement.GetOptions().size() > 0);
    }
}

void CEditCheat::RecordCurrentValues(void)
{
    m_EditName = GetItemText(IDC_CODE_NAME);
    m_EditCode = GetItemText(IDC_CHEAT_CODES);
    m_EditOptions = GetItemText(IDC_CHEAT_OPTIONS);
    m_EditNotes = GetItemText(IDC_NOTES);
}

bool CEditCheat::ValuesChanged(void)
{
    bool Changed = false;
    if (m_hWnd != nullptr)
    {
        if (m_EditName != GetItemText(IDC_CODE_NAME) ||
            m_EditCode != GetItemText(IDC_CHEAT_CODES) ||
            m_EditOptions != GetItemText(IDC_CHEAT_OPTIONS) ||
            m_EditNotes != GetItemText(IDC_NOTES))
        {
            Changed = true;
        }
    }
    if (!Changed)
    {
        return false;
    }
    int Result = MessageBox(wGS(CHEAT_CHANGED_MSG).c_str(), wGS(CHEAT_CHANGED_TITLE).c_str(), MB_YESNOCANCEL);
    if (Result == IDCANCEL)
    {
        return true;
    }
    if (Result == IDYES)
    {
        CEnhancement Enhancement(CEnhancement::CheatIdent);
        if (ReadEnhancement(Enhancement) && (Enhancement.CodeOptionSize() == 0 || Enhancement.GetOptions().size() > 0))
        {
            SendMessage(WM_COMMAND, MAKELPARAM(IDC_ADD, 0));
        }
        else
        {
            MessageBox(wGS(MSG_CHEAT_INVALID_MSG).c_str(), wGS(MSG_CHEAT_INVALID_TITLE).c_str(), MB_ICONERROR);
            return true;
        }
    }
    return false;
}

std::string CEditCheat::GetItemText(int nIDDlgItem)
{
    CWindow Window = GetDlgItem(nIDDlgItem);
    int length = Window.GetWindowTextLength();
    if (length == 0)
    {
        return "";
    }

    std::wstring Result;
    Result.resize(length + 1);
    Window.GetWindowText((wchar_t *)Result.c_str(), Result.length());
    return stdstr().FromUTF16(Result.c_str());
}

CEnhancementCodeEx::CEnhancementCodeEx(CEnhancement * Enhancement) :
    m_Enhancement(Enhancement)
{
}

LRESULT CEnhancementCodeEx::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SetWindowText(wGS(CHEAT_CODE_EXT_TITLE).c_str());
    GetDlgItem(IDC_NOTE).SetWindowText(wGS(CHEAT_CODE_EXT_TXT).c_str());
    GetDlgItem(IDOK).SetWindowText(wGS(CHEAT_OK).c_str());
    GetDlgItem(IDCANCEL).SetWindowText(wGS(CHEAT_CANCEL).c_str());

    GetDlgItem(IDC_CHEAT_NAME).SetWindowText(stdstr(m_Enhancement->GetName()).ToUTF16().c_str());
    
    CListBox CheatList = GetDlgItem(IDC_CHEAT_LIST);
    CEnhancement::CodeOptions Options = m_Enhancement->GetOptions();
    bool OptionSelected = m_Enhancement->OptionSelected();
    uint16_t SelectedOption = m_Enhancement->SelectedOption();
    for (size_t i = 0, n = Options.size(); i < n; i++)
    {
        stdstr_f Option(m_Enhancement->CodeOptionSize() == 2 ? "$%02X %s" : "$%04X %s", Options[i].Value, Options[i].Name.c_str());
        int index = CheatList.AddString(Option.ToUTF16().c_str());
        if (index >= 0)
        {
            CheatList.SetItemData(index, Options[i].Value);
            if (OptionSelected && SelectedOption == Options[i].Value)
            {
                CheatList.SetCurSel(index);
            }
        }
    }
    return 0;
}

LRESULT CEnhancementCodeEx::OnListDblClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    PostMessage(WM_COMMAND, IDOK, 0);
    return 0;
}

LRESULT CEnhancementCodeEx::OnOkCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CListBox CheatList = GetDlgItem(IDC_CHEAT_LIST);
    int index = CheatList.GetCurSel();
    if (index < 0)
    {
        index = 0;
    }

    m_Enhancement->SetSelectedOption((uint16_t)(CheatList.GetItemData(index)));
    if (g_Enhancements != nullptr)
    {
        g_Enhancements->UpdateCheats();
    }
    EndDialog(0);
    return 0;
}

LRESULT CEnhancementCodeEx::OnCloseCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(0);
    return 0;
}
