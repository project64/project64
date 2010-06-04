#include "stdafx.h"

static HANDLE hSramFile = NULL;

void CloseSram (void) {
	if (hSramFile) {
		CloseHandle(hSramFile);
		hSramFile = NULL;
	}
}

BOOL LoadSram (void) {
	char File[255], Directory[255];
	LPVOID lpMsgBuf;

	GetAutoSaveDir(Directory);
	sprintf(File,"%s%s.sra",Directory,g_RomName);
	
	hSramFile = CreateFile(File,GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,NULL,OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (hSramFile == INVALID_HANDLE_VALUE) {
		switch (GetLastError()) {
		case ERROR_PATH_NOT_FOUND:
			CreateDirectory(Directory,NULL);
			hSramFile = CreateFile(File,GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
				NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
			if (hSramFile == INVALID_HANDLE_VALUE) {
				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf,0,NULL 
				);
				DisplayError((const char *)lpMsgBuf);
				LocalFree( lpMsgBuf );
				return FALSE;
			}
			break;
		default:
			FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf,0,NULL 
			);
			DisplayError((const char *)lpMsgBuf);
			LocalFree( lpMsgBuf );
			return FALSE;
		}
	}
	return TRUE;
}

void DmaFromSram(BYTE * dest, int StartOffset, int len) {
	DWORD dwRead;

	if (hSramFile == NULL) {
		if (!LoadSram()) {
			return;
		}
	}
	SetFilePointer(hSramFile,StartOffset,NULL,FILE_BEGIN);	
	ReadFile(hSramFile,dest,len,&dwRead,NULL);

}

void DmaToSram(BYTE * Source, int StartOffset, int len) {
	DWORD dwWritten;

	if (hSramFile == NULL) {
		if (!LoadSram()) {
			return;
		}
	}
	SetFilePointer(hSramFile,StartOffset,NULL,FILE_BEGIN);	
	WriteFile(hSramFile,Source,len,&dwWritten,NULL);
}
