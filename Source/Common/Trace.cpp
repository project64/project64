#include "Trace.h"
#include "CriticalSection.h"
#include "Platform.h"
#include "StdString.h"
#include "Thread.h"
#include <map>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

typedef std::map<uint32_t, stdstr> ModuleNameMap;

uint32_t * g_ModuleLogLevel = nullptr;
static bool g_TraceClosed = false;
static ModuleNameMap g_ModuleNames;

class CTraceLog
{
    std::vector<CTraceModule *> m_Modules;
    CriticalSection m_CS;

public:
    CTraceLog()
    {
    }
    ~CTraceLog()
    {
        CloseTrace();
    }

    void TraceMessage(uint32_t module, uint8_t severity, const char * file, int line, const char * function, const char * Message);

    CTraceModule * AddTraceModule(CTraceModule * TraceModule);
    CTraceModule * RemoveTraceModule(CTraceModule * TraceModule);
    void CloseTrace(void);
    void FlushTrace(void);
};

CTraceLog & GetTraceObjet(void)
{
    static CTraceLog TraceLog;
    return TraceLog;
}

void TraceSetModuleName(uint8_t module, const char * Name)
{
    g_ModuleNames.insert(ModuleNameMap::value_type(module, Name));
}

void WriteTraceFull(uint32_t module, uint8_t severity, const char * file, int line, const char * function, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    size_t nlen = _vscprintf(format, args) + 1;
    char * Message = (char *)alloca(nlen * sizeof(char));
    Message[nlen - 1] = 0;
    if (Message != nullptr)
    {
        vsprintf(Message, format, args);
        GetTraceObjet().TraceMessage(module, severity, file, line, function, Message);
    }
    va_end(args);
}

void TraceFlushLog(void)
{
    GetTraceObjet().FlushTrace();
}

void CloseTrace(void)
{
    g_TraceClosed = true;
    GetTraceObjet().CloseTrace();

    if (g_ModuleLogLevel)
    {
        delete g_ModuleLogLevel;
        g_ModuleLogLevel = nullptr;
    }
}

void TraceSetMaxModule(uint32_t MaxModule, uint8_t DefaultSeverity)
{
    if (g_ModuleLogLevel)
    {
        delete g_ModuleLogLevel;
        g_ModuleLogLevel = nullptr;
    }
    g_ModuleLogLevel = new uint32_t[MaxModule];
    for (uint32_t i = 0; i < MaxModule; i++)
    {
        g_ModuleLogLevel[i] = DefaultSeverity;
    }
}

CTraceModule * CTraceLog::AddTraceModule(CTraceModule * TraceModule)
{
    CGuard Guard(m_CS);

    for (int i = 0; i < (int)m_Modules.size(); i++)
    {
        if (m_Modules[i] == TraceModule)
        {
            return TraceModule;
        }
    }
    m_Modules.push_back(TraceModule);
    return TraceModule;
}

CTraceModule * CTraceLog::RemoveTraceModule(CTraceModule * TraceModule)
{
    CGuard Guard(m_CS);

    for (std::vector<CTraceModule *>::iterator itr = m_Modules.begin(); itr != m_Modules.end(); itr++)
    {
        if ((*itr) == TraceModule)
        {
            m_Modules.erase(itr);
            return TraceModule;
        }
    }
    return nullptr;
}

void CTraceLog::CloseTrace(void)
{
    CGuard Guard(m_CS);
    m_Modules.clear();

    if (g_ModuleLogLevel)
    {
        delete g_ModuleLogLevel;
        g_ModuleLogLevel = nullptr;
    }
}

void CTraceLog::FlushTrace(void)
{
    CGuard Guard(m_CS);
    for (size_t i = 0, n = m_Modules.size(); i < n; i++)
    {
        m_Modules[i]->FlushTrace();
    }
}

void CTraceLog::TraceMessage(uint32_t module, uint8_t severity, const char * file, int line, const char * function, const char * Message)
{
    CGuard Guard(m_CS);

    for (size_t i = 0, n = m_Modules.size(); i < n; i++)
    {
        m_Modules[i]->Write(module, severity, file, line, function, Message);
    }
}

CTraceModule * TraceAddModule(CTraceModule * TraceModule)
{
    if (g_TraceClosed)
    {
        return nullptr;
    }
    GetTraceObjet().AddTraceModule(TraceModule);
    return TraceModule;
}

CTraceModule * TraceRemoveModule(CTraceModule * TraceModule)
{
    return GetTraceObjet().RemoveTraceModule(TraceModule);
}

const char * TraceSeverity(uint8_t severity)
{
    switch (severity)
    {
    case TraceError: return "Error";
    case TraceWarning: return "Warning";
    case TraceNotice: return "Notice";
    case TraceInfo: return "Info";
    case TraceDebug: return "Debug";
    case TraceVerbose: return "Verbose";
    }

    static stdstr Unknown;
    Unknown.Format("Unknown (%d)", (int32_t)severity);
    return Unknown.c_str();
}

const char * TraceModule(uint32_t module)
{
    ModuleNameMap::const_iterator itr = g_ModuleNames.find(module);
    if (itr != g_ModuleNames.end())
    {
        return itr->second.c_str();
    }
    static stdstr Unknown;
    Unknown.Format("Unknown (%d)", module);
    return Unknown.c_str();
}

CTraceFileLog::CTraceFileLog(const char * FileName, bool FlushFile, CLog::LOG_OPEN_MODE eMode, size_t dwMaxFileSize) :
    m_FlushFile(FlushFile)
{
    enum
    {
        MB = 1024 * 1024
    };

    m_hLogFile.SetFlush(false);
    m_hLogFile.SetTruncateFile(true);

    if (dwMaxFileSize < 3 || dwMaxFileSize > 2047)
    { // Clamp file size to 5MB if it exceeds 2047 or falls short of 3
        dwMaxFileSize = 5;
    }
    m_hLogFile.SetMaxFileSize((uint32_t)(dwMaxFileSize * MB));

    m_hLogFile.Open(FileName, eMode);
}

CTraceFileLog::~CTraceFileLog()
{
}

void CTraceFileLog::Write(uint32_t module, uint8_t severity, const char * /*file*/, int /*line*/, const char * function, const char * Message)
{
    if (!m_hLogFile.IsOpen())
    {
        return;
    }

#ifdef _WIN32
    SYSTEMTIME sysTime;
    ::GetLocalTime(&sysTime);
    stdstr_f timestamp("%04d/%02d/%02d %02d:%02d:%02d.%03d %05d,", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds, CThread::GetCurrentThreadId());
#else
    time_t ltime;
    ltime = time(&ltime);

    struct tm result = {0};
    localtime_r(&ltime, &result);

    struct timeval curTime;
    gettimeofday(&curTime, nullptr);
    int milliseconds = curTime.tv_usec / 1000;

    stdstr_f timestamp("%04d/%02d/%02d %02d:%02d:%02d.%03d %05d,", result.tm_year + 1900, result.tm_mon + 1, result.tm_mday, result.tm_hour, result.tm_min, result.tm_sec, milliseconds, CThread::GetCurrentThreadId());
#endif

    m_hLogFile.Log(timestamp.c_str());
    m_hLogFile.Log(TraceSeverity(severity));
    m_hLogFile.Log(",");
    m_hLogFile.Log(TraceModule(module));
    m_hLogFile.Log(",");
    m_hLogFile.Log(function);
    m_hLogFile.Log(",");
    m_hLogFile.Log(Message);
    m_hLogFile.Log("\r\n");
    if (m_FlushFile)
    {
        m_hLogFile.Flush();
    }
}

void CTraceFileLog::FlushTrace(void)
{
    m_hLogFile.Flush();
}

void CTraceFileLog::SetFlushFile(bool bFlushFile)
{
    m_FlushFile = bFlushFile;
    FlushTrace();
}
