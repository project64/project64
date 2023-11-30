package emu.project64.settings;

import emu.project64.R;
import emu.project64.SplashActivity;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import androidx.fragment.app.Fragment;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

public abstract class BaseSettingsFragment extends PreferenceFragmentCompat
{
    private static final String DIALOG_FRAGMENT_TAG = "androidx.preference.PreferenceFragment.DIALOG";

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
                            SplashActivity.Reset();
                            Intent SplashIntent = new Intent(getActivity(), SplashActivity.class);
                            SplashIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
                            startActivity(SplashIntent);
                        }
                    }
                };
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
