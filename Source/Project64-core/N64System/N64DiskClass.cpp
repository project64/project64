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
#include "stdafx.h"
#include "N64DiskClass.h"
#include "SystemGlobals.h"
#include <Common/Platform.h>
#include <Common/MemoryManagement.h>

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

    if (!AllocateAndLoadDiskImage(FileLoc))
    {
        return false;
    }

    if (g_Disk == this)
    {
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
    }

    return true;
}

bool CN64Disk::IsValidDiskImage(uint8_t Test[4])
{
    if (*((uint32_t *)&Test[0]) == 0x16D348E8) { return true; }
    return false;
}

bool CN64Disk::AllocateDiskImage(uint32_t DiskFileSize)
{
    WriteTrace(TraceN64System, TraceDebug, "Allocating memory for disk");
    std::auto_ptr<uint8_t> ImageBase(new uint8_t[DiskFileSize + 0x1000]);
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

    g_Notify->DisplayMessage(5, MSG_BYTESWAP);
    ByteSwapDisk();

    ProtectMemory(m_DiskImage, m_DiskFileSize, MEM_READWRITE);
    return true;
}

void CN64Disk::ByteSwapDisk()
{
    uint32_t count;

    switch (*((uint32_t *)&m_DiskImage[0]))
    {
    case 0x16D348E8:
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
    default:
        g_Notify->DisplayError(stdstr_f("ByteSwapDisk: %X", m_DiskImage[0]).c_str());
    }
}

void CN64Disk::SetError(LanguageStringID ErrorMsg)
{
    m_ErrorMsg = ErrorMsg;
}

void CN64Disk::UnallocateDiskImage()
{
    m_DiskFile.Close();

    if (m_DiskImageBase)
    {
        ProtectMemory(m_DiskImage, m_DiskFileSize, MEM_READWRITE);
        delete[] m_DiskImageBase;
        m_DiskImageBase = NULL;
    }
    m_DiskImage = NULL;
}