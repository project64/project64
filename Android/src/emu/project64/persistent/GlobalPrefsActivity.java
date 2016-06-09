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
package emu.project64.persistent;

import emu.project64.R;

import emu.project64.compat.AppCompatPreferenceActivity;
import android.annotation.TargetApi;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;

public class GlobalPrefsActivity extends AppCompatPreferenceActivity implements OnPreferenceClickListener,
        OnSharedPreferenceChangeListener
{
    private static final String NAVIGATION_MODE = "navigationMode";
       
    @SuppressWarnings( "deprecation" )
    @Override
    protected void onCreate( Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );
        
        // Load user preference menu structure from XML and update view
        addPreferencesFromResource( R.xml.preferences_global );
        
    }
    
    
    @Override
    public void onSharedPreferenceChanged( SharedPreferences sharedPreferences, String key )
    {
        if( key.equals( NAVIGATION_MODE ) )
        {
            // Sometimes one preference change affects the hierarchy or layout of the views. In this
            // case it's easier just to restart the activity than try to figure out what to fix.
			finish();
			startActivity(getIntent());
        }
        else
        {
            // Just refresh the preference screens in place
            refreshViews();
        }
    }
    
    @TargetApi( 9 )
    private void refreshViews()
    {
        // Refresh the preferences object
        
    }
    
    @Override
    public boolean onPreferenceClick( Preference preference )
    {
    	// Let Android handle all other preference clicks
        return false;
    }
}
