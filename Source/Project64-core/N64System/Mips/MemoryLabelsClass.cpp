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

DWORD CMemoryLabel::AsciiToHex (char * HexValue)
{
	DWORD Count, Finish, Current, Value = 0;

	Finish = strlen(HexValue);
	if (Finish > 8 )
	{
		Finish = 8;
	}

	for (Count = 0; Count < Finish; Count++)
	{
		Value = (Value << 4);
		Current = HexValue[Count];
		if(Current >= '0' && Current <= '9')
		{
			Value += Current - '0';
		}
		else
		{
			if(Current > 'F')
				Current -= 32; //32 is the distance between A and a

			if (Current >= 'A' && Current <= 'F')
			{
				Value += Current - 55; //55 is the code for 'A' less 10
			}
			else
			{
				Value = (Value >> 4);
				Count = Finish;
			}
		}
	}
	return Value;
}

void CMemoryLabel::AddMemoryLabel ( DWORD Address, const char * Message, ... )
{
	StringMap::iterator Item = m_LabelList.find(Address);
	if (Item == m_LabelList.end())
	{
		char Msg[1000];
		va_list ap;

		va_start( ap, Message );
		_vsnprintf( Msg,sizeof(Msg),Message, ap );
		va_end( ap );

		//if item is already in the list then do not add it
		m_LabelList.insert(StringMap::value_type(Address,stdstr(Msg)));
		m_NewLabels += 1;
	}
}

stdstr CMemoryLabel::LabelName ( DWORD Address ) const
{
	//StringMap::iterator theIterator = m_LabelList.find(Address);
	//if (theIterator != m_LabelList.end())
	//{
	//	return (*theIterator).second;
	//}

	char strLabelName[100];
	sprintf(strLabelName,"0x%08X",Address);
	return stdstr(strLabelName);
}

stdstr CMemoryLabel::StoredLabelName ( DWORD Address )
{
	StringMap::iterator theIterator = m_LabelList.find(Address);
	if (theIterator != m_LabelList.end())
	{
		return (*theIterator).second;
	}
	return stdstr("");
}

void CMemoryLabel::LoadLabelList ( char * file )
{
	m_LabelList.clear();
	CurrentLabelFile = file;

	HANDLE hFile = CreateFile(file,GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
		OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	SetFilePointer(hFile,0,NULL,FILE_BEGIN);

	DWORD FileSize = GetFileSize(hFile,NULL);
	void * FileContents = VirtualAlloc(NULL,FileSize,MEM_COMMIT,PAGE_READWRITE );

	if (FileContents)
	{
		DWORD dwRead;
		if (!ReadFile(hFile,FileContents,FileSize,&dwRead,NULL))
		{
			VirtualFree(FileContents, 0, MEM_RELEASE);
			FileContents = NULL;
		}
	}

	if (FileContents)
	{
		ProcessCODFile((BYTE *)FileContents, FileSize);
	}

	VirtualFree(FileContents, 0, MEM_RELEASE);
	CloseHandle(hFile);

	m_NewLabels = 0;
}

// How many new labels been added since loading/saving label file
int  CMemoryLabel::NewLabels()
{
	return m_NewLabels;
}

void CMemoryLabel::SaveLabelList()
{
	m_NewLabels = 0;

	if (CurrentLabelFile.length() == 0)
	{
		return;
	}

	HANDLE hFile = CreateFile(CurrentLabelFile.c_str(),GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
		CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetFilePointer(hFile,0,NULL,FILE_BEGIN);

	for (StringMap::iterator Item = m_LabelList.begin(); Item != m_LabelList.end(); Item++)
	{
		char Text[300];
		DWORD dwWritten;

		sprintf(Text, "0x%08X,%s\r\n",(*Item).first,((*Item).second).c_str());

		WriteFile( hFile,Text,strlen(Text),&dwWritten,NULL );
	}

	CloseHandle(hFile);

}

void CMemoryLabel::ProcessCODFile(BYTE * File, DWORD FileLen)
{
	char * CurrentPos = (char *)File;
	char Label[40];
	DWORD Address;
	int Length;

	while ( CurrentPos < (char *)File + FileLen )
	{
		if (*CurrentPos != '0')
		{
			return;
		}
		CurrentPos += 1;
		if (*CurrentPos != 'x')
		{
			return;
		}
		CurrentPos += 1;

		if (strchr(CurrentPos,',') - CurrentPos != 8)
		{
			return;
		}
		Address = AsciiToHex (CurrentPos);
		CurrentPos += 9;


		if (strchr(CurrentPos,'\r') == NULL)
		{
			Length = strchr(CurrentPos,'\n') - CurrentPos;
		}
		else
		{
			Length = strchr(CurrentPos,'\r') - CurrentPos;
			if (Length > (strchr(CurrentPos,'\n') - CurrentPos))
			{
				Length = strchr(CurrentPos,'\n') - CurrentPos;
			}
		}

		// Stay within label array bounds
		if (Length > 39)
			Length = 39;

		memcpy(Label,CurrentPos,Length);
		Label[Length] = '\0';

		AddMemoryLabel (Address, Label);
		CurrentPos = strchr(CurrentPos,'\n') + 1;
	}
}
