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
#include <Common/stdtypes.h>

class CN64Disk
{
public:
    CN64Disk();
    ~CN64Disk();

    bool    LoadDiskImage(const char * FileLoc);
    static bool IsValidDiskImage(uint8_t Test[4]);
    uint8_t *  GetDiskAddress() { return m_DiskImage; }
    void    UnallocateDiskImage();

private:
    bool   AllocateDiskImage(uint32_t DiskFileSize);
    bool   AllocateAndLoadDiskImage(const char * FileLoc);
    void   ByteSwapDisk();
    void   SetError(LanguageStringID ErrorMsg);

    //constant values
    enum { ReadFromRomSection = 0x400000 };

    //class variables
    CFile m_DiskFile;
    uint8_t * m_DiskImage;
    uint8_t * m_DiskImageBase;
    uint32_t m_DiskFileSize;
    LanguageStringID m_ErrorMsg;
    stdstr m_FileName, m_DiskIdent;
};