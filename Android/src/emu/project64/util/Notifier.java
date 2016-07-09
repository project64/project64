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

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.util.Log;
import android.view.Gravity;
import android.widget.Toast;

/**
 * A small class to encapsulate the notification process for Mupen64PlusAE.
 */
public final class Notifier
{   
    private static Toast sToast = null;
    private static Runnable sToastMessager = null;
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
                new AlertDialog.Builder(finalActivity)
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
                .create()
                .show();
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
    
    /**
     * Pop up a temporary message on the device.
     * 
     * @param activity The activity to display from
     * @param message  The message string to display.
     */
    public static void showToast( Activity activity, String message )
    {
        if( activity == null )
            return;
                
        // Create a messaging task if it doesn't already exist
        if( sToastMessager == null )
        {
            final String ToastMessage = new String(message);
            final Activity ToastActivity = activity;

            sToastMessager = new Runnable()
            {
                @Override
                public void run()
                {
                    // Just show the toast message
                    if( sToast != null )
                        sToast.show();

                    if( sToast != null )
                    {
                        // Toast exists, just change the text
                        Notifier.sToast.setText( ToastMessage );
                    }
                    else
                    {
                        // Message short in duration, and at the bottom of the screen
                        sToast = Toast.makeText( ToastActivity, ToastMessage, Toast.LENGTH_SHORT );
                        sToast.setGravity( Gravity.BOTTOM, 0, 0 );
                    }
                    sToastMessager = null;
                }
            };
        }
        activity.runOnUiThread( sToastMessager );
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
