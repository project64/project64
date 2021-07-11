package emu.project64.settings;

import emu.project64.R;

public class LoggingProject64Core extends BaseSettingsFragment
{
    @Override
    protected int getXml() 
    {
        return R.xml.logging_project64core;
    }

    @Override
    protected int getTitleId() 
    {
        return R.string.logging_project64core;
    }
}
