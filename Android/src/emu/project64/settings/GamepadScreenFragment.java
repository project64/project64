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

import android.os.Bundle;
import emu.project64.R;

public class GamepadScreenFragment extends BaseSettingsFragment
{
    @Override
    protected int getXml() 
    {
        return R.xml.setting_gamepad;
    }

    @Override
    protected int getTitleId() 
    {
        return R.string.gamepad_title;
    }
    
    @Override
    public void onCreatePreferences(Bundle bundle, String s)
    {
        super.onCreatePreferences(bundle, s);
        
        /*String profilesDir = AndroidDevice.PACKAGE_DIRECTORY + "/profiles";
        String touchscreenProfiles_cfg = profilesDir + "/touchscreen.cfg";
        ConfigFile touchscreenProfiles = new ConfigFile( touchscreenProfiles_cfg );
        Set<String> layoutsKeySet = touchscreenProfiles.keySet();
        String[] layouts = layoutsKeySet.toArray(new String[layoutsKeySet.size()]);
        
        CharSequence[] entries = new CharSequence[layouts.length];
        String[] entryValues = new String[layouts.length];
        String[] entrySubtitles = new String[layouts.length];
        
        for( int i = 0; i < layouts.length; i++ )
        {
            entries[i] = layouts[i];
            entryValues[i] = layouts[i];
            entrySubtitles[i] = touchscreenProfiles.get(layouts[i]).get("comment");
        }

        final TwoLinesListPreference listPreference = (TwoLinesListPreference) findPreference("touchscreenLayout");
        listPreference.setEntries(entries);
        listPreference.setEntryValues(entryValues);
        listPreference.setEntriesSubtitles(entrySubtitles);*/
    }
}
