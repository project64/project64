#include "stdafx.h"

#ifdef toremove

#include "Eeprom.h"
#include "mempak.h"

void ProcessControllerCommand ( int Control, BYTE * Command);
void ReadControllerCommand (int Control, BYTE * Command);


#define IncreaseMaxPif2 300
int MaxPif2Cmds = 300;
unsigned __int64 * Pif2Reply[4];

BOOLEAN pif2valid = FALSE;

void LoadPIF2 () {
	CPath Pif2FileName(CPath::MODULE_DIRECTORY,"pif2.dat");

	FILE *pif2db = fopen (Pif2FileName, "rt");
//	unsigned __int64 p1, p2, r1, r2;
	char buff[255];
	int cnt = 0;
	
	Pif2Reply[0] = new unsigned __int64[MaxPif2Cmds + 1]; 
	Pif2Reply[1] = new unsigned __int64[MaxPif2Cmds + 1]; 
	Pif2Reply[2] = new unsigned __int64[MaxPif2Cmds + 1]; 
	Pif2Reply[3] = new unsigned __int64[MaxPif2Cmds + 1]; 

	if (Pif2Reply[0] == NULL || Pif2Reply[1] == NULL ||
		Pif2Reply[2] == NULL || Pif2Reply[3] == NULL) 
	{
		DisplayError(GS(MSG_MEM_ALLOC_ERROR));
		ExitThread(0);
	}
	if (pif2db == NULL) {
		Pif2Reply[0][0] = -1; Pif2Reply[1][0] = -1; 
		Pif2Reply[2][0] = -1; Pif2Reply[3][0] = -1;
		return;
	}
	
	while (feof (pif2db) == 0) {
		if (cnt == MaxPif2Cmds) {
			/*MaxPif2Cmds += IncreaseMaxPif2;
			Pif2Reply[0] = realloc(Pif2Reply[0],(MaxPif2Cmds + 1) * sizeof(__int64));
			Pif2Reply[1] = realloc(Pif2Reply[1],(MaxPif2Cmds + 1) * sizeof(__int64));
			Pif2Reply[2] = realloc(Pif2Reply[2],(MaxPif2Cmds + 1) * sizeof(__int64));
			Pif2Reply[3] = realloc(Pif2Reply[3],(MaxPif2Cmds + 1) * sizeof(__int64));
			if (Pif2Reply[0] == NULL || Pif2Reply[1] == NULL ||
				Pif2Reply[2] == NULL || Pif2Reply[3] == NULL) 
			{*/
				DisplayError(GS(MSG_MEM_ALLOC_ERROR));
				ExitThread(0);
			//}
		}
		fgets (buff, 255, pif2db);
		if (buff[0] != ';') {
			if (buff[0] != '\n') {
				sscanf (buff, "0x%08X%08X, 0x%08X%08X, 0x%08X%08X, 0x%08X%08X", 
				((DWORD *)&Pif2Reply[0][cnt])+1, &Pif2Reply[0][cnt], 
				((DWORD *)&Pif2Reply[1][cnt])+1, &Pif2Reply[1][cnt],
				((DWORD *)&Pif2Reply[2][cnt])+1, &Pif2Reply[2][cnt], 
				((DWORD *)&Pif2Reply[3][cnt])+1, &Pif2Reply[3][cnt]);
				cnt++;
			}
		}
	}

	Pif2Reply[0][cnt] = Pif2Reply[1][cnt] = Pif2Reply[2][cnt] = Pif2Reply[3][cnt] = -1;
	
	pif2valid = TRUE;

	fclose (pif2db);
}


int LoadPifRom(int country) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT], PifRomName[255];
	HANDLE hPifFile;
	DWORD dwRead;

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );

	switch (country) {
	case 0x44: //Germany
	case 0x46: //french
	case 0x49: //Italian
	case 0x50: //Europe
	case 0x53: //Spanish
	case 0x55: //Australia
	case 0x58: // X (PAL)
	case 0x59: // X (PAL)
		sprintf(PifRomName,"%s%sPifrom\\pifpal.raw",drive,dir);
		break;
	case 0: //None
	case 0x37: // 7 (Beta)
	case 0x41: // ????
	case 0x45: //USA
	case 0x4A: //Japan
		sprintf(PifRomName,"%s%sPifrom\\pifntsc.raw",drive,dir);
		break;
	default:
		DisplayError(GS(MSG_UNKNOWN_COUNTRY));
	}
	

	hPifFile = CreateFile(PifRomName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (hPifFile == INVALID_HANDLE_VALUE) {
		memset(PifRom,0,0x7C0);
		return FALSE;
	}
	SetFilePointer(hPifFile,0,NULL,FILE_BEGIN);
	ReadFile(hPifFile,PifRom,0x7C0,&dwRead,NULL);	
	CloseHandle( hPifFile );
	return TRUE;
}

#endif