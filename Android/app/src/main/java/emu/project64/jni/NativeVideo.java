package emu.project64.jni;

public class NativeVideo
{
    static
    {
        System.loadLibrary( "Project64-video" );
    }
    
    public static native void UpdateScreenRes(int ScreenWidth, int ScreenHeight);
    public static native int getResolutionCount();
    public static native String getResolutionName(int Index);
    public static native int GetScreenResWidth(int Index);
    public static native int GetScreenResHeight(int Index);
}
