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

class CSettingTypeTempBool :
	public CSettingType
{
	bool m_value;

public:
	CSettingTypeTempBool(bool initialValue );
	~CSettingTypeTempBool();

	bool        IndexBasedSetting ( void ) const { return false; }
	SettingType GetSettingType    ( void ) const { return SettingType_BoolVariable; }	

	//return the values
	bool Load   ( int Index, bool & Value   ) const; 
	bool Load   ( int Index, ULONG & Value  ) const;
	bool Load   ( int Index, stdstr & Value ) const; 

	//return the default values
	void LoadDefault ( int Index, bool & Value   ) const; 
	void LoadDefault ( int Index, ULONG & Value  ) const; 
	void LoadDefault ( int Index, stdstr & Value ) const; 

	//Update the settings
	void Save   ( int Index, bool Value ); 
	void Save   ( int Index, ULONG Value ); 
	void Save   ( int Index, const stdstr & Value );
	void Save   ( int Index, const char * Value );

	// Delete the setting
	void Delete ( int Index ); 
};

