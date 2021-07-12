package emu.project64.input.provider;

import java.util.List;

import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.view.KeyEvent;
import android.view.View;

/**
 * A class for transforming Android KeyEvent inputs into a common format.
 */
public class KeyProvider extends AbstractProvider implements View.OnKeyListener,
        DialogInterface.OnKeyListener
{
    /**
     * The formula for decoding KeyEvent data for specific Android IMEs.
     */
    public enum ImeFormula
    {
        /** The default decoding formula. */
        DEFAULT,
        /** The formula for <i>USB/BT Joystick Center</i>, by Poke64738. */
        USB_BT_JOYSTICK_CENTER,
        /** The formula for <i>BT Controller</i>, by droidbean. */
        BT_CONTROLLER,
        /** An example decoding formula. */
        EXAMPLE_IME
    }
    
    /** The IME formula for decoding KeyEvent data. */
    private final ImeFormula mImeFormula;
    
    /** The list of key codes that should be ignored. */
    private final List<Integer> mIgnoredCodes;
    
    /**
     * Instantiates a new key provider.
     * 
     * @param formula      The decoding formula to be used.
     * @param ignoredCodes List of key codes that should be ignored.
     */
    public KeyProvider( ImeFormula formula, List<Integer> ignoredCodes )
    {
        // Assign the fields
        mImeFormula = formula;
        mIgnoredCodes = ignoredCodes;
    }
    
    /**
     * Instantiates a new key provider.
     * 
     * @param view         The view receiving KeyEvent data.
     * @param formula      The decoding formula to be used.
     * @param ignoredCodes List of key codes that should be ignored.
     */
    public KeyProvider( View view, ImeFormula formula, List<Integer> ignoredCodes )
    {
        this( formula, ignoredCodes );
        
        // Connect the input source
        view.setOnKeyListener( this );
        
        // Request focus for proper listening
        view.requestFocus();
    }
    
    /**
     * Instantiates a new key provider.
     * 
     * @param builder      The builder for the dialog receiving KeyEvent data.
     * @param formula      The decoding formula to be used.
     * @param ignoredCodes List of key codes that should be ignored.
     */
    public KeyProvider( Builder builder, ImeFormula formula, List<Integer> ignoredCodes )
    {
        this( formula, ignoredCodes );
        
        // Connect the input source
        builder.setOnKeyListener( this );
    }
    
    /*
     * (non-Javadoc)
     * 
     * @see android.view.View.OnKeyListener#onKey(android.view.View, int, android.view.KeyEvent)
     */
    @Override
    public boolean onKey( View v, int keyCode, KeyEvent event )
    {
        return onKey( keyCode, event );
    }
    
    /*
     * (non-Javadoc)
     * 
     * @see android.content.DialogInterface.OnKeyListener#onKey(android.content.DialogInterface,
     * int, android.view.KeyEvent)
     */
    @Override
    public boolean onKey( DialogInterface dialog, int keyCode, KeyEvent event )
    {
        return onKey( keyCode, event );
    }
    
    /**
     * Manually dispatches a KeyEvent through the provider's listening chain.
     * 
     * @param keyCode The code for the physical key that was pressed.
     * @param event   The KeyEvent object containing full information about the event.
     * 
     * @return True if the listener has consumed the event, false otherwise.
     */
    public boolean onKey( int keyCode, KeyEvent event )
    {
        // Ignore specified key codes
        if( mIgnoredCodes != null && mIgnoredCodes.contains( keyCode ) )
        {
            return false;
        }
        
        // Translate input code and analog strength (ranges between 0.0 and 1.0)
        int inputCode;
        float strength;
        if( keyCode <= 0xFF )
        {
            // Ordinary key/button changed state
            inputCode = keyCode;
            strength = 1;
        }
        else
        {
            // Analog axis changed state, decode using IME-specific formula
            switch( mImeFormula )
            {
                case DEFAULT:
                case USB_BT_JOYSTICK_CENTER:
                case BT_CONTROLLER:
                default:
                    // Formula defined between paulscode and poke64738
                    inputCode = keyCode / 100;
                    strength = ( (float) keyCode % 100 ) / 64f;
                    break;
                case EXAMPLE_IME:
                    // Low byte stores input code, high byte stores strength
                    inputCode = keyCode & 0xFF;
                    strength = ( (float) ( keyCode >> 8 ) ) / 0xFF;
                    break;
            }
        }
        
        // Strength is zero when the button/axis is released
        if( event.getAction() == KeyEvent.ACTION_UP )
            strength = 0;
        
        // Notify listeners about new input data
        notifyListeners( inputCode, strength, getHardwareId( event ) );
        
        return true;
    }
}
