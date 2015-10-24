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

class CSettingTypeApplication :
	public CSettingType
{
	
protected:
	const LPCSTR    m_DefaultStr;
	const DWORD     m_DefaultValue;
	const SettingID m_DefaultSetting;

	stdstr FixSectionName (LPCSTR Section);

	static CIniFile * m_SettingsIniFile;
	static bool       m_UseRegistry;
	const stdstr      m_Section;
	const stdstr      m_KeyName;
	mutable stdstr    m_KeyNameIdex;

	virtual LPCSTR    SectionName ( void ) const;

public:
	CSettingTypeApplication(LPCSTR Section, LPCSTR Name, LPCSTR DefaultValue );
	CSettingTypeApplication(LPCSTR Section, LPCSTR Name, bool DefaultValue );
	CSettingTypeApplication(LPCSTR Section, LPCSTR Name, DWORD DefaultValue );
	CSettingTypeApplication(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting );
	virtual ~CSettingTypeApplication();

	virtual bool        IndexBasedSetting ( void ) const { return false; }
	virtual SettingType GetSettingType    ( void ) const { return m_UseRegistry ? SettingType_Registry : SettingType_CfgFile; }	

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

	// Initialize this class to use ini or registry
	static void Initialize( const char * AppName );
	static void CleanUp   ( void );
	static void Flush     ( void );

	LPCSTR GetKeyName ( void) const { return m_KeyName.c_str(); }
};

