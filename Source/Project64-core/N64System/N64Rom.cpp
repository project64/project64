#include "stdafx.h"

#include "N64Rom.h"
#include "SystemGlobals.h"
#include <Common/IniFile.h>
#include <Common/MemoryManagement.h>
#include <Common/Platform.h>
#include <Common/md5.h>
#include <Project64-core/3rdParty/zip.h>
#include <memory>

#ifdef _WIN32
#include <Project64-core/3rdParty/7zip.h>
#endif

CN64Rom::CN64Rom() :
    m_ROMImage(nullptr),
    m_ROMImageBase(nullptr),
    m_RomFileSize(0),
    m_ErrorMsg(EMPTY_STRING),
    m_Country(Country_Unknown),
    m_CicChip(CIC_UNKNOWN)
{
}

CN64Rom::~CN64Rom()
{
    UnallocateRomImage();
}

bool CN64Rom::AllocateRomImage(uint32_t RomFileSize)
{
    WriteTrace(TraceN64System, TraceDebug, "Allocating memory for ROM");
    std::unique_ptr<uint8_t> ImageBase(new uint8_t[RomFileSize + 0x2000]);
    if (ImageBase.get() == nullptr)
    {
        SetError(MSG_MEM_ALLOC_ERROR);
        WriteTrace(TraceN64System, TraceError, "Failed to allocate memory for ROM (size: 0x%X)", RomFileSize);
        return false;
    }
    uint8_t * Image = (uint8_t *)(((uint64_t)ImageBase.get() + 0xFFF) & ~0xFFF); // Start at beginning of memory page
    WriteTrace(TraceN64System, TraceDebug, "Allocated ROM memory (%p)", Image);

    // Save information about the ROM loaded
    m_ROMImageBase = ImageBase.release();
    m_ROMImage = Image;
    m_RomFileSize = RomFileSize;
    return true;
}

bool CN64Rom::AllocateAndLoadN64Image(const char * FileLoc, bool LoadBootCodeOnly)
{
    WriteTrace(TraceN64System, TraceDebug, "Trying to open %s", FileLoc);
    if (!m_RomFile.Open(FileLoc, CFileBase::modeRead))
    {
        SetError(MSG_ROM_FAILED_TO_OPEN);
        WriteTrace(TraceN64System, TraceError, "Failed to open %s", FileLoc);
        return false;
    }

    // Read the first 4 bytes and make sure it is a valid N64 image
    uint8_t Test[4];
    m_RomFile.SeekToBegin();
    if (m_RomFile.Read(Test, sizeof(Test)) != sizeof(Test))
    {
        m_RomFile.Close();
        SetError(MSG_ROM_FAILED_READ_IDENT);
        WriteTrace(TraceN64System, TraceError, "Failed to read ident bytes");
        return false;
    }
    if (!IsValidRomImage(Test))
    {
        m_RomFile.Close();
        SetError(MSG_ROM_INVALID_IMAGE_FILE);
        WriteTrace(TraceN64System, TraceError, "Invalid image file %X %X %X %X", Test[0], Test[1], Test[2], Test[3]);
        return false;
    }
    uint32_t RomFileSize = m_RomFile.GetLength();
    WriteTrace(TraceN64System, TraceDebug, "Successfully opened, size: 0x%X", RomFileSize);

    // If loading boot code then just load the first 0x1000 bytes
    if (LoadBootCodeOnly)
    {
        WriteTrace(TraceN64System, TraceDebug, "Loading boot code, so loading the first 0x1000 bytes", RomFileSize);
        RomFileSize = 0x1000;
    }

    if (!AllocateRomImage(RomFileSize))
    {
        m_RomFile.Close();
        SetError(MSG_ROM_FAILED_ROM_ALLOCATE);
        return false;
    }

    // Load the N64 ROM into the allocated memory
    g_Notify->DisplayMessage(5, MSG_LOADING);
    m_RomFile.SeekToBegin();

    uint32_t count, TotalRead = 0;
    for (count = 0; count < (int)RomFileSize; count += ReadFromRomSection)
    {
        uint32_t dwToRead = RomFileSize - count;
        if (dwToRead > ReadFromRomSection)
        {
            dwToRead = ReadFromRomSection;
        }

        if (m_RomFile.Read(&m_ROMImage[count], dwToRead) != dwToRead)
        {
            m_RomFile.Close();
            SetError(MSG_FAIL_IMAGE);
            WriteTrace(TraceN64System, TraceError, "Failed to read file (TotalRead: 0x%X)", TotalRead);
            return false;
        }
        TotalRead += dwToRead;

        // Show message of how much of the ROM has been loaded (as a percent)
        g_Notify->DisplayMessage(0, stdstr_f("%s: %.2f%c", GS(MSG_LOADED), ((float)TotalRead / (float)RomFileSize) * 100.0f, '%').c_str());
    }

    if (RomFileSize != TotalRead)
    {
        m_RomFile.Close();
        SetError(MSG_FAIL_IMAGE);
        WriteTrace(TraceN64System, TraceError, "Expected to read: 0x%X, read: 0x%X", TotalRead, RomFileSize);
        return false;
    }

    g_Notify->DisplayMessage(5, MSG_BYTESWAP);
    ByteSwapRom();

    // Protect the memory so that it can't be written to
    ProtectMemory(m_ROMImage, m_RomFileSize, MEM_READONLY);
    return true;
}

bool CN64Rom::AllocateAndLoadZipImage(const char * FileLoc, bool LoadBootCodeOnly)
{
    unzFile file = unzOpen(FileLoc);
    if (file == nullptr)
    {
        return false;
    }

    int port = unzGoToFirstFile(file);
    bool FoundRom = false;

    // Scan through all files in zip until a suitable file is found
    while (port == UNZ_OK && !FoundRom)
    {
        unz_file_info info;
        char zname[260];

        unzGetCurrentFileInfo(file, &info, zname, sizeof(zname), nullptr, 0, nullptr, 0);
        if (unzLocateFile(file, zname, 1) != UNZ_OK)
        {
            SetError(MSG_FAIL_ZIP);
            break;
        }
        if (unzOpenCurrentFile(file) != UNZ_OK)
        {
            SetError(MSG_FAIL_ZIP);
            break;
        }

        // Read the first 4 bytes to check magic number
        uint8_t Test[4];
        unzReadCurrentFile(file, Test, sizeof(Test));
        if (IsValidRomImage(Test))
        {
            // Get the size of the ROM and try to allocate the memory needed
            uint32_t RomFileSize = info.uncompressed_size;
            if (LoadBootCodeOnly)
            {
                RomFileSize = 0x1000;
            }

            if (!AllocateRomImage(RomFileSize))
            {
                m_RomFile.Close();
                return false;
            }

            // Load the N64 ROM into the allocated memory
            g_Notify->DisplayMessage(5, MSG_LOADING);
            memcpy(m_ROMImage, Test, 4);

            uint32_t dwRead, count, TotalRead = 0;
            for (count = 4; count < (int)RomFileSize; count += ReadFromRomSection)
            {
                uint32_t dwToRead = RomFileSize - count;
                if (dwToRead > ReadFromRomSection)
                {
                    dwToRead = ReadFromRomSection;
                }

                dwRead = unzReadCurrentFile(file, &m_ROMImage[count], dwToRead);
                if (dwRead == 0)
                {
                    SetError(MSG_FAIL_ZIP);
                    unzCloseCurrentFile(file);
                    break;
                }
                TotalRead += dwRead;

                // Show message of how much of the ROM has been loaded (as a percent)
                g_Notify->DisplayMessage(5, stdstr_f("%s: %.2f%c", GS(MSG_LOADED), ((float)TotalRead / (float)RomFileSize) * 100.0f, '%').c_str());
            }
            dwRead = TotalRead + 4;

            if (RomFileSize != dwRead)
            {
                unzCloseCurrentFile(file);
                SetError(MSG_FAIL_ZIP);
                g_Notify->DisplayMessage(1, "");
                break;
            }
            FoundRom = true;

            g_Notify->DisplayMessage(5, MSG_BYTESWAP);
            ByteSwapRom();

            // Protect the memory so that it can't be written to
            ProtectMemory(m_ROMImage, m_RomFileSize, MEM_READONLY);
        }
        unzCloseCurrentFile(file);

        if (!FoundRom)
        {
            port = unzGoToNextFile(file);
        }
    }
    unzClose(file);

    return FoundRom;
}

void CN64Rom::ByteSwapRom()
{
    uint32_t count;

    switch (*((uint32_t *)&m_ROMImage[0]))
    {
    case 0x12408037:
        for (count = 0; count < m_RomFileSize; count += 4)
        {
            m_ROMImage[count] ^= m_ROMImage[count + 2];
            m_ROMImage[count + 2] ^= m_ROMImage[count];
            m_ROMImage[count] ^= m_ROMImage[count + 2];
            m_ROMImage[count + 1] ^= m_ROMImage[count + 3];
            m_ROMImage[count + 3] ^= m_ROMImage[count + 1];
            m_ROMImage[count + 1] ^= m_ROMImage[count + 3];
        }
        break;
    case 0x40072780: // 64DD IPL
    case 0x40123780:
        for (count = 0; count < m_RomFileSize; count += 4)
        {
            m_ROMImage[count] ^= m_ROMImage[count + 3];
            m_ROMImage[count + 3] ^= m_ROMImage[count];
            m_ROMImage[count] ^= m_ROMImage[count + 3];
            m_ROMImage[count + 1] ^= m_ROMImage[count + 2];
            m_ROMImage[count + 2] ^= m_ROMImage[count + 1];
            m_ROMImage[count + 1] ^= m_ROMImage[count + 2];
        }
        break;
    case 0x80371240: break;
    default:
        g_Notify->DisplayError(stdstr_f("ByteSwapRom: %X", m_ROMImage[0]).c_str());
    }
}

CICChip CN64Rom::GetCicChipID(uint8_t * RomData, uint64_t * CRC)
{
    uint64_t crc = 0;
    uint64_t crcAleck64 = 0;
    int32_t count;

    for (count = 0x40; count < 0x1000; count += 4)
    {
        if (count == 0xC00) crcAleck64 = crc; //From 0x40 to 0xC00 (Aleck64)
        crc += *(uint32_t *)(RomData + count);
    }
    if (CRC != nullptr)
    {
        *CRC = crc;
    }

    switch (crc)
    {
    case 0x000000D0027FDF31: return CIC_NUS_6101;
    case 0x000000CFFB631223: return CIC_NUS_6101;
    case 0x000000D057C85244: return CIC_NUS_6102;
    case 0x000000D6497E414B: return CIC_NUS_6103;
    case 0x0000011A49F60E96: return CIC_NUS_6105;
    case 0x000000D6D5BE5580: return CIC_NUS_6106;
    case 0x000001053BC19870: return CIC_NUS_5167; // 64DD conversion CIC
    case 0x000000D2E53EF008: return CIC_NUS_8303; // 64DD IPL
    case 0x000000D2E53EF39F: return CIC_NUS_8401; // 64DD IPL tool
    case 0x000000D2E53E5DDA: return CIC_NUS_DDUS; // 64DD IPL US (different CIC)
    case 0x0000000AF3A34BC8: return CIC_MINI_IPL3;
    case 0x0000007c56242373: return CIC_NUS_6102; // LibDragon IPL3
    default:
        //Aleck64 CIC
        if (crcAleck64 == 0x000000A5F80BF620)
        {
            if (CRC != nullptr)
            {
                *CRC = crcAleck64;
            }
            return CIC_NUS_5101;
        }
        return CIC_UNKNOWN;
    }
}

void CN64Rom::CalculateCicChip()
{
    uint64_t CRC = 0;

    if (m_ROMImage == nullptr)
    {
        m_CicChip = CIC_UNKNOWN;
        return;
    }
    m_CicChip = GetCicChipID(m_ROMImage, &CRC);
    if (m_CicChip == CIC_UNKNOWN)
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("Unknown CIC checksum:\n%llX.", CRC).c_str());
        }
    }
}

void CN64Rom::CalculateRomCrc()
{
    uint32_t t0, t2, t3, t4, t5;
    uint32_t a0, a1, a2, a3;
    uint32_t s0;
    uint32_t v0, v1;
    uint32_t length = 0x00100000;

    // CIC_NUS_6101 at=0x5D588B65 , s6=0x3F
    // CIC_NUS_6102 at=0x5D588B65 , s6=0x3F
    // CIC_NUS_6103 at=0x6C078965 , s6=0x78
    // CIC_NUS_6105 at=0x5d588b65 , s6=0x91
    // CIC_NUS_6106 at=0x6C078965 , s6=0x85

    // 64DD IPL (JPN) at=0x02E90EDD , s6=0xdd
    // 64DD IPL (USA) at=0x02E90EDD , s6=0xde
    // 64DD TOOL IPL  at=0x0260BCD5 , s6=0xdd

    // CIC_NUS_5101 (Aleck64) at=0x6C078965 , s6=0xac

    //v0 = 0xFFFFFFFF & (s6 * at) + 1;
    switch (m_CicChip)
    {
    case CIC_NUS_6101:
    case CIC_NUS_6102: v0 = 0xF8CA4DDC; break;
    case CIC_NUS_6103: v0 = 0xA3886759; break;
    case CIC_NUS_6105: v0 = 0xDF26F436; break;
    case CIC_NUS_6106: v0 = 0x1FEA617A; break;
    case CIC_NUS_DDUS:
        length = 0x000A0000;
        v0 = 0x861AE3A7;
        break;
    case CIC_NUS_8303:
        length = 0x000A0000;
        v0 = 0x8331D4CA;
        break;
    case CIC_NUS_8401:
        length = 0x000A0000;
        v0 = 0x0D8303E2;
        break;
    case CIC_NUS_5101: v0 = 0x95104FDD; break;
    default:
        return;
    }

    ProtectMemory(m_ROMImage, m_RomFileSize, MEM_READWRITE);

    if (m_CicChip == CIC_NUS_5101 && (*(uint32_t *)(m_ROMImage + 0x8) == 0x80100400))
        length = 0x003FE000;

    v1 = 0;
    t0 = 0;
    t5 = 0x20;

    a3 = v0;
    t2 = v0;
    t3 = v0;
    s0 = v0;
    a2 = v0;
    t4 = v0;

    for (t0 = 0; t0 < length; t0 += 4)
    {
        v0 = *(uint32_t *)(m_ROMImage + t0 + 0x1000);

        v1 = a3 + v0;
        a1 = v1;

        if (v1 < a3)
        {
            if (m_CicChip == CIC_NUS_DDUS || m_CicChip == CIC_NUS_8303)
            {
                t2 = t2 ^ t3;
            }
            else
            {
                t2 += 0x1;
            }
        }
        v1 = v0 & 0x001F;

        a0 = (v0 << v1) | (v0 >> (t5 - v1));

        a3 = a1;
        t3 = t3 ^ v0;

        s0 = s0 + a0;
        if (a2 < v0)
        {
            a2 = a3 ^ v0 ^ a2;
        }
        else
        {
            if (m_CicChip == CIC_NUS_8303)
                a2 = a2 + a0;
            else
                a2 = a2 ^ a0;
        }

        if (m_CicChip == CIC_NUS_6105)
        {
            t4 = (v0 ^ (*(uint32_t *)(m_ROMImage + (0xFF & t0) + 0x750))) + t4;
        }
        else
            t4 = (v0 ^ s0) + t4;
    }
    if (m_CicChip == CIC_NUS_6103)
    {
        a3 = (a3 ^ t2) + t3;
        s0 = (s0 ^ a2) + t4;
    }
    else if (m_CicChip == CIC_NUS_6106)
    {
        a3 = 0xFFFFFFFF & (a3 * t2) + t3;
        s0 = 0xFFFFFFFF & (s0 * a2) + t4;
    }
    else if (m_CicChip == CIC_NUS_5101)
    {
        a3 = 0xFFFFFFFF & (a3 ^ t2) + t3;
        s0 = 0xFFFFFFFF & (s0 ^ a2) + t4;
    }
    else
    {
        a3 = a3 ^ t2 ^ t3;
        s0 = s0 ^ a2 ^ t4;
    }

    *(uint32_t *)(m_ROMImage + 0x10) = a3;
    *(uint32_t *)(m_ROMImage + 0x14) = s0;

    ProtectMemory(m_ROMImage, m_RomFileSize, MEM_READONLY);
}

CICChip CN64Rom::CicChipID()
{
    return m_CicChip;
}

bool CN64Rom::IsValidRomImage(uint8_t Test[4])
{
    if (*((uint32_t *)&Test[0]) == 0x40123780)
    {
        return true;
    }
    if (*((uint32_t *)&Test[0]) == 0x12408037)
    {
        return true;
    }
    if (*((uint32_t *)&Test[0]) == 0x80371240)
    {
        return true;
    }
    if (*((uint32_t *)&Test[0]) == 0x40072780)
    {
        return true;
    } // 64DD IPL
    return false;
}

bool CN64Rom::IsLoadedRomDDIPL()
{
    switch (CicChipID())
    {
    case CIC_NUS_8303:
    case CIC_NUS_DDUS:
    case CIC_NUS_8401:
        return true;
    default:
        return false;
    }
}

void CN64Rom::CleanRomName(char * RomName, bool byteswap)
{
    if (byteswap)
    {
        for (int count = 0; count < 20; count += 4)
        {
            RomName[count] ^= RomName[count + 3];
            RomName[count + 3] ^= RomName[count];
            RomName[count] ^= RomName[count + 3];
            RomName[count + 1] ^= RomName[count + 2];
            RomName[count + 2] ^= RomName[count + 1];
            RomName[count + 1] ^= RomName[count + 2];
        }
    }

    // Truncate all the spaces at the end of the string
    for (int count = 19; count >= 0; count--)
    {
        if (RomName[count] == ' ')
        {
            RomName[count] = '\0';
        }
        else if (RomName[count] == '\0')
        {
        }
        else
        {
            count = -1;
        }
    }
    RomName[20] = '\0';

    // Remove all special characters from the string
    for (int count = 0; count < (int)strlen(RomName); count++)
    {
        switch (RomName[count])
        {
        case '/':
        case '\\': RomName[count] = '-'; break;
        case ':': RomName[count] = ';'; break;
        }
    }
}

void CN64Rom::NotificationCB(const char * Status, CN64Rom * /*_this*/)
{
    g_Notify->DisplayMessage(5, stdstr_f("%s", Status).c_str());
}

bool CN64Rom::LoadN64Image(const char * FileLoc, bool LoadBootCodeOnly)
{
    WriteTrace(TraceN64System, TraceDebug, "Start (FileLoc: \"%s\" LoadBootCodeOnly: %s)", FileLoc, LoadBootCodeOnly ? "true" : "false");

    UnallocateRomImage();
    m_ErrorMsg = EMPTY_STRING;

    stdstr ext = CPath(FileLoc).GetExtension();
    bool Loaded7zFile = false;

#ifdef _WIN32
    if (strstr(FileLoc, "?") != nullptr || _stricmp(ext.c_str(), "7z") == 0)
    {
        stdstr FullPath = FileLoc;

        // This should be a 7-zip file
        char * SubFile = strstr(const_cast<char *>(FullPath.c_str()), "?");
        if (SubFile != nullptr)
        {
            *SubFile = '\0';
            SubFile += 1;
        }
        //else load first found file until dialog is implemented
        //{
        // Pop up a dialog and select file
        // Allocate memory for sub name and copy selected file name to variable
        //}

        C7zip ZipFile(FullPath.c_str());
        ZipFile.SetNotificationCallback((C7zip::LP7ZNOTIFICATION)NotificationCB, this);
        for (int i = 0; i < ZipFile.NumFiles(); i++)
        {
            CSzFileItem * f = ZipFile.FileItem(i);
            if (f->IsDir)
            {
                continue;
            }

            stdstr ZipFileName;
            ZipFileName.FromUTF16(ZipFile.FileNameIndex(i).c_str());
            if (SubFile != nullptr)
            {
                if (_stricmp(ZipFileName.c_str(), SubFile) != 0)
                {
                    continue;
                }
            }

            // Get the size of the ROM and try to allocate the memory needed
            uint32_t RomFileSize = (uint32_t)f->Size;
            // If loading boot code then just load the first 0x1000 bytes
            if (LoadBootCodeOnly)
            {
                RomFileSize = 0x1000;
            }

            if (!AllocateRomImage(RomFileSize))
            {
                WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
                return false;
            }

            // Load the N64 ROM to the allocated memory
            g_Notify->DisplayMessage(5, MSG_LOADING);
            if (!ZipFile.GetFile(i, m_ROMImage, RomFileSize))
            {
                SetError(MSG_FAIL_IMAGE);
                WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
                return false;
            }

            if (!IsValidRomImage(m_ROMImage))
            {
                if (i < ZipFile.NumFiles() - 1)
                {
                    UnallocateRomImage();
                    continue;
                }
                SetError(MSG_FAIL_IMAGE);
                WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
                return false;
            }
            g_Notify->DisplayMessage(5, MSG_BYTESWAP);
            ByteSwapRom();

            // Protect the memory so that it can't be written to
            ProtectMemory(m_ROMImage, m_RomFileSize, MEM_READONLY);
            Loaded7zFile = true;
            break;
        }
        if (!Loaded7zFile)
        {
            SetError(MSG_7Z_FILE_NOT_FOUND);
            WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
            return false;
        }
    }
#endif

    // Try to open the file as a zip file
    if (!Loaded7zFile)
    {
        if (!AllocateAndLoadZipImage(FileLoc, LoadBootCodeOnly))
        {
            if (m_ErrorMsg != EMPTY_STRING)
            {
                WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
                return false;
            }
            if (!AllocateAndLoadN64Image(FileLoc, LoadBootCodeOnly))
            {
                WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
                return false;
            }
        }
    }

    char RomName[260];
    // Get the header from the ROM image
    memcpy(&RomName[0], (void *)(m_ROMImage + 0x20), 20);
    CN64Rom::CleanRomName(RomName);

    if (strlen(RomName) == 0)
    {
        strcpy(RomName, CPath(FileLoc).GetName().c_str());
        CN64Rom::CleanRomName(RomName, false);
    }

    WriteTrace(TraceN64System, TraceDebug, "RomName \"%s\"", RomName);

    m_RomName = RomName;
    m_FileName = FileLoc;
    m_MD5 = "";

    if (!LoadBootCodeOnly)
    {
        // Calculate files MD5 checksum
        m_MD5 = MD5((const unsigned char *)m_ROMImage, m_RomFileSize).hex_digest();
        WriteTrace(TraceN64System, TraceDebug, "MD5: %s", m_MD5.c_str());
    }

    m_Country = (Country)m_ROMImage[0x3D];
    CalculateCicChip();
    uint32_t CRC1, CRC2;

    if (IsLoadedRomDDIPL())
    {
        // Handle CRC differently if it is a 64DD IPL
        CRC1 = (*(uint16_t *)(&m_ROMImage[0x608]) << 16) | *(uint16_t *)(&m_ROMImage[0x60C]);
        CRC2 = (*(uint16_t *)(&m_ROMImage[0x638]) << 16) | *(uint16_t *)(&m_ROMImage[0x63C]);
    }
    else
    {
        CRC1 = *(uint32_t *)(&m_ROMImage[0x10]);
        CRC2 = *(uint32_t *)(&m_ROMImage[0x14]);
    }

    m_RomIdent = stdstr_f("%08X-%08X-C:%X", CRC1, CRC2, m_ROMImage[0x3D]);
    {
        CIniFileBase::SectionList GameIdentifiers;
        CIniFile RomDatabase(g_Settings->LoadStringVal(SupportFile_RomDatabase).c_str());
        RomDatabase.GetVectorOfSections(GameIdentifiers);

        if (GameIdentifiers.find(m_RomIdent.c_str()) == GameIdentifiers.end())
        {
            char InternalName[22] = {0};
            memcpy(InternalName, (void *)(m_ROMImage + 0x20), 20);
            CN64Rom::CleanRomName(InternalName);

            std::string AltIdentifier = stdstr_f("%s-C:%X", stdstr(InternalName).Trim().ToUpper().c_str(), m_Country);
            AltIdentifier = RomDatabase.GetString(AltIdentifier.c_str(), "Alt Identifier", "");
            if (!AltIdentifier.empty())
            {
                m_RomIdent = AltIdentifier;
            }
        }
    }
    WriteTrace(TraceN64System, TraceDebug, "Ident: %s", m_RomIdent.c_str());

    if (!LoadBootCodeOnly && g_Rom == this)
    {
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
        if (!g_Disk)
        {
            SaveRomSettingID(false);
        }
        else if (!IsLoadedRomDDIPL())
        {
            g_Settings->SaveString(Game_GameName, m_RomName.c_str()); // Use base games save file if loaded in combo
        }
    }

    if (g_Settings->LoadBool(Game_CRC_Recalc))
    {
        // Calculate ROM header CRC
        CalculateRomCrc();
    }

    WriteTrace(TraceN64System, TraceDebug, "Done (res: true)");
    return true;
}

bool CN64Rom::LoadN64ImageIPL(const char * FileLoc, bool LoadBootCodeOnly)
{
    UnallocateRomImage();
    m_ErrorMsg = EMPTY_STRING;

    stdstr ext = CPath(FileLoc).GetExtension();
    bool Loaded7zFile = false;
#ifdef _WIN32
    if (strstr(FileLoc, "?") != nullptr || _stricmp(ext.c_str(), "7z") == 0)
    {
        stdstr FullPath = FileLoc;

        // This should be a 7-zip file
        char * SubFile = strstr(const_cast<char *>(FullPath.c_str()), "?");
        if (SubFile != nullptr)
        {
            *SubFile = '\0';
            SubFile += 1;
        }
        //else load first found file until dialog is implemented
        //{
        // Pop up a dialog and select file
        // Allocate memory for sub name and copy selected file name to variable
        //}

        C7zip ZipFile(FullPath.c_str());
        ZipFile.SetNotificationCallback((C7zip::LP7ZNOTIFICATION)NotificationCB, this);
        for (int i = 0; i < ZipFile.NumFiles(); i++)
        {
            CSzFileItem * f = ZipFile.FileItem(i);
            if (f->IsDir)
            {
                continue;
            }

            stdstr ZipFileName;
            ZipFileName.FromUTF16(ZipFile.FileNameIndex(i).c_str());
            if (SubFile != nullptr)
            {
                if (_stricmp(ZipFileName.c_str(), SubFile) != 0)
                {
                    continue;
                }
            }

            // Get the size of the ROM and try to allocate the memory needed
            uint32_t RomFileSize = (uint32_t)f->Size;
            // If loading boot code then just load the first 0x1000 bytes
            if (LoadBootCodeOnly)
            {
                RomFileSize = 0x1000;
            }

            if (!AllocateRomImage(RomFileSize))
            {
                WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
                return false;
            }

            // Load the N64 ROM to the allocated memory
            g_Notify->DisplayMessage(5, MSG_LOADING);
            if (!ZipFile.GetFile(i, m_ROMImage, RomFileSize))
            {
                SetError(MSG_FAIL_IMAGE_IPL);
                WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
                return false;
            }

            if (!IsValidRomImage(m_ROMImage))
            {
                if (i < ZipFile.NumFiles() - 1)
                {
                    UnallocateRomImage();
                    continue;
                }
                SetError(MSG_FAIL_IMAGE_IPL);
                WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
                return false;
            }
            g_Notify->DisplayMessage(5, MSG_BYTESWAP);
            ByteSwapRom();

            // Protect the memory so that it can't be written to
            ProtectMemory(m_ROMImage, m_RomFileSize, MEM_READONLY);
            Loaded7zFile = true;
            break;
        }
        if (!Loaded7zFile)
        {
            SetError(MSG_7Z_FILE_NOT_FOUND);
            WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
            return false;
        }
    }
#endif

    // Try to open the file as a zip file
    if (!Loaded7zFile)
    {
        if (!AllocateAndLoadZipImage(FileLoc, LoadBootCodeOnly))
        {
            if (m_ErrorMsg != EMPTY_STRING)
            {
                return false;
            }
            if (!AllocateAndLoadN64Image(FileLoc, LoadBootCodeOnly))
            {
                return false;
            }
        }
    }

    char RomName[260];
    // Get the header from the ROM image
    memcpy(&RomName[0], (void *)(m_ROMImage + 0x20), 20);
    CN64Rom::CleanRomName(RomName);
    if (strlen(RomName) == 0)
    {
        strcpy(RomName, CPath(FileLoc).GetName().c_str());
        CN64Rom::CleanRomName(RomName, false);
    }
    WriteTrace(TraceN64System, TraceDebug, "RomName %s", RomName);

    m_RomName = RomName;
    m_FileName = FileLoc;
    m_MD5 = "";

    if (!LoadBootCodeOnly)
    {
        // Calculate files MD5 checksum
        m_MD5 = MD5((const unsigned char *)m_ROMImage, m_RomFileSize).hex_digest();
        WriteTrace(TraceN64System, TraceDebug, "MD5: %s", m_MD5.c_str());
    }

    m_Country = (Country)m_ROMImage[0x3D];
    CalculateCicChip();
    uint32_t CRC1, CRC2;

    if (IsLoadedRomDDIPL())
    {
        // Handle CRC differently if it is a 64DD IPL
        CRC1 = (*(uint16_t *)(&m_ROMImage[0x608]) << 16) | *(uint16_t *)(&m_ROMImage[0x60C]);
        CRC2 = (*(uint16_t *)(&m_ROMImage[0x638]) << 16) | *(uint16_t *)(&m_ROMImage[0x63C]);
    }
    else
    {
        CRC1 = *(uint32_t *)(&m_ROMImage[0x10]);
        CRC2 = *(uint32_t *)(&m_ROMImage[0x14]);
    }

    m_RomIdent = stdstr_f("%08X-%08X-C:%X", CRC1, CRC2, m_ROMImage[0x3D]);
    WriteTrace(TraceN64System, TraceDebug, "Ident: %s", m_RomIdent.c_str());

    if (!IsLoadedRomDDIPL())
    {
        SetError(MSG_FAIL_IMAGE_IPL);
        return false;
    }

    if (!LoadBootCodeOnly && g_DDRom == this)
    {
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
    }

    if (g_Settings->LoadBool(Game_CRC_Recalc))
    {
        // Calculate ROM header CRC
        CalculateRomCrc();
    }

    return true;
}

// Save the settings of the loaded ROM, so all loaded settings about ROM will be identified with this ROM

void CN64Rom::SaveRomSettingID(bool temp)
{
    g_Settings->SaveBool(Game_TempLoaded, temp);
    g_Settings->SaveString(Game_GameName, m_RomName.c_str());
    g_Settings->SaveString(Game_IniKey, m_RomIdent.c_str());
    g_Settings->SaveString(Game_UniqueSaveDir, stdstr_f("%s-%s", m_RomName.c_str(), m_MD5.c_str()).c_str());
    g_Settings->SaveDword(Game_SystemType, IsPal() ? SYSTEM_PAL : SYSTEM_NTSC);
}

void CN64Rom::ClearRomSettingID()
{
    g_Settings->SaveString(Game_GameName, "");
    g_Settings->SaveString(Game_IniKey, "");
}

void CN64Rom::SetError(LanguageStringID ErrorMsg)
{
    m_ErrorMsg = ErrorMsg;
}

bool CN64Rom::IsPal()
{
    switch (m_Country)
    {
    case Country_Germany:
    case Country_French:
    case Country_Italian:
    case Country_Europe:
    case Country_Spanish:
    case Country_Australia:
    case Country_EuropeanX_PAL:
    case Country_EuropeanY_PAL:
        return true;
    }
    return false;
}

void CN64Rom::UnallocateRomImage()
{
    m_RomFile.Close();

    if (m_ROMImageBase)
    {
        ProtectMemory(m_ROMImage, m_RomFileSize, MEM_READWRITE);
        delete[] m_ROMImageBase;
        m_ROMImageBase = nullptr;
    }
    m_ROMImage = nullptr;
}