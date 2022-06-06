package emu.project64.jni;

public enum UISettingID
{		
    AssertsVersion,
    BuildVersion,
    ScreenOrientation,

    //Recent Game
    FileRecentGameFileCount,
    FileRecentGameFileIndex,

    //Touch Screen
    TouchScreenButtonScale,
    TouchScreenLayout,

    //Controller Config
    ControllerConfigFile,
    ControllerCurrentProfile,
    ControllerDeadzone,
    ControllerSensitivity,

    //App Info
    AppInfoRunCount,
    ;

    @Override
    public String toString() {
        return "UISettingID." + super.toString();
    }
}
