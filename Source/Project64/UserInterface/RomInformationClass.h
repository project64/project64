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

class RomInformation
{
    bool const    m_DeleteRomInfo;
    bool const    m_DeleteDiskInfo;
    stdstr  const m_FileName;
    CN64Rom *     m_pRomInfo;
    CN64Disk *    m_pDiskInfo;

    friend DWORD CALLBACK RomInfoProc(HWND, DWORD, DWORD, DWORD);

public:
    RomInformation(const char* RomFile);
    RomInformation(CN64Rom* RomInfo);
    RomInformation(CN64Disk* DiskInfo);
    ~RomInformation();

    void DisplayInformation(HWND hParent) const;
};
