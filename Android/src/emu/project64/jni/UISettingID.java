/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
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

    //Support Window
    SupportWindow_FirstRun,
    SupportWindow_AlwaysShow,
    SupportWindow_ShowingSupportWindow,
    SupportWindow_RunCount,
    SupportWindow_PatreonEmail,

    //Game Settings
    Game_RunCount,
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
