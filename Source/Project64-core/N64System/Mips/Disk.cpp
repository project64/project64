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
// Based from MAME's N64DD driver code by Happy_
#include "stdafx.h"
#include "Disk.h"
#include <Project64-core/N64System/N64DiskClass.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/Mips/SystemTiming.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

uint8_t dd_swapdelay;

bool dd_write;
bool dd_reset_hold;
uint32_t dd_track_offset, dd_zone;
uint32_t dd_start_block, dd_current;

void DiskCommand()
{
    //ASIC_CMD_STATUS - Commands
    uint32_t cmd = g_Reg->ASIC_CMD;
    WriteTrace(TraceN64System, TraceDebug, "DD CMD %08X - DATA %08X", cmd, g_Reg->ASIC_DATA);

#ifdef _WIN32
    SYSTEMTIME sysTime;
    ::GetLocalTime(&sysTime);

    //BCD format needed for 64DD RTC
    uint8_t year = (uint8_t)(((sysTime.wYear / 10 % 10) << 4) | (sysTime.wYear % 10));
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

    //BCD format needed for 64DD RTC
    uint8_t year = (uint8_t)(((result.tm_year / 10 % 10) << 4) | (result.tm_year % 10));
    uint8_t month = (uint8_t)((((result.tm_mon + 1) / 10) << 4) | ((result.tm_mon + 1) % 10));
    uint8_t day = (uint8_t)(((result.tm_mday / 10) << 4) | (result.tm_mday % 10));
    uint8_t hour = (uint8_t)(((result.tm_hour / 10) << 4) | (result.tm_hour % 10));
    uint8_t minute = (uint8_t)(((result.tm_min / 10) << 4) | (result.tm_min % 10));
    uint8_t second = (uint8_t)(((result.tm_sec / 10) << 4) | (result.tm_sec % 10));
#endif

    //Used for seek times
    bool isSeek = false;

    switch (cmd & 0xFFFF0000)
    {
    case 0x00010000:
        //Seek Read
        g_Reg->ASIC_CUR_TK = g_Reg->ASIC_DATA | 0x60000000;
        dd_write = false;
        isSeek = true;
        break;
    case 0x00020000:
        //Seek Write
        g_Reg->ASIC_CUR_TK = g_Reg->ASIC_DATA | 0x60000000;
        dd_write = true;
        isSeek = true;
        break;
    case 0x00080000:
        //Unset Disk Changed Bit
        g_Reg->ASIC_STATUS &= ~DD_STATUS_DISK_CHNG; break;
    case 0x00090000:
        //Unset Reset & Disk Changed bit Bit
        g_Reg->ASIC_STATUS &= ~DD_STATUS_RST_STATE;
        g_Reg->ASIC_STATUS &= ~DD_STATUS_DISK_CHNG;
        //F-Zero X + Expansion Kit fix so it doesn't enable "swapping" at boot
        dd_swapdelay = 0;
        if (g_Disk != NULL)
            g_Reg->ASIC_STATUS |= DD_STATUS_DISK_PRES;
        break;
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

    if (isSeek)
    {
        //Emulate Seek Times, send interrupt later
        uint32_t seektime = 0x179200;

        //Start Motor, can take half a second, delay the response
        if (g_Reg->ASIC_STATUS & DD_STATUS_MTR_N_SPIN)
        {
            seektime += (0x5A00000 / 2);
            g_Reg->ASIC_STATUS &= ~DD_STATUS_MTR_N_SPIN;
        }

        //Get Zone to calculate seek times
        uint32_t track = g_Reg->ASIC_CUR_TK >> 16 & 0x0FFF;
        uint32_t zone = 0;
        uint32_t zonebound = 0;
        for (uint8_t i = 0; i < 8; i++)
        {
            zonebound += ddZoneTrackSize[i];
            if (track < zonebound)
            {
                zone = i;
                if (g_Reg->ASIC_CUR_TK & 0x10000000)
                    zone++;
                break;
            }
        }

        //Add delay depending on the zone
        switch (zone)
        {
        case 0:
        default:
            break;
        case 1:
            seektime += 0x1900;
            break;
        case 2:
            seektime += 0x2A00;
            break;
        case 3:
            seektime += 0x4500;
            break;
        case 4:
            seektime += 0x5E00;
            break;
        case 5:
            seektime += 0x7A00;
            break;
        case 6:
            seektime += 0x9700;
            break;
        case 7:
            seektime += 0xAF00;
            break;
        case 8:
            seektime += 0xCC00;
            break;
        }

        //Set timer for seek response
        g_SystemTimer->SetTimer(g_SystemTimer->DDSeekTimer, seektime, false);

        //Set timer for motor to shutdown in 5 seconds, reset the timer if other seek commands were sent
        g_SystemTimer->SetTimer(g_SystemTimer->DDMotorTimer, 0x5A00000 * 5, false);
    }
    else
    {
        //Other commands are basically instant
        g_Reg->ASIC_STATUS |= DD_STATUS_MECHA_INT;
        g_Reg->FAKE_CAUSE_REGISTER |= CAUSE_IP3;
        g_Reg->CheckInterrupts();
    }
}

void DiskReset(void)
{
    //ASIC_HARD_RESET 0xAAAA0000
    WriteTrace(TraceN64System, TraceDebug, "DD RESET");
    g_Reg->ASIC_STATUS |= DD_STATUS_RST_STATE;
    dd_swapdelay = 0;
    if (g_Disk != NULL)
        g_Reg->ASIC_STATUS |= DD_STATUS_DISK_PRES;
}

void DiskBMControl(void)
{
    g_Reg->ASIC_CUR_SECTOR = g_Reg->ASIC_BM_CTL & 0x00FF0000;
    
    if ((g_Reg->ASIC_CUR_SECTOR >> 16) == 0x00)
    {
        dd_start_block = 0;
        dd_current = 0;
    }
    else if ((g_Reg->ASIC_CUR_SECTOR >> 16) == 0x5A)
    {
        dd_start_block = 1;
        dd_current = 0;
    }
    
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
        dd_start_block = 0;
        dd_current = 0;
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
    //On 64DD Status Register Read

    //Buffer Manager Interrupt, Gap Sector Check
    if (g_Reg->ASIC_STATUS & DD_STATUS_BM_INT)
    {
        if (SECTORS_PER_BLOCK < dd_current)
        {
            g_Reg->ASIC_STATUS &= ~DD_STATUS_BM_INT;
            g_Reg->FAKE_CAUSE_REGISTER &= ~CAUSE_IP3;
            g_Reg->CheckInterrupts();
            DiskBMUpdate();
        }
    }

    //Delay Disk Swapping by removing the disk for a certain amount of time, then insert the newly loaded disk (after 50 Status Register reads, here).
    if (!(g_Reg->ASIC_STATUS & DD_STATUS_DISK_PRES) && g_Disk != NULL && g_Settings->LoadBool(GameRunning_LoadingInProgress) == false)
    {
        dd_swapdelay++;
        if (dd_swapdelay >= 50)
        {
            g_Reg->ASIC_STATUS |= (DD_STATUS_DISK_PRES | DD_STATUS_DISK_CHNG);
            dd_swapdelay = 0;
            WriteTrace(TraceN64System, TraceDebug, "DD SWAP DONE");
        }
    }
}

void DiskBMUpdate()
{
    if (!(g_Reg->ASIC_BM_STATUS & DD_BM_STATUS_RUNNING))
        return;

    if (dd_write)
    {
        //Write Data
        if (dd_current < SECTORS_PER_BLOCK)
        {
            //User Sector
            if (!DiskBMReadWrite(true))
                g_Reg->ASIC_STATUS |= DD_STATUS_DATA_RQ;
            dd_current += 1;
        }
        else if (dd_current < SECTORS_PER_BLOCK + 1)
        {
            //C2 Sector
            if (g_Reg->ASIC_BM_STATUS & DD_BM_STATUS_BLOCK)
            {
                dd_start_block = 1 - dd_start_block;
                dd_current = 0;
                if (!DiskBMReadWrite(true))
                    g_Reg->ASIC_STATUS |= DD_STATUS_DATA_RQ;
                dd_current += 1;
                g_Reg->ASIC_BM_STATUS &= ~DD_BM_STATUS_BLOCK;
            }
            else
            {
                dd_current += 1;
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
        if (((g_Reg->ASIC_CUR_TK >> 16) & 0x1FFF) == 6 && g_Reg->ASIC_CUR_SECTOR == 0 && g_Disk->GetCountry() != Country::UnknownCountry)
        {
            //Copy Protection if Retail Disk
            g_Reg->ASIC_STATUS &= ~DD_STATUS_DATA_RQ;
            g_Reg->ASIC_BM_STATUS |= DD_BM_STATUS_MICRO;
        }
        else if (dd_current < SECTORS_PER_BLOCK)
        {
            //User Sector
            if (!DiskBMReadWrite(false))
                g_Reg->ASIC_STATUS |= DD_STATUS_DATA_RQ;
            dd_current += 1;
        }
        else if (dd_current < SECTORS_PER_BLOCK + 4)
        {
            //C2 sectors (All 00s)
            dd_current += 1;
            if (dd_current == SECTORS_PER_BLOCK + 4)
                g_Reg->ASIC_STATUS |= DD_STATUS_C2_XFER;
        }
        else if (dd_current == SECTORS_PER_BLOCK + 4)
        {
            //Gap Sector
            if (g_Reg->ASIC_BM_STATUS & DD_BM_STATUS_BLOCK)
            {
                dd_start_block = 1 - dd_start_block;
                dd_current = 0;
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

bool DiskBMReadWrite(bool write)
{
    //Returns true if error
    uint16_t head = ((g_Reg->ASIC_CUR_TK >> 16) / 0x1000) & 1;
    uint16_t track = (g_Reg->ASIC_CUR_TK >> 16) & 0xFFF;
    uint16_t block = dd_start_block;
    uint16_t sector = dd_current;
    uint16_t sectorsize = (((g_Reg->ASIC_HOST_SECBYTE & 0x00FF0000) >> 16) + 1);
    
    uint32_t addr = g_Disk->GetDiskAddressBlock(head, track, block, sector, sectorsize);

    if (addr == 0xFFFFFFFF)
    {
        //Error
        return true;
    }
    else
    {
        g_Disk->SetDiskAddressBuffer(addr);
        return false;
    }
}

void DiskDMACheck(void)
{
    if (g_Reg->PI_CART_ADDR_REG == 0x05000000)
    {
        g_Reg->ASIC_STATUS &= ~(DD_STATUS_BM_INT | DD_STATUS_BM_ERR | DD_STATUS_C2_XFER);
        g_Reg->FAKE_CAUSE_REGISTER &= ~CAUSE_IP3;
        g_Reg->CheckInterrupts();
    }
    else if (g_Reg->PI_CART_ADDR_REG == 0x05000400)
    {
        g_Reg->ASIC_STATUS &= ~(DD_STATUS_BM_INT | DD_STATUS_BM_ERR | DD_STATUS_DATA_RQ);
        g_Reg->FAKE_CAUSE_REGISTER &= ~CAUSE_IP3;
        g_Reg->CheckInterrupts();
    }
}