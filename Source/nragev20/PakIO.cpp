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

#include <algorithm>
#include <stdio.h>

#include <windows.h>
#include <Commctrl.h>
#include <tchar.h>

#include "commonIncludes.h"
#include "DirectInput.h"
#include "FileAccess.h"
#include "GBCart.h"
#include "Interface.h"
#include "NRagePluginV2.h"
#include "PakIO.h"

using std::min;
using std::max;

// ProtoTypes
BYTE AddressCRC( const unsigned char * Address );
BYTE DataCRC( const unsigned char * Data, const int iLength );
VOID CALLBACK WritebackProc( HWND hWnd, UINT msg, UINT_PTR idEvent, DWORD dwTime );

bool InitControllerPak( const int iControl )
// Prepares the Pak
{
	if( !g_pcControllers[iControl].fPlugged )
		return false;
	bool bReturn = false;
	if( g_pcControllers[iControl].pPakData )
	{
		SaveControllerPak( iControl );
		CloseControllerPak( iControl );
	}

	switch( g_pcControllers[iControl].PakType )
	{
	case PAK_MEM:
		{
			g_pcControllers[iControl].pPakData = P_malloc( sizeof(MEMPAK));
			MEMPAK *mPak = (MEMPAK*)g_pcControllers[iControl].pPakData;
			mPak->bPakType = PAK_MEM;
			mPak->fReadonly = false;
			mPak->fDexSave = false;
			mPak->hMemPakHandle = NULL;

			DWORD dwFilesize = PAK_MEM_SIZE;	// expected file size
			TCHAR szBuffer[MAX_PATH+1],
				  szFullPath[MAX_PATH+1],
				  *pcFile;

			GetAbsoluteFileName( szBuffer, g_pcControllers[iControl].szMempakFile, DIRECTORY_MEMPAK );
			GetFullPathName( szBuffer, sizeof(szFullPath) / sizeof(TCHAR), szFullPath, &pcFile );

			bool isNewfile = !CheckFileExists( szBuffer );

			if( pcFile == NULL )
			{ // no Filename specified
				WarningMessage( IDS_ERR_MEM_NOSPEC, MB_OK | MB_ICONWARNING );
				g_pcControllers[iControl].PakType = PAK_NONE;
				break; // memory is freed at the end of this function
			}

			TCHAR *pcPoint = _tcsrchr( pcFile, '.' );
			if( !lstrcmpi( pcPoint, _T(".n64") ) )
			{
				mPak->fDexSave = true;
				dwFilesize += PAK_MEM_DEXOFFSET;
			}
			else
			{
				mPak->fDexSave = false;
			}

			HANDLE hFileHandle = CreateFile( szFullPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL );
			if( hFileHandle == INVALID_HANDLE_VALUE )
			{// test if Read-only access is possible
				hFileHandle = CreateFile( szFullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL );
				if( hFileHandle != INVALID_HANDLE_VALUE )
				{
					TCHAR tszTitle[DEFAULT_BUFFER], tszText[DEFAULT_BUFFER];

					LoadString( g_hResourceDLL, IDS_DLG_MEM_READONLY, tszText, DEFAULT_BUFFER );
					LoadString( g_hResourceDLL, IDS_DLG_WARN_TITLE, tszTitle, DEFAULT_BUFFER );
					wsprintf( szBuffer, tszText, pcFile );
					MessageBox( g_strEmuInfo.hMainWindow, szBuffer, tszTitle, MB_OK | MB_ICONWARNING );
					mPak->fReadonly = true;
					DebugWriteA("Ramfile opened in READ ONLY mode.\n");
				}
				else
				{
					TCHAR tszTitle[DEFAULT_BUFFER], tszText[DEFAULT_BUFFER];

					LoadString( g_hResourceDLL, IDS_ERR_MEMOPEN, tszText, DEFAULT_BUFFER );
					LoadString( g_hResourceDLL, IDS_DLG_WARN_TITLE, tszTitle, DEFAULT_BUFFER );
					wsprintf( szBuffer, tszText, pcFile );
					MessageBox( g_strEmuInfo.hMainWindow, szBuffer, tszTitle, MB_OK | MB_ICONWARNING );
					g_pcControllers[iControl].PakType = PAK_NONE;	// set so that CloseControllerPak doesn't try to close a file that isn't open
					DebugWrite(_T("Unable to read or create MemPak file %s.\n"), pcFile);
					break; // memory is freed at the end of this function
				}
			}

			DWORD dwCurrentSize = GetFileSize( hFileHandle, NULL );
			if ( mPak->fReadonly )
			{
				DWORD dwBytesRead = 0;

				if( mPak->fDexSave )
				{
					SetFilePointer( hFileHandle, PAK_MEM_DEXOFFSET, NULL, FILE_BEGIN );
				}
				else
				{
					SetFilePointer( hFileHandle, 0L, NULL, FILE_BEGIN );
				}

				dwFilesize = min( dwFilesize, GetFileSize( hFileHandle, NULL ));
				if( !isNewfile )
				{
					mPak->aMemPakData = (LPBYTE)P_malloc( sizeof(BYTE) * PAK_MEM_SIZE );
					if( ReadFile( hFileHandle, mPak->aMemPakData, PAK_MEM_SIZE, &dwBytesRead, NULL ))
					{
						if( dwBytesRead < dwFilesize )
							FillMemory( (LPBYTE)mPak->aMemPakData + dwBytesRead, PAK_MEM_SIZE - dwBytesRead, 0xFF );

						bReturn = true;
					}
					else
						P_free( mPak->aMemPakData );
				}
				else
				{
					FormatMemPak( mPak->aMemPakData );
					bReturn = true;
				}
				CloseHandle( hFileHandle );
			}
			else
			{
				// use mapped file
				mPak->hMemPakHandle = CreateFileMapping( hFileHandle, NULL, mPak->fReadonly ? PAGE_READONLY : PAGE_READWRITE, 0, dwFilesize, NULL);
				// test for invalid handle
				if (mPak->hMemPakHandle == NULL)
				{
					// note: please don't move the CloseHandle call before GetLastError
					DebugWriteA("Couldn't CreateFileMapping for MemPak file : %08x\n", GetLastError());
					CloseHandle(hFileHandle);
					break; // memory is freed at the end of the function
				}
				CloseHandle(hFileHandle); // we can close the file handle now with no problems

				mPak->aMemPakData = (LPBYTE)MapViewOfFile( mPak->hMemPakHandle, FILE_MAP_ALL_ACCESS, 0, 0,  mPak->fDexSave ? PAK_MEM_SIZE + PAK_MEM_DEXOFFSET : PAK_MEM_SIZE );

				// this is a bit tricky:
				// if it's a dexsave, move the pakdata pointer forward so it points to where the actual mempak data starts
				// we need to make sure to move it back when we UnmapViewOfFile
				if (mPak->aMemPakData == NULL)
					ErrorMessage(IDS_ERR_MAPVIEW, GetLastError(), false);

				if ( mPak->fDexSave )
					mPak->aMemPakData += PAK_MEM_DEXOFFSET;

                if( dwCurrentSize < dwFilesize )
					FillMemory( (LPBYTE)mPak->aMemPakData + (mPak->fDexSave ? dwCurrentSize - PAK_MEM_DEXOFFSET : dwCurrentSize), dwFilesize - dwCurrentSize, 0xFF );

				if( isNewfile )
				{
					if (mPak->fDexSave )
					{
						PVOID pHeader = MapViewOfFile( mPak->hMemPakHandle, FILE_MAP_ALL_ACCESS, 0, 0, PAK_MEM_DEXOFFSET );
						const char szHeader[] = "123-456-STD";	// "OMG-WTF-BBQ"? --rabid
						ZeroMemory( pHeader, PAK_MEM_DEXOFFSET );
						CopyMemory( pHeader, szHeader, sizeof(szHeader) );
						FlushViewOfFile( pHeader, PAK_MEM_DEXOFFSET );
						UnmapViewOfFile( pHeader );
					}
					FormatMemPak( mPak->aMemPakData );
				}

				bReturn = true;
			}			
		}
		break;
	case PAK_RUMBLE:
		{
			g_pcControllers[iControl].pPakData = P_malloc( sizeof(RUMBLEPAK));
			RUMBLEPAK *rPak = (RUMBLEPAK*)g_pcControllers[iControl].pPakData;
			rPak->bPakType = PAK_RUMBLE;

			rPak->fLastData = true;		// statistically, if uninitted it would return true --rabid
//			rPak->bRumbleTyp = g_pcControllers[iControl].bRumbleTyp;
//			rPak->bRumbleStrength = g_pcControllers[iControl].bRumbleStrength;
//			rPak->fVisualRumble = g_pcControllers[iControl].fVisualRumble;
			if( !g_pcControllers[iControl].fXInput )	//used to make sure only xinput cotroller rumbles --tecnicors
				CreateEffectHandle( iControl, g_pcControllers[iControl].bRumbleTyp, g_pcControllers[iControl].bRumbleStrength );
			bReturn = true;
		}
		break;
	case PAK_TRANSFER:
		{
			g_pcControllers[iControl].pPakData = P_malloc( sizeof(TRANSFERPAK));
			LPTRANSFERPAK tPak = (LPTRANSFERPAK)g_pcControllers[iControl].pPakData;
			tPak->bPakType = PAK_TRANSFER;

			tPak->gbCart.hRomFile = NULL;
			tPak->gbCart.hRamFile = NULL;
			tPak->gbCart.RomData = NULL;
			tPak->gbCart.RamData = NULL;

			/*
			 * Once the Interface is implemented g_pcControllers[iControl].szTransferRom will hold filename of the GB-Rom
			 * and g_pcControllers[iControl].szTransferSave holds Filename of the SRAM Save
			 * 
			 * Here, both files should be opened and the handles stored in tPak ( modify the struct for Your own purposes, only bPakType must stay at first )
			 */

			//CreateFile( g_pcControllers[iControl].szTransferSave, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL );
			tPak->iCurrentAccessMode = 0;
			tPak->iCurrentBankNo = 0;
			tPak->iEnableState = false;
			tPak->iAccessModeChanged = 0x44;

			tPak->bPakInserted = LoadCart( &tPak->gbCart, g_pcControllers[iControl].szTransferRom, g_pcControllers[iControl].szTransferSave, _T("") );

			if (tPak->bPakInserted)
			{
				DebugWriteA( "*** Init Transfer Pak - Success***\n" );
			}
			else
			{
				DebugWriteA( "*** Init Transfer Pak - FAILURE***\n" );
			}

			bReturn = true;
		}
		break;
	/*case PAK_VOICE:
		{
			g_pcControllers[iControl].pPakData = P_malloc( sizeof(VOICEPAK));
			VOICEPAK *vPak = (VOICEPAK*)g_pcControllers[iControl].pPakData;
			vPak->bPakType = PAK_VOICE;

			bReturn = true;
		}
		break;*/
	case PAK_ADAPTOID:
		if( !g_pcControllers[iControl].fIsAdaptoid )
			g_pcControllers[iControl].PakType = PAK_NONE;
		else
		{
			g_pcControllers[iControl].pPakData = P_malloc( sizeof(ADAPTOIDPAK));
			ADAPTOIDPAK *aPak = (ADAPTOIDPAK*)g_pcControllers[iControl].pPakData;
			aPak->bPakType = PAK_ADAPTOID;

			aPak->bIdentifier = 0x80;
#ifdef ADAPTOIDPAK_RUMBLEFIX
			aPak->fRumblePak = true;
#pragma message( "Driver-fix for Rumble with Adaptoid enabled" )
#else
			aPak->fRumblePak = false;
#endif
			bReturn = true;
		}
		break;
	/*case PAK_NONE:
		break;*/
	}

	// if there were any unrecoverable errors and we have allocated pPakData, free it and set paktype to NONE
	if( !bReturn && g_pcControllers[iControl].pPakData )
		CloseControllerPak( iControl );

	return bReturn;
}


BYTE ReadControllerPak( const int iControl, LPBYTE Command )
{
	BYTE bReturn = RD_ERROR;
	LPBYTE Data = &Command[2];

#ifdef MAKEADRESSCRCCHECK
#pragma message( "Addresscheck for Pak-Reads active" )
	if( AddressCRC( Command ) != (Command[1] & 0x1F) )
	{
		g_pcControllers[iControl].fPakCRCError = true;
		if( WarningMessage( IDS_DLG_MEM_BADADDRESSCRC, MB_OKCANCEL | MB_ICONQUESTION ) == IDCANCEL )
			return RD_ERROR;
	}
#endif

	if( !g_pcControllers[iControl].pPakData )
		return RD_ERROR;

	WORD dwAddress = (Command[0] << 8) + (Command[1] & 0xE0);

	switch( *(BYTE*)g_pcControllers[iControl].pPakData )
	{
	case PAK_MEM:
		{
			MEMPAK *mPak = (MEMPAK*)g_pcControllers[iControl].pPakData;
			
			if( dwAddress < 0x8000 )
				CopyMemory( Data, &mPak->aMemPakData[dwAddress], 32 );
			else
				CopyMemory( Data, &mPak->aMemPakTemp[(dwAddress%0x100)], 32 );
			Data[32] = DataCRC( Data, 32 );
			bReturn = RD_OK;
		}
		break;
	case PAK_RUMBLE:
		if(( dwAddress >= 0x8000 ) && ( dwAddress < 0x9000 ) )
		{
			RUMBLEPAK *rPak = (RUMBLEPAK*)g_pcControllers[iControl].pPakData;

			if (rPak->fLastData)
				FillMemory( Data, 32, 0x80 );
			else
				ZeroMemory( Data, 32 );

			if( g_pcControllers[iControl].fXInput )	// xinput controller rumble --tecnicors
				VibrateXInputController( g_pcControllers[iControl].xiController.nControl, 0, 0);
			else if (g_apFFDevice[iControl])
				g_apFFDevice[iControl]->Acquire();
		}
		else
			ZeroMemory( Data, 32 );

		Data[32] = DataCRC( Data, 32 );
		bReturn = RD_OK;
		break;

	case PAK_TRANSFER:
		{
			LPTRANSFERPAK tPak = (LPTRANSFERPAK)g_pcControllers[iControl].pPakData;	// TODO: null pointer check on tPak
			// set bReturn = RD_OK when implementing Transferpak
			bReturn = RD_OK;
			DebugWriteA( "TPak Read:\n" );
			DebugWriteA( "  Address: %04X\n", dwAddress );

			switch (dwAddress >> 12)
			{
			case 0x8: //	if ((dwAddress >= 0x8000) && (dwAddress <= 0x8FFF))
				DebugWriteA( "Query Enable State: %u\n", tPak->iEnableState );
				if (tPak->iEnableState == false)
					ZeroMemory(Data, 32);
				else
					FillMemory(Data, 32, 0x84);
				break;
			case 0xB: //	if ((dwAddress >= 0xB000) && (dwAddress <= 0xBFFF))
				if (tPak->iEnableState == true)
				{
					DebugWriteA( "Query Cart. State:" );
					if (tPak->bPakInserted)
					{
						if (tPak->iCurrentAccessMode == 1)
						{
							FillMemory(Data, 32, 0x89);
							DebugWriteA( " Inserted, Access Mode 1\n" );
						}
						else
						{
							FillMemory(Data, 32, 0x80);
							DebugWriteA( " Inserted, Access Mode 0\n" );
						}
						Data[0] = Data[0] | tPak->iAccessModeChanged;
					}
					else
					{
						FillMemory(Data, 32, 0x40); // Cart not inserted.
						DebugWriteA( " Not Inserted\n" );
					}
					tPak->iAccessModeChanged = 0;
				}
				break;
			case 0xC:
			case 0xD:
			case 0xE:
			case 0xF: //	if ((dwAddress >= 0xC000))
				if (tPak->iEnableState == true)
				{
					DebugWriteA( "Cart Read: Bank:%i\n", tPak->iCurrentBankNo );
					DebugWriteA( "    Address:%04X\n", ((dwAddress & 0xFFE0) - 0xC000) + ((tPak->iCurrentBankNo & 3) * 0x4000) );

					tPak->gbCart.ptrfnReadCart(&tPak->gbCart, ((dwAddress & 0xFFE0) - 0xC000) + ((tPak->iCurrentBankNo & 3) * 0x4000), Data);
				}
				break;
			default:
				DebugWriteA("WARNING: Unusual Pak Read\n" );
				DebugWriteA("  Address: %04X\n", dwAddress);
			} // end switch (dwAddress >> 12)

#ifdef ENABLE_RAWPAK_DEBUG
			DebugWriteA( "TPak Data: " );

			for (int i = 0; i < 32; i ++)
			{
				if ((i < 31) && ((i & 7) == 0)) DebugWriteA( "\n  " );
				DebugWriteByteA(Data[i]);
				if (i < 31)
				{
					DebugWriteA( ", ");
				}
			}
			DebugWriteA( "\n" );
#endif

			Data[32] = DataCRC( Data, 32 );

			bReturn = RD_OK;
		}
		break;
	/*case PAK_VOICE:
		break;*/
	case PAK_ADAPTOID:
		if( ReadAdaptoidPak( iControl, dwAddress, Data ) == DI_OK )
		{
			Data[32] = DataCRC( Data, 32 );
			bReturn = RD_OK;

			if( ((ADAPTOIDPAK*)g_pcControllers[iControl].pPakData)->fRumblePak )
			{
				BYTE bId = ((ADAPTOIDPAK*)g_pcControllers[iControl].pPakData)->bIdentifier;
				if(	(( dwAddress == 0x8000 ) && ( bId == 0x80 ) && ( Data[0] != 0x80 ))
					|| (( dwAddress == 0x8000 ) && ( bId != 0x80 ) && ( Data[0] != 0x00 ))
					|| (( dwAddress < 0x8000 ) && ( Data[0] != 0x00 )))
				{
					((ADAPTOIDPAK*)g_pcControllers[iControl].pPakData)->fRumblePak = false;
					DebugWriteA( "\nAssuming the inserted Pak AINT a RumblePak\nDisabling Rumblefix\n" );
				}	
			}
		}
		break;

	/*case PAK_NONE:
		break;*/
	}

	return bReturn;
}

// Called when the N64 tries to write to the controller pak, e.g. a mempak
BYTE WriteControllerPak( const int iControl, LPBYTE Command )
{
	BYTE bReturn = RD_ERROR;
	BYTE *Data = &Command[2];

#ifdef MAKEADRESSCRCCHECK
#pragma message( "Addresscheck for Pak-Writes active" )
	if( AddressCRC( Command ) != (Command[1] & 0x1F) )
	{
		g_pcControllers[iControl].fPakCRCError = true;
		if( WarningMessage( IDS_DLG_MEM_BADADDRESSCRC, MB_OKCANCEL | MB_ICONQUESTION ) == IDCANCEL )
			return RD_ERROR;
	}
#endif

	if( !g_pcControllers[iControl].pPakData )
		return RD_ERROR;

	WORD dwAddress = (Command[0] << 8) + (Command[1] & 0xE0);

	switch( *(BYTE*)g_pcControllers[iControl].pPakData )
	{
	case PAK_MEM:
		{
			// Switched to memory-mapped file
			// That way, if the computer dies due to power loss or something mid-play, the savegame is still there.
			MEMPAK *mPak = (MEMPAK*)g_pcControllers[iControl].pPakData;

			if( dwAddress < 0x8000 )
			{
				CopyMemory( &mPak->aMemPakData[dwAddress], Data, 32 );
				if (!mPak->fReadonly )
					SetTimer( g_strEmuInfo.hMainWindow, PAK_MEM, 2000, (TIMERPROC) WritebackProc ); // if we go 2 seconds without a write, call the Writeback proc (which will flush the cache)
			}
			else
				CopyMemory( &mPak->aMemPakTemp[(dwAddress%0x100)], Data, 32 );
			Data[32] = DataCRC( Data, 32 );
			bReturn = RD_OK;
		}
		break;
	case PAK_RUMBLE:
		if( dwAddress == PAK_IO_RUMBLE )
		{
			if( g_pcControllers[iControl].fXInput )	// xinput controller rumble --tecnicors
			{
				if( *Data )
					VibrateXInputController( g_pcControllers[iControl].xiController.nControl );
				else
					VibrateXInputController( g_pcControllers[iControl].xiController.nControl, 0, 0 );
				goto end_rumble;
			}

			if( g_pcControllers[iControl].fVisualRumble )
				FlashWindow( g_strEmuInfo.hMainWindow, ( *Data != 0 ) ? TRUE : FALSE );
			if( g_pcControllers[iControl].bRumbleTyp == RUMBLE_DIRECT )
			{  // Adaptoid Direct Rumble
				if( g_pcControllers[iControl].fIsAdaptoid )
					DirectRumbleCommand( iControl, *Data );
			}
			else
			{  // FF-FeedBack Rumble
				if( g_apdiEffect[iControl] )
				{
					g_apFFDevice[iControl]->Acquire();
					if( *Data )
					{
						// g_apdiEffect[iControl]->Start( 1, DIES_SOLO );
						HRESULT hr; 
						hr = g_apdiEffect[iControl]->Start( 1, DIES_NODOWNLOAD );
						if( hr != DI_OK )// just download if needed( seems to work smoother)
						{
							hr = g_apdiEffect[iControl]->Start( 1, 0 );
							if (hr != DI_OK)
							{
								DebugWriteA("Rumble: Can't rumble %d: %lX\n", iControl, hr);
							}
							else
								DebugWriteA("Rumble: DIES_NODOWNLOAD failed, regular OK on control %d\n", iControl);
						}
						else
							DebugWriteA("Rumble: DIES_NODOWNLOAD OK on control %d\n", iControl);
					}
					else
					{
						g_apdiEffect[iControl]->Stop();
					}
				}
			}
		}
		else if (dwAddress >= 0x8000 && dwAddress < 0x9000)
		{
			RUMBLEPAK *rPak = (RUMBLEPAK*)g_pcControllers[iControl].pPakData;
			rPak->fLastData = (*Data) ? true : false;
		}

end_rumble:		// added so after xinput controller rumbles, gets here --tecnicors
		Data[32] = DataCRC( Data, 32 );
		bReturn = RD_OK;
		break;
	case PAK_TRANSFER:
		{
			LPTRANSFERPAK tPak = (LPTRANSFERPAK)g_pcControllers[iControl].pPakData;
			// set bReturn = RD_OK when implementing Transferpak
			DebugWriteA( "TPak Write:\n" );
			DebugWriteA( "  Address: %04X\n", dwAddress );

#ifdef ENABLE_RAWPAK_DEBUG
			DebugWriteA( "  Data: ");

			for (int i = 0; i < 32; i++)
			{
				if ((i < 31) && ((i & 7) == 0))
				{
					DebugWriteA( "\n    " );
				}
				DebugWriteByteA( Data[i]);
				if (i < 31)
				{
					DebugWriteA( ", " );
				}
			}

			DebugWriteA( "\n" );
#endif // #ifdef ENABLE_RAWPAK_DEBUG

			switch (dwAddress >> 12)
			{
			case 0x8: //	if ((dwAddress >= 0x8000) && (dwAddress <= 0x8FFF))
				if (Data[0] == 0xFE)
				{
					DebugWriteA("Cart Disable\n" );
					tPak->iEnableState = false;
				}
				else if (Data[0] == 0x84)
				{
					DebugWriteA("Cart Enable\n" );
					tPak->iEnableState = true;
				}
				else
				{
					DebugWriteA("WARNING: Unusual Cart Enable/Disable\n" );
					DebugWriteA("  Address: " );
					DebugWriteWordA(dwAddress);
					DebugWriteA("\n" );
					DebugWriteA("  Data: " );
					DebugWriteByteA(Data[0]);
					DebugWriteA("\n" );
				}
				break;
			case 0xA: //	if ((dwAddress >= 0xA000) && (dwAddress <= 0xAFFF))
				if (tPak->iEnableState == true)
				{
					tPak->iCurrentBankNo = Data[0];
					DebugWriteA("Set TPak Bank No:%02X\n", Data[0] );
				}
				break;
			case 0xB: //	if ((dwAddress >= 0xB000) && (dwAddress <= 0xBFFF))
				if (tPak->iEnableState == true)
				{
					tPak->iCurrentAccessMode = Data[0] & 1;
					tPak->iAccessModeChanged = 4;
					DebugWriteA("Set TPak Access Mode: %04X\n", tPak->iCurrentAccessMode);
					if ((Data[0] != 1) && (Data[0] != 0))
					{
						DebugWriteA("WARNING: Unusual Access Mode Change\n" );
						DebugWriteA("  Address: " );
						DebugWriteWordA(dwAddress);
						DebugWriteA("\n" );
						DebugWriteA("  Data: " );
						DebugWriteByteA(Data[0]);
						DebugWriteA("\n" );
					}
				}
				break;
			case 0xC:
			case 0xD:
			case 0xE:
			case 0xF: //	if (dwAddress >= 0xC000)
				tPak->gbCart.ptrfnWriteCart(&tPak->gbCart, ((dwAddress & 0xFFE0) - 0xC000) + ((tPak->iCurrentBankNo & 3) * 0x4000), Data);
				if (tPak->gbCart.hRamFile != NULL )
					SetTimer( g_strEmuInfo.hMainWindow, PAK_TRANSFER, 2000, (TIMERPROC) WritebackProc ); // if we go 2 seconds without a write, call the Writeback proc (which will flush the cache)
				break;
			default:
				DebugWriteA("WARNING: Unusual Pak Write\n" );
				DebugWriteA("  Address: %04X\n", dwAddress);
			} // end switch (dwAddress >> 12)

			Data[32] = DataCRC( Data, 32 );
			bReturn = RD_OK; 
		}
		break;
	/*case PAK_VOICE:
		break;*/
	case PAK_ADAPTOID:
		if(( dwAddress == PAK_IO_RUMBLE ) && ((ADAPTOIDPAK*)g_pcControllers[iControl].pPakData)->fRumblePak )
		{
			if( DirectRumbleCommand( iControl, *Data ) == DI_OK )
			{
				Data[32] = DataCRC( Data, 32 );
				bReturn = RD_OK;
			}
		}
		else
		{
			if( WriteAdaptoidPak( iControl, dwAddress, Data ) == DI_OK )
			{
				Data[32] = DataCRC( Data, 32 );
				if( dwAddress == 0x8000 )
					((ADAPTOIDPAK*)g_pcControllers[iControl].pPakData)->bIdentifier = Data[0];

				bReturn = RD_OK;
			}
		}
		break;
	/*case PAK_NONE:
		break;*/
	}

	return bReturn;
}

void SaveControllerPak( const int iControl )
{
	if( !g_pcControllers[iControl].pPakData )
		return;

	switch( *(BYTE*)g_pcControllers[iControl].pPakData )
	{
	case PAK_MEM:
		{
			MEMPAK *mPak = (MEMPAK*)g_pcControllers[iControl].pPakData;

			if( !mPak->fReadonly )
				FlushViewOfFile( mPak->aMemPakData, PAK_MEM_SIZE );	// we've already written the stuff, just flush the cache
		}
		break;
	case PAK_RUMBLE:
		break;
	case PAK_TRANSFER:
		{
			LPTRANSFERPAK tPak = (LPTRANSFERPAK)g_pcControllers[iControl].pPakData;
			// here the changes( if any ) in the SRAM should be saved

			if (tPak->gbCart.hRamFile != NULL)
			{
				SaveCart(&tPak->gbCart, g_pcControllers[iControl].szTransferSave, _T(""));
				DebugWriteA( "*** Save Transfer Pak ***\n" );
			}
		}
		break;
	case PAK_VOICE:
		break;
	case PAK_ADAPTOID:
		break;
	/*case PAK_NONE:
		break;*/
	}
}

// if there is pPakData for the controller, does any closing of handles before freeing the pPakData struct and setting it to NULL
// also sets fPakInitialized to false
void CloseControllerPak( const int iControl )
{
	if( !g_pcControllers[iControl].pPakData )
		return;

	g_pcControllers[iControl].fPakInitialized = false;

	switch( *(BYTE*)g_pcControllers[iControl].pPakData )
	{
	case PAK_MEM:
		{
			MEMPAK *mPak = (MEMPAK*)g_pcControllers[iControl].pPakData;
			
			if( mPak->fReadonly )
			{
				P_free( mPak->aMemPakData );
				mPak->aMemPakData = NULL;
			}
			else
			{
				FlushViewOfFile( mPak->aMemPakData, PAK_MEM_SIZE );
				// if it's a dexsave, our original mapped view is not aMemPakData 
				UnmapViewOfFile( mPak->fDexSave ? mPak->aMemPakData - PAK_MEM_DEXOFFSET : mPak->aMemPakData );
				if ( mPak->hMemPakHandle != NULL )
					CloseHandle( mPak->hMemPakHandle );
			}
		}
		break;
	case PAK_RUMBLE:
		ReleaseEffect( g_apdiEffect[iControl] );
		g_apdiEffect[iControl] = NULL;
		break;
	case PAK_TRANSFER:
		{
			LPTRANSFERPAK tPak = (LPTRANSFERPAK)g_pcControllers[iControl].pPakData;
			UnloadCart(&tPak->gbCart);
			DebugWriteA( "*** Close Transfer Pak ***\n" );
			// close files and free any additionally ressources
		}
		break;
	case PAK_VOICE:
		break;
	case PAK_ADAPTOID:
		break;
	/*case PAK_NONE:
		break;*/
	}

	freePakData( &g_pcControllers[iControl] );
	return;
}

// returns the number of remaining blocks in a mempak
// aNoteSizes should be an array of 16 bytes, which will be overwritten with the size in blocks of each note
inline WORD CountBlocks( const unsigned char * bMemPakBinary, LPBYTE aNoteSizes )
{
	WORD wRemainingBlocks = 123;
	BYTE bNextIndex;
	int i = 0;
	while( i < 16 && wRemainingBlocks <= 123 )
	{
		aNoteSizes[i] = 0;
		bNextIndex = bMemPakBinary[0x307 + (i*0x20)];
		while(( bNextIndex >= 5 ) && ( aNoteSizes[i] < wRemainingBlocks))
		{
			aNoteSizes[i]++;
			bNextIndex = bMemPakBinary[0x101 + (bNextIndex*2)];
		}

		if( aNoteSizes[i] > wRemainingBlocks )
			wRemainingBlocks = 0xFF;
		else
			wRemainingBlocks -= aNoteSizes[i];
	
		i++;
	}
	return wRemainingBlocks;
}

void FormatMemPak( LPBYTE aMemPak )
{
	size_t iRand, n;

	FillMemory(aMemPak, 0x100, 0xFF);
	aMemPak[0] = 0x81;

	// generate a valid code( i hope i can calculate it one day)
	BYTE aValidCodes[] = {	0x12, 0xC5, 0x8F, 0x6F, 0xA4, 0x28, 0x5B, 0xCA };
	BYTE aCode[8];

	iRand  = ((size_t)aMemPak / 4) + ((size_t)g_strEmuInfo.hMainWindow / 4);
	iRand %= sizeof(aValidCodes) / 8;
	for (n = 0; n < 8; n++)
		aCode[n] = aValidCodes[n + iRand];

	//----------

	aMemPak[0x20+0] = aMemPak[0x60+0] = aMemPak[0x80+0] = aMemPak[0xC0+0] = 0xFF;
	aMemPak[0x20+1] = aMemPak[0x60+1] = aMemPak[0x80+1] = aMemPak[0xC0+1] = 0xFF;
	aMemPak[0x20+2] = aMemPak[0x60+2] = aMemPak[0x80+2] = aMemPak[0xC0+2] = 0xFF;
	aMemPak[0x20+3] = aMemPak[0x60+3] = aMemPak[0x80+3] = aMemPak[0xC0+3] = 0xFF;

	aMemPak[0x20+4] = aMemPak[0x60+4] = aMemPak[0x80+4] = aMemPak[0xC0+4] = aCode[0];
	aMemPak[0x20+5] = aMemPak[0x60+5] = aMemPak[0x80+5] = aMemPak[0xC0+5] = aCode[1];
	aMemPak[0x20+6] = aMemPak[0x60+6] = aMemPak[0x80+6] = aMemPak[0xC0+6] = aCode[2];
	aMemPak[0x20+7] = aMemPak[0x60+7] = aMemPak[0x80+7] = aMemPak[0xC0+7] = aCode[3];

	//aMemPak[0x30+9] = aMemPak[0x70+9] = aMemPak[0x90+9] = aMemPak[0xD0+9] = 0x01; // not sure
	aMemPak[0x30+10] = aMemPak[0x70+10] = aMemPak[0x90+10] = aMemPak[0xD0+10] = 0x01;

	aMemPak[0x30+12] = aMemPak[0x70+12] = aMemPak[0x90+12] = aMemPak[0xD0+12] = aCode[4];
	aMemPak[0x30+13] = aMemPak[0x70+13] = aMemPak[0x90+13] = aMemPak[0xD0+13] = aCode[5];
	aMemPak[0x30+14] = aMemPak[0x70+14] = aMemPak[0x90+14] = aMemPak[0xD0+14] = aCode[6];
	aMemPak[0x30+15] = aMemPak[0x70+15] = aMemPak[0x90+15] = aMemPak[0xD0+15] = aCode[7];

	// Index
	ZeroMemory( &aMemPak[0x100], 0x400 );

	aMemPak[0x100+1] = aMemPak[0x200+1] = 0x71;
	for( int i = 0x00b; i < 0x100; i += 2 )
		aMemPak[0x100+i] = aMemPak[0x200+i] = 03;

	FillMemory( &aMemPak[0x500], 0x7B00, 0xFF );
}

// Translates a mempak header into a real Unicode string, for display in the Mempaks window
// bNote is now where you want to START translating, Text is where the output gets sent, iChars is the number of TCHARs you want translated
// return value is the number of characters actually translated
// Text automatically gets a terminating null, so make sure there's enough space.
int TranslateNotesW( const unsigned char * bNote, LPWSTR Text, const int iChars )
{
	WCHAR aSpecial[] =	{	0x0021,	0x0022,	0x0023,	0x0060,	0x002A,	0x002B,	0x002C,	0x002D,	0x002E,	0x002F,	0x003A,	0x003D,	0x003F,	0x0040,	0x2122,	0x00A9,	0x00AE };
					//	{	'!' ,	'\"',	'#' ,	'`' ,	'*' ,	'+' ,	',' ,	'-' ,	'.' ,	'/' ,	':' ,	'=' ,	'?' ,	'>' ,	'tm',	'(c)',	'(r)' };
	const WCHAR aJmap[] =	{	// map of Japanese characters N64 to UTF-16, starting at 0x45
								0x00A1, 0x30A3, 0x30A5, 0x30A7, 0x30A9, 0x30E3, 0x30E5, 0x30E7, 0x30C3, 0x30F2,	// small a-i-u-e-o, next are probably small ya-yu-yo, small tsu, wo
								0x30F3,		// 0x4F -> 'n'
								0x30A2, 0x30A4, 0x30A6, 0x30A8, 0x30AA, 0x30AB, 0x30AD, 0x30AF, 0x30B1, 0x30B3, 0x30B5, 0x30B7, 0x30B9, 0x30BB, 0x30BD, // nil-K-S
								0x30BF, 0x30C1, 0x30C4, 0x30C6, 0x30C8, 0x30CA, 0x30CB, 0x30CC, 0x30CD, 0x30CE, 0x30CF, 0x30D2, 0x30D5, 0x30D8, 0x30DB,	// T-N-H
								0x30DE, 0x30DF, 0x30E0, 0x30E1, 0x30E2, 0x30E4, 0x30E6, 0x30E8, 0x30E9, 0x30EA, 0x30EB, 0x30EC, 0x30ED,	// M-Y-R
								0x30EF,		// 'wa'
								0x30AC, 0x30AE, 0x30B0, 0x30B2, 0x30B4, 0x30B6, 0x30B8, 0x30BA, 0x30BC, 0x30BE, 0x30C0, 0x30C2, 0x30C5, 0x30C7, 0x30C9,	// G-Z-D
								0x30D0, 0x30D3, 0x30D6, 0x30D9, 0x30DC, 0x30D1, 0x30D4, 0x30D7, 0x30DA, 0x30DD	// B-P
							};
	int iReturn = 0;

	while (iChars - iReturn > 0 && *bNote)
	{
		BYTE b = *bNote;
		if( b <= 0x0F ) // translate Icons as Spaces
			*Text = 0x0020;
		else if( b <= 0x19 ) // Numbers
			*Text = 0x0020 + b;
		else if( b <= 0x33 ) // English characters
			*Text = 0x0047 + b;
		else if( b <= 0x44 ) // special symbols
			*Text = aSpecial[b - 0x34];
		else if( b <= 0x94 ) // Japanese (halfwidth katakana, mapped similarly to JIS X 0201 but not enough that we can use a simple codepage)
		{
			aSpecial[7] = 0x30fc; // change regular dash to Japanese "long sound dash"
			*Text = aJmap[b - 0x45];
		}
		else // unknown
			*Text = 0x00A4; // unknown characters become "currency sign" (looks like a letter o superimposed on an x)
		Text++;
		iReturn++;
		bNote++;
	}

	*Text = L'\0';

	return iReturn;
}

// TODO: rename this function!  It serves a completely different function from TranslateNotesW
// Translates a mempak header into an ASCII string for output to .a64 file
// bNote is now where you want to START translating, Text is where the output gets sent, iChars is the number of chars you want translated
// return value is the number of characters actually translated
// Text automatically gets a terminating null, so make sure there's enough space.
int TranslateNotesA( const unsigned char * bNote, LPSTR Text, const int iChars )
{
	const UCHAR aSpecial[] ={ 0x21, 0x22, 0x23, 0x60, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x3A, 0x3D, 0x3F, 0x40, 0x99, 0xA9, 0xAE };
						//	{ '!' , '\"', '#' , '`' , '*' , '+' , ',' , '-' , '.' , '/' , ':' , '=' , '?' , '>' , 'tm', '(c)','(r)' };
	int iReturn = 0;

	while (iChars - iReturn > 0 && *bNote)
	{
		BYTE b = *bNote;
		if( b <= 0x0F ) // translate Icons as Spaces
			*Text = 0x20;
		else if( b <= 0x19 ) // Numbers
			*Text = 0x20 + b;
		else if( b <= 0x33 ) // English characters
			*Text = 0x47 + b;
		else if( b <= 0x44 ) // special symbols
			*Text = aSpecial[b - 0x34];
		else if( b <= 0x94 ) // Japan
			*Text = 0xC0 + ( b % 40 );
		else // unknown
			*Text = (UCHAR)0xA4; // HACK: this will screw up any save with unknown characters

		Text++;
		iReturn++;
		bNote++;
	}

	*Text = '\0';
	return iReturn;
}

int ReverseNotesA( LPCSTR Text, LPBYTE Note )
{
	const UCHAR aSpecial[] =	{ 0x21, 0x22, 0x23, 0x60, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x3A, 0x3D, 0x3F, 0x40, 0x74, 0xA9, 0xAE };
						//	{ '!' , '\"', '#' , '`' , '*' , '+' , ',' , '-' , '.' , '/' , ':' , '=' , '?' , '>' , 'tm', '(r)','(c)' };

	LPCSTR TextPos = Text;
	while( *TextPos != '\0' )
	{
		char c = *TextPos;
		*Note = 0x0F;

		if( c >= '0' && c <= '9' )
			*Note = c - '0' + 0x10;
		else if( c >= 'A' && c <= 'Z' )
			*Note = c - 'A' + 0x1A;
		else if( c >= 'a' && c <= 'z' )
			*Note = c - 'a' + 0x1A;

		else
		{
			for( int i = 0; i < ARRAYSIZE(aSpecial); ++i )
			{
				if( c == aSpecial[i] )
				{
					*Note = i + 0x34;
					break;
				}
			}
		}
		TextPos++;
		Note++;
	}
	return TextPos - Text;
}

WORD ShowMemPakContent( const unsigned char * bMemPakBinary, HWND hListWindow )
{
	BYTE bMemPakValid = MPAK_OK;
	TCHAR szBuffer[40];
	BYTE aNoteSizes[16];
	bool bFirstChar;


	LVITEM lvItem;
	lvItem.mask = LVIF_TEXT | LVIF_PARAM;
	lvItem.iItem = 0;
	lvItem.iSubItem = 0;
	lvItem.pszText = szBuffer;

	int i = 0,
		nNotes = 0,
		iSum = 0,
		iRemainingBlocks = 0;

	for( i = 0x10A; i < 0x200; i++ )
		iSum += bMemPakBinary[i];


	if((( iSum % 256 ) == bMemPakBinary[0x101] ))
	{
		iRemainingBlocks = CountBlocks( bMemPakBinary, aNoteSizes );

		if( iRemainingBlocks <= 123 )
		{
			for( lvItem.lParam = 0; lvItem.lParam < 16; lvItem.lParam++ )
			{
				
				if( bMemPakBinary[0x300 + (lvItem.lParam*32)] ||
					bMemPakBinary[0x301 + (lvItem.lParam*32)] ||
					bMemPakBinary[0x302 + (lvItem.lParam*32)] )
				{
					int iChars = TranslateNotes( &bMemPakBinary[0x300 + (lvItem.lParam*32) + 0x10], szBuffer, 16 );

					if( TranslateNotes( &bMemPakBinary[0x300 + (lvItem.lParam*32) + 0x0C], &szBuffer[iChars + 1], 1 ) )
						szBuffer[iChars] = _T('_');

					bFirstChar = true;
					for( i = 0; i < (int)lstrlen(szBuffer); i++ )
					{
						if( szBuffer[i] == ' ' )
							bFirstChar = true;
						else
						{
							if( bFirstChar && ( szBuffer[i] >= 'a') && ( szBuffer[i] <= 'z'))
							{
								bFirstChar = false;
								szBuffer[i] -= 0x20;
							}
						}

					}
	
					i = ListView_InsertItem( hListWindow, &lvItem );

					switch( bMemPakBinary[0x303 + (lvItem.lParam*32)] )
					{
					case 0x00:
						LoadString( g_hResourceDLL, IDS_P_MEM_NOREGION, szBuffer, 40 );
						break;
					case 0x37:
						LoadString( g_hResourceDLL, IDS_P_MEM_BETA, szBuffer, 40 );
						break;
					case 0x41:
						LoadString( g_hResourceDLL, IDS_P_MEM_NTSC, szBuffer, 40 );
						break;
					case 0x44:
						LoadString( g_hResourceDLL, IDS_P_MEM_GERMANY, szBuffer, 40 );
						break;
					case 0x45:
						LoadString( g_hResourceDLL, IDS_P_MEM_USA, szBuffer, 40 );
						break;
					case 0x46:
						LoadString( g_hResourceDLL, IDS_P_MEM_FRANCE, szBuffer, 40 );
						break;
					case 0x49:
						LoadString( g_hResourceDLL, IDS_P_MEM_ITALY, szBuffer, 40 );
						break;
					case 0x4A:
						LoadString( g_hResourceDLL, IDS_P_MEM_JAPAN, szBuffer, 40 );
						break;
					case 0x50:
						LoadString( g_hResourceDLL, IDS_P_MEM_EUROPE, szBuffer, 40 );
						break;
					case 0x53:
						LoadString( g_hResourceDLL, IDS_P_MEM_SPAIN, szBuffer, 40 );
						break;
					case 0x55:
						LoadString( g_hResourceDLL, IDS_P_MEM_AUSTRALIA, szBuffer, 40 );
						break;
					case 0x58:
					case 0x59:
						LoadString( g_hResourceDLL, IDS_P_MEM_PAL, szBuffer, 40 );
						break;
					default:
						{
							TCHAR szTemp[40];
							LoadString( g_hResourceDLL, IDS_P_MEM_UNKNOWNREGION, szTemp, 40 );
							wsprintf( szBuffer, szTemp, bMemPakBinary[0x303 + (lvItem.lParam*32)] );
						}
					}

					ListView_SetItemText( hListWindow, i, 1, szBuffer );

					wsprintf( szBuffer, _T("%i"), aNoteSizes[lvItem.lParam] );
					ListView_SetItemText( hListWindow, i, 2, szBuffer );
					nNotes++;
				}
			}
			
		}
		else
			bMemPakValid = MPAK_DAMAGED;

	}
	else
		bMemPakValid = MPAK_DAMAGED;

	return MAKEWORD( (BYTE)iRemainingBlocks, bMemPakValid );
}

void HextoTextA( const unsigned char * Data, LPSTR szText, const int nBytes )
{
	const char acValues[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
							  'A', 'B', 'C', 'D', 'E', 'F' };

	for( int i = 0; i < nBytes; i++ )
	{
		BYTE byte = *Data;
		szText[0] = acValues[(byte>>4) & 0x0F];
		szText[1] = acValues[byte & 0x0F];

		++Data;
		szText+=2;
	}
	*szText = '\0';
}

// used when reading in a Note file, to convert text to binary (unserialize)
void TexttoHexA( LPCSTR szText, LPBYTE Data, const int nBytes )
{
	bool fLowByte = false;
	LPCSTR endText = szText + nBytes * 2;

	for( ; szText < endText; ++szText )
	{
		BYTE bByte = 0;

		if(( '0' <= *szText ) && ( *szText <= '9' ))
			bByte = *szText - '0';
		else
		{
			if(( 'A' <= *szText ) && ( *szText <= 'F' ))
				bByte = szText[0] - 'A' + 10;
			else if(( 'a' <= *szText ) && ( *szText <= 'f' ))
				bByte = szText[0] - 'a' + 10;
		}

		if( !fLowByte )
			*Data = bByte << 4;
		else
		{
			*Data |= bByte;
			++Data;
		}

		fLowByte = !fLowByte;
	}
}

bool SaveNoteFileA( const unsigned char * aMemPak, const int iNote, LPCTSTR pszFileName )
{
	BYTE aNoteSizes[16];
	LPBYTE aNote;
	bool bReturn = false;
	if( CountBlocks( aMemPak, aNoteSizes ) > 123 )
		return false;

	aNote = (LPBYTE)P_malloc( aNoteSizes[iNote] * 0x100 + 32 );
	if( !aNote )
		return false;

	CopyMemory( aNote, &aMemPak[0x300 + iNote * 32], 32 );
	BYTE bNextIndex = aNote[0x7];
	int iBlock = 0;
	while( iBlock < aNoteSizes[iNote] )
	{
		CopyMemory( &aNote[32 + iBlock * 0x100], &aMemPak[bNextIndex * 0x100], 0x100);
		bNextIndex = aMemPak[0x101 + (bNextIndex*2)];
		
		iBlock++;
	}

	HANDLE hFile = CreateFile( pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if ( hFile != INVALID_HANDLE_VALUE )
	{	
		SetFilePointer( hFile, 0L, NULL, FILE_BEGIN );
		char szLine[70];
		
		
		DWORD dwBytesWritten = 0;
		lstrcpyA( szLine, "a64-notes\r\ndescription of game save goes here...\r\na64-data\r\n" );
		WriteFile( hFile, szLine, lstrlenA(szLine), &dwBytesWritten, NULL );

		CopyMemory( szLine, aNote, 4 );
		szLine[4] = ' ';
		szLine[5] = aNote[4];
		szLine[6] = aNote[5];
		szLine[7] = ' ';
		HextoTextA( &aNote[8], &szLine[8], 2 );

		int pos = 12;
		szLine[pos++] = ' ';
		szLine[pos++] = aNote[0x0A] + '0';
		szLine[pos++] = ' ';
		szLine[pos++] = '{';
		
		pos += TranslateNotesA( &aNote[0x0C], &szLine[pos], 1 );

		szLine[pos++] = '}';
		szLine[pos++] = ' ';
		szLine[pos++] = '{';

		pos += TranslateNotesA( &aNote[0x10], &szLine[pos], 16 );

		lstrcatA( szLine, "}\r\n" );
		WriteFile( hFile, szLine, lstrlenA(szLine), &dwBytesWritten, NULL );

		for( int i = 32; i < aNoteSizes[iNote] * 0x100 + 32; i += 32 )
		{
			HextoTextA( &aNote[i], szLine, 32 );
			WriteFile( hFile, szLine, lstrlenA(szLine), &dwBytesWritten, NULL );
			WriteFile( hFile, "\r\n", 2, &dwBytesWritten, NULL );
		}
		WriteFile( hFile, "a64-crc\r\n", 9, &dwBytesWritten, NULL );

		// TODO: insert crc here
		lstrcpynA( szLine, "00000000\r\n", 70 );
		WriteFile( hFile, szLine, lstrlenA(szLine), &dwBytesWritten, NULL );
		//
		WriteFile( hFile, "a64-end\r\n", 9, &dwBytesWritten, NULL );

		SetEndOfFile( hFile );
		bReturn = true;
		CloseHandle( hFile );
	}
	else
		ErrorMessage( IDS_ERR_NOTEREAD, GetLastError(), false );

	P_free( aNote );
	return bReturn;
}

// read a Note from a file pszFileName (.a64 format), and insert it into the given MemPak
// returns true on success, false otherwise
bool InsertNoteFile( LPBYTE aMemPak, LPCTSTR pszFileName )
{
//	bool bReturn = false;

	FILE* nFile = NULL;

	if( (nFile = _tfopen(pszFileName, _T("r") ) ) != NULL )
	{
		char szLine[128];
		fpos_t pDataStart;

		while( fgets(szLine, sizeof(szLine) - 1, nFile) )
		{
			if( !strncmp( "a64-data", szLine, 8 ))
			{
				fgetpos(nFile, &pDataStart);
				break;
			}
		}

		// assumes the file keeps going...
		// discard the next line
		fgets(szLine, sizeof(szLine) - 1, nFile); // not really necessary to check for EOF, as it will fail gracefully when dwNoteSize is zero.

		DWORD dwNoteSize = 0;
		while( fgets(szLine, sizeof(szLine) - 1, nFile) && strncmp( "a64-crc", szLine, 7 ))
		{
			dwNoteSize++;
		}
		dwNoteSize /= 8;

		BYTE aNoteSizes[16];
		WORD wRemainingBlocks;
		int i,
			ifreeNote = -1;

		wRemainingBlocks = CountBlocks( aMemPak, aNoteSizes );

		if( dwNoteSize <= 0 )
		{
			ErrorMessage( IDS_ERR_NOTEREAD, 0, false );
			fclose(nFile);
			return false;
		}
		else
		{
			if( wRemainingBlocks < dwNoteSize )
			{
				ErrorMessage( IDS_ERR_MEMPAK_SPACE, 0, false );
				fclose(nFile);
				return false;
			}
			else
			{
				i = 0;
				while(( i < 16 ) && ( ifreeNote == -1 ))
				{
					if( aNoteSizes[i] == 0 )
						ifreeNote = i;
					i++;
				}
				if( ifreeNote == -1 )
				{
                    ErrorMessage( IDS_ERR_MEMPAK_NONOTES, 0, false );
					fclose(nFile);
					return false;
				}
			}
		}

		// HEADER START

		// .a64 header should look something like this:
		// NBCE 01 0203 0 {} {BLASTCORPS GAME}
		// first 4 chars are the first 4 bytes
		// next 2 chars are the next 2 bytes
		// next 4 chars are bytes 8 and 9, in hex (huh?)
		// next character is byte 10, in hex (but only one character this time)
		// now we've got two sets of braces... the first one contains byte 12 in encoded form (use ReverseNotesA)
		// the second one should contain bytes 16 through 31 (ReverseNotesA)

		BYTE *pBlock;
		fsetpos(nFile, &pDataStart);
		if (! fgets(szLine, sizeof(szLine) - 1, nFile) )
		{
			ErrorMessage( IDS_ERR_NOTEEOF, 0, false );
			fclose(nFile);
			return false;
		}

		szLine[strlen(szLine) - 1] = '\0'; // remove newline
		
		pBlock = &aMemPak[0x300 + ifreeNote*32];
		CopyMemory( pBlock, szLine, 4 );
		pBlock[4] = szLine[5];
		pBlock[5] = szLine[6];
		TexttoHexA( &szLine[8], &pBlock[8], 2 );
		pBlock[10] = szLine[13] - '0';

		int len = lstrlenA( szLine );

		i = 16;
		while(( szLine[i] != '}' ) && (i < len))
			i++;

		szLine[i] = '\0';
		i += ReverseNotesA( &szLine[16], &pBlock[12] );

		while(( szLine[i] != '{' ) && (i < len))
			i++;

		if(i < len)
		{
			int start = i+1;
			while(( szLine[i] != '}' ) && (i < len))
				i++;
			if(i < len)
			{
				szLine[i] = '\0';
				ReverseNotesA( &szLine[start], &pBlock[16] );
			}
		}

		while(( szLine[i] != '}' ) && (i < len))
			i++;
		szLine[i] = '\0';

		// HEADER END

		BYTE bDataBlock = 5;
		pBlock = &pBlock[7];
		BYTE *pDataBlock;

		while( dwNoteSize > 0 )
		{
			while( aMemPak[0x101 + bDataBlock*2] != 0x03 )
				bDataBlock++;
			*pBlock = bDataBlock;
			pBlock = &aMemPak[0x101 + bDataBlock*2];
			pDataBlock = &aMemPak[bDataBlock * 0x100];
			for( i = 0; i < 0x100; i+=32 )
			{
				if (! fgets(szLine, sizeof(szLine) - 1, nFile) )
				{
					ErrorMessage( IDS_ERR_NOTEEOF, 0, false );
					fclose(nFile);
					return false;
				}

				szLine[strlen(szLine) - 1] = '\0'; // remove newline
				TexttoHexA( szLine, &pDataBlock[i], 32 );
			}
			bDataBlock++;
			dwNoteSize--;
		}
		*pBlock = 0x01;

		int iSum = 0;

		for( i = 0x10A; i < 0x200; i++ )
			iSum += aMemPak[i];

		aMemPak[0x101] = iSum % 256;

		CopyMemory( &aMemPak[0x200], &aMemPak[0x100], 0x100 );

		fclose(nFile);
		return true;
	}
	else
		ErrorMessage( IDS_ERR_NOTEREAD, 0, false );
	return false;
}

// Remove a mempak "Note"
// See "MemPak-Format.doc" for more info
bool RemoveNote( LPBYTE aMemPak, const int iNote )
{
	BYTE bBlock = aMemPak[0x307 + iNote*32];
	int iPos;

	while( bBlock >= 0x05 )
	{
		iPos = 0x101 + bBlock*2;
		bBlock = aMemPak[iPos];
		aMemPak[iPos] = 0x03;
	}

	int i = 0, iSum = 0;
	for( i = 0x10A; i < 0x200; i++ )
		iSum += aMemPak[i];

	aMemPak[0x101] = iSum % 256;
	CopyMemory( &aMemPak[0x200], &aMemPak[0x100], 0x100 );
	ZeroMemory( &aMemPak[0x300 + iNote*32], 32 );
	return true;
}

BYTE AddressCRC( const unsigned char * Address )
{
	bool HighBit;
	WORD Data = MAKEWORD( Address[1], Address[0] );
	register BYTE Remainder = ( Data >> 11 ) & 0x1F;

	BYTE bBit = 5;

	while( bBit < 16 )
	{
		HighBit = (Remainder & 0x10) != 0;
		Remainder = (Remainder << 1) & 0x1E;

		Remainder += ( bBit < 11 && Data & (0x8000 >> bBit )) ? 1 : 0;
		Remainder ^= (HighBit) ? 0x15 : 0;
		
		bBit++;
	}

	return Remainder;
}

BYTE DataCRC( const unsigned char * Data, const int iLength )
{
	register BYTE Remainder = Data[0];

	int iByte = 1;
	BYTE bBit = 0;

	while( iByte <= iLength )
	{
		bool HighBit = ((Remainder & 0x80) != 0);
		Remainder = Remainder << 1;

		Remainder += ( iByte < iLength && Data[iByte] & (0x80 >> bBit )) ? 1 : 0;

		Remainder ^= (HighBit) ? 0x85 : 0;
		
		bBit++;
		iByte += bBit/8;
		bBit %= 8;
	}

	return Remainder;
}

VOID CALLBACK WritebackProc( HWND hWnd, UINT msg, UINT_PTR idEvent, DWORD dwTime )
{
	KillTimer(hWnd, idEvent); // timer suicide

	switch (idEvent)
	{
	case PAK_MEM:
		DebugWriteA("Mempak: WritebackProc flushed file writes\n");
		for( int i = 0; i < 4; i++ )
		{
			MEMPAK *mPak = (MEMPAK*)g_pcControllers[i].pPakData;

			if ( mPak && mPak->bPakType == PAK_MEM && !mPak->fReadonly && mPak->hMemPakHandle != NULL )
				FlushViewOfFile( mPak->aMemPakData, PAK_MEM_SIZE );
		}
		return;
	case PAK_TRANSFER:
		DebugWriteA("TPak: WritebackProc flushed file writes\n");
		for( int i = 0; i < 4; i++ )
		{
			LPTRANSFERPAK tPak = (LPTRANSFERPAK)g_pcControllers[i].pPakData;

			if (tPak && tPak->bPakType == PAK_TRANSFER && tPak->bPakInserted && tPak->gbCart.hRamFile != NULL )
				FlushViewOfFile( tPak->gbCart.RamData, (tPak->gbCart.RomData[0x149] == 1 ) ? 0x0800 : tPak->gbCart.iNumRamBanks * 0x2000);
		}
		return;
	}
}
