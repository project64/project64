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

extern CCheatsUI * g_cheatUI = NULL;

enum
{
    WM_EDITCHEAT = WM_USER + 0x120,
    UM_CHANGECODEEXTENSION = WM_USER + 0x121,
};

CCheatsUI::CCheatsUI(void) :
m_rcList(new RECT),
m_rcAdd(new RECT),
m_EditCheat(-1),
m_DeleteingEntries(false)
{
    m_Window = NULL;
    m_hSelectCheat = NULL;
    m_AddCheat = NULL;
    m_hCheatTree = NULL;
}

CCheatsUI::~CCheatsUI()
{
    delete m_rcList;
    delete m_rcAdd;
}

void CCheatsUI::AddCodeLayers(int CheatNumber, const stdstr &CheatName, HWND hParent, bool CheatActive)
{
    TV_INSERTSTRUCT tv;

    //Work out text to add
    wchar_t Text[500], Item[500];
    if (CheatName.length() > (sizeof(Text) - 5)) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    wcscpy(Text, CheatName.ToUTF16().c_str());
    if (wcschr(Text, L'\\') > 0) { *wcschr(Text, L'\\') = 0; }

    //See if text is already added
    tv.item.mask = TVIF_TEXT;
    tv.item.pszText = Item;
    tv.item.cchTextMax = sizeof(Item);
    tv.item.hItem = TreeView_GetChild(m_hCheatTree, hParent);
    while (tv.item.hItem)
    {
        TreeView_GetItem(m_hCheatTree, &tv.item);
        if (wcscmp(Text, Item) == 0)
        {
            //If already exists then just use existing one
            int State = TV_GetCheckState(m_hCheatTree, (HWND)tv.item.hItem);
            if ((CheatActive && State == TV_STATE_CLEAR) || (!CheatActive && State == TV_STATE_CHECKED))
            {
                TV_SetCheckState(m_hCheatTree, (HWND)tv.item.hItem, TV_STATE_INDETERMINATE);
            }
            size_t StartPos = wcslen(Text) + 1;
            stdstr TempCheatName;
            if (StartPos < CheatName.length())
            {
                TempCheatName = CheatName.substr(StartPos);
            }
            AddCodeLayers(CheatNumber, TempCheatName, (HWND)tv.item.hItem, CheatActive);
            return;
        }
        tv.item.hItem = TreeView_GetNextSibling(m_hCheatTree, tv.item.hItem);
    }

    //Add to dialog
    tv.hInsertAfter = TVI_SORT;
    tv.item.mask = TVIF_TEXT | TVIF_PARAM;
    tv.item.pszText = Text;
    tv.item.lParam = CheatNumber;
    tv.hParent = (HTREEITEM)hParent;
    hParent = (HWND)TreeView_InsertItem(m_hCheatTree, &tv);
    TV_SetCheckState(m_hCheatTree, hParent, CheatActive ? TV_STATE_CHECKED : TV_STATE_CLEAR);

    if (wcscmp(Text, CheatName.ToUTF16().c_str()) == 0) { return; }
    AddCodeLayers(CheatNumber, (stdstr)(CheatName.substr(wcslen(Text) + 1)), hParent, CheatActive);
}

void CCheatsUI::RefreshCheatManager()
{
    if (m_Window == NULL) { return; }

    m_DeleteingEntries = true;
    TreeView_DeleteAllItems(m_hCheatTree);
    m_DeleteingEntries = false;
    for (int i = 0; i < CCheats::MaxCheats; i++)
    {
        stdstr Name = GetCheatName(i, true);
        if (Name.length() == 0) { break; }

        AddCodeLayers(i, Name, (HWND)TVI_ROOT, g_Settings->LoadBoolIndex(Cheat_Active, i) != 0);
    }
}

stdstr CCheatsUI::GetDlgItemStr(HWND hDlg, int nIDDlgItem)
{
    HWND hDlgItem = GetDlgItem(hDlg, nIDDlgItem);
    int length = SendMessage(hDlgItem, WM_GETTEXTLENGTH, 0, 0);
    if (length == 0)
    {
        return "";
    }

    std::wstring Result;
    Result.resize(length + 1);

    GetWindowText(hDlgItem, (wchar_t *)Result.c_str(), Result.length());
    return stdstr().FromUTF16(Result.c_str());
}

void CCheatsUI::SelectCheats(HWND hParent, bool BlockExecution)
{
    if (g_BaseSystem)
    {
        g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_Cheats);
    }
    if (m_Window != NULL)
    {
        SetForegroundWindow(m_Window);
        return;
    }
    if (hParent)
    {
        if (BlockExecution)
        {
            DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_Cheats_Select), hParent, (DLGPROC)ManageCheatsProc, (LPARAM)this);
        }
        else
        {
            CreateDialogParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_Cheats_Select), hParent, (DLGPROC)ManageCheatsProc, (LPARAM)this);
        }
    }
}

bool CCheatsUI::CheatChanged(HWND hDlg)
{
    bool Changed = false;
    if (m_EditName != GetDlgItemStr(hDlg, IDC_CODE_NAME) ||
        m_EditCode != GetDlgItemStr(hDlg, IDC_CHEAT_CODES) ||
        m_EditOptions != GetDlgItemStr(hDlg, IDC_CHEAT_OPTIONS) ||
        m_EditNotes != GetDlgItemStr(hDlg, IDC_NOTES))
    {
        Changed = true;
    }
    if (!Changed)
    {
        return false;
    }
    int Result = MessageBoxW(hDlg, wGS(CHEAT_CHANGED_MSG).c_str(), wGS(CHEAT_CHANGED_TITLE).c_str(), MB_YESNOCANCEL);
    if (Result == IDCANCEL)
    {
        return true;
    }
    if (Result == IDYES)
    {
        SendMessage(hDlg, WM_COMMAND, MAKELPARAM(IDC_ADD, 0), (LPARAM)GetDlgItem(hDlg, IDC_ADD));
    }
    return false;
}

void CCheatsUI::RecordCheatValues(HWND hDlg)
{
    m_EditName = GetDlgItemStr(hDlg, IDC_CODE_NAME);
    m_EditCode = GetDlgItemStr(hDlg, IDC_CHEAT_CODES);
    m_EditOptions = GetDlgItemStr(hDlg, IDC_CHEAT_OPTIONS);
    m_EditNotes = GetDlgItemStr(hDlg, IDC_NOTES);
}

int CALLBACK CCheatsUI::CheatAddProc(HWND hDlg, uint32_t uMsg, uint32_t wParam, uint32_t lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        CCheatsUI   * _this = (CCheatsUI *)lParam;
        SetProp(hDlg, L"Class", _this);

        SetWindowTextW(GetDlgItem(hDlg, IDC_NAME), wGS(CHEAT_ADDCHEAT_NAME).c_str());
        SetWindowTextW(GetDlgItem(hDlg, IDC_CODE), wGS(CHEAT_ADDCHEAT_CODE).c_str());
        SetWindowTextW(GetDlgItem(hDlg, IDC_LABEL_OPTIONS), wGS(CHEAT_ADDCHEAT_OPT).c_str());
        SetWindowTextW(GetDlgItem(hDlg, IDC_CODE_DES), wGS(CHEAT_ADDCHEAT_CODEDES).c_str());
        SetWindowTextW(GetDlgItem(hDlg, IDC_LABEL_OPTIONS_FORMAT), wGS(CHEAT_ADDCHEAT_OPTDES).c_str());
        SetWindowTextW(GetDlgItem(hDlg, IDC_CHEATNOTES), wGS(CHEAT_ADDCHEAT_NOTES).c_str());
        SetWindowTextW(GetDlgItem(hDlg, IDC_NEWCHEAT), wGS(CHEAT_ADDCHEAT_NEW).c_str());
        SetWindowTextW(GetDlgItem(hDlg, IDC_ADD), wGS(CHEAT_ADDCHEAT_ADD).c_str());
        SetProp(hDlg, L"validcodes", false);
        _this->RecordCheatValues(hDlg);
    }
    break;
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_CODE_NAME:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                bool validcodes, validoptions, nooptions;
                int  CodeFormat;
                ReadCodeString(hDlg, validcodes, validoptions, nooptions, CodeFormat);
                if (!nooptions)
                {
                    ReadOptionsString(hDlg, validcodes, validoptions, nooptions, CodeFormat);
                }

                if (validcodes && (validoptions || nooptions) && SendDlgItemMessage(hDlg, IDC_CODE_NAME, EM_LINELENGTH, 0, 0) > 0)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_ADD), true);
                }
                else
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_ADD), false);
                }
            }
            break;
        case IDC_CHEAT_CODES:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                bool validcodes, validoptions, nooptions;
                int  CodeFormat;
                ReadCodeString(hDlg, validcodes, validoptions, nooptions, CodeFormat);

                if ((CodeFormat > 0) && !IsWindowEnabled(GetDlgItem(hDlg, IDC_LABEL_OPTIONS)))
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS), true);
                    EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS_FORMAT), true);
                    EnableWindow(GetDlgItem(hDlg, IDC_CHEAT_OPTIONS), true);
                }
                if ((CodeFormat <= 0) && IsWindowEnabled(GetDlgItem(hDlg, IDC_LABEL_OPTIONS)))
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS), false);
                    EnableWindow(GetDlgItem(hDlg, IDC_LABEL_OPTIONS_FORMAT), false);
                    EnableWindow(GetDlgItem(hDlg, IDC_CHEAT_OPTIONS), false);
                }

                if (!nooptions)
                {
                    ReadOptionsString(hDlg, validcodes, validoptions, nooptions, CodeFormat);
                }

                if (validcodes && (validoptions || nooptions) && SendDlgItemMessage(hDlg, IDC_CODE_NAME, EM_LINELENGTH, 0, 0) > 0)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_ADD), true);
                }
                else
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_ADD), false);
                }
            }
            break;
        case IDC_CHEAT_OPTIONS:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                bool validcodes, validoptions, nooptions;
                int  CodeFormat;
                ReadOptionsString(hDlg, validcodes, validoptions, nooptions, CodeFormat);

                if (validcodes && (validoptions || nooptions) && SendDlgItemMessage(hDlg, IDC_CODE_NAME, EM_LINELENGTH, 0, 0) > 0)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_ADD), true);
                }
                else
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_ADD), false);
                }
            }
            break;
        case IDC_ADD:
        {
            CCheatsUI * _this = (CCheatsUI *)GetProp(hDlg, L"Class");

            stdstr NewCheatName = GetDlgItemStr(hDlg, IDC_CODE_NAME);
            int i = 0;
            for (i = 0; i < CCheats::MaxCheats; i++)
            {
                if (_this->m_EditCheat == i)
                {
                    continue;
                }
                stdstr CheatName(_this->GetCheatName(i, false));
                if (CheatName.length() == 0)
                {
                    if (_this->m_EditCheat < 0)
                    {
                        _this->m_EditCheat = i;
                    }
                    break;
                }
                if (_stricmp(CheatName.c_str(), NewCheatName.c_str()) == 0)
                {
                    g_Notify->DisplayWarning(GS(MSG_CHEAT_NAME_IN_USE));
                    SetFocus(GetDlgItem(hDlg, IDC_CODE_NAME));
                    return true;
                }
            }
            if (_this->m_EditCheat < 0 && i == CCheats::MaxCheats)
            {
                g_Notify->DisplayError(GS(MSG_MAX_CHEATS));
                return true;
            }

            //Update the entries
            bool validcodes, validoptions, nooptions;
            int  CodeFormat;
            stdstr_f Cheat("\"%s\"%s", NewCheatName.c_str(), ReadCodeString(hDlg, validcodes, validoptions, nooptions, CodeFormat).c_str());
            stdstr Options = ReadOptionsString(hDlg, validcodes, validoptions, nooptions, CodeFormat);

            g_Settings->SaveStringIndex(Cheat_Entry, _this->m_EditCheat, Cheat.c_str());
            g_Settings->SaveStringIndex(Cheat_Notes, _this->m_EditCheat, GetDlgItemStr(hDlg, IDC_NOTES));
            g_Settings->SaveStringIndex(Cheat_Options, _this->m_EditCheat, Options);
            _this->RecordCheatValues(hDlg);
            CSettingTypeCheats::FlushChanges();
            _this->RefreshCheatManager();
        }
        break;
        case IDC_NEWCHEAT:
        {
            CCheatsUI * _this = (CCheatsUI *)GetProp(hDlg, L"Class");

            if (_this->CheatChanged(hDlg))
            {
                break;
            }
            _this->m_EditCheat = -1;
            SetDlgItemText(hDlg, IDC_CODE_NAME, L"");
            SetDlgItemText(hDlg, IDC_CHEAT_CODES, L"");
            SetDlgItemText(hDlg, IDC_CHEAT_OPTIONS, L"");
            SetDlgItemText(hDlg, IDC_NOTES, L"");
            EnableWindow(GetDlgItem(hDlg, IDC_ADD), false);
            EnableWindow(GetDlgItem(hDlg, IDC_CHEAT_OPTIONS), false);
            SetDlgItemTextW(hDlg, IDC_ADD, wGS(CHEAT_ADDNEW).c_str());
            _this->RecordCheatValues(hDlg);
        }
        break;
        }
    }
    break;
    case WM_EDITCHEAT:
    {
        CCheatsUI * _this = (CCheatsUI *)GetProp(hDlg, L"Class");
        _this->m_EditCheat = wParam;
        if (_this->m_EditCheat < 0)
        {
            break;
        }

        if (_this->CheatChanged(hDlg))
        {
            break;
        }

        stdstr CheatEntryStr = g_Settings->LoadStringIndex(Cheat_Entry, _this->m_EditCheat);
        const char * String = CheatEntryStr.c_str();

        //Set Cheat Name
        int len = strrchr(String, '"') - strchr(String, '"') - 1;
        stdstr CheatName(strchr(String, '"') + 1);
        CheatName.resize(len);
        SetDlgItemText(hDlg, IDC_CODE_NAME, CheatName.ToUTF16().c_str());

        //Add Gameshark codes to screen
        const char * ReadPos = strrchr(String, '"') + 2;
        stdstr Buffer;
        do
        {
            char * End = strchr((char *)ReadPos, ',');
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
        SetDlgItemText(hDlg, IDC_CHEAT_CODES, Buffer.ToUTF16().c_str());

        //Add option values to screen
        stdstr CheatOptionStr = g_Settings->LoadStringIndex(Cheat_Options, _this->m_EditCheat);
        ReadPos = strchr(CheatOptionStr.c_str(), '$');
        Buffer.erase();
        if (ReadPos)
        {
            ReadPos += 1;
            do
            {
                char * End = strchr((char *)ReadPos, ',');
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
        SetDlgItemText(hDlg, IDC_CHEAT_OPTIONS, Buffer.ToUTF16().c_str());

        //Add cheat Notes
        stdstr CheatNotesStr = g_Settings->LoadStringIndex(Cheat_Notes, _this->m_EditCheat);
        SetDlgItemText(hDlg, IDC_NOTES, CheatNotesStr.ToUTF16().c_str());

        SendMessage(hDlg, WM_COMMAND, MAKELPARAM(IDC_CHEAT_CODES, EN_CHANGE), (LPARAM)GetDlgItem(hDlg, IDC_CHEAT_CODES));
        SetDlgItemTextW(hDlg, IDC_ADD, wGS(CHEAT_EDITCHEAT_UPDATE).c_str());

        _this->RecordCheatValues(hDlg);
    }
    break;
    default:
        return false;
    }
    return true;
}

int CALLBACK CCheatsUI::CheatListProc(HWND hDlg, uint32_t uMsg, uint32_t wParam, uint32_t lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        CCheatsUI   * _this = (CCheatsUI *)lParam;
        SetProp(hDlg, L"Class", _this);

        uint32_t Style;
        RECT rcList;
        RECT rcButton;

        SetWindowTextW(GetDlgItem(hDlg, IDC_NOTESFRAME), wGS(CHEAT_NOTES_FRAME).c_str());
        SetWindowTextW(GetDlgItem(hDlg, IDC_UNMARK), wGS(CHEAT_MARK_NONE).c_str());

        GetWindowRect(hDlg, &rcList);
        GetWindowRect(GetDlgItem(hDlg, IDC_UNMARK), &rcButton);

        _this->m_hCheatTree = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | TVS_HASLINES |
            TVS_HASBUTTONS | TVS_LINESATROOT | TVS_DISABLEDRAGDROP | WS_TABSTOP |
            TVS_FULLROWSELECT, 6, 4, rcList.right - rcList.left - 13,
            rcButton.top - rcList.top - 8, hDlg, (HMENU)IDC_MYTREE, GetModuleHandle(NULL), NULL);
        Style = GetWindowLong(_this->m_hCheatTree, GWL_STYLE);
        SetWindowLong(_this->m_hCheatTree, GWL_STYLE, TVS_CHECKBOXES | TVS_SHOWSELALWAYS | Style);

        //Creats an image list from the bitmap in the resource section
        HIMAGELIST hImageList;
        HBITMAP hBmp;

        hImageList = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 40, 40);
        hBmp = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_TRI_STATE));
        ImageList_AddMasked(hImageList, hBmp, RGB(255, 0, 255));
        DeleteObject(hBmp);

        TreeView_SetImageList(_this->m_hCheatTree, hImageList, TVSIL_STATE);

        _this->m_hSelectedItem = NULL;
    }
    break;
    case WM_COMMAND:
    {
        CCheatsUI   * _this = (CCheatsUI *)GetProp(hDlg, L"Class");

        switch (LOWORD(wParam))
        {
        case ID_POPUP_DELETE:
        {
            TVITEM item;

            int Response = MessageBoxW(hDlg, wGS(MSG_DEL_SURE).c_str(), wGS(MSG_DEL_TITLE).c_str(), MB_YESNO | MB_ICONQUESTION);
            if (Response != IDYES) { break; }

            //Delete selected cheat
            item.hItem = (HTREEITEM)_this->m_hSelectedItem;
            item.mask = TVIF_PARAM;
            TreeView_GetItem(_this->m_hCheatTree, &item);

            _this->ChangeChildrenStatus((HWND)TVI_ROOT, false);
            _this->DeleteCheat(item.lParam);
            _this->RefreshCheatManager();
        }
        break;
        case IDC_UNMARK:
            _this->ChangeChildrenStatus((HWND)TVI_ROOT, false);
            if (g_BaseSystem)
            {
                g_BaseSystem->SetCheatsSlectionChanged(true);
            }
            break;
        }
    }
    break;
    case WM_NOTIFY:
    {
        CCheatsUI   * _this = (CCheatsUI *)GetProp(hDlg, L"Class");

        if (_this->m_DeleteingEntries)
        {
            break;
        }
        LPNMHDR lpnmh = (LPNMHDR)lParam;

        if ((lpnmh->code == NM_RCLICK) && (lpnmh->idFrom == IDC_MYTREE))
        {
            //Work out what item is selected
            TVHITTESTINFO ht = { 0 };
            uint32_t dwpos = GetMessagePos();

            // include <windowsx.h> and <windows.h> header files
            ht.pt.x = GET_X_LPARAM(dwpos);
            ht.pt.y = GET_Y_LPARAM(dwpos);
            MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

            TreeView_HitTest(lpnmh->hwndFrom, &ht);
            _this->m_hSelectedItem = (HWND)ht.hItem;
            if (g_Settings->LoadBool(UserInterface_BasicMode)) { return true; }

            //Show Menu
            HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_CHEAT_MENU));
            HMENU hPopupMenu = GetSubMenu(hMenu, 0);
            POINT Mouse;

            GetCursorPos(&Mouse);

            MenuSetText(hPopupMenu, 0, wGS(CHEAT_ADDNEW).c_str(), NULL);
            MenuSetText(hPopupMenu, 1, wGS(CHEAT_EDIT).c_str(), NULL);
            MenuSetText(hPopupMenu, 3, wGS(CHEAT_DELETE).c_str(), NULL);

            if (_this->m_hSelectedItem != NULL &&
                TreeView_GetChild(_this->m_hCheatTree, _this->m_hSelectedItem) == NULL)
            {
                TrackPopupMenu(hPopupMenu, 0, Mouse.x, Mouse.y, 0, hDlg, NULL);
            }
            DestroyMenu(hMenu);
        }
        else if ((lpnmh->code == NM_CLICK) && (lpnmh->idFrom == IDC_MYTREE))
        {
            TVHITTESTINFO ht = { 0 };
            uint32_t dwpos = GetMessagePos();

            // include <windowsx.h> and <windows.h> header files
            ht.pt.x = GET_X_LPARAM(dwpos);
            ht.pt.y = GET_Y_LPARAM(dwpos);
            MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

            TreeView_HitTest(lpnmh->hwndFrom, &ht);

            if (TVHT_ONITEMSTATEICON & ht.flags)
            {
                switch (TV_GetCheckState(_this->m_hCheatTree, (HWND)ht.hItem))
                {
                case TV_STATE_CLEAR:
                case TV_STATE_INDETERMINATE:
                    //Make sure that the item has a valid code extenstion selected
                    if (TreeView_GetChild(_this->m_hCheatTree, ht.hItem) == NULL)
                    {
                        TVITEM item;
                        item.mask = TVIF_PARAM;
                        item.hItem = (HTREEITEM)ht.hItem;
                        TreeView_GetItem(_this->m_hCheatTree, &item);
                        stdstr LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, item.lParam);
                        if (CheatUsesCodeExtensions(LineEntry))
                        {
                            stdstr CheatExtension;
                            if (!g_Settings->LoadStringIndex(Cheat_Extension, item.lParam, CheatExtension))
                            {
                                SendMessage(hDlg, UM_CHANGECODEEXTENSION, 0, (LPARAM)ht.hItem);
                                TV_SetCheckState(_this->m_hCheatTree, (HWND)ht.hItem, TV_STATE_CLEAR);
                                break;
                            }
                        }
                    }
                    TV_SetCheckState(_this->m_hCheatTree, (HWND)ht.hItem, TV_STATE_CHECKED);
                    _this->ChangeChildrenStatus((HWND)ht.hItem, true);
                    _this->CheckParentStatus((HWND)TreeView_GetParent((HWND)_this->m_hCheatTree, (HWND)ht.hItem));
                    break;
                case TV_STATE_CHECKED:
                    TV_SetCheckState(_this->m_hCheatTree, (HWND)ht.hItem, TV_STATE_CLEAR);
                    _this->ChangeChildrenStatus((HWND)ht.hItem, false);
                    _this->CheckParentStatus((HWND)TreeView_GetParent((HWND)_this->m_hCheatTree, (HWND)ht.hItem));
                    break;
                }
                switch (TV_GetCheckState(_this->m_hCheatTree, (HWND)ht.hItem))
                {
                case TV_STATE_CHECKED: TV_SetCheckState(_this->m_hCheatTree, (HWND)ht.hItem, TV_STATE_INDETERMINATE); break;
                case TV_STATE_CLEAR:   TV_SetCheckState(_this->m_hCheatTree, (HWND)ht.hItem, TV_STATE_CHECKED); break;
                case TV_STATE_INDETERMINATE: TV_SetCheckState(_this->m_hCheatTree, (HWND)ht.hItem, TV_STATE_CLEAR); break;
                }

                if (g_BaseSystem)
                {
                    g_BaseSystem->SetCheatsSlectionChanged(true);
                }
            }
        }
        else if ((lpnmh->code == NM_DBLCLK) && (lpnmh->idFrom == IDC_MYTREE))
        {
            TVHITTESTINFO ht = { 0 };
            uint32_t dwpos = GetMessagePos();

            // include <windowsx.h> and <windows.h> header files
            ht.pt.x = GET_X_LPARAM(dwpos);
            ht.pt.y = GET_Y_LPARAM(dwpos);
            MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

            TreeView_HitTest(lpnmh->hwndFrom, &ht);

            if (TVHT_ONITEMLABEL & ht.flags)
            {
                PostMessage(hDlg, UM_CHANGECODEEXTENSION, 0, (LPARAM)ht.hItem);
            }
        }
        else if ((lpnmh->code == TVN_SELCHANGEDW) && (lpnmh->idFrom == IDC_MYTREE))
        {
            HTREEITEM hItem;

            hItem = TreeView_GetSelection(_this->m_hCheatTree);
            if (TreeView_GetChild(_this->m_hCheatTree, hItem) == NULL)
            {
                TVITEM item;

                item.mask = TVIF_PARAM;
                item.hItem = hItem;
                TreeView_GetItem(_this->m_hCheatTree, &item);

                stdstr Notes(g_Settings->LoadStringIndex(Cheat_Notes, item.lParam));
                SetDlgItemText(hDlg, IDC_NOTES, Notes.ToUTF16().c_str());
                if (_this->m_AddCheat)
                {
                    SendMessage(_this->m_AddCheat, WM_EDITCHEAT, item.lParam, 0); //edit cheat
                }
            }
            else
            {
                SetDlgItemText(hDlg, IDC_NOTES, L"");
            }
        }
    }
    break;
    case UM_CHANGECODEEXTENSION:
    {
        CCheatsUI * _this = (CCheatsUI *)GetProp(hDlg, L"Class");

        //Get the selected item
        _this->m_hSelectedItem = (HWND)lParam;
        TVITEM item;
        item.mask = TVIF_PARAM;
        item.hItem = (HTREEITEM)_this->m_hSelectedItem;
        if (!TreeView_GetItem(_this->m_hCheatTree, &item))
        {
            break;
        }

        //Make sure the selected line can use code extensions
        stdstr LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, item.lParam);
        if (!CheatUsesCodeExtensions(LineEntry)) { break; }

        stdstr Options;
        if (g_Settings->LoadStringIndex(Cheat_Options, item.lParam, Options) && Options.length() > 0)
        {
            DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_Cheats_CodeEx), hDlg, (DLGPROC)CheatsCodeExProc, (LPARAM)_this);
        }
        else
        {
            stdstr Range;
            if (g_Settings->LoadStringIndex(Cheat_Range, item.lParam, Range) && Range.length() > 0)
            {
                DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_Cheats_Range), hDlg, (DLGPROC)CheatsCodeQuantProc, (LPARAM)_this);
            }
        }

        //Update cheat listing with new extention
        stdstr CheatName(_this->GetCheatName(item.lParam, true));
        char * Cheat = strrchr((char *)CheatName.c_str(), '\\');
        if (Cheat == NULL)
        {
            Cheat = const_cast<char *>(CheatName.c_str());
        }
        else
        {
            Cheat += 1;
        }
        std::wstring wcCheat = stdstr(Cheat).ToUTF16();
        item.mask = TVIF_TEXT;
        item.pszText = (LPWSTR)wcCheat.c_str();
        item.cchTextMax = CheatName.length();
        TreeView_SetItem(_this->m_hCheatTree, &item);
    }
    break;
    default:
        return false;
    }
    return true;
}

int CALLBACK CCheatsUI::CheatsCodeExProc(HWND hDlg, uint32_t uMsg, uint32_t wParam, uint32_t lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        CCheatsUI   * _this = (CCheatsUI *)lParam;
        SetProp(hDlg, L"Class", _this);

        //Find the cheat Number of the option being selected
        TVITEM item;
        item.hItem = (HTREEITEM)_this->m_hSelectedItem;
        item.mask = TVIF_PARAM;
        TreeView_GetItem(_this->m_hCheatTree, &item);
        stdstr CheatName = _this->GetCheatName(item.lParam, false);

        //Set up language support for dialog
        SetWindowTextW(hDlg, wGS(CHEAT_CODE_EXT_TITLE).c_str());
        SetDlgItemTextW(hDlg, IDC_NOTE, wGS(CHEAT_CODE_EXT_TXT).c_str());
        SetDlgItemTextW(hDlg, IDOK, wGS(CHEAT_OK).c_str());
        SetDlgItemTextW(hDlg, IDCANCEL, wGS(CHEAT_CANCEL).c_str());
        SetDlgItemText(hDlg, IDC_CHEAT_NAME, CheatName.ToUTF16().c_str());

        //Read through and add all options to the list box
        stdstr Options(g_Settings->LoadStringIndex(Cheat_Options, item.lParam));
        stdstr CurrentExt(g_Settings->LoadStringIndex(Cheat_Extension, item.lParam));
        const char * ReadPos = Options.c_str();
        while (*ReadPos != 0)
        {
            const char * NextComma = strchr(ReadPos, ',');
            int len = NextComma == NULL ? strlen(ReadPos) : NextComma - ReadPos;
            stdstr CheatExt(ReadPos);
            CheatExt.resize(len);
            int index = SendMessage(GetDlgItem(hDlg, IDC_CHEAT_LIST), LB_ADDSTRING, 0, (LPARAM)CheatExt.c_str());
            if (CheatExt == CurrentExt)
            {
                SendMessage(GetDlgItem(hDlg, IDC_CHEAT_LIST), LB_SETCURSEL, index, 0);
            }
            //Move to next entry or end
            ReadPos = NextComma ? NextComma + 1 : ReadPos + strlen(ReadPos);
        }
    }
    break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CHEAT_LIST:
            if (HIWORD(wParam) == LBN_DBLCLK) { PostMessage(hDlg, WM_COMMAND, IDOK, 0); break; }
            break;
        case IDOK:
        {
            CCheatsUI * _this = (CCheatsUI *)GetProp(hDlg, L"Class");

            //Find the cheat Number of the option being selected
            TVITEM item;
            item.hItem = (HTREEITEM)_this->m_hSelectedItem;
            item.mask = TVIF_PARAM;
            TreeView_GetItem(_this->m_hCheatTree, &item);

            //Get the selected cheat extension
            char CheatExten[300];
            int index = SendMessage(GetDlgItem(hDlg, IDC_CHEAT_LIST), LB_GETCURSEL, 0, 0);
            if (index < 0) { index = 0; }
            SendMessage(GetDlgItem(hDlg, IDC_CHEAT_LIST), LB_GETTEXT, index, (LPARAM)CheatExten);

            g_Settings->SaveStringIndex(Cheat_Extension, item.lParam, CheatExten);
            if (g_BaseSystem)
            {
                g_BaseSystem->SetCheatsSlectionChanged(true);
            }
        }
        RemoveProp(hDlg, L"Class");
        EndDialog(hDlg, 0);
        break;
        case IDCANCEL:
            RemoveProp(hDlg, L"Class");
            EndDialog(hDlg, 0);
            break;
        }
    default:
        return false;
    }
    return true;
}

int CALLBACK CCheatsUI::CheatsCodeQuantProc(HWND hDlg, uint32_t uMsg, uint32_t wParam, uint32_t lParam)
{
    static uint16_t Start, Stop, SelStart, SelStop;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        CCheatsUI * _this = (CCheatsUI *)lParam;
        SetProp(hDlg, L"Class", _this);

        //Find the cheat Number of the option being selected
        TVITEM item;
        item.hItem = (HTREEITEM)_this->m_hSelectedItem;
        item.mask = TVIF_PARAM;
        TreeView_GetItem(_this->m_hCheatTree, &item);
        stdstr CheatName = _this->GetCheatName(item.lParam, false);
        stdstr RangeNote(g_Settings->LoadStringIndex(Cheat_RangeNotes, item.lParam));
        stdstr Range(g_Settings->LoadStringIndex(Cheat_Range, item.lParam));
        stdstr Value(g_Settings->LoadStringIndex(Cheat_Extension, item.lParam));

        //Set up language support for dialog
        SetWindowTextW(hDlg, wGS(CHEAT_CODE_EXT_TITLE).c_str());
        SetDlgItemTextW(hDlg, IDC_DIGITAL_TEXT, wGS(CHEAT_CHOOSE_VALUE).c_str());
        SetDlgItemTextW(hDlg, IDC_VALUE_TEXT, wGS(CHEAT_VALUE).c_str());
        SetDlgItemTextW(hDlg, IDC_NOTES_TEXT, wGS(CHEAT_NOTES).c_str());
        SetDlgItemText(hDlg, IDC_NOTES, RangeNote.ToUTF16().c_str());
        SetDlgItemText(hDlg, IDC_CHEAT_NAME, CheatName.ToUTF16().c_str());
        SetDlgItemText(hDlg, IDC_VALUE, Value.ToUTF16().c_str());

        Start = (uint16_t)(Range.c_str()[0] == '$' ? strtoul(&Range.c_str()[1], 0, 16) : atol(Range.c_str()));
        const char * ReadPos = strrchr(Range.c_str(), '-');
        if (ReadPos != NULL)
        {
            Stop = (uint16_t)(ReadPos[1] == '$' ? strtoul(&ReadPos[2], 0, 16) : atol(&ReadPos[1]));
        }
        else
        {
            Stop = 0;
        }

        char Text[500];
        sprintf(Text, "%s $%X %s $%X", GS(CHEAT_FROM), Start, GS(CHEAT_TO), Stop);
        SetDlgItemText(hDlg, IDC_RANGE, stdstr(Text).ToUTF16().c_str());
    }
    break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_VALUE:
            if (HIWORD(wParam) == EN_UPDATE)
            {
                TCHAR szTmp[10], szTmp2[10];
                uint32_t Value;
                GetDlgItemText(hDlg, IDC_VALUE, szTmp, sizeof(szTmp));
                Value = szTmp[0] == '$' ? wcstoul(&szTmp[1], 0, 16) : wcstoul(&szTmp[0], 0, 16);
                if (Value > Stop)  { Value = Stop; }
                if (Value < Start) { Value = Start; }
                wsprintf(szTmp2, L"$%X", Value);
                if (wcscmp(szTmp, szTmp2) != 0)
                {
                    SetDlgItemText(hDlg, IDC_VALUE, szTmp2);
                    if (SelStop == 0) { SelStop = (uint16_t)wcslen(szTmp2); SelStart = SelStop; }
                    SendDlgItemMessage(hDlg, IDC_VALUE, EM_SETSEL, (WPARAM)SelStart, (LPARAM)SelStop);
                }
                else
                {
                    uint16_t NewSelStart, NewSelStop;
                    SendDlgItemMessage(hDlg, IDC_VALUE, EM_GETSEL, (WPARAM)&NewSelStart, (LPARAM)&NewSelStop);
                    if (NewSelStart != 0) { SelStart = NewSelStart; SelStop = NewSelStop; }
                }
            }
            break;
        case IDOK:
        {
            CCheatsUI * _this = (CCheatsUI *)GetProp(hDlg, L"Class");

            //Find the cheat Number of the option being selected
            TVITEM item;
            item.hItem = (HTREEITEM)_this->m_hSelectedItem;
            item.mask = TVIF_PARAM;
            TreeView_GetItem(_this->m_hCheatTree, &item);

            //Get the selected cheat extension
            TCHAR CheatExten[300], szTmp[10];
            uint32_t Value;

            GetDlgItemText(hDlg, IDC_VALUE, szTmp, sizeof(szTmp));
            Value = szTmp[0] == L'$' ? wcstol(&szTmp[1], 0, 16) : wcstol(&szTmp[0], 0, 16);
            if (Value > Stop) { Value = Stop; }
            if (Value < Start) { Value = Start; }
            wsprintf(CheatExten, L"$%X", Value);

            g_Settings->SaveStringIndex(Cheat_Extension, item.lParam, stdstr().FromUTF16(CheatExten).c_str());
            if (g_BaseSystem)
            {
                g_BaseSystem->SetCheatsSlectionChanged(true);
            }
        }
        RemoveProp(hDlg, L"Class");
        EndDialog(hDlg, 0);
        break;
        case IDCANCEL:
            RemoveProp(hDlg, L"Class");
            EndDialog(hDlg, 0);
            break;
        }
    default:
        return false;
    }
    return true;
}

bool CCheatsUI::IsCheatMessage(MSG * msg)
{
    if (m_Window)
    {
        return IsDialogMessage(m_Window, msg) != 0;
    }
    return false;
}

int CALLBACK CCheatsUI::ManageCheatsProc(HWND hDlg, uint32_t uMsg, uint32_t wParam, uint32_t lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        CCheatsUI * _this = (CCheatsUI *)lParam;
        SetProp(hDlg, L"Class", _this);
        _this->m_Window = hDlg;

        WINDOWPLACEMENT WndPlac;
        WndPlac.length = sizeof(WndPlac);
        GetWindowPlacement(hDlg, &WndPlac);

        SetWindowTextW(hDlg, wGS(CHEAT_TITLE).c_str());
        _this->m_hSelectCheat = CreateDialogParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_Cheats_List), hDlg, (DLGPROC)CheatListProc, (LPARAM)_this);
        SetWindowPos(_this->m_hSelectCheat, HWND_TOP, 5, 8, 0, 0, SWP_NOSIZE);
        ShowWindow(_this->m_hSelectCheat, SW_SHOW);

        RECT * rc = &WndPlac.rcNormalPosition;
        if (g_Settings->LoadDword(UserInterface_BasicMode))
        {
            RECT * rcList = (RECT *)_this->m_rcList;
            GetWindowRect(_this->m_hSelectCheat, rcList);
            _this->m_MinSizeDlg = rcList->right - rcList->left + 16;
            _this->m_MaxSizeDlg = _this->m_MinSizeDlg;

            _this->m_DialogState = CONTRACTED;
            WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + _this->m_MinSizeDlg;
            SetWindowPlacement(hDlg, &WndPlac);

            ShowWindow(GetDlgItem(hDlg, IDC_STATE), SW_HIDE);
        }
        else
        {
            _this->m_AddCheat = CreateDialogParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_Cheats_Add), hDlg, (DLGPROC)CheatAddProc, (LPARAM)_this);
            SetWindowPos(_this->m_AddCheat, HWND_TOP, (rc->right - rc->left) / 2, 8, 0, 0, SWP_NOSIZE);
            ShowWindow(_this->m_AddCheat, SW_HIDE);

            RECT * rcAdd = (RECT *)_this->m_rcAdd, *rcList = (RECT *)_this->m_rcList;
            GetWindowRect(_this->m_hSelectCheat, rcList);
            GetWindowRect(_this->m_AddCheat, rcAdd);
            _this->m_MinSizeDlg = rcList->right - rcList->left + 32;
            _this->m_MaxSizeDlg = rcAdd->right - rcList->left + 32;

            _this->m_DialogState = CONTRACTED;
            WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + _this->m_MinSizeDlg;
            SetWindowPlacement(hDlg, &WndPlac);

            GetClientRect(hDlg, rc);
            HWND hStateButton = GetDlgItem(hDlg, IDC_STATE);
            SetWindowPos(hStateButton, HWND_TOP, (rc->right - rc->left) - 16, 0, 16, rc->bottom - rc->top, 0);
            HANDLE hIcon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_RIGHT), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
            SendDlgItemMessage(hDlg, IDC_STATE, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);
        }

        //re-center cheat window
        RECT rcDlg, rcParent;
        GetWindowRect(hDlg, &rcDlg);
        GetWindowRect(GetParent(hDlg), &rcParent);

        int DlgWidth = rcDlg.right - rcDlg.left;
        int DlgHeight = rcDlg.bottom - rcDlg.top;

        int X = (((rcParent.right - rcParent.left) - DlgWidth) / 2) + rcParent.left;
        int Y = (((rcParent.bottom - rcParent.top) - DlgHeight) / 2) + rcParent.top;

        SetWindowPos(hDlg, NULL, X, Y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        _this->RefreshCheatManager();
    }
    break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
        {
            CCheatsUI * _this = (CCheatsUI *)GetProp(hDlg, L"Class");
            if (_this->m_AddCheat)
            {
                DestroyWindow(_this->m_AddCheat);
                _this->m_AddCheat = NULL;
            }
            _this->m_Window = NULL;
            RemoveProp(hDlg, L"Class");
            EndDialog(hDlg, 0);
            if (g_BaseSystem)
            {
                g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_Cheats);
            }
            if (g_cheatUI == _this)
            {
                delete g_cheatUI;
                g_cheatUI = NULL;
            }
        }
        break;
        case IDC_STATE:
        {
            CCheatsUI * _this = (CCheatsUI *)GetProp(hDlg, L"Class");
            WINDOWPLACEMENT WndPlac;
            WndPlac.length = sizeof(WndPlac);
            GetWindowPlacement(hDlg, &WndPlac);

            if (_this->m_DialogState == CONTRACTED)
            {
                _this->m_DialogState = EXPANDED;
                WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + _this->m_MaxSizeDlg;
                SetWindowPlacement(hDlg, &WndPlac);

                RECT clientrect;
                GetClientRect(hDlg, &clientrect);
                HWND hStateButton = GetDlgItem(hDlg, IDC_STATE);
                SetWindowPos(hStateButton, HWND_TOP, (clientrect.right - clientrect.left) - 16, 0, 16, clientrect.bottom - clientrect.top, 0);

                HANDLE hIcon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_LEFT), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
                SendDlgItemMessage(hDlg, IDC_STATE, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);

                ShowWindow(_this->m_AddCheat, SW_SHOW);
            }
            else
            {
                _this->m_DialogState = CONTRACTED;
                WndPlac.rcNormalPosition.right = WndPlac.rcNormalPosition.left + _this->m_MinSizeDlg;
                SetWindowPlacement(hDlg, &WndPlac);

                HANDLE hIcon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_RIGHT), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
                SendDlgItemMessage(hDlg, IDC_STATE, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)(HANDLE)hIcon);

                RECT clientrect;
                GetClientRect(hDlg, &clientrect);
                HWND hStateButton = GetDlgItem(hDlg, IDC_STATE);
                SetWindowPos(hStateButton, HWND_TOP, (clientrect.right - clientrect.left) - 16, 0, 16, clientrect.bottom - clientrect.top, 0);

                ShowWindow(_this->m_AddCheat, SW_HIDE);
            }
        }
        break;
        }
        break;
    default:
        return false;
    }
    return true;
}

bool CCheatsUI::TV_SetCheckState(HWND hwndTreeView, HWND hItem, TV_CHECK_STATE state)
{
    TVITEM tvItem;

    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = (HTREEITEM)hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;

    /*Image 1 in the tree-view check box image list is the
    unchecked box. Image 2 is the checked box.*/

    switch (state)
    {
    case TV_STATE_CHECKED: tvItem.state = INDEXTOSTATEIMAGEMASK(1); break;
    case TV_STATE_CLEAR: tvItem.state = INDEXTOSTATEIMAGEMASK(2); break;
    case TV_STATE_INDETERMINATE: tvItem.state = INDEXTOSTATEIMAGEMASK(3); break;
    default: tvItem.state = INDEXTOSTATEIMAGEMASK(0); break;
    }
    return TreeView_SetItem(hwndTreeView, &tvItem) != 0;
}

int CCheatsUI::TV_GetCheckState(HWND hwndTreeView, HWND hItem)
{
    TVITEM tvItem;

    // Prepare to receive the desired information.
    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = (HTREEITEM)hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;

    // Request the information.
    TreeView_GetItem(hwndTreeView, &tvItem);

    // Return zero if it's not checked, or nonzero otherwise.
    switch (tvItem.state >> 12) {
    case 1: return TV_STATE_CHECKED;
    case 2: return TV_STATE_CLEAR;
    case 3: return TV_STATE_INDETERMINATE;
    }
    return ((int)(tvItem.state >> 12) - 1);
}

void CCheatsUI::MenuSetText(HMENU hMenu, int MenuPos, const wchar_t * Title, const wchar_t * ShortCut)
{
    MENUITEMINFOW MenuInfo;
    wchar_t String[256];

    if (Title == NULL || wcslen(Title) == 0) { return; }

    memset(&MenuInfo, 0, sizeof(MENUITEMINFO));
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

stdstr CCheatsUI::GetCheatName(int CheatNo, bool AddExtension) const
{
    if (CheatNo > CCheats::MaxCheats) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    stdstr LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, CheatNo);
    if (LineEntry.length() == 0) { return LineEntry; }

    //Find the start and end of the name which is surrounded in ""
    int StartOfName = LineEntry.find("\"");
    if (StartOfName == -1) { return stdstr(""); }
    int EndOfName = LineEntry.find("\"", StartOfName + 1);
    if (EndOfName == -1) { return stdstr(""); }

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

bool CCheatsUI::CheatUsesCodeExtensions(const stdstr &LineEntry)
{
    //Find the start and end of the name which is surronded in ""
    if (LineEntry.length() == 0){ return false; }
    int StartOfName = LineEntry.find("\"");
    if (StartOfName == -1)      { return false; }
    int EndOfName = LineEntry.find("\"", StartOfName + 1);
    if (EndOfName == -1)        { return false; }

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

void CCheatsUI::DeleteCheat(int Index)
{
    for (int CheatNo = Index; CheatNo < CCheats::MaxCheats; CheatNo++)
    {
        stdstr LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, CheatNo + 1);
        if (LineEntry.empty())
        {
            g_Settings->DeleteSettingIndex(Cheat_RangeNotes, CheatNo);
            g_Settings->DeleteSettingIndex(Cheat_Range, CheatNo);
            g_Settings->DeleteSettingIndex(Cheat_Options, CheatNo);
            g_Settings->DeleteSettingIndex(Cheat_Notes, CheatNo);
            g_Settings->DeleteSettingIndex(Cheat_Extension, CheatNo);
            g_Settings->DeleteSettingIndex(Cheat_Entry, CheatNo);
            g_Settings->DeleteSettingIndex(Cheat_Active, CheatNo);
            break;
        }
        stdstr Value;
        if (g_Settings->LoadStringIndex(Cheat_RangeNotes, CheatNo + 1, Value))
        {
            g_Settings->SaveStringIndex(Cheat_RangeNotes, CheatNo, Value);
        }
        else
        {
            g_Settings->DeleteSettingIndex(Cheat_RangeNotes, CheatNo);
        }

        if (g_Settings->LoadStringIndex(Cheat_Range, CheatNo + 1, Value))
        {
            g_Settings->SaveStringIndex(Cheat_Range, CheatNo, Value);
        }
        else
        {
            g_Settings->DeleteSettingIndex(Cheat_Range, CheatNo);
        }

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

void CCheatsUI::ChangeChildrenStatus(HWND hParent, bool Checked)
{
    HTREEITEM hItem = TreeView_GetChild(m_hCheatTree, hParent);
    if (hItem == NULL)
    {
        if ((HTREEITEM)hParent == TVI_ROOT) { return; }

        TVITEM item;
        item.mask = TVIF_PARAM;
        item.hItem = (HTREEITEM)hParent;
        TreeView_GetItem(m_hCheatTree, &item);

        //if cheat uses a extension and it is not set then do not set it
        if (Checked)
        {
            stdstr LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, item.lParam);
            if (CheatUsesCodeExtensions(LineEntry))
            {
                stdstr CheatExten;
                if (!g_Settings->LoadStringIndex(Cheat_Extension, item.lParam, CheatExten) || CheatExten.empty())
                {
                    return;
                }
            }
        }

        //Save Cheat
        TV_SetCheckState(m_hCheatTree, hParent, Checked ? TV_STATE_CHECKED : TV_STATE_CLEAR);
        g_Settings->SaveBoolIndex(Cheat_Active, item.lParam, Checked);
        return;
    }
    TV_CHECK_STATE state = TV_STATE_UNKNOWN;
    while (hItem != NULL)
    {
        TV_CHECK_STATE ChildState = (TV_CHECK_STATE)TV_GetCheckState(m_hCheatTree, (HWND)hItem);
        if ((ChildState != TV_STATE_CHECKED || !Checked) &&
            (ChildState != TV_STATE_CLEAR || Checked))
        {
            ChangeChildrenStatus((HWND)hItem, Checked);
        }
        ChildState = (TV_CHECK_STATE)TV_GetCheckState(m_hCheatTree, (HWND)hItem);
        if (state == TV_STATE_UNKNOWN) { state = ChildState; }
        if (state != ChildState) { state = TV_STATE_INDETERMINATE; }
        hItem = TreeView_GetNextSibling(m_hCheatTree, hItem);
    }
    if (state != TV_STATE_UNKNOWN)
    {
        TV_SetCheckState(m_hCheatTree, hParent, state);
    }
}

void CCheatsUI::CheckParentStatus(HWND hParent)
{
    TV_CHECK_STATE CurrentState, InitialState;
    HTREEITEM hItem;

    if (!hParent) { return; }
    hItem = TreeView_GetChild(m_hCheatTree, (HTREEITEM)hParent);
    InitialState = (TV_CHECK_STATE)TV_GetCheckState(m_hCheatTree, hParent);
    CurrentState = (TV_CHECK_STATE)TV_GetCheckState(m_hCheatTree, (HWND)hItem);

    while (hItem != NULL)
    {
        if (TV_GetCheckState(m_hCheatTree, (HWND)hItem) != CurrentState)
        {
            CurrentState = TV_STATE_INDETERMINATE;
            break;
        }
        hItem = TreeView_GetNextSibling(m_hCheatTree, hItem);
    }
    TV_SetCheckState(m_hCheatTree, hParent, CurrentState);
    if (InitialState != CurrentState)
    {
        CheckParentStatus((HWND)TreeView_GetParent((HWND)m_hCheatTree, (HTREEITEM)hParent));
    }
}

stdstr CCheatsUI::ReadCodeString(HWND hDlg, bool &validcodes, bool &validoptions, bool &nooptions, int &codeformat)
{
    int numlines, linecount, len;
    wchar_t wc_str[128];
    int i;
    char* formatnormal = "XXXXXXXX XXXX";
    char* formatoptionlb = "XXXXXXXX XX??";
    char* formatoptionw = "XXXXXXXX ????";
    char tempformat[128];

    validcodes = true;
    nooptions = true;
    codeformat = -1;
    int numcodes = 0;

    char codestring[2048];
    memset(codestring, '\0', sizeof(codestring));

    numlines = SendDlgItemMessage(hDlg, IDC_CHEAT_CODES, EM_GETLINECOUNT, 0, 0);
    if (numlines == 0) { validcodes = false; }

    for (linecount = 0; linecount < numlines; linecount++) //read line after line (bypassing limitation GetDlgItemText)
    {
        memset(tempformat, 0, sizeof(tempformat));

        //str[0] = sizeof(str) > 255?255:sizeof(str);
        *(LPWORD)wc_str = sizeof(wc_str) / sizeof(wc_str[0]);
        len = SendDlgItemMessage(hDlg, IDC_CHEAT_CODES, EM_GETLINE, (WPARAM)linecount, (LPARAM)(const char *)wc_str);
        wc_str[len] = 0;

        if (len <= 0) { continue; }

        std::string str = stdstr().FromUTF16(wc_str);
        for (i = 0; i < 128; i++)
        {
            if (isxdigit(str[i]))
            {
                tempformat[i] = 'X';
            }
            if ((str[i] == ' ') || (str[i] == '?'))
            {
                tempformat[i] = str[i];
            }
            if (str[i] == 0) { break; }
        }
        if (strcmp(tempformat, formatnormal) == 0)
        {
            strcat(codestring, ",");
            strcat(codestring, str.c_str());
            numcodes++;
            if (codeformat < 0) codeformat = 0;
        }
        else if (strcmp(tempformat, formatoptionlb) == 0)
        {
            if (codeformat != 2)
            {
                strcat(codestring, ",");
                strcat(codestring, str.c_str());
                numcodes++;
                codeformat = 1;
                nooptions = false;
                validoptions = false;
            }
            else
            {
                validcodes = false;
            }
        }
        else if (strcmp(tempformat, formatoptionw) == 0)
        {
            if (codeformat != 1)
            {
                strcat(codestring, ",");
                strcat(codestring, str.c_str());
                numcodes++;
                codeformat = 2;
                nooptions = false;
                validoptions = false;
            }
            else
            {
                validcodes = false;
            }
        }
        else
        {
            validcodes = false;
        }
    }
    if (strlen(codestring) == 0)
    {
        validcodes = false;
    }
    return codestring;
}

stdstr CCheatsUI::ReadOptionsString(HWND hDlg, bool &/*validcodes*/, bool &validoptions, bool &/*nooptions*/, int &codeformat)
{
    int numlines, linecount, len;
    wchar_t wc_str[128];
    int i, j;

    validoptions = true;
    int numoptions = 0;

    char optionsstring[2048];
    memset(optionsstring, '\0', sizeof(optionsstring));

    numlines = SendDlgItemMessage(hDlg, IDC_CHEAT_OPTIONS, EM_GETLINECOUNT, 0, 0);

    for (linecount = 0; linecount < numlines; linecount++) //read line after line (bypassing limitation GetDlgItemText)
    {
        memset(wc_str, 0, sizeof(wc_str));
        //str[0] = sizeof(str) > 255?255:sizeof(str);
        *(LPWORD)wc_str = sizeof(wc_str) / sizeof(wc_str[0]);
        len = SendDlgItemMessage(hDlg, IDC_CHEAT_OPTIONS, EM_GETLINE, (WPARAM)linecount, (LPARAM)(const char *)wc_str);
        wc_str[len] = 0;

        if (len > 0)
        {
            std::string str = stdstr().FromUTF16(wc_str);
            switch (codeformat)
            {
            case 1: //option = lower byte
                if (len >= 2) {
                    for (i = 0; i < 2; i++)
                    {
                        if (!isxdigit(str[i]))
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

                    for (j = 0; j < 2; j++)
                    {
                        str[j] = (char)toupper(str[j]);
                    }

                    if (optionsstring[0] == 0)
                    {
                        strcat(optionsstring, "$");
                    }
                    else
                    {
                        strcat(optionsstring, ",$");
                    }
                    strcat(optionsstring, str.c_str());
                    numoptions++;
                }
                else
                {
                    validoptions = false;
                    break;
                }
                break;

            case 2: //option = word
                if (len >= 4)
                {
                    for (i = 0; i < 4; i++)
                    {
                        if (!isxdigit(str[i]))
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

                    for (j = 0; j < 4; j++)
                    {
                        str[j] = (char)toupper(str[j]);
                    }

                    strcat(optionsstring, ",$");
                    strcat(optionsstring, str.c_str());
                    numoptions++;
                }
                else
                {
                    validoptions = false;
                    break;
                }
                break;
            default:
                break;
            }
        }
    }

    if (numoptions < 1)
    {
        validoptions = false;
    }
    return optionsstring;
}