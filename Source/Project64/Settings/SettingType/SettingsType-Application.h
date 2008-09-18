#pragma once

class CSettingTypeApplication :
	public CSettingType
{
	
	const LPCSTR    m_DefaultStr;
	const DWORD     m_DefaultValue;
	const SettingID m_DefaultSetting;

	stdstr FixSectionName (LPCSTR Section);

protected:
	static CIniFile * m_SettingsIniFile;
	static bool       m_UseRegistry;
	const stdstr      m_Section;
	const LPCSTR      m_KeyName;
	mutable stdstr    m_KeyNameIdex;

public:
	CSettingTypeApplication(LPCSTR Section, LPCSTR Name, LPCSTR DefaultValue );
	CSettingTypeApplication(LPCSTR Section, LPCSTR Name, bool DefaultValue );
	CSettingTypeApplication(LPCSTR Section, LPCSTR Name, DWORD DefaultValue );
	CSettingTypeApplication(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting );
	~CSettingTypeApplication();

	virtual bool            IndexBasedSetting   ( void ) const { return false; }
	virtual SettingLocation GetSettingsLocation ( void ) const { return m_UseRegistry ? SettingLocation_Registry : SettingLocation_CfgFile; }	

	//return the values
	virtual bool Load ( int Index, bool & Value   ) const; 
	virtual bool Load ( int Index, ULONG & Value  ) const;
	virtual bool Load ( int Index, stdstr & Value ) const; 

	//Update the settings
	virtual void Save ( int Index, bool Value ); 
	virtual void Save ( int Index, ULONG Value ); 
	virtual void Save ( int Index, const stdstr & Value );
	virtual void Save ( int Index, const char * Value );

	// Initilize this class to use ini or registry
	static void Initilize ( const char * AppName );
	static void CleanUp   ( void );
};

