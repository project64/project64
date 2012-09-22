#pragma once

class CSettingTypeGame :
	public CSettingTypeApplication
{
protected:
	static bool     m_RdbEditor;
	static bool     m_EraseDefaults;
	static stdstr * m_SectionIdent;

	static void   UpdateSettings ( void * /*Data */ );

	virtual LPCSTR SectionName ( void ) const;

public:
	CSettingTypeGame(LPCSTR Name, LPCSTR DefaultValue );
	CSettingTypeGame(LPCSTR Name, DWORD DefaultValue );
	CSettingTypeGame(LPCSTR Name, SettingID DefaultSetting );
	virtual ~CSettingTypeGame();

	virtual bool        IndexBasedSetting ( void ) const { return false; }
	virtual SettingType GetSettingType    ( void ) const { return SettingType_GameSetting; }	

	static void Initilize ( void );
	static void CleanUp   ( void );

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
};

