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
