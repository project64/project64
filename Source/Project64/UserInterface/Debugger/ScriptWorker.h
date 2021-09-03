#include <windows.h>
#pragma once

class CScriptInstance;

class CScriptWorker
{
private:
    bool m_bStopping;
    bool m_bRegistered;
    static DWORD WINAPI ThreadProc(void* _this);

public:
    CScriptWorker(CScriptInstance* instance, void* dukObjectHeapPtr);
    virtual ~CScriptWorker();
    void StartWorkerProc();
    virtual void StopWorkerProc();

protected:
    CScriptInstance* m_Instance;
    void*            m_DukObjectHeapPtr;
    CriticalSection  m_CS;
    HANDLE           m_hThread;

    // Implementation should return when StopRequested() is true
    virtual void WorkerProc() = 0;
    bool StopRequested();
};
