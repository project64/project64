#pragma once

class CSettingTypeRomDatabase :
	public CSettingType
{
	
	const LPCSTR    m_KeyName;
	const LPCSTR    m_DefaultStr;
	const int       m_DefaultValue;
	const SettingID m_DefaultSetting;

	static CIniFile * m_SettingsIniFile;
	static bool       m_UseRegistry;
	stdstr            m_SectionIdent;

public:
	CSettingTypeRomDatabase(LPCSTR Name, LPCSTR DefaultValue );
	CSettingTypeRomDatabase(LPCSTR Name, bool DefaultValue );
	CSettingTypeRomDatabase(LPCSTR Name, int DefaultValue );
	CSettingTypeRomDatabase(LPCSTR Name, SettingID DefaultSetting );
	~CSettingTypeRomDatabase();

	virtual SettingLocation GetSettingsLocation ( void ) const { return SettingLocation_RomDatabase; }	

	//return the values
	virtual bool Load ( int Index, bool & Value   ) const; 
	virtual bool Load ( int Index, ULONG & Value  ) const;
	virtual bool Load ( int Index, stdstr & Value ) const; 

	//Update the settings
	virtual void Save ( int Index, bool Value ); 
	virtual void Save ( int Index, ULONG Value ); 
	virtual void Save ( int Index, const stdstr & Value );
	virtual void Save ( int Index, const char * Value );

	static void Initilize ( void );
};

