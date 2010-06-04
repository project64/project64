#include "stdafx.h"

typedef enum TFlashRam_Modes {
	FLASHRAM_MODE_NOPES = 0,
	FLASHRAM_MODE_ERASE = 1,
	FLASHRAM_MODE_WRITE,
	FLASHRAM_MODE_READ,
	FLASHRAM_MODE_STATUS,
};

BOOL LoadFlashram (void);

DWORD FlashRAM_Offset, FlashFlag = FLASHRAM_MODE_NOPES;
static HANDLE hFlashRamFile = NULL;
BYTE * FlashRamPointer;
QWORD FlashStatus = 0;

void CloseFlashRam ( void)
{
	if (hFlashRamFile) {
		CloseHandle(hFlashRamFile);
		hFlashRamFile = NULL;
	}
}

void DmaFromFlashram(BYTE * dest, int StartOffset, int len) {
	BYTE FlipBuffer[0x10000];
	DWORD dwRead, count;

	switch (FlashFlag) {
	case FLASHRAM_MODE_READ:
		if (hFlashRamFile == NULL) {
			if (!LoadFlashram()) { return; }
		}
		if (len > 0x10000) { 
#ifndef EXTERNAL_RELEASE
			DisplayError("DmaFromFlashram FlipBuffer to small (len: %d)",len); 
#endif
			len = 0x10000;
		}
		if ((len & 3) != 0) {
#ifndef EXTERNAL_RELEASE
			DisplayError("Unaligned flash ram read ???");
#endif
			return;
		}
		memset(FlipBuffer,0,sizeof(FlipBuffer));
		StartOffset = StartOffset << 1;
		SetFilePointer(hFlashRamFile,StartOffset,NULL,FILE_BEGIN);	
		ReadFile(hFlashRamFile,FlipBuffer,len,&dwRead,NULL);
		for (count = dwRead; (int)count < len; count ++) {
			FlipBuffer[count] = 0xFF;
		}
		_asm {
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
		if (StartOffset != 0 && len != 8) {
#ifndef EXTERNAL_RELEASE
			DisplayError("Reading flashstatus not being handled correctly\nStart: %X len: %X",StartOffset,len);
#endif
		}
		*((DWORD *)(dest)) = (DWORD)(FlashStatus >> 32);
		*((DWORD *)(dest) + 1) = (DWORD)(FlashStatus);
		break;
#ifndef EXTERNAL_RELEASE
	default:
		DisplayError("DmaFromFlashram Start: %X, Offset: %X len: %X",dest - _MMU->Rdram(),StartOffset,len);
#endif
	}
}

void DmaToFlashram(BYTE * Source, int StartOffset, int len) {
	switch (FlashFlag) {
	case FLASHRAM_MODE_WRITE:
		FlashRamPointer = Source;
		break;
#ifndef EXTERNAL_RELEASE
	default:
		DisplayError("DmaToFlashram Start: %X, Offset: %X len: %X",Source - _MMU->Rdram(),StartOffset,len);
#endif
	}
}

DWORD ReadFromFlashStatus (DWORD PAddr) {
	switch (PAddr) {
	case 0x08000000: return (DWORD)(FlashStatus >> 32);
	default:
#ifndef EXTERNAL_RELEASE
		DisplayError("Reading from flash ram status (%X)",PAddr);
#endif
		break;
	}
	return (DWORD)(FlashStatus >> 32);
}

BOOL LoadFlashram (void) {
	char File[255], Directory[255];

	GetAutoSaveDir(Directory);
	sprintf(File,"%s%s.fla",Directory,g_RomName);
	
	hFlashRamFile = CreateFile(File,GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,NULL,OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (hFlashRamFile == INVALID_HANDLE_VALUE) {
		switch (GetLastError()) {
		case ERROR_PATH_NOT_FOUND:
			CreateDirectory(Directory,NULL);
			hFlashRamFile = CreateFile(File,GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
				NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
			if (hFlashRamFile == INVALID_HANDLE_VALUE) {
				DisplayError(GS(MSG_FAIL_OPEN_FLASH));
				return FALSE;
			}
			break;
		default:
			DisplayError(GS(MSG_FAIL_OPEN_FLASH));
			return FALSE;
		}
	}
	return TRUE;
}

void WriteToFlashCommand(DWORD FlashRAM_Command) {
	BYTE EmptyBlock[128];
	DWORD dwWritten;

	switch (FlashRAM_Command & 0xFF000000) {
	case 0xD2000000: 
		switch (FlashFlag) {
		case FLASHRAM_MODE_NOPES: break;
		case FLASHRAM_MODE_READ: break;
		case FLASHRAM_MODE_STATUS: break;
		case FLASHRAM_MODE_ERASE:
			memset(EmptyBlock,0xFF,sizeof(EmptyBlock));
			if (hFlashRamFile == NULL) {
				if (!LoadFlashram()) { return; }
			}
			SetFilePointer(hFlashRamFile,FlashRAM_Offset,NULL,FILE_BEGIN);	
			WriteFile(hFlashRamFile,EmptyBlock,128,&dwWritten,NULL);
			break;
		case FLASHRAM_MODE_WRITE:
			if (hFlashRamFile == NULL) {
				if (!LoadFlashram()) { return; }
			}
			{
				BYTE FlipBuffer[128];
				DWORD dwWritten;

				memset(FlipBuffer,0,sizeof(FlipBuffer));
				_asm {
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

				SetFilePointer(hFlashRamFile,FlashRAM_Offset,NULL,FILE_BEGIN);	
				WriteFile(hFlashRamFile,FlipBuffer,128,&dwWritten,NULL);
			}
			break;
		default:
			DisplayError("Writing %X to flash ram command register\nFlashFlag: %d",FlashRAM_Command,FlashFlag);
		}
		FlashFlag = FLASHRAM_MODE_NOPES;
		break;
	case 0xE1000000: 
		FlashFlag = FLASHRAM_MODE_STATUS;
		FlashStatus = 0x1111800100C20000;
		break;
	case 0xF0000000: 
		FlashFlag = FLASHRAM_MODE_READ;
		FlashStatus = 0x11118004F0000000;
		break;
	case 0x4B000000:
		FlashRAM_Offset = (FlashRAM_Command & 0xffff) * 128;
		break;
	case 0x78000000:
		FlashFlag = FLASHRAM_MODE_ERASE;
		FlashStatus = 0x1111800800C20000;
		break;
	case 0xB4000000: 
		FlashFlag = FLASHRAM_MODE_WRITE; //????
		break;
	case 0xA5000000:
		FlashRAM_Offset = (FlashRAM_Command & 0xffff) * 128;
		FlashStatus = 0x1111800400C20000;
		break;
#ifndef EXTERNAL_RELEASE
	default:
		DisplayError("Writing %X to flash ram command register",FlashRAM_Command);
#endif
	}
}

