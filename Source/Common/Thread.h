#pragma once

class CThread 
{
public:
    typedef uint32_t(__stdcall * CTHREAD_START_ROUTINE)(void * lpThreadParameter);
    CThread(CTHREAD_START_ROUTINE lpStartAddress);
    ~CThread();

    bool Start(void * lpThreadParameter);
    
    inline uint32_t ThreadID(void) const { return m_threadID;  }
    
    bool isRunning(void) const;
    void Terminate(void);
    static uint32_t GetCurrentThreadId(void);

private:
    CThread(void);                          // Disable default constructor
    CThread(const CThread&);				// Disable copy constructor
    CThread& operator=(const CThread&);		// Disable assignment

    static void * ThreadWrapper (CThread * _this);

    CTHREAD_START_ROUTINE m_StartAddress;
    void * m_lpThreadParameter;
    void * m_thread;
    uint32_t m_threadID;
};