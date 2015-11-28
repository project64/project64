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
            m_PosChanged = g_Settings->LoadDwordIndex(RomBrowser_PosIndex, m_ID, (uint32_t &)m_Pos);
            g_Settings->LoadDwordIndex(RomBrowser_WidthIndex, m_ID, m_ColWidth);
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
        g_Settings->SaveDwordIndex(RomBrowser_WidthIndex, m_ID, m_ColWidth);
    }
    void SetColPos(int Pos)
    {
        m_Pos = Pos;
        g_Settings->SaveDwordIndex(RomBrowser_PosIndex, m_ID, m_Pos);
        m_PosChanged = true;
    }
    void ResetPos(void)
    {
        m_Pos = m_DefaultPos;
        g_Settings->DeleteSettingIndex(RomBrowser_PosIndex, m_ID);
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
class CRomBrowser
{
    enum { IDC_ROMLIST = 223 };
    enum
    {
        RB_FileName = 0, RB_InternalName = 1, RB_GoodName = 2,
        RB_Status = 3, RB_RomSize = 4, RB_CoreNotes = 5,
        RB_PluginNotes = 6, RB_UserNotes = 7, RB_CartridgeID = 8,
        RB_Manufacturer = 9, RB_Country = 10, RB_Developer = 11,
        RB_Crc1 = 12, RB_Crc2 = 13, RB_CICChip = 14,
        RB_ReleaseDate = 15, RB_Genre = 16, RB_Players = 17,
        RB_ForceFeedback = 18, RB_FileFormat = 19
    };

    enum FILE_FORMAT
    {
        Format_Uncompressed,
        Format_Zip,
        Format_7zip,
    };

    enum
    {
        NoOfSortKeys = 3
    };

    struct ROM_INFO
    {
        wchar_t     szFullFileName[300];
        FILE_FORMAT FileFormat;
        wchar_t     Status[60];
        wchar_t     FileName[200];
        wchar_t     InternalName[22];
        wchar_t     GoodName[200];
        wchar_t     CartID[3];
        wchar_t     PluginNotes[250];
        wchar_t     CoreNotes[250];
        wchar_t     UserNotes[250];
        wchar_t     Developer[30];
        wchar_t     ReleaseDate[30];
        wchar_t     Genre[15];
        int32_t	    Players;
        uint32_t    TextColor;
        int32_t     SelColor;
        uint32_t    SelTextColor;
        uint32_t    SelColorBrush;
        int32_t     RomSize;
        uint8_t     Manufacturer;
        uint8_t     Country;
        uint32_t    CRC1;
        uint32_t    CRC2;
        int32_t     CicChip;
        wchar_t     ForceFeedback[15];
    };

    typedef std::vector<ROM_INFO>   ROMINFO_LIST;

    HWND           & m_MainWindow;
    HWND           & m_StatusWindow;
    HWND             m_hRomList;
    ROMBROWSER_FIELDS_LIST m_Fields;
    FIELD_TYPE_LIST        m_FieldType;
    ROMINFO_LIST           m_RomInfo;
	std::wstring           m_SelectedRom;
    bool                   m_Visible;
    bool                   m_ShowingRomBrowser;
    HANDLE                 m_RefreshThread;
    bool                   m_StopRefresh;
    CIniFile             * m_RomIniFile;
    CIniFile             * m_NotesIniFile;
    CIniFile             * m_ExtIniFile;
    CIniFile             * m_ZipIniFile;
    bool                   m_AllowSelectionLastRom;

    void  AddFileNameToList(strlist & FileList, const stdstr & Directory, CPath & File);
    void  AddRomToList(const wchar_t * RomLocation, const wchar_t * lpLastRom);
    void  AddRomInfoToList(ROM_INFO &RomInfo, const wchar_t * lpLastRom);
    void  AllocateBrushs(void);
    static void  ByteSwapRomData(uint8_t * Data, int DataLen);
    int   CalcSortPosition(uint32_t lParam);
    void  CreateRomListControl(void);
    void  DeallocateBrushs(void);
    void  FillRomExtensionInfo(ROM_INFO * pRomInfo);
    bool  FillRomInfo(ROM_INFO * pRomInfo);
    void  FillRomList(strlist & FileList, const CPath & BaseDirectory, const stdstr & Directory, const char * lpLastRom);
    void  FixRomListWindow(void);
    static int32_t GetCicChipID(uint8_t * RomData);
    bool  LoadDataFromRomFile(const wchar_t * FileName, uint8_t * Data, int32_t DataLen, int32_t * RomSize, FILE_FORMAT & FileFormat);
    void  LoadRomList(void);
    void  MenuSetText(HMENU hMenu, int32_t MenuPos, const wchar_t * Title, char * ShortCut);
    void  SaveRomList(strlist & FileList);
    void  RomList_ColoumnSortList(uint32_t pnmh);
    void  RomList_GetDispInfo(uint32_t pnmh);
    void  RomList_OpenRom(uint32_t pnmh);
    void  RomList_PopupMenu(uint32_t pnmh);
    void  RomList_SortList(void);
    bool  GetRomFileNames(strlist & FileList, const CPath & BaseDirectory, const stdstr & Directory, bool InWatchThread);
    MD5   RomListHash(strlist & FileList);

    static void  __stdcall NotificationCB(LPCWSTR Status, CRomBrowser * _this);

    //Watch Directory Changed function
    HANDLE m_WatchThread, m_WatchStopEvent;
    DWORD  m_WatchThreadID;
    stdstr m_WatchRomDir;
    void WatchThreadStart(void);
    void WatchThreadStop(void);
    bool RomDirNeedsRefresh(void); // Called from watch thread
    static void WatchRomDirChanged(CRomBrowser * _this);
    static void RefreshRomBrowserStatic(CRomBrowser * _this);
    static void AddField(ROMBROWSER_FIELDS_LIST & Fields, const char * Name, int32_t Pos, int32_t ID, int32_t Width, LanguageStringID LangID, bool UseDefault);

    //Callback
    static int CALLBACK SelectRomDirCallBack(HWND hwnd, uint32_t uMsg, uint32_t lp, uint32_t lpData);
    static int CALLBACK RomList_CompareItems(uint32_t lParam1, uint32_t lParam2, uint32_t lParamSort);

public:
    CRomBrowser(HWND & hMainWindow, HWND & StatusWindow);
    ~CRomBrowser(void);
    void  HighLightLastRom(void);
    void  HideRomList(void);
    void  RefreshRomBrowser(void);
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
    const wchar_t * CurrentedSelectedRom(void) { return m_SelectedRom.c_str(); }

    static void GetFieldInfo(ROMBROWSER_FIELDS_LIST & Fields, bool UseDefault = false);
};
