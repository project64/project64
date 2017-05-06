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

#include "SettingsPage.h"

COptionsShortCutsPage::COptionsShortCutsPage(HWND hParent, const RECT & rcDispay) :
    m_EnableReset(false)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    SetDlgItemTextW(m_hWnd, IDC_S_CPU_STATE, wGS(ACCEL_CPUSTATE_TITLE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_MENU_ITEM_TEXT, wGS(ACCEL_MENUITEM_TITLE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_S_CURRENT_KEYS, wGS(ACCEL_CURRENTKEYS_TITLE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_S_SELECT_SHORT, wGS(ACCEL_SELKEY_TITLE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_S_CURRENT_ASSIGN, wGS(ACCEL_ASSIGNEDTO_TITLE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_ASSIGN, wGS(ACCEL_ASSIGN_BTN).c_str());
    SetDlgItemTextW(m_hWnd, IDC_REMOVE, wGS(ACCEL_REMOVE_BTN).c_str());
    SetDlgItemTextW(m_hWnd, IDC_KEY_PROMPT, wGS(ACCEL_DETECTKEY).c_str());

    m_CreateNewShortCut.AttachToDlgItem(m_hWnd, IDC_S_SELECT_SHORT);
    m_CpuState.Attach(GetDlgItem(IDC_C_CPU_STATE));
    m_MenuItems.Attach(GetDlgItem(IDC_MENU_ITEMS));
    m_CurrentKeys.Attach(GetDlgItem(IDC_CURRENT_KEYS));
    m_VirtualKeyList.Attach(GetDlgItem(IDC_VIRTUALKEY));

    m_MenuItems.ModifyStyle(0, TVS_SHOWSELALWAYS);

    m_CpuState.SetItemData(m_CpuState.AddStringW(wGS(ACCEL_CPUSTATE_1).c_str()), CMenuShortCutKey::RUNNING_STATE_NOT_RUNNING);
    m_CpuState.SetItemData(m_CpuState.AddStringW(wGS(ACCEL_CPUSTATE_3).c_str()), CMenuShortCutKey::RUNNING_STATE_WINDOWED);
    m_CpuState.SetItemData(m_CpuState.AddStringW(wGS(ACCEL_CPUSTATE_4).c_str()), CMenuShortCutKey::RUNNING_STATE_FULLSCREEN);
    m_CpuState.SetCurSel(0);

    int VirtualKeyListSize;
    VIRTUAL_KEY * VirtualKeyList = CMenuShortCutKey::VirtualKeyList(VirtualKeyListSize);
    for (int count = 0; count < VirtualKeyListSize; count++)
    {
        m_VirtualKeyList.SetItemData(m_VirtualKeyList.AddString(VirtualKeyList[count].Name), VirtualKeyList[count].Key);
    }

    OnCpuStateChanged(LBN_SELCHANGE, IDC_C_CPU_STATE, GetDlgItem(IDC_C_CPU_STATE));
    CheckResetEnable();
}

void COptionsShortCutsPage::CheckResetEnable(void)
{
    MSC_MAP & ShortCuts = m_ShortCuts.GetShortCuts();
    for (MSC_MAP::iterator Item = ShortCuts.begin(); Item != ShortCuts.end(); Item++)
    {
        const SHORTCUT_KEY_LIST & ShortCutList = Item->second.GetAccelItems();
        for (SHORTCUT_KEY_LIST::const_iterator ShortCut_item = ShortCutList.begin(); ShortCut_item != ShortCutList.end(); ShortCut_item++)
        {
            if (!ShortCut_item->Inactive() && !ShortCut_item->UserAdded())
            {
                continue;
            }
            m_EnableReset = true;
            return;
        }
    }
    m_EnableReset = false;
}

void COptionsShortCutsPage::OnCpuStateChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    RUNNING_STATE RunningState = (RUNNING_STATE)m_CpuState.GetItemData(m_CpuState.GetCurSel());

    MSC_MAP & ShortCuts = m_ShortCuts.GetShortCuts();
    m_MenuItems.DeleteAllItems();

    for (MSC_MAP::iterator Item = ShortCuts.begin(); Item != ShortCuts.end(); Item++)
    {
        if (!Item->second.Avaliable(RunningState))
        {
            continue;
        }

        //find Parent
        HTREEITEM hParent = m_MenuItems.GetChildItem(TVI_ROOT);
        while (hParent)
        {
            if (m_MenuItems.GetItemData(hParent) == (DWORD_PTR)Item->second.Section())
            {
                break;
            }
            hParent = m_MenuItems.GetNextSiblingItem(hParent);
        }

        if (hParent == NULL)
        {
            hParent = m_MenuItems.InsertItemW(TVIF_TEXT | TVIF_PARAM, wGS(Item->second.Section()).c_str(), 0, 0, 0, 0, Item->second.Section(), TVI_ROOT, TVI_LAST);
        }

        wstring str = wGS(Item->second.Title());
        std::wstring::size_type pos = str.find(L"&");
        while (pos != std::wstring::npos)
        {
            str.replace(pos, 1, L"");
            pos = str.find(L"&", pos);
        }
        pos = str.find(L"...");
        while (pos != std::wstring::npos)
        {
            str.replace(pos, 3, L"");
            pos = str.find(L"...", pos);
        }

        HTREEITEM hItem = m_MenuItems.InsertItemW(TVIF_TEXT | TVIF_PARAM, str.c_str(), 0, 0, 0, 0, (DWORD_PTR)&Item->second, hParent, TVI_LAST);

        const SHORTCUT_KEY_LIST & ShortCutList = Item->second.GetAccelItems();
        for (SHORTCUT_KEY_LIST::const_iterator ShortCut_item = ShortCutList.begin(); ShortCut_item != ShortCutList.end(); ShortCut_item++)
        {
            if (!ShortCut_item->Inactive() && !ShortCut_item->UserAdded())
            {
                continue;
            }
            m_MenuItems.SetItemState(hItem, TVIS_BOLD, TVIS_BOLD);
            m_MenuItems.SetItemState(hParent, TVIS_BOLD, TVIS_BOLD);
            break;
        }
    }
}

void COptionsShortCutsPage::OnRemoveClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    HTREEITEM hSelectedItem = m_MenuItems.GetSelectedItem();
    if (hSelectedItem == NULL)
    {
        g_Notify->DisplayError(GS(MSG_NO_SEL_SHORTCUT));
        return;
    }
    HTREEITEM hParent = m_MenuItems.GetParentItem(hSelectedItem);
    if (hParent == NULL)
    {
        g_Notify->DisplayError(GS(MSG_NO_SEL_SHORTCUT));
        return;
    }

    CShortCutItem * ShortCut = (CShortCutItem *)m_MenuItems.GetItemData(hSelectedItem);

    //Make sure an item is selected
    int index = m_CurrentKeys.GetCurSel();
    if (index < 0)
    {
        g_Notify->DisplayError(GS(MSG_NO_SEL_SHORTCUT));
        return;
    }
    ShortCut->RemoveItem((CMenuShortCutKey *)m_CurrentKeys.GetItemData(index));
    m_MenuItems.SetItemState(hSelectedItem, TVIS_BOLD, TVIS_BOLD);
    m_MenuItems.SetItemState(hParent, TVIS_BOLD, TVIS_BOLD);
    m_EnableReset = true;

    RefreshShortCutOptions(hSelectedItem);
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void COptionsShortCutsPage::OnDetectKeyClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)stInputGetKeys, this, 0, NULL));
}

void COptionsShortCutsPage::OnAssignClicked(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    //Get the virtual key info
    int index = m_VirtualKeyList.GetCurSel();
    if (index < 0)
    {
        g_Notify->DisplayError(GS(MSG_NO_SHORTCUT_SEL));
        return;
    }

    WORD key = (WORD)SendDlgItemMessage(IDC_VIRTUALKEY, CB_GETITEMDATA, index, 0);
    bool bCtrl = (SendDlgItemMessage(IDC_CTL, BM_GETCHECK, 0, 0) == BST_CHECKED);
    bool bAlt = (SendDlgItemMessage(IDC_ALT, BM_GETCHECK, 0, 0) == BST_CHECKED);
    bool bShift = (SendDlgItemMessage(IDC_SHIFT, BM_GETCHECK, 0, 0) == BST_CHECKED);

    RUNNING_STATE RunningState = (RUNNING_STATE)m_CpuState.GetItemData(m_CpuState.GetCurSel());

    HTREEITEM hSelectedItem = m_MenuItems.GetSelectedItem();
    if (hSelectedItem == NULL)
    {
        g_Notify->DisplayError(GS(MSG_NO_MENUITEM_SEL));
        return;
    }
    HTREEITEM hParent = m_MenuItems.GetParentItem(hSelectedItem);
    if (hParent == NULL)
    {
        g_Notify->DisplayError(GS(MSG_NO_MENUITEM_SEL));
        return;
    }

    CShortCutItem * ShortCut = (CShortCutItem *)m_MenuItems.GetItemData(hSelectedItem);
    LanguageStringID strid = m_ShortCuts.GetMenuItemName(key, bCtrl, bAlt, bShift, RunningState);
    if (strid != EMPTY_STRING)
    {
        g_Notify->DisplayError(GS(MSG_MENUITEM_ASSIGNED));
        return;
    }

    CMenuShortCutKey::ACCESS_MODE AccessMode = CMenuShortCutKey::ACCESS_NONE;
    switch (RunningState)
    {
    case CMenuShortCutKey::RUNNING_STATE_NOT_RUNNING: AccessMode = CMenuShortCutKey::ACCESS_GAME_NOT_RUNNING; break;
    case CMenuShortCutKey::RUNNING_STATE_WINDOWED: AccessMode = CMenuShortCutKey::ACCESS_GAME_RUNNING; break;
    case CMenuShortCutKey::RUNNING_STATE_FULLSCREEN: AccessMode = CMenuShortCutKey::ACCESS_GAME_RUNNING_FULLSCREEN; break;
    }
    ShortCut->AddShortCut(key, bCtrl, bAlt, bShift, AccessMode, true, false);
    m_MenuItems.SetItemState(hSelectedItem, TVIS_BOLD, TVIS_BOLD);
    m_MenuItems.SetItemState(hParent, TVIS_BOLD, TVIS_BOLD);
    m_EnableReset = true;

    RefreshShortCutOptions(hSelectedItem);
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void COptionsShortCutsPage::OnShortCutChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    //Get the virtual key info
    int index = m_VirtualKeyList.GetCurSel();
    if (index < 0) { return; }
    WORD key = (WORD)m_VirtualKeyList.GetItemData(index);
    bool bCtrl = (SendDlgItemMessage(IDC_CTL, BM_GETCHECK, 0, 0) == BST_CHECKED);
    bool bAlt = (SendDlgItemMessage(IDC_ALT, BM_GETCHECK, 0, 0) == BST_CHECKED);
    bool bShift = (SendDlgItemMessage(IDC_SHIFT, BM_GETCHECK, 0, 0) == BST_CHECKED);

    RUNNING_STATE RunningState = (RUNNING_STATE)m_CpuState.GetItemData(m_CpuState.GetCurSel());

    stdstr str = GS(m_ShortCuts.GetMenuItemName(key, bCtrl, bAlt, bShift, RunningState));
    if (str.length() > 0)
    {
        str.resize(std::remove(str.begin(), str.end(), '&') - str.begin());
    }
    else
    {
        str = "None";
    }
    SetDlgItemText(IDC_ASSIGNED_MENU_ITEM, str.c_str());
}

LRESULT COptionsShortCutsPage::OnMenuItemChanged(LPNMHDR lpnmh)
{
    RefreshShortCutOptions(((LPNMTREEVIEW)lpnmh)->itemNew.hItem);
    return true;
}

void COptionsShortCutsPage::RefreshShortCutOptions(HTREEITEM hItem)
{
    HTREEITEM hParent = m_MenuItems.GetParentItem(hItem);
    if (hParent == NULL)
    {
        return;
    }

    RUNNING_STATE RunningState = (RUNNING_STATE)m_CpuState.GetItemData(m_CpuState.GetCurSel());
    CShortCutItem * ShortCut = (CShortCutItem *)m_MenuItems.GetItemData(hItem);

    m_CurrentKeys.ResetContent();

    const SHORTCUT_KEY_LIST & ShortCutList = ShortCut->GetAccelItems();
    for (SHORTCUT_KEY_LIST::const_iterator ShortCut_item = ShortCutList.begin(); ShortCut_item != ShortCutList.end(); ShortCut_item++)
    {
        if (ShortCut_item->Inactive() || !ShortCut_item->Active(RunningState))
        {
            continue;
        }

        stdstr Name = ShortCut_item->Name();
        m_CurrentKeys.SetItemData(m_CurrentKeys.AddString(Name.c_str()), (DWORD_PTR)&*ShortCut_item);
    }
}

BOOL CALLBACK KeyPromptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            SetForegroundWindow(GetParent(hDlg));
            DestroyWindow(hDlg);
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void COptionsShortCutsPage::InputGetKeys(void)
{
    HWND hKeyDlg = CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Key_Prompt), m_hWnd, (DLGPROC)KeyPromptDlgProc, (LPARAM)::GetDlgItem(m_hWnd, IDC_VIRTUALKEY));
    ::EnableWindow(GetParent(), false);
    MSG msg;

    for (bool fDone = false; !fDone; MsgWaitForMultipleObjects(0, NULL, false, 45, QS_ALLINPUT)) {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                fDone = true;
                ::PostMessage(NULL, WM_QUIT, 0, 0);
                break;
            }
            if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {
                int nVirtKey = (int)msg.wParam;
                if (nVirtKey == VK_SHIFT) { continue; }
                if (nVirtKey == VK_CONTROL) { continue; }
                if (nVirtKey == VK_MENU) { continue; }
                SendDlgItemMessage(IDC_VIRTUALKEY, CB_SETCURSEL, (WPARAM)-1, 0);
                for (int count = 0; count < SendDlgItemMessage(IDC_VIRTUALKEY, CB_GETCOUNT, 0, 0); count++) {
                    int Data = (int)SendDlgItemMessage(IDC_VIRTUALKEY, CB_GETITEMDATA, count, 0);
                    if (Data != nVirtKey) { continue; }
                    SendDlgItemMessage(IDC_VIRTUALKEY, CB_SETCURSEL, count, 0);
                    SendDlgItemMessage(IDC_CTL, BM_SETCHECK, (GetKeyState(VK_CONTROL) & 0x80) != 0 ? BST_CHECKED : BST_UNCHECKED, 0);
                    SendDlgItemMessage(IDC_ALT, BM_SETCHECK, (GetKeyState(VK_MENU) & 0x80) != 0 ? BST_CHECKED : BST_UNCHECKED, 0);
                    SendDlgItemMessage(IDC_SHIFT, BM_SETCHECK, (GetKeyState(VK_SHIFT) & 0x80) != 0 ? BST_CHECKED : BST_UNCHECKED, 0);
                    SendMessage(WM_COMMAND, MAKELPARAM(IDC_VIRTUALKEY, LBN_SELCHANGE), (LPARAM)::GetDlgItem(m_hWnd, IDC_VIRTUALKEY));
                    SetForegroundWindow(GetParent());
                    ::DestroyWindow(hKeyDlg);
                }
                continue;
            }
            if (!::IsDialogMessage(hKeyDlg, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (!::IsWindow(hKeyDlg)) { fDone = true; }
    }
    ::SetFocus(GetParent());
    ::EnableWindow(GetParent(), true);
}

void COptionsShortCutsPage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void COptionsShortCutsPage::ShowPage()
{
    ShowWindow(SW_SHOW);
}

void COptionsShortCutsPage::ApplySettings(bool /*UpdateScreen*/)
{
    m_ShortCuts.Save();
    UISettingsSaveBool(Info_ShortCutsChanged, true);
}

bool COptionsShortCutsPage::EnableReset(void)
{
    return m_EnableReset;
}

void COptionsShortCutsPage::ResetPage()
{
    m_EnableReset = false;
    m_ShortCuts.Load(true);
    OnCpuStateChanged(LBN_SELCHANGE, IDC_C_CPU_STATE, GetDlgItem(IDC_C_CPU_STATE));
    SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    m_CurrentKeys.ResetContent();
    CSettingsPageImpl<COptionsShortCutsPage>::ResetPage();
}