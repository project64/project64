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
