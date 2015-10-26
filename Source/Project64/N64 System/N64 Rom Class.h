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
#pragma once

#include "N64 Types.h"

class CN64Rom :
	protected CDebugSettings
{
	//constant values
	enum { ReadFromRomSection = 0x400000 };
	
	//class variables
	HANDLE  m_hRomFile, m_hRomFileMapping;
	BYTE *  m_ROMImage;
	DWORD   m_RomFileSize;
	Country m_Country;
	CICChip m_CicChip;
	LanguageStringID m_ErrorMsg;
	stdstr m_RomName, m_FileName, m_MD5, m_RomIdent;

	bool   AllocateAndLoadN64Image ( const char * FileLoc, bool LoadBootCodeOnly );
	bool   AllocateAndLoadZipImage ( const char * FileLoc, bool LoadBootCodeOnly );
	void   ByteSwapRom             ();
	void   SetError                ( LanguageStringID ErrorMsg );
	static void  __stdcall NotificationCB ( LPCWSTR Status, CN64Rom * _this );
	void   CalculateCicChip        ();
	void   CalculateRomCrc         ();

public:
	CN64Rom();
	~CN64Rom();

	bool    LoadN64Image       ( const char * FileLoc, bool LoadBootCodeOnly = false );
	static bool IsValidRomImage( BYTE Test[4] );
	void    SaveRomSettingID   ( bool temp );
	void    ClearRomSettingID  ();
	CICChip CicChipID          ();
	BYTE *  GetRomAddress      () { return m_ROMImage; }
	DWORD   GetRomSize         () const { return m_RomFileSize; }
	stdstr  GetRomMD5          () const { return m_MD5; }
	stdstr  GetRomName         () const { return m_RomName; }
	stdstr  GetFileName        () const { return m_FileName; }
	Country GetCountry         () const { return m_Country; }
	void    UnallocateRomImage ();

	//Get a message id for the reason that you failed to load the rom
	LanguageStringID GetError  () const { return m_ErrorMsg; }
};
