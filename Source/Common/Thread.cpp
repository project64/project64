#include "stdafx.h"
#include "Thread.h"
#ifndef _WIN32
# include <pthread.h>
# include <signal.h>
# include <sys/types.h>
# if defined(__ANDROID__)
#  include <unistd.h>
# elif defined(__linux)
#  include <cxxabi.h>
# endif
# define THREAD(t) *static_cast<pthread_t*>(t)
#endif


// glibc workaround for gettid()
#ifdef __GNU_LIBRARY__
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

CThread::CThread(CTHREAD_START_ROUTINE lpStartAddress) :
    m_StartAddress(lpStartAddress),
    m_thread(NULL)
{
    WriteTrace(TraceThread, TraceDebug, "Start");
#ifndef _WIN32
    m_thread = static_cast<void*>(new pthread_t);
#endif
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
#else
    if(m_thread != NULL)
    {
        delete &THREAD(m_thread);
        m_thread = NULL;
    }
#endif
    WriteTrace(TraceThread, TraceDebug, "Done");
}

bool CThread::Start(void * lpThreadParameter)
{   WriteTrace(TraceThread, TraceDebug, "Start");
    m_lpThreadParameter = lpThreadParameter;
    
#ifdef _WIN32
    m_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadWrapper, this, 0, (LPDWORD)&m_threadID);
#else
    int rc =  pthread_create(&THREAD(m_thread), NULL, (void *(*)(void *))&CThread::ThreadWrapper, this);
    if (rc != 0)
    {
        WriteTrace(TraceThread, TraceError, "Creating thread failed!");
        return false;
    }
#endif
    WriteTrace(TraceThread, TraceDebug, "Done");
    return true;
}

void * CThread::ThreadWrapper (CThread * _this)
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    
    /* Get Thread ID on UNIX */
#ifndef _WIN32
    _this->m_threadID = CThread::GetCurrentThreadId();
#endif
    
    /* Set pthread cancel state and type */
#if !defined(_WIN32) && !defined(__ANDROID__)
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif

    void * res = NULL;
    try
    {
        res = (void *)_this->m_StartAddress(_this->m_lpThreadParameter);
    }
#if defined(__linux) && !defined(__ANDROID__) && defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 2
    // for post gcc 4.1.2 or glibc version used in RHEL6 and above -
    // pthread_exit and pthread_cancel will cause
    // an abi::__forced_unwind to be thrown. Rethrow it.
    catch (abi::__forced_unwind&)
    {
        throw;
    }
#endif
    catch (...)
    {
#if defined(__linux) && !defined(__ANDROID__) && defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ == 1
        // for gcc 4.1.2 or glibc version used in RHEL 5
        // that does not work with the abi::__forced_unwind
        throw;
#endif
        //WriteTrace(TraceUserInterface, TraceError, "Unhandled Exception ");
    }

#ifdef _WIN32
    CloseHandle(_this->m_thread);
    _this->m_thread = NULL;
#endif
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
#else
    if(pthread_kill(THREAD(m_thread), 0) == 0)
    {
        WriteTrace(TraceThread, TraceDebug, "Done (res: true)");
        return true;
    }
#endif
    WriteTrace(TraceThread, TraceDebug, "Done (res: false)");
    return false;
}

void CThread::Terminate(void)
{
    WriteTrace(TraceThread, TraceDebug, "Start");
    if (isRunning())
    {
        WriteTrace(TraceThread, TraceDebug, "Terminating thread");
#ifdef _WIN32
        TerminateThread(m_thread, 0);
        m_thread = NULL;
#else
#  if defined(__ANDROID__)
        usleep(1000);
        pthread_kill(THREAD(m_thread), SIGTERM);
#  else
        // On Mac OS X thread is not terminated
        // unless it calls any of the cancellation point function
        // or call pthread_testcancel()
        pthread_cancel(THREAD(m_thread));
#  endif
        pthread_join(THREAD(m_thread), NULL);
#endif
    }
    WriteTrace(TraceThread, TraceDebug, "Done");
}

uint32_t CThread::GetCurrentThreadId(void)
{
#ifdef _WIN32
    return ::GetCurrentThreadId();
#elif defined(__APPLE__)
    uint64_t tid;
    pthread_threadid_np(NULL,&tid);
    return tid;
#else
    return gettid();
#endif
}
