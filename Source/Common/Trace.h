#pragma once

#include "CriticalSection.h"
#include "LogClass.h"

class CTraceModule
{
    TraceLevel m_Type;

public:
    CTraceModule () {  m_Type = TrLvError; }
    virtual ~CTraceModule () {}

    inline  void       SetTraceLevel ( TraceLevel Type ) { m_Type = Type; }
    inline  TraceLevel GetTraceLevel ( void ) const     { return m_Type; }
    virtual void       Write         ( const char * Message, bool EndOfLine ) = 0;
};

class CTraceFileLog : public CTraceModule
{
    enum { MB = 1024 * 1024 };

    CriticalSection m_CriticalSection;
    CLog            m_hLogFile;
    bool            m_FlushFile;

public:
    CTraceFileLog (const char * FileName, bool FlushFile = true);
    CTraceFileLog(const char * FileName, bool FlushFile, LOG_OPEN_MODE eMode, size_t dwMaxFileSize = 5);
    virtual ~CTraceFileLog ();

    void Write ( const char * Message, bool EndOfLine );
    void SetFlushFile ( bool bFlushFile );
};

class CDebugTraceLog : public CTraceModule
{
public:
    void Write ( const char * Message, bool EndOfLine );
};

CTraceModule * AddTraceModule    ( CTraceModule * TraceModule ); // Must be created with new
CTraceModule * RemoveTraceModule ( CTraceModule * TraceModule ); // Is not automaticly deleted
