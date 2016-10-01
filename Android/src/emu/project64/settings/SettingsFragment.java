/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
package emu.project64.settings;

import emu.project64.R;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;

public class SettingsFragment extends BaseSettingsFragment
{
    @Override
    protected int getXml() 
    {
        if (NativeExports.SettingsLoadBool(SettingsID.UserInterface_BasicMode.getValue()))
        {
            return R.xml.settings_basic;
        }
        return R.xml.settings;
    }

    @Override
    protected int getTitleId() 
    {
        return R.string.preferences;
    }
}
