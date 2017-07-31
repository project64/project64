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
#include "Mempak.h"

#include <stdio.h>
#include <Common/path.h>

CMempak::CMempak()
{
    for (uint32_t i = 0; i < sizeof(m_Formatted) / sizeof(m_Formatted[0]); i++)
    {
        m_Formatted[i] = 0;
        m_SaveExists[i] = true;
     }
    memset(m_Mempaks, 0, sizeof(m_Mempaks));
}

void CMempak::LoadMempak(int32_t Control, bool Create)
{
    stdstr MempakName;
    MempakName.Format("%s_Cont_%d", g_Settings->LoadStringVal(Game_GameName).c_str(), Control + 1);

    CPath MempakPath(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), stdstr_f("%s.mpk", MempakName.c_str()).c_str());
    if (g_Settings->LoadBool(Setting_UniqueSaveDir))
    {
        MempakPath.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
    }

    if (!Create && !MempakPath.Exists())
    {
        if (!m_Formatted[Control])
        {
            CMempak::Format(Control);
            m_Formatted[Control] = true;
        }
        m_SaveExists[Control] = false;
        return;
    }

    if (!MempakPath.DirectoryExists())
    {
        MempakPath.DirectoryCreate();
    }

    bool formatMempak = !MempakPath.Exists();

    m_MempakHandle[Control].Open(MempakPath, CFileBase::modeReadWrite | CFileBase::modeNoTruncate | CFileBase::modeCreate);
    m_MempakHandle[Control].SeekToBegin();

    if (formatMempak)
    {
        if (!m_Formatted[Control])
        {
            CMempak::Format(Control);
            m_Formatted[Control] = true;
        }
        m_MempakHandle[Control].Write(m_Mempaks[Control], 0x8000);
    }
    else
    {
        m_MempakHandle[Control].Read(m_Mempaks[Control], 0x8000);
        m_Formatted[Control] = true;
    }
}

void CMempak::Format(int32_t Control)
{
    static const uint8_t Initialize[] = {
        0x81, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x1A, 0x5F, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0xFF, 0x66, 0x25, 0x99, 0xCD,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x1A, 0x5F, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0xFF, 0x66, 0x25, 0x99, 0xCD,
        0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x1A, 0x5F, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0xFF, 0x66, 0x25, 0x99, 0xCD,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x1A, 0x5F, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0xFF, 0x66, 0x25, 0x99, 0xCD,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x71, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03,
    };

    memcpy(&m_Mempaks[Control][0], &Initialize[0], sizeof(Initialize));

    for (size_t count = sizeof(Initialize); count < 128 * 256; count += 2)
    {
        m_Mempaks[Control][count + 0] = 0x00;
        m_Mempaks[Control][count + 1] = 0x03;
    }
}

uint8_t CMempak::CalculateCrc(uint8_t * DataToCrc)
{
    uint32_t Count;
    uint32_t XorTap;

    int32_t Length;
    uint8_t CRC = 0;

    for (Count = 0; Count < 0x21; Count++)
    {
        for (Length = 0x80; Length >= 1; Length >>= 1)
        {
            XorTap = (CRC & 0x80) ? 0x85 : 0x00;
            CRC <<= 1;
            if (Count == 0x20)
            {
                CRC &= 0xFF;
            }
            else
            {
                if ((*DataToCrc & Length) != 0)
                {
                    CRC |= 1;
                }
            }
            CRC ^= XorTap;
        }
        DataToCrc++;
    }

    return CRC;
}

void CMempak::ReadFrom(int32_t Control, uint32_t address, uint8_t * data)
{
    if (address < 0x8000)
    {
        if (m_SaveExists[Control] && !m_MempakHandle[Control].IsOpen())
        {
            LoadMempak(Control, false);
        }

        memcpy(data, &m_Mempaks[Control][address], 0x20);
    }
    else
    {
        memset(data, 0x00, 0x20);
        /* Rumble pack area */
    }
}

void CMempak::WriteTo(int32_t Control, uint32_t address, uint8_t * data)
{
    if (address < 0x8000)
    {
        if (!m_Formatted[Control])
        {
            CMempak::Format(Control);
            m_Formatted[Control] = true;
        }
        if (memcmp(&m_Mempaks[Control][address], data, 0x20) != 0)
        {
            if (!m_MempakHandle[Control].IsOpen())
            {
                LoadMempak(Control, true);
            }
            memcpy(&m_Mempaks[Control][address], data, 0x20);

            m_MempakHandle[Control].Seek(address, CFile::begin);
            m_MempakHandle[Control].Write(data, 0x20);
        }
    }
    else
    {
        /* Rumble pack area */
    }
}