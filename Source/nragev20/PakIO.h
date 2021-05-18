/*
N-Rage`s Dinput8 Plugin
(C) 2002, 2006  Norbert Wladyka

Author`s Email: norbert.wladyka@chello.at
Website: http://go.to/nrage

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _PAKIO_H_
#define _PAKIO_H_

bool InitControllerPak( const int iControl );
void SaveControllerPak( const int iControl );
BYTE ReadControllerPak( const int iControl, LPBYTE Command );
BYTE WriteControllerPak( const int iControl, LPBYTE Command );
void CloseControllerPak( const int iControl );
WORD ShowMemPakContent( const unsigned char * bMemPakBinary, HWND hListWindow );
int TranslateNotesA( const unsigned char * bNote, LPSTR Text, const int iChars );
int TranslateNotesW( const unsigned char * bNote, LPWSTR Text, const int iChars );
void FormatMemPak( LPBYTE aMemPak );
bool SaveNoteFileA( const unsigned char * aMemPak, const int iNote, LPCTSTR pszFileName );
bool InsertNoteFile( LPBYTE aMemPak, LPCTSTR pszFileName );
bool RemoveNote( LPBYTE aMemPak, const int iNote );

void TexttoHexA( LPCSTR szText, LPBYTE Data, const int nBytes );
void HextoTextA( const unsigned char * Data, LPSTR szText, const int nBytes );

#ifdef _UNICODE
#define TranslateNotes(x,y,z) TranslateNotesW(x,y,z)
#define ReverseNotes(x,y,z) ReverseNotesW(x,y,z)
#else
#define TranslateNotes(x,y,z) TranslateNotesA(x,y,z)
#define ReverseNotes(x,y,z) ReverseNotesA(x,y,z)
#endif


// Raw data
// Byte 1 = Number of bytes to send
// Byte 2 = Number of bytes to receive
// Byte 3 = Command type

// Get status
#define RD_GETSTATUS        0x00
// Read button values
#define RD_READKEYS         0x01
// Read from controller pak
#define RD_READPAK          0x02
// Write to controller pak
#define RD_WRITEPAK         0x03
// Reset controller
#define RD_RESETCONTROLLER  0xff
// Read EEPROM
#define RD_READEEPROM       0x04
// Write EEPROM
#define RD_WRITEEPROM       0x05

// Codes for retrieving status
// 0x010300 - A1B2C3FF

// A1
// Default controller
#define RD_ABSOLUTE         0x01
#define RD_RELATIVE         0x02
// Default controller
#define RD_GAMEPAD          0x04

// B2
#define RD_EEPROM           0x80
#define RD_NOEEPROM         0x00

// C3
// No plugin in controller
#define RD_NOPLUGIN         0x00
// Plugin in controller (memory pak, rumble pak, etc.)
#define RD_PLUGIN           0x01
// Pak interface was uninitialized before the call
#define RD_NOTINITIALIZED   0x02
// Address of last pak I/O was invalid
#define RD_ADDRCRCERR       0x04
// EEPROM busy
#define RD_EEPROMBUSY       0x80

// The error values are as follows:
// 0x01ER00 - ........

// ER
// No error, operation successful
#define RD_OK               0x00
// Error, device not present for specified command
#define RD_ERROR            0x80
// Error, unable to send/receive the number of bytes for command type
#define RD_WRONGSIZE        0x40

// The address where rumble-commands are sent to
// This is really 0xC01B but our addressing code truncates the last several bits
#define PAK_IO_RUMBLE       0xC000

// 32KB memory pak
#define PAK_MEM_SIZE        32*1024
#define PAK_MEM_DEXOFFSET   0x1040

// Pak-specific data
// First byte always determines current pak type
// This can be different to the pak type in the controller structure
// That makes sure to correctly handle/close the pak

//PAK_NONE
//pPakData = NULL;

//PAK_MEM
typedef struct _MEMPAK
{
    BYTE bPakType;              // Set to PAK_MEM
    HANDLE hMemPakHandle;       // A file mapping handle
    bool fDexSave;              // True if .n64 file, false if .mpk file
    bool fReadonly;             // Set if we can't open memory pak file in "write" mode
    LPBYTE aMemPakData;         //[PAK_MEM_SIZE];
    BYTE aMemPakTemp[0x100];    // Some extra on the top for "test" (temporary) data
} MEMPAK, *LPMEMPAK;

//PAK_RUMBLE
typedef struct _RUMBLEPAK
{
    BYTE bPakType;
//  BYTE bRumbleTyp;            // Obsolete: use g_pcControllers[i].xyz instead (comment by rabid)
//  BYTE bRumbleStrength;
//  bool fVisualRumble;
    bool fLastData;             // True if the last data sent to block 0x8000 was non-zero
} RUMBLEPAK, *LPRUMBLEPAK;

#include "GBCart.h"
//PAK_TRANSFER
typedef struct _TRANSFERPAK
{
    BYTE bPakType;
    int iCurrentBankNo;
    int iCurrentAccessMode;
    int iAccessModeChanged;
    bool iEnableState;
    bool bPakInserted;
    GBCART gbCart;
} TRANSFERPAK, *LPTRANSFERPAK;

//PAK_VOICE
typedef struct _VOICEPAK // Not supported
{
    BYTE bPakType;
} VOICEPAK, *LPVOICEPAK;

//PAK_ADAPTOID
typedef struct _ADAPTOIDPAK
{
    BYTE bPakType;
    BYTE bIdentifier;
    bool fRumblePak;
} ADAPTOIDPAK, *LPADAPTOIDPAK;

#endif // #ifndef _PAKIO_H_
