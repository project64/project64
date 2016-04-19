#pragma once
#include "LogClass.h"

enum TraceSeverity
{
    TraceError = 0x00000001,
    TraceWarning = 0x00000002,
    TraceNotice = 0x00000003,
    TraceInfo = 0x00000004,
    TraceDebug = 0x00000005,
    TraceVerbose = 0x00000006,
};

#if defined(_WIN32)
#include <objbase.h>
#else
#define __interface     struct
#endif

__interface CTraceModule
{
    virtual void Write(uint32_t module, uint8_t severity, const char * file, int line, const char * function, const char * Message) = 0;
};

class CTraceFileLog : public CTraceModule
{
public:
    CTraceFileLog(const char * FileName, bool FlushFile, CLog::LOG_OPEN_MODE eMode, size_t dwMaxFileSize = 5);
    virtual ~CTraceFileLog();

    void SetFlushFile(bool bFlushFile);
    void Write(uint32_t module, uint8_t severity, const char * file, int line, const char * function, const char * Message);

private:
    CLog m_hLogFile;
    bool m_FlushFile;
};

#define WriteTrace(m, s, format, ...) if(g_ModuleLogLevel[(m)] >= (s)) { WriteTraceFull((m), (s), __FILE__, __LINE__, __FUNCTION__, (format), ## __VA_ARGS__); }

CTraceModule * TraceAddModule(CTraceModule * TraceModule);
CTraceModule * TraceRemoveModule(CTraceModule * TraceModule);
const char * TraceSeverity(uint8_t severity);
const char * TraceModule(uint32_t module);
void TraceSetModuleName(uint8_t module, const char * Name);
void CloseTrace(void);

void WriteTraceFull(uint32_t module, uint8_t severity, const char * file, int line, const char * function, const char *format, ...);
void TraceSetMaxModule(uint32_t MaxModule, uint8_t DefaultSeverity);

extern uint32_t * g_ModuleLogLevel;
