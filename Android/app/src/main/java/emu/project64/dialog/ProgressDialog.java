package emu.project64.dialog;

import emu.project64.R;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;

public class ProgressDialog
{

    private final Activity mActivity;
    private final TextView mTextMessage;
    private AlertDialog mDialog;

    @SuppressLint( "InflateParams" )
    public ProgressDialog( Activity activity, CharSequence title, CharSequence message, boolean cancelable )
    {
        mActivity = activity;
        
        final LayoutInflater inflater = (LayoutInflater) mActivity.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
        View layout = inflater.inflate( R.layout.progress_dialog, null );

        TextView DlgTitle = (TextView)layout.findViewById(R.id.dlg_title);
        DlgTitle.setText(title);
        mTextMessage = (TextView) layout.findViewById( R.id.textMessage );
        mTextMessage.setText( message );

        Builder builder = new Builder( activity, R.style.Theme_Project64_Dialog_Alert );
        builder.setView( layout);
        builder.setCancelable( cancelable );
        builder.setPositiveButton( null, null );
        builder.setNegativeButton( null, null );
        mDialog = builder.create();
        mDialog.getWindow().setBackgroundDrawableResource(android.R.color.transparent);
    }

    public void show()
    {
        mDialog.show();
    }
    
    public void dismiss()
    {
        mDialog.dismiss();
    }

    public void setMessage( final CharSequence text )
    {
        mActivity.runOnUiThread( new Runnable()
        {
            @Override
            public void run()
            {
                mTextMessage.setText( text );
            }
        } );
    }
}
