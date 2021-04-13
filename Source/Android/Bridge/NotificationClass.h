#pragma once

#include <Project64-core/Notification.h>

class CNotificationImp :
    public CNotification
{
public:
    CNotificationImp(void);
	virtual ~CNotificationImp();

    // Error messages
    void DisplayError(const char * Message) const;
    void DisplayError(LanguageStringID StringID) const;

    void FatalError(const char * Message) const;
    void FatalError(LanguageStringID StringID) const;

    // User feedback
    void DisplayWarning(const char * Message) const;
    void DisplayWarning(LanguageStringID StringID) const;

    void DisplayMessage(int DisplayTime, const char * Message) const;
    void DisplayMessage(int DisplayTime, LanguageStringID StringID) const;

    void DisplayMessage2(const char * Message) const;

    // Ask a yes/no question to the user, yes = true, no = false
    bool AskYesNoQuestion(const char * Question) const;
    void BreakPoint(const char * FileName, int32_t LineNumber);

    void AppInitDone(void);
    bool ProcessGuiMessages(void) const;
    void ChangeFullScreen(void) const;

private:
    CNotificationImp(const CNotificationImp&);
    CNotificationImp& operator=(const CNotificationImp&);

    mutable time_t m_NextMsg;
};

CNotificationImp & Notify(void);
