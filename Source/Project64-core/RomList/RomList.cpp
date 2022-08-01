#include "stdafx.h"
#include "RomList.h"
#include <Project64-core/3rdParty/zip.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/N64System/N64Disk.h>

#ifdef _WIN32
#include <Project64-core/3rdParty/7zip.h>
#endif

static const char* ROM_extensions[] =
{
#ifdef _WIN32
    "7z",
#endif
    "zip",
    "v64",
    "z64",
    "n64",
    "rom",
    "jap",
    "pal",
    "usa",
    "eur",
    "bin",
    "ndd",
    "d64",
};

CRomList::CRomList() :
    m_RefreshThread((CThread::CTHREAD_START_ROUTINE)RefreshRomListStatic),
    m_StopRefresh(false),
    m_GameDir(g_Settings->LoadStringVal(RomList_GameDir).c_str()),
    m_NotesIniFile(nullptr),
    m_ExtIniFile(nullptr),
#ifdef _WIN32
    m_ZipIniFile(nullptr),
#endif
    m_RomIniFile(nullptr)
{
    WriteTrace(TraceRomList, TraceVerbose, "Start");
    if (g_Settings)
    {
        m_NotesIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_Notes).c_str());
        m_ExtIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_ExtInfo).c_str());
        m_RomIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_RomDatabase).c_str());
#ifdef _WIN32
        m_ZipIniFile = new CIniFile(g_Settings->LoadStringVal(RomList_7zipCache).c_str());
#endif
        g_Settings->RegisterChangeCB(RomList_GameDir, this, (CSettings::SettingChangedFunc)RefreshSettings);
    }
    if (m_RomIniFile)
    {
        m_RomIniFile->GetVectorOfSections(m_GameIdentifiers);
    }
    WriteTrace(TraceRomList, TraceVerbose, "Done");
}

CRomList::~CRomList()
{
    WriteTrace(TraceRomList, TraceVerbose, "Start");
    m_StopRefresh = true;
    if (m_NotesIniFile)
    {
        delete m_NotesIniFile;
        m_NotesIniFile = nullptr;
    }
    if (m_ExtIniFile)
    {
        delete m_ExtIniFile;
        m_ExtIniFile = nullptr;
    }
    if (m_RomIniFile)
    {
        delete m_RomIniFile;
        m_RomIniFile = nullptr;
    }
#ifdef _WIN32
    if (m_ZipIniFile)
    {
        delete m_ZipIniFile;
        m_ZipIniFile = nullptr;
    }
#endif
    if (g_Settings)
    {
        g_Settings->UnregisterChangeCB(RomList_GameDir, this, (CSettings::SettingChangedFunc)RefreshSettings);
    }
    WriteTrace(TraceRomList, TraceVerbose, "Done");
}

void CRomList::RefreshRomList(void)
{
    if (m_RefreshThread.isRunning())
    {
        WriteTrace(TraceRomList, TraceVerbose, "Already refreshing, ignoring");
        return;
    }
    WriteTrace(TraceRomList, TraceDebug, "Starting thread");
    m_StopRefresh = false;
    m_RefreshThread.Start((void *)this);
    WriteTrace(TraceRomList, TraceVerbose, "Done");
}

void CRomList::RefreshRomListThread(void)
{
    WriteTrace(TraceRomList, TraceVerbose, "Start");
    // Delete cache
    CPath(g_Settings->LoadStringVal(RomList_RomListCache)).Delete();
    WriteTrace(TraceRomList, TraceVerbose, "Cache deleted");

    // Clear all current items
    RomListReset();
    m_RomInfo.clear();

    strlist FileNames;
    FillRomList(FileNames, "");
    RomListLoaded();
    SaveRomList(FileNames);
    WriteTrace(TraceRomList, TraceVerbose, "Done");
}

void CRomList::AddRomToList(const char * RomLocation)
{
    WriteTrace(TraceRomList, TraceVerbose, "Start (RomLocation: \"%s\")", RomLocation);
    ROM_INFO RomInfo = { 0 };

    strncpy(RomInfo.szFullFileName, RomLocation, (sizeof(RomInfo.szFullFileName) / sizeof(RomInfo.szFullFileName[0])) - 1);
    if (FillRomInfo(&RomInfo))
    {
        int32_t ListPos = (int32_t)m_RomInfo.size();
        m_RomInfo.push_back(RomInfo);
        RomAddedToList(ListPos);
    }
    else
    {
        WriteTrace(TraceRomList, TraceVerbose, "Failed to fill ROM information, ignoring");
    }
    WriteTrace(TraceRomList, TraceVerbose, "Done");
}

void CRomList::FillRomList(strlist & FileList, const char * Directory)
{
    WriteTrace(TraceRomList, TraceDebug, "Start (m_GameDir = %s, Directory: %s)", (const char *)m_GameDir, Directory);
    CPath SearchDir((const char *)m_GameDir, "*");
    SearchDir.AppendDirectory(Directory);

    WriteTrace(TraceRomList, TraceVerbose, "SearchDir: %s", (const char *)SearchDir);
    if (!SearchDir.FindFirst(CPath::FIND_ATTRIBUTE_ALLFILES))
    {
        WriteTrace(TraceRomList, TraceVerbose, "No files found");
        WriteTrace(TraceRomList, TraceDebug, "Done (Directory: %s)", Directory);
        return;
    }

    do
    {
        WriteTrace(TraceRomList, TraceVerbose, "Found: \"%s\" m_StopRefresh = %s", (const char *)SearchDir, m_StopRefresh ? "true" : "false");
        if (m_StopRefresh)
        {
            WriteTrace(TraceRomList, TraceVerbose, "Stop refresh set, stopping");
            break;
        }

        if (SearchDir.IsDirectory())
        {
            if (g_Settings->LoadBool(RomList_GameDirRecursive))
            {
                CPath CurrentDir(Directory);
                CurrentDir.AppendDirectory(SearchDir.GetLastDirectory().c_str());
                FillRomList(FileList, CurrentDir);
            }
            continue;
        }

        AddFileNameToList(FileList, Directory, SearchDir);

        stdstr Extension = stdstr(SearchDir.GetExtension()).ToLower();
        for (uint8_t i = 0; i < sizeof(ROM_extensions) / sizeof(ROM_extensions[0]); i++)
        {
            if (Extension != ROM_extensions[i])
            {
                continue;
            }
            WriteTrace(TraceRomList, TraceVerbose, "File has matching extension: \"%s\"", ROM_extensions[i]);
            if (Extension != "7z")
            {
                AddRomToList(SearchDir);
            }
#ifdef _WIN32
            else
            {
                WriteTrace(TraceRomList, TraceVerbose, "Looking at contents of 7z file");
                try
                {
                    C7zip ZipFile(SearchDir);
                    if (!ZipFile.OpenSuccess())
                    {
                        continue;
                    }
                    char ZipFileName[260];
                    stdstr_f SectionName("%s-%d", ZipFile.FileName(ZipFileName, sizeof(ZipFileName)), ZipFile.FileSize());
                    SectionName.ToLower();

                    WriteTrace(TraceUserInterface, TraceDebug, "4 %s", SectionName.c_str());
                    for (int32_t zi = 0; zi < ZipFile.NumFiles(); zi++)
                    {
                        CSzFileItem * f = ZipFile.FileItem(zi);
                        if (f->IsDir)
                        {
                            continue;
                        }
                        ROM_INFO RomInfo;

                        std::wstring FileNameW = ZipFile.FileNameIndex(zi);
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
                        stdstr_f zipFileName("%s?%s", (LPCSTR)SearchDir, FileName.c_str());
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
                            int32_t CicChip = CN64Rom::GetCicChipID(RomData);

                            // Save this info
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
                            *(uint32_t *)&RomData[x] = strtoul(&szHeader[2 * x], nullptr, 16);
                            szHeader[2 * x + delimit_offset] = backup_character;
                        }

                        WriteTrace(TraceUserInterface, TraceDebug, "14");
                        {
                            char InternalName[22];
                            memcpy(InternalName, (void *)(RomData + 0x20), 20);
                            CN64Rom::CleanRomName(InternalName);
                            strcpy(RomInfo.InternalName, stdstr(InternalName).c_str());
                        }
                        RomInfo.RomSize = (int32_t)f->Size;

                        WriteTrace(TraceUserInterface, TraceDebug, "15");
                        RomInfo.CartID[0] = *(RomData + 0x3F);
                        RomInfo.CartID[1] = *(RomData + 0x3E);
                        RomInfo.CartID[2] = '\0';
                        RomInfo.Media = *(RomData + 0x38);
                        RomInfo.Country = *(RomData + 0x3D);
                        RomInfo.CRC1 = *(uint32_t *)(RomData + 0x10);
                        RomInfo.CRC2 = *(uint32_t *)(RomData + 0x14);
                        m_ZipIniFile->GetNumber(SectionName.c_str(), stdstr_f("%s-Cic", FileName.c_str()).c_str(), (ULONG)-1, (uint32_t &)RomInfo.CicChip);
                        WriteTrace(TraceUserInterface, TraceDebug, "16");
                        FillRomExtensionInfo(&RomInfo);

                        WriteTrace(TraceUserInterface, TraceDebug, "17");
                        int32_t ListPos = (int32_t)m_RomInfo.size();
                        m_RomInfo.push_back(RomInfo);
                        RomAddedToList(ListPos);
                    }
                }
                catch (...)
                {
                    WriteTrace(TraceUserInterface, TraceError, "Exception processing %s", (LPCSTR)SearchDir);
                }
            }
#endif
            break;
        }
    } while (SearchDir.FindNext());
#ifdef _WIN32
    m_ZipIniFile->FlushChanges();
#endif
    WriteTrace(TraceRomList, TraceDebug, "Done (Directory: %s)", Directory);
}

void CRomList::NotificationCB(const char * Status, CRomList * /*_this*/)
{
    g_Notify->DisplayMessage(5, Status);
}

void CRomList::RefreshRomListStatic(CRomList * _this)
{
    _this->RefreshRomListThread();
}

bool CRomList::LoadDataFromRomFile(const char * FileName, uint8_t * Data, int32_t DataLen, int32_t * RomSize, FILE_FORMAT & FileFormat)
{
    uint8_t Test[0x20];

    if (_strnicmp(&FileName[strlen(FileName) - 4], ".ZIP", 4) == 0)
    {
        int32_t len, port = 0, FoundRom;
        unz_file_info info;
        char zname[132];
        unzFile file;
        file = unzOpen(FileName);
        if (file == nullptr) { return false; }

        port = unzGoToFirstFile(file);
        FoundRom = false;
        while (port == UNZ_OK && FoundRom == false)
        {
            unzGetCurrentFileInfo(file, &info, zname, 128, nullptr, 0, nullptr, 0);
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
        CFile File;
        if (!File.Open(FileName, CFileBase::modeRead))
        {
            return false;
        }
        File.SeekToBegin();
        if (!File.Read(Test, sizeof(Test)))
        {
            return false;
        }

        if (CN64Rom::IsValidRomImage(Test))
        {
            File.SeekToBegin();
            if (!File.Read(Data, DataLen))
            {
                return false;
            }
        }
        else if (!CN64Disk::IsValidDiskImage(Test) && (File.GetLength() == DISKSIZE_MAME || File.GetLength() == DISKSIZE_SDK))
        {
            uint32_t sysdataoffset = 0;
            uint32_t diskidoffset = 0x43670;
            uint32_t romdataoffset = 0x738C0;
            bool isValidDisk = false;
            // Could still be a disk image

            // System data
            const uint8_t blocks[7] = { 2, 3, 10, 11, 1, 8, 9 };
            for (int i = 0; i < 7; i++)
            {
                sysdataoffset = 0x4D08 * blocks[i];
                File.Seek(sysdataoffset, CFileBase::begin);
                if (File.Read(Test, sizeof(Test)) != sizeof(Test))
                {
                    return false;
                }

                isValidDisk = CN64Disk::IsValidDiskImage(Test);
                if (isValidDisk)
                    break;
            }

            if (!isValidDisk)
            {
                return false;
            }

            File.Seek(sysdataoffset, CFileBase::begin);
            if (!File.Read(Data, 0x100))
            {
                return false;
            }
            File.Seek(diskidoffset, CFileBase::begin);
            if (!File.Read(Data + 0x100, 0x20))
            {
                return false;
            }
            File.Seek(romdataoffset, CFileBase::begin);
            if (!File.Read(Data + 0x200, 0x4D08))
            {
                return false;
            }
        }
        else if (CN64Disk::IsValidDiskImage(Test))
        {
            // If it is a disk image
            uint32_t sysdataoffset = 0;
            uint32_t diskidoffset = 0x100;
            uint32_t romdataoffset = 0x200;

            if ((File.GetLength() == DISKSIZE_MAME) || (File.GetLength() == DISKSIZE_SDK))
            {
                diskidoffset = 0x43670;
                romdataoffset = 0x738C0;
            }

            File.Seek(sysdataoffset, CFileBase::begin);
            if (!File.Read(Data, 0x100))
            {
                return false;
            }
            File.Seek(diskidoffset, CFileBase::begin);
            if (!File.Read(Data + 0x100, 0x20))
            {
                return false;
            }
            File.Seek(romdataoffset, CFileBase::begin);
            if (!File.Read(Data + 0x200, 0x4D08))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        *RomSize = File.GetLength();
        FileFormat = Format_Uncompressed;
    }
    ByteSwapRomData(Data, DataLen);
    return true;
}

bool CRomList::FillRomInfo(ROM_INFO * pRomInfo)
{
    uint8_t RomData[0x4F08];

    if (LoadDataFromRomFile(pRomInfo->szFullFileName, RomData, sizeof(RomData), &pRomInfo->RomSize, pRomInfo->FileFormat))
    {
        if (strstr(pRomInfo->szFullFileName, "?") != nullptr)
        {
            strcpy(pRomInfo->FileName, strstr(pRomInfo->szFullFileName, "?") + 1);
        }
        else
        {
            strncpy(pRomInfo->FileName, g_Settings->LoadBool(RomList_ShowFileExtensions) ? CPath(pRomInfo->szFullFileName).GetNameExtension().c_str() : CPath(pRomInfo->szFullFileName).GetName().c_str(), sizeof(pRomInfo->FileName) / sizeof(pRomInfo->FileName[0]));
        }

        if ((CPath(pRomInfo->szFullFileName).GetExtension() != "ndd") && (CPath(pRomInfo->szFullFileName).GetExtension() != "d64"))
        {
            char InternalName[22];
            memcpy(InternalName, (void *)(RomData + 0x20), 20);
            CN64Rom::CleanRomName(InternalName);
            strcpy(pRomInfo->InternalName, InternalName);
            pRomInfo->CartID[0] = *(RomData + 0x3F);
            pRomInfo->CartID[1] = *(RomData + 0x3E);
            pRomInfo->CartID[2] = '\0';
            pRomInfo->Media = *(RomData + 0x38);
            pRomInfo->Country = *(RomData + 0x3D);
            pRomInfo->CRC1 = *(uint32_t *)(RomData + 0x10);
            pRomInfo->CRC2 = *(uint32_t *)(RomData + 0x14);
            pRomInfo->CicChip = CN64Rom::GetCicChipID(RomData);
            if (pRomInfo->CicChip == CIC_NUS_8303 || pRomInfo->CicChip == CIC_NUS_DDUS || pRomInfo->CicChip == CIC_NUS_8401)
            {
                pRomInfo->CRC1 = (*(uint16_t *)(RomData + 0x608) << 16) | *(uint16_t *)(RomData + 0x60C);
                pRomInfo->CRC2 = (*(uint16_t *)(RomData + 0x638) << 16) | *(uint16_t *)(RomData + 0x63C);
            }

            FillRomExtensionInfo(pRomInfo);
        }
        else
        {
            char InternalName[22];
            memcpy(InternalName, (void *)(RomData + 0x100), 4);
            strcpy(pRomInfo->InternalName, InternalName);
            pRomInfo->CartID[0] = *(RomData + 0x102);
            pRomInfo->CartID[1] = *(RomData + 0x101);
            pRomInfo->CartID[2] = '\0';
            pRomInfo->Media = *(RomData + 0x103);
            pRomInfo->Country = *(RomData + 0x100);
            pRomInfo->CRC1 = 0;
            for (uint32_t i = 0; i < 0x4D08; i += 4)
            {
                pRomInfo->CRC1 += *(uint32_t *)(&RomData[0x200 + i]);
            }
            pRomInfo->CRC2 = ~pRomInfo->CRC1;
            pRomInfo->CicChip = CIC_NUS_8303;
            FillRomExtensionInfo(pRomInfo);
        }
        return true;
    }
    return false;
}

void CRomList::FillRomExtensionInfo(ROM_INFO * pRomInfo)
{
    // Initialize the structure
    pRomInfo->UserNotes[0] = '\0';
    pRomInfo->Developer[0] = '\0';
    pRomInfo->ReleaseDate[0] = '\0';
    pRomInfo->Genre[0] = '\0';
    pRomInfo->Players = 1;
    pRomInfo->CoreNotes[0] = '\0';
    pRomInfo->PluginNotes[0] = '\0';
    strcpy(pRomInfo->GoodName, "#340#");
    strcpy(pRomInfo->Name, "#321#");
    strcpy(pRomInfo->Status, "Unknown");

    char Identifier[100];
    sprintf(Identifier, "%08X-%08X-C:%X", pRomInfo->CRC1, pRomInfo->CRC2, pRomInfo->Country);
    if (m_GameIdentifiers.find(Identifier) == m_GameIdentifiers.end())
    {
        std::string AltIdentifier = stdstr_f("%s-C:%X", stdstr(pRomInfo->InternalName).Trim().ToUpper().c_str(), pRomInfo->Country);
        AltIdentifier = m_RomIniFile->GetString(AltIdentifier.c_str(), "Alt Identifier", "");
        if (!AltIdentifier.empty())
        {
            strcpy(Identifier, AltIdentifier.c_str());
        }
    }

    // ROM notes
    strncpy(pRomInfo->UserNotes, m_NotesIniFile->GetString(Identifier, "Note", "").c_str(), sizeof(pRomInfo->UserNotes) / sizeof(char));

    // ROM extension info
    strncpy(pRomInfo->Developer, m_ExtIniFile->GetString(Identifier, "Developer", "").c_str(), sizeof(pRomInfo->Developer) / sizeof(char));
    strncpy(pRomInfo->ReleaseDate, m_ExtIniFile->GetString(Identifier, "ReleaseDate", "").c_str(), sizeof(pRomInfo->ReleaseDate) / sizeof(char));
    strncpy(pRomInfo->Genre, m_ExtIniFile->GetString(Identifier, "Genre", "").c_str(), sizeof(pRomInfo->Genre) / sizeof(char));
    m_ExtIniFile->GetNumber(Identifier, "Players", 1, (uint32_t &)pRomInfo->Players);
    strncpy(pRomInfo->ForceFeedback, m_ExtIniFile->GetString(Identifier, "ForceFeedback", "unknown").c_str(), sizeof(pRomInfo->ForceFeedback) / sizeof(char));

    // ROM settings
    strncpy(pRomInfo->GoodName, m_RomIniFile->GetString(Identifier, "Good Name", pRomInfo->GoodName).c_str(), sizeof(pRomInfo->GoodName) / sizeof(char));
    strncpy(pRomInfo->Name, m_RomIniFile->GetString(Identifier, "Good Name", pRomInfo->Name).c_str(), sizeof(pRomInfo->Name) / sizeof(char));
    strncpy(pRomInfo->Status, m_RomIniFile->GetString(Identifier, "Status", pRomInfo->Status).c_str(), sizeof(pRomInfo->Status) / sizeof(char));
    strncpy(pRomInfo->CoreNotes, m_RomIniFile->GetString(Identifier, "Core Note", "").c_str(), sizeof(pRomInfo->CoreNotes) / sizeof(char));
    strncpy(pRomInfo->PluginNotes, m_RomIniFile->GetString(Identifier, "Plugin Note", "").c_str(), sizeof(pRomInfo->PluginNotes) / sizeof(char));

    // Get the text color
    stdstr String = m_RomIniFile->GetString("Rom Status", pRomInfo->Status, "000000");
    pRomInfo->TextColor = (strtoul(String.c_str(), 0, 16) & 0xFFFFFF);
    pRomInfo->TextColor = (pRomInfo->TextColor & 0x00FF00) | ((pRomInfo->TextColor >> 0x10) & 0xFF) | ((pRomInfo->TextColor & 0xFF) << 0x10);

    // Get the selected color
    String.Format("%s.Sel", pRomInfo->Status);
    String = m_RomIniFile->GetString("Rom Status", String.c_str(), "FFFFFFFF");
    uint32_t selcol = strtoul(String.c_str(), nullptr, 16);
    if (selcol & 0x80000000)
    {
        pRomInfo->SelColor = -1;
    }
    else
    {
        selcol = (selcol & 0x00FF00) | ((selcol >> 0x10) & 0xFF) | ((selcol & 0xFF) << 0x10);
        pRomInfo->SelColor = selcol;
    }

    // Get the selected text color
    String.Format("%s.Seltext", pRomInfo->Status);
    String = m_RomIniFile->GetString("Rom Status", String.c_str(), "FFFFFF");
    pRomInfo->SelTextColor = (strtoul(String.c_str(), 0, 16) & 0xFFFFFF);
    pRomInfo->SelTextColor = (pRomInfo->SelTextColor & 0x00FF00) | ((pRomInfo->SelTextColor >> 0x10) & 0xFF) | ((pRomInfo->SelTextColor & 0xFF) << 0x10);
}

void CRomList::ByteSwapRomData(uint8_t * Data, int32_t DataLen)
{
    int32_t count;

    switch (*((uint32_t *)&Data[0]))
    {
    case 0x12408037:
    case 0x07408027: // 64DD IPL
    case 0xD316E848: // 64DD JP disk
    case 0xEE562263: // 64DD US disk
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
    case 0x40072780: // 64DD IPL
    case 0x16D348E8: // 64DD JP disk
    case 0x56EE6322: // 64DD US disk
    case 0x40123780:
    case 0x00000000: // 64DD DEV disk
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
    case 0x80371240:
    case 0x80270740: // 64DD IPL
    case 0xE848D316: // 64DD JP disk
    case 0x2263EE56: // 64DD US disk
        break;
    }
}

void CRomList::LoadRomList(void)
{
    WriteTrace(TraceRomList, TraceVerbose, "Start");
    CPath FileName(g_Settings->LoadStringVal(RomList_RomListCache));
    CFile file(FileName, CFileBase::modeRead | CFileBase::modeNoTruncate);

    if (!file.IsOpen())
    {
        // If file does not exist then refresh the data
        RefreshRomList();
        return;
    }
    unsigned char md5[16];
    if (!file.Read(md5, sizeof(md5)))
    {
        file.Close();
        RefreshRomList();
        return;
    }

    // Read the size of ROM_INFO
    int32_t RomInfoSize = 0;
    if (!file.Read(&RomInfoSize, sizeof(RomInfoSize)) || RomInfoSize != sizeof(ROM_INFO))
    {
        file.Close();
        RefreshRomList();
        return;
    }

    // Read the number of entries
    int32_t Entries = 0;
    file.Read(&Entries, sizeof(Entries));

    // Read every entry
    m_RomInfo.clear();
    RomListReset();
    for (int32_t count = 0; count < Entries; count++)
    {
        ROM_INFO RomInfo;
        file.Read(&RomInfo, RomInfoSize);
        int32_t ListPos = (int32_t)m_RomInfo.size();
        m_RomInfo.push_back(RomInfo);
        RomAddedToList(ListPos);
    }
    RomListLoaded();
    WriteTrace(TraceRomList, TraceVerbose, "Done");
}

// SaveRomList - save all the ROM information about the current ROMs in the ROM browser
// to a cache file, so it is quick to reload the information

void CRomList::SaveRomList(strlist & FileList)
{
    MD5 ListHash = RomListHash(FileList);

    CPath FileName(g_Settings->LoadStringVal(RomList_RomListCache));
    CFile file(FileName, CFileBase::modeWrite | CFileBase::modeCreate);
    file.Write(ListHash.raw_digest(), 16);

    // Write the size of ROM_INFO
    int32_t RomInfoSize = sizeof(ROM_INFO);
    file.Write(&RomInfoSize, sizeof(RomInfoSize));

    // Write the number of entries
    int32_t Entries = (int32_t)m_RomInfo.size();
    file.Write(&Entries, sizeof(Entries));

    // Write every entry
    for (int32_t count = 0; count < Entries; count++)
    {
        file.Write(&m_RomInfo[count], RomInfoSize);
    }

    // Close the file handle
    file.Close();
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
    MD5 md5Hash((const unsigned char *)NewFileNames.c_str(), (unsigned int)NewFileNames.length());
    WriteTrace(TraceUserInterface, TraceDebug, "%s - %s", md5Hash.hex_digest(), NewFileNames.c_str());
    return md5Hash;
}

void CRomList::RefreshSettings(CRomList * _this)
{
    _this->m_GameDir = g_Settings->LoadStringVal(RomList_GameDir).c_str();
}

void CRomList::AddFileNameToList(strlist & FileList, const stdstr & Directory, CPath & File)
{
    uint8_t i;

    if (FileList.size() > 3000)
    {
        return;
    }

    stdstr Extension = stdstr(File.GetExtension()).ToLower();
    for (i = 0; i < sizeof(ROM_extensions) / sizeof(ROM_extensions[0]); i++)
    {
        if (Extension == ROM_extensions[i])
        {
            stdstr FileName = Directory + File.GetNameExtension();
            FileName.ToLower();
            FileList.push_back(FileName);
            break;
        }
    }
}
