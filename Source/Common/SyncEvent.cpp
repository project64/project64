#include "stdafx.h"

SyncEvent::SyncEvent(bool bManualReset)
{
    m_Event = CreateEvent(NULL, bManualReset, FALSE, NULL);
}

SyncEvent::~SyncEvent()
{
    CloseHandle(m_Event);
}

void SyncEvent::Trigger()
{
    SetEvent(m_Event);
}

bool SyncEvent::IsTriggered(int32_t iWaitTime)
{
    return (WAIT_OBJECT_0 == WaitForSingleObject(m_Event,iWaitTime));
}

void SyncEvent::Reset()
{
    ResetEvent(m_Event);
}

void * SyncEvent::GetHandle()
{
    return m_Event;
}