#pragma once

class SyncEvent
{
public:
    enum { INFINITE_TIMEOUT = 0xFFFFFFFF };

    SyncEvent(bool bManualReset = true);
    ~SyncEvent(void);

    void Trigger (void);
    bool IsTriggered (int32_t iWaitTime = 0);
    void Reset();
    void * GetHandle();

protected:
    SyncEvent(const SyncEvent&);				// Disable copy constructor
    SyncEvent& operator=(const SyncEvent&);		// Disable assignment

    void * m_Event;
};