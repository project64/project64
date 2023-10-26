package emu.project64.jni;

import android.app.Activity;
import emu.project64.game.GameSurface;

public class NativeExports
{
    static
    {
        System.loadLibrary( "Project64-bridge" );
    }
    
    public static native boolean appInit (String BaseDir );
    public static native String appVersion();
    public static native void StopEmulation();
    public static native void StartEmulation();
    public static native void CloseSystem();
    public static native void SettingsSaveBool(String Type, boolean Value);
    public static native void SettingsSaveDword(String Type, int Value);
    public static native void SettingsSaveString(String Type, String Value);
    public static native boolean SettingsLoadBool(String Type);
    public static native int SettingsLoadDword(String Type);
    public static native String SettingsLoadString(String Type);
    public static native String SettingsLoadStringIndex(String Type, int Index);
    public static native boolean IsSettingSet(String Type);
    public static native void LoadGame(String FileLoc);
    public static native void StartGame(Activity activity, GameSurface.GLThread thread);
    public static native void LoadRomList();
    public static native void RefreshRomDir(String RomDir, boolean Recursive);
    public static native void ExternalEvent(int Type);
    public static native byte[] GetString(int StringId);
	
    public static native void SetSpeed(int Speed);
    public static native int GetSpeed();
    public static native int GetBaseSpeed();
    
    public static native void onSurfaceCreated();    
    public static native void onSurfaceChanged(int width, int height); 
    public static native void onDrawFrame();
}
