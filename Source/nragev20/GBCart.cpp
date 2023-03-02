/*
This file's purpose is to emulate the inner workings of a
Game Boy game pak cartridge. All code is by Mark McGough.
*/

#include <windows.h>

#include "commonIncludes.h"
#include "GBCart.h"
#include "NRagePluginV2.h"
#include "PakIO.h"

void ClearData(BYTE *Data, int Length);

bool ReadCartNorm(LPGBCART Cart, WORD dwAddress, BYTE *Data); // For all non-MBC carts; fixed 0x8000 ROM; fixed, optional 0x2000 RAM
bool WriteCartNorm(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool ReadCartMBC1(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool WriteCartMBC1(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool ReadCartMBC2(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool WriteCartMBC2(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool ReadCartMBC3(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool WriteCartMBC3(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool ReadCartMBC5(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool WriteCartMBC5(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool ReadCartCamera(LPGBCART Cart, WORD dwAddress, BYTE *Data);
bool WriteCartCamera(LPGBCART Cart, WORD dwAddress, BYTE *Data);

// Tries to read RTC data from separate file (not integrated into SAV)
// Success sets the useTDF flag
// Failure initializes the RTC at zero and maybe throws a warning
void ReadTDF(LPGBCART Cart)
{
}

void WriteTDF(LPGBCART Cart)
{
    // Check useTDF flag
    // Write data from RTC to TDF file
}

void UpdateRTC(LPGBCART Cart)
{
    time_t now, dif;
    int days;

    now = time(NULL);
    dif = now - Cart->timerLastUpdate;

    Cart->TimerData[0] += (BYTE)(dif % 60);
    dif /= 60;
    Cart->TimerData[1] += (BYTE)(dif % 60);
    dif /= 60;
    Cart->TimerData[2] += (BYTE)(dif % 24);
    dif /= 24;

    days = (int)(Cart->TimerData[3] + ((Cart->TimerData[4] & 1) << 8) + dif);
    Cart->TimerData[3] = (days & 0xFF);

    if (days > 255)
    {
        if (days > 511)
        {
            days &= 511;
            Cart->TimerData[4] |= 0x80;
        }
        if (days > 255)
        {
            Cart->TimerData[4] = (Cart->TimerData[4] & 0xFE) | (days > 255 ? 1 : 0);
        }
    }

    Cart->timerLastUpdate = now;

}

// Returns true if the ROM was loaded correctly
bool LoadCart(LPGBCART Cart, LPCTSTR RomFileName, LPCTSTR RamFileName, LPCTSTR TdfFileName)
{
    HANDLE hTemp;
    DWORD dwFilesize;
    DWORD NumQuarterBlocks = 0;

    UnloadCart(Cart);   // First, make sure any previous carts have been unloaded

    Cart->iCurrentRamBankNo = 0;
    Cart->iCurrentRomBankNo = 1;
    Cart->bRamEnableState = 0;
    Cart->bMBC1RAMbanking = 0;

    // Attempt to load the ROM file
    hTemp = CreateFile(RomFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hTemp != INVALID_HANDLE_VALUE && (Cart->hRomFile = CreateFileMapping(hTemp, NULL, PAGE_READONLY, 0, 0, NULL) ) )
    {
        // If the first case fails, the file doesn't exist. The second case can fail if the file size is zero.
        dwFilesize = GetFileSize(hTemp, NULL);
        CloseHandle(hTemp);
        Cart->RomData = (const unsigned char *)MapViewOfFile( Cart->hRomFile, FILE_MAP_READ, 0, 0, 0 );
    }
    else
    {
        DebugWriteA("Couldn't load the ROM file, GetLastError returned %08x\n", GetLastError());
        if (hTemp != INVALID_HANDLE_VALUE)
            CloseHandle(hTemp); // If file size was zero, make sure we don't leak the handle

        ErrorMessage(IDS_ERR_GBROM, 0, false);
        return false;
    }

    if (dwFilesize < 0x8000) // A ROM file has to be at least 32KB
    {
        DebugWriteA("ROM file wasn't big enough to be a Game Boy ROM!\n");
        ErrorMessage(IDS_ERR_GBROM, 0, false);

        UnloadCart(Cart);
        return false;
    }

    DebugWriteA(" cartridge type #:");
    DebugWriteByteA(Cart->RomData[0x147]);
    DebugWriteA("\n");
    switch (Cart->RomData[0x147])
    { // If we hadn't checked the file size before, this might have caused an access violation
    case 0x00:
        Cart->iCartType = GB_NORM;
        Cart->bHasRam = false;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x01:
        Cart->iCartType = GB_MBC1;
        Cart->bHasRam = false;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x02:
        Cart->iCartType = GB_MBC1;
        Cart->bHasRam = true;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x03:
        Cart->iCartType = GB_MBC1;
        Cart->bHasRam = true;
        Cart->bHasBattery = true;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x05:
        Cart->iCartType = GB_MBC2;
        Cart->bHasRam = false;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x06:
        Cart->iCartType = GB_MBC2;
        Cart->bHasRam = false;
        Cart->bHasBattery = true;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x08:
        Cart->iCartType = GB_NORM;
        Cart->bHasRam = true;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x09:
        Cart->iCartType = GB_NORM;
        Cart->bHasRam = true;
        Cart->bHasBattery = true;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x0B:
        Cart->iCartType = GB_MMMO1;
        Cart->bHasRam = false;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x0C:
        Cart->iCartType = GB_MMMO1;
        Cart->bHasRam = true;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x0D:
        Cart->iCartType = GB_MMMO1;
        Cart->bHasRam = true;
        Cart->bHasBattery = true;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x0F:
        Cart->iCartType = GB_MBC3;
        Cart->bHasRam = false;
        Cart->bHasBattery = true;
        Cart->bHasTimer = true;
        Cart->bHasRumble = false;
        break;
    case 0x10:
        Cart->iCartType = GB_MBC3;
        Cart->bHasRam = true;
        Cart->bHasBattery = true;
        Cart->bHasTimer = true;
        Cart->bHasRumble = false;
        break;
    case 0x11:
        Cart->iCartType = GB_MBC3;
        Cart->bHasRam = false;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x12:
        Cart->iCartType = GB_MBC3;
        Cart->bHasRam = true;
        Cart->bHasBattery = true;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x13:
        Cart->iCartType = GB_MBC3;
        Cart->bHasRam = true;
        Cart->bHasBattery = true;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x19:
        Cart->iCartType = GB_MBC5;
        Cart->bHasRam = false;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x1A:
        Cart->iCartType = GB_MBC5;
        Cart->bHasRam = true;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x1B:
        Cart->iCartType = GB_MBC5;
        Cart->bHasRam = true;
        Cart->bHasBattery = true;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    case 0x1C:
        Cart->iCartType = GB_MBC5;
        Cart->bHasRam = false;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = true;
        break;
    case 0x1D:
        Cart->iCartType = GB_MBC5;
        Cart->bHasRam = true;
        Cart->bHasBattery = false;
        Cart->bHasTimer = false;
        Cart->bHasRumble = true;
        break;
    case 0x1E:
        Cart->iCartType = GB_MBC5;
        Cart->bHasRam = true;
        Cart->bHasBattery = true;
        Cart->bHasTimer = false;
        Cart->bHasRumble = true;
        break;
    case 0xFC:
        //GAME BOY CAMERA
        Cart->iCartType = GB_CAMERA;
        Cart->bHasRam = true;
        Cart->bHasBattery = true;
        Cart->bHasTimer = false;
        Cart->bHasRumble = false;
        break;
    default:
        WarningMessage( IDS_ERR_GBROM, MB_OK | MB_ICONWARNING);
        DebugWriteA("TPak: unsupported pak type\n");
        UnloadCart(Cart);
        return false;
    }

    // Assign read/write handlers
    switch (Cart->iCartType)
    {
    case GB_NORM: // Raw cartridge
        Cart->ptrfnReadCart = &ReadCartNorm;
        Cart->ptrfnWriteCart = &WriteCartNorm;
        break;
    case GB_MBC1:
        Cart->ptrfnReadCart =  &ReadCartMBC1;
        Cart->ptrfnWriteCart = &WriteCartMBC1;
        break;
    case GB_MBC2:
        Cart->ptrfnReadCart =  &ReadCartMBC2;
        Cart->ptrfnWriteCart = &WriteCartMBC2;
        break;
    case GB_MBC3:
        Cart->ptrfnReadCart =  &ReadCartMBC3;
        Cart->ptrfnWriteCart = &WriteCartMBC3;
        break;
    case GB_MBC5:
        Cart->ptrfnReadCart =  &ReadCartMBC5;
        Cart->ptrfnWriteCart = &WriteCartMBC5;
        break;
    case GB_CAMERA:
        Cart->ptrfnReadCart =  &ReadCartCamera;
        Cart->ptrfnWriteCart = &WriteCartCamera;
        break;
    default: // Don't pretend we know how to handle carts we don't support
        Cart->ptrfnReadCart = NULL;
        Cart->ptrfnWriteCart = NULL;
        DebugWriteA("Unsupported pak type: can't read/write cart type %02X\n", Cart->iCartType);
        UnloadCart(Cart);
        return false;
    }

    // Determine ROM size for paging checks
    Cart->iNumRomBanks = 2;
    switch (Cart->RomData[0x148])
    {
    case 0x01:
        Cart->iNumRomBanks = 4;
        break;
    case 0x02:
        Cart->iNumRomBanks = 8;
        break;
    case 0x03:
        Cart->iNumRomBanks = 16;
        break;
    case 0x04:
        Cart->iNumRomBanks = 32;
        break;
    case 0x05:
        Cart->iNumRomBanks = 64;
        break;
    case 0x06:
        Cart->iNumRomBanks = 128;
        break;
    case 0x52:
        Cart->iNumRomBanks = 72;
        break;
    case 0x53:
        Cart->iNumRomBanks = 80;
        break;
    case 0x54:
        Cart->iNumRomBanks = 96;
        break;
    }

    if (dwFilesize != 0x4000 * Cart->iNumRomBanks) // Now that we know how big the ROM is supposed to be, check it again
    {
        ErrorMessage(IDS_ERR_GBROM, 0, false);

        UnloadCart(Cart);
        return false;
    }

    // Determine RAM size for paging checks
    Cart->iNumRamBanks = 0;
    switch (Cart->RomData[0x149]) {
    case 0x01:
        Cart->iNumRamBanks = 1;
        NumQuarterBlocks = 1;
        break;
    case 0x02:
        Cart->iNumRamBanks = 1;
        NumQuarterBlocks = 4;
        break;
    case 0x03:
        Cart->iNumRamBanks = 4;
        NumQuarterBlocks = 16;
        break;
    case 0x04:
        Cart->iNumRamBanks = 16;
        NumQuarterBlocks = 64;
        break;
    case 0x05:
        Cart->iNumRamBanks = 8;
        NumQuarterBlocks = 32;
        break;
    }

    DebugWriteA("Game Boy cart has %d ROM banks, %d RAM quarter banks\n", Cart->iNumRomBanks, NumQuarterBlocks);
    if (Cart->bHasTimer)
    {
        DebugWriteA("Game Boy cart timer present\n");
    }

    // Attempt to load the SRAM file, but only if RAM is supposed to be present.
    // For saving back to a file, if we map too much it will expand the file.
    if (Cart->bHasRam)
    {
        if (Cart->bHasBattery)
        {
            hTemp = CreateFile( RamFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL );
            if( hTemp == INVALID_HANDLE_VALUE )
            {// Test if read-only access is possible
                hTemp = CreateFile( RamFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL );
                if (Cart->bHasTimer && Cart->bHasBattery)
                {
                    Cart->RamData = (LPBYTE)P_malloc(NumQuarterBlocks * 0x0800 + sizeof(gbCartRTC));
                    ClearData(Cart->RamData, NumQuarterBlocks * 0x0800 + sizeof(gbCartRTC));
                }
                else
                {
                    Cart->RamData = (LPBYTE)P_malloc(NumQuarterBlocks * 0x0800);
                    ClearData(Cart->RamData, NumQuarterBlocks * 0x0800);
                }

                if( hTemp != INVALID_HANDLE_VALUE )
                {
                    DWORD dwBytesRead;

                    if (Cart->bHasTimer && Cart->bHasBattery)
                        ReadFile(hTemp, Cart->RamData, NumQuarterBlocks * 0x0800 + sizeof(gbCartRTC), &dwBytesRead, NULL);
                    else
                        ReadFile(hTemp, Cart->RamData, NumQuarterBlocks * 0x0800, &dwBytesRead, NULL);
                    WarningMessage( IDS_DLG_TPAK_READONLY, MB_OK | MB_ICONWARNING);
                }
                else
                {
                    WarningMessage( IDS_ERR_GBSRAMERR, MB_OK | MB_ICONWARNING);
                    return true;
                }
            }
            else
            { // File is OK, use a mapping
                if (Cart->bHasTimer && Cart->bHasBattery)
                    Cart->hRamFile = CreateFileMapping( hTemp, NULL, PAGE_READWRITE, 0, NumQuarterBlocks * 0x0800 + sizeof(gbCartRTC), NULL);
                else
                    Cart->hRamFile = CreateFileMapping( hTemp, NULL, PAGE_READWRITE, 0, NumQuarterBlocks * 0x0800, NULL);

                if (Cart->hRamFile != NULL)
                {
                    Cart->RamData = (LPBYTE)MapViewOfFile( Cart->hRamFile, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
                }
                else
                { // Could happen, if the file isn't big enough and can't be grown to fit
                    DWORD dwBytesRead;
                    if (Cart->bHasTimer && Cart->bHasBattery)
                    {
                        Cart->RamData = (LPBYTE)P_malloc(NumQuarterBlocks * 0x0800 + sizeof(gbCartRTC));
                        ReadFile(hTemp, Cart->RamData, NumQuarterBlocks * 0x0800 + sizeof(gbCartRTC), &dwBytesRead, NULL);
                    }
                    else
                    {
                        Cart->RamData = (LPBYTE)P_malloc(NumQuarterBlocks * 0x0800);
                        ReadFile(hTemp, Cart->RamData, NumQuarterBlocks * 0x0800, &dwBytesRead, NULL);
                    }

                    if (dwBytesRead < NumQuarterBlocks * 0x0800 + ((Cart->bHasTimer && Cart->bHasBattery) ? sizeof(gbCartRTC) : 0))
                    {
                        ClearData(Cart->RamData, NumQuarterBlocks * 0x0800 + ((Cart->bHasTimer && Cart->bHasBattery) ? sizeof(gbCartRTC) : 0));
                        WarningMessage( IDS_ERR_GBSRAMERR, MB_OK | MB_ICONWARNING);
                    }
                    else
                    {
                        WarningMessage( IDS_DLG_TPAK_READONLY, MB_OK | MB_ICONWARNING);
                    }
                }
            }

            if (Cart->bHasTimer && Cart->bHasBattery)
            {
                dwFilesize = GetFileSize(hTemp, 0);
                if (dwFilesize >= (NumQuarterBlocks * 0x0800 + sizeof(gbCartRTC) ) )
                {
                    // Looks like there is extra data in the SAV file than just RAM data...assume it is RTC data
                    gbCartRTC RTCTimer;
                    CopyMemory( &RTCTimer, &Cart->RamData[NumQuarterBlocks * 0x0800], sizeof(RTCTimer) );
                    Cart->TimerData[0] = RTCTimer.mapperSeconds;
                    Cart->TimerData[1] = RTCTimer.mapperMinutes;
                    Cart->TimerData[2] = RTCTimer.mapperHours;
                    Cart->TimerData[3] = RTCTimer.mapperDays;
                    Cart->TimerData[4] = RTCTimer.mapperControl;
                    Cart->LatchedTimerData[0] = RTCTimer.mapperLSeconds;
                    Cart->LatchedTimerData[1] = RTCTimer.mapperLMinutes;
                    Cart->LatchedTimerData[2] = RTCTimer.mapperLHours;
                    Cart->LatchedTimerData[3] = RTCTimer.mapperLDays;
                    Cart->LatchedTimerData[4] = RTCTimer.mapperLControl;
                    Cart->timerLastUpdate = RTCTimer.mapperLastTime;
                    UpdateRTC(Cart);
                }
                else
                {
                    ReadTDF(Cart);  // Try to open TDF format, clear/initialize Cart->TimerData if that fails
                }
            }

            CloseHandle(hTemp);
        }
        else
        {
            // No battery; just allocate some RAM
            Cart->RamData = (LPBYTE)P_malloc(Cart->iNumRamBanks * 0x2000);
        }
    }

    Cart->TimerDataLatched = false;

    return true;
}

// Done
bool ReadCartNorm(LPGBCART Cart, WORD dwAddress, BYTE *Data) // For all non-MBC carts; fixed 0x8000 ROM; fixed, optional 0x2000 RAM
{
    switch (dwAddress >> 13) // Hack: examine highest 3 bits
    {
    case 0:
    case 1:
    case 2:
    case 3: //  if ((dwAddress >= 0) && (dwAddress <= 0x7FFF))
        CopyMemory(Data, &Cart->RomData[dwAddress], 32);
        DebugWriteA("Non-banked ROM read - raw\n");
        break;
    case 5:
        if (Cart->bHasRam)  // No MBC, so no enable state to check
        {
            if (Cart->RomData[0x149] == 1 && (dwAddress - 0xA000) / 0x0800 ) // Only 1/4 of the RAM space is used, and we're out of bounds
            {
                DebugWriteA("Failed RAM read: Unbanked (out of bounds)");
                ZeroMemory(Data, 32);
            }
            else
            {
                CopyMemory(Data, &Cart->RamData[dwAddress - 0xA000], 32);
                DebugWriteA("RAM read: Unbanked\n");
            }
        }
        else
        {
            ZeroMemory(Data, 32);
            DebugWriteA("Failed RAM read: Unbanked (RAM not present)\n");
        }
        break;
    default:
        DebugWriteA("Bad read from raw cart, address %04X\n", dwAddress);
    }

    return true;
}

// Done
bool WriteCartNorm(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    if (!Cart->bHasRam)
    {
        DebugWriteA("RAM write: no RAM\n");
        return true;
    }

    if (Cart->RomData[0x149] == 1)
    { // Whoops...Only 1/4 of the RAM space is used.
        if ((dwAddress >= 0xA000) && (dwAddress <= 0xA7FF))
        { // Write to RAM
            DebugWriteA("RAM write: Unbanked\n");
            CopyMemory(&Cart->RamData[dwAddress - 0xA000], Data, 32);
        }
        else
        {
            DebugWriteA("RAM write: Unbanked (out of range!)\n");
        }
    }
    else
    {
        if ((dwAddress >= 0xA000) && (dwAddress <= 0xBFFF))
        { // Write to RAM
            DebugWriteA("RAM write: Unbanked\n");
            CopyMemory(&Cart->RamData[dwAddress - 0xA000], Data, 32);
        }
    }
    return true;
}

// Done
bool ReadCartMBC1(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    if ((dwAddress >= 0) && (dwAddress <= 0x3FFF))
    {
        CopyMemory(Data, &Cart->RomData[dwAddress], 32);
        DebugWriteA("Non-banked ROM read - MBC1\n");
    }
    else if ((dwAddress >= 0x4000) && (dwAddress <= 0x7FFF))
    {
        if (Cart->iCurrentRomBankNo >= Cart->iNumRomBanks)
        {
            ZeroMemory(Data, 32);
            DebugWriteA("Banked ROM read: (Banking Error) Bank %02X\n", Cart->iCurrentRomBankNo);
        }
        else
        {
            // for (i=0; i<32; i++) Data[i] = Cart->RomData[(dwAddress - 0x4000) + i + (Cart->iCurrentRomBankNo * 0x4000)];
            CopyMemory(Data, &Cart->RomData[dwAddress - 0x4000 + (Cart->iCurrentRomBankNo << 14)], 32);
            DebugWriteA("Banked ROM read: Bank %02X\n", Cart->iCurrentRomBankNo);
        }
    }
    else if ((dwAddress >= 0xA000) && (dwAddress <= 0xBFFF))
    {
        if (Cart->bHasRam/* && Cart->bRamEnableState)*/)
        {
            if (Cart->iCurrentRamBankNo >= Cart->iNumRamBanks)
            {
                ZeroMemory(Data, 32);
                DebugWriteA("Failed RAM read: (Banking Error) %02X\n", Cart->iCurrentRamBankNo);
            }
            else
            {
                CopyMemory(Data, &Cart->RamData[dwAddress - 0xA000 + (Cart->iCurrentRamBankNo << 13)], 32);
                DebugWriteA("RAM read: Bank %02X\n", Cart->iCurrentRamBankNo);
            }
        }
        else
        {
            ZeroMemory(Data, 32);
            DebugWriteA("Failed RAM read: (RAM not present)\n");
        }
    }
    else
    {
        DebugWriteA("Bad read from MBC1 cart, address %04X\n", dwAddress);
    }

    return true;
}

// Done
bool WriteCartMBC1(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    if ((dwAddress >= 0x0000) && (dwAddress <= 0x1FFF)) // RAM enable
    {
        Cart->bRamEnableState = (Data[0] == 0x0A);
        DebugWriteA("Set RAM enable: %d\n", Cart->bRamEnableState);
    }
    else if ((dwAddress >= 0x2000) && (dwAddress <= 0x3FFF)) // ROM bank select
    {
        Cart->iCurrentRomBankNo &= 0x60;    // Keep MSB
        Cart->iCurrentRomBankNo |= Data[0] & 0x1F;

        // Emulate quirk: 0x00 -> 0x01, 0x20 -> 0x21, 0x40->0x41, 0x60 -> 0x61
        if ((Cart->iCurrentRomBankNo & 0x1F) == 0)
        {
            Cart->iCurrentRomBankNo |= 0x01;
        }
        DebugWriteA("Set ROM Bank: %02X\n", Cart->iCurrentRomBankNo);
    }
    else if ((dwAddress >= 0x4000) && (dwAddress <= 0x5FFF)) // RAM bank select
    {
        if (Cart->bMBC1RAMbanking)
        {
            Cart->iCurrentRamBankNo = Data[0] & 0x03;
            DebugWriteA("Set RAM Bank: %02X\n", Cart->iCurrentRamBankNo);
        }
        else
        {
            Cart->iCurrentRomBankNo &= 0x1F;
            Cart->iCurrentRomBankNo |= ((Data[0] & 0x03) << 5); // Set bits 5 and 6 of ROM bank
            DebugWriteA("Set ROM Bank MSB, ROM bank now: %02X\n", Cart->iCurrentRomBankNo);
        }
    }
    else if ((dwAddress >= 0x6000) && (dwAddress <= 0x7FFF)) // MBC1 mode select
    {
        // This is overly complicated, but it keeps us from having to do bitwise math later.
        // Basically we shuffle the 2 "magic bits" between iCurrentRomBankNo and iCurrentRamBankNo as necessary.
        if (Cart->bMBC1RAMbanking != (Data[0] & 0x01))
        {
            // We should only alter the ROM and RAM bank numbers if we have changed modes
            Cart->bMBC1RAMbanking = Data[0] & 0x01;
            if (Cart->bMBC1RAMbanking)
            {
                Cart->iCurrentRamBankNo = Cart->iCurrentRomBankNo >> 5; // Set the ram bank to the "magic bits"
                Cart->iCurrentRomBankNo &= 0x1F; // Zero out bits 5 and 6 to keep consistency
            }
            else
            {
                Cart->iCurrentRomBankNo &= 0x1F;
                Cart->iCurrentRomBankNo |= (Cart->iCurrentRamBankNo << 5);
                Cart->iCurrentRamBankNo = 0x00; // We can only reach RAM page 0
            }
            DebugWriteA("Set MBC1 mode: %s\n", Cart->bMBC1RAMbanking ? "ROMbanking" : "RAMbanking");
        }
        else
        {
            DebugWriteA("Already in MBC1 mode: %s\n", Cart->bMBC1RAMbanking ? "ROMbanking" : "RAMbanking");
        }
    }
    else if ((dwAddress >= 0xA000) && (dwAddress <= 0xBFFF)) // Write to RAM
    {
        if (Cart->bHasRam) // && Cart->bRamEnableState)
        {
            DebugWriteA("RAM write: Bank %02X\n", Cart->iCurrentRamBankNo);
            CopyMemory(&Cart->RamData[dwAddress - 0xA000 + (Cart->iCurrentRamBankNo << 13)], Data, 32);
        }
        else
        {
            DebugWriteA("Failed RAM write: (RAM not present)\n");
        }
    }
    else
    {
        DebugWriteA("Bad write to MBC1 cart, address %04X\n", dwAddress);
    }

    return true;
}

// Done
bool ReadCartMBC2(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    if ((dwAddress <= 0x3FFF))
    {
        CopyMemory(Data, &Cart->RomData[dwAddress], 32);
        DebugWriteA("Non-banked ROM read - MBC2\n");
    }
    else if ((dwAddress >= 0x4000) && (dwAddress <= 0x7FFF))
    {
        if (Cart->iCurrentRomBankNo >= Cart->iNumRomBanks)
        {
            ZeroMemory(Data, 32);
            DebugWriteA("Banked ROM read: (Banking Error) %02X\n", Cart->iCurrentRomBankNo);
        }
        else
        {
            CopyMemory(Data, &Cart->RomData[dwAddress - 0x4000 + (Cart->iCurrentRomBankNo << 14)], 32);
            DebugWriteA("Banked ROM read: Bank %02X\n", Cart->iCurrentRomBankNo);
        }
    }
    else if ((dwAddress >= 0xA000) && (dwAddress <= 0xBFFF))
    {
        if (Cart->bHasRam && Cart->bRamEnableState)
        {
            CopyMemory(Data, &Cart->RamData[dwAddress - 0xA000], 32);
            DebugWriteA("RAM read: Unbanked\n");
        }
        else
        {
            ZeroMemory(Data, 32);
            DebugWriteA("Failed RAM read: (RAM not present or not active)\n");
        }
    }
    else
    {
        DebugWriteA("Bad read from MBC2 cart, address %04X\n", dwAddress);
    }

    return true;
}

// Done
bool WriteCartMBC2(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    if ((dwAddress >= 0x0000) && (dwAddress <= 0x1FFF)) // We shouldn't be able to read/write to RAM unless this is toggled on
    {
        Cart->bRamEnableState = (Data[0] == 0x0A);
        DebugWriteA("Set RAM enable: %d\n", Cart->bRamEnableState);
    }
    else if ((dwAddress >= 0x2000) && (dwAddress <= 0x3FFF)) // ROM bank select
    {
        Cart->iCurrentRomBankNo = Data[0] & 0x0F;
        if (Cart->iCurrentRomBankNo == 0)
        {
            Cart->iCurrentRomBankNo = 1;
        }
        DebugWriteA("Set ROM Bank: %02X\n", Cart->iCurrentRomBankNo);
    }
    else if ((dwAddress >= 0x4000) && (dwAddress <= 0x5FFF)) // RAM bank select
    {
        if (Cart->bHasRam)
        {
            Cart->iCurrentRamBankNo = Data[0] & 0x07;
            DebugWriteA("Set RAM Bank: %02X\n", Cart->iCurrentRamBankNo);
        }
    }
    else if ((dwAddress >= 0xA000) && (dwAddress <= 0xBFFF) && Cart->bRamEnableState) // Write to RAM
    {
        if (Cart->bHasRam)
        {
            DebugWriteA("RAM write: Bank %02X\n", Cart->iCurrentRamBankNo);
            CopyMemory(&Cart->RamData[dwAddress - 0xA000 + (Cart->iCurrentRamBankNo << 13)], Data, 32);
        }
    }
    else
    {
        DebugWriteA("Bad write to MBC2 cart, address %04X\n", dwAddress);
    }

    return true;
}

// Done
bool ReadCartMBC3(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    if ((dwAddress < 0x4000)) //Rom Bank 0
    {
        CopyMemory(Data, &Cart->RomData[dwAddress], 32);
        DebugWriteA("Non-banked ROM read - MBC3\n");
    }
    else if ((dwAddress >= 0x4000) && (dwAddress < 0x8000)) // Switchable ROM bank
    {
        if (Cart->iCurrentRomBankNo >= Cart->iNumRomBanks)
        {
            ZeroMemory(Data, 32);
            DebugWriteA("Banked ROM read: (Banking Error) %02X\n", Cart->iCurrentRomBankNo);
        }
        else
        {
            CopyMemory(Data, &Cart->RomData[dwAddress - 0x4000 + (Cart->iCurrentRomBankNo * 0x4000)], 32);
            DebugWriteA("Banked ROM read: Bank %02X\n", Cart->iCurrentRomBankNo);
        }
    }
    else if ((dwAddress >= 0xA000) && (dwAddress <= 0xC000)) // Upper Bounds of memory map
    {
        if (Cart->bHasTimer && (Cart->iCurrentRamBankNo >= 0x08 && Cart->iCurrentRamBankNo <= 0x0c))
        {
            // The timer was just read!
            if (Cart->TimerDataLatched)
            {
                for (int i = 0; i < 32; i++)
                    Data[i] = Cart->LatchedTimerData[Cart->iCurrentRamBankNo - 0x08];
            }
            else
            {
                UpdateRTC(Cart);
                for (int i = 0; i < 32; i++)
                    Data[i] = Cart->TimerData[Cart->iCurrentRamBankNo - 0x08];
            }
        }
        else if (Cart->bHasRam)
        {
            if (Cart->iCurrentRamBankNo >= Cart->iNumRamBanks)
            {
                ZeroMemory(Data, 32);
                DebugWriteA("Failed RAM read: (Banking Error) %02X\n", Cart->iCurrentRamBankNo);
            }
            else
            {
                CopyMemory(Data, &Cart->RamData[dwAddress - 0xA000 + (Cart->iCurrentRamBankNo * 0x2000)], 32);
                DebugWriteA("RAM read: Bank %02X\n", Cart->iCurrentRamBankNo);
            }/*
            else
            {
                ZeroMemory(Data, 32);
                //for (i=0; i<32; i++) Data[i] = 0;
                DebugWriteA("Failed RAM read: (RAM not active)\n");
            }*/
        }
        else
        {
            ZeroMemory(Data, 32);
            DebugWriteA("Failed RAM read: (RAM not present)\n");
        }
    }
    else
    {
        DebugWriteA("Bad read from MBC3 cart, address %04X\n", dwAddress);
    }

    return true;
}

// Done
bool WriteCartMBC3(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    int i;

    if ((dwAddress >= 0x0000) && (dwAddress <= 0x1FFF)) // We shouldn't be able to read/write to RAM unless this is toggled on
    {
        Cart->bRamEnableState = (Data[0] == 0x0A);
        DebugWriteA("Set RAM enable: %d\n", Cart->bRamEnableState);
    }
    else if ((dwAddress >= 0x2000) && (dwAddress <= 0x3FFF)) // ROM bank select
    {
        Cart->iCurrentRomBankNo = Data[0] & 0x7F;
        if (Cart->iCurrentRomBankNo == 0) {
            Cart->iCurrentRomBankNo = 1;
        }
        DebugWriteA("Set ROM Bank: %02X\n", Cart->iCurrentRomBankNo);
    }
    else if ((dwAddress >= 0x4000) && (dwAddress <= 0x5FFF)) // RAM/clock bank select
    {
        if (Cart->bHasRam)
        {
            Cart->iCurrentRamBankNo = Data[0] & 0x07;
            DebugWriteA("Set RAM Bank: %02X\n", Cart->iCurrentRamBankNo);
            if (Cart->bHasTimer && (Data[0] >= 0x08 && Data[0] <= 0x0c))
            {
                // Set the bank for the timer
                Cart->iCurrentRamBankNo = Data[0];
            }
        }
    }
    else if ((dwAddress >= 0x6000) && (dwAddress <= 0x7FFF)) // Latch timer data
    {
        CopyMemory(Cart->LatchedTimerData, Cart->TimerData, 5 * sizeof(Cart->TimerData[0]));
        if (Data[0] & 1)
        {
            // Update timer, save latch values, and set latch state
            UpdateRTC(Cart);
            for (i = 0; i < 4; i++)
                Cart->LatchedTimerData[i] = Cart->TimerData[i];
            Cart->TimerDataLatched = true;
            DebugWriteA("Timer data latch: Enable\n");
        }
        else
        {
            Cart->TimerDataLatched = false;
            DebugWriteA("Timer data latch: Disable\n");
        }
    }
    else if ((dwAddress >= 0xA000) && (dwAddress <= 0xBFFF)) // Write to RAM
    {
        if (Cart->bHasRam)
        {
            if (Cart->iCurrentRamBankNo >= 0x08 && Cart->iCurrentRamBankNo <= 0x0c)
            {
                // Write to the timer
                DebugWriteA("Timer write: Bank %02X\n", Cart->iCurrentRamBankNo);
                Cart->TimerData[Cart->iCurrentRamBankNo - 0x08] = Data[0];
            }
            else
            {
                DebugWriteA("RAM write: Bank %02X%s\n", Cart->iCurrentRamBankNo, Cart->bRamEnableState ? "" : " -- NOT ENABLED (but wrote anyway)");
                CopyMemory(&Cart->RamData[dwAddress - 0xA000 + (Cart->iCurrentRamBankNo * 0x2000)], Data, 32);
            }
        }
    }
    else
    {
        DebugWriteA("Bad write to MBC3 cart, address %04X\n", dwAddress);
    }

    return true;
}

// Done
bool ReadCartMBC5(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    if ((dwAddress < 0x4000)) //Rom Bank 0
    {
        CopyMemory(Data, &Cart->RomData[dwAddress], 32);
        DebugWriteA("Non-banked ROM read - MBC5\n");
    }
    else if ((dwAddress >= 0x4000) && (dwAddress < 0x8000)) // Switchable ROM bank
    {
        if (Cart->iCurrentRomBankNo >= Cart->iNumRomBanks)
        {
            ZeroMemory(Data, 32);
            DebugWriteA("Banked ROM read: (Banking Error)");
            DebugWriteByteA(Cart->iCurrentRomBankNo);
            DebugWriteA("\n");
        }
        else {
            CopyMemory(Data, &Cart->RomData[dwAddress - 0x4000 + (Cart->iCurrentRomBankNo << 14)], 32);
            DebugWriteA("Banked ROM read: Bank=");
            DebugWriteByteA(Cart->iCurrentRomBankNo);
            DebugWriteA("\n");
        }
    }
    else if ((dwAddress >= 0xA000) && (dwAddress <= 0xC000)) // Upper bounds of memory map
    {
        if (Cart->bHasRam)
        {
            if (Cart->iCurrentRamBankNo >= Cart->iNumRamBanks)
            {
                ZeroMemory(Data, 32);
                DebugWriteA("Failed RAM read: (Banking Error) %02X\n", Cart->iCurrentRamBankNo);
            }
            else
            {
                CopyMemory(Data, &Cart->RamData[dwAddress - 0xA000 + (Cart->iCurrentRamBankNo << 13)], 32);
                DebugWriteA("RAM read: Bank %02X\n", Cart->iCurrentRamBankNo);
            }
        }
        else
        {
            ZeroMemory(Data, 32);
            DebugWriteA("Failed RAM read: (RAM Not Present)\n");
        }
    }
    else
    {
        DebugWriteA("Bad read from MBC5 cart, address %04X\n", dwAddress);
    }

    return true;
}

// Done
bool WriteCartMBC5(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    if ((dwAddress >= 0x0000) && (dwAddress <= 0x1FFF)) // We shouldn't be able to read/write to RAM unless this is toggled on
    {
        Cart->bRamEnableState = (Data[0] == 0x0A);
        DebugWriteA("Set RAM enable: %d\n", Cart->bRamEnableState);
    }
    else if ((dwAddress >= 0x2000) && (dwAddress <= 0x2FFF)) // ROM bank select, low bits
    {
        Cart->iCurrentRomBankNo &= 0xFF00;
        Cart->iCurrentRomBankNo |= Data[0];
        // Cart->iCurrentRomBankNo = ((int) Data[0]) | (Cart->iCurrentRomBankNo & 0x100);
        DebugWriteA("Set ROM Bank: %02X\n", Cart->iCurrentRomBankNo);
    }
    else if ((dwAddress >= 0x3000) && (dwAddress <= 0x3FFF)) // ROM bank select, high bit
    {
        Cart->iCurrentRomBankNo &= 0x00FF;
        Cart->iCurrentRomBankNo |= (Data[0] & 0x01) << 8;
        // Cart->iCurrentRomBankNo = (Cart->iCurrentRomBankNo & 0xFF) | ((((int) Data[0]) & 1) * 0x100);
        DebugWriteA("Set ROM Bank: %02X\n", Cart->iCurrentRomBankNo);
    }
    else if ((dwAddress >= 0x4000) && (dwAddress <= 0x5FFF)) // RAM bank select
    {
        if (Cart->bHasRam)
        {
            Cart->iCurrentRamBankNo = Data[0] & 0x0F;
            DebugWriteA("Set RAM Bank: %02X\n", Cart->iCurrentRamBankNo);
        }
    }
    else if ((dwAddress >= 0xA000) && (dwAddress <= 0xBFFF)) // Write to RAM
    {
        if (Cart->bHasRam)
        {
            if (Cart->iCurrentRamBankNo >= Cart->iNumRamBanks)
            {
                DebugWriteA("RAM write: Buffer error on ");
                DebugWriteByteA(Cart->iCurrentRamBankNo);
                DebugWriteA("\n");
            }
            else
            {
                DebugWriteA("RAM write: Bank %02X\n", Cart->iCurrentRamBankNo);
                CopyMemory(&Cart->RamData[dwAddress - 0xA000 + (Cart->iCurrentRamBankNo << 13)], Data, 32);
            }
        }
    }
    else
    {
        DebugWriteA("Bad write to MBC5 cart, address %04X\n", dwAddress);
    }

    return true;
}

// Done
bool ReadCartCamera(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    if ((dwAddress < 0x4000)) // ROM bank 0
    {
        CopyMemory(Data, &Cart->RomData[dwAddress], 32);
        DebugWriteA("Non-banked ROM read - CAMERA\n");
    }
    else if ((dwAddress >= 0x4000) && (dwAddress < 0x8000)) // Switchable ROM bank
    {
        if (Cart->iCurrentRomBankNo >= Cart->iNumRomBanks)
        {
            ZeroMemory(Data, 32);
            DebugWriteA("Banked ROM read: (Banking Error)");
            DebugWriteByteA(Cart->iCurrentRomBankNo);
            DebugWriteA("\n");
        }
        else {
            CopyMemory(Data, &Cart->RomData[dwAddress - 0x4000 + (Cart->iCurrentRomBankNo << 14)], 32);
            DebugWriteA("Banked ROM read: Bank=");
            DebugWriteByteA(Cart->iCurrentRomBankNo);
            DebugWriteA("\n");
        }
    }
    else if ((dwAddress >= 0xA000) && (dwAddress <= 0xC000)) // Upper bounds of memory map
    {
        if (Cart->iCurrentRamBankNo & 0x10)
        {
            // Register mode
            ZeroMemory(Data, 32);
            DebugWriteA("REGISTER read (Camera): All Zero\n", Cart->iCurrentRamBankNo);
        }
        else
        {
            // RAM mode
            if (Cart->bHasRam)
            {
                if (Cart->iCurrentRamBankNo >= Cart->iNumRamBanks)
                {
                    ZeroMemory(Data, 32);
                    DebugWriteA("Failed RAM read: (Banking Error) %02X\n", Cart->iCurrentRamBankNo);
                }
                else
                {
                    CopyMemory(Data, &Cart->RamData[dwAddress - 0xA000 + (Cart->iCurrentRamBankNo << 13)], 32);
                    DebugWriteA("RAM read: Bank %02X\n", Cart->iCurrentRamBankNo);
                }
            }
            else
            {
                ZeroMemory(Data, 32);
                DebugWriteA("Failed RAM read: (RAM Not Present)\n");
            }
        }
    }
    else
    {
        DebugWriteA("Bad read from Game Boy Camera cart, address %04X\n", dwAddress);
    }

    return true;
}

// Done
bool WriteCartCamera(LPGBCART Cart, WORD dwAddress, BYTE *Data)
{
    if ((dwAddress >= 0x0000) && (dwAddress <= 0x1FFF)) // We shouldn't be able to read/write to RAM unless this is toggled on
    {
        Cart->bRamEnableState = (Data[0] == 0x0A);
        DebugWriteA("Set RAM enable: %d\n", Cart->bRamEnableState);
    }
    else if ((dwAddress >= 0x2000) && (dwAddress <= 0x2FFF)) // ROM bank select, low bits
    {
        Cart->iCurrentRomBankNo &= 0xFF00;
        Cart->iCurrentRomBankNo |= Data[0];
        // Cart->iCurrentRomBankNo = ((int) Data[0]) | (Cart->iCurrentRomBankNo & 0x100);
        DebugWriteA("Set ROM Bank: %02X\n", Cart->iCurrentRomBankNo);
    }
    else if ((dwAddress >= 0x4000) && (dwAddress <= 0x4FFF)) // Camera register and RAM bank select
    {
        if (Data[0] & 0x10)
        {
            // Register mode
            Cart->iCurrentRamBankNo = Data[0];
            DebugWriteA("Set Register Bank (Camera): %02X\n", Cart->iCurrentRamBankNo);
        }
        else
        {
            // RAM mode
            if (Cart->bHasRam)
            {
                Cart->iCurrentRamBankNo = Data[0] & 0x0F;
                DebugWriteA("Set RAM Bank: %02X\n", Cart->iCurrentRamBankNo);
            }
        }
    }
    else if ((dwAddress >= 0xA000) && (dwAddress <= 0xBFFF)) // Write to RAM
    {
        if (Cart->iCurrentRamBankNo & 0x10)
        {
            // Register mode (do nothing)
            DebugWriteA("REGISTER write (Camera): Do nothing\n");
        }
        else
        {
            // RAM mode
            if (Cart->bHasRam)
            {
                if (Cart->iCurrentRamBankNo >= Cart->iNumRamBanks)
                {
                    DebugWriteA("RAM write: Buffer error on ");
                    DebugWriteByteA(Cart->iCurrentRamBankNo);
                    DebugWriteA("\n");
                }
                else
                {
                    DebugWriteA("RAM write: Bank %02X\n", Cart->iCurrentRamBankNo);
                    CopyMemory(&Cart->RamData[dwAddress - 0xA000 + (Cart->iCurrentRamBankNo << 13)], Data, 32);
                }
            }
        }
    }
    else
    {
        DebugWriteA("Bad write to Game Boy Camera cart, address %04X\n", dwAddress);
    }

    return true;
}

bool SaveCart(LPGBCART Cart, LPTSTR SaveFile, LPTSTR TimeFile)
{
    DWORD NumQuarterBlocks = 0;
    gbCartRTC RTCTimer;

    if (Cart->bHasRam && Cart->bHasBattery)
    { // Write only the bytes that NEED writing!
        switch (Cart->RomData[0x149])
        {
        case 1:
            NumQuarterBlocks = 1;
            break;
        case 2:
            NumQuarterBlocks = 4;
            break;
        case 3:
            NumQuarterBlocks = 16;
            break;
        case 4:
            NumQuarterBlocks = 64;
            break;
        }
        FlushViewOfFile( Cart->RamData, NumQuarterBlocks * 0x0800 );
        if (Cart->bHasTimer)
        {
            // Save RTC in Visual Boy Advance format
            // TODO: Check if VBA saves are compatible with other emus
            // TODO: Only write RTC data if VBA RTC data was originally present
            RTCTimer.mapperSeconds = Cart->TimerData[0];
            RTCTimer.mapperMinutes = Cart->TimerData[1];
            RTCTimer.mapperHours = Cart->TimerData[2];
            RTCTimer.mapperDays = Cart->TimerData[3];
            RTCTimer.mapperControl = Cart->TimerData[4];
            RTCTimer.mapperLSeconds = Cart->LatchedTimerData[0];
            RTCTimer.mapperLMinutes = Cart->LatchedTimerData[1];
            RTCTimer.mapperLHours = Cart->LatchedTimerData[2];
            RTCTimer.mapperLDays = Cart->LatchedTimerData[3];
            RTCTimer.mapperLControl = Cart->LatchedTimerData[4];
            RTCTimer.mapperLastTime = Cart->timerLastUpdate;

            CopyMemory(Cart->RamData + NumQuarterBlocks * 0x0800, &RTCTimer, sizeof(RTCTimer));
            FlushViewOfFile(Cart->RamData + NumQuarterBlocks * 0x0800, sizeof(gbCartRTC));
        }
    }
    return true;
}

bool UnloadCart(LPGBCART Cart)
{
    if (Cart->hRomFile != NULL)
    {
        UnmapViewOfFile(Cart->RomData);
        CloseHandle(Cart->hRomFile);
        Cart->hRomFile = NULL;
    }
    else if (Cart->RomData != NULL)
    {
        P_free((LPVOID)(Cart->RomData));
        Cart->RomData = NULL;
    }

    if (Cart->hRamFile != NULL)
    {
        UnmapViewOfFile(Cart->RamData);
        CloseHandle(Cart->hRamFile);
        Cart->hRamFile = NULL;
    }
    else if (Cart->RamData != NULL)
    {
        P_free(Cart->RamData);
        Cart->RamData = NULL;
    }
    return true;
}

// This is used to clear the RAM data to look like it has just been turned on.
// When a RAM chip is first turned on, it is filled with alternating 128-byte
// blocks of 0x00 and 0xFF.
void ClearData(BYTE *Data, int Length)
{
    int i;

    for (i = 0; i < Length; i++)
    {
        if ((i & 0x80) != 0x80)
        {
            Data[i] = 0x00;
        }
        else
        {
            Data[i] = 0xFF;
        }
    }
}
