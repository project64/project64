#include "stdafx.h"
#include "Thread.h"
#ifndef _WIN32
#include <unistd.h>
#include <pthread.h>
#endif

CThread::CThread(CTHREAD_START_ROUTINE lpStartAddress) :
    m_StartAddress(lpStartAddress),
    m_thread(NULL)
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    WriteTrace(TraceThread, TraceDebug, "Done");
}

CThread::~CThread()
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    if (isRunning())
    {
        Terminate();
    }
#ifdef _WIN32
    CloseHandle(m_thread);
#endif
    WriteTrace(TraceThread, TraceDebug, "Done");
}

bool CThread::Start(void * lpThreadParameter)
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    m_lpThreadParameter = lpThreadParameter;
    m_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadWrapper, this, 0, (LPDWORD)&m_threadID);
    WriteTrace(TraceThread, TraceDebug, "Done");
    return true;
}

void * CThread::ThreadWrapper (CThread * _this)
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    void * res = NULL;
    try
    {
        res = (void *)_this->m_StartAddress(_this->m_lpThreadParameter);
    }
    catch (...)
    {
        //WriteTrace(TraceUserInterface, TraceError, "Unhandled Exception ");
    }
    CloseHandle(_this->m_thread);
    _this->m_thread = NULL;
    WriteTrace(TraceThread, TraceDebug, "Done");
    return res;
}

bool CThread::isRunning(void) const
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    if (m_thread == NULL)
    {
        WriteTrace(TraceThread, TraceDebug, "Done (res: false), m_thread is null");
        return false;
    }

    DWORD ExitCode;
    if (GetExitCodeThread(m_thread, &ExitCode))
    {
        if (ExitCode == STILL_ACTIVE)
        {
            WriteTrace(TraceThread, TraceDebug, "Done (res: true)");
            return true;
        }
    }
    WriteTrace(TraceThread, TraceDebug, "Done (res: false)");
    return false;
}

void CThread::Terminate(void)
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    if (isRunning())
    {
        WriteTrace(TraceThread, TraceDebug, "Terminating thread");
        TerminateThread(m_thread, 0);
    }
    WriteTrace(TraceThread, TraceDebug, "Done");
}

uint32_t CThread::GetCurrentThreadId(void)
{
    return ::GetCurrentThreadId();
}
