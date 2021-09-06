package emu.project64.settings;

import emu.project64.R;
import android.os.Bundle;
import androidx.appcompat.app.AlertDialog;
import androidx.preference.DialogPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceDialogFragmentCompat;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.SeekBar.OnSeekBarChangeListener;

public class SeekBarPreferencePreferenceDialogFragmentCompat extends PreferenceDialogFragmentCompat
    implements DialogPreference.TargetFragment, OnSeekBarChangeListener
{
    private TextView mTextView;
    private SeekBar mSeekBar;

    public void onDialogClosed(boolean positiveResult) 
    {        
        if( positiveResult )
        {
            final SeekBarPreference preference = getSeekBarPreference();
            int value = mSeekBar.getProgress() + preference.getMinValue();
            preference.setValue( value );
        }
    }

    public static SeekBarPreferencePreferenceDialogFragmentCompat newInstance(String key)
    {
        SeekBarPreferencePreferenceDialogFragmentCompat fragment = new SeekBarPreferencePreferenceDialogFragmentCompat();
        Bundle b = new Bundle(1);
        b.putString("key", key);
        fragment.setArguments(b);
        return fragment;
    }
    
    @Override
    public Preference findPreference(CharSequence charSequence) 
    {
        return getPreference();
    }
    
    private SeekBarPreference getSeekBarPreference()
    {
        return (SeekBarPreference)this.getPreference();
    }

    @Override
    protected void onBindDialogView(View view) 
    {
        super.onBindDialogView(view);
        
        // Grab the widget references
        mTextView = (TextView) view.findViewById( R.id.textFeedback );
        mSeekBar = (SeekBar) view.findViewById( R.id.seekbar );
    }
    
    protected void onPrepareDialogBuilder(AlertDialog.Builder builder) 
    {
        super.onPrepareDialogBuilder(builder);
        final SeekBarPreference preference = getSeekBarPreference();
        
        // Initialize and refresh the widgets
        mSeekBar.setMax( preference.getMaxValue() - preference.getMinValue());
        mSeekBar.setOnSeekBarChangeListener( this );
        mSeekBar.setProgress( preference.getValue() - preference.getMinValue() );
        mTextView.setText( preference.getValueString( preference.getValue() ) );
    }
    
    @Override
    public void onStartTrackingTouch( SeekBar seekBar )
    {
    }
    
    @Override
    public void onStopTrackingTouch( SeekBar seekBar )
    {
    }
    
    @Override
    public void onProgressChanged( SeekBar seekBar, int progress, boolean fromUser )
    {
        final SeekBarPreference preference = getSeekBarPreference();

        int value = preference.validate( progress + preference.getMinValue() );
        if( value != ( progress + preference.getMinValue() ) )
            seekBar.setProgress( value - preference.getMinValue() );
        mTextView.setText( preference.getValueString( value ) );
    }
}
