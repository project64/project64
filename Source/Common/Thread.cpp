#include "Thread.h"
#include "Trace.h"
#include "TraceModulesCommon.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#endif

CThread::CThread(CTHREAD_START_ROUTINE lpStartAddress) :
    m_StartAddress(lpStartAddress),
    m_lpThreadParameter(nullptr),
    m_thread(nullptr),
#ifndef _WIN32
    m_running(false),
#endif
    m_threadID(0)
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    WriteTrace(TraceThread, TraceDebug, "Done");
}

CThread::~CThread()
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    if (CThread::GetCurrentThreadId() == m_threadID)
    {
        WriteTrace(TraceThread, TraceError, "Deleting from thread!");
    }
    if (CThread::GetCurrentThreadId() != m_threadID && isRunning())
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
#ifdef _WIN32
    m_thread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ThreadWrapper, this, 0, (LPDWORD)&m_threadID);
#else
    pthread_t * thread_id = new pthread_t;

    m_thread = (void*)thread_id;

    int res = pthread_create(thread_id, nullptr, (void *(*)(void *))ThreadWrapper, this);
#endif
    WriteTrace(TraceThread, TraceDebug, "Done");
    return true;
}

void * CThread::ThreadWrapper (CThread * _this)
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    _this->m_threadID = CThread::GetCurrentThreadId();
#ifndef _WIN32
    _this->m_running = true;
    WriteTrace(TraceThread, TraceDebug, "Thread is running");
#endif
    void * res = nullptr;
    try
    {
#if defined(__i386__) || defined(_M_IX86) || defined(__arm__) || defined(_M_ARM)
        res = (void *)_this->m_StartAddress(_this->m_lpThreadParameter);
#else
        res = (void *)((uint64_t)_this->m_StartAddress(_this->m_lpThreadParameter));
#endif
    }
    catch (...)
    {
        //WriteTrace(TraceUserInterface, TraceError, "Unhandled Exception ");
    }
#ifndef _WIN32
    _this->m_running = false;
    WriteTrace(TraceThread, TraceDebug, "Thread is finished");
#endif
#ifdef _WIN32
    CloseHandle(_this->m_thread);
#else
    pthread_t * thread_id = (pthread_t *)_this->m_thread;
    delete thread_id;
#endif
    _this->m_thread = nullptr;
    WriteTrace(TraceThread, TraceDebug, "Done");
    return res;
}

bool CThread::isRunning(void) const
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    if (m_thread == nullptr)
    {
        WriteTrace(TraceThread, TraceDebug, "Done (res: false), m_thread is null");
        return false;
    }
#ifndef _WIN32
    WriteTrace(TraceThread, TraceDebug, "Done (res: %s)", m_running ? "true" : "false");
    return m_running;
#endif
#ifdef _WIN32
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
#else
    return true;
#endif
}

void CThread::Terminate(void)
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    if (isRunning())
    {
#ifdef _WIN32
        WriteTrace(TraceThread, TraceDebug, "Terminating thread");
        TerminateThread(m_thread, 0);
#else
        WriteTrace(TraceThread, TraceError, "Need to fix");
#endif
    }
    WriteTrace(TraceThread, TraceDebug, "Done");
}

uint32_t CThread::GetCurrentThreadId(void)
{
#ifdef _WIN32
    return ::GetCurrentThreadId();
#elif defined(SYS_gettid) || defined(__NR_gettid)
    return syscall(__NR_gettid); // GLIBC has no implementation of gettid()
#else
    return gettid();
#endif
}
