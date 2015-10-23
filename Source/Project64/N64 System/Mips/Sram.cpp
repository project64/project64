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

CSram::CSram ( bool ReadOnly ) :
	m_ReadOnly(ReadOnly),
	m_hFile(NULL)
{
	
}

CSram::~CSram()
{
	if (m_hFile)
	{
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}
}

bool CSram::LoadSram()
{
	CPath FileName;

	FileName.SetDriveDirectory( g_Settings->LoadString(Directory_NativeSave).c_str());
	FileName.SetName(g_Settings->LoadString(Game_GameName).c_str());
	FileName.SetExtension("sra");

	if (!FileName.DirectoryExists())
	{
		FileName.CreateDirectory();
	}

	m_hFile = CreateFile(FileName,m_ReadOnly ? GENERIC_READ : GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE) 
	{
		WriteTraceF(TraceError,__FUNCTION__ ": Failed to open (%s), ReadOnly = %d, LastError = %X",(LPCTSTR)FileName, m_ReadOnly, GetLastError());
		return false;
	}
	SetFilePointer(m_hFile,0,NULL,FILE_BEGIN);	
	return true;
}

void CSram::DmaFromSram(BYTE * dest, int StartOffset, int len)
{
	DWORD dwRead;
	DWORD i;
	BYTE tmp[4];

	if (m_hFile == NULL)
	{
		if (!LoadSram())
		{
			return;
		}
	}
	
	// Fix Dezaemon 3D saves
	if ((StartOffset >= 0x00000000) && (StartOffset < 0x00008000))
	{
		//StartOffset = StartOffset;
	}
	else if ((StartOffset >= 0x00040000) && (StartOffset < 0x00048000))
	{
		StartOffset -= 0x38000;
	}
	else if ((StartOffset >= 0x00080000) && (StartOffset < 0x00088000))
	{
		StartOffset -= 0x70000;
	}
	else if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
	{
		g_Notify->DisplayError(L"DmaFromSram: %08X", StartOffset);
	}
	
	DWORD Offset = StartOffset & 3;

	if (Offset == 0)
	{
		SetFilePointer(m_hFile, StartOffset, NULL, FILE_BEGIN);
		ReadFile(m_hFile, dest, len, &dwRead, NULL);
	}
	else
	{
		SetFilePointer(m_hFile, StartOffset - Offset, NULL, FILE_BEGIN);

		ReadFile(m_hFile, tmp, 4, &dwRead, NULL);
		for (i = 0; i < (4 - Offset); i++)
		{
			dest[i + Offset] = tmp[i];
		}
		for (i = 4 - Offset; i < len - Offset; i += 4)
		{
			ReadFile(m_hFile, tmp, 4, &dwRead, NULL);
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
		ReadFile(m_hFile, tmp, 4, &dwRead, NULL);
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

void CSram::DmaToSram(BYTE * Source, int StartOffset, int len)
{
	DWORD dwWritten;
	DWORD i;
	BYTE tmp[4];

	if (m_ReadOnly)
	{
		return;
	}

	if (m_hFile == NULL)
	{
		if (!LoadSram())
		{
			return;
		}
	}

	// Fix Dezaemon 3D saves
	if ((StartOffset >= 0x00000000) && (StartOffset < 0x00008000))
	{
		//StartOffset = StartOffset;
	}
	else if ((StartOffset >= 0x00040000) && (StartOffset < 0x00048000))
	{
		StartOffset -= 0x38000;
	}
	else if ((StartOffset >= 0x00080000) && (StartOffset < 0x00088000))
	{
		StartOffset -= 0x70000;
	}
	else if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
	{
		g_Notify->DisplayError(L"DmaToSram: %08X", StartOffset);
	}
	
	DWORD Offset = StartOffset & 3;
	
	if (Offset == 0)
	{
		SetFilePointer(m_hFile, StartOffset, NULL, FILE_BEGIN);
		WriteFile(m_hFile, Source, len, &dwWritten, NULL);
	}
	else
	{
		for (i = 0; i < (4 - Offset); i++)
		{
			tmp[i] = Source[i + Offset];
		}
		SetFilePointer(m_hFile, StartOffset - Offset, NULL, FILE_BEGIN);
		WriteFile(m_hFile, tmp, (4 - Offset), &dwWritten, NULL);

		SetFilePointer(m_hFile, Offset, NULL, FILE_CURRENT);
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
			WriteFile(m_hFile, tmp, 4, &dwWritten, NULL);
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
		SetFilePointer(m_hFile, 4 - Offset, NULL, FILE_CURRENT);
		WriteFile(m_hFile, tmp, Offset, &dwWritten, NULL);
	}
	FlushFileBuffers(m_hFile);
}
