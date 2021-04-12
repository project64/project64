#pragma once
#include <Project64-core/Settings/LoggingSettings.h>
#include <Common/FileClass.h>

class CLogging :
    public CLogSettings
{
public:
    static void StartLog(void);
    static void StopLog(void);

    static void Log_LW(uint32_t PC, uint32_t VAddr);
    static void Log_SW(uint32_t PC, uint32_t VAddr, uint32_t Value);
    static void LogMessage(const char * Message, ...);

private:
    static CFile * m_hLogFile;
};
