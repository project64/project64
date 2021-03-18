#include <common/StdString.h>
#include <Common/Trace.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Settings.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include "NotificationClass.h"
#include "JavaBridge.h"
#if defined(ANDROID)
#include <android/log.h>

extern JavaBridge * g_JavaBridge;
#endif

CNotificationImp & Notify(void)
{
    static CNotificationImp Notify;
    return Notify;
}

CNotificationImp::CNotificationImp() :
    m_NextMsg(0)
{
}

CNotificationImp::~CNotificationImp()
{
}

void CNotificationImp::DisplayError(const char * Message) const
{
#ifdef ANDROID
    g_JavaBridge->DisplayError(Message);
#else
    Message = NULL; // Not used
#endif
}

void CNotificationImp::DisplayError(LanguageStringID StringID) const
{
    if (g_Lang)
    {
        DisplayError(g_Lang->GetString(StringID).c_str());
    }
}

void CNotificationImp::FatalError(LanguageStringID StringID) const
{
    if (g_Lang)
    {
        FatalError(g_Lang->GetString(StringID).c_str());
    }
}

void CNotificationImp::FatalError(const char * Message) const
{
    WriteTrace(TraceUserInterface, TraceError, Message);
    DisplayError(Message);
    if (g_BaseSystem)
    {
        g_BaseSystem->CloseCpu();
    }
}

void CNotificationImp::DisplayWarning(const char * Message) const
{
#ifdef ANDROID
    g_JavaBridge->DisplayError(Message);
#else
    Message = NULL; // Not used
#endif
}

void CNotificationImp::DisplayWarning(LanguageStringID StringID) const
{
    if (g_Lang)
    {
        DisplayError(g_Lang->GetString(StringID).c_str());
    }
}

void CNotificationImp::DisplayMessage(int DisplayTime, LanguageStringID StringID) const
{
    DisplayMessage(DisplayTime, g_Lang->GetString(StringID).c_str());
}

// User feedback
void CNotificationImp::DisplayMessage(int DisplayTime, const char * Message) const
{
#ifdef ANDROID
    if (g_JavaBridge == NULL) { return; }
    g_JavaBridge->DisplayMessage(Message, DisplayTime);
#else
    // Ignore warning usage
    DisplayTime = DisplayTime;
    Message = Message;
#endif
}

void CNotificationImp::DisplayMessage2(const char * Message) const
{
#ifdef ANDROID
    if (g_JavaBridge == NULL) { return; }

    g_JavaBridge->DisplayMessage2(Message);
#else
    // Ignore warning usage
    Message = Message;
#endif
}

// Ask a yes/no question to the user, yes = true, no = false
bool CNotificationImp::AskYesNoQuestion(const char * /*Question*/) const
{
    return false;
}

void CNotificationImp::BreakPoint(const char * FileName, int32_t LineNumber)
{
    Flush_Recompiler_Log();
    TraceFlushLog();
    if (g_Settings->LoadBool(Debugger_Enabled))
    {
        FatalError(stdstr_f("Break point found at\n%s\nLine: %d", FileName, LineNumber).c_str());
    }
    else
    {
        FatalError("Fatal Error: Emulation stopped");
    }
}

void CNotificationImp::AppInitDone(void)
{
}

bool CNotificationImp::ProcessGuiMessages(void) const
{
    return false;
}

void CNotificationImp::ChangeFullScreen(void) const
{
}
