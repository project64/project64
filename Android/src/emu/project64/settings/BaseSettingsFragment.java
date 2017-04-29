/*
 * *************************************************************************
 *  BasePreferenceFragment.java
 * **************************************************************************
 *  Copyright © 2015 VLC authors and VideoLAN
 *  Author: Geoffrey Métais
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *  ***************************************************************************
 */

package emu.project64.settings;

import emu.project64.R;
import emu.project64.SplashActivity;
import emu.project64.jni.NativeExports;
import emu.project64.profile.ControllerProfileActivity;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;

public abstract class BaseSettingsFragment extends PreferenceFragmentCompat
{
    private static final String DIALOG_FRAGMENT_TAG = "android.support.v7.preference.PreferenceFragment.DIALOG";

    protected abstract int getXml();
    protected abstract int getTitleId();

    @Override
    public void onCreatePreferences(Bundle bundle, String s)
    {
        addPreferencesFromResource(getXml());
    }

    @Override
    public void onStart()
    {
        super.onStart();
        final AppCompatActivity activity = (AppCompatActivity)getActivity();
        if (activity != null && activity.getSupportActionBar() != null)
        {
            activity.getSupportActionBar().setTitle(getString(getTitleId()));
        }
    }

    protected void loadFragment(Fragment fragment)
    {
        getActivity().getSupportFragmentManager().beginTransaction()
            .replace(R.id.fragment_placeholder, fragment)
            .addToBackStack("main").commit();
    }

    @Override
    public void onDisplayPreferenceDialog(Preference preference)
    {
        DialogFragment dialogFragment = null;
        if (preference instanceof SeekBarPreference)
        {
            dialogFragment = SeekBarPreferencePreferenceDialogFragmentCompat.newInstance(preference.getKey());
        }
        else if (preference instanceof TwoLinesListPreference)
        {
            dialogFragment = TwoLinesListPreferenceDialogFragmentCompat.newInstance(preference.getKey());
        }
        else
        {
            super.onDisplayPreferenceDialog(preference);
        }

        if (dialogFragment != null)
        {
            dialogFragment.setTargetFragment(this, 0);
            dialogFragment.show(getFragmentManager(), DIALOG_FRAGMENT_TAG);
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference)
    {
    	if (preference.getKey() != null)
    	{
            if (preference.getKey().equals("settings_input"))
            {
                loadFragment(new InputFragment());
            }
            else if (preference.getKey().equals("settings_touch_screen"))
            {
                loadFragment(new TouchScreenFragment());
            }
            else if (preference.getKey().equals("settings_gamepad_screen"))
            {
                final AppCompatActivity activity = (AppCompatActivity)getActivity();
                Intent intent = new Intent( activity, ControllerProfileActivity.class );
                activity.startActivity( intent );
            }
            else if (preference.getKey().equals("settings_video"))
            {
                loadFragment(new VideoFragment());
            }
            else if (preference.getKey().equals("settings_game_list"))
            {
                loadFragment(new GameListFragment());
            }
            else if (preference.getKey().equals("settings_audio"))
            {
                loadFragment(new AudioFragment());
            }
            else if (preference.getKey().equals("settings_patreon"))
            {
                loadFragment(new PatreonFragment());
            }
            else if (preference.getKey().equals("settings_advanced"))
            {
                loadFragment(new AdvancedFragment());
            }
            else if (preference.getKey().equals("logging_core"))
            {
                loadFragment(new LoggingProject64Core());
            }
            else if (preference.getKey().equals("logging_video"))
            {
                loadFragment(new LoggingVideo());
            }
            else if (preference.getKey().equals("logging_audio"))
            {
                loadFragment(new LoggingAudio());
            }
            else if (preference.getKey().equals("settings_reset"))
            {
                DialogInterface.OnClickListener internalListener = new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick( DialogInterface dialog, int which )
                    {
                        if( which == DialogInterface.BUTTON_POSITIVE )
                        {
                            NativeExports.ResetApplicationSettings();
                            SplashActivity.Reset();
                            Intent SplashIntent = new Intent(getActivity(), SplashActivity.class);
                            SplashIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
                            startActivity(SplashIntent);
                        }
                    }
                };

                String title = getString( R.string.settings_reset_title );
                String message = getString( R.string.settings_reset_message );
                AlertDialog.Builder builder = new AlertDialog.Builder( getActivity() ).setTitle( title ).setMessage( message ).setCancelable( false )
                        .setNegativeButton( getString( android.R.string.cancel ), internalListener )
                        .setPositiveButton( getString( android.R.string.ok ), internalListener );
                builder.create().show();
        	}
            else
            {
                return super.onPreferenceTreeClick(preference);
            }    		
    	}
        else
        {
            return super.onPreferenceTreeClick(preference);
        }
        return true;
    }
}
