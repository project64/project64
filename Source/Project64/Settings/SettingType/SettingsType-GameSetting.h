#pragma once

class CSettingTypeGame :
	public CSettingTypeApplication
{

public:
	CSettingTypeGame(LPCSTR Section, LPCSTR Name, LPCSTR DefaultValue );
	CSettingTypeGame(LPCSTR Section, LPCSTR Name, DWORD DefaultValue );
	CSettingTypeGame(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting );
	~CSettingTypeGame();

	SettingLocation GetSettingsLocation ( void ) const { return SettingLocation_GameSetting; }	

	//return the values
	bool Load ( bool & Value   ) const; 
	bool Load ( ULONG & Value  ) const;
	bool Load ( stdstr & Value ) const; 

	//Update the settings
	void Save ( bool Value ); 
	void Save ( ULONG Value ); 
	void Save ( const stdstr & Value );
	void Save ( const char * Value );
};

