#include "stdafx.h"
#include "N64DiskClass.h"
#include "SystemGlobals.h"
#include <Common/md5.h>
#include <Common/Platform.h>
#include <Common/MemoryManagement.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <memory>

CN64Disk::CN64Disk() :
    m_DiskImage(NULL),
    m_DiskImageBase(NULL),
    m_DiskHeader(NULL),
    m_DiskHeaderBase(NULL),
    m_ErrorMsg(EMPTY_STRING),
    m_DiskBufAddress(0),
    m_DiskSysAddress(0),
    m_DiskIDAddress(0),
    m_DiskRomAddress(0),
    m_DiskRamAddress(0),
    m_isShadowDisk(false)
{
}

CN64Disk::~CN64Disk()
{
}

bool CN64Disk::LoadDiskImage(const char * FileLoc)
{
    UnallocateDiskImage();
    m_ErrorMsg = EMPTY_STRING;

    //Assume the file extension is *.ndd or *.d64
    stdstr ext = CPath(FileLoc).GetExtension();
    stdstr ShadowFile = FileLoc;
    ShadowFile[ShadowFile.length() - 1] = 'r';

    WriteTrace(TraceN64System, TraceDebug, "Attempt to load shadow file.");
    if (!AllocateAndLoadDiskImage(ShadowFile.c_str()))
    {
        m_isShadowDisk = false;
        WriteTrace(TraceN64System, TraceDebug, "Loading Shadow file failed");
        UnallocateDiskImage();
        if (!AllocateAndLoadDiskImage(FileLoc))
        {
            return false;
        }
    }
    else
    {
        m_isShadowDisk = true;
    }

    char RomName[5];
    m_FileName = FileLoc;
    uint32_t crc1 = CalculateCrc();
    uint32_t crc2 = ~crc1;
    m_DiskIdent.Format("%08X-%08X-C:%X", crc1, crc2, GetDiskAddressID()[0]);
    //Get the disk ID from the disk image
    if (*(uint32_t *)(&GetDiskAddressID()[0]) != 0)
    {
        //if not 0x00000000
        RomName[0] = (char)*(GetDiskAddressID() + 3);
        RomName[1] = (char)*(GetDiskAddressID() + 2);
        RomName[2] = (char)*(GetDiskAddressID() + 1);
        RomName[3] = (char)*(GetDiskAddressID() + 0);
        RomName[4] = '\0';
    }
    else
    {
        //if 0x00000000 then use a made up one
        RomName[0] = m_DiskIdent[12];
        RomName[1] = m_DiskIdent[11];
        RomName[2] = m_DiskIdent[10];
        RomName[3] = m_DiskIdent[9];
        RomName[4] = '\0';

        for (uint8_t i = 0; i < 8; i++)
        {
            m_DiskHeader[0x20 + (i ^ 3)] = (uint8_t)m_DiskIdent[9 + i];
        }
    }
    m_RomName = RomName;
    m_Country = GetDiskCountryCode();
    m_DiskType = GetDiskAddressSys()[5 ^ 3] & 0x0F;

    GenerateLBAToPhysTable();
    InitSysDataD64();
    DetectRamAddress();
    LoadDiskRAMImage();

    if (g_Disk == this)
    {
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
        SaveDiskSettingID(false);
    }
    return true;
}

bool CN64Disk::SaveDiskImage()
{
    DeinitSysDataD64();

    //NO NEED TO SAVE IF DISK TYPE IS 6
    if (m_DiskType == 6)
    {
        m_DiskFile.Close();
        WriteTrace(TraceN64System, TraceDebug, "Loaded Disk Type is 6. No RAM area. Shadow file is not needed.");
        return true;
    }

    //Assume the file extension is *.ndd / *.d64
    if (m_DiskFormat == DiskFormatMAME || m_isShadowDisk || g_Settings->LoadDword(Setting_DiskSaveType) == SaveDisk_ShadowFile)
    {
        //Shadow File
        stdstr ShadowFile = m_FileName;
        ShadowFile[ShadowFile.length() - 1] = 'r';

        WriteTrace(TraceN64System, TraceDebug, "Trying to open %s (Shadow File)", ShadowFile.c_str());
        m_DiskFile.Close();
        if (!m_DiskFile.Open(ShadowFile.c_str(), CFileBase::modeWrite | CFileBase::modeCreate | CFileBase::modeNoTruncate))
        {
            WriteTrace(TraceN64System, TraceError, "Failed to open %s (Shadow File)", ShadowFile.c_str());
            return false;
        }

        m_DiskFile.SeekToBegin();
        ForceByteSwapDisk();

        if (!m_DiskFile.Write(m_DiskImage, m_DiskFileSize))
        {
            m_DiskFile.Close();
            WriteTrace(TraceN64System, TraceError, "Failed to write file");
            return false;
        }
    }
    else
    {
        //RAM File
        if (m_DiskFileSize <= m_DiskRamAddress || m_DiskRamAddress == 0)
        {
            m_DiskFile.Close();
            return true;
        }

        stdstr ShadowFile = m_FileName;
        ShadowFile[ShadowFile.length() - 1] = 'm';
        ShadowFile[ShadowFile.length() - 2] = 'a';
        ShadowFile[ShadowFile.length() - 3] = 'r';

        WriteTrace(TraceN64System, TraceDebug, "Trying to open %s (RAM File)", ShadowFile.c_str());
        m_DiskFile.Close();
        if (!m_DiskFile.Open(ShadowFile.c_str(), CFileBase::modeWrite | CFileBase::modeCreate | CFileBase::modeNoTruncate))
        {
            WriteTrace(TraceN64System, TraceError, "Failed to open %s (RAM File)", ShadowFile.c_str());
            return false;
        }

        m_DiskFile.SeekToBegin();
        ForceByteSwapDisk();

        if (!m_DiskFile.Write(GetDiskAddressRam(), m_DiskFileSize - m_DiskRamAddress))
        {
            m_DiskFile.Close();
            WriteTrace(TraceN64System, TraceError, "Failed to write file");
            return false;
        }
    }

    m_DiskFile.Close();
    return true;
}

void CN64Disk::SwapDiskImage(const char * FileLoc)
{
    g_Reg->ASIC_STATUS &= ~DD_STATUS_DISK_PRES;
    LoadDiskImage(FileLoc);
}

bool CN64Disk::IsValidDiskImage(uint8_t Test[0x20])
{
    //Basic System Data Check (first 0x20 bytes is enough)
    //Disk Type
    if ((Test[0x05] & 0xEF) > 6) return false;

    //IPL Load Block
    uint16_t ipl_load_blk = ((Test[0x06] << 8) | Test[0x07]);
    if (ipl_load_blk > 0x10C3 || ipl_load_blk == 0x0000) return false;

    //IPL Load Address
    uint32_t ipl_load_addr = (Test[0x1C] << 24) | (Test[0x1D] << 16) | (Test[0x1E] << 8) | Test[0x1F];
    if (ipl_load_addr < 0x80000000 && ipl_load_addr >= 0x80800000) return false;

    //Country Code
    if (*((uint32_t *)&Test[0]) == 0x16D348E8) { return true; }
    else if (*((uint32_t *)&Test[0]) == 0x56EE6322) { return true; }
    else if (*((uint32_t *)&Test[0]) == 0x00000000) { return true; }
    return false;
}

//Save the settings of the loaded rom, so all loaded settings about rom will be identified with
//this rom
void CN64Disk::SaveDiskSettingID(bool temp)
{
    g_Settings->SaveBool(Game_TempLoaded, temp);
    g_Settings->SaveString(Game_GameName, m_RomName.c_str());
    g_Settings->SaveString(Game_IniKey, m_DiskIdent.c_str());
    //g_Settings->SaveString(Game_UniqueSaveDir, stdstr_f("%s-%s", m_RomName.c_str(), m_MD5.c_str()).c_str());

    switch (GetCountry())
    {
    case Country_Germany:
    case Country_French:
    case Country_Italian:
    case Country_Europe:
    case Country_Spanish:
    case Country_Australia:
    case Country_EuropeanX_PAL:
    case Country_EuropeanY_PAL:
        g_Settings->SaveDword(Game_SystemType, SYSTEM_PAL);
        break;
    default:
        g_Settings->SaveDword(Game_SystemType, SYSTEM_NTSC);
        break;
    }
}

void CN64Disk::ClearDiskSettingID()
{
    g_Settings->SaveString(Game_GameName, "");
    g_Settings->SaveString(Game_IniKey, "");
}

bool CN64Disk::AllocateDiskImage(uint32_t DiskFileSize)
{
    WriteTrace(TraceN64System, TraceDebug, "Allocating memory for disk");
    std::unique_ptr<uint8_t> ImageBase(new uint8_t[DiskFileSize + 0x1000]);
    if (ImageBase.get() == NULL)
    {
        SetError(MSG_MEM_ALLOC_ERROR);
        WriteTrace(TraceN64System, TraceError, "Failed to allocate memory for disk (size: 0x%X)", DiskFileSize);
        return false;
    }
    uint8_t * Image = (uint8_t *)(((uint64_t)ImageBase.get() + 0xFFF) & ~0xFFF); // start at begining of memory page
    WriteTrace(TraceN64System, TraceDebug, "Allocated disk memory (%p)", Image);

    //save information about the disk loaded
    m_DiskImageBase = ImageBase.release();
    m_DiskImage = Image;
    m_DiskFileSize = DiskFileSize;
    return true;
}

bool CN64Disk::AllocateDiskHeader()
{
    WriteTrace(TraceN64System, TraceDebug, "Allocating memory for disk header forge");
    std::unique_ptr<uint8_t> HeaderBase(new uint8_t[0x40 + 0x1000]);
    if (HeaderBase.get() == NULL)
    {
        SetError(MSG_MEM_ALLOC_ERROR);
        WriteTrace(TraceN64System, TraceError, "Failed to allocate memory for disk header forge (size: 0x40)");
        return false;
    }
    uint8_t * Header = (uint8_t *)(((uint64_t)HeaderBase.get() + 0xFFF) & ~0xFFF); // start at begining of memory page
    WriteTrace(TraceN64System, TraceDebug, "Allocated disk memory (%p)", Header);

    //save information about the disk loaded
    m_DiskHeaderBase = HeaderBase.release();
    m_DiskHeader = Header;
    return true;
}

bool CN64Disk::AllocateAndLoadDiskImage(const char * FileLoc)
{
    WriteTrace(TraceN64System, TraceDebug, "Trying to open %s", FileLoc);
    if (!m_DiskFile.Open(FileLoc, CFileBase::modeRead))
    {
        WriteTrace(TraceN64System, TraceError, "Failed to open %s", FileLoc);
        return false;
    }

    //Make sure it is a valid disk image
    uint8_t Test[0x100];
    bool isValidDisk = false;

    const uint8_t blocks[8] = { 0, 1, 2, 3, 8, 9, 10, 11 };
    for (int i = 0; i < 8; i++)
    {
        m_DiskFile.Seek(0x4D08 * blocks[i], CFileBase::SeekPosition::begin);
        if (m_DiskFile.Read(Test, sizeof(Test)) != sizeof(Test))
        {
            m_DiskFile.Close();
            WriteTrace(TraceN64System, TraceError, "Failed to read ident bytes");
            return false;
        }

        isValidDisk = IsValidDiskImage(Test);
        if (isValidDisk)
            break;
    }
    if (!isValidDisk)
    {
        m_DiskFile.Close();
        WriteTrace(TraceN64System, TraceError, "invalid disk image file");
        return false;
    }
    uint32_t DiskFileSize = m_DiskFile.GetLength();
    stdstr ext = CPath(FileLoc).GetExtension();
    WriteTrace(TraceN64System, TraceDebug, "Successfully Opened, size: 0x%X", DiskFileSize);

    //Check Disk File Format
    if (((DiskFileSize == MameFormatSize) || (DiskFileSize == SDKFormatSize)) && (ext.compare("ndr") || ext.compare("ndd")))
    {
        if (DiskFileSize == MameFormatSize)
        {
            //If Disk is MAME Format (size is constant, it should be the same for every file), then continue
            m_DiskFormat = DiskFormatMAME;
            WriteTrace(TraceN64System, TraceDebug, "Disk File is MAME Format");
        }
        else
        {
            //If Disk is SDK format (made with SDK based dumpers like LuigiBlood's, or Nintendo's, size is also constant)
            m_DiskFormat = DiskFormatSDK;
            WriteTrace(TraceN64System, TraceDebug, "Disk File is SDK Format");
        }

        if (!AllocateDiskImage(DiskFileSize))
        {
            m_DiskFile.Close();
            return false;
        }

        //Load the n64 disk to the allocated memory
        g_Notify->DisplayMessage(5, MSG_LOADING);
        m_DiskFile.SeekToBegin();

        uint32_t count, TotalRead = 0;
        for (count = 0; count < (int)DiskFileSize; count += ReadFromRomSection)
        {
            uint32_t dwToRead = DiskFileSize - count;
            if (dwToRead > ReadFromRomSection) { dwToRead = ReadFromRomSection; }

            if (m_DiskFile.Read(&m_DiskImage[count], dwToRead) != dwToRead)
            {
                m_DiskFile.Close();
                SetError(MSG_FAIL_IMAGE);
                WriteTrace(TraceN64System, TraceError, "Failed to read file (TotalRead: 0x%X)", TotalRead);
                return false;
            }
            TotalRead += dwToRead;

            //Show Message of how much % wise of the rom has been loaded
            g_Notify->DisplayMessage(0, stdstr_f("%s: %.2f%c", GS(MSG_LOADED), ((float)TotalRead / (float)DiskFileSize) * 100.0f, '%').c_str());
        }

        if (DiskFileSize != TotalRead)
        {
            m_DiskFile.Close();
            SetError(MSG_FAIL_IMAGE);
            WriteTrace(TraceN64System, TraceError, "Expected to read: 0x%X, read: 0x%X", TotalRead, DiskFileSize);
            return false;
        }

        DetectSystemArea();

        g_Notify->DisplayMessage(5, MSG_BYTESWAP);
        ByteSwapDisk();
    }
    else if ((DiskFileSize > 0x4F08) && (ext.compare("d6r") || ext.compare("d64")))
    {
        m_DiskFormat = DiskFormatD64;
        WriteTrace(TraceN64System, TraceDebug, "Disk File is D64 Format");

        m_DiskType = Test[5];
        uint16_t ROM_LBA_END = (Test[0xE0] << 8) | Test[0xE1];
        uint16_t RAM_LBA_START = (Test[0xE2] << 8) | Test[0xE3];
        uint16_t RAM_LBA_END = (Test[0xE4] << 8) | Test[0xE5];

        if ((ROM_LBA_END + SYSTEM_LBAS) >= RAM_START_LBA[m_DiskType] ||
            ((RAM_LBA_START + SYSTEM_LBAS) != RAM_START_LBA[m_DiskType] && RAM_LBA_START != 0xFFFF))
        {
            m_DiskFile.Close();
            SetError(MSG_FAIL_IMAGE);
            WriteTrace(TraceN64System, TraceError, "Malformed D64 disk image");
            return false;
        }

        uint32_t ROM_SIZE = LBAToByte(SYSTEM_LBAS, ROM_LBA_END + 1);
        uint32_t RAM_SIZE = 0;
        if (RAM_LBA_START != 0xFFFF && RAM_LBA_END != 0xFFFF)
            RAM_SIZE = LBAToByte(SYSTEM_LBAS + RAM_LBA_START, RAM_LBA_END + 1 - RAM_LBA_START);
        uint32_t FULL_RAM_SIZE = RAM_SIZES[m_DiskType];

        if ((0x200 + ROM_SIZE + RAM_SIZE) != DiskFileSize)
        {
            m_DiskFile.Close();
            SetError(MSG_FAIL_IMAGE);
            WriteTrace(TraceN64System, TraceError, "Malformed D64 disk image, expected filesize of 0x200 + 0x%X + 0x%X = %08X, actual filesize: %08X", ROM_SIZE, RAM_SIZE, (0x200 + ROM_SIZE + RAM_SIZE), DiskFileSize);
            return false;
        }

        //Allocate File with Max RAM Area size
        WriteTrace(TraceN64System, TraceError, "Allocate D64 ROM %08X + RAM %08X", ROM_SIZE, FULL_RAM_SIZE);
        if (!AllocateDiskImage(0x200 + ROM_SIZE + FULL_RAM_SIZE))
        {
            m_DiskFile.Close();
            return false;
        }

        //Load the n64 disk to the allocated memory
        g_Notify->DisplayMessage(5, MSG_LOADING);
        m_DiskFile.SeekToBegin();

        uint32_t count, TotalRead = 0;
        for (count = 0; count < (int)DiskFileSize; count += ReadFromRomSection)
        {
            uint32_t dwToRead = DiskFileSize - count;
            if (dwToRead > ReadFromRomSection) { dwToRead = ReadFromRomSection; }

            if (m_DiskFile.Read(&m_DiskImage[count], dwToRead) != dwToRead)
            {
                m_DiskFile.Close();
                SetError(MSG_FAIL_IMAGE);
                WriteTrace(TraceN64System, TraceError, "Failed to read file (TotalRead: 0x%X)", TotalRead);
                return false;
            }
            TotalRead += dwToRead;

            //Show Message of how much % wise of the rom has been loaded
            g_Notify->DisplayMessage(0, stdstr_f("%s: %.2f%c", GS(MSG_LOADED), ((float)TotalRead / (float)DiskFileSize) * 100.0f, '%').c_str());
        }

        if (DiskFileSize != TotalRead)
        {
            m_DiskFile.Close();
            SetError(MSG_FAIL_IMAGE);
            WriteTrace(TraceN64System, TraceError, "Expected to read: 0x%X, read: 0x%X", TotalRead, DiskFileSize);
            return false;
        }

        DetectSystemArea();

        g_Notify->DisplayMessage(5, MSG_BYTESWAP);
        ForceByteSwapDisk();
    }
    else
    {
        //Else the disk file is invalid
        m_DiskFile.Close();
        WriteTrace(TraceN64System, TraceError, "Disk File is invalid, unexpected size");
        return false;
    }

    ProtectMemory(m_DiskImage, m_DiskFileSize, MEM_READWRITE);

    AllocateDiskHeader();
    memcpy(m_DiskHeader, GetDiskAddressSys(), 0x20);
    memcpy(m_DiskHeader + 0x20, GetDiskAddressID(), 0x20);
    memcpy(m_DiskHeader + 0x3B, GetDiskAddressID(), 5);
    return true;
}

bool CN64Disk::LoadDiskRAMImage()
{
    if (g_Settings->LoadDword(Setting_DiskSaveType) == DISKSAVE_SHADOW || m_DiskFormat == DiskFormatMAME ||
        m_isShadowDisk || m_DiskFileSize <= m_DiskRamAddress || m_DiskRamAddress == 0)
    {
        return true;
    }

    CFile ramfile;
    stdstr filename = m_FileName;

    filename[filename.length() - 1] = 'm';
    filename[filename.length() - 2] = 'a';
    filename[filename.length() - 3] = 'r';

    WriteTrace(TraceN64System, TraceDebug, "Trying to open %s", filename.c_str());
    if (!ramfile.Open(filename.c_str(), CFileBase::modeRead))
    {
        WriteTrace(TraceN64System, TraceError, "Failed to open %s", filename.c_str());
        return false;
    }

    if (ramfile.GetLength() != m_DiskFileSize - m_DiskRamAddress)
    {
        ramfile.Close();
        WriteTrace(TraceN64System, TraceError, "RAM save file is the wrong size");
        return false;
    }

    ForceByteSwapDisk();
    ramfile.SeekToBegin();
    if (ramfile.Read(GetDiskAddressRam(), m_DiskFileSize - m_DiskRamAddress) != (m_DiskFileSize - m_DiskRamAddress))
    {
        ramfile.Close();
        WriteTrace(TraceN64System, TraceError, "Failed to read RAM save data");
        return false;
    }

    ForceByteSwapDisk();
    return true;
}

void CN64Disk::ByteSwapDisk()
{
    uint32_t count;

    switch (*((uint32_t *)&GetDiskAddressSys()[8]))
    {
    case 0x281E140A:
    case 0x3024180C:
        for (count = 0; count < m_DiskFileSize; count += 4)
        {
            m_DiskImage[count] ^= m_DiskImage[count + 3];
            m_DiskImage[count + 3] ^= m_DiskImage[count];
            m_DiskImage[count] ^= m_DiskImage[count + 3];
            m_DiskImage[count + 1] ^= m_DiskImage[count + 2];
            m_DiskImage[count + 2] ^= m_DiskImage[count + 1];
            m_DiskImage[count + 1] ^= m_DiskImage[count + 2];
        }
        break;
    case 0x0A141E28: break;
    case 0x0C182430: break;
    default:
        g_Notify->DisplayError(stdstr_f("ByteSwapDisk: %08X - %08X", *((uint32_t *)&GetDiskAddressSys()[8]), m_DiskSysAddress).c_str());
    }
}

void CN64Disk::ForceByteSwapDisk()
{
    uint32_t count;

    for (count = 0; count < m_DiskFileSize; count += 4)
    {
        m_DiskImage[count] ^= m_DiskImage[count + 3];
        m_DiskImage[count + 3] ^= m_DiskImage[count];
        m_DiskImage[count] ^= m_DiskImage[count + 3];
        m_DiskImage[count + 1] ^= m_DiskImage[count + 2];
        m_DiskImage[count + 2] ^= m_DiskImage[count + 1];
        m_DiskImage[count + 1] ^= m_DiskImage[count + 2];
    }
}

void CN64Disk::SetError(LanguageStringID ErrorMsg)
{
    m_ErrorMsg = ErrorMsg;
}

void CN64Disk::UnallocateDiskImage()
{
    m_DiskFile.Close();

    if (m_DiskHeaderBase)
    {
        ProtectMemory(m_DiskHeader, 0x40, MEM_READWRITE);
        delete[] m_DiskHeaderBase;
        m_DiskHeaderBase = NULL;
    }
    m_DiskHeader = NULL;

    if (m_DiskImageBase)
    {
        ProtectMemory(m_DiskImage, m_DiskFileSize, MEM_READWRITE);
        delete[] m_DiskImageBase;
        m_DiskImageBase = NULL;
    }
    m_DiskImage = NULL;
}

uint32_t CN64Disk::CalculateCrc()
{
    //Custom CRC
    int crc = 0;
    for (int i = 0; i < 0x200; i += 4)
    {
        crc += *(uint32_t*)(&GetDiskAddressRom()[i]);
    }
    return crc;
}

uint32_t CN64Disk::GetDiskAddressBlock(uint16_t head, uint16_t track, uint16_t block, uint16_t sector, uint16_t sectorsize)
{
    uint32_t offset = 0;
    if (m_DiskFormat == DiskFormatMAME)
    {
        //MAME
        uint32_t tr_off = 0;
        uint16_t dd_zone = 0;

        if (track >= 0x425)
        {
            dd_zone = 7 + head;
            tr_off = track - 0x425;
        }
        else if (track >= 0x390)
        {
            dd_zone = 6 + head;
            tr_off = track - 0x390;
        }
        else if (track >= 0x2FB)
        {
            dd_zone = 5 + head;
            tr_off = track - 0x2FB;
        }
        else if (track >= 0x266)
        {
            dd_zone = 4 + head;
            tr_off = track - 0x266;
        }
        else if (track >= 0x1D1)
        {
            dd_zone = 3 + head;
            tr_off = track - 0x1D1;
        }
        else if (track >= 0x13C)
        {
            dd_zone = 2 + head;
            tr_off = track - 0x13C;
        }
        else if (track >= 0x9E)
        {
            dd_zone = 1 + head;
            tr_off = track - 0x9E;
        }
        else
        {
            dd_zone = 0 + head;
            tr_off = track;
        }

        offset = MAMEStartOffset[dd_zone] + tr_off * TRACKSIZE(dd_zone) + block * BLOCKSIZE(dd_zone) + sector * sectorsize;

        if (offset < (BLOCKSIZE(0) * SYSTEM_LBAS) && sector == 0)
        {
            uint16_t AddressBlock = (uint16_t)(offset / (BLOCKSIZE(0)));
            uint16_t block_sys = (uint16_t)(m_DiskSysAddress / (BLOCKSIZE(0)));
            uint16_t block_id = (uint16_t)(m_DiskIDAddress / (BLOCKSIZE(0)));

            if (AddressBlock < 12 && AddressBlock != block_sys)
            {
                offset = 0xFFFFFFFF;
            }
            else if (AddressBlock > 12 && AddressBlock < 16 && AddressBlock != block_id)
            {
                offset = 0xFFFFFFFF;
            }
        }
    }
    else if (m_DiskFormat == DiskFormatSDK)
    {
        //SDK
        offset = LBAToByte(0, PhysToLBA(head, track, block)) + sector * sectorsize;

        if (offset < (BLOCKSIZE(0) * SYSTEM_LBAS) && sector == 0)
        {
            uint16_t AddressBlock = (uint16_t)(offset / (BLOCKSIZE(0)));
            uint16_t block_sys = (uint16_t)(m_DiskSysAddress / (BLOCKSIZE(0)));
            uint16_t block_id = (uint16_t)(m_DiskIDAddress / (BLOCKSIZE(0)));

            if (AddressBlock < 12 && AddressBlock != block_sys)
            {
                offset = 0xFFFFFFFF;
            }
            else if (AddressBlock > 12 && AddressBlock < 16 && AddressBlock != block_id)
            {
                offset = 0xFFFFFFFF;
            }
        }
    }
    else
    {
        //D64
        uint16_t ROM_LBA_END = *(uint16_t*)(&GetDiskAddressSys()[0xE2]);
        uint16_t RAM_LBA_START = *(uint16_t*)(&GetDiskAddressSys()[0xE0]);
        uint16_t RAM_LBA_END = *(uint16_t*)(&GetDiskAddressSys()[0xE6]);
        uint16_t LBA = PhysToLBA(head, track, block);
        if (LBA < DISKID_LBA)
        {
            offset = m_DiskSysAddress;
        }
        else if ((LBA >= DISKID_LBA) && (LBA < SYSTEM_LBAS))
        {
            offset = m_DiskIDAddress;
        }
        else if (LBA <= (ROM_LBA_END + SYSTEM_LBAS))
        {
            offset = 0x200 + LBAToByte(SYSTEM_LBAS, LBA - SYSTEM_LBAS) + (sector * sectorsize);
        }
        else if (((LBA - SYSTEM_LBAS) <= RAM_LBA_END) && ((LBA - SYSTEM_LBAS) >= RAM_LBA_START))
        {
            offset = 0x200 + LBAToByte(SYSTEM_LBAS, ROM_LBA_END + 1);
            offset += LBAToByte(RAM_LBA_START + SYSTEM_LBAS, LBA - RAM_LBA_START - SYSTEM_LBAS) + (sector * sectorsize);
        }
        else
        {
            offset = 0xFFFFFFFF;
        }
    }

    if (offset >= m_DiskFileSize)
    {
        offset = 0xFFFFFFFF;
    }
    if (sector == 0)
    {
        WriteTrace(TraceN64System, TraceDebug, "Head %d Track %d Block %d - LBA %d - Address %08X", head, track, block, PhysToLBA(head, track, block), offset);
    }
    return offset;
}

void CN64Disk::DetectSystemArea()
{
    if ((m_DiskFormat == DiskFormatMAME) || (m_DiskFormat == DiskFormatSDK))
    {
        //MAME / SDK (System Area can be handled identically)
        m_DiskSysAddress = 0;
        m_DiskIDAddress = DISKID_LBA * 0x4D08;
        m_DiskRomAddress = SYSTEM_LBAS * 0x4D08;

        //Handle System Data
        const uint16_t sysblocks[4] = { 9, 8, 1, 0 };
        //Check if Disk is development disk
        bool isDevDisk = false;

        for (int i = 0; i < 4; i++)
        {
            if (IsSysSectorGood(sysblocks[i] + 2, 0xC0))
            {
                m_DiskSysAddress = ((sysblocks[i] + 2) * 0x4D08);
                isDevDisk = true;
            }
        }

        if (!isDevDisk)
        {
            for (int i = 0; i < 4; i++)
            {
                if (IsSysSectorGood(sysblocks[i], 0xE8))
                {
                    m_DiskSysAddress = (sysblocks[i] * 0x4D08);
                }
            }
        }

        //Handle Disk ID
        for (int i = 2; i > 0; i--)
        {
            //There are two Disk ID Blocks
            if (IsSysSectorGood(DISKID_LBA + i, 0xE8))
            {
                m_DiskIDAddress = ((DISKID_LBA + i) * 0x4D08);
            }
        }
    }
    else //if (m_DiskFormat == DiskFormatD64)
    {
        //D64 (uses fixed addresses)
        m_DiskSysAddress = 0x000;
        m_DiskIDAddress = 0x100;
        m_DiskRomAddress = 0x200;
    }
}

bool CN64Disk::IsSysSectorGood(uint32_t block, uint32_t sectorsize)
{
    //Checks if all sectors are identical (meant only to be used for System Area for MAME and SDK formats)
    for (int j = 1; j < SECTORS_PER_BLOCK; j++)
    {
        for (uint32_t k = 0; k < sectorsize; k++)
        {
            if (m_DiskImage[(block * 0x4D08) + (j * sectorsize) + k] != m_DiskImage[(block * 0x4D08) + k])
            {
                return false;
            }
        }
    }

    if (block < DISKID_LBA)
    {
        //Check System Data

        //System Format
        if (m_DiskImage[(block * 0x4D08) + 4] != 0x10)
            return false;

        //Disk Format
        if ((m_DiskImage[(block * 0x4D08) + 5] & 0xF0) != 0x10)
            return false;

        //Always 0xFFFFFFFF
        if (*(uint32_t*)&m_DiskImage[(block * 0x4D08) + 0x18] != 0xFFFFFFFF)
            return false;

        uint8_t alt = 0xC;  //Retail
        if ((block & 2) != 0)
            alt = 0xA;      //Development

        //Alternate Tracks Offsets (always the same)
        for (int i = 0; i < 16; i++)
        {
            if (m_DiskImage[(block * 0x4D08) + 8 + i] != ((i + 1) * alt))
                return false;
        }
    }

    return true;
}

Country CN64Disk::GetDiskCountryCode()
{
    switch (*(uint32_t*)&GetDiskAddressSys()[0])
    {
        case DISK_COUNTRY_JPN:
            return Country_Japan;
        case DISK_COUNTRY_USA:
            return Country_NorthAmerica;
        case DISK_COUNTRY_DEV:
        default:
            return Country_Unknown;
    }
}

void CN64Disk::InitSysDataD64()
{
    //Else the disk will not work properly.
    if (m_DiskFormat != DiskFormatD64)
        return;

    GetDiskAddressSys()[4 ^ 3] = 0x10;
    GetDiskAddressSys()[5 ^ 3] |= 0x10;

    //Expand RAM Area for file format consistency
    if (m_DiskType < 6)
    {
        *(uint16_t*)&GetDiskAddressSys()[0xE2 ^ 2] = RAM_START_LBA[m_DiskType] - SYSTEM_LBAS;
        *(uint16_t*)&GetDiskAddressSys()[0xE4 ^ 2] = MAX_LBA - SYSTEM_LBAS;
    }
    else
    {
        *(uint16_t*)&GetDiskAddressSys()[0xE2 ^ 2] = 0xFFFF;
        *(uint16_t*)&GetDiskAddressSys()[0xE4 ^ 2] = 0xFFFF;
    }
}

void CN64Disk::DeinitSysDataD64()
{
    //Restore the data
    if (m_DiskFormat != DiskFormatD64)
        return;

    GetDiskAddressSys()[4^3] = 0x00;
    GetDiskAddressSys()[5^3] &= 0x0F;
}

void CN64Disk::GenerateLBAToPhysTable()
{
    for (uint32_t lba = 0; lba < SIZE_LBA; lba++)
    {
        LBAToPhysTable[lba] = LBAToPhys(lba);
    }
}

void CN64Disk::DetectRamAddress()
{
    if (m_DiskFormat == DiskFormatMAME)
    {
        //Not supported
        m_DiskRamAddress = 0;
    }
    else if (m_DiskFormat == DiskFormatSDK)
    {
        m_DiskRamAddress = LBAToByte(0, RAM_START_LBA[m_DiskType]);
    }
    else //if (m_DiskFormat == DiskFormatD64)
    {
        m_DiskRamAddress = m_DiskRomAddress + LBAToByte(SYSTEM_LBAS, *(uint16_t*)(&GetDiskAddressSys()[0xE0 ^ 2]) + 1);
    }
}

uint32_t CN64Disk::LBAToVZone(uint32_t lba)
{
    for (uint32_t vzone = 0; vzone < 16; vzone++) {
        if (lba < VZONE_LBA_TBL[m_DiskType][vzone]) {
            return vzone;
        }
    }
    return 0;
};

uint32_t CN64Disk::LBAToByte(uint32_t lba, uint32_t nlbas)
{
    bool init_flag = true;
    uint32_t totalbytes = 0;
    uint32_t blocksize = 0;
    uint32_t vzone = 0, pzone = 0;
    if (nlbas != 0)
    {
        for (; nlbas != 0; nlbas--)
        {
            if ((init_flag == true) || (VZONE_LBA_TBL[m_DiskType][vzone] == lba))
            {
                vzone = LBAToVZone(lba);
                pzone = VZoneToPZone(vzone, m_DiskType);
                if (7 < pzone)
                {
                    pzone -= 7;
                }
                blocksize = SECTORSIZE_P[pzone] * SECTORS_PER_BLOCK;
            }

            totalbytes += blocksize;
            lba++;
            init_flag = false;
            if (((nlbas - 1) != 0) && (lba > MAX_LBA))
            {
                return 0xFFFFFFFF;
            }
        }
    }

    return totalbytes;
}

uint16_t CN64Disk::LBAToPhys(uint32_t lba)
{
    uint8_t * sys_data = GetDiskAddressSys();

    //Get Block 0/1 on Disk Track
    uint8_t block = 1;
    if (((lba & 3) == 0) || ((lba & 3) == 3))
        block = 0;

    //Get Virtual & Physical Disk Zones
    uint16_t vzone = (uint16_t)LBAToVZone(lba);
    uint16_t pzone = VZoneToPZone(vzone, m_DiskType);

    //Get Disk Head
    uint16_t head = (7 < pzone);

    //Get Disk Zone
    uint16_t disk_zone = pzone;
    if (disk_zone != 0)
        disk_zone = pzone - 7;

    //Get Virtual Zone LBA start, if Zone 0, it's LBA 0
    uint16_t vzone_lba = 0;
    if (vzone != 0)
        vzone_lba = VZONE_LBA_TBL[m_DiskType][vzone - 1];

    //Calculate Physical Track
    uint16_t track = (uint16_t)((lba - vzone_lba) >> 1);

    //Get the start track from current zone
    uint16_t track_zone_start = SCYL_ZONE_TBL[0][pzone];
    if (head != 0)
    {
        //If Head 1, count from the other way around
        track = -track;
        track_zone_start = OUTERCYL_TBL[disk_zone - 1];
    }
    track += SCYL_ZONE_TBL[0][pzone];

    //Get the relative offset to defect tracks for the current zone (if Zone 0, then it's 0)
    uint16_t defect_offset = 0;
    if (pzone != 0)
        defect_offset = sys_data[(8 + pzone - 1) ^ 3];

    //Get amount of defect tracks for the current zone
    uint16_t defect_amount = sys_data[(8 + pzone) ^ 3] - defect_offset;

    //Skip defect tracks
    while ((defect_amount != 0) && ((sys_data[(0x20 + defect_offset) ^ 3] + track_zone_start) <= track))
    {
        track++;
        defect_offset++;
        defect_amount--;
    }

    return track | (head * 0x1000) | (block * 0x2000);
}

uint16_t CN64Disk::PhysToLBA(uint16_t head, uint16_t track, uint16_t block)
{
    uint16_t expectedvalue = track | (head * 0x1000) | (block * 0x2000);

    for (uint16_t lba = 0; lba < SIZE_LBA; lba++)
    {
        if (LBAToPhysTable[lba] == expectedvalue)
        {
            return lba;
        }
    }
    return 0xFFFF;
}
