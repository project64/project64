package emu.project64.jni;

import android.app.Activity;
import emu.project64.game.GameSurface;

public class NativeExports
{
    static
    {
        System.loadLibrary( "Project64-bridge" );
    }
    
    public static native void appInit (String BaseDir );
    public static native String appVersion();
    public static native void StopEmulation();
    public static native void StartEmulation();
    public static native void CloseSystem();
    public static native void SettingsSaveBool(int type, boolean value);
    public static native void SettingsSaveDword(int type, int value);
    public static native void SettingsSaveString(int type, String value);
    public static native boolean SettingsLoadBool(int type);
    public static native int SettingsLoadDword(int type);
    public static native String SettingsLoadString(int type);
    public static native boolean IsSettingSet(int type);
    public static native void LoadGame(String FileLoc);
    public static native void StartGame(Activity activity, GameSurface.GLThread thread);
    public static native void LoadRomList();
    public static native void RefreshRomDir(String RomDir, boolean Recursive);
    public static native void ExternalEvent(int Type);
    public static native void ResetApplicationSettings();
    public static native byte[] GetString(int StringId);
	
    public static native void SetSpeed(int Speed);
    public static native int GetSpeed();
    public static native int GetBaseSpeed();
    
    public static native void onSurfaceCreated();    
    public static native void onSurfaceChanged(int width, int height); 
    public static native void onDrawFrame();
    
    public static native void UISettingsSaveBool(int Type, boolean Value);
    public static native void UISettingsSaveDword(int Type, int Value);
    public static native void UISettingsSaveString(int type, String value);

    public static native boolean UISettingsLoadBool(int Type);
    public static native int UISettingsLoadDword(int Type);
    public static native String UISettingsLoadString(int type);
    public static native String UISettingsLoadStringIndex(int Type, int Index);
}
