package emu.project64.settings;

import emu.project64.R;
import android.content.Context;
import android.content.res.TypedArray;
import android.support.v7.preference.DialogPreference;
import android.util.AttributeSet;
import android.widget.SeekBar;

/**
 * A type of {@link DialogPreference} that uses a {@link SeekBar} as a means of selecting a desired option.
 */
public class SeekBarPreference extends DialogPreference
{
    private static final int DEFAULT_VALUE = 50;
    private static final int DEFAULT_MIN = 0;
    private static final int DEFAULT_MAX = 100;
    private static final int DEFAULT_STEP = 10;
    private static final String DEFAULT_UNITS = "%";
    
    private int mValue = DEFAULT_VALUE;
    private int mMinValue = DEFAULT_MIN;
    private int mMaxValue = DEFAULT_MAX;
    private int mStepSize = DEFAULT_STEP;
    private String mUnits = DEFAULT_UNITS;

    /**
     * Constructor
     *
     * @param context The {@link Context} this SeekBarPreference is being used in.
     * @param attrs   A collection of attributes, as found associated with a tag in an XML document.
     */
    public SeekBarPreference( Context context, AttributeSet attrs )
    {
        super( context, attrs );
        
        // Get the attributes from the XML file, if provided
        TypedArray a = context.obtainStyledAttributes( attrs, R.styleable.SeekBarPreference );
        setMinValue( a.getInteger( R.styleable.SeekBarPreference_minimumValue, DEFAULT_MIN ) );
        setMaxValue( a.getInteger( R.styleable.SeekBarPreference_maximumValue, DEFAULT_MAX ) );
        setStepSize( a.getInteger( R.styleable.SeekBarPreference_stepSize, DEFAULT_STEP ) );
        setUnits( a.getString( R.styleable.SeekBarPreference_units ) );
        if (getUnits() == null)
        {
            setUnits(DEFAULT_UNITS);
        }
        a.recycle();
        
        // Setup the layout
        setDialogLayoutResource( R.layout.seek_bar_preference );
    }

    /**
     * Constructor
     *
     * @param context The {@link Context} this SeekBarPreference will be used in.
     */
    public SeekBarPreference( Context context )
    {
        this( context, null );
    }

    /**
     * Sets this SeekBarPreference to a specified value.
     *
     * @param value The value to set the SeekBarPreference to.
     */
    public void setValue( int value )
    {
        mValue = validate( value );
        if( shouldPersist() )
            persistInt( mValue );
        setSummary( getValueString( mValue ) );
    }

    /**
     * Sets the minimum value this SeekBarPreference may have.
     *
     * @param minValue The minimum value for this SeekBarPreference.
     */
    public void setMinValue( int minValue )
    {
        mMinValue = minValue;
    }

    /**
     * Sets the maximum value this SeekBarPreference may have.
     *
     * @param maxValue The maximum value for this SeekBarPreference.
     */
    public void setMaxValue( int maxValue )
    {
        mMaxValue = maxValue;
    }

    /**
     * Sets the size of each increment in this SeekBarPreference.
     *
     * @param stepSize The size of each increment.
     */
    public void setStepSize( int stepSize )
    {
        mStepSize = stepSize;
    }

    /**
     * Sets the type of units this SeekBarPreference uses (e.g. "%").
     * 
     * @param units The unit type for this SeekBarPreference to use.
     */
    public void setUnits( String units )
    {
        mUnits = units;
    }

    /**
     * Gets the currently set value.
     * 
     * @return The currently set value in this SeekBarPreference.
     */
    public int getValue()
    {
        return mValue;
    }

    /**
     * Gets the currently set minimum value.
     * 
     * @return The currently set minimum value for this SeekBarPreference.
     */
    public int getMinValue()
    {
        return mMinValue;
    }

    /**
     * Gets the currently set maximum value.
     * 
     * @return The currently set maximum value for this SeekBarPreference.
     */
    public int getMaxValue()
    {
        return mMaxValue;
    }

    /**
     * Gets the currently set increment step size.
     * 
     * @return The currently set increment step size for this SeekBarPreference.
     */
    public int getStepSize()
    {
        return mStepSize;
    }

    /**
     * Gets the currently set units.
     * 
     * @return The currently set unit type this SeekBarPreference uses.
     */
    public String getUnits()
    {
        return mUnits;
    }

    /**
     * Gets the value as a string with units appended.
     * 
     * @param value The value to use in the string.
     * 
     * @return The value as a String.
     */
    public String getValueString( int value )
    {
        return getContext().getString( R.string.seekBarPreference_summary, value, mUnits );
    }
    
    @Override
    protected Object onGetDefaultValue( TypedArray a, int index )
    {
        return a.getInteger( index, DEFAULT_VALUE );
    }
    
    @Override
    protected void onSetInitialValue( boolean restorePersistedValue, Object defaultValue )
    {
        setValue( restorePersistedValue ? getPersistedInt( mValue ) : (Integer) defaultValue );
    }
    
    @Override
    public void onAttached()
    {
        setSummary( getValueString( mValue ) );
        super.onAttached();
    }
    
    public int validate( int value )
    {
        // Round to nearest integer multiple of mStepSize
        int newValue = Math.round( value / (float)getStepSize() ) * getStepSize();
        
        // Address issues when mStepSize is not an integral factor of mMaxValue
        // e.g. mMaxValue = 100, mMinValue = 0, mStepSize = 9, progress = 100 --> newValue = 99 (should be 100)
        // e.g. mMaxValue = 100, mMinValue = 0, mStepSize = 6, progress = 99 --> newValue = 102 (should be 100)
        if( value == getMinValue() || newValue < getMinValue() )
            newValue = getMinValue();
        if( value == getMaxValue() || newValue > getMaxValue() )
            newValue = getMaxValue();
        
        return newValue;
    }
}
