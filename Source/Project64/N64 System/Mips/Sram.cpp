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
	m_hFile(NULL),
	m_ReadOnly(ReadOnly)
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

BOOL CSram::LoadSram()
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

	if (m_hFile == NULL)
	{
		if (!LoadSram())
		{
			return;
		}
	}
	SetFilePointer(m_hFile,StartOffset,NULL,FILE_BEGIN);	
	ReadFile(m_hFile,dest,len,&dwRead,NULL);

}

void CSram::DmaToSram(BYTE * Source, int StartOffset, int len)
{
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
	DWORD dwWritten;
	SetFilePointer(m_hFile,StartOffset,NULL,FILE_BEGIN);	
	WriteFile(m_hFile,Source,len,&dwWritten,NULL);
	FlushFileBuffers(m_hFile);
}
