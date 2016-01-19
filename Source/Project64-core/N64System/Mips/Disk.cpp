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
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>

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
	}
}

void DiskReset(void)
{
	//ASIC_HARD_RESET 0xAAAA0000
	g_Reg->ASIC_STATUS |= DD_STATUS_RST_STATE;
}

void DiskBMControl(void)
{
	if (g_Reg->ASIC_BM_CTL & DD_BM_CTL_MECHA_RST)
	{
		g_Reg->ASIC_STATUS &= ~DD_STATUS_MECHA_INT;
		g_Reg->FAKE_CAUSE_REGISTER &= ~CAUSE_IP3;
	}
}