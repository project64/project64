package emu.project64.util;

import emu.project64.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import androidx.appcompat.view.ContextThemeWrapper;

/**
 * A small class to encapsulate the notification process for Mupen64PlusAE.
 */
public final class Notifier
{
    private static Runnable sDisplayMessager = null;

    /**
     * Pop up a temporary message on the device.
     *
     * @param activity The activity to display from
     * @param message  The message string to display.
     */
    public static void DisplayError( Activity activity, String message )
    {
        if( activity == null )
            return;

        final String finalMessage = new String(message);
        final Activity finalActivity = activity;

        sDisplayMessager = new Runnable()
        {
            @Override
            public void run()
            {
                final LayoutInflater inflater = (LayoutInflater) finalActivity.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
                View layout = inflater.inflate( R.layout.dialog_menu, null );
                TextView DlgTitle = (TextView)layout.findViewById(R.id.dlg_title);
                DlgTitle.setText("Error");
                ListView ListOptions = (ListView)layout.findViewById(R.id.list_options);
                String[] data = { finalMessage };
                NotifierAdapter ad = new NotifierAdapter(finalActivity, sDisplayMessager, data);
                ListOptions.setAdapter(ad);

                final AlertDialog dialog = new AlertDialog.Builder(new ContextThemeWrapper(finalActivity,R.style.Theme_Project64_Dialog_Alert))
                .setView( layout)
                .setCancelable(false)
                .create();
                dialog.setCanceledOnTouchOutside(false);
                dialog.show();
            }
        };
        activity.runOnUiThread( sDisplayMessager );
        synchronized(sDisplayMessager)
        {
            try
            {
                sDisplayMessager.wait();
            }
            catch (InterruptedException e)
            {
            }
            catch (IllegalMonitorStateException e)
            {
            }
        }
        Log.d("DisplayError", "Done");
    }

    public static void showMessage( Activity activity, String message, int Duratation )
    {
        if( activity == null )
            return;

        /*GameOverlay overlay = (GameOverlay) activity.findViewById(R.id.gameOverlay);
        if (overlay == null)
            return;

        overlay.SetDisplayMessage(message, Duratation);*/
    }

    public static void showMessage2( Activity activity, String message )
    {
        if( activity == null )
            return;

        /*GameOverlay overlay = (GameOverlay) activity.findViewById(R.id.gameOverlay);
        overlay.SetDisplayMessage2(message);*/
    }

    public static void EmulationStarted (Activity activity)
    {
        /*((Project64Application) activity.getApplication()).getDefaultTracker().send(new HitBuilders.EventBuilder()
            .setCategory("game")
            .setAction(NativeExports.SettingsLoadString(SettingsID.Rdb_GoodName.getValue()))
            .setLabel(NativeExports.appVersion())
            .build());*/
    }

    private static Runnable runEmulationStopped = null;
    public static void EmulationStopped (Activity activity)
    {
        final Activity finalActivity = activity;

        runEmulationStopped = new Runnable()
        {
            @Override
            public void run()
            {
                finalActivity.finish();
                synchronized(runEmulationStopped)
                {
                    runEmulationStopped.notify();
                };
            }
        };
        activity.runOnUiThread( runEmulationStopped );
        synchronized(runEmulationStopped)
        {
            try
            {
                runEmulationStopped.wait();
            }
            catch (InterruptedException e)
            {
            }
            catch (IllegalMonitorStateException e)
            {
            }
        }
        Log.d("EmulationStopped", "Done");
    }
}
