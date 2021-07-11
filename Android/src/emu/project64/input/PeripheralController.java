package emu.project64.input;

import java.util.ArrayList;

import emu.project64.input.map.InputMap;
import emu.project64.input.provider.AbstractProvider;
import emu.project64.util.Utility;
import android.annotation.TargetApi;
import android.util.FloatMath;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;

/**
 * A class for generating N64 controller commands from peripheral hardware (gamepads, joysticks,
 * keyboards, mice, etc.).
 */
public class PeripheralController extends AbstractController implements
        AbstractProvider.OnInputListener
{    
    /** The map from input codes to commands. */
    private final InputMap mInputMap;
    
    /** The analog deadzone, between 0 and 1, inclusive. */
    private final float mDeadzoneFraction;
    
    /** The analog sensitivity, the amount by which to scale stick values, nominally 1. */
    private final float mSensitivityFraction;
    
    /** The user input providers. */
    private final ArrayList<AbstractProvider> mProviders;
    
    /** The positive analog-x strength, between 0 and 1, inclusive. */
    private float mStrengthXpos;
    
    /** The negative analog-x strength, between 0 and 1, inclusive. */
    private float mStrengthXneg;
    
    /** The positive analog-y strength, between 0 and 1, inclusive. */
    private float mStrengthYpos;
    
    /** The negative analogy-y strength, between 0 and 1, inclusive. */
    private float mStrengthYneg;
    
    /**
     * Instantiates a new peripheral controller.
     * 
     * @param player    The player number, between 1 and 4, inclusive.
     * @param inputMap  The map from input codes to commands.
     * @param inputDeadzone The analog deadzone in percent.
     * @param inputSensitivity The analog sensitivity in percent.
     * @param providers The user input providers. Null elements are safe.
     */
    public PeripheralController( int player, InputMap inputMap, int inputDeadzone, int inputSensitivity, AbstractProvider... providers )
    {
        setPlayerNumber( player );
        
        // Assign the maps
        mInputMap = inputMap;
        mDeadzoneFraction = ( (float) inputDeadzone ) / 100f;
        mSensitivityFraction = ( (float) inputSensitivity ) / 100f;
        
        // Assign the non-null input providers
        mProviders = new ArrayList<AbstractProvider>();
        for( AbstractProvider provider : providers )
        {
            if( provider != null )
            {
                mProviders.add( provider );
                provider.registerListener( this );
            }
        }
    }
    
    @TargetApi( 16 )
    @Override
    public void onInput( int inputCode, float strength, int hardwareId )
    {
        // Apply user changes to the controller state
        apply( inputCode, strength );
        
        // Notify the core that controller state has changed
        notifyChanged();
    }
    
    @Override
    public void onInput( int[] inputCodes, float[] strengths, int hardwareId )
    {
        // Apply user changes to the controller state
        for( int i = 0; i < inputCodes.length; i++ )
            apply( inputCodes[i], strengths[i] );
        
        // Notify the core that controller state has changed
        notifyChanged();
    }
    
    /**
     * Apply user input to the N64 controller state.
     * 
     * @param inputCode The universal input code that was dispatched.
     * @param strength  The input strength, between 0 and 1, inclusive.
     * 
     * @return True, if controller state changed.
     */
    private boolean apply( int inputCode, float strength )
    {
        boolean keyDown = strength > AbstractProvider.STRENGTH_THRESHOLD;
        int n64Index = mInputMap.get( inputCode );
        
        if( n64Index >= 0 && n64Index < NUM_N64_BUTTONS )
        {
            mState.buttons[n64Index] = keyDown;
            return true;
        }
        else if( n64Index < InputMap.NUM_N64_CONTROLS )
        {
            switch( n64Index )
            {
                case InputMap.AXIS_R:
                    mStrengthXpos = strength;
                    break;
                case InputMap.AXIS_L:
                    mStrengthXneg = strength;
                    break;
                case InputMap.AXIS_D:
                    mStrengthYneg = strength;
                    break;
                case InputMap.AXIS_U:
                    mStrengthYpos = strength;
                    break;
                default:
                    return false;
            }
            
            // Calculate the net position of the analog stick
            float rawX = mSensitivityFraction * ( mStrengthXpos - mStrengthXneg );
            float rawY = mSensitivityFraction * ( mStrengthYpos - mStrengthYneg );
            float magnitude = (float) Math.sqrt( ( rawX * rawX ) + ( rawY * rawY ) );
            
            // Update controller state
            if( magnitude > mDeadzoneFraction )
            {
                // Normalize the vector
                float normalizedX = rawX / magnitude;
                float normalizedY = rawY / magnitude;

                // Rescale strength to account for deadzone
                magnitude = ( magnitude - mDeadzoneFraction ) / ( 1f - mDeadzoneFraction );
				magnitude = Utility.clamp( magnitude, 0f, 1f );
                mState.axisFractionX = normalizedX * magnitude;
                mState.axisFractionY = normalizedY * magnitude;
            }
            else
            {
                // In the deadzone 
                mState.axisFractionX = 0;
                mState.axisFractionY = 0;
            }
        }
        return false;
    }
}
