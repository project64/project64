package emu.project64.settings;

import emu.project64.R;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;

public class SettingsFragment extends BaseSettingsFragment
{
    @Override
    protected int getXml() 
    {
        if (NativeExports.SettingsLoadBool(SettingsID.UserInterface_BasicMode.toString()))
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
