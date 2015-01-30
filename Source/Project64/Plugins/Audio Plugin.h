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
	CAudioPlugin(const char * FileName);
	~CAudioPlugin();

	virtual int GetDefaultSettingStartRange() const { return FirstAudioDefaultSet; }
	virtual int GetSettingStartRange() const { return FirstAudioSettings; }

	void UnloadPlugin();

	void DacrateChanged(SYSTEM_TYPE Type);
	bool Initiate(CN64System * System, CMainGui * RenderWindow);

	void(__cdecl *AiLenChanged)(void);
	DWORD(__cdecl *AiReadLength)(void);
	void(__cdecl *ProcessAList)(void);

protected:
	void * m_hAudioThread;

	bool Init(const char * FileName);

	struct AUDIO_INFO {
		HWND hwnd;
		HINSTANCE hinst;

		BOOL MemoryBswaped;    // If this is set to TRUE, then the memory has been pre
		//   bswap on a dword (32 bits) boundry 
		//	eg. the first 8 bytes are stored like this:
		//        4 3 2 1   8 7 6 5
		BYTE * HEADER;	// This is the rom header (first 40h bytes of the rom
		// This will be in the same memory format as the rest of the memory.
		BYTE * RDRAM;
		BYTE * DMEM;
		BYTE * IMEM;

		DWORD * MI__INTR_REG;

		DWORD * AI__DRAM_ADDR_REG;
		DWORD * AI__LEN_REG;
		DWORD * AI__CONTROL_REG;
		DWORD * AI__STATUS_REG;
		DWORD * AI__DACRATE_REG;
		DWORD * AI__BITRATE_REG;

		void(__cdecl *CheckInterrupts)(void);
	};

	BOOL(__cdecl *InitiateAudio)   (AUDIO_INFO Audio_Info);
	void(__cdecl *AiUpdate)		(BOOL Wait);
	void(__cdecl *AiDacrateChanged)(SYSTEM_TYPE Type);

	// Function used in a thread for using audio
	static void AudioThread(CAudioPlugin * _this);
};