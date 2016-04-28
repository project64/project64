#include "stdafx.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <atomic>
#include <mutex>
#include <chrono>
#include <condition_variable>
#endif

#ifndef _WIN32 // UNIX
#define EVENT_T(event) static_cast<event_t*>(event)
struct event_t
{
    std::condition_variable CVariable;
    std::mutex Mutex;
    std::atomic_bool AutoReset;
    std::atomic_bool State;
};
#endif


SyncEvent::SyncEvent(bool bManualReset)
{
#ifdef _WIN32
    m_Event = CreateEvent(NULL, bManualReset, false, NULL);
#else
    m_Event = static_cast<void*>(new event_t);
    
    EVENT_T(m_Event)->State.store(false);
    EVENT_T(m_Event)->AutoReset.store(!bManualReset);
#endif
}

SyncEvent::~SyncEvent()
{
#ifdef _WIN32
    CloseHandle(m_Event);
#else
    delete EVENT_T(m_Event);
#endif
}

void SyncEvent::Trigger()
{
#ifdef _WIN32
    SetEvent(m_Event);
#else    
    /* Set Event */
    EVENT_T(m_Event)->State.store(true);
    
    /* Notify Waiting Threads */
    if (EVENT_T(m_Event)->AutoReset.load())
    {
        EVENT_T(m_Event)->CVariable.notify_one();
    }
    else
    {
        EVENT_T(m_Event)->CVariable.notify_all();
    }
#endif
}

bool SyncEvent::IsTriggered(int32_t iWaitTime)
{
#ifdef _WIN32
    return (WAIT_OBJECT_0 == WaitForSingleObject(m_Event, iWaitTime));
#else
    if (iWaitTime == 0)
    {
        return EVENT_T(m_Event)->State.load();
    }
    
    std::unique_lock<std::mutex> lock(EVENT_T(m_Event)->Mutex);
    if(iWaitTime == INFINITE_TIMEOUT)
    {
        EVENT_T(m_Event)->CVariable.wait(
            lock,
            [&]{return EVENT_T(m_Event)->State.load();}
        );
    }
    else
    {
        return EVENT_T(m_Event)->CVariable.wait_for(
            lock,
            std::chrono::milliseconds(iWaitTime),
            [&]{return EVENT_T(m_Event)->State.load();}
         );
    }
    
    // Reset Event State
    if(EVENT_T(m_Event)->AutoReset.load())
    {
        EVENT_T(m_Event)->State.store(false);
    }
    
    return true;
#endif
}

void SyncEvent::Reset()
{
#ifdef _WIN32
    ResetEvent(m_Event);
#else
    EVENT_T(m_Event)->State = false;
#endif
}

void * SyncEvent::GetHandle()
{
    return m_Event;
}