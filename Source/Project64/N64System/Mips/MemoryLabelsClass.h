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
#pragma once

typedef std::map<DWORD, stdstr> StringMap;

class CMemoryLabel
{
	// Variable dealing with Labels
	StringMap m_LabelList;
	int       m_NewLabels;
	stdstr    CurrentLabelFile;

	DWORD  AsciiToHex     ( char * HexValue );
	void   ProcessCODFile ( BYTE * File, DWORD FileLen );

public:
	//Functions related to Labels
	void   AddMemoryLabel  ( DWORD Address, const char * Message, ...  );
	stdstr LabelName       ( DWORD Address ) const;
	stdstr StoredLabelName ( DWORD Address );
	void   LoadLabelList   ( char * file );
	int    NewLabels       (); // How many new labels been added since loading/saving label file
	void   SaveLabelList   ();
};
