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
import android.util.Log;
import emu.project64.AndroidDevice;
import emu.project64.R;
import emu.project64.jni.NativeVideo;

public class VideoFragment extends BaseSettingsFragment
{
    @Override
    protected int getXml()
    {
        return R.xml.setting_video;
    }

    @Override
    protected int getTitleId()
    {
        return R.string.video_screen_title;
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s)
    {
        super.onCreatePreferences(bundle, s);

        Log.d("VideoFragment", "onCreatePreferences");
        NativeVideo.UpdateScreenRes(AndroidDevice.nativeWidth, AndroidDevice.nativeHeight);
        
        int ResCount = NativeVideo.getResolutionCount();
        CharSequence[] ResEntries = new CharSequence[ResCount];
        String[] ResEntryValues = new String[ResCount];
        String[] ResEntrySubtitles = new String[ResCount];

        for( int i = 0; i < ResCount; i++ )
        {
            ResEntries[i] = NativeVideo.getResolutionName(i);
            ResEntryValues[i] = Integer.toString(i);
            ResEntrySubtitles[i] = "";
        }

        final TwoLinesListPreference listPreference = (TwoLinesListPreference) findPreference("video_screenResolution");
        listPreference.setEntries(ResEntries);
        listPreference.setEntryValues(ResEntryValues);
        listPreference.setEntriesSubtitles(ResEntrySubtitles);
    }
}
