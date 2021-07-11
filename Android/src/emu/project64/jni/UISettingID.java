package emu.project64.jni;

public enum UISettingID
{		
    Asserts_Version,
    Screen_Orientation,

    //Recent Game
    File_RecentGameFileCount,
    File_RecentGameFileIndex,

    //Touch Screen
    TouchScreen_ButtonScale,
    TouchScreen_Layout,

    //Controller Config
    Controller_ConfigFile,
    Controller_CurrentProfile,
    Controller_Deadzone,
    Controller_Sensitivity,

    //App Info
    AppInfo_RunCount,
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
    
    private UISettingID()
    {
    	this.value = StaticFields.Counter;
    	StaticFields.Counter += 1;
    }   
}
