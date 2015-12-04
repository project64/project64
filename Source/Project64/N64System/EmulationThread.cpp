/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include <Project64/N64System/N64Class.h>
#include <common/Util.h>
#include <Windows.h>
#include <Objbase.h>

void  CN64System::StartEmulationThead()
{
    ThreadInfo * Info = new ThreadInfo;
    HANDLE  * hThread = new HANDLE;
    *hThread = NULL;

    //create the needed info into a structure to pass as one parameter
    //for creating a thread
    Info->ThreadHandle = hThread;

    *hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartEmulationThread, Info, 0, (LPDWORD)&Info->ThreadID);
}

void CN64System::StartEmulationThread(ThreadInfo * Info)
{
    if (g_Settings->LoadBool(Setting_CN64TimeCritical))
    {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    }

    CoInitialize(NULL);

    EmulationStarting(*Info->ThreadHandle, Info->ThreadID);
    delete ((HANDLE  *)Info->ThreadHandle);
    delete Info;

    CoUninitialize();
}

void CN64System::CloseCpu()
{
    if (m_CPU_Handle == NULL)
    {
        return;
    }

    m_EndEmulation = true;
    if (g_Settings->LoadBool(GameRunning_CPU_Paused))
    {
        m_hPauseEvent.Trigger();
    }

    if (GetCurrentThreadId() == m_CPU_ThreadID)
    {
        ExternalEvent(SysEvent_CloseCPU);
        return;
    }

    HANDLE hThread = m_CPU_Handle;
    m_CPU_Handle = NULL;
    for (int count = 0; count < 200; count++)
    {
        pjutil::Sleep(100);
        if (g_Notify->ProcessGuiMessages())
        {
            return;
        }

        DWORD ExitCode;
        if (GetExitCodeThread(hThread, &ExitCode))
        {
            if (ExitCode != STILL_ACTIVE)
            {
                break;
            }
        }
    }

    if (hThread)
    {
        DWORD ExitCode;
        GetExitCodeThread(hThread, &ExitCode);
        if (ExitCode == STILL_ACTIVE)
        {
            TerminateThread(hThread, 0);
        }
    }
    CloseHandle(hThread);
    CpuStopped();
}