/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

CFlashram::CFlashram(bool ReadOnly):
	m_FlashFlag(FLASHRAM_MODE_NOPES),
	m_FlashStatus(0),
	m_FlashRamPointer(NULL),
	m_FlashRAM_Offset(0),
	m_ReadOnly(ReadOnly),
	m_hFile(NULL)
{
	
}

CFlashram::~CFlashram()
{
	if (m_hFile)
	{
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}
}

void CFlashram::DmaFromFlashram ( BYTE * dest, int StartOffset, int len) 
{
	BYTE FlipBuffer[0x10000];
	DWORD dwRead, count;

	switch (m_FlashFlag)
	{
	case FLASHRAM_MODE_READ:
		if (m_hFile == NULL)
		{
			if (!LoadFlashram())
			{
				return;
			}
		}
		if (len > 0x10000) 
		{
			if (bHaveDebugger())
			{
				g_Notify->DisplayError(L"DmaFromFlashram FlipBuffer to small (len: %d)",len); 
			}
			len = 0x10000;
		}
		if ((len & 3) != 0) 
		{
			if (bHaveDebugger())
			{
				g_Notify->DisplayError(L"Unaligned flash ram read ???");
			}
			return;
		}
		memset(FlipBuffer,0,sizeof(FlipBuffer));
		StartOffset = StartOffset << 1;
		SetFilePointer(m_hFile,StartOffset,NULL,FILE_BEGIN);	
		ReadFile(m_hFile,FlipBuffer,len,&dwRead,NULL);
		for (count = dwRead; (int)count < len; count ++)
		{
			FlipBuffer[count] = 0xFF;
		}
		_asm
		{
			mov edi, dest
			lea ecx, [FlipBuffer]
			mov edx, 0
			mov ebx, len

		memcpyloop:
			mov eax, dword ptr [ecx + edx]
			;bswap eax
			mov  dword ptr [edi + edx],eax
			add edx, 4
			cmp edx, ebx
			jb memcpyloop
		}
		break;
	case FLASHRAM_MODE_STATUS:
		if (StartOffset != 0 && len != 8) 
		{
			if (bHaveDebugger())
			{
				g_Notify->DisplayError(L"Reading m_FlashStatus not being handled correctly\nStart: %X len: %X",StartOffset,len);
			}
		}
		*((DWORD *)(dest)) = (DWORD)((m_FlashStatus >> 32) & 0xFFFFFFFF);
		*((DWORD *)(dest) + 1) = (DWORD)(m_FlashStatus & 0xFFFFFFFF);
		break;
	default:
		if (bHaveDebugger())
		{
			g_Notify->DisplayError(L"DmaFromFlashram Start: %X, Offset: %X len: %X",dest - g_MMU->Rdram(),StartOffset,len);
		}
	}
}

void CFlashram::DmaToFlashram(BYTE * Source, int StartOffset, int len)
{
	switch (m_FlashFlag)
	{
	case FLASHRAM_MODE_WRITE:
		m_FlashRamPointer = Source;
		break;
	default:
		if (bHaveDebugger())
		{
			g_Notify->DisplayError(L"DmaToFlashram Start: %X, Offset: %X len: %X",Source - g_MMU->Rdram(),StartOffset,len);
		}
	}
}


DWORD CFlashram::ReadFromFlashStatus (DWORD PAddr) 
{
	switch (PAddr)
	{
	case 0x08000000: return (DWORD)(m_FlashStatus >> 32);
	default:
		if (bHaveDebugger())
		{
			g_Notify->DisplayError(L"Reading from flash ram status (%X)",PAddr);
		}
		break;
	}
	return (DWORD)(m_FlashStatus >> 32);
}

bool CFlashram::LoadFlashram()
{
	CPath FileName;

	FileName.SetDriveDirectory( g_Settings->LoadString(Directory_NativeSave).c_str());
	FileName.SetName(g_Settings->LoadString(Game_GameName).c_str());
	FileName.SetExtension("fla");

	if (!FileName.DirectoryExists())
	{
		FileName.CreateDirectory();
	}

	m_hFile = CreateFile(FileName,m_ReadOnly ? GENERIC_READ : GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE) 
	{
		WriteTraceF(TraceError,__FUNCTION__ ": Failed to open (%s), ReadOnly = %d, LastError = %X",(LPCTSTR)FileName, m_ReadOnly, GetLastError());
		g_Notify->DisplayError(GS(MSG_FAIL_OPEN_FLASH));
		return false;
	}
	SetFilePointer(m_hFile,0,NULL,FILE_BEGIN);	
	return true;
}

void CFlashram::WriteToFlashCommand(DWORD FlashRAM_Command)
{
	BYTE EmptyBlock[128];
	DWORD dwWritten;

	switch (FlashRAM_Command & 0xFF000000)
	{
	case 0xD2000000: 
		switch (m_FlashFlag)
		{
		case FLASHRAM_MODE_NOPES: break;
		case FLASHRAM_MODE_READ: break;
		case FLASHRAM_MODE_STATUS: break;
		case FLASHRAM_MODE_ERASE:
			memset(EmptyBlock,0xFF,sizeof(EmptyBlock));
			if (m_hFile == NULL) {
				if (!LoadFlashram())
				{
					return;
				}
			}
			SetFilePointer(m_hFile,m_FlashRAM_Offset,NULL,FILE_BEGIN);	
			WriteFile(m_hFile,EmptyBlock,128,&dwWritten,NULL);
			break;
		case FLASHRAM_MODE_WRITE:
			if (m_hFile == NULL) {
				if (!LoadFlashram())
				{
					return;
				}
			}
			{
				BYTE FlipBuffer[128];
				DWORD dwWritten;
				BYTE * FlashRamPointer = m_FlashRamPointer;

				memset(FlipBuffer,0,sizeof(FlipBuffer));
				_asm
				{
					lea edi, [FlipBuffer]
					mov ecx, FlashRamPointer
					mov edx, 0

				memcpyloop:
					mov eax, dword ptr [ecx + edx]
					;bswap eax
					mov  dword ptr [edi + edx],eax
					add edx, 4
					cmp edx, 128
					jb memcpyloop
				}

				SetFilePointer(m_hFile,m_FlashRAM_Offset,NULL,FILE_BEGIN);	
				WriteFile(m_hFile,FlipBuffer,128,&dwWritten,NULL);
			}
			break;
		default:
			g_Notify->DisplayError(L"Writing %X to flash ram command register\nm_FlashFlag: %d",FlashRAM_Command,m_FlashFlag);
		}
		m_FlashFlag = FLASHRAM_MODE_NOPES;
		break;
	case 0xE1000000: 
		m_FlashFlag = FLASHRAM_MODE_STATUS;
		m_FlashStatus = 0x1111800100C2001E;
		break;
	case 0xF0000000: 
		m_FlashFlag = FLASHRAM_MODE_READ;
		m_FlashStatus = 0x11118004F0000000;
		break;
	case 0x4B000000:
		m_FlashRAM_Offset = (FlashRAM_Command & 0xffff) * 128;
		break;
	case 0x78000000:
		m_FlashFlag = FLASHRAM_MODE_ERASE;
		m_FlashStatus = 0x1111800800C2001E;
		break;
	case 0xB4000000: 
		m_FlashFlag = FLASHRAM_MODE_WRITE; //????
		break;
	case 0xA5000000:
		m_FlashRAM_Offset = (FlashRAM_Command & 0xffff) * 128;
		m_FlashStatus = 0x1111800400C2001E;
		break;
	default:
		if (bHaveDebugger())
		{
			g_Notify->DisplayError(L"Writing %X to flash ram command register",FlashRAM_Command);
		}
	}
}
