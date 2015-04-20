/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include "SettingType/SettingsType-Base.h"

enum SettingDataType {
	Data_DWORD    = 0, 
	Data_String   = 1, 
	Data_CPUTYPE  = 2, 
	Data_SelfMod  = 3, 
	Data_OnOff    = 4, 
	Data_YesNo    = 5, 
	Data_SaveChip = 6 
};

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
	
	bool Initialize ( const char * AppName );

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
	static DWORD  FindSetting ( CSettings * _this, char * Name );
	static void   FlushSettings ( CSettings * _this );

private:
	void NotifyCallBacks ( SettingID Type );

	SETTING_MAP      m_SettingInfo;
	SETTING_CALLBACK m_Callback;
	int              m_NextAutoSettingId;
};

extern CSettings * g_Settings;
