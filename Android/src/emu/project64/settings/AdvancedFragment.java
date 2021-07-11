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
