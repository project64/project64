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

#include <Project64-core/N64System/N64Types.h>
#include <Common/stdtypes.h>

class CN64Disk
{
public:
    CN64Disk();
    ~CN64Disk();

    bool    LoadDiskImage(const char * FileLoc);
    bool    SaveDiskImage();
    void    SwapDiskImage(const char * FileLoc);
    static bool IsValidDiskImage(uint8_t Test[4]);
    void    SaveDiskSettingID(bool temp);
    void    ClearDiskSettingID();
    uint8_t *  GetDiskAddress() { return m_DiskImage; }
    uint8_t *  GetDiskAddressBuffer() { return m_DiskImage + m_DiskBufAddress; }
    void    SetDiskAddressBuffer(uint32_t address) { m_DiskBufAddress = address; }
    stdstr  GetRomName() const { return m_RomName; }
    stdstr  GetFileName() const { return m_FileName; }
    stdstr  GetDiskIdent() const { return m_DiskIdent; }
    Country GetCountry() const { return m_Country; }
    void    UnallocateDiskImage();

    LanguageStringID GetError() const { return m_ErrorMsg; }

private:
    bool   AllocateDiskImage(uint32_t DiskFileSize);
    bool   AllocateAndLoadDiskImage(const char * FileLoc);
    void   ByteSwapDisk();
    void   ForceByteSwapDisk();
    void   SetError(LanguageStringID ErrorMsg);
    void   ConvertDiskFormat();
    void   ConvertDiskFormatBack();

    //constant values
    enum { ReadFromRomSection = 0x400000, MameFormatSize = 0x0435B0C0, SDKFormatSize = 0x03DEC800,
           DiskFormatMAME = 0x0, DiskFormatSDK = 0x1 };

    //class variables
    CFile m_DiskFile;
    uint8_t * m_DiskImage;
    uint8_t * m_DiskImageBase;
    uint32_t m_DiskFileSize;
    uint32_t m_DiskBufAddress;
    LanguageStringID m_ErrorMsg;
    Country m_Country;
    stdstr m_RomName, m_FileName, m_DiskIdent;
    uint8_t m_DiskFormat; //0 = MAME, 1 = SDK

    //disk convert
    #define SECTORS_PER_BLOCK	85
    #define BLOCKS_PER_TRACK	2

    #define BLOCKSIZE(_zone) ZoneSecSize[_zone] * SECTORS_PER_BLOCK
    #define TRACKSIZE(_zone) BLOCKSIZE(_zone) * BLOCKS_PER_TRACK
    #define ZONESIZE(_zone) TRACKSIZE(_zone) * ZoneTracks[_zone]
    #define VZONESIZE(_zone) TRACKSIZE(_zone) * (ZoneTracks[_zone] - 0xC)
};