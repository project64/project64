package emu.project64.jni;

public enum SystemEvent 
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
    SysEvent_ResumeCPU_FromMenu,
    SysEvent_ResumeCPU_AppGainedActive,
    SysEvent_ResumeCPU_AppGainedFocus,
    SysEvent_ResumeCPU_SaveGame,
    SysEvent_ResumeCPU_LoadGame,
    SysEvent_ResumeCPU_DumpMemory,
    SysEvent_ResumeCPU_SearchMemory,
    SysEvent_ResumeCPU_Settings,
    SysEvent_ResumeCPU_Cheats,
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
    ;

    private int value;
    
    public int getValue() 
    {
        return this.value;
    }
    private static final class StaticFields 
    {
        public static int Counter = 0;
    }
    
    private SystemEvent()
    {
    	this.value = StaticFields.Counter;
    	StaticFields.Counter += 1;
    }
}
