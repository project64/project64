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

#include <Project64-core/Notification.h>

class CNotificationImp :
    public CNotification
{
public:
    CNotificationImp(void);
	virtual ~CNotificationImp();

    //Error Messages
    void DisplayError(const char * Message) const;
    void DisplayError(LanguageStringID StringID) const;

    void FatalError(const char * Message) const;
    void FatalError(LanguageStringID StringID) const;

    //User Feedback
    void DisplayWarning(const char * Message) const;
    void DisplayWarning(LanguageStringID StringID) const;

    void DisplayMessage(int DisplayTime, const char * Message) const;
    void DisplayMessage(int DisplayTime, LanguageStringID StringID) const;

    void DisplayMessage2(const char * Message) const;

    // Ask a Yes/No Question to the user, yes = true, no = false
    bool AskYesNoQuestion(const char * Question) const;
    void BreakPoint(const char * FileName, int32_t LineNumber);

    void AppInitDone(void);
    bool ProcessGuiMessages(void) const;
    void ChangeFullScreen(void) const;

private:
    CNotificationImp(const CNotificationImp&);				// Disable copy constructor
    CNotificationImp& operator=(const CNotificationImp&);	// Disable assignment

    mutable time_t m_NextMsg;
};

CNotificationImp & Notify(void);