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

class CSettingTypeApplicationIndex :
	public CSettingTypeApplication
{

public:
	CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, LPCSTR DefaultValue );
	CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, bool DefaultValue );
	CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, DWORD DefaultValue );
	CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting );
	~CSettingTypeApplicationIndex();

	virtual bool            IndexBasedSetting   ( void ) const { return true; }

	//return the values
	virtual bool Load ( int Index, bool & Value   ) const; 
	virtual bool Load ( int Index, ULONG & Value  ) const;
	virtual bool Load ( int Index, stdstr & Value ) const; 

	//return the default values
	virtual void LoadDefault ( int Index, bool & Value   ) const; 
	virtual void LoadDefault ( int Index, ULONG & Value  ) const; 
	virtual void LoadDefault ( int Index, stdstr & Value ) const; 

	//Update the settings
	virtual void Save ( int Index, bool Value ); 
	virtual void Save ( int Index, ULONG Value ); 
	virtual void Save ( int Index, const stdstr & Value );
	virtual void Save ( int Index, const char * Value );

	// Delete the setting
	virtual void Delete ( int Index ); 
};

