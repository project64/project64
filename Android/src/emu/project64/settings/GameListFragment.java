package emu.project64.settings;

import emu.project64.R;

public class GameListFragment extends BaseSettingsFragment
{
    @Override
    protected int getXml() 
    {
        return R.xml.setting_gamelist;
    }

    @Override
    protected int getTitleId() 
    {
        return R.string.game_list_title;
    }
}
