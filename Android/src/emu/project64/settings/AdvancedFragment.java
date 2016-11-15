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
import android.support.v7.preference.Preference;

public class AdvancedFragment extends BaseSettingsFragment
{
    @Override
    protected int getXml() 
    {
        return R.xml.settings_advanced;
    }

    @Override
    protected int getTitleId() 
    {
        return R.string.advanced_screen_title;
    }
}
