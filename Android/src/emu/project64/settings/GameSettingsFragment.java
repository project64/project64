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
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceManager;

public class GameSettingsFragment extends BaseSettingsFragment
{
    @Override
    protected int getXml() 
    {
        return R.xml.game_settings;
    }

    @Override
    protected int getTitleId() 
    {
        return R.string.preferences;
    }
}
