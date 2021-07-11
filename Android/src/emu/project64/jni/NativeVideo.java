package emu.project64.jni;

/**
 * Calls made between the native input-android library and Java. Any function names changed here
 * should also be changed in the corresponding C code, and vice versa.
 * 
 * @see /Source/Android/PluginInput/Main.cpp
 * @see CoreInterface
 */
public class NativeVideo
{
    static
    {
        System.loadLibrary( "Project64-gfx" );
    }
    
    public static native void UpdateScreenRes(int ScreenWidth, int ScreenHeight);
    public static native int getResolutionCount();
    public static native String getResolutionName(int Index);
    public static native int GetScreenResWidth(int Index);
    public static native int GetScreenResHeight(int Index);
}
