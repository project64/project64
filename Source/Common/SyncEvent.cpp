#include "stdafx.h"
#ifdef _WIN32
#include <Windows.h>
#endif

SyncEvent::SyncEvent(bool bManualReset)
{
#ifdef _WIN32
    m_Event = CreateEvent(NULL, bManualReset, FALSE, NULL);
#else
    m_signalled = false;
    m_Event = new pthread_mutex_t;
    m_cond = new pthread_cond_t;
    pthread_mutex_init((pthread_mutex_t*)m_Event, NULL);
    pthread_cond_init((pthread_cond_t*)m_cond, NULL);
#endif
}

SyncEvent::~SyncEvent()
{
#ifdef _WIN32
    CloseHandle(m_Event);
#else
    pthread_mutex_destroy((pthread_mutex_t*)m_Event);
    pthread_cond_destroy((pthread_cond_t*)m_cond);
    delete (pthread_mutex_t*)m_Event;
    delete (pthread_cond_t*)m_cond;
#endif
}

void SyncEvent::Trigger()
{
#ifdef _WIN32
    SetEvent(m_Event);
#else
    pthread_mutex_lock((pthread_mutex_t*)m_Event);
    m_signalled = true;
    pthread_mutex_unlock((pthread_mutex_t*)m_Event);
    pthread_cond_signal((pthread_cond_t*)m_cond);
#endif
}

bool SyncEvent::IsTriggered(int32_t iWaitTime)
{
#ifdef _WIN32
    return (WAIT_OBJECT_0 == WaitForSingleObject(m_Event, iWaitTime));
#else
    pthread_mutex_lock((pthread_mutex_t*)m_Event);
    while (!m_signalled)
    {
        pthread_cond_wait((pthread_cond_t*)m_cond, (pthread_mutex_t*)m_Event);
    }
    pthread_mutex_unlock((pthread_mutex_t*)m_Event);
    Reset();
    return true;
#endif
}

void SyncEvent::Reset()
{
#ifdef _WIN32
    ResetEvent(m_Event);
#else
    pthread_mutex_lock((pthread_mutex_t*)m_Event);
    m_signalled = false;
    pthread_mutex_unlock((pthread_mutex_t*)m_Event);
#endif
}

void * SyncEvent::GetHandle()
{
    return m_Event;
}