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

    // Error messages
    virtual void DisplayError(const char * Message) const;
    virtual void DisplayError(LanguageStringID StringID) const;

    virtual void FatalError(const char * Message) const;
    virtual void FatalError(LanguageStringID StringID) const;

    // User feedback
    virtual void DisplayWarning(const char * Message) const;
    virtual void DisplayWarning(LanguageStringID StringID) const;

    virtual void DisplayMessage(int DisplayTime, const char * Message) const;
    virtual void DisplayMessage(int DisplayTime, LanguageStringID StringID) const;

    virtual void DisplayMessage2(const char * Message) const;

    // Ask a yes/no question to the user, yes = true, no = false
    virtual bool AskYesNoQuestion(const char * Question) const;

    virtual void BreakPoint(const char * FileName, int32_t LineNumber);

    void SetWindowCaption(const wchar_t * Caption);

    // Remember ROMS loaded and ROM directory selected
    void AddRecentDir(const char * RomDir);

    // GUI for responses
    void SetMainWindow(CMainGui * Gui);
    void RefreshMenu(void);
    void HideRomBrowser(void);
    void ShowRomBrowser(void);
    void BringToTop(void);
    bool ProcessGuiMessages(void) const;
    void ChangeFullScreen(void) const;
    void SetGfxPlugin(CGfxPlugin * Plugin);

private:
    CNotificationImp(const CNotificationImp&);
    CNotificationImp& operator=(const CNotificationImp&);

    CMainGui   * m_hWnd;
    CGfxPlugin * m_gfxPlugin;

    mutable time_t m_NextMsg;
};

CNotificationImp & Notify(void);
