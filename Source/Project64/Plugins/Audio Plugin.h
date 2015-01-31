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

class CAudioPlugin : public CPlugin
{
public:
	CAudioPlugin(void);
	~CAudioPlugin();

	void DacrateChanged(SYSTEM_TYPE Type);
	bool Initiate(CN64System * System, CMainGui * RenderWindow);

	void(__cdecl *AiLenChanged)(void);
	DWORD(__cdecl *AiReadLength)(void);
	void(__cdecl *ProcessAList)(void);

private:
	CAudioPlugin(const CAudioPlugin&);				// Disable copy constructor
	CAudioPlugin& operator=(const CAudioPlugin&);	// Disable assignment

	virtual int GetDefaultSettingStartRange() const { return FirstAudioDefaultSet; }
	virtual int GetSettingStartRange() const { return FirstAudioSettings; }
	PLUGIN_TYPE type() { return PLUGIN_TYPE_AUDIO; }

	void * m_hAudioThread;
	
	bool LoadFunctions ( void );
	void UnloadPluginDetails ( void );

	void(__cdecl *AiUpdate)		(BOOL Wait);
	void(__cdecl *AiDacrateChanged)(SYSTEM_TYPE Type);

	// Function used in a thread for using audio
	static void AudioThread(CAudioPlugin * _this);
};
