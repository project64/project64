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
#include <Project64-core/Settings/SettingType/SettingsType-Cheats.h>
#include <Project64-core/N64System/CheatClass.h>

CCheatsUI::CCheatsUI(void) :
    m_EditCheat(m_SelectCheat),
    m_SelectCheat(m_EditCheat)
{
}

CCheatsUI::~CCheatsUI()
{
}

void CCheatsUI::Display(HWND hParent, bool BlockExecution)
{
    if (g_BaseSystem)
    {
        g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_Cheats);
    }
    if (BlockExecution)
    {
        DoModal(hParent);
    }
    else if (m_hWnd != NULL)
    {
        SetFocus();
    }
    else
    {
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
    return 0;
}

LRESULT CCheatsUI::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_StateBtn.Detach();
    return 0;
}

LRESULT CCheatsUI::OnCloseCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
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

std::string CCheatsUI::GetCheatName(int CheatNo, bool AddExtension)
{
    if (CheatNo > CCheats::MaxCheats) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    stdstr LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, CheatNo);
    if (LineEntry.length() == 0) { return LineEntry; }

    //Find the start and end of the name which is surrounded in ""
    int StartOfName = LineEntry.find("\"");
    if (StartOfName == -1) { return ""; }
    int EndOfName = LineEntry.find("\"", StartOfName + 1);
    if (EndOfName == -1) { return ""; }

    stdstr Name = LineEntry.substr(StartOfName + 1, EndOfName - StartOfName - 1);
    const char * CodeString = &(LineEntry.c_str())[EndOfName + 2];
    if (!CCheats::IsValid16BitCode(CodeString))
    {
        Name.Format("*** %s", Name.c_str());
        Name.Replace("\\", "\\*** ");
    }
    if (AddExtension && CheatUsesCodeExtensions(LineEntry))
    {
        stdstr CheatValue(g_Settings->LoadStringIndex(Cheat_Extension, CheatNo));
        Name.Format("%s (=>%s)", Name.c_str(), CheatValue.c_str());
    }
    return Name;
}

bool CCheatsUI::CheatUsesCodeExtensions(const std::string & LineEntry)
{
    //Find the start and end of the name which is surronded in ""
    if (LineEntry.length() == 0){ return false; }
    int StartOfName = LineEntry.find("\"");
    if (StartOfName == -1) { return false; }
    int EndOfName = LineEntry.find("\"", StartOfName + 1);
    if (EndOfName == -1) { return false; }

    //Read through the gameshark entries till you find a ??
    const char *ReadPos = &(LineEntry.c_str())[EndOfName + 2];
    bool CodeExtension = false;

    for (int i = 0; i < CCheats::MaxGSEntries && CodeExtension == false; i++)
    {
        if (strchr(ReadPos, ' ') == NULL) { break; }
        ReadPos = strchr(ReadPos, ' ') + 1;
        if (ReadPos[0] == '?' && ReadPos[1] == '?') { CodeExtension = true; }
        if (ReadPos[2] == '?' && ReadPos[3] == '?') { CodeExtension = true; }
        if (strchr(ReadPos, ',') == NULL) { continue; }
        ReadPos = strchr(ReadPos, ',') + 1;
    }
    return CodeExtension;
}

CCheatList::CCheatList(CEditCheat & EditCheat) :
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
    if (!m_hCheatTree.GetItem(&item))
    {
        return 0;
    }

    std::string LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, item.lParam);
    if (!CCheatsUI::CheatUsesCodeExtensions(LineEntry))
    {
        return 0;
    }

    std::string Options;
    if (g_Settings->LoadStringIndex(Cheat_Options, item.lParam, Options) && Options.length() > 0)
    {
        CCheatsCodeEx(item.lParam).DoModal();
    }

    std::string CheatName(CCheatsUI::GetCheatName(item.lParam, true));
    const char * Cheat = strrchr(CheatName.c_str(), '\\');
    if (Cheat == NULL)
    {
        Cheat = CheatName.c_str();
    }
    else
    {
        Cheat += 1;
    }
    std::wstring wcCheat = stdstr(Cheat).ToUTF16();
    item.mask = TVIF_TEXT;
    item.pszText = (LPWSTR)wcCheat.c_str();
    item.cchTextMax = CheatName.length();
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
    if (g_BaseSystem)
    {
        g_BaseSystem->SetCheatsSlectionChanged(true);
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
                std::string LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, item.lParam);
                if (CCheatsUI::CheatUsesCodeExtensions(LineEntry))
                {
                    std::string CheatExtension;
                    if (!g_Settings->LoadStringIndex(Cheat_Extension, item.lParam, CheatExtension))
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

        if (g_BaseSystem)
        {
            g_BaseSystem->SetCheatsSlectionChanged(true);
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
    if (m_hCheatTree.GetChildItem(hItem) == NULL)
    {
        TVITEM item = { 0 };
        item.mask = TVIF_PARAM;
        item.hItem = hItem;
        m_hCheatTree.GetItem(&item);

        stdstr Notes(g_Settings->LoadStringIndex(Cheat_Notes, item.lParam));
        GetDlgItem(IDC_NOTES).SetWindowText(Notes.ToUTF16().c_str());
        if (m_EditCheat.m_hWnd != NULL)
        {
            m_EditCheat.SendMessage(CEditCheat::WM_EDITCHEAT, item.lParam, 0);
        }
    }
    else
    {
        GetDlgItem(IDC_NOTES).SetWindowText(L"");
    }
    return 0;
}

void CCheatList::RefreshItems()
{
    if (m_hWnd == NULL) { return; }

    m_DeleteingEntries = true;
    TreeView_DeleteAllItems(m_hCheatTree);
    m_DeleteingEntries = false;
    for (int i = 0; i < CCheats::MaxCheats; i++)
    {
        std::string Name = CCheatsUI::GetCheatName(i, true);
        if (Name.length() == 0) { break; }

        AddCodeLayers(i, stdstr(Name).ToUTF16(), TVI_ROOT, g_Settings->LoadBoolIndex(Cheat_Active, i) != 0);
    }
}

void CCheatList::AddCodeLayers(int CheatNumber, const std::wstring &CheatName, HTREEITEM hParent, bool CheatActive)
{
    TV_INSERTSTRUCT tv;

    wchar_t Text[500], Item[500];
    if (CheatName.length() > (sizeof(Text) - 5)) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    wcscpy(Text, CheatName.c_str());
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
            if (StartPos < CheatName.length())
            {
                TempCheatName = CheatName.substr(StartPos);
            }
            AddCodeLayers(CheatNumber, TempCheatName, tv.item.hItem, CheatActive);
            return;
        }
        tv.item.hItem = TreeView_GetNextSibling(m_hCheatTree, tv.item.hItem);
    }

    tv.hInsertAfter = TVI_SORT;
    tv.item.mask = TVIF_TEXT | TVIF_PARAM;
    tv.item.pszText = Text;
    tv.item.lParam = CheatNumber;
    tv.hParent = hParent;
    hParent = TreeView_InsertItem(m_hCheatTree, &tv);
    TV_SetCheckState(hParent, CheatActive ? TV_STATE_CHECKED : TV_STATE_CLEAR);

    if (wcscmp(Text, CheatName.c_str()) == 0) { return; }
    AddCodeLayers(CheatNumber, CheatName.substr(wcslen(Text) + 1), hParent, CheatActive);
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

        if (Checked)
        {
            std::string LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, item.lParam);
            if (CCheatsUI::CheatUsesCodeExtensions(LineEntry))
            {
                std::string CheatExten;
                if (!g_Settings->LoadStringIndex(Cheat_Extension, item.lParam, CheatExten) || CheatExten.empty())
                {
                    return;
                }
            }
        }

        TV_SetCheckState(hParent, Checked ? TV_STATE_CHECKED : TV_STATE_CLEAR);
        g_Settings->SaveBoolIndex(Cheat_Active, item.lParam, Checked);
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

void CCheatList::DeleteCheat(int Index)
{
    for (int CheatNo = Index; CheatNo < CCheats::MaxCheats; CheatNo++)
    {
        stdstr LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, CheatNo + 1);
        if (LineEntry.empty())
        {
            g_Settings->DeleteSettingIndex(Cheat_Options, CheatNo);
            g_Settings->DeleteSettingIndex(Cheat_Notes, CheatNo);
            g_Settings->DeleteSettingIndex(Cheat_Extension, CheatNo);
            g_Settings->DeleteSettingIndex(Cheat_Entry, CheatNo);
            g_Settings->DeleteSettingIndex(Cheat_Active, CheatNo);
            break;
        }
        stdstr Value;
        if (g_Settings->LoadStringIndex(Cheat_Options, CheatNo + 1, Value))
        {
            g_Settings->SaveStringIndex(Cheat_Options, CheatNo, Value);
        }
        else
        {
            g_Settings->DeleteSettingIndex(Cheat_Options, CheatNo);
        }

        if (g_Settings->LoadStringIndex(Cheat_Notes, CheatNo + 1, Value))
        {
            g_Settings->SaveStringIndex(Cheat_Notes, CheatNo, Value);
        }
        else
        {
            g_Settings->DeleteSettingIndex(Cheat_Notes, CheatNo);
        }

        if (g_Settings->LoadStringIndex(Cheat_Extension, CheatNo + 1, Value))
        {
            g_Settings->SaveStringIndex(Cheat_Extension, CheatNo, Value);
        }
        else
        {
            g_Settings->DeleteSettingIndex(Cheat_Extension, CheatNo);
        }

        bool bValue;
        if (g_Settings->LoadBoolIndex(Cheat_Active, CheatNo + 1, bValue))
        {
            g_Settings->SaveBoolIndex(Cheat_Active, CheatNo, bValue);
        }
        else
        {
            g_Settings->DeleteSettingIndex(Cheat_Active, CheatNo);
        }
        g_Settings->SaveStringIndex(Cheat_Entry, CheatNo, LineEntry);
    }
    CSettingTypeCheats::FlushChanges();
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

    MENUITEMINFOW MenuInfo = { 0 };
    wchar_t String[256];
    MenuInfo.cbSize = sizeof(MENUITEMINFO);
    MenuInfo.fMask = MIIM_TYPE;
    MenuInfo.fType = MFT_STRING;
    MenuInfo.fState = MFS_ENABLED;
    MenuInfo.dwTypeData = String;
    MenuInfo.cch = 256;

    GetMenuItemInfoW(hMenu, MenuPos, true, &MenuInfo);
    wcscpy(String, Title);
    if (wcschr(String, '\t') != NULL) { *(wcschr(String, '\t')) = '\0'; }
    if (ShortCut) { _swprintf(String, L"%s\t%s", String, ShortCut); }
    SetMenuItemInfoW(hMenu, MenuPos, true, &MenuInfo);
}

CEditCheat::CEditCheat(CCheatList & CheatList) :
    m_CheatList(CheatList),
    m_EditCheat(-1)
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
    RecordCheatValues();
    return 0;
}

LRESULT CEditCheat::OnEditCheat(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_EditCheat = (int32_t)wParam;
    if (m_EditCheat < 0)
    {
        return 0;
    }

    if (CheatChanged())
    {
        return 0;
    }

    std::string CheatEntryStr = g_Settings->LoadStringIndex(Cheat_Entry, m_EditCheat);
    int32_t len = strrchr(CheatEntryStr.c_str(), '"') - strchr(CheatEntryStr.c_str(), '"') - 1;
    stdstr CheatName(strchr(CheatEntryStr.c_str(), '"') + 1);
    CheatName.resize(len);
    GetDlgItem(IDC_CODE_NAME).SetWindowText(CheatName.ToUTF16().c_str());

    const char * ReadPos = strrchr(CheatEntryStr.c_str(), '"') + 2;
    stdstr Buffer;
    do
    {
        const char * End = strchr(ReadPos, ',');
        if (End)
        {
            Buffer.append(ReadPos, End - ReadPos);
        }
        else
        {
            Buffer.append(ReadPos);
        }

        ReadPos = strchr(ReadPos, ',');
        if (ReadPos != NULL)
        {
            Buffer.append("\r\n");
            ReadPos += 1;
        }
    } while (ReadPos);
    GetDlgItem(IDC_CHEAT_CODES).SetWindowText(Buffer.ToUTF16().c_str());

    //Add option values to screen
    std::string CheatOptionStr = g_Settings->LoadStringIndex(Cheat_Options, m_EditCheat);
    ReadPos = strchr(CheatOptionStr.c_str(), '$');
    Buffer.erase();
    if (ReadPos)
    {
        ReadPos += 1;
        do
        {
            const char * End = strchr(ReadPos, ',');
            if (End)
            {
                Buffer.append(ReadPos, End - ReadPos);
            }
            else
            {
                Buffer.append(ReadPos);
            }
            ReadPos = strchr(ReadPos, '$');
            if (ReadPos != NULL)
            {
                Buffer.append("\r\n");
                ReadPos += 1;
            }
        } while (ReadPos);
    }
    GetDlgItem(IDC_CHEAT_OPTIONS).SetWindowText(Buffer.ToUTF16().c_str());

    stdstr CheatNotesStr = g_Settings->LoadStringIndex(Cheat_Notes, m_EditCheat);
    GetDlgItem(IDC_NOTES).SetWindowText(CheatNotesStr.ToUTF16().c_str());

    SendMessage(WM_COMMAND, MAKELPARAM(IDC_CHEAT_CODES, EN_CHANGE), (LPARAM)(HWND)GetDlgItem(IDC_CHEAT_CODES));
    GetDlgItem(IDC_ADD).SetWindowTextW(wGS(CHEAT_EDITCHEAT_UPDATE).c_str());
    RecordCheatValues();
    return 0;
}

LRESULT CEditCheat::OnAddCheat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    std::string NewCheatName = GetItemText(IDC_CODE_NAME);
    int i = 0;
    for (i = 0; i < CCheats::MaxCheats; i++)
    {
        if (m_EditCheat == i)
        {
            continue;
        }
        std::string CheatName = CCheatsUI::GetCheatName(i, false);
        if (CheatName.length() == 0)
        {
            if (m_EditCheat < 0)
            {
                m_EditCheat = i;
            }
            break;
        }
        if (_stricmp(CheatName.c_str(), NewCheatName.c_str()) == 0)
        {
            g_Notify->DisplayWarning(GS(MSG_CHEAT_NAME_IN_USE));
            GetDlgItem(IDC_CODE_NAME).SetFocus();
            return true;
        }
    }
    if (m_EditCheat < 0 && i == CCheats::MaxCheats)
    {
        g_Notify->DisplayError(GS(MSG_MAX_CHEATS));
        return true;
    }

    bool validcodes, validoptions, nooptions;
    CodeFormat Format;
    stdstr_f Cheat("\"%s\"%s", NewCheatName.c_str(), ReadCodeString(validcodes, validoptions, nooptions, Format).c_str());
    stdstr Options = ReadOptionsString(validoptions, Format);

    g_Settings->SaveStringIndex(Cheat_Entry, m_EditCheat, Cheat.c_str());
    g_Settings->SaveStringIndex(Cheat_Notes, m_EditCheat, GetItemText(IDC_NOTES));
    g_Settings->SaveStringIndex(Cheat_Options, m_EditCheat, Options);
    RecordCheatValues();
    CSettingTypeCheats::FlushChanges();
    m_CheatList.RefreshItems();
    return 0;
}

LRESULT CEditCheat::OnNewCheat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (CheatChanged())
    {
        return 0;
    }
    m_EditCheat = -1;
    SetDlgItemText(IDC_CODE_NAME, L"");
    SetDlgItemText(IDC_CHEAT_CODES, L"");
    SetDlgItemText(IDC_CHEAT_OPTIONS, L"");
    SetDlgItemText(IDC_NOTES, L"");
    GetDlgItem(IDC_ADD).EnableWindow(false);
    GetDlgItem(IDC_CHEAT_OPTIONS).EnableWindow(false);
    RecordCheatValues();
    return 0;
}

LRESULT CEditCheat::OnCodeNameChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool validcodes, validoptions, nooptions;
    CodeFormat  Format;
    ReadCodeString(validcodes, validoptions, nooptions, Format);
    if (!nooptions)
    {
        ReadOptionsString(validoptions, Format);
    }

    bool CanAdd = validcodes && (validoptions || nooptions) && GetDlgItem(IDC_CODE_NAME).GetWindowTextLength() > 0;
    GetDlgItem(IDC_ADD).EnableWindow(CanAdd);
    return 0;
}

LRESULT CEditCheat::OnCheatCodeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool validcodes, validoptions, nooptions;
    CodeFormat Format;
    ReadCodeString(validcodes, validoptions, nooptions, Format);

    if (Format > 0 && !GetDlgItem(IDC_LABEL_OPTIONS).IsWindowEnabled())
    {
        GetDlgItem(IDC_LABEL_OPTIONS).EnableWindow(true);
        GetDlgItem(IDC_LABEL_OPTIONS_FORMAT).EnableWindow(true);
        GetDlgItem(IDC_CHEAT_OPTIONS).EnableWindow(true);
    }
    if (Format <= 0 && GetDlgItem(IDC_LABEL_OPTIONS).IsWindowEnabled())
    {
        GetDlgItem(IDC_LABEL_OPTIONS).EnableWindow(false);
        GetDlgItem(IDC_LABEL_OPTIONS_FORMAT).EnableWindow(false);
        GetDlgItem(IDC_CHEAT_OPTIONS).EnableWindow(false);
    }

    if (!nooptions)
    {
        ReadOptionsString(validoptions, Format);
    }

    bool CanAdd = validcodes && (validoptions || nooptions) && GetDlgItem(IDC_CODE_NAME).GetWindowTextLength() > 0;
    GetDlgItem(IDC_ADD).EnableWindow(CanAdd);
    return 0;
}

LRESULT CEditCheat::OnCheatOptionsChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool ValidCodes, ValidOptions, NoOptions;
    CodeFormat Format;
    ReadCodeString(ValidCodes, ValidOptions, NoOptions, Format);
    if (!NoOptions)
    {
        ReadOptionsString(ValidOptions, Format);
    }

    bool CanAdd = ValidCodes && (ValidOptions || NoOptions) && GetDlgItem(IDC_CODE_NAME).GetWindowTextLength() > 0;
    GetDlgItem(IDC_ADD).EnableWindow(CanAdd);
    return 0;
}

std::string CEditCheat::ReadCodeString(bool &ValidCodes, bool &ValidOptions, bool &NoOptions, CodeFormat & Format)
{
    const char * FormatNormal = "XXXXXXXX XXXX";
    const char * FormatOptionLB = "XXXXXXXX XX??";
    const char * FormatOptionW = "XXXXXXXX ????";

    ValidCodes = true;
    ValidOptions = true;
    NoOptions = true;
    Format = CodeFormat_Invalid;

    CEdit CheatCodes(GetDlgItem(IDC_CHEAT_CODES));
    int NumLines = CheatCodes.GetLineCount();
    if (NumLines == 0)
    {
        ValidCodes = false;
    }

    std::string CodeString;
    int NumCodes = 0;
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
        char TempFormat[128] = { 0 };
        for (size_t c = 0; c < str.length(); c++)
        {
            if (isxdigit(str[c]))
            {
                TempFormat[c] = 'X';
            }
            else if ((str[c] == ' ') || (str[c] == '?'))
            {
                TempFormat[c] = str[c];
            }
            else
            {
                TempFormat[c] = '#';
            }
        }

        if (strcmp(TempFormat, FormatNormal) == 0)
        {
            CodeString += ",";
            CodeString += str.c_str();
            NumCodes += 1;
            if (Format == CodeFormat_Invalid)
            {
                Format = CodeFormat_Normal;
            }
        }
        else if (strcmp(TempFormat, FormatOptionLB) == 0)
        {
            if (Format != CodeFormat_Word)
            {
                CodeString += ",";
                CodeString += str.c_str();
                NumCodes += 1;
                Format = CodeFormat_LowerByte;
                NoOptions = false;
                ValidOptions = false;
            }
            else
            {
                ValidCodes = false;
            }
        }
        else if (strcmp(TempFormat, FormatOptionW) == 0)
        {
            if (Format != CodeFormat_LowerByte)
            {
                CodeString += ",";
                CodeString += str.c_str();
                NumCodes += 1;
                Format = CodeFormat_Word;
                NoOptions = false;
                ValidOptions = false;
            }
            else
            {
                ValidCodes = false;
            }
        }
        else
        {
            ValidCodes = false;
        }
    }

    if (CodeString.length() == 0)
    {
        ValidCodes = false;
    }
    return CodeString;
}

std::string CEditCheat::ReadOptionsString(bool &validoptions, CodeFormat Format)
{
    validoptions = true;

    CEdit CheatOptions(GetDlgItem(IDC_CHEAT_OPTIONS));
    int NumLines = CheatOptions.GetLineCount();
    int NumOptions = 0;
    std::string OptionsStr;

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

        switch (Format)
        {
        case CodeFormat_LowerByte:
            if (len >= 2) {
                for (int c = 0; c < 2; c++)
                {
                    if (!isxdigit(str[c]))
                    {
                        validoptions = false;
                        break;
                    }
                }

                if ((str[2] != ' ') && (len > 2))
                {
                    validoptions = false;
                    break;
                }

                for (int c = 0; c < 2; c++)
                {
                    str[c] = (char)toupper(str[c]);
                }

                OptionsStr += OptionsStr.empty() ? "$" : ",$";
                OptionsStr += str.c_str();
                NumOptions += 1;
            }
            else
            {
                validoptions = false;
                break;
            }
            break;
        case CodeFormat_Word:
            if (len >= 4)
            {
                for (int c = 0; c < 4; c++)
                {
                    if (!isxdigit(str[c]))
                    {
                        validoptions = false;
                        break;
                    }
                }

                if (str[4] != ' ' && (len > 4))
                {
                    validoptions = false;
                    break;
                }

                for (int c = 0; c < 4; c++)
                {
                    str[c] = (char)toupper(str[c]);
                }

                OptionsStr += OptionsStr.empty() ? "$" : ",$";
                OptionsStr += str.c_str();
                NumOptions += 1;
            }
            else
            {
                validoptions = false;
                break;
            }
            break;
        }
    }

    if (NumOptions < 1)
    {
        validoptions = false;
    }
    return OptionsStr;
}

void CEditCheat::RecordCheatValues(void)
{
    m_EditName = GetItemText(IDC_CODE_NAME);
    m_EditCode = GetItemText(IDC_CHEAT_CODES);
    m_EditOptions = GetItemText(IDC_CHEAT_OPTIONS);
    m_EditNotes = GetItemText(IDC_NOTES);
}

bool CEditCheat::CheatChanged(void)
{
    bool Changed = false;
    if (m_EditName != GetItemText(IDC_CODE_NAME) ||
        m_EditCode != GetItemText(IDC_CHEAT_CODES) ||
        m_EditOptions != GetItemText(IDC_CHEAT_OPTIONS) ||
        m_EditNotes != GetItemText(IDC_NOTES))
    {
        Changed = true;
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
        SendMessage(WM_COMMAND, MAKELPARAM(IDC_ADD, 0));
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

CCheatsCodeEx::CCheatsCodeEx(int EditCheat) :
    m_EditCheat(EditCheat)
{
}

LRESULT CCheatsCodeEx::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SetWindowText(wGS(CHEAT_CODE_EXT_TITLE).c_str());
    GetDlgItem(IDC_NOTE).SetWindowText(wGS(CHEAT_CODE_EXT_TXT).c_str());
    GetDlgItem(IDOK).SetWindowText(wGS(CHEAT_OK).c_str());
    GetDlgItem(IDCANCEL).SetWindowText(wGS(CHEAT_CANCEL).c_str());

    stdstr CheatName = CCheatsUI::GetCheatName(m_EditCheat, false);
    GetDlgItem(IDC_CHEAT_NAME).SetWindowText(CheatName.ToUTF16().c_str());

    std::string Options(g_Settings->LoadStringIndex(Cheat_Options, m_EditCheat));
    std::string CurrentExt(g_Settings->LoadStringIndex(Cheat_Extension, m_EditCheat));
    const char * ReadPos = Options.c_str();
    CListBox CheatList = GetDlgItem(IDC_CHEAT_LIST);
    while (*ReadPos != 0)
    {
        const char * NextComma = strchr(ReadPos, ',');
        int len = NextComma == NULL ? strlen(ReadPos) : NextComma - ReadPos;
        stdstr CheatExt(ReadPos);
        CheatExt.resize(len);
        int index = CheatList.AddString(CheatExt.ToUTF16().c_str());
        if (CheatExt == CurrentExt)
        {
            CheatList.SetCurSel(index);
        }
        ReadPos = NextComma ? NextComma + 1 : ReadPos + strlen(ReadPos);
    }
    return 0;
}

LRESULT CCheatsCodeEx::OnListDblClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    PostMessage(WM_COMMAND, IDOK, 0);
    return 0;
}

LRESULT CCheatsCodeEx::OnOkCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CListBox CheatList = GetDlgItem(IDC_CHEAT_LIST);
    int index = CheatList.GetCurSel();
    if (index < 0)
    {
        index = 0;
    }

    int length = CheatList.GetTextLen(index);
    std::wstring CheatExten;
    CheatExten.resize(length + 1);
    CheatList.GetText(index, (wchar_t *)CheatExten.c_str());

    g_Settings->SaveStringIndex(Cheat_Extension, m_EditCheat, stdstr().FromUTF16(CheatExten.c_str()).c_str());
    if (g_BaseSystem)
    {
        g_BaseSystem->SetCheatsSlectionChanged(true);
    }
    EndDialog(0);
    return 0;
}

LRESULT CCheatsCodeEx::OnCloseCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(0);
    return 0;
}
