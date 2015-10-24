#include <windows.h>
#include <stdio.h>
#include "Settings.h"

enum SettingLocation {
	SettingType_ConstString       = 0, 
	SettingType_ConstValue        = 1, 
	SettingType_CfgFile           = 2, 
	SettingType_Registry          = 3, 
	SettingType_RelativePath      = 4, 
	TemporarySetting              = 5, 
	SettingType_RomDatabase       = 6, 
	SettingType_CheatSetting      = 7, 
	SettingType_GameSetting       = 8,
	SettingType_BoolVariable      = 9, 
	SettingType_NumberVariable    = 10, 
	SettingType_StringVariable    = 11, 
	SettingType_SelectedDirectory = 12, 
	SettingType_RdbSetting        = 13, 
};

enum SettingDataType {
	Data_DWORD = 0, Data_String = 1, Data_CPUTYPE = 2, Data_SelfMod = 3, Data_OnOff = 4, Data_YesNo = 5, Data_SaveChip = 6 
};

typedef struct {
	DWORD  dwSize;
	int    DefaultStartRange;
	int    SettingStartRange;
	int    MaximumSettings;
	int    NoDefault;
	int    DefaultLocation;
	void * handle;
	unsigned int (*GetSetting)      ( void * handle, int ID );
	const char * (*GetSettingSz)    ( void * handle, int ID, char * Buffer, int BufferLen );
    void         (*SetSetting)      ( void * handle, int ID, unsigned int Value );
    void         (*SetSettingSz)    ( void * handle, int ID, const char * Value );
	void         (*RegisterSetting) ( void * handle, int ID, int DefaultID, SettingDataType Type, 
                                      SettingLocation Location, const char * Category, const char * DefaultStr, DWORD Value );
	void         (*UseUnregisteredSetting) (int ID);
} PLUGIN_SETTINGS;

typedef struct {
	unsigned int (*FindSystemSettingId) ( void * handle, const char * Name );
} PLUGIN_SETTINGS2;

typedef struct {
	void (*FlushSettings) ( void * handle );
} PLUGIN_SETTINGS3;

static PLUGIN_SETTINGS  g_PluginSettings;
static PLUGIN_SETTINGS2 g_PluginSettings2;
static PLUGIN_SETTINGS3 g_PluginSettings3;
static bool             g_PluginInitilized = false;
static char             g_PluginSettingName[300];

extern "C" {
__declspec(dllexport) void SetSettingInfo (PLUGIN_SETTINGS * info);
__declspec(dllexport) void SetSettingInfo2 (PLUGIN_SETTINGS2 * info);
__declspec(dllexport) void SetSettingInfo3 (PLUGIN_SETTINGS3 * info);
}

__declspec(dllexport) void SetSettingInfo (PLUGIN_SETTINGS * info) 
{
	g_PluginSettings   = *info;
	g_PluginInitilized = true;
	info->UseUnregisteredSetting = UseUnregisteredSetting;
}

__declspec(dllexport) void SetSettingInfo2 (PLUGIN_SETTINGS2 * info) 
{
	g_PluginSettings2 = *info;
}

__declspec(dllexport) void SetSettingInfo3 (PLUGIN_SETTINGS3 * info) 
{
	g_PluginSettings3 = *info;
}

BOOL SettingsInitilized ( void )
{
	return g_PluginInitilized;
}

void SetModuleName      ( const char * Name )
{
	_snprintf(g_PluginSettingName,sizeof(g_PluginSettingName),"%s",Name);
}

void RegisterSetting    ( short SettingID, SETTING_DATA_TYPE Type, const char * Name, const char * Category,
					        unsigned int DefaultDW, const char * DefaultStr )
{
	int DefaultID  = g_PluginSettings.NoDefault;
	SettingLocation Location = (SettingLocation)g_PluginSettings.DefaultLocation;
	char FullCategory[400];
	if (Category && Category[0] != 0)
	{
		_snprintf(FullCategory,sizeof(FullCategory),"%s\\%s",g_PluginSettingName,Category);
	}
	else
	{
		_snprintf(FullCategory,sizeof(FullCategory),"%s",g_PluginSettingName);
	}

	switch (Type)
	{
	case Data_DWORD_Game:
	case Data_String_Game:
		Location = SettingType_GameSetting;
		break;
	case Data_DWORD_RDB:
	case Data_String_RDB:
		Location = SettingType_RomDatabase;
		break;
	case Data_DWORD_RDB_Setting:
	case Data_String_RDB_Setting:
		Location = SettingType_RdbSetting;
		break;
	}
	
	switch (Type)
	{
	case Data_DWORD_Game:
		g_PluginSettings.RegisterSetting(g_PluginSettings.handle,SettingID + g_PluginSettings.SettingStartRange,
			g_PluginSettings.NoDefault,Data_DWORD,Location,FullCategory,Name,DefaultDW);
		break;
	case Data_DWORD_General:
	case Data_DWORD_RDB:
	case Data_DWORD_RDB_Setting:
		if (DefaultDW != 0)
		{
			//create default
			DefaultID = SettingID + g_PluginSettings.DefaultStartRange;
			g_PluginSettings.RegisterSetting(g_PluginSettings.handle,DefaultID,g_PluginSettings.NoDefault,
				Data_DWORD,SettingType_ConstValue,g_PluginSettingName,"",DefaultDW);
		}

		g_PluginSettings.RegisterSetting(g_PluginSettings.handle,SettingID + g_PluginSettings.SettingStartRange,
			DefaultID,Data_DWORD,Location,FullCategory,Name,0);
		break;
	case Data_String_General:
	case Data_String_Game:
	case Data_String_RDB:
	case Data_String_RDB_Setting:
		if (DefaultStr != NULL && strlen(DefaultStr) > 0)
		{
			//create default
			DefaultID = SettingID + g_PluginSettings.DefaultStartRange;
			g_PluginSettings.RegisterSetting(g_PluginSettings.handle,DefaultID,g_PluginSettings.NoDefault,
				Data_String,SettingType_ConstString,g_PluginSettingName,DefaultStr,0);
		}

		g_PluginSettings.RegisterSetting(g_PluginSettings.handle,SettingID + g_PluginSettings.SettingStartRange,
			DefaultID,Data_String,Location,FullCategory,Name,0);
		break;
	}
}

short FindSystemSettingId ( const char * Name )
{
	if (g_PluginSettings2.FindSystemSettingId && g_PluginSettings.handle)
	{
		return (short)g_PluginSettings2.FindSystemSettingId(g_PluginSettings.handle,Name);
	}
	return 0;
}

void FlushSettings ( void )
{
	if (g_PluginSettings3.FlushSettings && g_PluginSettings.handle)
	{
		g_PluginSettings3.FlushSettings(g_PluginSettings.handle);
	}
}

unsigned int GetSetting ( short SettingID )
{
	if (g_PluginSettings.GetSetting == NULL)
	{
		return 0;
	}
	return g_PluginSettings.GetSetting(g_PluginSettings.handle,SettingID + g_PluginSettings.SettingStartRange);
}

unsigned int GetSystemSetting  ( short SettingID )
{
	return g_PluginSettings.GetSetting(g_PluginSettings.handle,SettingID);
}

const char * GetSettingSz ( short SettingID, char * Buffer, int BufferLen )
{
	return g_PluginSettings.GetSettingSz(g_PluginSettings.handle,SettingID + g_PluginSettings.SettingStartRange,Buffer,BufferLen);
}

const char * GetSystemSettingSz ( short SettingID, char * Buffer, int BufferLen )
{
	if (g_PluginSettings.GetSettingSz == NULL)
	{
		return "";
	}
	return g_PluginSettings.GetSettingSz(g_PluginSettings.handle,SettingID,Buffer,BufferLen);
}

void SetSetting ( short SettingID, unsigned int Value )
{
	g_PluginSettings.SetSetting(g_PluginSettings.handle,SettingID + g_PluginSettings.SettingStartRange, Value);
}

void SetSettingSz ( short SettingID, const char * Value )
{
	g_PluginSettings.SetSettingSz(g_PluginSettings.handle,SettingID + g_PluginSettings.SettingStartRange, Value);
}
