#pragma once

class CSettingTypeRelativePath :
	public CSettingType
{
	CPath m_FileName;

public:
	CSettingTypeRelativePath(LPCSTR Path, LPCSTR FileName);
	~CSettingTypeRelativePath();

	bool        IndexBasedSetting ( void ) const { return false; }
	SettingType GetSettingType    ( void ) const { return SettingType_RelativePath; }	

	//return the values
	bool Load   ( int /*Index*/, bool & /*Value*/ ) const { return false; }; 
	bool Load   ( int /*Index*/, ULONG & /*Value*/  ) const { return false; };
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

