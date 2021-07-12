package emu.project64.jni;

public enum AudioSettingID
{
    Output_SwapChannels,
    Output_DefaultFrequency,
    Buffer_PrimarySize,
    Buffer_SecondarySize,
    Buffer_SecondaryNbr,
    Logging_LogAudioInitShutdown,
    Logging_LogAudioInterface,
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
    
    private AudioSettingID()
    {
        this.value = StaticFields.Counter;
        StaticFields.Counter += 1;
    }   
}
