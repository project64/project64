#include <Settings/Settings.h>
#include <Common\StdString.h>
#include "InputSettingsID.h"
#include "InputSettings.h"

CInputSettings * g_Settings = nullptr;

static char * Control0_U_DPAD_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} 17 0 5";
static char * Control0_D_DPAD_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} 25 0 5";
static char * Control0_L_DPAD_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} 24 0 5";
static char * Control0_R_DPAD_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} 26 0 5";
static char * Control0_A_BUTTON_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} 2D 0 5";
static char * Control0_B_BUTTON_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} 2E 0 5";
static char * Control0_U_CBUTTON_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} C7 0 5";
static char * Control0_D_CBUTTON_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} CF 0 5";
static char * Control0_L_CBUTTON_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} D1 0 5";
static char * Control0_R_CBUTTON_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} D3 0 5";
static char * Control0_START_BUTTON_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} 1C 0 5";
static char * Control0_Z_TRIG_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} 2C 0 5";
static char * Control0_R_TRIG_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} 1F 0 5";
static char * Control0_L_TRIG_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} 1E 0 5";
static char * Control0_U_ANALOG_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} C8 0 5";
static char * Control0_D_ANALOG_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} D0 0 5";
static char * Control0_L_ANALOG_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} CB 0 5";
static char * Control0_R_ANALOG_Default = "{6F1D2B61-D5A0-11CF-BFC7-444553540000} CD 0 5";
static const uint32_t Default_DeadZone = 25;
static const uint32_t Default_Range = 100;
static const uint32_t Default_Plugin = PLUGIN_MEMPAK;
static const bool Default_RealN64Range = true;

CInputSettings::CInputSettings()
{
    RegisterSettings();
}

CInputSettings::~CInputSettings()
{
}

void CInputSettings::LoadController(uint32_t ControlIndex, CONTROL & ControllerInfo, N64CONTROLLER & Controller)
{
    struct 
    {
        BUTTON & Button;
        InputSettingID SettingId;
        uint32_t ControlIndex;
    } 
    Buttons[] =
    {
        { Controller.U_DPAD, Set_Control0_U_DPAD, 0 },
        { Controller.D_DPAD, Set_Control0_D_DPAD, 0 },
        { Controller.L_DPAD, Set_Control0_L_DPAD, 0 },
        { Controller.R_DPAD, Set_Control0_R_DPAD, 0 },
        { Controller.A_BUTTON, Set_Control0_A_BUTTON, 0 },
        { Controller.B_BUTTON, Set_Control0_B_BUTTON, 0 },
        { Controller.U_CBUTTON, Set_Control0_U_CBUTTON, 0 },
        { Controller.D_CBUTTON, Set_Control0_D_CBUTTON, 0 },
        { Controller.L_CBUTTON, Set_Control0_L_CBUTTON, 0 },
        { Controller.R_CBUTTON, Set_Control0_R_CBUTTON, 0 },
        { Controller.START_BUTTON, Set_Control0_START_BUTTON, 0 },
        { Controller.Z_TRIG, Set_Control0_Z_TRIG, 0 },
        { Controller.R_TRIG, Set_Control0_R_TRIG, 0 },
        { Controller.L_TRIG, Set_Control0_L_TRIG, 0 },
        { Controller.U_ANALOG, Set_Control0_U_ANALOG, 0 },
        { Controller.D_ANALOG, Set_Control0_D_ANALOG, 0 },
        { Controller.L_ANALOG, Set_Control0_L_ANALOG, 0 },
        { Controller.R_ANALOG, Set_Control0_R_ANALOG, 0 },
    };

    char Buffer[400];
    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        if (Buttons[i].ControlIndex != ControlIndex)
        {
            continue;
        }
        Buttons[i].Button = StrToButton(GetSettingSz((short)Buttons[i].SettingId, Buffer, sizeof(Buffer) / sizeof(Buffer[0])));
    }

    InputSettingID PresentSettings[] = { Set_Control0_Present };
    InputSettingID PluginSettings[] = { Set_Control0_Plugin };
    InputSettingID RangeSettings[] = { Set_Control0_Range };
    InputSettingID DeadZoneSettings[] = { Set_Control0_Deadzone };
    InputSettingID RealN64RangeSettings[] = { Set_Control0_RealN64Range };

    ControllerInfo.Present = ControlIndex < (sizeof(PresentSettings) / sizeof(PresentSettings[0])) ? GetSetting((short)PresentSettings[ControlIndex]) != 0 : 0;
    ControllerInfo.Plugin = ControlIndex < (sizeof(PluginSettings) / sizeof(PluginSettings[0])) ? GetSetting((short)PluginSettings[ControlIndex]) : Default_Plugin;
    Controller.Range = (uint8_t)(ControlIndex < (sizeof(RangeSettings) / sizeof(RangeSettings[0])) ? GetSetting((short)RangeSettings[ControlIndex]) : Default_Range);
    if (Controller.Range == 0) { Controller.Range = 1; }
    if (Controller.Range > 100) { Controller.Range = 100; }
    Controller.DeadZone = (uint8_t)(ControlIndex < (sizeof(DeadZoneSettings) / sizeof(DeadZoneSettings[0])) ? GetSetting((short)DeadZoneSettings[ControlIndex]) : Default_DeadZone);
    if (Controller.DeadZone > 100) { Controller.DeadZone = 100; }
    Controller.RealN64Range = (ControlIndex < (sizeof(RealN64RangeSettings) / sizeof(RealN64RangeSettings[0])) ? GetSetting((short)RealN64RangeSettings[ControlIndex]) != 0 : Default_RealN64Range);
}

void CInputSettings::SaveController(uint32_t ControlIndex, const CONTROL & ControllerInfo, const N64CONTROLLER & Controller)
{
    struct 
    {
        const BUTTON & Button;
        InputSettingID SettingId;
        uint32_t ControlIndex;
    }
    Buttons[] =
    {
        { Controller.U_DPAD, Set_Control0_U_DPAD, 0 },
        { Controller.D_DPAD, Set_Control0_D_DPAD, 0 },
        { Controller.L_DPAD, Set_Control0_L_DPAD, 0 },
        { Controller.R_DPAD, Set_Control0_R_DPAD, 0 },
        { Controller.A_BUTTON, Set_Control0_A_BUTTON, 0 },
        { Controller.B_BUTTON, Set_Control0_B_BUTTON, 0 },
        { Controller.U_CBUTTON, Set_Control0_U_CBUTTON, 0 },
        { Controller.D_CBUTTON, Set_Control0_D_CBUTTON, 0 },
        { Controller.L_CBUTTON, Set_Control0_L_CBUTTON, 0 },
        { Controller.R_CBUTTON, Set_Control0_R_CBUTTON, 0 },
        { Controller.START_BUTTON, Set_Control0_START_BUTTON, 0 },
        { Controller.Z_TRIG, Set_Control0_Z_TRIG, 0 },
        { Controller.R_TRIG, Set_Control0_R_TRIG, 0 },
        { Controller.L_TRIG, Set_Control0_L_TRIG, 0 },
        { Controller.U_ANALOG, Set_Control0_U_ANALOG, 0 },
        { Controller.D_ANALOG, Set_Control0_D_ANALOG, 0 },
        { Controller.L_ANALOG, Set_Control0_L_ANALOG, 0 },
        { Controller.R_ANALOG, Set_Control0_R_ANALOG, 0 },
    };

    InputSettingID PresentSettings[] = { Set_Control0_Present };
    InputSettingID PluginSettings[] = { Set_Control0_Plugin };
    InputSettingID RangeSettings[] = { Set_Control0_Range };
    InputSettingID DeadzoneSettings[] = { Set_Control0_Deadzone };
    InputSettingID RealN64RangeSettings[] = { Set_Control0_RealN64Range };

    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        if (Buttons[i].ControlIndex != ControlIndex)
        {
            continue;
        }
        SetSettingSz((short)Buttons[i].SettingId, ButtonToStr(Buttons[i].Button).c_str());
    }

    if (ControlIndex < (sizeof(PresentSettings) / sizeof(PresentSettings[0])))
    {
        SetSetting((short)PresentSettings[ControlIndex], ControllerInfo.Present);
    }
    if (ControlIndex < (sizeof(PluginSettings) / sizeof(PluginSettings[0])))
    {
        SetSetting((short)PluginSettings[ControlIndex], ControllerInfo.Plugin);
    }

    if (ControlIndex < (sizeof(RangeSettings) / sizeof(RangeSettings[0])))
    {
        SetSetting((short)RangeSettings[ControlIndex], Controller.Range);
    }
  
    if (ControlIndex < (sizeof(DeadzoneSettings) / sizeof(DeadzoneSettings[0])))
    {
        SetSetting((short)DeadzoneSettings[ControlIndex], Controller.DeadZone);
    }
    if (ControlIndex < (sizeof(RealN64RangeSettings) / sizeof(RealN64RangeSettings[0])))
    {
        SetSetting((short)RealN64RangeSettings[ControlIndex], Controller.RealN64Range ? 1 : 0);
    }
    FlushSettings();
}

void CInputSettings::ResetController(uint32_t ControlIndex, CONTROL & ControllerInfo, N64CONTROLLER & Controller)
{
    struct 
    {
        BUTTON & Button;
        const char * DefaultValue;
        uint32_t ControlIndex;
    }
    Buttons[] =
    {
        { Controller.U_DPAD, Control0_U_DPAD_Default, 0 },
        { Controller.D_DPAD, Control0_D_DPAD_Default, 0 },
        { Controller.L_DPAD, Control0_L_DPAD_Default, 0 },
        { Controller.R_DPAD, Control0_R_DPAD_Default, 0 },
        { Controller.A_BUTTON, Control0_A_BUTTON_Default, 0 },
        { Controller.B_BUTTON, Control0_B_BUTTON_Default, 0 },
        { Controller.U_CBUTTON, Control0_U_CBUTTON_Default, 0 },
        { Controller.D_CBUTTON, Control0_D_CBUTTON_Default, 0 },
        { Controller.L_CBUTTON, Control0_L_CBUTTON_Default, 0 },
        { Controller.R_CBUTTON, Control0_R_CBUTTON_Default, 0 },
        { Controller.START_BUTTON, Control0_START_BUTTON_Default, 0 },
        { Controller.Z_TRIG, Control0_Z_TRIG_Default, 0 },
        { Controller.R_TRIG, Control0_R_TRIG_Default, 0 },
        { Controller.L_TRIG, Control0_L_TRIG_Default, 0 },
        { Controller.U_ANALOG, Control0_U_ANALOG_Default, 0 },
        { Controller.D_ANALOG, Control0_D_ANALOG_Default, 0 },
        { Controller.L_ANALOG, Control0_L_ANALOG_Default, 0 },
        { Controller.R_ANALOG, Control0_R_ANALOG_Default, 0 },
    };

    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        if (Buttons[i].ControlIndex != ControlIndex)
        {
            continue;
        }
        Buttons[i].Button = StrToButton(Buttons[i].DefaultValue);
    }
    Controller.Range = Default_Range;
    Controller.DeadZone = Default_DeadZone;
    ControllerInfo.Present = ControlIndex == 0 ? 1 : 0;
    ControllerInfo.Plugin = Default_Plugin;
}

BUTTON CInputSettings::StrToButton(const char * Buffer)
{
    BUTTON Button = { 0 };
    GUID &guid = Button.DeviceGuid;
    uint32_t ButtonOffset = 0, ButtonAxisID = 0, ButtonType = 0;
    sscanf(Buffer,
        "{%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx} %x %u %u",
        &guid.Data1, &guid.Data2, &guid.Data3,
        &guid.Data4[0], &guid.Data4[1], &guid.Data4[2], &guid.Data4[3],
        &guid.Data4[4], &guid.Data4[5], &guid.Data4[6], &guid.Data4[7],
        &ButtonOffset, &ButtonAxisID, &ButtonType);
    Button.Offset = (uint8_t)(ButtonOffset & 0xFF);
    Button.AxisID = (uint8_t)(ButtonAxisID & 0xFF);
    Button.BtnType = (BtnType)ButtonType;
    return Button;
}

std::string CInputSettings::ButtonToStr(const BUTTON & Button)
{
    return stdstr_f("%s %02X %u %u", GUIDtoString(Button.DeviceGuid).c_str(), Button.Offset, Button.AxisID, Button.BtnType);
}

std::string CInputSettings::GUIDtoString(const GUID & guid)
{
    return stdstr_f("{%08.8lX-%04.4hX-%04.4hX-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

void CInputSettings::RegisterSettings(void)
{
    SetModuleName("Input");
    RegisterSetting(Set_Control0_Present, Data_DWORD_General, "Present", "Controller 1", 1, nullptr);
    RegisterSetting(Set_Control0_Plugin, Data_DWORD_General, "Plugin", "Controller 1", Default_Plugin, nullptr);
    RegisterSetting(Set_Control0_Range, Data_DWORD_General, "Range", "Controller 1", Default_Range, nullptr);
    RegisterSetting(Set_Control0_Deadzone, Data_DWORD_General, "Deadzone", "Controller 1", Default_DeadZone, nullptr);
    RegisterSetting(Set_Control0_RealN64Range, Data_DWORD_General, "RealN64Range", "Controller 1", Default_RealN64Range, nullptr);
    RegisterSetting(Set_Control0_U_DPAD, Data_String_General, "DPadUp", "Controller 1", 0, Control0_U_DPAD_Default);
    RegisterSetting(Set_Control0_D_DPAD, Data_String_General, "DPadDown", "Controller 1", 0, Control0_D_DPAD_Default);
    RegisterSetting(Set_Control0_L_DPAD, Data_String_General, "DPadLeft", "Controller 1", 0, Control0_L_DPAD_Default);
    RegisterSetting(Set_Control0_R_DPAD, Data_String_General, "DPadRight", "Controller 1", 0, Control0_R_DPAD_Default);
    RegisterSetting(Set_Control0_A_BUTTON, Data_String_General, "ButtonA", "Controller 1", 0, Control0_A_BUTTON_Default);
    RegisterSetting(Set_Control0_B_BUTTON, Data_String_General, "ButtonB", "Controller 1", 0, Control0_B_BUTTON_Default);
    RegisterSetting(Set_Control0_U_CBUTTON, Data_String_General, "CButtonUp", "Controller 1", 0, Control0_U_CBUTTON_Default);
    RegisterSetting(Set_Control0_D_CBUTTON, Data_String_General, "CButtonDown", "Controller 1", 0, Control0_D_CBUTTON_Default);
    RegisterSetting(Set_Control0_L_CBUTTON, Data_String_General, "CButtonLeft", "Controller 1", 0, Control0_L_CBUTTON_Default);
    RegisterSetting(Set_Control0_R_CBUTTON, Data_String_General, "CButtonRight", "Controller 1", 0, Control0_R_CBUTTON_Default);
    RegisterSetting(Set_Control0_START_BUTTON, Data_String_General, "ButtonStart", "Controller 1", 0, Control0_START_BUTTON_Default);
    RegisterSetting(Set_Control0_Z_TRIG, Data_String_General, "ButtonZ", "Controller 1", 0, Control0_Z_TRIG_Default);
    RegisterSetting(Set_Control0_R_TRIG, Data_String_General, "ButtonR", "Controller 1", 0, Control0_R_TRIG_Default);
    RegisterSetting(Set_Control0_L_TRIG, Data_String_General, "ButtonL", "Controller 1", 0, Control0_L_TRIG_Default);
    RegisterSetting(Set_Control0_U_ANALOG, Data_String_General, "AnalogUp", "Controller 1", 0, Control0_U_ANALOG_Default);
    RegisterSetting(Set_Control0_D_ANALOG, Data_String_General, "AnalogDown", "Controller 1", 0, Control0_D_ANALOG_Default);
    RegisterSetting(Set_Control0_L_ANALOG, Data_String_General, "AnalogLeft", "Controller 1", 0, Control0_L_ANALOG_Default);
    RegisterSetting(Set_Control0_R_ANALOG, Data_String_General, "AnalogRight", "Controller 1", 0, Control0_R_ANALOG_Default);
}

void SetupInputSettings(void)
{
    if (g_Settings == nullptr)
    {
        g_Settings = new CInputSettings;
    }
}

void CleanupInputSettings(void)
{
    if (g_Settings)
    {
        delete g_Settings;
        g_Settings = nullptr;
    }
}

#ifdef _WIN32
#include <Windows.h>
#endif

extern "C" void UseUnregisteredSetting(int /*SettingID*/)
{
#ifdef _WIN32
    DebugBreak();
#endif
}
