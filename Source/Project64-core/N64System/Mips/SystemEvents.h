#pragma once

#include <Common/CriticalSection.h>

enum SystemEvent
{
    SysEvent_ExecuteInterrupt,
    SysEvent_GSButtonPressed,
    SysEvent_ResetCPU_Soft,
    SysEvent_ResetCPU_SoftDone,
    SysEvent_ResetCPU_Hard,
    SysEvent_CloseCPU,
    SysEvent_PauseCPU_FromMenu,
    SysEvent_PauseCPU_AppLostActive,
    SysEvent_PauseCPU_AppLostActiveDelay,
    SysEvent_PauseCPU_AppLostFocus,
    SysEvent_PauseCPU_SaveGame,
    SysEvent_PauseCPU_LoadGame,
    SysEvent_PauseCPU_DumpMemory,
    SysEvent_PauseCPU_SearchMemory,
    SysEvent_PauseCPU_Settings,
    SysEvent_PauseCPU_Cheats,
    SysEvent_PauseCPU_ChangingBPs,
    SysEvent_PauseCPU_Enhancement,
    SysEvent_ResumeCPU_FromMenu,
    SysEvent_ResumeCPU_AppGainedActive,
    SysEvent_ResumeCPU_AppGainedFocus,
    SysEvent_ResumeCPU_SaveGame,
    SysEvent_ResumeCPU_LoadGame,
    SysEvent_ResumeCPU_DumpMemory,
    SysEvent_ResumeCPU_SearchMemory,
    SysEvent_ResumeCPU_Settings,
    SysEvent_ResumeCPU_Cheats,
    SysEvent_ResumeCPU_ChangingBPs,
    SysEvent_ResumeCPU_Enhancement,
    SysEvent_ChangingFullScreen,
    SysEvent_ChangePlugins,
    SysEvent_SaveMachineState,
    SysEvent_LoadMachineState,
    SysEvent_Interrupt_SP,
    SysEvent_Interrupt_SI,
    SysEvent_Interrupt_AI,
    SysEvent_Interrupt_VI,
    SysEvent_Interrupt_PI,
    SysEvent_Interrupt_DP,
    SysEvent_ResetFunctionTimes,
    SysEvent_DumpFunctionTimes,
    SysEvent_ResetRecompilerCode,
};

const char * SystemEventName(SystemEvent event);

class CN64System;
class CRegisters;
class CPlugins;

class CSystemEvents
{
    typedef std::vector<SystemEvent> EventList;

public:
    CSystemEvents(CN64System & System, CPlugins * Plugins);
    ~CSystemEvents();

public:
    void ExecuteEvents();
    void QueueEvent(SystemEvent action);

    const bool & DoSomething() const
    {
        return m_DoSomething;
    }

private:
    CSystemEvents();
    CSystemEvents(const CSystemEvents &);
    CSystemEvents & operator=(const CSystemEvents &);

    void ChangePluginFunc();

    CN64System & m_System;
    CRegisters & m_Reg;
    CPlugins * m_Plugins;
    EventList m_Events;
    bool m_DoSomething;
    CriticalSection m_CS;
};
