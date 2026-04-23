#pragma once
#include <Common/File.h>

class CLogging
{
public:
    static void StartLog(void);
    static void StopLog(void);

    static void LogMessage(const char * Message, ...);

private:
    static CFile * m_hLogFile;
};
