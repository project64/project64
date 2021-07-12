package emu.project64.input;

import java.util.Set;

import emu.project64.AndroidDevice;
import emu.project64.input.map.TouchMap;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.graphics.Point;
import android.os.Vibrator;
import android.util.SparseIntArray;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

/**
 * A class for generating N64 controller commands from a touchscreen.
 */
public class TouchController extends AbstractController implements OnTouchListener
{
    public interface OnStateChangedListener
    {
        /**
         * Called after the analog stick values have changed.
         * 
         * @param axisFractionX The x-axis fraction, between -1 and 1, inclusive.
         * @param axisFractionY The y-axis fraction, between -1 and 1, inclusive.
         */
        public void onAnalogChanged( float axisFractionX, float axisFractionY );
        
        /**
         * Called after auto-hold button state changed.
         * 
         * @param pressed The auto-hold state.
         * @param index The index of the auto-hold mask.
         */
        public void onAutoHold( boolean pressed, int index );
    }
    
    public static final int AUTOHOLD_METHOD_DISABLED = 0;
    public static final int AUTOHOLD_METHOD_LONGPRESS = 1;
    public static final int AUTOHOLD_METHOD_SLIDEOUT = 2;
    
    /** The number of milliseconds to wait before auto-holding (long-press method). */
    private static final int AUTOHOLD_LONGPRESS_TIME = 1000;
    
    /** The pattern vibration when auto-hold is engaged. */
    private static final long[] AUTOHOLD_VIBRATE_PATTERN = { 0, 50, 50, 50 };
    
    /** The number of milliseconds of vibration when pressing a key. */
    private static final int FEEDBACK_VIBRATE_TIME = 50;
    
    /** The maximum number of pointers to query. */
    private static final int MAX_POINTER_IDS = 256;
    
    /** The state change listener. */
    private final OnStateChangedListener mListener;
    
    /** The map from screen coordinates to N64 controls. */
    private final TouchMap mTouchMap;
    
    /** The map from pointer ids to N64 controls. */
    private final SparseIntArray mPointerMap = new SparseIntArray();
    
    /** The method used for auto-holding buttons. */
    private final int mAutoHoldMethod;
    
    /** The set of auto-holdable buttons. */
    private final Set<Integer> mAutoHoldables;
    
    /** Whether touchscreen feedback is enabled. */
    private final boolean mTouchscreenFeedback;
    
    /** The touch state of each pointer. True indicates down, false indicates up. */
    private final boolean[] mTouchState = new boolean[MAX_POINTER_IDS];
    
    /** The x-coordinate of each pointer, between 0 and (screenwidth-1), inclusive. */
    private final int[] mPointerX = new int[MAX_POINTER_IDS];
    
    /** The y-coordinate of each pointer, between 0 and (screenheight-1), inclusive. */
    private final int[] mPointerY = new int[MAX_POINTER_IDS];
    
    /** The pressed start time of each pointer. */
    private final long[] mStartTime = new long[MAX_POINTER_IDS];
    
    /** The time between press and release of each pointer. */
    private final long[] mElapsedTime = new long[MAX_POINTER_IDS];
    
    /**
     * The identifier of the pointer associated with the analog stick. -1 indicates the stick has
     * been released.
     */
    private int mAnalogPid = -1;
    
    /** The touch event source to listen to, or 0 to listen to all sources. */
    private int mSourceFilter = 0;
    
    private Vibrator mVibrator = null;
    
    /**
     * Instantiates a new touch controller.
     * 
     * @param touchMap            The map from touch coordinates to N64 controls.
     * @param view                The view receiving touch event data.
     * @param listener            The listener for controller state changes.
     * @param vibrator            The haptic feedback device. MUST BE NULL if vibrate permission not granted.
     * @param autoHoldMethod      The method for auto-holding buttons.
     * @param touchscreenFeedback True if haptic feedback should be used.
     * @param autoHoldableButtons The N64 commands that correspond to auto-holdable buttons.
     */
    public TouchController( TouchMap touchMap, View view, OnStateChangedListener listener,
            Vibrator vibrator, int autoHoldMethod, boolean touchscreenFeedback,
            Set<Integer> autoHoldableButtons )
    {
        mListener = listener;
        mTouchMap = touchMap;
        mVibrator = vibrator;
        mAutoHoldMethod = autoHoldMethod;
        mTouchscreenFeedback = touchscreenFeedback;
        mAutoHoldables = autoHoldableButtons;
        view.setOnTouchListener( this );
    }
    
    /**
     * Sets the touch event source filter.
     * 
     * @param source The source to listen to, or 0 to listen to all sources.
     */
    public void setSourceFilter( int source )
    {
        mSourceFilter = source;
    }
    
    /*
     * (non-Javadoc)
     * 
     * @see android.view.View.OnTouchListener#onTouch(android.view.View, android.view.MotionEvent)
     */
    @SuppressLint( "ClickableViewAccessibility" )
    @Override
    @TargetApi( 9 )
    public boolean onTouch( View view, MotionEvent event )
    {
        // Filter by source, if applicable
        int source = AndroidDevice.IS_GINGERBREAD ? event.getSource() : 0;
        if( mSourceFilter != 0 && mSourceFilter != source )
            return false;
        
        int action = event.getAction();
        int actionCode = action & MotionEvent.ACTION_MASK;
        
        int pid = -1;
        switch( actionCode )
        {
            case MotionEvent.ACTION_POINTER_DOWN:
                // A non-primary touch has been made
                pid = event.getPointerId( action >> MotionEvent.ACTION_POINTER_INDEX_SHIFT );
                mStartTime[pid] = System.currentTimeMillis();
                mTouchState[pid] = true;
                break;
            case MotionEvent.ACTION_POINTER_UP:
                // A non-primary touch has been released
                pid = event.getPointerId( action >> MotionEvent.ACTION_POINTER_INDEX_SHIFT );
                mElapsedTime[pid] = System.currentTimeMillis() - mStartTime[pid];
                mTouchState[pid] = false;
                break;
            case MotionEvent.ACTION_DOWN:
                // A touch gesture has started (e.g. analog stick movement)
                for( int i = 0; i < event.getPointerCount(); i++ )
                {
                    pid = event.getPointerId( i );
                    mStartTime[pid] = System.currentTimeMillis();
                    mTouchState[pid] = true;
                }
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                // A touch gesture has ended or canceled (e.g. analog stick movement)
                for( int i = 0; i < event.getPointerCount(); i++ )
                {
                    pid = event.getPointerId( i );
                    mElapsedTime[pid] = System.currentTimeMillis() - mStartTime[pid];
                    mTouchState[pid] = false;
                }
                break;
            default:
                break;
        }
        
        // Update the coordinates of down pointers and record max PID for speed
        int maxPid = -1;
        for( int i = 0; i < event.getPointerCount(); i++ )
        {
            pid = event.getPointerId( i );
            if( pid > maxPid )
                maxPid = pid;
            if( mTouchState[pid] )
            {
                mPointerX[pid] = (int) event.getX( i );
                mPointerY[pid] = (int) event.getY( i );
            }
        }
        
        // Process each touch
        processTouches( mTouchState, mPointerX, mPointerY, mElapsedTime, maxPid );
        
        return true;
    }
    
    /**
     * Sets the N64 controller state based on where the screen is (multi-) touched. Values outside
     * the ranges listed below are safe.
     * 
     * @param touchstate The touch state of each pointer. True indicates down, false indicates up.
     * @param pointerX   The x-coordinate of each pointer, between 0 and (screenwidth-1), inclusive.
     * @param pointerY   The y-coordinate of each pointer, between 0 and (screenheight-1), inclusive.
     * @param maxPid     Maximum ID of the pointers that have changed (speed optimization).
     */
    private void processTouches( boolean[] touchstate, int[] pointerX, int[] pointerY,
            long[] elapsedTime, int maxPid )
    {
        boolean analogMoved = false;
        
        // Process each pointer in sequence
        for( int pid = 0; pid <= maxPid; pid++ )
        {
            // Release analog if its pointer is not touching the screen
            if( pid == mAnalogPid && !touchstate[pid] )
            {
                analogMoved = true;
                mAnalogPid = -1;
                mState.axisFractionX = 0;
                mState.axisFractionY = 0;
            }
            
            // Process button inputs
            if( pid != mAnalogPid )
                processButtonTouch( touchstate[pid], pointerX[pid], pointerY[pid],
                        elapsedTime[pid], pid );
            
            // Process analog inputs
            if( touchstate[pid] && processAnalogTouch( pid, pointerX[pid], pointerY[pid] ) )
                analogMoved = true;
        }
        
        // Call the super method to send the input to the core
        notifyChanged();
        
        // Update the skin if the virtual analog stick moved
        if( analogMoved && mListener != null )
            mListener.onAnalogChanged( mState.axisFractionX, mState.axisFractionY );
    }
    
    /**
     * Process a touch as if intended for a button. Values outside the ranges listed below are safe.
     * 
     * @param touched   Whether the button is pressed or not.
     * @param xLocation The x-coordinate of the touch, between 0 and (screenwidth-1), inclusive.
     * @param yLocation The y-coordinate of the touch, between 0 and (screenheight-1), inclusive.
     * @param pid       The identifier of the touch pointer.
     */
    private void processButtonTouch( boolean touched, int xLocation, int yLocation,
            long timeElapsed, int pid )
    {
        // Determine the index of the button that was pressed
        int index = touched
                ? mTouchMap.getButtonPress( xLocation, yLocation )
                : mPointerMap.get( pid, TouchMap.UNMAPPED );
                
        // Update the pointer map
        if( !touched )
        {
            // Finger lifted off screen, forget what this pointer was touching
            mPointerMap.delete( pid );
        }
        else
        {
            // Determine where the finger came from if is was slid
            int prevIndex = mPointerMap.get( pid, TouchMap.UNMAPPED );
            
            // Finger touched somewhere on screen, remember what this pointer is touching
            mPointerMap.put( pid, index );
            
            if( prevIndex != index )
            {
                // Finger slid from somewhere else, act accordingly
                // There are three possibilities:
                // - old button --> new button
                // - nothing --> new button
                // - old button --> nothing
                
                // Reset this pointer's start time
                mStartTime[pid] = System.currentTimeMillis();
                
                if( prevIndex != TouchMap.UNMAPPED )
                {
                    // Slid off a valid button
                    if( !isAutoHoldable( prevIndex ) || mAutoHoldMethod == AUTOHOLD_METHOD_DISABLED )
                    {
                        // Slid off a non-auto-hold button
                        setTouchState( prevIndex, false );
                    }
                    else
                    {
                        // Slid off an auto-hold button
                        switch( mAutoHoldMethod )
                        {
                            case AUTOHOLD_METHOD_LONGPRESS:
                                // Using long-press method, release auto-hold button
                                if( mListener != null )
                                    mListener.onAutoHold( false, prevIndex );
                                setTouchState( prevIndex, false );
                                break;
                            
                            case AUTOHOLD_METHOD_SLIDEOUT:
                                // Using slide-off method, engage auto-hold button
                                if( mVibrator != null )
                                {
                                    mVibrator.cancel();
                                    mVibrator.vibrate( AUTOHOLD_VIBRATE_PATTERN, -1 );
                                }
                                if( mListener != null )
                                    mListener.onAutoHold( true, prevIndex );
                                setTouchState( prevIndex, true );
                                break;
                        }
                    }
                }
            }
        }
        
        if( index != TouchMap.UNMAPPED )
        {
            // Finger is on a valid button
            
            // Provide simple vibration feedback for any valid button when first touched
            if( touched && mTouchscreenFeedback && mVibrator != null )
            {
                boolean firstTouched;
                if( index < NUM_N64_BUTTONS )
                {
                    // Single button pressed
                    firstTouched = !mState.buttons[index];
                }
                else
                {
                    // Two d-pad buttons pressed simultaneously
                    switch( index )
                    {
                        case TouchMap.DPD_RU:
                            firstTouched = !( mState.buttons[DPD_R] && mState.buttons[DPD_U] );
                            break;
                        case TouchMap.DPD_RD:
                            firstTouched = !( mState.buttons[DPD_R] && mState.buttons[DPD_D] );
                            break;
                        case TouchMap.DPD_LD:
                            firstTouched = !( mState.buttons[DPD_L] && mState.buttons[DPD_D] );
                            break;
                        case TouchMap.DPD_LU:
                            firstTouched = !( mState.buttons[DPD_L] && mState.buttons[DPD_U] );
                            break;
                        default:
                            firstTouched = false;
                            break;
                    }
                }
                
                if( firstTouched )
                {
                    mVibrator.cancel();
                    mVibrator.vibrate( FEEDBACK_VIBRATE_TIME );
                }
            }
            
            // Set the controller state accordingly
            if( touched || !isAutoHoldable( index ) || mAutoHoldMethod == AUTOHOLD_METHOD_DISABLED )
            {
                // Finger just touched a button (any kind) OR
                // Finger just lifted off non-auto-holdable button
                setTouchState( index, touched );
                // Do not provide auto-hold feedback yet
            }
            else
            {
                // Finger just lifted off an auto-holdable button
                switch( mAutoHoldMethod )
                {
                    case AUTOHOLD_METHOD_SLIDEOUT:
                        // Release auto-hold button if using slide-off method
                        if( mListener != null )
                            mListener.onAutoHold( false, index );
                        setTouchState( index, false );
                        break;
                    
                    case AUTOHOLD_METHOD_LONGPRESS:
                        if( timeElapsed < AUTOHOLD_LONGPRESS_TIME )
                        {
                            // Release auto-hold if short-pressed
                            if( mListener != null )
                                mListener.onAutoHold( false, index );
                            setTouchState( index, false );
                        }
                        else
                        {
                            // Engage auto-hold if long-pressed
                            if( mVibrator != null )
                            {
                                mVibrator.cancel();
                                mVibrator.vibrate( AUTOHOLD_VIBRATE_PATTERN, -1 );
                            }
                            if( mListener != null )
                                mListener.onAutoHold( true, index );
                            setTouchState( index, true );
                        }
                        break;
                }
            }
        }
    }
    
    /**
     * Checks if the button mapped to an N64 command is auto-holdable.
     * 
     * @param commandIndex The index to the N64 command.
     * 
     * @return True if the button mapped to the command is auto-holdable.
     */
    private boolean isAutoHoldable( int commandIndex )
    {
        return mAutoHoldables != null && mAutoHoldables.contains( commandIndex );
    }
    
    /**
     * Sets the state of a button, and handles the D-Pad diagonals.
     * 
     * @param index   Which button is affected.
     * @param touched Whether the button is pressed or not.
     */
    private void setTouchState( int index, boolean touched )
    {
        // Set the button state
        if( index < AbstractController.NUM_N64_BUTTONS )
        {
            // A single button was pressed
            mState.buttons[index] = touched;
        }
        else
        {
            // Two d-pad buttons pressed simultaneously
            switch( index )
            {
                case TouchMap.DPD_RU:
                    mState.buttons[DPD_R] = touched;
                    mState.buttons[DPD_U] = touched;
                    break;
                case TouchMap.DPD_RD:
                    mState.buttons[DPD_R] = touched;
                    mState.buttons[DPD_D] = touched;
                    break;
                case TouchMap.DPD_LD:
                    mState.buttons[DPD_L] = touched;
                    mState.buttons[DPD_D] = touched;
                    break;
                case TouchMap.DPD_LU:
                    mState.buttons[DPD_L] = touched;
                    mState.buttons[DPD_U] = touched;
                    break;
                default:
                    break;
            }
        }
    }
    
    /**
     * Process a touch as if intended for the analog stick. Values outside the ranges listed below
     * are safe.
     * 
     * @param pointerId The pointer identifier.
     * @param xLocation The x-coordinate of the touch, between 0 and (screenwidth-1), inclusive.
     * @param yLocation The y-coordinate of the touch, between 0 and (screenheight-1), inclusive.
     * 
     * @return True, if the analog state changed.
     */
    private boolean processAnalogTouch( int pointerId, int xLocation, int yLocation )
    {
        // Get the cartesian displacement of the analog stick
        Point point = mTouchMap.getAnalogDisplacement( xLocation, yLocation );
        
        // Compute the pythagorean displacement of the stick
        int dX = point.x;
        int dY = point.y;
        float displacement = (float) Math.sqrt( ( dX * dX ) + ( dY * dY ) );
        
        // "Capture" the analog control
        if( mTouchMap.isInCaptureRange( displacement ) )
            mAnalogPid = pointerId;
        
        if( pointerId == mAnalogPid )
        {
            // User is controlling the analog stick
            
            // Limit range of motion to an octagon (like the real N64 controller)
            point = mTouchMap.getConstrainedDisplacement( dX, dY );
            dX = point.x;
            dY = point.y;
            displacement = (float) Math.sqrt( ( dX * dX ) + ( dY * dY ) );
            
            // Fraction of full-throttle, between 0 and 1, inclusive
            float p = mTouchMap.getAnalogStrength( displacement );
            
            // Store the axis values in the super fields (screen y is inverted)
            mState.axisFractionX = p * dX / displacement;
            mState.axisFractionY = -p * dY / displacement;
            
            // Analog state changed
            return true;
        }
        
        // Analog state did not change
        return false;
    }
}
