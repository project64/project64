#ifndef _GBCART_H_
#define _GBCART_H_

#include <time.h>

#include <windows.h>

typedef struct _gbCartRTC {
  UINT mapperSeconds;
  UINT mapperMinutes;
  UINT mapperHours;
  UINT mapperDays;
  UINT mapperControl;
  UINT mapperLSeconds;
  UINT mapperLMinutes;
  UINT mapperLHours;
  UINT mapperLDays;
  UINT mapperLControl;
  time_t mapperLastTime;
} gbCartRTC, *lpgbCartRTC;

typedef struct _GBCART
{
    unsigned int iCurrentRomBankNo;
    unsigned int iCurrentRamBankNo;
    int iCartType;
    bool bHasRam;
    bool bHasBattery;
    bool bHasTimer;
    bool bHasRumble;
    bool bRamEnableState;
    bool bMBC1RAMbanking;   // If false, use 2 magic bits for most significant bits of ROM banking (default); if true, use the 2 magic bits for RAM banking
    unsigned int iNumRomBanks;
    unsigned int iNumRamBanks;
    BYTE TimerData[5];
    BYTE LatchedTimerData[5];
    time_t timerLastUpdate;
    bool TimerDataLatched;
    HANDLE hRomFile;        // A file mapping handle
    HANDLE hRamFile;        // A file mapping handle, must be NULL if malloc'd RAM is being used instead of a valid memory mapped file
    const unsigned char * RomData;      // max [0x200 * 0x4000];
    LPBYTE RamData;         // max [0x10 * 0x2000];
    bool (*ptrfnReadCart)(_GBCART * Cart, WORD dwAddress, BYTE *Data);  // ReadCart handler
    bool (*ptrfnWriteCart)(_GBCART * Cart, WORD dwAddress, BYTE *Data); // WriteCart handler
} GBCART, *LPGBCART;

bool LoadCart(LPGBCART Cart, LPCTSTR RomFile, LPCTSTR RamFile, LPCTSTR TdfFile);
// bool ReadCart(LPGBCART Cart, WORD dwAddress, BYTE *Data);
// bool WriteCart(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool SaveCart(LPGBCART Cart, LPTSTR SaveFile, LPTSTR TimeFile);
bool UnloadCart(LPGBCART Cart);

/*
iCartType values:
0 = no MBC
1 = MBC1
2 = MBC2
3 = MMMO1
4 = MBC3
5 = MBC5
6 = Pocket Camera
7 = TAMA 5
8 = HuC 3
9 = HuC 1
TODO: Note, that 7 and up are not implemented yet.
*/

#define GB_NORM     0x00
#define GB_MBC1     0x01
#define GB_MBC2     0x02
#define GB_MMMO1    0x03
#define GB_MBC3     0x04
#define GB_MBC5     0x05
#define GB_CAMERA   0x06
#define GB_TAMA5    0x07
#define GB_HUC3     0x08
#define GB_HUC1     0x09

#endif // #ifndef _GBCART_H_
