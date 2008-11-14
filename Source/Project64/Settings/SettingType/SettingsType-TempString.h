#pragma once

class CSettingTypeTempString :
	public CSettingType
{

	stdstr m_value;
	
public:
	CSettingTypeTempString(LPCSTR initialValue);
	~CSettingTypeTempString();

	bool        IndexBasedSetting ( void ) const { return false; }
	SettingType GetSettingType    ( void ) const { return SettingType_StringVariable; }
	
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

