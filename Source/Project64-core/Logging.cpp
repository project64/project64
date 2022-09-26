#include "stdafx.h"
#include <Project64-core/Logging.h>

#include <Common/path.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <stdarg.h>
#include <stdio.h>

CFile * CLogging::m_hLogFile = nullptr;

void CLogging::LogMessage(const char * Message, ...)
{
    char Msg[400];
    va_list ap;

    if (!g_Settings->LoadBool(Debugger_Enabled))
    {
        return;
    }
    if (m_hLogFile == nullptr)
    {
        return;
    }

    va_start(ap, Message);
    vsprintf(Msg, Message, ap);
    va_end(ap);

    strcat(Msg, "\r\n");

    m_hLogFile->Write(Msg, (uint32_t)strlen(Msg));
}

void CLogging::StartLog(void)
{
    if (!GenerateLog())
    {
        StopLog();
        return;
    }
    if (m_hLogFile != nullptr)
    {
        return;
    }

    CPath LogFile(g_Settings->LoadStringVal(Directory_Log).c_str(), "cpudebug.log");
    m_hLogFile = new CFile(LogFile, CFileBase::modeCreate | CFileBase::modeWrite);
}

void CLogging::StopLog(void)
{
    if (m_hLogFile)
    {
        delete m_hLogFile;
        m_hLogFile = nullptr;
    }
}
