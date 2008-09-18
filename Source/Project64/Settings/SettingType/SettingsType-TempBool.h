#pragma once

class CSettingTypeTempBool :
	public CSettingType
{
	bool m_value;

public:
	CSettingTypeTempBool(bool initialValue );
	~CSettingTypeTempBool();

	bool            IndexBasedSetting   ( void ) const { return false; }
	SettingLocation GetSettingsLocation ( void ) const { return SettingLocation_BoolVariable; }	

	//return the values
	bool Load ( int Index, bool & Value   ) const; 
	bool Load ( int Index, ULONG & Value  ) const;
	bool Load ( int Index, stdstr & Value ) const; 

	//Update the settings
	void Save ( int Index, bool Value ); 
	void Save ( int Index, ULONG Value ); 
	void Save ( int Index, const stdstr & Value );
	void Save ( int Index, const char * Value );
};

