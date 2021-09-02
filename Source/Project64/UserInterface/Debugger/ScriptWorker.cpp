#include <stdafx.h>
#include "ScriptWorker.h"
#include "ScriptInstance.h"

CScriptWorker::CScriptWorker(CScriptInstance* instance, void* dukObjectHeapPtr) :
    m_bStopping(false),
    m_bRegistered(false),
    m_hThread(nullptr),
    m_Instance(instance),
    m_DukObjectHeapPtr(dukObjectHeapPtr)
{
    m_bRegistered = m_Instance->RegisterWorker(this);
}

CScriptWorker::~CScriptWorker()
{
    if (m_bRegistered)
    {
        StopWorkerProc();
        m_Instance->UnregisterWorker(this);
    }
}

DWORD WINAPI CScriptWorker::ThreadProc(void* _this) {
    ((CScriptWorker*)_this)->WorkerProc();
    return 0;
}

void CScriptWorker::StartWorkerProc()
{
    if (!m_bRegistered)
    {
        return;
    }

    if (m_hThread != nullptr)
    {
        return;
    }

    m_hThread = CreateThread(0, 0, ThreadProc, (void*)this, 0, nullptr);
}

void CScriptWorker::StopWorkerProc()
{
    if (m_hThread == nullptr)
    {
        return;
    }

    {
        CGuard guard(m_CS);
        m_bStopping = true;
    }

    WaitForSingleObject(m_hThread, INFINITE);
    CloseHandle(m_hThread);
    m_hThread = nullptr;
    m_bStopping = false;
}

bool CScriptWorker::StopRequested()
{
    CGuard guard(m_CS);
    return m_bStopping;
}
