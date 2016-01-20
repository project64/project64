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
#include "Disk.h"
#include <Project64-core/N64System/N64DiskClass.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>

bool dd_write;
bool dd_reset_hold;
uint32_t dd_track_offset, dd_zone;

uint8_t dd_buffer[0x100];

void DiskCommand()
{
    //ASIC_CMD_STATUS - Commands
    uint32_t cmd = g_Reg->ASIC_CMD;

#ifdef _WIN32
    SYSTEMTIME sysTime;
    ::GetLocalTime(&sysTime);
    //stdstr_f timestamp("%04d/%02d/%02d %02d:%02d:%02d.%03d %05d,", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds, GetCurrentThreadId());

    //BCD format needed for 64DD RTC
    uint8_t year = (uint8_t)(((sysTime.wYear / 10) << 4) | (sysTime.wYear % 10));
    uint8_t month = (uint8_t)(((sysTime.wMonth / 10) << 4) | (sysTime.wMonth % 10));
    uint8_t day = (uint8_t)(((sysTime.wDay / 10) << 4) | (sysTime.wDay % 10));
    uint8_t hour = (uint8_t)(((sysTime.wHour / 10) << 4) | (sysTime.wHour % 10));
    uint8_t minute = (uint8_t)(((sysTime.wMinute / 10) << 4) | (sysTime.wMinute % 10));
    uint8_t second = (uint8_t)(((sysTime.wSecond / 10) << 4) | (sysTime.wSecond % 10));
#else
    time_t ltime;
    ltime = time(&ltime);
    
    struct tm result = { 0 };
    localtime_r(&ltime, &result);

    //stdstr_f timestamp("%04d/%02d/%02d %02d:%02d:%02d.%03d %05d,", result.tm_year + 1900, result.tm_mon + 1, result.tm_mday, result.tm_hour, result.tm_min, result.tm_sec, milliseconds, GetCurrentThreadId());

    //BCD format needed for 64DD RTC
    uint8_t year = (uint8_t)(((result.tm_year / 10) << 4) | (result.tm_year % 10));
    uint8_t month = (uint8_t)(((result.tm_mon / 10) << 4) | (result.tm_mon % 10));
    uint8_t day = (uint8_t)(((result.tm_mday / 10) << 4) | (result.tm_mday % 10));
    uint8_t hour = (uint8_t)(((result.tm_hour / 10) << 4) | (result.tm_hour % 10));
    uint8_t minute = (uint8_t)(((result.tm_min / 10) << 4) | (result.tm_min % 10));
    uint8_t second = (uint8_t)(((result.tm_sec / 10) << 4) | (result.tm_sec % 10));
#endif

    switch (cmd & 0xFFFF0000)
    {
    case 0x00010000:
        //Seek Read
        g_Reg->ASIC_CUR_TK = g_Reg->ASIC_DATA | 0x60000000;
        dd_write = false;
        break;
    case 0x00020000:
        //Seek Write
        g_Reg->ASIC_CUR_TK = g_Reg->ASIC_DATA | 0x60000000;
        dd_write = true;
        break;
    case 0x00080000:
        //Unset Disk Changed Bit
        g_Reg->ASIC_STATUS &= ~DD_STATUS_DISK_CHNG; break;
    case 0x00090000:
        //Unset Reset Bit
        g_Reg->ASIC_STATUS &= ~DD_STATUS_RST_STATE; break;
    case 0x00120000:
        //RTC Get Year & Month
        g_Reg->ASIC_DATA = (year << 24) | (month << 16); break;
    case 0x00130000:
        //RTC Get Day & Hour
        g_Reg->ASIC_DATA = (day << 24) | (hour << 16); break;
    case 0x00140000:
        //RTC Get Minute & Second
        g_Reg->ASIC_DATA = (minute << 24) | (second << 16); break;
    case 0x001B0000:
        //Disk Inquiry
        g_Reg->ASIC_DATA = 0x00000000; break;
    }
}

void DiskReset(void)
{
    //ASIC_HARD_RESET 0xAAAA0000
    g_Reg->ASIC_STATUS |= DD_STATUS_RST_STATE;
}

void DiskBMControl(void)
{
    g_Reg->ASIC_CUR_SECTOR = g_Reg->ASIC_BM_CTL & 0x00FF0000;
    /*
    if ((g_Reg->ASIC_CUR_SECTOR >> 16) == 0x00)
    {

    }
    else if ((g_Reg->ASIC_CUR_SECTOR >> 16) == 0x5A)
    {

    }
    */
    if (g_Reg->ASIC_BM_CTL & DD_BM_CTL_BLK_TRANS)
        g_Reg->ASIC_BM_STATUS |= DD_BM_STATUS_BLOCK;

    if (g_Reg->ASIC_BM_CTL & DD_BM_CTL_MECHA_RST)
        g_Reg->ASIC_STATUS &= ~DD_STATUS_MECHA_INT;

    if (g_Reg->ASIC_BM_CTL & DD_BM_CTL_RESET)
        dd_reset_hold = true;

    if (!(g_Reg->ASIC_BM_CTL & DD_BM_CTL_RESET) && dd_reset_hold)
    {
        dd_reset_hold = false;
        g_Reg->ASIC_STATUS &= ~(DD_STATUS_BM_INT | DD_STATUS_BM_ERR | DD_STATUS_DATA_RQ | DD_STATUS_C2_XFER);
        g_Reg->ASIC_BM_STATUS = 0;
        g_Reg->ASIC_CUR_SECTOR = 0;
    }

    if (!(g_Reg->ASIC_STATUS & DD_STATUS_MECHA_INT) && !(g_Reg->ASIC_STATUS & DD_STATUS_BM_INT))
        g_Reg->FAKE_CAUSE_REGISTER &= ~CAUSE_IP3;

    if (g_Reg->ASIC_BM_CTL & DD_BM_CTL_START)
    {
        g_Reg->ASIC_BM_STATUS |= DD_BM_STATUS_RUNNING;
        DiskBMUpdate();
    }
}

void DiskGapSectorCheck()
{
    if (g_Reg->ASIC_STATUS & DD_STATUS_BM_INT)
    {
        uint16_t testsector = (uint16_t)(g_Reg->ASIC_CUR_SECTOR >> 16);
        if (testsector >= 0x5A)
            testsector -= 0x5A;

        if (SECTORS_PER_BLOCK < testsector)
        {
            g_Reg->ASIC_STATUS &= ~DD_STATUS_BM_INT;
            g_Reg->FAKE_CAUSE_REGISTER &= ~CAUSE_IP3;
            DiskBMUpdate();
        }
    }
}

void DiskBMUpdate()
{
    if (!(g_Reg->ASIC_BM_STATUS & DD_BM_STATUS_RUNNING))
        return;

    uint16_t testsector = (uint16_t)(g_Reg->ASIC_CUR_SECTOR >> 16);
    if (testsector >= 0x5A)
        testsector -= 0x5A;

    if (dd_write)
    {
        //Write Data
        if (testsector == 0)
        {
            g_Reg->ASIC_CUR_SECTOR += 0x00010000;
            g_Reg->ASIC_STATUS |= DD_STATUS_DATA_RQ;
        }
        else if (testsector < SECTORS_PER_BLOCK)
        {
            DiskBMWrite();
            g_Reg->ASIC_CUR_SECTOR += 0x00010000;
            g_Reg->ASIC_STATUS |= DD_STATUS_DATA_RQ;
        }
        else if (testsector < SECTORS_PER_BLOCK + 1)
        {
            if (g_Reg->ASIC_BM_STATUS & DD_BM_STATUS_BLOCK)
            {
                DiskBMWrite();
                g_Reg->ASIC_CUR_SECTOR += 0x00010000;
                if (g_Reg->ASIC_CUR_SECTOR >> 16 >= 0xB4)
                    g_Reg->ASIC_CUR_SECTOR = 0x00010000;
                g_Reg->ASIC_BM_STATUS &= ~DD_BM_STATUS_BLOCK;
                g_Reg->ASIC_STATUS |= DD_STATUS_DATA_RQ;
            }
            else
            {
                DiskBMWrite();
                g_Reg->ASIC_CUR_SECTOR += 0x00010000;
                g_Reg->ASIC_BM_STATUS &= ~DD_BM_STATUS_RUNNING;
            }
        }

        g_Reg->ASIC_STATUS |= DD_STATUS_BM_INT;
        g_Reg->FAKE_CAUSE_REGISTER |= CAUSE_IP3;
        g_Reg->CheckInterrupts();
        return;
    }
    else
    {
        //Read Data
        if (((g_Reg->ASIC_CUR_TK >> 16) & 0xFFF) == 6 && g_Reg->ASIC_CUR_SECTOR == 0)
        {
            g_Reg->ASIC_STATUS &= ~DD_STATUS_DATA_RQ;
            g_Reg->ASIC_BM_STATUS |= DD_BM_STATUS_MICRO;
        }
        else if (testsector == 0)
        {
            DiskBMRead();
            g_Reg->ASIC_CUR_SECTOR += 0x00010000;
            g_Reg->ASIC_STATUS |= DD_STATUS_DATA_RQ;
        }
        else if (testsector < SECTORS_PER_BLOCK + 4)
        {
            //READ C2 (00!)
            g_Reg->ASIC_CUR_SECTOR += 0x00010000;
            if ((g_Reg->ASIC_CUR_SECTOR >> 16) == SECTORS_PER_BLOCK + 4)
                g_Reg->ASIC_STATUS |= DD_STATUS_C2_XFER;
        }
        else if (testsector == SECTORS_PER_BLOCK + 4)
        {
            if (g_Reg->ASIC_BM_STATUS & DD_BM_STATUS_BLOCK)
            {
                if (g_Reg->ASIC_CUR_SECTOR >> 16 >= 0xB4)
                    g_Reg->ASIC_CUR_SECTOR = 0x00000000;
                g_Reg->ASIC_BM_STATUS &= ~DD_BM_STATUS_BLOCK;
            }
            else
            {
                g_Reg->ASIC_BM_STATUS &= ~DD_BM_STATUS_RUNNING;
            }
        }

        g_Reg->ASIC_STATUS |= DD_STATUS_BM_INT;
        g_Reg->FAKE_CAUSE_REGISTER |= CAUSE_IP3;
        g_Reg->CheckInterrupts();
    }
}

void DiskBMRead()
{
    uint8_t * sector;
    sector = (uint8_t*)g_Disk->GetDiskAddress();
    sector += dd_track_offset;
    uint16_t block = 0;
    if (g_Reg->ASIC_CUR_SECTOR >= 0x005A0000)
        block = 1;
    sector += block * SECTORS_PER_BLOCK * ddZoneSecSize[dd_zone];
    uint16_t block2 = (uint16_t)(g_Reg->ASIC_CUR_SECTOR >> 16);
    if (block2 >= 0x5A)
        block -= 0x5A;
    sector += block * ((g_Reg->ASIC_SEC_BYTE >> 16) + 1);

    for (int i = 0; i < ((g_Reg->ASIC_SEC_BYTE >> 16) + 1) / 4; i++)
    {
        dd_buffer[i] = sector[(i * 4 + 0)] << 24 | sector[(i * 4 + 1)] << 16 |
            sector[(i * 4 + 2)] << 8 | sector[(i * 4 + 3)];
    }

    return;
}

void DiskBMWrite()
{
    uint8_t * sector;
    sector = (uint8_t*)g_Disk->GetDiskAddress();
    sector += dd_track_offset;
    uint16_t block = 0;
    if (g_Reg->ASIC_CUR_SECTOR >= 0x005A0000)
        block = 1;
    sector += block * SECTORS_PER_BLOCK * ddZoneSecSize[dd_zone];
    uint16_t block2 = (uint16_t)(g_Reg->ASIC_CUR_SECTOR >> 16);
    if (block2 >= 0x5A)
        block -= 0x5A;
    sector += block * ((g_Reg->ASIC_SEC_BYTE >> 16) + 1);

    for (int i = 0; i < ddZoneSecSize[dd_zone] / 4; i++)
    {
        sector[i * 4 + 0] = (dd_buffer[i] >> 24) & 0xFF;
        sector[i * 4 + 1] = (dd_buffer[i] >> 16) & 0xFF;
        sector[i * 4 + 2] = (dd_buffer[i] >> 8) & 0xFF;
        sector[i * 4 + 3] = (dd_buffer[i] >> 0) & 0xFF;
    }

    return;
}

void DiskSetOffset()
{
    uint16_t head = ((g_Reg->ASIC_CUR_TK >> 16) & 0x1000) >> 9; // Head * 8
    uint16_t track = (g_Reg->ASIC_CUR_TK >> 16) & 0xFFF;
    uint16_t tr_off = 0;

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

    dd_track_offset = ddStartOffset[dd_zone] + tr_off * ddZoneSecSize[dd_zone] * SECTORS_PER_BLOCK * BLOCKS_PER_TRACK;
}