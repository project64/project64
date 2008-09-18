#pragma once

enum SettingLocation {
	SettingLocation_ConstString  = 0, 
	SettingLocation_ConstValue   = 1, 
	SettingLocation_CfgFile      = 2, 
	SettingLocation_Registry     = 3, 
	SettingLocation_RelativePath = 4, 
	TemporarySetting = 5, 
	SettingLocation_RomDatabase  = 6, 
	CheatSetting     = 7, 
	SettingLocation_GameSetting    = 8,
	SettingLocation_BoolVariable   = 9, 
	SettingLocation_NumberVariable = 10, 
	SettingLocation_StringVariable = 11, 
};

class CSettingType
{
public:
	virtual SettingLocation GetSettingsLocation ( void ) const = 0;	
	virtual bool            IndexBasedSetting   ( void ) const = 0;

	//return the values
	virtual bool Load ( int Index, bool & Value   ) const = 0; 
	virtual bool Load ( int Index, ULONG & Value  ) const = 0; 
	virtual bool Load ( int Index, stdstr & Value ) const = 0; 

	//Update the settings
	virtual void Save ( int Index, bool Value ) = 0; 
	virtual void Save ( int Index, ULONG Value ) = 0; 
	virtual void Save ( int Index, const stdstr & Value ) = 0;
	virtual void Save ( int Index, const char * Value ) = 0;

};

