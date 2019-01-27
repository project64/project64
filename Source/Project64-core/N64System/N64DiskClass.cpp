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
    uint8_t disktype = m_DiskImage[5] & 0xF;
    if (disktype == 0x6)
    {
        m_DiskFile.Close();
        WriteTrace(TraceN64System, TraceDebug, "Loaded Disk Type is 0x7. No RAM area. Shadow file is not needed.");
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
        if (!m_DiskFile.Write(m_DiskImage, MameFormatSize))
        {
            m_DiskFile.Close();
            WriteTrace(TraceN64System, TraceError, "Failed to write file");
            return false;
        }
    }
    else if (m_DiskFormat == DiskFormatSDK)
    {
        //If original file was SDK format, we need to convert it back
        WriteTrace(TraceN64System, TraceDebug, "64DD disk is SDK format");
        ConvertDiskFormatBack();
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
    if (DiskFileSize == MameFormatSize)
    {
        //If Disk is MAME Format (size is constant, it should be the same for every file), then continue
        m_DiskFormat = DiskFormatMAME;
        WriteTrace(TraceN64System, TraceDebug, "Disk File is MAME Format");

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
    else if (DiskFileSize == SDKFormatSize)
    {
        //If Disk is SDK format (made with SDK based dumpers like LuigiBlood's, or Nintendo's, size is also constant)
        //We need to convert it.
        m_DiskFormat = DiskFormatSDK;

        g_Notify->DisplayMessage(5, MSG_LOADING);

        //Allocate supported size
        if (!AllocateDiskImage(MameFormatSize))
        {
            m_DiskFile.Close();
            return false;
        }

        ConvertDiskFormat();
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
    memcpy_s(m_DiskHeader, 0x20, m_DiskImage, 0x20);
    memcpy_s(m_DiskHeader + 0x20, 0x20, m_DiskImage + 0x43670, 0x20);
    memcpy_s(m_DiskHeader + 0x3B, 5, m_DiskImage + 0x43670, 5);
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

void CN64Disk::ConvertDiskFormat()
{
    //Original code by Happy_
    m_DiskFile.SeekToBegin();

    const uint32_t ZoneSecSize[16] = { 232, 216, 208, 192, 176, 160, 144, 128,
        216, 208, 192, 176, 160, 144, 128, 112 };
    const uint32_t ZoneTracks[16] = { 158, 158, 149, 149, 149, 149, 149, 114,
        158, 158, 149, 149, 149, 149, 149, 114 };
    const uint32_t DiskTypeZones[7][16] = {
        { 0, 1, 2, 9, 8, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10 },
        { 0, 1, 2, 3, 10, 9, 8, 4, 5, 6, 7, 15, 14, 13, 12, 11 },
        { 0, 1, 2, 3, 4, 11, 10, 9, 8, 5, 6, 7, 15, 14, 13, 12 },
        { 0, 1, 2, 3, 4, 5, 12, 11, 10, 9, 8, 6, 7, 15, 14, 13 },
        { 0, 1, 2, 3, 4, 5, 6, 13, 12, 11, 10, 9, 8, 7, 15, 14 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 14, 13, 12, 11, 10, 9, 8, 15 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8 }
    };
    const uint32_t RevDiskTypeZones[7][16] = {
        { 0, 1, 2, 5, 6, 7, 8, 9, 4, 3, 15, 14, 13, 12, 11, 10 },
        { 0, 1, 2, 3, 7, 8, 9, 10, 6, 5, 4, 15, 14, 13, 12, 11 },
        { 0, 1, 2, 3, 4, 9, 10, 11, 8, 7, 6, 5, 15, 14, 13, 12 },
        { 0, 1, 2, 3, 4, 5, 11, 12, 10, 9, 8, 7, 6, 15, 14, 13 },
        { 0, 1, 2, 3, 4, 5, 6, 13, 12, 11, 10, 9, 8, 7, 15, 14 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 14, 13, 12, 11, 10, 9, 8, 15 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8 }
    };
    const uint32_t StartBlock[7][16] = {
        { 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1 },
        { 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1 },
        { 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1 }
    };

    uint32_t disktype = 0;
    uint32_t zone, track = 0;
    int32_t atrack = 0;
    int32_t block = 0;
    uint8_t SystemData[0xE8];
    uint8_t BlockData0[0x100 * SECTORS_PER_BLOCK];
    uint8_t BlockData1[0x100 * SECTORS_PER_BLOCK];
    uint32_t InOffset, OutOffset = 0;
    uint32_t InStart[16];
    uint32_t OutStart[16];

    InStart[0] = 0;
    OutStart[0] = 0;

    //Read System Area
    m_DiskFile.Read(&SystemData, 0xE8);

    disktype = SystemData[5] & 0xF;

    //Prepare Input Offsets
    for (zone = 1; zone < 16; zone++)
    {
        InStart[zone] = InStart[zone - 1] +
            VZONESIZE(DiskTypeZones[disktype][zone - 1]);
    }

    //Prepare Output Offsets
    for (zone = 1; zone < 16; zone++)
    {
        OutStart[zone] = OutStart[zone - 1] + ZONESIZE(zone - 1);
    }

    //Copy Head 0
    for (zone = 0; zone < 8; zone++)
    {
        OutOffset = OutStart[zone];
        InOffset = InStart[RevDiskTypeZones[disktype][zone]];
        m_DiskFile.Seek(InOffset, CFileBase::begin);
        block = StartBlock[disktype][zone];
        atrack = 0;
        for (track = 0; track < ZoneTracks[zone]; track++)
        {
            if (atrack < 0xC && track == SystemData[0x20 + zone * 0xC + atrack])
            {
                memset((void *)(&BlockData0), 0, BLOCKSIZE(zone));
                memset((void *)(&BlockData1), 0, BLOCKSIZE(zone));
                atrack += 1;
            }
            else
            {
                if ((block % 2) == 1)
                {
                    m_DiskFile.Read(&BlockData1, BLOCKSIZE(zone));
                    m_DiskFile.Read(&BlockData0, BLOCKSIZE(zone));
                }
                else
                {
                    m_DiskFile.Read(&BlockData0, BLOCKSIZE(zone));
                    m_DiskFile.Read(&BlockData1, BLOCKSIZE(zone));
                }
                block = 1 - block;
            }
            memcpy(m_DiskImage + OutOffset, &BlockData0, BLOCKSIZE(zone));
            OutOffset += BLOCKSIZE(zone);
            memcpy(m_DiskImage + OutOffset, &BlockData1, BLOCKSIZE(zone));
            OutOffset += BLOCKSIZE(zone);
        }
    }

    //Copy Head 1
    for (zone = 8; zone < 16; zone++)
    {
        //OutOffset = OutStart[zone];
        InOffset = InStart[RevDiskTypeZones[disktype][zone]];
        m_DiskFile.Seek(InOffset, CFileBase::begin);
        block = StartBlock[disktype][zone];
        atrack = 0xB;
        for (track = 1; track < ZoneTracks[zone] + 1; track++)
        {
            if (atrack > -1 && (ZoneTracks[zone] - track) == SystemData[0x20 + (zone)* 0xC + atrack])
            {
                memset((void *)(&BlockData0), 0, BLOCKSIZE(zone));
                memset((void *)(&BlockData1), 0, BLOCKSIZE(zone));
                atrack -= 1;
            }
            else
            {
                if ((block % 2) == 1)
                {
                    m_DiskFile.Read(&BlockData1, BLOCKSIZE(zone));
                    m_DiskFile.Read(&BlockData0, BLOCKSIZE(zone));
                }
                else
                {
                    m_DiskFile.Read(&BlockData0, BLOCKSIZE(zone));
                    m_DiskFile.Read(&BlockData1, BLOCKSIZE(zone));
                }
                block = 1 - block;
            }
            OutOffset = OutStart[zone] + (ZoneTracks[zone] - track) * TRACKSIZE(zone);
            memcpy(m_DiskImage + OutOffset, &BlockData0, BLOCKSIZE(zone));
            OutOffset += BLOCKSIZE(zone);
            memcpy(m_DiskImage + OutOffset, &BlockData1, BLOCKSIZE(zone));
            OutOffset += BLOCKSIZE(zone);
        }
    }
}

void CN64Disk::ConvertDiskFormatBack()
{
    //Original code by Happy_
    const uint32_t ZoneSecSize[16] = { 232, 216, 208, 192, 176, 160, 144, 128,
        216, 208, 192, 176, 160, 144, 128, 112 };
    const uint32_t ZoneTracks[16] = { 158, 158, 149, 149, 149, 149, 149, 114,
        158, 158, 149, 149, 149, 149, 149, 114 };
    const uint32_t DiskTypeZones[7][16] = {
        { 0, 1, 2, 9, 8, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10 },
        { 0, 1, 2, 3, 10, 9, 8, 4, 5, 6, 7, 15, 14, 13, 12, 11 },
        { 0, 1, 2, 3, 4, 11, 10, 9, 8, 5, 6, 7, 15, 14, 13, 12 },
        { 0, 1, 2, 3, 4, 5, 12, 11, 10, 9, 8, 6, 7, 15, 14, 13 },
        { 0, 1, 2, 3, 4, 5, 6, 13, 12, 11, 10, 9, 8, 7, 15, 14 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 14, 13, 12, 11, 10, 9, 8, 15 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8 }
    };
    const uint32_t RevDiskTypeZones[7][16] = {
        { 0, 1, 2, 5, 6, 7, 8, 9, 4, 3, 15, 14, 13, 12, 11, 10 },
        { 0, 1, 2, 3, 7, 8, 9, 10, 6, 5, 4, 15, 14, 13, 12, 11 },
        { 0, 1, 2, 3, 4, 9, 10, 11, 8, 7, 6, 5, 15, 14, 13, 12 },
        { 0, 1, 2, 3, 4, 5, 11, 12, 10, 9, 8, 7, 6, 15, 14, 13 },
        { 0, 1, 2, 3, 4, 5, 6, 13, 12, 11, 10, 9, 8, 7, 15, 14 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 14, 13, 12, 11, 10, 9, 8, 15 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8 }
    };
    const uint32_t StartBlock[7][16] = {
        { 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1 },
        { 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1 },
        { 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 },
        { 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1 }
    };

    uint32_t disktype = 0;
    uint32_t zone, track = 0;
    int32_t atrack = 0;
    int32_t block = 0;
    uint8_t SystemData[0xE8];
    uint8_t BlockData0[0x100 * SECTORS_PER_BLOCK];
    uint8_t BlockData1[0x100 * SECTORS_PER_BLOCK];
    uint32_t InOffset, OutOffset = 0;
    uint32_t InStart[16];
    uint32_t OutStart[16];

    //SDK DISK RAM
    WriteTrace(TraceN64System, TraceDebug, "Allocating memory for disk SDK format");
    AUTO_PTR<uint8_t> ImageBase(new uint8_t[SDKFormatSize + 0x1000]);
    if (ImageBase.get() == NULL)
    {
        SetError(MSG_MEM_ALLOC_ERROR);
        WriteTrace(TraceN64System, TraceError, "Failed to allocate memory for disk SDK format (size: 0x%X)", SDKFormatSize);
        return;
    }
    uint8_t * Image = (uint8_t *)(((uint64_t)ImageBase.get() + 0xFFF) & ~0xFFF); // start at begining of memory page
    WriteTrace(TraceN64System, TraceDebug, "Allocated disk SDK format memory (%p)", Image);

    //save information about the disk loaded
    uint8_t * s_DiskImageBase = ImageBase.release();
    uint8_t * s_DiskImage = Image;
    //END

    InStart[0] = 0;
    OutStart[0] = 0;

    //Read System Area
    memcpy(&SystemData, m_DiskImage, 0xE8);

    disktype = SystemData[5] & 0xF;

    //Prepare Input Offsets
    for (zone = 1; zone < 16; zone++)
    {
        InStart[zone] = InStart[zone - 1] +
            VZONESIZE(DiskTypeZones[disktype][zone - 1]);
    }

    //Prepare Output Offsets
    for (zone = 1; zone < 16; zone++)
    {
        OutStart[zone] = OutStart[zone - 1] + ZONESIZE(zone - 1);
    }

    //Copy Head 0
    for (zone = 0; zone < 8; zone++)
    {
        block = StartBlock[disktype][zone];
        atrack = 0;
        for (track = 0; track < ZoneTracks[zone]; track++)
        {
            InOffset = OutStart[zone] + (track)* TRACKSIZE(zone);
            OutOffset = InStart[RevDiskTypeZones[disktype][zone]] + (track - atrack) * TRACKSIZE(zone);

            if (atrack < 0xC && track == SystemData[0x20 + zone * 0xC + atrack])
            {
                atrack += 1;
            }
            else
            {
                if ((block % 2) == 1)
                {
                    memcpy(&BlockData1, m_DiskImage + InOffset, BLOCKSIZE(zone));
                    InOffset += BLOCKSIZE(zone);
                    memcpy(&BlockData0, m_DiskImage + InOffset, BLOCKSIZE(zone));
                    InOffset += BLOCKSIZE(zone);
                }
                else
                {
                    memcpy(&BlockData0, m_DiskImage + InOffset, BLOCKSIZE(zone));
                    InOffset += BLOCKSIZE(zone);
                    memcpy(&BlockData1, m_DiskImage + InOffset, BLOCKSIZE(zone));
                    InOffset += BLOCKSIZE(zone);
                }
                block = 1 - block;
                memcpy(s_DiskImage + OutOffset, &BlockData0, BLOCKSIZE(zone));
                OutOffset += BLOCKSIZE(zone);
                memcpy(s_DiskImage + OutOffset, &BlockData1, BLOCKSIZE(zone));
                OutOffset += BLOCKSIZE(zone);
            }
        }
    }

    //Copy Head 1
    for (zone = 8; zone < 16; zone++)
    {
        block = StartBlock[disktype][zone];
        atrack = 0xB;
        for (track = 1; track < ZoneTracks[zone] + 1; track++)
        {
            InOffset = OutStart[zone] + (ZoneTracks[zone] - track) * TRACKSIZE(zone);
            OutOffset = InStart[RevDiskTypeZones[disktype][zone]] + (track - (0xB - atrack) - 1) * TRACKSIZE(zone);

            if (atrack > -1 && (ZoneTracks[zone] - track) == SystemData[0x20 + (zone)* 0xC + atrack])
            {
                atrack -= 1;
            }
            else
            {
                if ((block % 2) == 1)
                {
                    memcpy(&BlockData1, m_DiskImage + InOffset, BLOCKSIZE(zone));
                    InOffset += BLOCKSIZE(zone);
                    memcpy(&BlockData0, m_DiskImage + InOffset, BLOCKSIZE(zone));
                    InOffset += BLOCKSIZE(zone);
                }
                else
                {
                    memcpy(&BlockData0, m_DiskImage + InOffset, BLOCKSIZE(zone));
                    InOffset += BLOCKSIZE(zone);
                    memcpy(&BlockData1, m_DiskImage + InOffset, BLOCKSIZE(zone));
                    InOffset += BLOCKSIZE(zone);
                }
                block = 1 - block;
                memcpy(s_DiskImage + OutOffset, &BlockData0, BLOCKSIZE(zone));
                OutOffset += BLOCKSIZE(zone);
                memcpy(s_DiskImage + OutOffset, &BlockData1, BLOCKSIZE(zone));
                OutOffset += BLOCKSIZE(zone);
            }
        }
    }

    if (!m_DiskFile.Write(s_DiskImage, SDKFormatSize))
    {
        m_DiskFile.Close();
        WriteTrace(TraceN64System, TraceError, "Failed to write file");
    }

    WriteTrace(TraceN64System, TraceDebug, "Unallocating disk SDK format memory");
    delete[] s_DiskImageBase;
    s_DiskImageBase = NULL;
    s_DiskImage = NULL;
}