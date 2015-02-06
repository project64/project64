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

class CSettingTypeRomDatabase :
	public CSettingType
{
public:
	CSettingTypeRomDatabase(LPCSTR Name, LPCSTR DefaultValue, bool DeleteOnDefault = false );
	CSettingTypeRomDatabase(LPCSTR Name, bool DefaultValue, bool DeleteOnDefault = false );
	CSettingTypeRomDatabase(LPCSTR Name, int DefaultValue, bool DeleteOnDefault = false );
	CSettingTypeRomDatabase(LPCSTR Name, SettingID DefaultSetting, bool DeleteOnDefault = false );
	
	virtual ~CSettingTypeRomDatabase();

	virtual bool        IndexBasedSetting ( void ) const { return false; }
	virtual SettingType GetSettingType    ( void ) const { return SettingType_RomDatabase; }	

	//return the values
	virtual bool Load   ( int Index, bool & Value   ) const; 
	virtual bool Load   ( int Index, ULONG & Value  ) const;
	virtual bool Load   ( int Index, stdstr & Value ) const; 

	//return the default values
	virtual void LoadDefault ( int Index, bool & Value   ) const; 
	virtual void LoadDefault ( int Index, ULONG & Value  ) const; 
	virtual void LoadDefault ( int Index, stdstr & Value ) const; 

	//Update the settings
	virtual void Save   ( int Index, bool Value ); 
	virtual void Save   ( int Index, ULONG Value ); 
	virtual void Save   ( int Index, const stdstr & Value );
	virtual void Save   ( int Index, const char * Value );

	// Delete the setting
	virtual void Delete ( int Index ); 

	static void Initilize ( void );
	static void CleanUp   ( void );

protected:
	static void GameChanged ( void * /*Data */ );

	static bool IsGlideSetting (LPCSTR Name);
	static LPCSTR StripNameSection (LPCSTR Name);
	virtual LPCSTR Section ( void ) const { return m_SectionIdent->c_str(); }

	mutable stdstr  m_KeyName;
	const LPCSTR    m_DefaultStr;
	const int       m_DefaultValue;
	const SettingID m_DefaultSetting;
	const bool      m_DeleteOnDefault;
	bool            m_GlideSetting;

	static stdstr   * m_SectionIdent;
	static CIniFile * m_SettingsIniFile;
	static CIniFile * m_GlideIniFile;
};

