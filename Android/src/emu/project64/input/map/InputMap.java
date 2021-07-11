package emu.project64.input.map;

import emu.project64.input.AbstractController;
import emu.project64.input.provider.AbstractProvider;

/**
 * A class for mapping arbitrary user inputs to N64 buttons/axes.
 * 
 * @see AbstractProvider
 * @see PeripheralController
 * @see ControllerProfileActivity
 */
public class InputMap extends SerializableMap
{
    /** Map flag: Input code is not mapped. */
    public static final int UNMAPPED                    = -1;

    /** Map offset: N64 non-button controls. */
    public static final int OFFSET_EXTRAS               = AbstractController.NUM_N64_BUTTONS;
    
    /** N64 control: analog-right. */
    public static final int AXIS_R                      = OFFSET_EXTRAS;
    
    /** N64 control: analog-left. */
    public static final int AXIS_L                      = OFFSET_EXTRAS + 1;
    
    /** N64 control: analog-down. */
    public static final int AXIS_D                      = OFFSET_EXTRAS + 2;
    
    /** N64 control: analog-up. */
    public static final int AXIS_U                      = OFFSET_EXTRAS + 3;
    
    /** Total number of N64 controls. */
    public static final int NUM_N64_CONTROLS            = OFFSET_EXTRAS + 4;
    /** Total number of mappable controls/functions. */
    public static final int NUM_MAPPABLES               = NUM_N64_CONTROLS;
    
    public InputMap( String serializedMap )
    {
        super( serializedMap );
    }
    
    /**
     * Gets the command mapped to a given input code.
     * 
     * @param inputCode The standardized input code.
     * 
     * @return The command the code is mapped to, or UNMAPPED.
     * 
     * @see AbstractProvider
     * @see InputMap#UNMAPPED
     */
    public int get( int inputCode )
    {
        return mMap.get( inputCode, UNMAPPED );
    }
    
    /**
     * Maps an input code to a command.
     * 
     * @param inputCode The standardized input code to be mapped.
     * @param command   The index to the N64/Mupen command.
     */
    public void map( int inputCode, int command )
    {
        // Map the input if a valid index was given
        if( command >= 0 && command < NUM_MAPPABLES && inputCode != 0 )
        {
            if( inputCode < 0 )
            {
                // If an analog input is mapped, it should be the only thing mapped to this command
                unmapCommand( command );
            }
            else
            {
                // If a digital input is mapped, no analog inputs can be mapped to this command
                for( int i = mMap.size() - 1; i >= 0; i-- )
                {
                    if( mMap.valueAt( i ) == command && mMap.keyAt( i ) < 0 )
                        mMap.removeAt( i );
                }                
            }
            mMap.put( inputCode, command );
        }
    }
    
    /**
     * Unmaps a command.
     * 
     * @param command The index to the command.
     */
    public void unmapCommand( int command )
    {
        // Remove any matching key-value pairs (count down to accommodate removal)
        for( int i = mMap.size() - 1; i >= 0; i-- )
        {
            if( mMap.valueAt( i ) == command )
                mMap.removeAt( i );
        }
    }
    
    /**
     * Checks if a command is mapped to at least one input code.
     * 
     * @param command The index to the command.
     * 
     * @return True, if the mapping exists.
     */
    public boolean isMapped( int command )
    {
        return mMap.indexOfValue( command ) >= 0;
    }
    public String getMappedCodeInfo( int command )
    {
        String result = "";
        for( int i = 0; i < mMap.size(); i++ )
        {
            if( mMap.valueAt( i ) == command )
            {
                result += AbstractProvider.getInputName( mMap.keyAt( i ) ) + "\n";
            }
        }
        return result.trim();
    }
}
