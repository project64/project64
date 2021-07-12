package emu.project64.input;

import java.util.ArrayList;

import android.util.Log;
import emu.project64.jni.NativeInput;

/**
 * The abstract base class for implementing all N64 controllers.
 * <p/>
 * Subclasses should implement the following pattern:
 * <ul>
 * <li>Register a listener to the upstream input (e.g. touch, keyboard, mouse, joystick, etc.).</li>
 * <li>Translate the input data into N64 controller button/axis states, and set the values of the
 * protected fields mState.buttons and mState.axisFraction* accordingly.</li>
 * <li>Call the protected method notifyChanged().</li>
 * </ul>
 * This abstract class will call the emulator's native libraries to update game state whenever
 * notifyChanged() is called. Subclasses should not call any native methods themselves. (If they do,
 * then this abstract class should be expanded to cover those needs.)
 * <p>
 * Note that this class is stateful, in that it remembers controller button/axis state between calls
 * from the subclass. For best performance, subclasses should only call notifyChanged() when the
 * input state has actually changed, and should bundle the protected field modifications before
 * calling notifyChanged(). For example,
 *
 * <pre>
 * {@code
 * buttons[0] = true; notifyChanged(); buttons[1] = false; notifyChanged(); // Inefficient
 * buttons[0] = true; buttons[1] = false; notifyChanged(); // Better
 * }
 * </pre>
 *
 * @see PeripheralController
 * @see TouchController
 */
public abstract class AbstractController
{
    protected final static boolean LOG_CONTROLLER = false;

    /**
     * A small class that encapsulates controller state.
     */
    protected static class State
    {
        /** The pressed state of each controller button. */
        public boolean[] buttons = new boolean[NUM_N64_BUTTONS];

        /** The fractional value of the analog-x axis, between -1 and 1, inclusive. */
        public float axisFractionX = 0;

        /** The fractional value of the analog-y axis, between -1 and 1, inclusive. */
        public float axisFractionY = 0;
    }

    // Constants must match EButton listing in plugin.h! (input-sdl plug-in)

    /** N64 button: dpad-right. */
    public static final int DPD_R = 0;

    /** N64 button: dpad-left. */
    public static final int DPD_L = 1;

    /** N64 button: dpad-down. */
    public static final int DPD_D = 2;

    /** N64 button: dpad-up. */
    public static final int DPD_U = 3;

    /** N64 button: start. */
    public static final int START = 4;

    /** N64 button: trigger-z. */
    public static final int BTN_Z = 5;

    /** N64 button: b. */
    public static final int BTN_B = 6;

    /** N64 button: a. */
    public static final int BTN_A = 7;

    /** N64 button: cpad-right. */
    public static final int CPD_R = 8;

    /** N64 button: cpad-left. */
    public static final int CPD_L = 9;

    /** N64 button: cpad-down. */
    public static final int CPD_D = 10;

    /** N64 button: cpad-up. */
    public static final int CPD_U = 11;

    /** N64 button: shoulder-r. */
    public static final int BTN_R = 12;

    /** N64 button: shoulder-l. */
    public static final int BTN_L = 13;

    /** N64 button: reserved-1. */
    public static final int BTN_RESERVED1 = 14;

    /** N64 button: reserved-2. */
    public static final int BTN_RESERVED2 = 15;

    /** Total number of N64 buttons. */
    public static final int NUM_N64_BUTTONS = 16;

    /** The state of all four player controllers. */
    private static final ArrayList<State> sStates = new ArrayList<State>();

    /** The state of this controller. */
    protected State mState;

    /** The player number, between 1 and 4, inclusive. */
    protected int mPlayerNumber = 1;

    /** The factor by which the axis fractions are scaled before going to the core. */
    private static final float AXIS_SCALE = 80;

    static
    {
        sStates.add( new State() );
        sStates.add( new State() );
        sStates.add( new State() );
        sStates.add( new State() );
    }

    /**
     * Instantiates a new abstract controller.
     */
    protected AbstractController()
    {
        mState = sStates.get( 0 );
    }

    /**
     * Notifies the core that the N64 controller state has changed.
     */
    protected void notifyChanged()
    {

        int axisX = Math.round( AXIS_SCALE * mState.axisFractionX );
        int axisY = Math.round( AXIS_SCALE * mState.axisFractionY );
        if (LOG_CONTROLLER)
        {
            Log.i("Controller", "notifyChanged: axisX=" + axisX + " axisY=" + axisY);
        }
        NativeInput.setState( mPlayerNumber - 1, mState.buttons, axisX, axisY );
    }

    /**
     * Gets the player number.
     *
     * @return The player number, between 1 and 4, inclusive.
     */
    public int getPlayerNumber()
    {
        return mPlayerNumber;
    }

    /**
     * Sets the player number.
     *
     * @param player The new player number, between 1 and 4, inclusive.
     */
    public void setPlayerNumber( int player )
    {
        mPlayerNumber = player;
        mState = sStates.get( mPlayerNumber - 1 );
    }
}
