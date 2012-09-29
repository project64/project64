#include "stdafx.h"
#include <time.h>

CEeprom::CEeprom (bool ReadOnly):
	m_ReadOnly(ReadOnly),
	m_hFile(NULL)
{
	memset(m_EEPROM,0,sizeof(m_EEPROM));
	//LoadEeprom();
}

CEeprom::~CEeprom (void) {
	if (m_hFile) {
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}
}

unsigned char byte2bcd(int n)
{
	n %= 100;
	return ((n / 10) << 4) | (n % 10);
}

void CEeprom::EepromCommand ( BYTE * Command) {
	time_t curtime_time;
	struct tm curtime;

	if (g_SaveUsing == SaveChip_Auto) { g_SaveUsing = SaveChip_Eeprom_4K; }

	switch (Command[2]) {
	case 0: // check
		if (g_SaveUsing != SaveChip_Eeprom_4K &&  g_SaveUsing != SaveChip_Eeprom_16K) {
			Command[1] |= 0x80;
			break;
		}
		if (Command[1] != 3)
		{ 
			Command[1] |= 0x40; 
			if ((Command[1] & 3) > 0)
				Command[3] = 0x00;
			if ((Command[1] & 3) > 1)
				 Command[4] = (g_SaveUsing == SaveChip_Eeprom_4K) ? 0x80 : 0xC0;
			if ((Command[1] & 3) > 2)
				Command[5] = 0x00;
		} else {
			Command[3] = 0x00;
			Command[4] = g_SaveUsing == SaveChip_Eeprom_4K?0x80:0xC0;
			Command[5] = 0x00;
		}
		break;
	case 4: // Read from Eeprom
#ifndef EXTERNAL_RELEASE
		if (Command[0] != 2) { _Notify->DisplayError("What am I meant to do with this Eeprom Command"); }
		if (Command[1] != 8) { _Notify->DisplayError("What am I meant to do with this Eeprom Command"); }
#endif
		ReadFrom(&Command[4],Command[3]);
		break;
	case 5:
#ifndef EXTERNAL_RELEASE
		if (Command[0] != 10) { _Notify->DisplayError("What am I meant to do with this Eeprom Command"); }
		if (Command[1] != 1) { _Notify->DisplayError("What am I meant to do with this Eeprom Command"); }
#endif
		WriteTo(&Command[4],Command[3]);
		break;
	case 6: //RTC Status query
		Command[3] = 0x00;
		Command[4] = 0x10;
		Command[12] = 0x00;
		break;
	case 7: //Read RTC
		switch(Command[3])
		{
			case 0:
				Command[4] = 0x00;
				Command[5] = 0x02;
				Command[12] = 0x00;
				break;
			case 1:
				//read block, Command[2], Unimplemented
				break;
			case 2: //Set RTC Time
				time(&curtime_time);
				memcpy(&curtime, localtime(&curtime_time), sizeof(curtime)); // fd's fix
				Command[4]  = byte2bcd(curtime.tm_sec);
				Command[5]  = byte2bcd(curtime.tm_min);
				Command[6]  = 0x80 + byte2bcd(curtime.tm_hour);
				Command[7]  = byte2bcd(curtime.tm_mday);
				Command[8]  = byte2bcd(curtime.tm_wday);
				Command[9]  = byte2bcd(curtime.tm_mon + 1);
				Command[10] = byte2bcd(curtime.tm_year);
				Command[11] = byte2bcd(curtime.tm_year / 100);
				Command[12] = 0x00;	// status
				break;
		}
		break;
	case 8:
		//Write RTC, unimplemented
		break;
	default:
		if (_Settings->LoadDword(Debugger_ShowPifErrors)) { _Notify->DisplayError("Unknown EepromCommand %d",Command[2]); }
	}
}

void CEeprom::LoadEeprom (void) {
	CPath FileName;
	DWORD dwRead;

	memset(m_EEPROM,0,sizeof(m_EEPROM));

	FileName.SetDriveDirectory( _Settings->LoadString(Directory_NativeSave).c_str());
	FileName.SetName(_Settings->LoadString(Game_GameName).c_str());
	FileName.SetExtension("eep");

	if (!FileName.DirectoryExists())
	{
		FileName.CreateDirectory();
	}

	m_hFile = CreateFile(FileName,m_ReadOnly ? GENERIC_READ : GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE) 
	{
		WriteTraceF(TraceError,"CEeprom::LoadEeprom: Failed to open (%s), ReadOnly = %d, LastError = %X",(LPCTSTR)FileName, m_ReadOnly, GetLastError());
		_Notify->DisplayError(GS(MSG_FAIL_OPEN_EEPROM));
		return;
	}
	SetFilePointer(m_hFile,0,NULL,FILE_BEGIN);	
	ReadFile(m_hFile,m_EEPROM,sizeof(m_EEPROM),&dwRead,NULL);
}

void CEeprom::ReadFrom(BYTE * Buffer, int line) {
	int i;
	
	if (m_hFile == NULL) 
	{
		LoadEeprom();
	}
	
	for(i=0; i < 8; i++) 
	{
		Buffer[i] = m_EEPROM[line*8+i]; 
	}
}

void CEeprom::WriteTo(BYTE * Buffer, int line) {
	DWORD dwWritten;
	int i;

	if (m_hFile == NULL) 
	{
		LoadEeprom();
	}
	for(i=0;i<8;i++) { m_EEPROM[line*8+i]=Buffer[i]; }
	SetFilePointer(m_hFile,line*8,NULL,FILE_BEGIN);	
	WriteFile( m_hFile,Buffer,8,&dwWritten,NULL );
	FlushFileBuffers(m_hFile);
}
