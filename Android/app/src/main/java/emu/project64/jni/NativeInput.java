package emu.project64.jni;

/**
 * Calls made between the native input-android library and Java. Any function names changed here
 * should also be changed in the corresponding C code, and vice versa.
 * 
 * @see /Source/Android/PluginInput/Main.cpp
 * @see CoreInterface
 */
public class NativeInput
{
    static
    {
        System.loadLibrary( "Project64-input-android" );
    }
    
    /**
     * Set the button/axis state of a controller.
     * 
     * @param controllerNum Controller index, in the range [0,3].
     * @param buttons The pressed state of the buttons.
     * @param axisX The analog value of the x-axis, in the range [-80,80].
     * @param axisY The analog value of the y-axis, in the range [-80,80].
     */
    public static native void setState( int controllerNum, boolean[] buttons, int axisX, int axisY );    
}
