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

import emu.project64.AndroidDevice;
import emu.project64.R;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.MenuItem;

public class GameSettingsActivity extends AppCompatActivity implements SharedPreferences.OnSharedPreferenceChangeListener
{
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.settings_activity);
        
        // Add the tool bar to the activity (which supports the fancy menu/arrow animation)
        Toolbar toolbar = (Toolbar) findViewById( R.id.toolbar );
        toolbar.setTitle( getString(R.string.settings_title) );
        setSupportActionBar( toolbar );
        ActionBar actionbar = getSupportActionBar();
        
        if (AndroidDevice.IS_ICE_CREAM_SANDWICH)
        {
            actionbar.setHomeButtonEnabled(true);
            actionbar.setDisplayHomeAsUpEnabled(true);
        }
        
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        sharedPrefs.edit().clear()
        .putString("Game_CpuType",String.valueOf(NativeExports.SettingsLoadDword(SettingsID.Game_CpuType.getValue())))
        .putBoolean("Game_BlockLinking",NativeExports.SettingsLoadBool(SettingsID.Game_BlockLinking.getValue()))
        .apply();  

        sharedPrefs.registerOnSharedPreferenceChangeListener(this);
        
        if (savedInstanceState == null)
        {
            getSupportFragmentManager().beginTransaction().replace(R.id.fragment_placeholder, new GameSettingsFragment()).commit();
        }
    }
    
    @Override
    protected void onStop()
    {
        super.onStop();
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        sharedPrefs.unregisterOnSharedPreferenceChangeListener(this);
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) 
    {
        switch (item.getItemId()) 
        {
        case android.R.id.home:
            if (!getSupportFragmentManager().popBackStackImmediate())
                finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key)
    {
        if (key.equals("Game_CpuType")) { NativeExports.SettingsSaveDword(SettingsID.Game_CpuType.getValue(), Integer.valueOf(sharedPreferences.getString(key, "1"))); }
        else if (key.equals("Game_BlockLinking")) { NativeExports.SettingsSaveBool(SettingsID.Game_BlockLinking.getValue(), sharedPreferences.getBoolean(key,false)); }
    }
}
