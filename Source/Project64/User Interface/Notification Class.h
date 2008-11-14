#ifndef __NOTIFACTION_CLASS__H__
#define __NOTIFACTION_CLASS__H__

#include "../Settings/Notification Settings.h"

class CSettings;

class CNotification :
	CNotificationSettings
{
	CMainGui   * _hWnd;
	CGfxPlugin * _gfxPlugin;
	
	mutable time_t       m_NextMsg;
	
public:
         CNotification    ( void );
	
	// Make sure we are not in full screen
	void WindowMode       ( void ) const;

	//Error Messages
	void DisplayError     ( const char * Message, ... ) const;
	void DisplayError     ( const char * Message, va_list ap ) const;
	void DisplayError     ( LanguageStringID StringID ) const { stdstr str = _Lang->GetString(StringID); DisplayError(str.c_str()); }
	void FatalError       ( const char * Message, ... ) const;
	void FatalError       ( const char * Message, va_list ap ) const;
	void FatalError       ( LanguageStringID StringID ) const { stdstr str = _Lang->GetString(StringID); FatalError(str.c_str()); }
	
	//User Feedback
	void DisplayMessage   ( int DisplayTime, const char * Message, ... ) const;
	void DisplayMessage   ( int DisplayTime, const char * Message, va_list ap ) const;
	void DisplayMessage   ( int DisplayTime, LanguageStringID StringID ) const 
	{ 
		stdstr str = _Lang->GetString(StringID); 
		DisplayMessage(DisplayTime,"%s",str.c_str()); 
	}
	void DisplayMessage2  ( const char * Message, ... ) const;
	void DisplayMessage2  ( const char * Message, va_list ap ) const;
	void SetWindowCaption ( const char * Caption );
	
	//Remember roms loaded and Rom Dir selected
	void AddRecentDir     ( const char * RomDir );
	void AddRecentRom     ( const char * ImagePath );

	//Gui for responses
	void SetMainWindow      ( CMainGui * Gui );
	void RefreshMenu        ( void );
	void HideRomBrowser     ( void );
	void ShowRomBrowser     ( void );
	void MakeWindowOnTop    ( bool OnTop );
	void BringToTop         ( void );
	void BreakPoint         ( const char * File, const int LineNumber);
	bool ProcessGuiMessages ( void ) const;
	void ChangeFullScreen   ( void ) const;
	void SetGfxPlugin       ( CGfxPlugin * Plugin );
};

CNotification  & Notify ( void );

#endif
