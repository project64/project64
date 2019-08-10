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
#include "N64DiskClass.h"
#include "SystemGlobals.h"
#include <Common/Platform.h>
#include <Common/SmartPointer.h>
#include <Common/MemoryManagement.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <memory>

CN64Disk::CN64Disk() :
m_DiskImage(NULL),
m_DiskImageBase(NULL),
m_ErrorMsg(EMPTY_STRING),
m_DiskBufAddress(0)
{
}

CN64Disk::~CN64Disk()
{
}

bool CN64Disk::LoadDiskImage(const char * FileLoc)
{
    UnallocateDiskImage();

    //Assume the file extension is *.ndd (it is the only case where it is loaded)
    stdstr ShadowFile = FileLoc;
    ShadowFile[ShadowFile.length() - 1] = 'r';

    WriteTrace(TraceN64System, TraceDebug, "Attempt to load shadow file.");
    if (!AllocateAndLoadDiskImage(ShadowFile.c_str()))
    {
        WriteTrace(TraceN64System, TraceDebug, "Loading Shadow file failed");
        UnallocateDiskImage();
        if (!AllocateAndLoadDiskImage(FileLoc))
        {
            return false;
        }
    }

    char RomName[5];
    m_FileName = FileLoc;
    if (*(uint32_t *)(&m_DiskImage[0x43670]) != 0)
    {
        m_DiskIdent.Format("%08X-%08X-C:%X", *(uint32_t *)(&m_DiskImage[0]), *(uint32_t *)(&m_DiskImage[0x43670]), m_DiskImage[0x43670]);
        //Get the disk ID from the disk image
        RomName[0] = (char)*(m_DiskImage + 0x43673);
        RomName[1] = (char)*(m_DiskImage + 0x43672);
        RomName[2] = (char)*(m_DiskImage + 0x43671);
        RomName[3] = (char)*(m_DiskImage + 0x43670);
        RomName[4] = '\0';
    }
    else
    {
        uint32_t crc = 0;
        for (uint8_t i = 0; i < 0xE8; i += 4)
        {
            crc += *(uint32_t *)(m_DiskImage + i);
        }
        m_DiskIdent.Format("%08X-%08X-C:%X", *(uint32_t *)(&m_DiskImage[0]), crc, m_DiskImage[0x43670]);

        //Get the disk ID from the disk image
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
    m_Country = (Country)m_DiskImage[0x43670];
    m_DiskType = m_DiskImage[5 ^ 3] & 0x0F;
    GenerateLBAToPhysTable();

    if (g_Disk == this)
    {
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
        SaveDiskSettingID(false);
    }
    return true;
}

bool CN64Disk::SaveDiskImage()
{
    //NO NEED TO SAVE IF DISK TYPE IS 6
    if (m_DiskType == 6)
    {
        m_DiskFile.Close();
        WriteTrace(TraceN64System, TraceDebug, "Loaded Disk Type is 6. No RAM area. Shadow file is not needed.");
        return true;
    }

    //Assume the file extension is *.ndd (it is the only case where it is loaded)
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

    if (m_DiskFormat == DiskFormatMAME)
    {
        //If original file was MAME format, just copy
        WriteTrace(TraceN64System, TraceDebug, "64DD disk is MAME format");
    }
    else if (m_DiskFormat == DiskFormatSDK)
    {
        //If original file was SDK format, we need to convert it back
        WriteTrace(TraceN64System, TraceDebug, "64DD disk is SDK format");
    }

    if (!m_DiskFile.Write(m_DiskImage, m_DiskFileSize))
    {
        m_DiskFile.Close();
        WriteTrace(TraceN64System, TraceError, "Failed to write file");
        return false;
    }

    m_DiskFile.Close();
    return true;
}

void CN64Disk::SwapDiskImage(const char * FileLoc)
{
    g_Reg->ASIC_STATUS &= ~DD_STATUS_DISK_PRES;
    LoadDiskImage(FileLoc);
}

bool CN64Disk::IsValidDiskImage(uint8_t Test[4])
{
    if (*((uint32_t *)&Test[0]) == 0x16D348E8) { return true; }
    else if (*((uint32_t *)&Test[0]) == 0x56EE6322) { return true; }
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
    case Germany: case french:  case Italian:
    case Europe:  case Spanish: case Australia:
    case X_PAL:   case Y_PAL:
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
    AUTO_PTR<uint8_t> ImageBase(new uint8_t[DiskFileSize + 0x1000]);
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
    AUTO_PTR<uint8_t> HeaderBase(new uint8_t[0x40 + 0x1000]);
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

    //Read the first 4 bytes and make sure it is a valid disk image
    uint8_t Test[4];
    m_DiskFile.SeekToBegin();
    if (m_DiskFile.Read(Test, sizeof(Test)) != sizeof(Test))
    {
        m_DiskFile.Close();
        WriteTrace(TraceN64System, TraceError, "Failed to read ident bytes");
        return false;
    }
    if (!IsValidDiskImage(Test))
    {
        m_DiskFile.Close();
        WriteTrace(TraceN64System, TraceError, "invalid image file %X %X %X %X", Test[0], Test[1], Test[2], Test[3]);
        return false;
    }
    uint32_t DiskFileSize = m_DiskFile.GetLength();
    WriteTrace(TraceN64System, TraceDebug, "Successfully Opened, size: 0x%X", DiskFileSize);

    //Check Disk File Format
    if ((DiskFileSize == MameFormatSize) || (DiskFileSize == SDKFormatSize))
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
    }
    else
    {
        //Else the disk file is invalid
        m_DiskFile.Close();
        WriteTrace(TraceN64System, TraceError, "Disk File is invalid, unexpected size");
        return false;
    }

    g_Notify->DisplayMessage(5, MSG_BYTESWAP);
    ByteSwapDisk();

    ProtectMemory(m_DiskImage, m_DiskFileSize, MEM_READWRITE);

    AllocateDiskHeader();
    memcpy(m_DiskHeader, m_DiskImage, 0x20);
    memcpy(m_DiskHeader + 0x20, m_DiskImage + 0x43670, 0x20);
    memcpy(m_DiskHeader + 0x3B, m_DiskImage + 0x43670, 5);
    return true;
}

void CN64Disk::ByteSwapDisk()
{
    uint32_t count;

    switch (*((uint32_t *)&m_DiskImage[0]))
    {
    case 0x16D348E8:
    case 0x56EE6322:
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
    case 0xE848D316: break;
    case 0x2263EE56: break;
    default:
        g_Notify->DisplayError(stdstr_f("ByteSwapDisk: %X", m_DiskImage[0]).c_str());
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

uint32_t CN64Disk::GetDiskAddressBlock(uint16_t head, uint16_t track, uint16_t block)
{
    uint32_t offset = 0;
    if (m_DiskFormat == DiskFormatMAME)
    {
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

        offset = MAMEStartOffset[dd_zone] + tr_off * TRACKSIZE(dd_zone) + block * BLOCKSIZE(dd_zone);
    }
    else if (m_DiskFormat == DiskFormatSDK)
    {
        offset = LBAToByte(0, PhysToLBA(head, track, block));
    }
    //WriteTrace(TraceN64System, TraceDebug, "Head %d Track %d Block %d - LBA %d - Address %08X", head, track, block, PhysToLBA(head, track, block), offset);
    return offset;
}

void CN64Disk::GenerateLBAToPhysTable()
{
    for (uint32_t lba = 0; lba < SIZE_LBA; lba++)
    {
        LBAToPhysTable[lba] = LBAToPhys(lba);
    }
}

uint32_t CN64Disk::LBAToVZone(uint32_t lba)
{
    for (uint32_t vzone = 0; vzone < 16; vzone++) {
        if (lba < VZONE_LBA_TBL[m_DiskType][vzone]) {
            return vzone;
        }
    }
};

uint32_t CN64Disk::LBAToByte(uint32_t lba, uint32_t nlbas)
{
    bool init_flag = true;
    uint32_t totalbytes = 0;
    uint32_t blocksize = 0;
    uint32_t vzone, pzone = 0;
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
            if ((nlbas != 0) && (lba > MAX_LBA))
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
    uint16_t vzone = LBAToVZone(lba);
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
    uint16_t track = (lba - vzone_lba) >> 1;

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
