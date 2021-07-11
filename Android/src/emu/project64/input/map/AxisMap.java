package emu.project64.input.map;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import android.annotation.TargetApi;
import android.text.TextUtils;
import android.util.SparseArray;
import android.view.InputDevice;
import android.view.InputDevice.MotionRange;
import android.view.MotionEvent;

@TargetApi( 9 )
public class AxisMap extends SerializableMap
{
    public static final int AXIS_CLASS_UNKNOWN = 0;
    public static final int AXIS_CLASS_IGNORED = 1;
    public static final int AXIS_CLASS_STICK = 2;
    public static final int AXIS_CLASS_TRIGGER = 3;
    public static final int AXIS_CLASS_N64_USB_STICK = 102;
    
    private static final int SIGNATURE_HASH_XBOX360 = 449832952;
    private static final int SIGNATURE_HASH_XBOX360_WIRELESS = -412618953;
    private static final int SIGNATURE_HASH_PS3 = -528816963;
    private static final int SIGNATURE_HASH_LOGITECH_WINGMAN_RUMBLEPAD = 1247256123;
    private static final int SIGNATURE_HASH_MOGA_PRO = -1933523749;
    private static final int SIGNATURE_HASH_OUYA = 699487739;
    private static final int SIGNATURE_HASH_AMAZON_FIRE = 2050752785;
    
    private static final SparseArray<AxisMap> sAllMaps = new SparseArray<AxisMap>();
    private final String mSignature;
    private final String mSignatureName;
    
    public static AxisMap getMap( InputDevice device )
    {
        if( device == null )
            return null;
        
        int id = device.hashCode();
        AxisMap map = sAllMaps.get( id );
        if( map == null )
        {
            // Add an entry to the map if not found
            map = new AxisMap( device );
            sAllMaps.put( id, map );
        }
        return map;
    }
    
    @TargetApi( 12 )
    public AxisMap( InputDevice device )
    {
        // Auto-classify the axes
        List<MotionRange> motionRanges = device.getMotionRanges();
        List<Integer> axisCodes = new ArrayList<Integer>();
        for( MotionRange motionRange : motionRanges )
        {
            if( motionRange.getSource() == InputDevice.SOURCE_JOYSTICK )
            {
                int axisCode = motionRange.getAxis();
                int axisClass = detectClass( motionRange );
                setClass( axisCode, axisClass );
                axisCodes.add( axisCode );
            }
        }
        
        // Construct the signature based on the available axes
        Collections.sort( axisCodes );
        mSignature = TextUtils.join( ",", axisCodes );
        String signatureName = "Default";
        String deviceName = device.getName();
        
        // Use the signature to override faulty auto-classifications
        switch( mSignature.hashCode() )
        {
            case SIGNATURE_HASH_XBOX360:
                // Resting value is -1 on the analog triggers; fix that
                if( deviceName.contains( "Sony Computer Entertainment Wireless Controller" ) )
                {
                    // Note that the PS4 controller uses the same axes but uses different ones for
                    // the triggers, so we have to differentiate them.
                    setClass( MotionEvent.AXIS_RX, AXIS_CLASS_TRIGGER );
                    setClass( MotionEvent.AXIS_RY, AXIS_CLASS_TRIGGER );
                    signatureName = "PS4 compatible";
                }
                else
                {
                    setClass( MotionEvent.AXIS_Z, AXIS_CLASS_TRIGGER );
                    setClass( MotionEvent.AXIS_RZ, AXIS_CLASS_TRIGGER );
                    signatureName = "Xbox 360 compatible";
                }
                break;
            
            case SIGNATURE_HASH_XBOX360_WIRELESS:
                // Resting value is -1 on the analog triggers; fix that
                setClass( MotionEvent.AXIS_Z, AXIS_CLASS_TRIGGER );
                setClass( MotionEvent.AXIS_RZ, AXIS_CLASS_TRIGGER );
                signatureName = "Xbox 360 wireless";
                break;
            
            case SIGNATURE_HASH_PS3:
                // Ignore pressure sensitive buttons (buggy on Android)
                setClass( MotionEvent.AXIS_GENERIC_1, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_2, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_3, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_4, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_5, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_6, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_7, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_8, AXIS_CLASS_IGNORED );
                signatureName = "PS3 compatible";
                break;
            
            case SIGNATURE_HASH_LOGITECH_WINGMAN_RUMBLEPAD:
                // Bug in controller firmware cross-wires throttle and right stick up/down
                setClass( MotionEvent.AXIS_THROTTLE, AXIS_CLASS_STICK );
                signatureName = "Logitech Wingman Rumblepad";
                break;
            
            case SIGNATURE_HASH_MOGA_PRO:
                // Ignore two spurious axes for MOGA Pro in HID (B) mode (as of MOGA Pivot v1.15)
                // http://www.paulscode.com/forum/index.php?topic=581.msg10094#msg10094
                setClass( MotionEvent.AXIS_GENERIC_1, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_2, AXIS_CLASS_IGNORED );
                signatureName = "Moga Pro (HID mode)";
                break;
                
            case SIGNATURE_HASH_OUYA:
                // Ignore phantom triggers
                setClass( MotionEvent.AXIS_GENERIC_1, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_2, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_3, AXIS_CLASS_IGNORED );
                setClass( MotionEvent.AXIS_GENERIC_4, AXIS_CLASS_IGNORED );
                signatureName = "OUYA controller";
                break;
                
            case SIGNATURE_HASH_AMAZON_FIRE:
                // Ignore floating generic axis
                setClass( MotionEvent.AXIS_GENERIC_1, AXIS_CLASS_IGNORED );
                signatureName = "Amazon Fire Game Controller";
                break;
        }
        
        // Check if the controller is an N64/USB adapter, to compensate for range of motion
        if( deviceName.contains( "raphnet.net GC/N64_USB" ) ||
            deviceName.contains( "raphnet.net GC/N64 to USB, v2" ) ||
            deviceName.contains( "HuiJia  USB GamePad" ) ) // double space is not a typo
        {
            setClass( MotionEvent.AXIS_X, AXIS_CLASS_N64_USB_STICK );
            setClass( MotionEvent.AXIS_Y, AXIS_CLASS_N64_USB_STICK );
            signatureName = "N64 USB adapter";
        }
        
        mSignatureName = signatureName;
    }
    
    public void setClass( int axisCode, int axisClass )
    {
        if( axisClass == AXIS_CLASS_UNKNOWN )
            mMap.delete( axisCode );
        else
            mMap.put( axisCode, axisClass );
    }
    
    public int getClass( int axisCode )
    {
        return mMap.get( axisCode );
    }
    
    public String getSignature()
    {
        return mSignature;
    }
    
    public String getSignatureName()
    {
        return mSignatureName;
    }
    
    @TargetApi( 12 )
    private static int detectClass( MotionRange motionRange )
    {
        if( motionRange != null )
        {
            if( motionRange.getMin() == -1 )
                return AXIS_CLASS_STICK;
            else if( motionRange.getMin() == 0 )
                return AXIS_CLASS_TRIGGER;
        }
        return AXIS_CLASS_UNKNOWN;
    }
}
