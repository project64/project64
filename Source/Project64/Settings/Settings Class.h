#pragma once

#include "SettingType/SettingsType-Base.h"

/*#ifndef __SETTINGS_CLASS__H__
#define __SETTINGS_CLASS__H__

#pragma warning(disable:4786)
#include "..\Settings.h"
#include "..\Support.h"

#include <string>
#include <map>      //stl map

typedef unsigned long DWORD;
typedef const char *  LPCSTR;

  */

enum SettingDataType {
	Data_DWORD    = 0, 
	Data_String   = 1, 
	Data_CPUTYPE  = 2, 
	Data_SelfMod  = 3, 
	Data_OnOff    = 4, 
	Data_YesNo    = 5, 
	Data_SaveChip = 6 
};
/*

class CSettingInfo {
public:
	CSettingInfo ( void ) {
		this->Type         = No_Default;
		this->DefaultValue = No_Default;
		this->StartOfRange = No_Default;
		this->DataType     = Data_DWORD;
		this->Location     = ConstValue;
		this->Name         = "";
		this->SubNode      = "";
		this->Value        = 0;
	}
	CSettingInfo ( SettingID Type, SettingID DefaultValue, SettingDataType DataType,
		SettingLocation Location, LPCSTR Name, LPCSTR SubNode = NULL, DWORD Value = 0, SettingID StartOfRange = No_Default )
	{
		this->Type         = Type;
		this->DefaultValue = DefaultValue;
		this->DataType     = DataType;
		this->Location     = Location;
		this->Name         = Name;
		this->SubNode      = SubNode;
		this->Value        = Value;
		if (StartOfRange == No_Default) {
			this->StartOfRange = Type;
		} else {
			this->StartOfRange = StartOfRange;
		}
	}
	SettingID       Type; 
	SettingID       DefaultValue;
	SettingID       StartOfRange; 
	SettingDataType DataType;
	SettingLocation Location;
	stdstr          Name;      //if in the registry this it the Registry key name
	stdstr          SubNode;
	DWORD           Value;
};


class CN64System;
class CMainGui;

typedef std::map<SettingID, CSettingInfo> SETTING_MAP;

class CTempInfo {
public:
	SettingID       Type; 
	SettingDataType DataType;
	char *          String;
	DWORD           Value;

	CTempInfo ( CTempInfo const &b ) {
		this->Type     = b.Type;
		this->DataType = b.DataType;
		this->Value    = b.Value;
		this->String   = NULL;
		if (b.DataType == Data_String && b.String != NULL) {
			this->String   = new char [(strlen(b.String) + 1)];
			strcpy(this->String,b.String);
		}
	}

	CTempInfo ( SettingID Type, const char * String ) {
		this->Type     = Type;
		this->DataType = Data_String;
		this->String   = new char [(strlen(String) + 1)];
		strcpy(this->String,String);
		this->Value = 0;
	}
	CTempInfo ( SettingID Type, DWORD Value ) {
		this->Type     = Type;
		this->DataType = Data_DWORD;
		this->String   = 0;
		this->Value    = Value;
	}
	~CTempInfo ( void ) {
		if (DataType == Data_String && String != NULL) {
			delete [] String;
		}
	}
};

typedef std::map<SettingID, CTempInfo>    TEMP_SETTING_MAP;
class CNotification;
class CriticalSection;

class CSettings  {
public:
	typedef void (* SettingChangedFunc)(void *);

private:
	char Registrylocation[255];
	int  RegistryKey;

	typedef struct {
		SettingID Type;
		void    * Data;
		SettingChangedFunc Func;
	} SETTING_CHANGED_CB;

	typedef std::list<SETTING_CHANGED_CB> SETTING_CHANGED_CB_LIST;

	SETTING_MAP      SettingInfo;
	TEMP_SETTING_MAP TempKeys;
	CIniFile       * RomIniFile;
	CIniFile       * CheatIniFile;
	CIniFile       * SettingsIniFile;
	CriticalSection m_CS;
	SETTING_CHANGED_CB_LIST m_CBDwordList;
	
	
	void AddHowToHandleSetting (void);
	static void DisplayConfigWindow   (CSettings * _this);
	void UnknownSetting (SettingID Type);
	
public:

	CSettings(void);
	~CSettings(void);
	
	bool Initilize ( const char * AppName );

	void Config    (void * ParentWindow, CN64System * System, CMainGui * Gui);
	void ConfigRom (void * ParentWindow, CMainGui * Gui);

	//Load a DWORD value from the settings, if value is not in settings then the passed
	//DWORD is unchanged.
	void  Load(SettingID Type, DWORD & Value); 
	void  Load(SettingID Type, char * Buffer, int BufferSize);

	//return the values
	bool   LoadBool   ( SettingID Type );
	DWORD  LoadDword  ( SettingID Type ); 
	stdstr LoadString ( SettingID Type ); 

	//Update the settings
	void  SaveBool    ( SettingID Type, bool Value ); 
	void  SaveDword   ( SettingID Type, DWORD Value ); 
	void  SaveString  ( SettingID Type, const char * Buffer );

	// static functions for plugins
	static DWORD  GetSetting      ( CSettings * _this, SettingID Type );
	static LPCSTR GetSettingSz    ( CSettings * _this, SettingID Type, char * Buffer, int BufferSize );
    static void   SetSetting      ( CSettings * _this, SettingID ID, unsigned int Value );
    static void   SetSettingSz    ( CSettings * _this, SettingID ID, const char * Value );
	static void   RegisterSetting ( CSettings * _this, SettingID ID, SettingID DefaultID, SettingDataType Type, 
                                      SettingLocation Location, const char * Category, const char * DefaultStr, 
									  DWORD Value );
	// plugin call backs
	void (*UnknownSetting_RSP)    ( int ID );
	void (*UnknownSetting_GFX)    ( int ID );
	void (*UnknownSetting_AUDIO)  ( int ID );
	void (*UnknownSetting_CTRL)   ( int ID );

	//Register Notification of change
	void RegisterChangeCB(SettingID Type,void * Data, SettingChangedFunc Func);
	void UnregisterChangeCB(SettingID Type,void * Data, SettingChangedFunc Func);
};

extern CSettings * _Settings;

#endif*/

class CSettings  {
public:
	typedef void (* SettingChangedFunc)(void *);

private:
	void AddHandler ( SettingID TypeID, CSettingType * Handler );
	void AddHowToHandleSetting (void);
	void UnknownSetting (SettingID Type);

	typedef struct _SETTING_CHANGED_CB
	{
		void                * Data;
		SettingChangedFunc    Func;
		_SETTING_CHANGED_CB * Next; 
	} SETTING_CHANGED_CB;

	typedef std::map<SettingID, SETTING_CHANGED_CB *> SETTING_CALLBACK;
	typedef std::map<SettingID, CSettingType *> SETTING_MAP;
	typedef SETTING_MAP::iterator SETTING_HANDLER;

public:
	CSettings(void);
	~CSettings(void);
	
	bool Initilize ( const char * AppName );

	//return the values
	bool   LoadBool         ( SettingID Type );
	bool   LoadBool         ( SettingID Type, bool & Value );
	bool   LoadBoolIndex    ( SettingID Type, int index  );
	bool   LoadBoolIndex    ( SettingID Type, int index , bool & Value );
	DWORD  LoadDword        ( SettingID Type ); 
	bool   LoadDword        ( SettingID Type, DWORD & Value); 
	DWORD  LoadDwordIndex   ( SettingID Type, int index ); 
	bool   LoadDwordIndex   ( SettingID Type, int index, DWORD & Value); 
	stdstr LoadString       ( SettingID Type ); 
	bool   LoadString       ( SettingID Type, stdstr & Value ); 
	bool   LoadString       ( SettingID Type, char * Buffer, int BufferSize ); 
	stdstr LoadStringIndex  ( SettingID Type, int index ); 
	bool   LoadStringIndex  ( SettingID Type, int index, stdstr & Value ); 
	bool   LoadStringIndex  ( SettingID Type, int index, char * Buffer, int BufferSize ); 

	//Load the default value for the setting
	bool   LoadDefaultBool         ( SettingID Type );
	void   LoadDefaultBool         ( SettingID Type, bool & Value );
	bool   LoadDefaultBoolIndex    ( SettingID Type, int index  );
	void   LoadDefaultBoolIndex    ( SettingID Type, int index , bool & Value );
	DWORD  LoadDefaultDword        ( SettingID Type ); 
	void   LoadDefaultDword        ( SettingID Type, DWORD & Value); 
	DWORD  LoadDefaultDwordIndex   ( SettingID Type, int index ); 
	void   LoadDefaultDwordIndex   ( SettingID Type, int index, DWORD & Value); 
	stdstr LoadDefaultString       ( SettingID Type ); 
	void   LoadDefaultString       ( SettingID Type, stdstr & Value ); 
	void   LoadDefaultString       ( SettingID Type, char * Buffer, int BufferSize ); 
	stdstr LoadDefaultStringIndex  ( SettingID Type, int index ); 
	void   LoadDefaultStringIndex  ( SettingID Type, int index, stdstr & Value ); 
	void   LoadDefaultStringIndex  ( SettingID Type, int index, char * Buffer, int BufferSize ); 

	//Update the settings
	void   SaveBool         ( SettingID Type, bool Value ); 
	void   SaveBoolIndex    ( SettingID Type, int index, bool Value ); 
	void   SaveDword        ( SettingID Type, DWORD Value ); 
	void   SaveDwordIndex   ( SettingID Type, int index, DWORD Value ); 
	void   SaveString       ( SettingID Type, const stdstr & Value );
	void   SaveStringIndex  ( SettingID Type, int index, const stdstr & Value );
	void   SaveString       ( SettingID Type, const char * Buffer );
	void   SaveStringIndex  ( SettingID Type, int index, const char * Buffer );

	// Delete a setting
	void   DeleteSetting      ( SettingID Type );
	void   DeleteSettingIndex ( SettingID Type, int index );
	
	//Register Notification of change
	void RegisterChangeCB   ( SettingID Type, void * Data, SettingChangedFunc Func);
	void UnregisterChangeCB ( SettingID Type, void * Data, SettingChangedFunc Func);

	// information about setting
	SettingType   GetSettingType     ( SettingID Type   );
	bool          IndexBasedSetting  ( SettingID Type   );
	void          SettingTypeChanged ( SettingType Type );

	// static functions for plugins
	static DWORD  GetSetting      ( CSettings * _this, SettingID Type );
	static LPCSTR GetSettingSz    ( CSettings * _this, SettingID Type, char * Buffer, int BufferSize );
    static void   SetSetting      ( CSettings * _this, SettingID ID, unsigned int Value );
    static void   SetSettingSz    ( CSettings * _this, SettingID ID, const char * Value );
	static void   RegisterSetting ( CSettings * _this, SettingID ID, SettingID DefaultID, SettingDataType DataType, 
                                      SettingType Type, const char * Category, const char * DefaultStr, 
									  DWORD Value );
private:
	void NotifyCallBacks ( SettingID Type );

	SETTING_MAP      m_SettingInfo;
	SETTING_CALLBACK m_Callback;
};

extern CSettings * _Settings;
