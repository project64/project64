package emu.project64.settings;

import emu.project64.R;

public class AudioFragment extends BaseSettingsFragment
{
    @Override
    protected int getXml() 
    {
        return R.xml.setting_audio;
    }

    @Override
    protected int getTitleId() 
    {
        return R.string.audio_screen_title;
    }
}
