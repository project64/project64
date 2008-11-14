#pragma once

class CSettingTypeRDBCpuType :
	public CSettingTypeRomDatabase
{
	
public:
	CSettingTypeRDBCpuType(LPCSTR Name, SettingID DefaultSetting );
	CSettingTypeRDBCpuType(LPCSTR Name, int DefaultValue );
	~CSettingTypeRDBCpuType();

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

