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
#include <Project64-core/N64System/Mips/FlashRam.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Common/path.h>

CFlashram::CFlashram(bool ReadOnly) :
m_FlashRamPointer(NULL),
m_FlashFlag(FLASHRAM_MODE_NOPES),
m_FlashStatus(0),
m_FlashRAM_Offset(0),
m_ReadOnly(ReadOnly)
{
}

CFlashram::~CFlashram()
{
}

void CFlashram::DmaFromFlashram(uint8_t * dest, int32_t StartOffset, int32_t len)
{
    uint8_t FlipBuffer[0x10000];

    switch (m_FlashFlag)
    {
    case FLASHRAM_MODE_READ:
        if (!m_File.IsOpen() && !LoadFlashram())
        {
            return;
        }
        if (len > sizeof(FlipBuffer))
        {
            if (HaveDebugger())
            {
                g_Notify->DisplayError(stdstr_f("%s: DmaFromFlashram FlipBuffer to small (len: %d)", __FUNCTION__, len).c_str());
            }
            len = sizeof(FlipBuffer);
        }
        if ((len & 3) != 0)
        {
            if (HaveDebugger())
            {
                g_Notify->DisplayError(stdstr_f("%s: Unaligned flash ram read ???", __FUNCTION__).c_str());
            }
            return;
        }
        memset(FlipBuffer, 0, sizeof(FlipBuffer));
        StartOffset = StartOffset << 1;
        m_File.Seek(StartOffset, CFile::begin);
        m_File.Read(FlipBuffer, len);

        for (int32_t count = m_File.GetLength(); count < len; count++)
        {
            FlipBuffer[count] = 0xFF;
        }

        for (int32_t count = 0; count < len; count += 4)
        {
            *(uint32_t *)(dest + count) = *(uint32_t *)&FlipBuffer[count];
        }
        break;
    case FLASHRAM_MODE_STATUS:
        if (StartOffset != 0 && len != 8)
        {
            if (HaveDebugger())
            {
                g_Notify->DisplayError(stdstr_f("%s: Reading m_FlashStatus not being handled correctly\nStart: %X len: %X", __FUNCTION__, StartOffset, len).c_str());
            }
        }
        *((uint32_t *)(dest)+0) = (uint32_t)((m_FlashStatus >> 32) & 0xFFFFFFFF);
        *((uint32_t *)(dest)+1) = (uint32_t)(m_FlashStatus & 0xFFFFFFFF);
        break;
    default:
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s: Start: %X, Offset: %X len: %X", __FUNCTION__, dest - g_MMU->Rdram(), StartOffset, len).c_str());
        }
    }
}

void CFlashram::DmaToFlashram(uint8_t * Source, int32_t StartOffset, int32_t len)
{
    switch (m_FlashFlag)
    {
    case FLASHRAM_MODE_WRITE:
        m_FlashRamPointer = Source;
        break;
    default:
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s: Start: %X, Offset: %X len: %X", __FUNCTION__, Source - g_MMU->Rdram(), StartOffset, len).c_str());
        }
    }
}

uint32_t CFlashram::ReadFromFlashStatus(uint32_t PAddr)
{
    switch (PAddr)
    {
    case 0x08000000: return (uint32_t)(m_FlashStatus >> 32);
    default:
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("%s: PAddr (%X)", __FUNCTION__, PAddr).c_str());
        }
        break;
    }
    return (uint32_t)(m_FlashStatus >> 32);
}

bool CFlashram::LoadFlashram()
{
    CPath FileName(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), stdstr_f("%s.fla", g_Settings->LoadStringVal(Game_GameName).c_str()).c_str());
    if (g_Settings->LoadBool(Setting_UniqueSaveDir))
    {
        FileName.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
    }
#ifdef _WIN32
    FileName.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif

    if (!FileName.DirectoryExists())
    {
        FileName.DirectoryCreate();
    }

    if (!m_File.Open(FileName, (m_ReadOnly ? CFileBase::modeRead : CFileBase::modeReadWrite) | CFileBase::modeNoTruncate | CFileBase::modeCreate))
    {
        WriteTrace(TraceN64System, TraceError, "Failed to open (%s), ReadOnly = %d", (const char *)FileName, m_ReadOnly);
        g_Notify->DisplayError(GS(MSG_FAIL_OPEN_FLASH));
        return false;
    }
    m_File.SeekToBegin();
    return true;
}

void CFlashram::WriteToFlashCommand(uint32_t FlashRAM_Command)
{
    uint8_t EmptyBlock[16 * sizeof(int64_t)];

    switch (FlashRAM_Command & 0xFF000000)
    {
    case 0xD2000000:
        switch (m_FlashFlag)
        {
        case FLASHRAM_MODE_NOPES: break;
        case FLASHRAM_MODE_READ: break;
        case FLASHRAM_MODE_STATUS: break;
        case FLASHRAM_MODE_ERASE:
            memset(EmptyBlock, 0xFF, sizeof(EmptyBlock));
            if (!m_File.IsOpen() && !LoadFlashram())
            {
                return;
            }
            if (!m_ReadOnly)
            {
                m_File.Seek(m_FlashRAM_Offset, CFile::begin);
                m_File.Write(EmptyBlock, sizeof(EmptyBlock));
            }
            break;
        case FLASHRAM_MODE_WRITE:
            if (!m_File.IsOpen() && !LoadFlashram())
            {
                return;
            }
            {
                uint8_t FlipBuffer[sizeof(EmptyBlock)];
                uint8_t * FlashRamPointer = m_FlashRamPointer;

                memset(FlipBuffer, 0, sizeof(FlipBuffer));
                memcpy(&FlipBuffer[0], FlashRamPointer, sizeof(EmptyBlock));

                if (!m_ReadOnly)
                {
                    m_File.Seek(m_FlashRAM_Offset, CFile::begin);
                    m_File.Write(FlipBuffer, sizeof(EmptyBlock));
                }
            }
            break;
        default:
            g_Notify->DisplayError(stdstr_f("Writing %X to flash ram command register\nm_FlashFlag: %d", FlashRAM_Command, m_FlashFlag).c_str());
        }
        m_FlashFlag = FLASHRAM_MODE_NOPES;
        break;
    case 0xE1000000:
        m_FlashFlag = FLASHRAM_MODE_STATUS;
        m_FlashStatus = 0x1111800100C2001E;
        break;
    case 0xF0000000:
    case 0x00000000:
        m_FlashFlag = FLASHRAM_MODE_READ;
        m_FlashStatus = 0x11118004F0000000;
        break;
    case 0x4B000000:
        m_FlashRAM_Offset = (FlashRAM_Command & 0xFFFF) * sizeof(EmptyBlock);
        break;
    case 0x78000000:
        m_FlashFlag = FLASHRAM_MODE_ERASE;
        m_FlashStatus = 0x1111800800C2001E;
        break;
    case 0xB4000000:
        m_FlashFlag = FLASHRAM_MODE_WRITE; //????
        break;
    case 0xA5000000:
        m_FlashRAM_Offset = (FlashRAM_Command & 0xFFFF) * sizeof(EmptyBlock);
        m_FlashStatus = 0x1111800400C2001E;
        break;
    default:
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("Writing %X to flash ram command register", FlashRAM_Command).c_str());
        }
    }
}