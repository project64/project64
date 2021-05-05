#include "stdafx.h"
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/Notification.h>
#include <Common/Util.h>

void  CN64System::StartEmulationThead()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");
    
    CThread * thread = new CThread((CThread::CTHREAD_START_ROUTINE)StartEmulationThread);
    thread->Start(thread);
    WriteTrace(TraceN64System, TraceDebug, "Done");
}

void CN64System::StartEmulationThread(CThread * thread)
{
    WriteTrace(TraceN64System, TraceDebug, "Start");
#ifdef _WIN32
    if (g_Settings->LoadBool(Setting_CN64TimeCritical))
    {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    }

    CoInitialize(nullptr);

    EmulationStarting(thread);

    CoUninitialize();
#else
    EmulationStarting(thread);
#endif
    WriteTrace(TraceN64System, TraceDebug, "Done");
}

void CN64System::CloseCpu()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");
    if (m_thread == nullptr)
    {
        return;
    }

    WriteTrace(TraceN64System, TraceDebug, "Setting end emulation");
    m_EndEmulation = true;
    if (g_Settings->LoadBool(GameRunning_CPU_Paused))
    {
        WriteTrace(TraceN64System, TraceDebug, "Resume CPU");
        m_hPauseEvent.Trigger();
    }

    if (CThread::GetCurrentThreadId() == m_thread->ThreadID())
    {
        WriteTrace(TraceN64System, TraceDebug, "CloseCpu called on emulation thread");
        ExternalEvent(SysEvent_CloseCPU);
        return;
    }

    CThread * hThread = m_thread;
    m_thread = nullptr;
    for (int count = 0; count < 200; count++)
    {
        if (hThread == nullptr || !hThread->isRunning())
        {
            WriteTrace(TraceN64System, TraceDebug, "Thread no longer running");
            break;
        }
        WriteTrace(TraceN64System, TraceDebug, "%d - waiting", count);
        pjutil::Sleep(100);
        WriteTrace(TraceN64System, TraceDebug, "%d - Finished wait", count);
        if (g_Notify->ProcessGuiMessages())
        {
            return;
        }
    }
    CpuStopped();
    WriteTrace(TraceN64System, TraceDebug, "Deleting thread object");
    delete hThread;
    WriteTrace(TraceN64System, TraceDebug, "Done");
}
