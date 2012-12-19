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

class CAudioPlugin 
{
public:
	CAudioPlugin  ( const char * FileName);
	~CAudioPlugin ( void );

	void DacrateChanged ( SystemType Type );
	bool Initiate       ( CN64System * System, CMainGui * RenderWindow );
	void Close          ( void );
	void GameReset      ( void );
	void RomOpened      ( void );
	stdstr PluginName ( void ) const { return m_PluginInfo.Name; }

	inline bool  Initilized    ( void ) const { return m_Initilized; }

	void  (__cdecl *LenChanged)     ( void );
	void  (__cdecl *Config)         ( DWORD hParent );
	DWORD (__cdecl *ReadLength)     ( void );
	void  (__cdecl *ProcessAList)   ( void );

private:
	void * m_hDll;	
	bool m_Initilized, m_RomOpen;
	void * m_hAudioThread;
	PLUGIN_INFO m_PluginInfo;
	
	void Init ( const char * FileName );
	void UnloadPlugin         ( void );

	void (__cdecl *CloseDLL)  ( void );
	void (__cdecl *RomOpen)   ( void );
	void (__cdecl *RomClosed) ( void );
	void (__cdecl *Update)    ( BOOL Wait );
	void (__cdecl *m_DacrateChanged) ( SystemType Type );
	void (__cdecl *PluginOpened)     ( void );
	void (__cdecl *SetSettingInfo)   ( PLUGIN_SETTINGS * info );
	void (__cdecl *SetSettingInfo2)  ( PLUGIN_SETTINGS2 * info );

	//Function used in a thread for using audio
	static void AudioThread   (CAudioPlugin * _this);

};
