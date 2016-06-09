/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
package emu.project64.dialog;

import emu.project64.R;
import emu.project64.util.DeviceUtil;
import android.annotation.SuppressLint;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;

public class Popups
{
    
    public static void showHardwareInfo( Context context )
    {
        String title = context.getString( R.string.menuItem_hardwareInfo );
        String axisInfo = DeviceUtil.getAxisInfo();
        String peripheralInfo = DeviceUtil.getPeripheralInfo();
        String cpuInfo = DeviceUtil.getCpuInfo();
        String message = axisInfo + peripheralInfo + cpuInfo;
        showShareableText( context, title, message );
    }
    
    public static void showShareableText( final Context context, String title, final String message )
    {
        // Set up click handler to share text with a user-selected app (email, clipboard, etc.)
        DialogInterface.OnClickListener shareHandler = new DialogInterface.OnClickListener()
        {
            @SuppressLint( "InlinedApi" )
            @Override
            public void onClick( DialogInterface dialog, int which )
            {
                launchPlainText( context, message,
                        context.getText( R.string.actionShare_title ) );
            }
        };
        
        new Builder( context ).setTitle( title ).setMessage( message.toString() )
                .setNeutralButton( R.string.actionShare_title, shareHandler ).create().show();
    }

	@SuppressLint( "InlinedApi" )
    public static void launchPlainText( Context context, String text, CharSequence chooserTitle )
    {
        // See http://android-developers.blogspot.com/2012/02/share-with-intents.html
        Intent intent = new Intent( android.content.Intent.ACTION_SEND );
        intent.setType( "text/plain" );
        intent.addFlags( Intent.FLAG_ACTIVITY_NEW_DOCUMENT );
        intent.putExtra( Intent.EXTRA_TEXT, text );
        // intent.putExtra( Intent.EXTRA_SUBJECT, subject );
        // intent.putExtra( Intent.EXTRA_EMAIL, new String[] { emailTo } );
        context.startActivity( Intent.createChooser( intent, chooserTitle ) );
    }
}
