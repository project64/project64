#pragma once
#include <Project64-core/Settings/LoggingSettings.h>
#include <Common/File.h>

class CLogging :
    public CLogSettings
{
public:
    static void StartLog(void);
    static void StopLog(void);

    static void LogMessage(const char * Message, ...);

private:
    static CFile * m_hLogFile;
};
