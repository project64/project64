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
#include <Project64/Settings/UISettings.h>

#include <commctrl.h>
#include <shlobj.h>

std::string CRomBrowser::m_UnknownGoodName;

CRomBrowser::CRomBrowser(HWND & MainWindow, HWND & StatusWindow) :
    m_MainWindow(MainWindow),
    m_StatusWindow(StatusWindow),
    m_ShowingRomBrowser(false),
    m_AllowSelectionLastRom(true),
    m_WatchThreadID(0),
    m_WatchThread(NULL),
    m_WatchStopEvent(NULL)
{
    m_hRomList = 0;
    m_Visible = false;

    GetFieldInfo(m_Fields);
    m_FieldType.resize(m_Fields.size());
}

CRomBrowser::~CRomBrowser(void)
{
    WatchThreadStop();
    DeallocateBrushs();
}

void CRomBrowser::AddField(ROMBROWSER_FIELDS_LIST & Fields, LPCSTR Name, int32_t Pos, int32_t ID, int32_t Width, LanguageStringID LangID, bool UseDefault)
{
    Fields.push_back(ROMBROWSER_FIELDS(Name, Pos, ID, Width, LangID, UseDefault));
}

void CRomBrowser::GetFieldInfo(ROMBROWSER_FIELDS_LIST & Fields, bool UseDefault /* = false  */)
{
    Fields.clear();

    AddField(Fields, "File Name", -1, RB_FileName, 218, RB_FILENAME, UseDefault);
    AddField(Fields, "Internal Name", -1, RB_InternalName, 200, RB_INTERNALNAME, UseDefault);
    AddField(Fields, "Good Name", 0, RB_GoodName, 218, RB_GOODNAME, UseDefault);
    AddField(Fields, "Status", 1, RB_Status, 92, RB_STATUS, UseDefault);
    AddField(Fields, "Rom Size", -1, RB_RomSize, 100, RB_ROMSIZE, UseDefault);
    AddField(Fields, "Notes (Core)", 2, RB_CoreNotes, 120, RB_NOTES_CORE, UseDefault);
    AddField(Fields, "Notes (default plugins)", 3, RB_PluginNotes, 188, RB_NOTES_PLUGIN, UseDefault);
    AddField(Fields, "Notes (User)", -1, RB_UserNotes, 100, RB_NOTES_USER, UseDefault);
    AddField(Fields, "Cartridge ID", -1, RB_CartridgeID, 100, RB_CART_ID, UseDefault);
    AddField(Fields, "Manufacturer", -1, RB_Manufacturer, 100, RB_MANUFACTUER, UseDefault);
    AddField(Fields, "Country", -1, RB_Country, 100, RB_COUNTRY, UseDefault);
    AddField(Fields, "Developer", -1, RB_Developer, 100, RB_DEVELOPER, UseDefault);
    AddField(Fields, "CRC1", -1, RB_Crc1, 100, RB_CRC1, UseDefault);
    AddField(Fields, "CRC2", -1, RB_Crc2, 100, RB_CRC2, UseDefault);
    AddField(Fields, "CIC Chip", -1, RB_CICChip, 100, RB_CICCHIP, UseDefault);
    AddField(Fields, "Release Date", -1, RB_ReleaseDate, 100, RB_RELEASE_DATE, UseDefault);
    AddField(Fields, "Genre", -1, RB_Genre, 100, RB_GENRE, UseDefault);
    AddField(Fields, "Players", -1, RB_Players, 100, RB_PLAYERS, UseDefault);
    AddField(Fields, "Force Feedback", -1, RB_ForceFeedback, 100, RB_FORCE_FEEDBACK, UseDefault);
    AddField(Fields, "File Format", -1, RB_FileFormat, 100, RB_FILE_FORMAT, UseDefault);
}

int32_t CRomBrowser::CalcSortPosition(uint32_t lParam)
{
    int32_t Start = 0;
    int32_t End = ListView_GetItemCount(m_hRomList) - 1;
    if (End < 0)
    {
        return 0;
    }

    for (int32_t SortIndex = NoOfSortKeys; SortIndex >= 0; SortIndex--)
    {
        std::string SortFieldName = UISettingsLoadStringIndex(RomBrowser_SortFieldIndex, SortIndex);
        if (SortFieldName.length() == 0)
        {
            continue;
        }

        if (End == Start)
        {
            break;
        }

        size_t index;
        for (index = 0; index < m_Fields.size(); index++)
        {
            if (_stricmp(m_Fields[index].Name(), SortFieldName.c_str()) == 0) { break; }
        }
        if (index >= m_Fields.size()) { continue; }
        SORT_FIELD SortFieldInfo;
        SortFieldInfo._this = this;
        SortFieldInfo.Key = index;
        SortFieldInfo.KeyAscend = UISettingsLoadBoolIndex(RomBrowser_SortAscendingIndex, SortIndex);

        //calc new start and end
        int32_t LastTestPos = -1;
        while (Start < End)
        {
            int32_t TestPos = (int32_t)floor((float)((Start + End) / 2));
            if (LastTestPos == TestPos)
            {
                TestPos += 1;
            }
            LastTestPos = TestPos;

            LVITEMW lvItem;
            memset(&lvItem, 0, sizeof(lvItem));
            lvItem.mask = LVIF_PARAM;
            lvItem.iItem = TestPos;
            if (!SendMessageW(m_hRomList, LVM_GETITEMW, 0, (LPARAM)&lvItem))
            {
                return End;
            }

            int32_t Result = RomList_CompareItems(lParam, lvItem.lParam, (uint32_t)&SortFieldInfo);
            if (Result < 0)
            {
                if (End == TestPos)
                {
                    break;
                }
                End = TestPos;
            }
            else if (Result > 0)
            {
                if (Start == TestPos)
                {
                    break;
                }
                Start = TestPos;
            }
            else
            {
                //Find new start
                float Left = (float)Start;
                float Right = (float)TestPos;
                while (Left < Right)
                {
                    int32_t NewTestPos = (int32_t)floor((Left + Right) / 2);
                    if (LastTestPos == NewTestPos)
                    {
                        NewTestPos += 1;
                    }
                    LastTestPos = NewTestPos;

                    memset(&lvItem, 0, sizeof(lvItem));
                    lvItem.mask = LVIF_PARAM;
                    lvItem.iItem = NewTestPos;
                    if (!SendMessageW(m_hRomList, LVM_GETITEMW, 0, (LPARAM)&lvItem))
                    {
                        return End;
                    }

                    Result = RomList_CompareItems(lParam, lvItem.lParam, (uint32_t)&SortFieldInfo);
                    if (Result <= 0)
                    {
                        if (Right == NewTestPos)
                        {
                            break;
                        }
                        Right = (float)NewTestPos;
                    }
                    else if (Result > 0)
                    {
                        Left = Left != (float)NewTestPos ? (float)NewTestPos : Left + 1;
                    }
                }
                Start = (int32_t)((float)Right);

                //Find new end
                Left = (float)TestPos;
                Right = (float)End;
                while (Left < Right)
                {
                    int32_t NewTestPos = (int32_t)ceil((Left + Right) / 2);
                    if (LastTestPos == NewTestPos)
                    {
                        NewTestPos -= 1;
                    }
                    LastTestPos = NewTestPos;

                    memset(&lvItem, 0, sizeof(lvItem));
                    lvItem.mask = LVIF_PARAM;
                    lvItem.iItem = NewTestPos;
                    if (!SendMessageW(m_hRomList, LVM_GETITEMW, 0, (LPARAM)&lvItem)) { return End; }

                    Result = RomList_CompareItems(lParam, lvItem.lParam, (uint32_t)&SortFieldInfo);
                    if (Result >= 0)
                    {
                        if (Left == NewTestPos)
                        {
                            break;
                        }
                        Left = (float)NewTestPos;
                    }
                    if (Result < 0)
                    {
                        Right = (float)NewTestPos;
                    }
                }
                End = (int32_t)Left;
                break;
            }
        }
    }

    //Compare end with item to see if we should do it after or before it
    for (int32_t SortIndex = 0; SortIndex < NoOfSortKeys; SortIndex++)
    {
        std::string SortFieldName = UISettingsLoadStringIndex(RomBrowser_SortFieldIndex, SortIndex);
        if (SortFieldName.length() == 0)
        {
            continue;
        }

        size_t index;
        for (index = 0; index < m_Fields.size(); index++)
        {
            if (_stricmp(m_Fields[index].Name(), SortFieldName.c_str()) == 0) { break; }
        }
        if (index >= m_Fields.size()) { continue; }
        SORT_FIELD SortFieldInfo;
        SortFieldInfo._this = this;
        SortFieldInfo.Key = index;
        SortFieldInfo.KeyAscend = UISettingsLoadBoolIndex(RomBrowser_SortAscendingIndex, SortIndex) != 0;

        LVITEMW lvItem;
        memset(&lvItem, 0, sizeof(LVITEMW));
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = End;
        if (!SendMessageW(m_hRomList, LVM_GETITEMW, 0, (LPARAM)&lvItem)) { return End; }

        int32_t Result = RomList_CompareItems(lParam, lvItem.lParam, (uint32_t)&SortFieldInfo);
        if (Result < 0)
        {
            return End;
        }
        if (Result > 0)
        {
            return End + 1;
        }
    }
    return End + 1;
}

void CRomBrowser::RomAddedToList(int32_t ListPos)
{
    LVITEMW lvItem;
    memset(&lvItem, 0, sizeof(lvItem));
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.iItem = CalcSortPosition(ListPos);
    lvItem.lParam = (LPARAM)ListPos;
    lvItem.pszText = LPSTR_TEXTCALLBACKW;

    int32_t index = SendMessageW(m_hRomList, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);
    int32_t iItem = ListView_GetNextItem(m_hRomList, -1, LVNI_SELECTED);

    //if the last rom then highlight the item
    if (iItem < 0 && _stricmp(m_RomInfo[ListPos].szFullFileName, m_LastRom.c_str()) == 0)
    {
        ListView_SetItemState(m_hRomList, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    if (iItem >= 0)
    {
        ListView_EnsureVisible(m_hRomList, iItem, FALSE);
    }
}

void CRomBrowser::RomListReset(void)
{
    WriteTrace(TraceUserInterface, TraceDebug, "1");
    ListView_DeleteAllItems(m_hRomList);
    WriteTrace(TraceUserInterface, TraceDebug, "2");
    InvalidateRect(m_hRomList, NULL, TRUE);
    Sleep(100);
    WriteTrace(TraceUserInterface, TraceDebug, "3");
    m_LastRom = UISettingsLoadStringIndex(File_RecentGameFileIndex, 0);

    if (m_WatchRomDir != g_Settings->LoadStringVal(RomList_GameDir))
    {
        WriteTrace(TraceUserInterface, TraceDebug, "4");
        WatchThreadStop();
        WriteTrace(TraceUserInterface, TraceDebug, "5");
        WatchThreadStart();
        WriteTrace(TraceUserInterface, TraceDebug, "6");
    }
}

void CRomBrowser::CreateRomListControl(void)
{
    m_hRomList = CreateWindowW(WC_LISTVIEWW, NULL, WS_TABSTOP | WS_VISIBLE | WS_CHILD | LVS_OWNERDRAWFIXED | LVS_SINGLESEL | LVS_REPORT, 0, 0, 0, 0, m_MainWindow, (HMENU)IDC_ROMLIST, GetModuleHandle(NULL), NULL);
    ResetRomBrowserColomuns();
    LoadRomList();
}

void CRomBrowser::DeallocateBrushs(void)
{
    for (HBRUSH_MAP::iterator itr = m_Brushes.begin(); itr != m_Brushes.end(); itr++)
    {
        DeleteObject(itr->second);
    }
    m_Brushes.clear();
}

void CRomBrowser::RomListLoaded(void)
{
    RomList_SortList();
}

void CRomBrowser::RomDirChanged(void)
{
    PostMessage(m_MainWindow, WM_COMMAND, ID_FILE_REFRESHROMLIST, 0);
}

void CRomBrowser::HighLightLastRom(void)
{
    if (!m_AllowSelectionLastRom)
    {
        return;
    }
    m_LastRom = UISettingsLoadStringIndex(File_RecentGameFileIndex, 0);

    //Make sure Rom browser is visible
    if (!RomBrowserVisible()) { return; }

    LVITEMW lvItem;
    lvItem.mask = LVIF_PARAM;

    int32_t ItemCount = ListView_GetItemCount(m_hRomList);
    for (int32_t index = 0; index < ItemCount; index++)
    {
        //Get The next item
        lvItem.iItem = index;
        if (!SendMessageW(m_hRomList, LVM_GETITEMW, 0, (LPARAM)&lvItem)) { return; }

        //Get the rom info for that item
        if (lvItem.lParam < 0 || lvItem.lParam >= (LPARAM)m_RomInfo.size())
        {
            return;
        }
        ROM_INFO * pRomInfo = &m_RomInfo[lvItem.lParam];

        if (!m_AllowSelectionLastRom)
        {
            return;
        }

        //if the last rom then highlight the item
        if (_stricmp(pRomInfo->szFullFileName, m_LastRom.c_str()) == 0)
        {
            ListView_SetItemState(m_hRomList, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(m_hRomList, index, FALSE);
            return;
        }
    }
}

void CRomBrowser::MenuSetText(HMENU hMenu, int32_t MenuPos, const wchar_t * Title, char * ShortCut)
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

    GetMenuItemInfoW(hMenu, MenuPos, TRUE, &MenuInfo);
    wcscpy(String, Title);
    if (wcschr(String, '\t') != NULL) { *(wcschr(String, '\t')) = '\0'; }
    if (ShortCut) { swprintf(String, sizeof(String) / sizeof(String[0]), L"%s\t%s", String, stdstr(ShortCut).ToUTF16().c_str()); }
    SetMenuItemInfoW(hMenu, MenuPos, TRUE, &MenuInfo);
}

void CRomBrowser::ResetRomBrowserColomuns(void)
{
    size_t Coloumn, index;
    LV_COLUMNW lvColumn;
    wchar_t szString[300];

    GetFieldInfo(m_Fields);

    //Remove all current coloumns
    memset(&lvColumn, 0, sizeof(lvColumn));
    lvColumn.mask = LVCF_FMT;
    while (ListView_GetColumn(m_hRomList, 0, &lvColumn))
    {
        ListView_DeleteColumn(m_hRomList, 0);
    }

    //Add Colomuns
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.pszText = szString;

    for (Coloumn = 0; Coloumn < m_Fields.size(); Coloumn++)
    {
        for (index = 0; index < m_Fields.size(); index++)
        {
            if (m_Fields[index].Pos() == Coloumn) { break; }
        }
        if (index == m_Fields.size() || m_Fields[index].Pos() != Coloumn)
        {
            m_FieldType[Coloumn] = -1;
            break;
        }

        m_FieldType[Coloumn] = m_Fields[index].ID();
        lvColumn.cx = m_Fields[index].ColWidth();
        wcsncpy(szString, wGS(m_Fields[index].LangID()).c_str(), sizeof(szString) / sizeof(szString[0]));
        SendMessage(m_hRomList, LVM_INSERTCOLUMNW, (WPARAM)(int32_t)(Coloumn), (LPARAM)(const LV_COLUMNW *)(&lvColumn));
    }
}

void CRomBrowser::ResizeRomList(WORD nWidth, WORD nHeight)
{
    if (RomBrowserVisible())
    {
        if (UISettingsLoadBool(RomBrowser_Maximized) == 0 && nHeight != 0)
        {
            if (UISettingsLoadDword(RomBrowser_Width) != nWidth)
            {
                UISettingsSaveDword(RomBrowser_Width, nWidth);
            }
            if (UISettingsLoadDword(RomBrowser_Height) != nHeight)
            {
                UISettingsSaveDword(RomBrowser_Height, nHeight);
            }
        }
        if (IsWindow((HWND)m_StatusWindow))
        {
            RECT rc;

            GetWindowRect((HWND)m_StatusWindow, &rc);
            nHeight -= (WORD)(rc.bottom - rc.top);
        }
        MoveWindow(m_hRomList, 0, 0, nWidth, nHeight, TRUE);
    }
}

bool CRomBrowser::RomBrowserVisible(void)
{
    if (!IsWindow(m_hRomList)) { return false; }
    if (!IsWindowVisible(m_hRomList)) { return false; }
    if (!m_Visible) { return false; }
    return true;
}

void CRomBrowser::RomBrowserToTop(void)
{
    BringWindowToTop(m_hRomList);
    SetFocus(m_hRomList);
}

void CRomBrowser::RomBrowserMaximize(bool Mazimize)
{
    UISettingsSaveBool(RomBrowser_Maximized, Mazimize);
}

bool CRomBrowser::RomListDrawItem(int32_t idCtrl, uint32_t lParam)
{
    if (idCtrl != IDC_ROMLIST) { return false; }
    LPDRAWITEMSTRUCT ditem = (LPDRAWITEMSTRUCT)lParam;

    RECT rcItem, rcDraw;
    wchar_t String[300];
    LVITEMW lvItem;
    HBRUSH hBrush = (HBRUSH)(COLOR_WINDOW + 1);
    LV_COLUMN lvc;
    int32_t nColumn;

    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = ditem->itemID;
    if (!SendMessageW(m_hRomList, LVM_GETITEMW, 0, (LPARAM)&lvItem)) { return false; }
    lvItem.state = ListView_GetItemState(m_hRomList, ditem->itemID, -1);
    bool bSelected = (lvItem.state & LVIS_SELECTED) != 0;

    if (lvItem.lParam < 0 || lvItem.lParam >= (LPARAM)m_RomInfo.size())
    {
        return true;
    }
    ROM_INFO * pRomInfo = &m_RomInfo[lvItem.lParam];
    if (pRomInfo == NULL)
    {
        return true;
    }
    if (bSelected)
    {
        HBRUSH_MAP::iterator itr = m_Brushes.find(pRomInfo->SelColor);
        if (itr != m_Brushes.end())
        {
            hBrush = itr->second;
        }
        else
        {
            std::pair<HBRUSH_MAP::iterator, bool> res = m_Brushes.insert(HBRUSH_MAP::value_type(pRomInfo->SelColor, CreateSolidBrush(pRomInfo->SelColor)));
            hBrush = res.first->second;
        }
        SetTextColor(ditem->hDC, pRomInfo->SelTextColor);
    }
    else
    {
        SetTextColor(ditem->hDC, pRomInfo->TextColor);
    }
    FillRect(ditem->hDC, &ditem->rcItem, hBrush);
    SetBkMode(ditem->hDC, TRANSPARENT);

    //Draw
    ListView_GetItemRect(m_hRomList, ditem->itemID, &rcItem, LVIR_LABEL);
    lvItem.iSubItem = 0;
    lvItem.cchTextMax = sizeof(String) / sizeof(String[0]);
    lvItem.pszText = String;
    SendMessageW(m_hRomList, LVM_GETITEMTEXTW, (WPARAM)ditem->itemID, (LPARAM)&lvItem);

    memcpy(&rcDraw, &rcItem, sizeof(RECT));
    rcDraw.right -= 3;
    std::wstring text = String;
    if (wcscmp(L"#340#", text.c_str()) == 0)
    {
        text = wGS(RB_NOT_GOOD_FILE);
    }

    DrawTextW(ditem->hDC, text.c_str(), text.length(), &rcDraw, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_WORD_ELLIPSIS);

    memset(&lvc, 0, sizeof(lvc));
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    for (nColumn = 1; ListView_GetColumn(m_hRomList, nColumn, &lvc); nColumn += 1)
    {
        rcItem.left = rcItem.right;
        rcItem.right += lvc.cx;

        lvItem.iSubItem = nColumn;
        lvItem.cchTextMax = sizeof(String) / sizeof(String[0]);
        lvItem.pszText = String;
        SendMessageW(m_hRomList, LVM_GETITEMTEXTW, ditem->itemID, (LPARAM)&lvItem);
        memcpy(&rcDraw, &rcItem, sizeof(RECT));
        rcDraw.right -= 3;
        text = String;
        if (wcscmp(L"#340#", text.c_str()) == 0)
        {
            text = wGS(RB_NOT_GOOD_FILE);
        }
        DrawTextW(ditem->hDC, text.c_str(), text.length(), &rcDraw, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_WORD_ELLIPSIS);
    }
    return true;
}

bool CRomBrowser::RomListNotify(int32_t idCtrl, uint32_t pnmh)
{
    if (idCtrl != IDC_ROMLIST) { return false; }
    if (!RomBrowserVisible()) { return false; }

    switch (((LPNMHDR)pnmh)->code)
    {
    case LVN_COLUMNCLICK: RomList_ColoumnSortList(pnmh); break;
    case NM_RETURN:       RomList_OpenRom(pnmh); break;
    case NM_DBLCLK:       RomList_OpenRom(pnmh); break;
    case LVN_GETDISPINFOW: RomList_GetDispInfo(pnmh); break;
    case NM_RCLICK:       RomList_PopupMenu(pnmh); break;
    case NM_CLICK:
    {
        LONG iItem = ListView_GetNextItem(m_hRomList, -1, LVNI_SELECTED);
        if (iItem != -1)
        {
            m_AllowSelectionLastRom = false;
        }
    }
    break;
    default:
        return false;
    }
    return true;
}

void CRomBrowser::RomList_ColoumnSortList(uint32_t pnmh)
{
    LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;
    size_t index;

    for (index = 0; index < m_Fields.size(); index++)
    {
        if (m_Fields[index].Pos() == (size_t)pnmv->iSubItem) { break; }
    }
    if (m_Fields.size() == index) { return; }
    if (_stricmp(UISettingsLoadStringIndex(RomBrowser_SortFieldIndex, 0).c_str(), m_Fields[index].Name()) == 0)
    {
        UISettingsSaveBoolIndex(RomBrowser_SortAscendingIndex, 0, !UISettingsLoadBoolIndex(RomBrowser_SortAscendingIndex, 0));
    }
    else
    {
        int32_t count;

        for (count = NoOfSortKeys; count > 0; count--)
        {
            UISettingsSaveStringIndex(RomBrowser_SortFieldIndex, count, UISettingsLoadStringIndex(RomBrowser_SortFieldIndex, count - 1).c_str());
            UISettingsSaveBoolIndex(RomBrowser_SortAscendingIndex, count, UISettingsLoadBoolIndex(RomBrowser_SortAscendingIndex, count - 1));
        }
        UISettingsSaveStringIndex(RomBrowser_SortFieldIndex, 0, m_Fields[index].Name());
        UISettingsSaveBoolIndex(RomBrowser_SortAscendingIndex, 0, true);
    }
    RomList_SortList();
}

int32_t CALLBACK CRomBrowser::RomList_CompareItems(uint32_t lParam1, uint32_t lParam2, uint32_t lParamSort)
{
    SORT_FIELD * SortFieldInfo = (SORT_FIELD *)lParamSort;
    CRomBrowser * _this = SortFieldInfo->_this;
    if (lParam1 < 0 || lParam1 >= _this->m_RomInfo.size())
    {
        return 0;
    }
    if (lParam2 < 0 || lParam2 >= _this->m_RomInfo.size())
    {
        return 0;
    }
    ROM_INFO * pRomInfo1 = &_this->m_RomInfo[SortFieldInfo->KeyAscend ? lParam1 : lParam2];
    ROM_INFO * pRomInfo2 = &_this->m_RomInfo[SortFieldInfo->KeyAscend ? lParam2 : lParam1];
    int32_t result;

    const char * GoodName1 = NULL, *GoodName2 = NULL;
    if (SortFieldInfo->Key == RB_GoodName)
    {
        GoodName1 = strcmp("#340#", pRomInfo1->GoodName) != 0 ? pRomInfo1->GoodName : m_UnknownGoodName.c_str();
        GoodName2 = strcmp("#340#", pRomInfo2->GoodName) != 0 ? pRomInfo2->GoodName : m_UnknownGoodName.c_str();
    }

    switch (SortFieldInfo->Key)
    {
    case RB_FileName: result = (int32_t)lstrcmpi(pRomInfo1->FileName, pRomInfo2->FileName); break;
    case RB_InternalName: result = (int32_t)lstrcmpi(pRomInfo1->InternalName, pRomInfo2->InternalName); break;
    case RB_GoodName: result = (int32_t)lstrcmpi(GoodName1, GoodName2); break;
    case RB_Status: result = (int32_t)lstrcmpi(pRomInfo1->Status, pRomInfo2->Status); break;
    case RB_RomSize: result = (int32_t)pRomInfo1->RomSize - (int32_t)pRomInfo2->RomSize; break;
    case RB_CoreNotes: result = (int32_t)lstrcmpi(pRomInfo1->CoreNotes, pRomInfo2->CoreNotes); break;
    case RB_PluginNotes: result = (int32_t)lstrcmpi(pRomInfo1->PluginNotes, pRomInfo2->PluginNotes); break;
    case RB_UserNotes: result = (int32_t)lstrcmpi(pRomInfo1->UserNotes, pRomInfo2->UserNotes); break;
    case RB_CartridgeID: result = (int32_t)lstrcmpi(pRomInfo1->CartID, pRomInfo2->CartID); break;
    case RB_Manufacturer: result = (int32_t)pRomInfo1->Manufacturer - (int32_t)pRomInfo2->Manufacturer; break;
    case RB_Country: result = (int32_t)pRomInfo1->Country - (int32_t)pRomInfo2->Country; break;
    case RB_Developer: result = (int32_t)lstrcmpi(pRomInfo1->Developer, pRomInfo2->Developer); break;
    case RB_Crc1: result = (int32_t)pRomInfo1->CRC1 - (int32_t)pRomInfo2->CRC1; break;
    case RB_Crc2: result = (int32_t)pRomInfo1->CRC2 - (int32_t)pRomInfo2->CRC2; break;
    case RB_CICChip: result = (int32_t)pRomInfo1->CicChip - (int32_t)pRomInfo2->CicChip; break;
    case RB_ReleaseDate: result = (int32_t)lstrcmpi(pRomInfo1->ReleaseDate, pRomInfo2->ReleaseDate); break;
    case RB_Players: result = (int32_t)pRomInfo1->Players - (int32_t)pRomInfo2->Players; break;
    case RB_ForceFeedback: result = (int32_t)lstrcmpi(pRomInfo1->ForceFeedback, pRomInfo2->ForceFeedback); break;
    case RB_Genre: result = (int32_t)lstrcmpi(pRomInfo1->Genre, pRomInfo2->Genre); break;
    case RB_FileFormat: result = (int32_t)pRomInfo1->FileFormat - (int32_t)pRomInfo2->FileFormat; break;
    default: result = 0; break;
    }
    return result;
}

void CRomBrowser::RomList_GetDispInfo(uint32_t pnmh)
{
    LV_DISPINFOW * lpdi = (LV_DISPINFOW *)pnmh;
    if (lpdi->item.lParam < 0 || lpdi->item.lParam >= (LPARAM)m_RomInfo.size())
    {
        return;
    }

    ROM_INFO * pRomInfo = &m_RomInfo[lpdi->item.lParam];

    if (pRomInfo == NULL)
    {
        wcscpy(lpdi->item.pszText, L" ");
        return;
    }

    switch (m_FieldType[lpdi->item.iSubItem])
    {
    case RB_FileName: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->FileName).ToUTF16(CP_ACP).c_str(), lpdi->item.cchTextMax); break;
    case RB_InternalName: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->InternalName).ToUTF16(stdstr::CODEPAGE_932).c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_GoodName: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->GoodName).ToUTF16().c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_CoreNotes: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->CoreNotes).ToUTF16().c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_PluginNotes: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->PluginNotes).ToUTF16().c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_Status: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->Status).ToUTF16().c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_RomSize: swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"%.1f MBit", (float)pRomInfo->RomSize / 0x20000); break;
    case RB_CartridgeID: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->CartID).ToUTF16().c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_Manufacturer:
        switch (pRomInfo->Manufacturer)
        {
        case 'N':wcsncpy(lpdi->item.pszText, L"Nintendo", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 0:  wcsncpy(lpdi->item.pszText, L"None", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        default: swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"(Unknown %c (%X))", pRomInfo->Manufacturer, pRomInfo->Manufacturer); break;
        }
        break;
    case RB_Country:
        switch (pRomInfo->Country)
        {
        case '7': wcsncpy(lpdi->item.pszText, L"Beta", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'A': wcsncpy(lpdi->item.pszText, L"NTSC", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'D': wcsncpy(lpdi->item.pszText, L"Germany", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'E': wcsncpy(lpdi->item.pszText, L"America", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'F': wcsncpy(lpdi->item.pszText, L"France", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'J': wcsncpy(lpdi->item.pszText, L"Japan", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'I': wcsncpy(lpdi->item.pszText, L"Italy", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'P': wcsncpy(lpdi->item.pszText, L"Europe", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'S': wcsncpy(lpdi->item.pszText, L"Spain", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'U': wcsncpy(lpdi->item.pszText, L"Australia", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'X': wcsncpy(lpdi->item.pszText, L"PAL", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 'Y': wcsncpy(lpdi->item.pszText, L"PAL", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case 0: wcsncpy(lpdi->item.pszText, L"None", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        default: swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"Unknown %c (%02X)", pRomInfo->Country, pRomInfo->Country); break;
        }
        break;
    case RB_Crc1: swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"0x%08X", pRomInfo->CRC1); break;
    case RB_Crc2: swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"0x%08X", pRomInfo->CRC2); break;
    case RB_CICChip:
        if (pRomInfo->CicChip < 0)
        {
            swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"Unknown CIC Chip");
        }
        else if (pRomInfo->CicChip == CIC_NUS_8303)
        {
            swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"CIC-NUS-8303");
        }
        else if (pRomInfo->CicChip == CIC_NUS_5167)
        {
            swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"CIC-NUS-5167");
        }
        else if (pRomInfo->CicChip == CIC_NUS_DDUS)
        {
            swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"CIC-NUS-????");
        }
        else
        {
            swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"CIC-NUS-610%d", pRomInfo->CicChip);
        }
        break;
    case RB_UserNotes: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->UserNotes).ToUTF16().c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_Developer: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->Developer).ToUTF16().c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_ReleaseDate: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->ReleaseDate).ToUTF16().c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_Genre: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->Genre).ToUTF16().c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_Players: swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"%d", pRomInfo->Players); break;
    case RB_ForceFeedback: wcsncpy(lpdi->item.pszText, stdstr(pRomInfo->ForceFeedback).ToUTF16().c_str(), lpdi->item.cchTextMax / sizeof(wchar_t)); break;
    case RB_FileFormat:
        switch (pRomInfo->FileFormat)
        {
        case Format_Uncompressed: wcsncpy(lpdi->item.pszText, L"Uncompressed", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case Format_Zip:          wcsncpy(lpdi->item.pszText, L"Zip", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        case Format_7zip:         wcsncpy(lpdi->item.pszText, L"7zip", lpdi->item.cchTextMax / sizeof(wchar_t)); break;
        default: swprintf(lpdi->item.pszText, lpdi->item.cchTextMax / sizeof(wchar_t), L"Unknown (%X)", pRomInfo->FileFormat); break;
        }
        break;
    default: wcsncpy(lpdi->item.pszText, L" ", lpdi->item.cchTextMax);
    }
    if (lpdi->item.pszText == NULL || wcslen(lpdi->item.pszText) == 0) { lpdi->item.pszText = L" "; }
}

void CRomBrowser::RomList_OpenRom(uint32_t /*pnmh*/)
{
    ROM_INFO * pRomInfo;
    LV_ITEM lvItem;
    LONG iItem;

    iItem = ListView_GetNextItem(m_hRomList, -1, LVNI_SELECTED);
    if (iItem == -1) { return; }

    memset(&lvItem, 0, sizeof(LV_ITEM));
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = iItem;
    if (!SendMessageW(m_hRomList, LVM_GETITEMW, 0, (LPARAM)&lvItem)) { return; }
    if (lvItem.lParam < 0 || lvItem.lParam >= (LPARAM)m_RomInfo.size())
    {
        return;
    }
    pRomInfo = &m_RomInfo[lvItem.lParam];

    if (!pRomInfo) { return; }
    m_StopRefresh = true;
    delete g_DDRom;
    g_DDRom = NULL;

    CN64System::RunFileImage(pRomInfo->szFullFileName);
}

void CRomBrowser::RomList_PopupMenu(uint32_t /*pnmh*/)
{
    LONG iItem = ListView_GetNextItem(m_hRomList, -1, LVNI_SELECTED);
    m_SelectedRom = "";
    if (iItem != -1)
    {
        LV_ITEM lvItem;
        memset(&lvItem, 0, sizeof(LV_ITEM));
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iItem;
        if (!SendMessageW(m_hRomList, LVM_GETITEMW, 0, (LPARAM)&lvItem)) { return; }
        if (lvItem.lParam < 0 || lvItem.lParam >= (LPARAM)m_RomInfo.size())
        {
            return;
        }
        ROM_INFO * pRomInfo = &m_RomInfo[lvItem.lParam];

        if (!pRomInfo) { return; }
        m_SelectedRom = pRomInfo->szFullFileName;
    }

    //Load the menu
    HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_POPUP));
    HMENU hPopupMenu = (HMENU)GetSubMenu(hMenu, 0);

    //Fix up menu
    MenuSetText(hPopupMenu, 0, wGS(POPUP_PLAY).c_str(), NULL);
    MenuSetText(hPopupMenu, 1, wGS(POPUP_PLAYDISK).c_str(), NULL);
    MenuSetText(hPopupMenu, 3, wGS(MENU_REFRESH).c_str(), NULL);
    MenuSetText(hPopupMenu, 4, wGS(MENU_CHOOSE_ROM).c_str(), NULL);
    MenuSetText(hPopupMenu, 6, wGS(POPUP_INFO).c_str(), NULL);
    MenuSetText(hPopupMenu, 7, wGS(POPUP_GFX_PLUGIN).c_str(), NULL);
    MenuSetText(hPopupMenu, 9, wGS(POPUP_SETTINGS).c_str(), NULL);
    MenuSetText(hPopupMenu, 10, wGS(POPUP_CHEATS).c_str(), NULL);

    if (m_SelectedRom.size() == 0)
    {
        DeleteMenu(hPopupMenu, 10, MF_BYPOSITION);
        DeleteMenu(hPopupMenu, 9, MF_BYPOSITION);
        DeleteMenu(hPopupMenu, 8, MF_BYPOSITION);
        DeleteMenu(hPopupMenu, 7, MF_BYPOSITION);
        DeleteMenu(hPopupMenu, 6, MF_BYPOSITION);
        DeleteMenu(hPopupMenu, 5, MF_BYPOSITION);
        DeleteMenu(hPopupMenu, 2, MF_BYPOSITION);
        DeleteMenu(hPopupMenu, 1, MF_BYPOSITION);
        DeleteMenu(hPopupMenu, 0, MF_BYPOSITION);
    }
    else
    {
        bool inBasicMode = g_Settings->LoadDword(UserInterface_BasicMode) != 0;
        bool CheatsRemembered = g_Settings->LoadDword(Setting_RememberCheats) != 0;
        if (!CheatsRemembered) { DeleteMenu(hPopupMenu, 10, MF_BYPOSITION); }
        if (inBasicMode) { DeleteMenu(hPopupMenu, 9, MF_BYPOSITION); }
        if (inBasicMode && !CheatsRemembered) { DeleteMenu(hPopupMenu, 8, MF_BYPOSITION); }
        DeleteMenu(hPopupMenu, 7, MF_BYPOSITION);
        if (!inBasicMode && g_Plugins && g_Plugins->Gfx() && g_Plugins->Gfx()->GetRomBrowserMenu != NULL)
        {
            HMENU GfxMenu = (HMENU)g_Plugins->Gfx()->GetRomBrowserMenu();
            if (GfxMenu)
            {
                MENUITEMINFO lpmii;
                InsertMenuW(hPopupMenu, 7, MF_POPUP | MF_BYPOSITION, (uint32_t)GfxMenu, wGS(POPUP_GFX_PLUGIN).c_str());
                lpmii.cbSize = sizeof(MENUITEMINFO);
                lpmii.fMask = MIIM_STATE;
                lpmii.fState = 0;
                SetMenuItemInfo(hPopupMenu, (uint32_t)GfxMenu, MF_BYCOMMAND, &lpmii);
            }
        }
    }

    //Get the current Mouse location
    POINT Mouse;
    GetCursorPos(&Mouse);

    //Show the menu
    TrackPopupMenu(hPopupMenu, 0, Mouse.x, Mouse.y, 0, m_MainWindow, NULL);
    DestroyMenu(hMenu);
}

void CRomBrowser::RomList_SortList(void)
{
    SORT_FIELD SortFieldInfo;
    m_UnknownGoodName = stdstr().FromUTF16(wGS(RB_NOT_GOOD_FILE).c_str());

    for (int32_t count = NoOfSortKeys; count >= 0; count--)
    {
        stdstr SortFieldName = UISettingsLoadStringIndex(RomBrowser_SortFieldIndex, count);

        size_t index;
        for (index = 0; index < m_Fields.size(); index++)
        {
            if (_stricmp(m_Fields[index].Name(), SortFieldName.c_str()) == 0) { break; }
        }
        if (index >= m_Fields.size()) { continue; }
        SortFieldInfo._this = this;
        SortFieldInfo.Key = index;
        SortFieldInfo.KeyAscend = UISettingsLoadBoolIndex(RomBrowser_SortAscendingIndex, count) != 0;
        ListView_SortItems(m_hRomList, RomList_CompareItems, &SortFieldInfo);
    }
}

void CRomBrowser::SaveRomListColoumnInfo(void)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    //	if (!RomBrowserVisible()) { return; }
    if (g_Settings == NULL) { return; }

    LV_COLUMN lvColumn;

    memset(&lvColumn, 0, sizeof(lvColumn));
    lvColumn.mask = LVCF_WIDTH;

    for (size_t Coloumn = 0; ListView_GetColumn(m_hRomList, Coloumn, &lvColumn); Coloumn++)
    {
        size_t index;
        bool bFound = false;
        for (index = 0; index < m_Fields.size(); index++)
        {
            if (m_Fields[index].Pos() == Coloumn)
            {
                bFound = true;
                break;
            }
        }
        if (bFound)
        {
            if (m_Fields[index].ColWidth() != lvColumn.cx)
            {
                m_Fields[index].SetColWidth(lvColumn.cx);
            }
        }
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

int32_t CALLBACK CRomBrowser::SelectRomDirCallBack(HWND hwnd, uint32_t uMsg, uint32_t /*lp*/, uint32_t lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        // WParam is TRUE since you are passing a path.
        // It would be FALSE if you were passing a pidl.
        if (lpData)
        {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
            SetWindowTextW(hwnd, wGS(DIR_SELECT_ROM).c_str());
        }
        break;
    }
    return 0;
}

void CRomBrowser::SelectRomDir(void)
{
    wchar_t SelectedDir[MAX_PATH];
    LPITEMIDLIST pidl;
    BROWSEINFOW bi;

    std::wstring title = wGS(SELECT_ROM_DIR);

    WriteTrace(TraceUserInterface, TraceDebug, "1");
    stdstr RomDir = g_Settings->LoadStringVal(RomList_GameDir).c_str();
    bi.hwndOwner = m_MainWindow;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = SelectedDir;
    bi.lpszTitle = title.c_str();
    bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    bi.lpfn = (BFFCALLBACK)SelectRomDirCallBack;
    bi.lParam = (uint32_t)RomDir.c_str();
    WriteTrace(TraceUserInterface, TraceDebug, "2");
    if ((pidl = SHBrowseForFolderW(&bi)) != NULL)
    {
        WriteTrace(TraceUserInterface, TraceDebug, "3");
        char Directory[_MAX_PATH];
        if (SHGetPathFromIDList(pidl, Directory))
        {
            int32_t len = strlen(Directory);

            WriteTrace(TraceUserInterface, TraceDebug, "4");
            if (Directory[len - 1] != '\\')
            {
                strcat(Directory, "\\");
            }
            WriteTrace(TraceUserInterface, TraceDebug, "5");
            WriteTrace(TraceUserInterface, TraceDebug, "6");
            g_Settings->SaveString(RomList_GameDir, Directory);
            WriteTrace(TraceUserInterface, TraceDebug, "7");
            Notify().AddRecentDir(Directory);
            WriteTrace(TraceUserInterface, TraceDebug, "8");
            RefreshRomList();
            WriteTrace(TraceUserInterface, TraceDebug, "9");
        }
    }
}

void CRomBrowser::FixRomListWindow(void)
{
    //Change the window Style
    long Style = GetWindowLong(m_MainWindow, GWL_STYLE) | WS_SIZEBOX | WS_MAXIMIZEBOX;
    SetWindowLong(m_MainWindow, GWL_STYLE, Style);

    //Get the current window size
    RECT rect;
    GetWindowRect(m_MainWindow, &rect);

    //We find the middle position of the screen, we use this if theres no setting
    int32_t X = (GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
    int32_t	Y = (GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;

    //Load the value from settings, if none is available, default to above
    UISettingsLoadDword(RomBrowser_Top, (uint32_t &)Y);
    UISettingsLoadDword(RomBrowser_Left, (uint32_t &)X);

    SetWindowPos(m_MainWindow, NULL, X, Y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    //Fix height and width
    int32_t Width = UISettingsLoadDword(RomBrowser_Width);
    int32_t Height = UISettingsLoadDword(RomBrowser_Height);

    if (Width < 200) { Width = 200; }
    if (Height < 200) { Height = 200; }

    RECT rcClient;
    rcClient.top = 0;
    rcClient.bottom = Height;
    rcClient.left = 0;
    rcClient.right = Width;
    AdjustWindowRect(&rcClient, GetWindowLong(m_MainWindow, GWL_STYLE), true);

    int32_t WindowHeight = rcClient.bottom - rcClient.top;
    int32_t WindowWidth = rcClient.right - rcClient.left;

    SetWindowPos(m_MainWindow, NULL, 0, 0, WindowWidth, WindowHeight, SWP_NOMOVE | SWP_NOZORDER);
}

void CRomBrowser::ShowRomList(void)
{
    if (m_Visible || g_Settings->LoadBool(GameRunning_CPU_Running)) { return; }
    m_ShowingRomBrowser = true;
    WatchThreadStop();
    if (m_hRomList == NULL) { CreateRomListControl(); }
    EnableWindow(m_hRomList, TRUE);
    ShowWindow(m_hRomList, SW_SHOW);
    FixRomListWindow();
    m_AllowSelectionLastRom = true;

    //Make sure selected item is visible
    int32_t iItem = ListView_GetNextItem(m_hRomList, -1, LVNI_SELECTED);
    ListView_EnsureVisible(m_hRomList, iItem, FALSE);

    //Mark the window as visible
    m_Visible = true;

    RECT rcWindow;
    if (GetClientRect(m_MainWindow, &rcWindow))
    {
        ResizeRomList((WORD)rcWindow.right, (WORD)rcWindow.bottom);
    }

    InvalidateRect(m_hRomList, NULL, TRUE);

    //Start thread to watch for dir changed
    WatchThreadStart();
    m_ShowingRomBrowser = false;
}

void CRomBrowser::HideRomList(void)
{
    if (!RomBrowserVisible()) { return; }
    ShowWindow(m_MainWindow, SW_HIDE);

    SaveRomListColoumnInfo();
    WatchThreadStop();

    //Make sure the window does disappear
    Sleep(100);

    //Disable the rom list
    EnableWindow(m_hRomList, FALSE);
    ShowWindow(m_hRomList, SW_HIDE);

    if (UISettingsLoadBool(RomBrowser_Maximized)) { ShowWindow(m_MainWindow, SW_RESTORE); }

    //Change the window style
    long Style = GetWindowLong(m_MainWindow, GWL_STYLE) &	~(WS_SIZEBOX | WS_MAXIMIZEBOX);
    SetWindowLong(m_MainWindow, GWL_STYLE, Style);

    //Move window to correct location
    RECT rect;
    GetWindowRect(m_MainWindow, &rect);
    int32_t X = (GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
    int32_t	Y = (GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;
    UISettingsLoadDword(UserInterface_MainWindowTop, (uint32_t &)Y);
    UISettingsLoadDword(UserInterface_MainWindowLeft, (uint32_t &)X);
    SetWindowPos(m_MainWindow, NULL, X, Y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    //Mark the window as not visible
    m_Visible = false;

    //Make the main window visible again
    ShowWindow(m_MainWindow, SW_SHOW);
    BringWindowToTop(m_MainWindow);
    PostMessage(m_MainWindow, WM_MAKE_FOCUS, 0, 0);
}

bool CRomBrowser::RomDirNeedsRefresh(void)
{
    bool InWatchThread = (m_WatchThreadID == GetCurrentThreadId());

    //Get Old MD5 of file names
    stdstr FileName = g_Settings->LoadStringVal(RomList_RomListCache);
    if (!CPath(FileName).Exists())
    {
        //if file does not exist then refresh the data
        return true;
    }

    CFile hFile(FileName.c_str(), CFileBase::modeRead);
    if (!hFile.IsOpen())
    {
        //Could not validate, assume it is fine
        return false;
    }

    unsigned char CurrentFileMD5[16];
    hFile.Read(&CurrentFileMD5, sizeof(CurrentFileMD5));
    hFile.Close();

    //Get Current MD5 of file names
    strlist FileNames;
    if (!GetRomFileNames(FileNames, CPath(g_Settings->LoadStringVal(RomList_GameDir)), stdstr(""), InWatchThread))
    {
        return false;
    }
    FileNames.sort();

    MD5 NewMd5 = RomListHash(FileNames);
    if (memcmp(NewMd5.raw_digest(), CurrentFileMD5, sizeof(CurrentFileMD5)) != 0)
    {
        return true;
    }
    return false;
}

void CRomBrowser::WatchRomDirChanged(CRomBrowser * _this)
{
    try
    {
        WriteTrace(TraceUserInterface, TraceDebug, "1");
        _this->m_WatchRomDir = g_Settings->LoadStringVal(RomList_GameDir);
        WriteTrace(TraceUserInterface, TraceDebug, "2");
        if (_this->RomDirNeedsRefresh())
        {
            WriteTrace(TraceUserInterface, TraceDebug, "2a");
            _this->RomDirChanged();
        }
        WriteTrace(TraceUserInterface, TraceDebug, "3");
        HANDLE hChange[] =
        {
            _this->m_WatchStopEvent,
            FindFirstChangeNotification(_this->m_WatchRomDir.c_str(), g_Settings->LoadBool(RomList_GameDirRecursive), FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE),
        };
        WriteTrace(TraceUserInterface, TraceDebug, "4");
        for (;;)
        {
            WriteTrace(TraceUserInterface, TraceDebug, "5");
            if (WaitForMultipleObjects(sizeof(hChange) / sizeof(hChange[0]), hChange, false, INFINITE) == WAIT_OBJECT_0)
            {
                WriteTrace(TraceUserInterface, TraceDebug, "5a");
                FindCloseChangeNotification(hChange[1]);
                return;
            }
            WriteTrace(TraceUserInterface, TraceDebug, "5b");
            if (_this->RomDirNeedsRefresh())
            {
                _this->RomDirChanged();
            }
            WriteTrace(TraceUserInterface, TraceDebug, "5c");
            if (!FindNextChangeNotification(hChange[1]))
            {
                FindCloseChangeNotification(hChange[1]);
                return;
            }
            WriteTrace(TraceUserInterface, TraceDebug, "5d");
        }
    }
    catch (...)
    {
        WriteTrace(TraceUserInterface, TraceError, __FUNCTION__ ":  Unhandled Exception");
    }
}

void CRomBrowser::WatchThreadStart(void)
{
    if (m_WatchThread != NULL)
    {
        // thread already running
        return;
    }
    WriteTrace(TraceUserInterface, TraceDebug, "1");
    WatchThreadStop();
    WriteTrace(TraceUserInterface, TraceDebug, "2");
    if (m_WatchStopEvent == NULL)
    {
        m_WatchStopEvent = CreateEvent(NULL, true, false, NULL);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "3");
    m_WatchThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WatchRomDirChanged, this, 0, &m_WatchThreadID);
    WriteTrace(TraceUserInterface, TraceDebug, "4");
}

void CRomBrowser::WatchThreadStop(void)
{
    if (m_WatchThread == NULL)
    {
        return;
    }
    WriteTrace(TraceUserInterface, TraceDebug, "1");
    SetEvent(m_WatchStopEvent);
    DWORD ExitCode = 0;
    for (int32_t count = 0; count < 20; count++)
    {
        WriteTrace(TraceUserInterface, TraceDebug, "2");
        GetExitCodeThread(m_WatchThread, &ExitCode);
        if (ExitCode != STILL_ACTIVE)
        {
            break;
        }
        Sleep(200);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "3");
    if (ExitCode == STILL_ACTIVE)
    {
        WriteTrace(TraceUserInterface, TraceDebug, "3a");
        TerminateThread(m_WatchThread, 0);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "4");

    CloseHandle(m_WatchThread);
    CloseHandle(m_WatchStopEvent);
    m_WatchStopEvent = NULL;
    m_WatchThread = NULL;
    m_WatchThreadID = 0;
    WriteTrace(TraceUserInterface, TraceDebug, "5");
}

bool CRomBrowser::GetRomFileNames(strlist & FileList, const CPath & BaseDirectory, const std::string & Directory, bool InWatchThread)
{
    if (!BaseDirectory.DirectoryExists())
    {
        return false;
    }
    CPath SearchPath(BaseDirectory, "*.*");
    SearchPath.AppendDirectory(Directory.c_str());

    if (!SearchPath.FindFirst(CPath::FIND_ATTRIBUTE_ALLFILES))
    {
        return false;
    }

    do
    {
        if (InWatchThread && WaitForSingleObject(m_WatchStopEvent, 0) != WAIT_TIMEOUT)
        {
            return false;
        }

        if (SearchPath.IsDirectory())
        {
            if (g_Settings->LoadBool(RomList_GameDirRecursive))
            {
                CPath CurrentDir(Directory);
                CurrentDir.AppendDirectory(SearchPath.GetLastDirectory().c_str());
                GetRomFileNames(FileList, BaseDirectory, CurrentDir, InWatchThread);
            }
        }
        else
        {
            AddFileNameToList(FileList, Directory, SearchPath);
        }
    } while (SearchPath.FindNext());
    return true;
}
