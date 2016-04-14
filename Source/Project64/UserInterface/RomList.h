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

class CRomList
{
public:
    enum FILE_FORMAT
    {
        Format_Uncompressed,
        Format_Zip,
        Format_7zip,
    };

    struct ROM_INFO
    {
        char        szFullFileName[300];
        FILE_FORMAT FileFormat;
        wchar_t     Status[60];
        char        FileName[200];
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
        int32_t     RomSize;
        uint8_t     Manufacturer;
        uint8_t     Country;
        uint32_t    CRC1;
        uint32_t    CRC2;
        int32_t     CicChip;
        wchar_t     ForceFeedback[15];
    };

    CRomList();
    virtual ~CRomList();

    void  RefreshRomList(void);
    void LoadRomList(void);

protected:
    typedef std::vector<ROM_INFO> ROMINFO_LIST;
    virtual void RomListReset(void) {}
    virtual void RomAddedToList(int32_t /*ListPos*/) {}
    virtual void RomListLoaded(void) {}
    virtual void RomDirChanged(void) {}

    void WatchThreadStart(void);
    void WatchThreadStop(void);

    ROMINFO_LIST m_RomInfo;
    bool m_StopRefresh;

private:
    void AddRomToList(const char * RomLocation);
    void FillRomList(strlist & FileList, const CPath & BaseDirectory, const char * Directory, const char * lpLastRom);
    bool FillRomInfo(ROM_INFO * pRomInfo);
    void FillRomExtensionInfo(ROM_INFO * pRomInfo);
    bool LoadDataFromRomFile(const char * FileName, uint8_t * Data, int32_t DataLen, int32_t * RomSize, FILE_FORMAT & FileFormat);
    void  AddFileNameToList(strlist & FileList, const stdstr & Directory, CPath & File);
    void  SaveRomList(strlist & FileList);
    void  RefreshRomListThread(void);
    bool  GetRomFileNames(strlist & FileList, const CPath & BaseDirectory, const std::string & Directory, bool InWatchThread);
    MD5   RomListHash(strlist & FileList);

    //Watch Directory Changed function
    HANDLE m_WatchThread, m_WatchStopEvent;
    DWORD  m_WatchThreadID;
    bool RomDirNeedsRefresh(void); // Called from watch thread

    static void NotificationCB(const char * Status, CRomList * _this);
    static void WatchRomDirChanged(CRomList * _this);
    static void RefreshRomListStatic(CRomList * _this);
    static void  ByteSwapRomData(uint8_t * Data, int DataLen);
    static int32_t GetCicChipID(uint8_t * RomData);

    CIniFile * m_NotesIniFile;
    CIniFile * m_ExtIniFile;
    CIniFile * m_RomIniFile;
    CIniFile * m_ZipIniFile;
    HANDLE m_RefreshThread;
    stdstr m_WatchRomDir;

};
