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

static const char* ROM_extensions[] =
{
    "zip", "7z", "v64", "z64", "n64", "rom", "jap", "pal", "usa", "eur", "bin",
};

CRomList::CRomList() :
    m_RefreshThread(NULL),
    m_NotesIniFile(NULL),
    m_ExtIniFile(NULL),
    m_ZipIniFile(NULL),
    m_RomIniFile(NULL),
    m_WatchThreadID(0),
    m_WatchThread(NULL),
    m_WatchStopEvent(NULL)
{
    if (g_Settings)
    {
        m_NotesIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_Notes).c_str());
        m_ExtIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_ExtInfo).c_str());
        m_RomIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_RomDatabase).c_str());
        m_ZipIniFile = new CIniFile(g_Settings->LoadStringVal(RomList_7zipCache).c_str());
    }
}

CRomList::~CRomList()
{
    WatchThreadStop();
    m_StopRefresh = true;
    if (m_NotesIniFile)
    {
        delete m_NotesIniFile;
        m_NotesIniFile = NULL;
    }
    if (m_ExtIniFile)
    {
        delete m_ExtIniFile;
        m_ExtIniFile = NULL;
    }
    if (m_RomIniFile)
    {
        delete m_RomIniFile;
        m_RomIniFile = NULL;
    }
    if (m_ZipIniFile)
    {
        delete m_ZipIniFile;
        m_ZipIniFile = NULL;
    }
}

void CRomList::RefreshRomBrowser(void)
{
    DWORD ThreadID;

    if (m_RefreshThread)
    {
        return;
    }
    WriteTrace(TraceUserInterface, TraceDebug, "1");
    m_StopRefresh = false;
    m_RefreshThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RefreshRomBrowserStatic, (LPVOID)this, 0, &ThreadID);
    WriteTrace(TraceUserInterface, TraceDebug, "2");
}

void CRomList::RefreshRomListThread(void)
{
    //delete cache
    CPath(g_Settings->LoadStringVal(RomList_RomListCache)).Delete();

    //clear all current items
    RomListReset();
    m_RomInfo.clear();

    if (m_WatchRomDir != g_Settings->LoadStringVal(RomList_GameDir))
    {
        WriteTrace(TraceUserInterface, TraceDebug, "4");
        WatchThreadStop();
        WriteTrace(TraceUserInterface, TraceDebug, "5");
        WatchThreadStart();
        WriteTrace(TraceUserInterface, TraceDebug, "6");
    }

    WriteTrace(TraceUserInterface, TraceDebug, "7");
    stdstr RomDir = g_Settings->LoadStringVal(RomList_GameDir);
    stdstr LastRom = UISettingsLoadStringIndex(File_RecentGameFileIndex, 0);
    WriteTrace(TraceUserInterface, TraceDebug, "8");

    strlist FileNames;
    FillRomList(FileNames, CPath(RomDir), "", LastRom.c_str());
    WriteTrace(TraceUserInterface, TraceDebug, "9");
    SaveRomList(FileNames);
    WriteTrace(TraceUserInterface, TraceDebug, "10");
    CloseHandle(m_RefreshThread);
    m_RefreshThread = NULL;
    WriteTrace(TraceUserInterface, TraceDebug, "11");
}

void CRomList::AddRomToList(const char * RomLocation)
{
    ROM_INFO RomInfo;

    memset(&RomInfo, 0, sizeof(ROM_INFO));
    strncpy(RomInfo.szFullFileName, RomLocation, (sizeof(RomInfo.szFullFileName) / sizeof(RomInfo.szFullFileName[0])) - 1);
    if (FillRomInfo(&RomInfo))
    {
        int32_t ListPos = m_RomInfo.size();
        m_RomInfo.push_back(RomInfo);
        RomAddedToList(ListPos);
    }
}

void CRomList::FillRomList(strlist & FileList, const CPath & BaseDirectory, const char * Directory, const char * lpLastRom)
{
    CPath SearchPath(BaseDirectory, "*");
    SearchPath.AppendDirectory(Directory);

    WriteTrace(TraceUserInterface, TraceDebug, "1 %s", (const char *)SearchPath);
    if (!SearchPath.FindFirst(CPath::FIND_ATTRIBUTE_ALLFILES))
    {
        return;
    }

    do
    {
        uint8_t ext_ID;
        int8_t new_list_entry = 0;
        const uint8_t exts = sizeof(ROM_extensions) / sizeof(ROM_extensions[0]);

        WriteTrace(TraceUserInterface, TraceDebug, ": 2 %s m_StopRefresh = %d", (const char *)SearchPath, m_StopRefresh);
        if (m_StopRefresh) { break; }

        if (SearchPath.IsDirectory())
        {
            if (g_Settings->LoadBool(RomList_GameDirRecursive))
            {
                CPath CurrentDir(Directory);
                CurrentDir.AppendDirectory(SearchPath.GetLastDirectory().c_str());
                FillRomList(FileList, BaseDirectory, CurrentDir, lpLastRom);
            }
            continue;
        }

        AddFileNameToList(FileList, Directory, SearchPath);

        stdstr Extension = stdstr(SearchPath.GetExtension()).ToLower();

        for (ext_ID = 0; ext_ID < exts; ext_ID++)
        {
            if (Extension == ROM_extensions[ext_ID] && Extension != "7z")
            {
                new_list_entry = 1;
                break;
            }
        }
        if (new_list_entry)
        {
            AddRomToList(SearchPath);
            continue;
        }

        if (Extension == "7z")
        {
            try
            {
                C7zip ZipFile(SearchPath);
                if (!ZipFile.OpenSuccess())
                {
                    continue;
                }
                char ZipFileName[260];
                stdstr_f SectionName("%s-%d", ZipFile.FileName(ZipFileName, sizeof(ZipFileName)), ZipFile.FileSize());
                SectionName.ToLower();

                WriteTrace(TraceUserInterface, TraceDebug, "4 %s", SectionName.c_str());
                for (int32_t i = 0; i < ZipFile.NumFiles(); i++)
                {
                    CSzFileItem * f = ZipFile.FileItem(i);
                    if (f->IsDir)
                    {
                        continue;
                    }
                    ROM_INFO RomInfo;

                    std::wstring FileNameW = ZipFile.FileNameIndex(i);
                    if (FileNameW.length() == 0)
                    {
                        continue;
                    }

                    stdstr FileName;
                    FileName.FromUTF16(FileNameW.c_str());
                    WriteTrace(TraceUserInterface, TraceDebug, "5");
                    char drive2[_MAX_DRIVE], dir2[_MAX_DIR], FileName2[MAX_PATH], ext2[_MAX_EXT];
                    _splitpath(FileName.c_str(), drive2, dir2, FileName2, ext2);

                    WriteTrace(TraceUserInterface, TraceDebug, ": 6 %s", ext2);
                    if (_stricmp(ext2, ".bin") == 0)
                    {
                        continue;
                    }
                    WriteTrace(TraceUserInterface, TraceDebug, "7");
                    memset(&RomInfo, 0, sizeof(ROM_INFO));
                    stdstr_f zipFileName("%s?%s", (LPCSTR)SearchPath, FileName.c_str());
                    ZipFile.SetNotificationCallback((C7zip::LP7ZNOTIFICATION)NotificationCB, this);

                    strncpy(RomInfo.szFullFileName, zipFileName.c_str(), sizeof(RomInfo.szFullFileName) - 1);
                    RomInfo.szFullFileName[sizeof(RomInfo.szFullFileName) - 1] = 0;
                    strcpy(RomInfo.FileName, strstr(RomInfo.szFullFileName, "?") + 1);
                    RomInfo.FileFormat = Format_7zip;

                    WriteTrace(TraceUserInterface, TraceDebug, "8");
                    char szHeader[0x90];
                    if (m_ZipIniFile->GetString(SectionName.c_str(), FileName.c_str(), "", szHeader, sizeof(szHeader)) == 0)
                    {
                        uint8_t RomData[0x1000];
                        if (!ZipFile.GetFile(i, RomData, sizeof(RomData)))
                        {
                            continue;
                        }
                        WriteTrace(TraceUserInterface, TraceDebug, "9");
                        if (!CN64Rom::IsValidRomImage(RomData)) { continue; }
                        WriteTrace(TraceUserInterface, TraceDebug, "10");
                        ByteSwapRomData(RomData, sizeof(RomData));
                        WriteTrace(TraceUserInterface, TraceDebug, "11");

                        stdstr RomHeader;
                        for (int32_t x = 0; x < 0x40; x += 4)
                        {
                            RomHeader += stdstr_f("%08X", *((uint32_t *)&RomData[x]));
                        }
                        WriteTrace(TraceUserInterface, TraceDebug, "11a %s", RomHeader.c_str());
                        int32_t CicChip = GetCicChipID(RomData);

                        //save this info
                        WriteTrace(TraceUserInterface, TraceDebug, "12");
                        m_ZipIniFile->SaveString(SectionName.c_str(), FileName.c_str(), RomHeader.c_str());
                        m_ZipIniFile->SaveNumber(SectionName.c_str(), stdstr_f("%s-Cic", FileName.c_str()).c_str(), CicChip);
                        strcpy(szHeader, RomHeader.c_str());
                    }
                    WriteTrace(TraceUserInterface, TraceDebug, "13");
                    uint8_t RomData[0x40];

                    for (int32_t x = 0; x < 0x40; x += 4)
                    {
                        const size_t delimit_offset = sizeof("FFFFFFFF") - 1;
                        const char backup_character = szHeader[2 * x + delimit_offset];

                        szHeader[2 * x + delimit_offset] = '\0';
                        *(uint32_t *)&RomData[x] = strtoul(&szHeader[2 * x], NULL, 16);
                        szHeader[2 * x + delimit_offset] = backup_character;
                    }
                    WriteTrace(TraceUserInterface, TraceDebug, "14");
                    {
                        char InternalName[22];
                        memcpy(InternalName, (void *)(RomData + 0x20), 20);
                        for (int32_t count = 0; count < 20; count += 4)
                        {
                            InternalName[count] ^= InternalName[count + 3];
                            InternalName[count + 3] ^= InternalName[count];
                            InternalName[count] ^= InternalName[count + 3];
                            InternalName[count + 1] ^= InternalName[count + 2];
                            InternalName[count + 2] ^= InternalName[count + 1];
                            InternalName[count + 1] ^= InternalName[count + 2];
                        }
                        InternalName[20] = '\0';
                        wcscpy(RomInfo.InternalName, stdstr(InternalName).ToUTF16(stdstr::CODEPAGE_932).c_str());
                    }
                    RomInfo.RomSize = (int32_t)f->Size;

                    WriteTrace(TraceUserInterface, TraceDebug, "15");
                    RomInfo.CartID[0] = *(RomData + 0x3F);
                    RomInfo.CartID[1] = *(RomData + 0x3E);
                    RomInfo.CartID[2] = '\0';
                    RomInfo.Manufacturer = *(RomData + 0x38);
                    RomInfo.Country = *(RomData + 0x3D);
                    RomInfo.CRC1 = *(uint32_t *)(RomData + 0x10);
                    RomInfo.CRC2 = *(uint32_t *)(RomData + 0x14);
                    m_ZipIniFile->GetNumber(SectionName.c_str(), stdstr_f("%s-Cic", FileName.c_str()).c_str(), (ULONG)-1, (uint32_t &)RomInfo.CicChip);
                    WriteTrace(TraceUserInterface, TraceDebug, "16");
                    FillRomExtensionInfo(&RomInfo);

                    if (RomInfo.SelColor == -1)
                    {
                        RomInfo.SelColorBrush = (uint32_t)((HBRUSH)(COLOR_HIGHLIGHT + 1));
                    }
                    else
                    {
                        RomInfo.SelColorBrush = (uint32_t)CreateSolidBrush(RomInfo.SelColor);
                    }
                    WriteTrace(TraceUserInterface, TraceDebug, "17");
                    int32_t ListPos = m_RomInfo.size();
                    m_RomInfo.push_back(RomInfo);
                    RomAddedToList(ListPos);
                }
            }
            catch (...)
            {
                WriteTrace(TraceUserInterface, TraceError, "execpetion processing %s", (LPCSTR)SearchPath);
            }
            continue;
        }
    } while (SearchPath.FindNext());
    m_ZipIniFile->FlushChanges();
}

void CRomList::NotificationCB(const char * Status, CRomList * /*_this*/)
{
    g_Notify->DisplayMessage(5, Status);
}

void CRomList::RefreshRomBrowserStatic(CRomList * _this)
{
    _this->RefreshRomListThread();
}

bool CRomList::LoadDataFromRomFile(const char * FileName, uint8_t * Data, int32_t DataLen, int32_t * RomSize, FILE_FORMAT & FileFormat)
{
    uint8_t Test[4];

    if (_strnicmp(&FileName[strlen(FileName) - 4], ".ZIP", 4) == 0)
    {
        int32_t len, port = 0, FoundRom;
        unz_file_info info;
        char zname[132];
        unzFile file;
        file = unzOpen(FileName);
        if (file == NULL) { return false; }

        port = unzGoToFirstFile(file);
        FoundRom = false;
        while (port == UNZ_OK && FoundRom == false)
        {
            unzGetCurrentFileInfo(file, &info, zname, 128, NULL, 0, NULL, 0);
            if (unzLocateFile(file, zname, 1) != UNZ_OK)
            {
                unzClose(file);
                return true;
            }
            if (unzOpenCurrentFile(file) != UNZ_OK)
            {
                unzClose(file);
                return true;
            }
            unzReadCurrentFile(file, Test, 4);
            if (CN64Rom::IsValidRomImage(Test))
            {
                FoundRom = true;
                memcpy(Data, Test, 4);
                len = unzReadCurrentFile(file, &Data[4], DataLen - 4) + 4;

                if ((int32_t)DataLen != len)
                {
                    unzCloseCurrentFile(file);
                    unzClose(file);
                    return false;
                }
                *RomSize = info.uncompressed_size;
                if (unzCloseCurrentFile(file) == UNZ_CRCERROR)
                {
                    unzClose(file);
                    return false;
                }
                unzClose(file);
            }
            if (FoundRom == false)
            {
                unzCloseCurrentFile(file);
                port = unzGoToNextFile(file);
            }
        }
        if (FoundRom == false)
        {
            return false;
        }
        FileFormat = Format_Zip;
    }
    else
    {
        HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
        if (hFile == INVALID_HANDLE_VALUE) { return false; }
        SetFilePointer(hFile, 0, 0, FILE_BEGIN);

        DWORD dwRead;
        ReadFile(hFile, Test, 4, &dwRead, NULL);
        if (!CN64Rom::IsValidRomImage(Test)) { CloseHandle(hFile); return false; }
        SetFilePointer(hFile, 0, 0, FILE_BEGIN);
        if (!ReadFile(hFile, Data, DataLen, &dwRead, NULL)) { CloseHandle(hFile); return false; }
        *RomSize = GetFileSize(hFile, NULL);
        CloseHandle(hFile);
        FileFormat = Format_Uncompressed;
    }
    ByteSwapRomData(Data, DataLen);
    return true;
}

bool CRomList::FillRomInfo(ROM_INFO * pRomInfo)
{
    int32_t count;
    uint8_t RomData[0x1000];

    if (!LoadDataFromRomFile(pRomInfo->szFullFileName, RomData, sizeof(RomData), &pRomInfo->RomSize, pRomInfo->FileFormat))
    {
        return false;
    }
    else
    {
        if (strstr(pRomInfo->szFullFileName, "?") != NULL)
        {
            strcpy(pRomInfo->FileName, strstr(pRomInfo->szFullFileName, "?") + 1);
        }
        else
        {
            strncpy(pRomInfo->FileName, CPath(pRomInfo->szFullFileName).GetNameExtension().c_str(), sizeof(pRomInfo->FileName) / sizeof(pRomInfo->FileName[0]));
        }
        char InternalName[22];
        memcpy(InternalName, (void *)(RomData + 0x20), 20);
        for (count = 0; count < 20; count += 4)
        {
            InternalName[count] ^= InternalName[count + 3];
            InternalName[count + 3] ^= InternalName[count];
            InternalName[count] ^= InternalName[count + 3];
            InternalName[count + 1] ^= InternalName[count + 2];
            InternalName[count + 2] ^= InternalName[count + 1];
            InternalName[count + 1] ^= InternalName[count + 2];
        }
        InternalName[20] = '\0';
        wcscpy(pRomInfo->InternalName, stdstr(InternalName).ToUTF16(stdstr::CODEPAGE_932).c_str());
        pRomInfo->CartID[0] = *(RomData + 0x3F);
        pRomInfo->CartID[1] = *(RomData + 0x3E);
        pRomInfo->CartID[2] = '\0';
        pRomInfo->Manufacturer = *(RomData + 0x38);
        pRomInfo->Country = *(RomData + 0x3D);
        pRomInfo->CRC1 = *(uint32_t *)(RomData + 0x10);
        pRomInfo->CRC2 = *(uint32_t *)(RomData + 0x14);
        pRomInfo->CicChip = GetCicChipID(RomData);

        FillRomExtensionInfo(pRomInfo);

        if (pRomInfo->SelColor == -1)
        {
            pRomInfo->SelColorBrush = (uint32_t)((HBRUSH)(COLOR_HIGHLIGHT + 1));
        }
        else
        {
            pRomInfo->SelColorBrush = (uint32_t)CreateSolidBrush(pRomInfo->SelColor);
        }

        return true;
    }
}

void CRomList::FillRomExtensionInfo(ROM_INFO * pRomInfo)
{
    //Initialize the structure
    pRomInfo->UserNotes[0] = 0;
    pRomInfo->Developer[0] = 0;
    pRomInfo->ReleaseDate[0] = 0;
    pRomInfo->Genre[0] = 0;
    pRomInfo->Players = 1;
    pRomInfo->CoreNotes[0] = 0;
    pRomInfo->PluginNotes[0] = 0;
    wcscpy(pRomInfo->GoodName, L"#340#");
    wcscpy(pRomInfo->Status, L"Unknown");

    //Get File Identifier
    char Identifier[100];
    sprintf(Identifier, "%08X-%08X-C:%X", pRomInfo->CRC1, pRomInfo->CRC2, pRomInfo->Country);

    //Rom Notes
    wcsncpy(pRomInfo->UserNotes, m_NotesIniFile->GetString(Identifier, "Note", "").ToUTF16().c_str(), sizeof(pRomInfo->UserNotes) / sizeof(wchar_t));

    //Rom Extension info
    wcsncpy(pRomInfo->Developer, m_ExtIniFile->GetString(Identifier, "Developer", "").ToUTF16().c_str(), sizeof(pRomInfo->Developer) / sizeof(wchar_t));
    wcsncpy(pRomInfo->ReleaseDate, m_ExtIniFile->GetString(Identifier, "ReleaseDate", "").ToUTF16().c_str(), sizeof(pRomInfo->ReleaseDate) / sizeof(wchar_t));
    wcsncpy(pRomInfo->Genre, m_ExtIniFile->GetString(Identifier, "Genre", "").ToUTF16().c_str(), sizeof(pRomInfo->Genre) / sizeof(wchar_t));
    m_ExtIniFile->GetNumber(Identifier, "Players", 1, (uint32_t &)pRomInfo->Players);
    wcsncpy(pRomInfo->ForceFeedback, m_ExtIniFile->GetString(Identifier, "ForceFeedback", "unknown").ToUTF16().c_str(), sizeof(pRomInfo->ForceFeedback) / sizeof(wchar_t));

    //Rom Settings
    wcsncpy(pRomInfo->GoodName, m_RomIniFile->GetString(Identifier, "Good Name", stdstr().FromUTF16(pRomInfo->GoodName).c_str()).ToUTF16().c_str(), sizeof(pRomInfo->GoodName) / sizeof(wchar_t));
    wcsncpy(pRomInfo->Status, m_RomIniFile->GetString(Identifier, "Status", stdstr().FromUTF16(pRomInfo->Status).c_str()).ToUTF16().c_str(), sizeof(pRomInfo->Status) / sizeof(wchar_t));
    wcsncpy(pRomInfo->CoreNotes, m_RomIniFile->GetString(Identifier, "Core Note", "").ToUTF16().c_str(), sizeof(pRomInfo->CoreNotes) / sizeof(wchar_t));
    wcsncpy(pRomInfo->PluginNotes, m_RomIniFile->GetString(Identifier, "Plugin Note", "").ToUTF16().c_str(), sizeof(pRomInfo->PluginNotes) / sizeof(wchar_t));

    //Get the text color
    stdstr String = m_RomIniFile->GetString("Rom Status", stdstr().FromUTF16(pRomInfo->Status).c_str(), "000000");
    pRomInfo->TextColor = (std::strtoul(String.c_str(), 0, 16) & 0xFFFFFF);
    pRomInfo->TextColor = (pRomInfo->TextColor & 0x00FF00) | ((pRomInfo->TextColor >> 0x10) & 0xFF) | ((pRomInfo->TextColor & 0xFF) << 0x10);

    //Get the selected color
    String.Format("%ws.Sel", pRomInfo->Status);
    String = m_RomIniFile->GetString("Rom Status", String.c_str(), "FFFFFFFF");
    uint32_t selcol = std::strtoul(String.c_str(), NULL, 16);
    if (selcol & 0x80000000)
    {
        pRomInfo->SelColor = -1;
    }
    else
    {
        selcol = (selcol & 0x00FF00) | ((selcol >> 0x10) & 0xFF) | ((selcol & 0xFF) << 0x10);
        pRomInfo->SelColor = selcol;
    }

    //Get the selected text color
    String.Format("%ws.Seltext", pRomInfo->Status);
    String = m_RomIniFile->GetString("Rom Status", String.c_str(), "FFFFFF");
    pRomInfo->SelTextColor = (std::strtoul(String.c_str(), 0, 16) & 0xFFFFFF);
    pRomInfo->SelTextColor = (pRomInfo->SelTextColor & 0x00FF00) | ((pRomInfo->SelTextColor >> 0x10) & 0xFF) | ((pRomInfo->SelTextColor & 0xFF) << 0x10);
}

void CRomList::ByteSwapRomData(uint8_t * Data, int32_t DataLen)
{
    int32_t count;

    switch (*((uint32_t *)&Data[0]))
    {
    case 0x12408037:
        for (count = 0; count < DataLen; count += 4)
        {
            Data[count] ^= Data[count + 2];
            Data[count + 2] ^= Data[count];
            Data[count] ^= Data[count + 2];
            Data[count + 1] ^= Data[count + 3];
            Data[count + 3] ^= Data[count + 1];
            Data[count + 1] ^= Data[count + 3];
        }
        break;
    case 0x40072780: //64DD IPL
    case 0x40123780:
        for (count = 0; count < DataLen; count += 4)
        {
            Data[count] ^= Data[count + 3];
            Data[count + 3] ^= Data[count];
            Data[count] ^= Data[count + 3];
            Data[count + 1] ^= Data[count + 2];
            Data[count + 2] ^= Data[count + 1];
            Data[count + 1] ^= Data[count + 2];
        }
        break;
    case 0x80371240: break;
    }
}

void CRomList::LoadRomList(void)
{
    CPath FileName(g_Settings->LoadStringVal(RomList_RomListCache));
    CFile file(FileName, CFileBase::modeRead | CFileBase::modeNoTruncate);

    if (!file.IsOpen())
    {
        //if file does not exist then refresh the data
        RefreshRomBrowser();
        return;
    }
    unsigned char md5[16];
    if (!file.Read(md5, sizeof(md5)))
    {
        file.Close();
        RefreshRomBrowser();
        return;
    }

    //Read the size of ROM_INFO
    int32_t RomInfoSize = 0;
    if (!file.Read(&RomInfoSize, sizeof(RomInfoSize)) || RomInfoSize != sizeof(ROM_INFO))
    {
        file.Close();
        RefreshRomBrowser();
        return;
    }

    //Read the Number of entries
    int32_t Entries = 0;
    file.Read(&Entries, sizeof(Entries));

    //Read Every Entry
    m_RomInfo.clear();
    RomListReset();
    for (int32_t count = 0; count < Entries; count++)
    {
        ROM_INFO RomInfo;
        file.Read(&RomInfo, RomInfoSize);
        int32_t ListPos = m_RomInfo.size();
        m_RomInfo.push_back(RomInfo);
        RomAddedToList(ListPos);
    }
    RomListLoaded();
}

/*
* 	SaveRomList - save all the rom information about the current roms in the rom brower
*                to a cache file, so it is quick to reload the information
*/
void CRomList::SaveRomList(strlist & FileList)
{
    MD5 ListHash = RomListHash(FileList);

    CPath FileName(g_Settings->LoadStringVal(RomList_RomListCache));
    CFile file(FileName, CFileBase::modeWrite | CFileBase::modeCreate);
    file.Write(ListHash.raw_digest(), 16);

    //Write the size of ROM_INFO
    int32_t RomInfoSize = sizeof(ROM_INFO);
    file.Write(&RomInfoSize, sizeof(RomInfoSize));

    //Write the Number of entries
    int32_t Entries = m_RomInfo.size();
    file.Write(&Entries, sizeof(Entries));

    //Write Every Entry
    for (int32_t count = 0; count < Entries; count++)
    {
        file.Write(&m_RomInfo[count], RomInfoSize);
    }

    //Close the file handle
    file.Close();
}

void CRomList::WatchRomDirChanged(CRomList * _this)
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

bool CRomList::RomDirNeedsRefresh(void)
{
    bool InWatchThread = (m_WatchThreadID == GetCurrentThreadId());

    //Get Old MD5 of file names
    stdstr FileName = g_Settings->LoadStringVal(RomList_RomListCache);
    HANDLE hFile = CreateFile(FileName.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        //if file does not exist then refresh the data
        return true;
    }

    DWORD dwRead;
    unsigned char CurrentFileMD5[16];
    ReadFile(hFile, &CurrentFileMD5, sizeof(CurrentFileMD5), &dwRead, NULL);
    CloseHandle(hFile);

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

void CRomList::WatchThreadStart(void)
{
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

void CRomList::WatchThreadStop(void)
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

MD5 CRomList::RomListHash(strlist & FileList)
{
    stdstr NewFileNames;
    FileList.sort();
    for (strlist::iterator iter = FileList.begin(); iter != FileList.end(); iter++)
    {
        NewFileNames += *iter;
        NewFileNames += ";";
    }
    MD5 md5Hash((const unsigned char *)NewFileNames.c_str(), NewFileNames.length());
    WriteTrace(TraceUserInterface, TraceDebug, "%s - %s", md5Hash.hex_digest(), NewFileNames.c_str());
    return md5Hash;
}

void CRomList::AddFileNameToList(strlist & FileList, const stdstr & Directory, CPath & File)
{
    uint8_t i;

    if (FileList.size() > 3000)
    {
        return;
    }

    stdstr Drive, Dir, Name, Extension;
    File.GetComponents(NULL, &Dir, &Name, &Extension);
    Extension.ToLower();
    for (i = 0; i < sizeof(ROM_extensions) / sizeof(ROM_extensions[0]); i++)
    {
        if (Extension == ROM_extensions[i])
        {
            stdstr FileName = Directory + Name + Extension;
            FileName.ToLower();
            FileList.push_back(FileName);
            break;
        }
    }
}

bool CRomList::GetRomFileNames(strlist & FileList, const CPath & BaseDirectory, const std::string & Directory, bool InWatchThread)
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

int32_t CRomList::GetCicChipID(uint8_t * RomData)
{
    __int64 CRC = 0;
    int32_t count;

    for (count = 0x40; count < 0x1000; count += 4)
    {
        CRC += *(uint32_t *)(RomData + count);
    }
    switch (CRC)
    {
    case 0x000000D0027FDF31: return CIC_NUS_6101;
    case 0x000000CFFB631223: return CIC_NUS_6101;
    case 0x000000D057C85244: return CIC_NUS_6102;
    case 0x000000D6497E414B: return CIC_NUS_6103;
    case 0x0000011A49F60E96: return CIC_NUS_6105;
    case 0x000000D6D5BE5580: return CIC_NUS_6106;
    case 0x000001053BC19870: return CIC_NUS_5167; //64DD CONVERSION CIC
    case 0x000000D2E53EF008: return CIC_NUS_8303; //64DD IPL
    default:
        return CIC_UNKNOWN;
    }
}
