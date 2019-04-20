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

#include "../Settings/NotificationSettings.h"
#include <Project64-core/Notification.h>

class CSettings;

class CNotificationImp :
    public CNotification,
    CNotificationSettings
{
public:
    CNotificationImp(void);

    void AppInitDone(void);

    // Make sure we are not in full screen
    void WindowMode(void) const;

    //Error Messages
    virtual void DisplayError(const char * Message) const;
    virtual void DisplayError(LanguageStringID StringID) const;

    virtual void FatalError(const char * Message) const;
    virtual void FatalError(LanguageStringID StringID) const;

    //User Feedback
    virtual void DisplayWarning(const char * Message) const;
    virtual void DisplayWarning(LanguageStringID StringID) const;

    virtual void DisplayMessage(int DisplayTime, const char * Message) const;
    virtual void DisplayMessage(int DisplayTime, LanguageStringID StringID) const;

    virtual void DisplayMessage2(const char * Message) const;

    // Ask a Yes/No Question to the user, yes = true, no = false
    virtual bool AskYesNoQuestion(const char * Question) const;

    virtual void BreakPoint(const char * FileName, int32_t LineNumber);

    void SetWindowCaption(const wchar_t * Caption);

    //Remember roms loaded and Rom Dir selected
    void AddRecentDir(const char * RomDir);

    //Gui for responses
    void SetMainWindow(CMainGui * Gui);
    void RefreshMenu(void);
    void HideRomBrowser(void);
    void ShowRomBrowser(void);
    void BringToTop(void);
    bool ProcessGuiMessages(void) const;
    void ChangeFullScreen(void) const;
    void SetGfxPlugin(CGfxPlugin * Plugin);

private:
    CNotificationImp(const CNotificationImp&);				// Disable copy constructor
    CNotificationImp& operator=(const CNotificationImp&);		// Disable assignment

    CMainGui   * m_hWnd;
    CGfxPlugin * m_gfxPlugin;

    mutable time_t m_NextMsg;
};

CNotificationImp & Notify(void);
