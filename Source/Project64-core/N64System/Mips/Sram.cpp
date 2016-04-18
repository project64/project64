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
#include <Project64-core/N64System/Mips/Sram.h>
#include <Common/path.h>

CSram::CSram(bool ReadOnly) :
m_ReadOnly(ReadOnly)
{
}

CSram::~CSram()
{
}

bool CSram::LoadSram()
{

    CPath FileName(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), "");
    FileName.SetName(g_Settings->LoadStringVal(Game_GameName).c_str());
    FileName.SetExtension("sra");

    if (!FileName.DirectoryExists())
    {
        FileName.DirectoryCreate();
    }

    if (!m_File.Open(FileName, (m_ReadOnly ? CFileBase::modeRead : CFileBase::modeReadWrite) | CFileBase::modeNoTruncate | CFileBase::modeCreate))
    {
#ifdef _WIN32
        WriteTrace(TraceN64System, TraceError, "Failed to open (%s), ReadOnly = %d, LastError = %X", (const char *)FileName, m_ReadOnly, GetLastError());
#else
        WriteTrace(TraceN64System, TraceError, "Failed to open (%s), ReadOnly = %d", (const char *)FileName, m_ReadOnly);
#endif
        return false;
    }
    m_File.SeekToBegin();
    return true;
}

void CSram::DmaFromSram(uint8_t * dest, int32_t StartOffset, int32_t len)
{
    uint32_t i;
    uint8_t tmp[4];

    if (!m_File.IsOpen())
    {
        if (!LoadSram())
        {
            return;
        }
    }

    // Fix Dezaemon 3D saves
    StartOffset = ((StartOffset >> 3) & 0xFFFF8000) | (StartOffset & 0x7FFF);

    uint32_t Offset = StartOffset & 3;

    if (Offset == 0)
    {
        m_File.Seek(StartOffset, CFile::begin);
        m_File.Read(dest, len);
    }
    else
    {
        m_File.Seek(StartOffset - Offset, CFile::begin);
        m_File.Read(tmp, 4);

        for (i = 0; i < (4 - Offset); i++)
        {
            dest[i + Offset] = tmp[i];
        }
        for (i = 4 - Offset; i < len - Offset; i += 4)
        {
            m_File.Read(tmp, 4);
            switch (Offset)
            {
            case 1:
                dest[i + 2] = tmp[0];
                dest[i + 3] = tmp[1];
                dest[i + 4] = tmp[2];
                dest[i - 3] = tmp[3];
                break;
            case 2:
                dest[i + 4] = tmp[0];
                dest[i + 5] = tmp[1];
                dest[i - 2] = tmp[2];
                dest[i - 1] = tmp[3];
                break;
            case 3:
                dest[i + 6] = tmp[0];
                dest[i - 1] = tmp[1];
                dest[i] = tmp[2];
                dest[i + 1] = tmp[3];
                break;
            default:
                break;
            }
        }
        m_File.Read(tmp, 4);
        switch (Offset)
        {
        case 1:
            dest[i - 3] = tmp[3];
            break;
        case 2:
            dest[i - 2] = tmp[2];
            dest[i - 1] = tmp[3];
            break;
        case 3:
            dest[i - 1] = tmp[1];
            dest[i] = tmp[2];
            dest[i + 1] = tmp[3];
            break;
        default:
            break;
        }
    }
}

void CSram::DmaToSram(uint8_t * Source, int32_t StartOffset, int32_t len)
{
    uint32_t i;
    uint8_t tmp[4];

    if (m_ReadOnly)
    {
        return;
    }

    if (!m_File.IsOpen())
    {
        if (!LoadSram())
        {
            return;
        }
    }

    // Fix Dezaemon 3D saves
    StartOffset = ((StartOffset >> 3) & 0xFFFF8000) | (StartOffset & 0x7FFF);

    uint32_t Offset = StartOffset & 3;

    if (Offset == 0)
    {
        m_File.Seek(StartOffset, CFile::begin);
        m_File.Write(Source, len);
    }
    else
    {
        for (i = 0; i < (4 - Offset); i++)
        {
            tmp[i] = Source[i + Offset];
        }
        m_File.Seek(StartOffset - Offset, CFile::begin);
        m_File.Write(tmp, (4 - Offset));
        m_File.Seek(Offset, CFile::current);

        for (i = 4 - Offset; i < len - Offset; i += 4)
        {
            switch (Offset)
            {
            case 1:
                tmp[0] = Source[i + 2];
                tmp[1] = Source[i + 3];
                tmp[2] = Source[i + 4];
                tmp[3] = Source[i - 3];
                break;
            case 2:
                tmp[0] = Source[i + 4];
                tmp[1] = Source[i + 5];
                tmp[2] = Source[i - 2];
                tmp[3] = Source[i - 1];
                break;
            case 3:
                tmp[0] = Source[i + 6];
                tmp[1] = Source[i - 1];
                tmp[2] = Source[i];
                tmp[3] = Source[i + 1];
                break;
            default:
                break;
            }
            m_File.Write(tmp, 4);
        }
        switch (Offset)
        {
        case 1:
            tmp[0] = Source[i - 3];
            break;
        case 2:
            tmp[0] = Source[i - 2];
            tmp[0] = Source[i - 1];
            break;
        case 3:
            tmp[0] = Source[i - 1];
            tmp[0] = Source[i];
            tmp[0] = Source[i + 1];
            break;
        default:
            break;
        }
        m_File.Seek(4 - Offset, CFile::current);
        m_File.Write(tmp, Offset);
    }
    m_File.Flush();
}