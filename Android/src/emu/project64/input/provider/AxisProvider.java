package emu.project64.input.provider;

import emu.project64.AndroidDevice;
import emu.project64.input.map.AxisMap;
import android.annotation.TargetApi;
import android.view.InputDevice;
import android.view.InputDevice.MotionRange;
import android.view.MotionEvent;
import android.view.View;

/**
 * A class for transforming Android MotionEvent inputs into a common format.
 */
public class AxisProvider extends AbstractProvider
{
    /** The input codes to listen for. */
    private int[] mInputCodes;
    
    /** The default number of input codes to listen for. */
    private static final int DEFAULT_NUM_INPUTS = 128;
    
    /**
     * Instantiates a new axis provider.
     */
    @TargetApi( 12 )
    public AxisProvider()
    {
        // By default, provide data from all possible axes
        mInputCodes = new int[DEFAULT_NUM_INPUTS];
        for( int i = 0; i < mInputCodes.length; i++ )
            mInputCodes[i] = -( i + 1 );
    }
    
    /**
     * Instantiates a new axis provider.
     * 
     * @param view The view receiving MotionEvent data.
     */
    @TargetApi( 12 )
    public AxisProvider( View view )
    {
        this();
        
        // Connect the input source
        view.setOnGenericMotionListener( new GenericMotionListener() );
        
        // Request focus for proper listening
        view.requestFocus();
    }
    
    /**
     * Restricts listening to a set of universal input codes.
     * 
     * @param inputCodeFilter The new input codes to listen for.
     */
    public void setInputCodeFilter( int[] inputCodeFilter )
    {
        mInputCodes = inputCodeFilter.clone();
    }
    
    /**
     * Manually dispatches a MotionEvent through the provider's listening chain.
     * 
     * @param event The MotionEvent object containing full information about the event.
     * 
     * @return True if the listener has consumed the event, false otherwise.
     */
    public boolean onGenericMotion( MotionEvent event )
    {
        if( AndroidDevice.IS_HONEYCOMB_MR1 )
            return new GenericMotionListener().onGenericMotion( null, event );
        else
            return false;
    }
    
    /**
     * Just an indirection class that eliminates some logcat chatter about benign errors. If we make
     * the parent class implement View.OnGenericMotionListener, then we get logcat error messages
     * <i>even if</i> we conditionally exclude calls to the class based on API. These errors are not
     * actually harmful, so the logcat messages are simply a nuisance during debugging.
     * <p/>
     * For a detailed explanation, see <a href=http://stackoverflow.com/questions/13103902/
     * android-recommended-way-of-safely-supporting-newer-apis-has-error-if-the-class-i>here</a>.
     */
    @TargetApi( 12 )
    public class GenericMotionListener implements View.OnGenericMotionListener
    {
        /*
         * (non-Javadoc)
         * 
         * @see android.view.View.OnGenericMotionListener#onGenericMotion(android.view.View,
         * android.view.MotionEvent)
         */
        @Override
        public boolean onGenericMotion( View v, MotionEvent event )
        {
            // Ignore motion events from non-joysticks (mice are a problem)
            if( event.getSource() != InputDevice.SOURCE_JOYSTICK )
                return false;
            
            InputDevice device = event.getDevice();
            AxisMap axisInfo = AxisMap.getMap( device );
            
            // Read all the requested axes
            float[] strengths = new float[mInputCodes.length];
            for( int i = 0; i < mInputCodes.length; i++ )
            {
                int inputCode = mInputCodes[i];
                
                // Compute the axis code from the input code
                int axisCode = inputToAxisCode( inputCode );
                
                // Get the analog value using the Android API
                float strength = event.getAxisValue( axisCode );
                
                // Modify strength if necessary
                strength = normalizeStrength( strength, axisInfo, device, axisCode );
                
                // If the strength points in the correct direction, record it
                boolean direction1 = inputToAxisDirection( inputCode );
                boolean direction2 = strength > 0;
                if( direction1 == direction2 )
                    strengths[i] = Math.abs( strength );
                else
                    strengths[i] = 0;
            }
            
            // Notify listeners about new input data
            notifyListeners( mInputCodes, strengths, getHardwareId( event ) );
            
            return true;
        }
        
        private float normalizeStrength( float strength, AxisMap axisInfo, InputDevice device,
                int axisCode )
        {
            if( axisInfo != null )
            {
                int axisClass = axisInfo.getClass( axisCode );
                
                if( axisClass == AxisMap.AXIS_CLASS_IGNORED )
                {
                    // We should ignore this axis
                    strength = 0;
                }
                else if( device != null )
                {
                    // We should normalize this axis
                    MotionRange motionRange = device.getMotionRange( axisCode, InputDevice.SOURCE_JOYSTICK );
                    if( motionRange != null )
                    {
                        switch( axisClass )
                        {
                            case AxisMap.AXIS_CLASS_STICK:
                                // Normalize to [-1,1]
                                strength = ( strength - motionRange.getMin() ) / motionRange.getRange() * 2f - 1f;
                                break;
                            case AxisMap.AXIS_CLASS_TRIGGER:
                                // Normalize to [0,1]
                                strength = ( strength - motionRange.getMin() ) / motionRange.getRange();
                                break;
                            case AxisMap.AXIS_CLASS_N64_USB_STICK:
                                // Normalize to [-1,1]
                                // The Raphnet adapters through v2.x and some other USB adapters assume the N64
                                // controller produces values in the range [-127,127].  However, the official N64 spec
                                // says that raw values of +/- 80 indicate full strength.  Therefore we rescale by
                                // multiplying by 127/80 (dividing by 0.63).
                                // http://naesten.dyndns.org:8080/psyq/man/os/osContGetReadData.html
                                // http://raphnet-tech.com/products/gc_n64_usb_adapters/
                                strength = strength / 0.63f;
                                break;
                            case AxisMap.AXIS_CLASS_UNKNOWN:
                            default:
                                // Do nothing
                        }
                    }
                }
            }
            return strength;
        }
    }
}
