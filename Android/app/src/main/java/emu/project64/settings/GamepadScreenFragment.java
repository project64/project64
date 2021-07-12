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
    }
}
