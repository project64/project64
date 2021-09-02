#include <stdafx.h>
#include "JSIntervalWorker.h"
#include "../ScriptInstance.h"
#include "ScriptAPI.h"

CJSIntervalWorker::CJSIntervalWorker(CScriptInstance* inst, void* dukObjectHeapPtr, int delayMS, bool bOnce) :
    CScriptWorker(inst, dukObjectHeapPtr),
    m_DelayMS(delayMS),
    m_bOnce(bOnce)
{
    m_hTimerQuitEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

CJSIntervalWorker::~CJSIntervalWorker()
{
    StopWorkerProc();
    CloseHandle(m_hTimerQuitEvent);
}

void CJSIntervalWorker::WorkerProc()
{
    HANDLE hTimer = CreateWaitableTimer(nullptr, false, nullptr);

    LARGE_INTEGER liTime;
    liTime.QuadPart = -m_DelayMS * 10000;
    SetWaitableTimer(hTimer, &liTime, m_DelayMS, nullptr, nullptr, true);

    HANDLE hWaitHandles[] = { hTimer, m_hTimerQuitEvent };

    while (true)
    {
        DWORD nHandle = WaitForMultipleObjects(2, hWaitHandles, FALSE, INFINITE);

        if (nHandle == WAIT_OBJECT_0)
        {
            m_Instance->PostCMethodCall(m_DukObjectHeapPtr, ScriptAPI::js__IntervalContext_invokeFunc);

            if (m_bOnce)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    CloseHandle(hTimer);
    m_Instance->PostCMethodCall(m_DukObjectHeapPtr, ScriptAPI::js__IntervalContext_remove);
}

void CJSIntervalWorker::StopWorkerProc()
{
    SetEvent(m_hTimerQuitEvent);
    CScriptWorker::StopWorkerProc();
}
