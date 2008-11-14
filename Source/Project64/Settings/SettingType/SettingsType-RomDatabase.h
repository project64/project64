#pragma once

class CSettingTypeRomDatabase :
	public CSettingType
{
protected:
	mutable stdstr  m_KeyName;
	const LPCSTR    m_DefaultStr;
	const int       m_DefaultValue;
	const SettingID m_DefaultSetting;

	static stdstr     m_SectionIdent;
	static CIniFile * m_SettingsIniFile;

	static void GameChanged ( void * /*Data */ );

public:
	CSettingTypeRomDatabase(LPCSTR Name, LPCSTR DefaultValue );
	CSettingTypeRomDatabase(LPCSTR Name, bool DefaultValue );
	CSettingTypeRomDatabase(LPCSTR Name, int DefaultValue );
	CSettingTypeRomDatabase(LPCSTR Name, SettingID DefaultSetting );
	
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
};

