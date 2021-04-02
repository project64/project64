#pragma once

#include <Project64-core/Multilanguage.h>

#ifndef _MSC_VER
#define __interface struct
#endif

__interface CNotification
{
public:
    // Error messages
    virtual void DisplayError(const char * Message) const = 0;
    virtual void DisplayError(LanguageStringID StringID) const = 0;

    virtual void FatalError(const char * Message) const = 0;
    virtual void FatalError(LanguageStringID StringID) const = 0;

    // User feedback
    virtual void DisplayWarning(const char * Message) const = 0;
    virtual void DisplayWarning(LanguageStringID StringID) const = 0;

    virtual void DisplayMessage(int DisplayTime, const char * Message) const = 0;
    virtual void DisplayMessage(int DisplayTime, LanguageStringID StringID) const = 0;
    virtual void DisplayMessage2(const char * Message) const = 0;

    // Ask a yes/no question to the user, yes = true, no = false
    virtual bool AskYesNoQuestion(const char * Question) const = 0;

    virtual void BreakPoint(const char * FileName, int32_t LineNumber) = 0;

    virtual void AppInitDone(void) = 0;
    virtual bool ProcessGuiMessages(void) const = 0;
    virtual void ChangeFullScreen(void) const = 0;
};

extern CNotification * g_Notify;
