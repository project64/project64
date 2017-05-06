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
package emu.project64.util;

import emu.project64.game.GameOverlay;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;
import emu.project64.Project64Application;
import emu.project64.R;

import com.google.android.gms.analytics.HitBuilders;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.util.Log;

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
                final AlertDialog dialog = new AlertDialog.Builder(finalActivity)
                .setTitle("Error")
                .setMessage(finalMessage)
                .setPositiveButton("OK", new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int id)
                    {
                        // You don't have to do anything here if you just want it dismissed when clicked
                       synchronized(sDisplayMessager)
                       {
                           sDisplayMessager.notify();
                       };
                    }
                })
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

        GameOverlay overlay = (GameOverlay) activity.findViewById(R.id.gameOverlay);
        if (overlay == null)
            return;

        overlay.SetDisplayMessage(message, Duratation);
    }

    public static void showMessage2( Activity activity, String message )
    {
        if( activity == null )
            return;

        GameOverlay overlay = (GameOverlay) activity.findViewById(R.id.gameOverlay);
        overlay.SetDisplayMessage2(message);
    }

    public static void EmulationStarted (Activity activity)
    {
        ((Project64Application) activity.getApplication()).getDefaultTracker().send(new HitBuilders.EventBuilder()
            .setCategory("game")
            .setAction(NativeExports.SettingsLoadString(SettingsID.Rdb_GoodName.getValue()))
            .setLabel(NativeExports.appVersion())
            .build());
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
