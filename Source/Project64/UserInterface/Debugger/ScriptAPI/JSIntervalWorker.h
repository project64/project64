#include "../ScriptWorker.h"

#pragma once

class CJSIntervalWorker : public CScriptWorker {
private:
    int m_DelayMS;
    bool m_bOnce;
    HANDLE m_hTimerQuitEvent;
public:
    CJSIntervalWorker(CScriptInstance* inst, void* dukObjectHeapPtr, int delayMS, bool bOnce);
    virtual ~CJSIntervalWorker();

    virtual void WorkerProc();
    virtual void StopWorkerProc();
};