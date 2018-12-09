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
#pragma once

#include <vector>
#include <Project64/Settings/UISettings.h>
#include <Project64-core/RomList/RomList.h>

class CMainGui;
class CPlugins;

class ROMBROWSER_FIELDS
{
    stdstr m_Name;
    size_t m_Pos, m_DefaultPos;
    int    m_ID;
    uint32_t  m_ColWidth;
    LanguageStringID  m_LangID;
    bool   m_PosChanged;

public:
    ROMBROWSER_FIELDS(const char * Name, int Pos, int ID, int ColWidth, LanguageStringID LangID, bool UseDefault) :
        m_Name(Name),
        m_Pos(Pos),
        m_DefaultPos(Pos),
        m_ID(ID),
        m_ColWidth(ColWidth),
        m_LangID(LangID),
        m_PosChanged(false)

    {
        if (!UseDefault)
        {
            m_PosChanged = UISettingsLoadDwordIndex(RomBrowser_PosIndex, m_ID, (uint32_t &)m_Pos);
            UISettingsLoadDwordIndex(RomBrowser_WidthIndex, m_ID, m_ColWidth);
        }
    }
    inline LPCSTR Name(void) const { return m_Name.c_str(); }
    inline size_t Pos(void) const { return m_Pos; }
    inline bool   PosChanged(void) const { return m_PosChanged; }
    inline int    ID(void) const { return m_ID; }
    inline int    ColWidth(void) const { return m_ColWidth; }
    inline LanguageStringID  LangID(void) const { return m_LangID; }

    void SetColWidth(int ColWidth)
    {
        m_ColWidth = ColWidth;
        UISettingsSaveDwordIndex(RomBrowser_WidthIndex, m_ID, m_ColWidth);
    }
    void SetColPos(int Pos)
    {
        m_Pos = Pos;
        UISettingsSaveDwordIndex(RomBrowser_PosIndex, m_ID, m_Pos);
        m_PosChanged = true;
    }
    void ResetPos(void)
    {
        m_Pos = m_DefaultPos;
        UISettingsDeleteSettingIndex(RomBrowser_PosIndex, m_ID);
        m_PosChanged = false;
    }
};

typedef std::vector<ROMBROWSER_FIELDS>   ROMBROWSER_FIELDS_LIST;
typedef std::vector<int>                 FIELD_TYPE_LIST;

class CRomBrowser;
struct SORT_FIELD
{
    CRomBrowser * _this;
    int           Key;
    bool          KeyAscend;
};

class C7zip;
class CRomBrowser :
    public CRomList
{
public:
    CRomBrowser(HWND & hMainWindow, HWND & StatusWindow);
    ~CRomBrowser(void);
    void  HighLightLastRom(void);
    void  HideRomList(void);
    void  ResetRomBrowserColomuns(void);
    void  ResizeRomList(WORD nWidth, WORD nHeight);
    void  RomBrowserToTop(void);
    void  RomBrowserMaximize(bool Mazimize);
    bool  RomBrowserVisible(void);
    bool  RomListDrawItem(int idCtrl, uint32_t lParam);
    bool  RomListNotify(int idCtrl, uint32_t pnmh);
    void  SaveRomListColoumnInfo(void);
    void  SelectRomDir(void);
    void  ShowRomList(void);
    bool  ShowingRomBrowser(void) { return m_ShowingRomBrowser; }
    const char * CurrentedSelectedRom(void) { return m_SelectedRom.c_str(); }

    static void GetFieldInfo(ROMBROWSER_FIELDS_LIST & Fields, bool UseDefault = false);

private:
    enum { IDC_ROMLIST = 223 };
    enum
    {
        RB_FileName = 0, RB_InternalName = 1, RB_GoodName = 2,
        RB_Name = 3, RB_Status = 4, RB_RomSize = 5,
        RB_CoreNotes = 6, RB_PluginNotes = 7, RB_UserNotes = 8,
        RB_CartridgeID = 9, RB_Manufacturer = 10, RB_Country = 11,
        RB_Developer = 12, RB_Crc1 = 13, RB_Crc2 = 14,
        RB_CICChip = 15, RB_ReleaseDate = 16, RB_Genre = 17,
        RB_Players = 18, RB_ForceFeedback = 19, RB_FileFormat = 20
    };

    enum
    {
        NoOfSortKeys = 3
    };

    typedef std::map<int32_t, HBRUSH> HBRUSH_MAP;

    void  RomListReset(void);
    void  RomListLoaded(void);
    void  RomAddedToList(int32_t ListPos);
    int   CalcSortPosition(uint32_t lParam);
    void  CreateRomListControl(void);
    void  DeallocateBrushs(void);
    void  FixRomListWindow(void);
    void  MenuSetText(HMENU hMenu, int32_t MenuPos, const wchar_t * Title, char * ShortCut);
    void  RomList_ColoumnSortList(uint32_t pnmh);
    void  RomList_GetDispInfo(uint32_t pnmh);
    void  RomList_OpenRom(uint32_t pnmh);
    void  RomList_PopupMenu(uint32_t pnmh);
    void  RomList_SortList(void);

    bool RomDirNeedsRefresh(void); // Called from watch thread
    void RomDirChanged(void);
    bool GetRomFileNames(strlist & FileList, const CPath & BaseDirectory, const std::string & Directory, bool InWatchThread);
    void WatchThreadStart(void);
    void WatchThreadStop(void);

    static void WatchRomDirChanged(CRomBrowser * _this);
    static void AddField(ROMBROWSER_FIELDS_LIST & Fields, const char * Name, int32_t Pos, int32_t ID, int32_t Width, LanguageStringID LangID, bool UseDefault);

    //Callback
    static int CALLBACK SelectRomDirCallBack(HWND hwnd, uint32_t uMsg, uint32_t lp, uint32_t lpData);
    static int CALLBACK RomList_CompareItems(uint32_t lParam1, uint32_t lParam2, uint32_t lParamSort);
    
    //Watch Directory Changed function
    HANDLE m_WatchThread, m_WatchStopEvent;
    DWORD  m_WatchThreadID;

    HWND & m_MainWindow;
    HWND & m_StatusWindow;
    HWND m_hRomList;
    ROMBROWSER_FIELDS_LIST m_Fields;
    FIELD_TYPE_LIST m_FieldType;
    std::string m_SelectedRom;
    bool m_Visible;
    bool m_ShowingRomBrowser;
    bool m_AllowSelectionLastRom;
    static std::string m_UnknownGoodName;
    HBRUSH_MAP m_Brushes;
    std::string m_LastRom;
    stdstr m_WatchRomDir;
};
