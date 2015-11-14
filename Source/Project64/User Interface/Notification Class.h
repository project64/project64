/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include "../Settings/Notification Settings.h"

class CSettings;

class CNotification :
	CNotificationSettings
{
public:
         CNotification    ( void );
	
	void AppInitDone ( void );

	// Make sure we are not in full screen
	void WindowMode       ( void ) const;

	//Error Messages
	virtual void DisplayError(const wchar_t * Message) const;
	virtual void DisplayError(LanguageStringID StringID) const;

	virtual void FatalError(const wchar_t * Message) const;
	virtual void FatalError(LanguageStringID StringID) const;
		
	//User Feedback
	virtual void DisplayMessage(int DisplayTime, const wchar_t * Message) const;
	virtual void DisplayMessage(int DisplayTime, LanguageStringID StringID) const;
	virtual void DisplayMessageWide(int DisplayTime, const wchar_t * Message, ...) const;
	virtual void DisplayMessageWide(int DisplayTime, const wchar_t * Message, va_list ap) const;

	virtual void DisplayMessage2(const wchar_t * Message) const;
	virtual void BreakPoint(const wchar_t * FileName, const int LineNumber);

	void SetWindowCaption ( const wchar_t * Caption );
	
	//Remember roms loaded and Rom Dir selected
	void AddRecentDir     ( const char * RomDir );

	//Gui for responses
	void SetMainWindow      ( CMainGui * Gui );
	void RefreshMenu        ( void );
	void HideRomBrowser     ( void );
	void ShowRomBrowser     ( void );
	void MakeWindowOnTop    ( bool OnTop );
	void BringToTop         ( void );
	bool ProcessGuiMessages ( void ) const;
	void ChangeFullScreen   ( void ) const;
	void SetGfxPlugin       ( CGfxPlugin * Plugin );

private:
	CNotification(const CNotification&);				// Disable copy constructor
	CNotification& operator=(const CNotification&);		// Disable assignment

	CMainGui   * m_hWnd;
	CGfxPlugin * m_gfxPlugin;

	mutable time_t m_NextMsg;
};

CNotification  & Notify ( void );
