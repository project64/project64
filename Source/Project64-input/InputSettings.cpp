#include <Settings/Settings.h>
#include <Common/StdString.h>
#include "InputSettingsID.h"
#include "InputSettings.h"

CInputSettings * g_Settings = nullptr;

/* Default First N64 Controller Setup */
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
static const bool Default_RemoveDuplicate = true;

/* Default Mouse Setup (Forced) */
static char* Mouse_A_BUTTON_Default = "{6F1D2B60-D5A0-11CF-BFC7-444553540000} 00 0 6";
static char* Mouse_B_BUTTON_Default = "{6F1D2B60-D5A0-11CF-BFC7-444553540000} 01 0 6";
static char* Mouse_U_ANALOG_Default = "{6F1D2B60-D5A0-11CF-BFC7-444553540000} 01 0 7";
static char* Mouse_D_ANALOG_Default = "{6F1D2B60-D5A0-11CF-BFC7-444553540000} 01 1 7";
static char* Mouse_L_ANALOG_Default = "{6F1D2B60-D5A0-11CF-BFC7-444553540000} 00 0 7";
static char* Mouse_R_ANALOG_Default = "{6F1D2B60-D5A0-11CF-BFC7-444553540000} 00 1 7";
static const uint32_t DefaultMouse_DeadZone = 1;
static const uint32_t DefaultMouse_Range = 100;
static const uint32_t DefaultMouse_Plugin = PLUGIN_NONE;
static const bool DefaultMouse_RealN64Range = false;
static const bool DefaultMouse_RemoveDuplicate = true;

CInputSettings::CInputSettings()
{
    RegisterSettings();
}

CInputSettings::~CInputSettings()
{
}

void CInputSettings::LoadController(uint32_t ControlIndex, CONTROL & ControllerInfo, N64CONTROLLER & Controller)
{
    InputSettingID PresentSettings[] = { Set_Control0_Present, Set_Control1_Present, Set_Control2_Present, Set_Control3_Present };
    InputSettingID PluginSettings[] = { Set_Control0_Plugin, Set_Control1_Plugin, Set_Control2_Plugin, Set_Control3_Plugin };

    ControllerInfo.Present = ControlIndex < (sizeof(PresentSettings) / sizeof(PresentSettings[0])) ? GetSetting((short)PresentSettings[ControlIndex]) : PRESENT_NONE;
    ControllerInfo.Plugin = ControlIndex < (sizeof(PluginSettings) / sizeof(PluginSettings[0])) ? GetSetting((short)PluginSettings[ControlIndex]) : Default_Plugin;

    if (ControllerInfo.Present == PRESENT_MOUSE)
    {
        GetControllerMouse(Controller);
    }
    else
    {
        struct
        {
            BUTTON& Button;
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

            { Controller.U_DPAD, Set_Control1_U_DPAD, 1 },
            { Controller.D_DPAD, Set_Control1_D_DPAD, 1 },
            { Controller.L_DPAD, Set_Control1_L_DPAD, 1 },
            { Controller.R_DPAD, Set_Control1_R_DPAD, 1 },
            { Controller.A_BUTTON, Set_Control1_A_BUTTON, 1 },
            { Controller.B_BUTTON, Set_Control1_B_BUTTON, 1 },
            { Controller.U_CBUTTON, Set_Control1_U_CBUTTON, 1 },
            { Controller.D_CBUTTON, Set_Control1_D_CBUTTON, 1 },
            { Controller.L_CBUTTON, Set_Control1_L_CBUTTON, 1 },
            { Controller.R_CBUTTON, Set_Control1_R_CBUTTON, 1 },
            { Controller.START_BUTTON, Set_Control1_START_BUTTON, 1 },
            { Controller.Z_TRIG, Set_Control1_Z_TRIG, 1 },
            { Controller.R_TRIG, Set_Control1_R_TRIG, 1 },
            { Controller.L_TRIG, Set_Control1_L_TRIG, 1 },
            { Controller.U_ANALOG, Set_Control1_U_ANALOG, 1 },
            { Controller.D_ANALOG, Set_Control1_D_ANALOG, 1 },
            { Controller.L_ANALOG, Set_Control1_L_ANALOG, 1 },
            { Controller.R_ANALOG, Set_Control1_R_ANALOG, 1 },

            { Controller.U_DPAD, Set_Control2_U_DPAD, 2 },
            { Controller.D_DPAD, Set_Control2_D_DPAD, 2 },
            { Controller.L_DPAD, Set_Control2_L_DPAD, 2 },
            { Controller.R_DPAD, Set_Control2_R_DPAD, 2 },
            { Controller.A_BUTTON, Set_Control2_A_BUTTON, 2 },
            { Controller.B_BUTTON, Set_Control2_B_BUTTON, 2 },
            { Controller.U_CBUTTON, Set_Control2_U_CBUTTON, 2 },
            { Controller.D_CBUTTON, Set_Control2_D_CBUTTON, 2 },
            { Controller.L_CBUTTON, Set_Control2_L_CBUTTON, 2 },
            { Controller.R_CBUTTON, Set_Control2_R_CBUTTON, 2 },
            { Controller.START_BUTTON, Set_Control2_START_BUTTON, 2 },
            { Controller.Z_TRIG, Set_Control2_Z_TRIG, 2 },
            { Controller.R_TRIG, Set_Control2_R_TRIG, 2 },
            { Controller.L_TRIG, Set_Control2_L_TRIG, 2 },
            { Controller.U_ANALOG, Set_Control2_U_ANALOG, 2 },
            { Controller.D_ANALOG, Set_Control2_D_ANALOG, 2 },
            { Controller.L_ANALOG, Set_Control2_L_ANALOG, 2 },
            { Controller.R_ANALOG, Set_Control2_R_ANALOG, 2 },

            { Controller.U_DPAD, Set_Control3_U_DPAD, 3 },
            { Controller.D_DPAD, Set_Control3_D_DPAD, 3 },
            { Controller.L_DPAD, Set_Control3_L_DPAD, 3 },
            { Controller.R_DPAD, Set_Control3_R_DPAD, 3 },
            { Controller.A_BUTTON, Set_Control3_A_BUTTON, 3 },
            { Controller.B_BUTTON, Set_Control3_B_BUTTON, 3 },
            { Controller.U_CBUTTON, Set_Control3_U_CBUTTON, 3 },
            { Controller.D_CBUTTON, Set_Control3_D_CBUTTON, 3 },
            { Controller.L_CBUTTON, Set_Control3_L_CBUTTON, 3 },
            { Controller.R_CBUTTON, Set_Control3_R_CBUTTON, 3 },
            { Controller.START_BUTTON, Set_Control3_START_BUTTON, 3 },
            { Controller.Z_TRIG, Set_Control3_Z_TRIG, 3 },
            { Controller.R_TRIG, Set_Control3_R_TRIG, 3 },
            { Controller.L_TRIG, Set_Control3_L_TRIG, 3 },
            { Controller.U_ANALOG, Set_Control3_U_ANALOG, 3 },
            { Controller.D_ANALOG, Set_Control3_D_ANALOG, 3 },
            { Controller.L_ANALOG, Set_Control3_L_ANALOG, 3 },
            { Controller.R_ANALOG, Set_Control3_R_ANALOG, 3 },
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

        InputSettingID RangeSettings[] = { Set_Control0_Range, Set_Control1_Range, Set_Control2_Range, Set_Control3_Range };
        InputSettingID DeadZoneSettings[] = { Set_Control0_Deadzone, Set_Control1_Deadzone, Set_Control2_Deadzone,Set_Control3_Deadzone };
        InputSettingID RealN64RangeSettings[] = { Set_Control0_RealN64Range,  Set_Control1_RealN64Range, Set_Control2_RealN64Range, Set_Control3_RealN64Range };
        InputSettingID RemoveDuplicateSettings[] = { Set_Control0_RemoveDuplicate, Set_Control1_RemoveDuplicate, Set_Control2_RemoveDuplicate, Set_Control3_RemoveDuplicate };

        Controller.Range = (uint8_t)(ControlIndex < (sizeof(RangeSettings) / sizeof(RangeSettings[0])) ? GetSetting((short)RangeSettings[ControlIndex]) : Default_Range);
        if (Controller.Range == 0) { Controller.Range = 1; }
        if (Controller.Range > 100) { Controller.Range = 100; }
        Controller.DeadZone = (uint8_t)(ControlIndex < (sizeof(DeadZoneSettings) / sizeof(DeadZoneSettings[0])) ? GetSetting((short)DeadZoneSettings[ControlIndex]) : Default_DeadZone);
        if (Controller.DeadZone > 100) { Controller.DeadZone = 100; }
        Controller.RealN64Range = (ControlIndex < (sizeof(RealN64RangeSettings) / sizeof(RealN64RangeSettings[0])) ? GetSetting((short)RealN64RangeSettings[ControlIndex]) != 0 : Default_RealN64Range);
        Controller.RemoveDuplicate = (ControlIndex < (sizeof(RemoveDuplicateSettings) / sizeof(RemoveDuplicateSettings[0])) ? GetSetting((short)RemoveDuplicateSettings[ControlIndex]) != 0 : Default_RemoveDuplicate);
    }
}

void CInputSettings::SaveController(uint32_t ControlIndex, const CONTROL & ControllerInfo, const N64CONTROLLER & Controller)
{
    InputSettingID PresentSettings[] = { Set_Control0_Present, Set_Control1_Present, Set_Control2_Present, Set_Control3_Present };
    InputSettingID PluginSettings[] = { Set_Control0_Plugin, Set_Control1_Plugin, Set_Control2_Plugin, Set_Control3_Plugin };

    if (ControlIndex < (sizeof(PresentSettings) / sizeof(PresentSettings[0])))
    {
        SetSetting((short)PresentSettings[ControlIndex], ControllerInfo.Present);
    }
    if (ControlIndex < (sizeof(PluginSettings) / sizeof(PluginSettings[0])))
    {
        SetSetting((short)PluginSettings[ControlIndex], ControllerInfo.Plugin);
    }

    if (ControllerInfo.Present == PRESENT_MOUSE)
    {
        //Using Forced Controller Mouse Setup, do not overwrite Controller Configuration with it
        FlushSettings();
        return;
    }

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

        { Controller.U_DPAD, Set_Control1_U_DPAD, 1 },
        { Controller.D_DPAD, Set_Control1_D_DPAD, 1 },
        { Controller.L_DPAD, Set_Control1_L_DPAD, 1 },
        { Controller.R_DPAD, Set_Control1_R_DPAD, 1 },
        { Controller.A_BUTTON, Set_Control1_A_BUTTON, 1 },
        { Controller.B_BUTTON, Set_Control1_B_BUTTON, 1 },
        { Controller.U_CBUTTON, Set_Control1_U_CBUTTON, 1 },
        { Controller.D_CBUTTON, Set_Control1_D_CBUTTON, 1 },
        { Controller.L_CBUTTON, Set_Control1_L_CBUTTON, 1 },
        { Controller.R_CBUTTON, Set_Control1_R_CBUTTON, 1 },
        { Controller.START_BUTTON, Set_Control1_START_BUTTON, 1 },
        { Controller.Z_TRIG, Set_Control1_Z_TRIG, 1 },
        { Controller.R_TRIG, Set_Control1_R_TRIG, 1 },
        { Controller.L_TRIG, Set_Control1_L_TRIG, 1 },
        { Controller.U_ANALOG, Set_Control1_U_ANALOG, 1 },
        { Controller.D_ANALOG, Set_Control1_D_ANALOG, 1 },
        { Controller.L_ANALOG, Set_Control1_L_ANALOG, 1 },
        { Controller.R_ANALOG, Set_Control1_R_ANALOG, 1 },

        { Controller.U_DPAD, Set_Control2_U_DPAD, 2 },
        { Controller.D_DPAD, Set_Control2_D_DPAD, 2 },
        { Controller.L_DPAD, Set_Control2_L_DPAD, 2 },
        { Controller.R_DPAD, Set_Control2_R_DPAD, 2 },
        { Controller.A_BUTTON, Set_Control2_A_BUTTON, 2 },
        { Controller.B_BUTTON, Set_Control2_B_BUTTON, 2 },
        { Controller.U_CBUTTON, Set_Control2_U_CBUTTON, 2 },
        { Controller.D_CBUTTON, Set_Control2_D_CBUTTON, 2 },
        { Controller.L_CBUTTON, Set_Control2_L_CBUTTON, 2 },
        { Controller.R_CBUTTON, Set_Control2_R_CBUTTON, 2 },
        { Controller.START_BUTTON, Set_Control2_START_BUTTON, 2 },
        { Controller.Z_TRIG, Set_Control2_Z_TRIG, 2 },
        { Controller.R_TRIG, Set_Control2_R_TRIG, 2 },
        { Controller.L_TRIG, Set_Control2_L_TRIG, 2 },
        { Controller.U_ANALOG, Set_Control2_U_ANALOG, 2 },
        { Controller.D_ANALOG, Set_Control2_D_ANALOG, 2 },
        { Controller.L_ANALOG, Set_Control2_L_ANALOG, 2 },
        { Controller.R_ANALOG, Set_Control2_R_ANALOG, 2 },

        { Controller.U_DPAD, Set_Control3_U_DPAD, 3 },
        { Controller.D_DPAD, Set_Control3_D_DPAD, 3 },
        { Controller.L_DPAD, Set_Control3_L_DPAD, 3 },
        { Controller.R_DPAD, Set_Control3_R_DPAD, 3 },
        { Controller.A_BUTTON, Set_Control3_A_BUTTON, 3 },
        { Controller.B_BUTTON, Set_Control3_B_BUTTON, 3 },
        { Controller.U_CBUTTON, Set_Control3_U_CBUTTON, 3 },
        { Controller.D_CBUTTON, Set_Control3_D_CBUTTON, 3 },
        { Controller.L_CBUTTON, Set_Control3_L_CBUTTON, 3 },
        { Controller.R_CBUTTON, Set_Control3_R_CBUTTON, 3 },
        { Controller.START_BUTTON, Set_Control3_START_BUTTON, 3 },
        { Controller.Z_TRIG, Set_Control3_Z_TRIG, 3 },
        { Controller.R_TRIG, Set_Control3_R_TRIG, 3 },
        { Controller.L_TRIG, Set_Control3_L_TRIG, 3 },
        { Controller.U_ANALOG, Set_Control3_U_ANALOG, 3 },
        { Controller.D_ANALOG, Set_Control3_D_ANALOG, 3 },
        { Controller.L_ANALOG, Set_Control3_L_ANALOG, 3 },
        { Controller.R_ANALOG, Set_Control3_R_ANALOG, 3 },
    };

    InputSettingID RangeSettings[] = { Set_Control0_Range, Set_Control1_Range, Set_Control2_Range, Set_Control3_Range };
    InputSettingID DeadZoneSettings[] = { Set_Control0_Deadzone, Set_Control1_Deadzone, Set_Control2_Deadzone,Set_Control3_Deadzone };
    InputSettingID RealN64RangeSettings[] = { Set_Control0_RealN64Range,  Set_Control1_RealN64Range, Set_Control2_RealN64Range, Set_Control3_RealN64Range };
    InputSettingID RemoveDuplicateSettings[] = { Set_Control0_RemoveDuplicate, Set_Control1_RemoveDuplicate, Set_Control2_RemoveDuplicate, Set_Control3_RemoveDuplicate };

    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        if (Buttons[i].ControlIndex != ControlIndex)
        {
            continue;
        }
        SetSettingSz((short)Buttons[i].SettingId, ButtonToStr(Buttons[i].Button).c_str());
    }

    if (ControlIndex < (sizeof(RangeSettings) / sizeof(RangeSettings[0])))
    {
        SetSetting((short)RangeSettings[ControlIndex], Controller.Range);
    }
  
    if (ControlIndex < (sizeof(DeadZoneSettings) / sizeof(DeadZoneSettings[0])))
    {
        SetSetting((short)DeadZoneSettings[ControlIndex], Controller.DeadZone);
    }
    if (ControlIndex < (sizeof(RealN64RangeSettings) / sizeof(RealN64RangeSettings[0])))
    {
        SetSetting((short)RealN64RangeSettings[ControlIndex], Controller.RealN64Range ? 1 : 0);
    }
    if (ControlIndex < (sizeof(RemoveDuplicateSettings) / sizeof(RemoveDuplicateSettings[0])))
    {
        SetSetting((short)RemoveDuplicateSettings[ControlIndex], Controller.RemoveDuplicate ? 1 : 0);
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

        { Controller.U_DPAD, "", 1 },
        { Controller.D_DPAD, "", 1 },
        { Controller.L_DPAD, "", 1 },
        { Controller.R_DPAD, "", 1 },
        { Controller.A_BUTTON, "", 1 },
        { Controller.B_BUTTON, "", 1 },
        { Controller.U_CBUTTON, "", 1 },
        { Controller.D_CBUTTON, "", 1 },
        { Controller.L_CBUTTON, "", 1 },
        { Controller.R_CBUTTON, "", 1 },
        { Controller.START_BUTTON, "", 1 },
        { Controller.Z_TRIG, "", 1 },
        { Controller.R_TRIG, "", 1 },
        { Controller.L_TRIG, "", 1 },
        { Controller.U_ANALOG, "", 1 },
        { Controller.D_ANALOG, "", 1 },
        { Controller.L_ANALOG, "", 1 },
        { Controller.R_ANALOG, "", 1 },

        { Controller.U_DPAD, "", 2 },
        { Controller.D_DPAD, "", 2 },
        { Controller.L_DPAD, "", 2 },
        { Controller.R_DPAD, "", 2 },
        { Controller.A_BUTTON, "", 2 },
        { Controller.B_BUTTON, "", 2 },
        { Controller.U_CBUTTON, "", 2 },
        { Controller.D_CBUTTON, "", 2 },
        { Controller.L_CBUTTON, "", 2 },
        { Controller.R_CBUTTON, "", 2 },
        { Controller.START_BUTTON, "", 2 },
        { Controller.Z_TRIG, "", 2 },
        { Controller.R_TRIG, "", 2 },
        { Controller.L_TRIG, "", 2 },
        { Controller.U_ANALOG, "", 2 },
        { Controller.D_ANALOG, "", 2 },
        { Controller.L_ANALOG, "", 2 },
        { Controller.R_ANALOG, "", 2 },

        { Controller.U_DPAD, "", 3 },
        { Controller.D_DPAD, "", 3 },
        { Controller.L_DPAD, "", 3 },
        { Controller.R_DPAD, "", 3 },
        { Controller.A_BUTTON, "", 3 },
        { Controller.B_BUTTON, "", 3 },
        { Controller.U_CBUTTON, "", 3 },
        { Controller.D_CBUTTON, "", 3 },
        { Controller.L_CBUTTON, "", 3 },
        { Controller.R_CBUTTON, "", 3 },
        { Controller.START_BUTTON, "", 3 },
        { Controller.Z_TRIG, "", 3 },
        { Controller.R_TRIG, "", 3 },
        { Controller.L_TRIG, "", 3 },
        { Controller.U_ANALOG, "", 3 },
        { Controller.D_ANALOG, "", 3 },
        { Controller.L_ANALOG, "", 3 },
        { Controller.R_ANALOG, "", 3 },
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
    ControllerInfo.Present = ControlIndex == 0 ? PRESENT_CONT : PRESENT_NONE;
    ControllerInfo.Plugin = Default_Plugin;
}

void CInputSettings::GetControllerMouse(N64CONTROLLER& Controller)
{
    struct
    {
        BUTTON& Button;
        const char* DefaultValue;
    }
    Buttons[] =
    {
        { Controller.U_DPAD, "" },
        { Controller.D_DPAD, "" },
        { Controller.L_DPAD, "" },
        { Controller.R_DPAD, "" },
        { Controller.A_BUTTON, Mouse_A_BUTTON_Default },
        { Controller.B_BUTTON, Mouse_B_BUTTON_Default },
        { Controller.U_CBUTTON, "" },
        { Controller.D_CBUTTON, "" },
        { Controller.L_CBUTTON, "" },
        { Controller.R_CBUTTON, "" },
        { Controller.START_BUTTON, "" },
        { Controller.Z_TRIG, "" },
        { Controller.R_TRIG, "" },
        { Controller.L_TRIG, "" },
        { Controller.U_ANALOG, Mouse_U_ANALOG_Default },
        { Controller.D_ANALOG, Mouse_D_ANALOG_Default },
        { Controller.L_ANALOG, Mouse_L_ANALOG_Default },
        { Controller.R_ANALOG, Mouse_R_ANALOG_Default },
    };

    for (size_t i = 0, n = sizeof(Buttons) / sizeof(Buttons[0]); i < n; i++)
    {
        Buttons[i].Button = StrToButton(Buttons[i].DefaultValue);
    }
    Controller.Range = DefaultMouse_Range;
    Controller.DeadZone = DefaultMouse_DeadZone;
    Controller.RealN64Range = DefaultMouse_RealN64Range;
    Controller.RemoveDuplicate = DefaultMouse_RemoveDuplicate;
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
    RegisterSetting(Set_Control0_RemoveDuplicate, Data_DWORD_General, "Remove Duplicate", "Controller 1", Default_RemoveDuplicate, nullptr);
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

    RegisterSetting(Set_Control1_Present, Data_DWORD_General, "Present", "Controller 2", 0, nullptr);
    RegisterSetting(Set_Control1_Plugin, Data_DWORD_General, "Plugin", "Controller 2", Default_Plugin, nullptr);
    RegisterSetting(Set_Control1_Range, Data_DWORD_General, "Range", "Controller 2", Default_Range, nullptr);
    RegisterSetting(Set_Control1_Deadzone, Data_DWORD_General, "Deadzone", "Controller 2", Default_DeadZone, nullptr);
    RegisterSetting(Set_Control1_RealN64Range, Data_DWORD_General, "RealN64Range", "Controller 2", Default_RealN64Range, nullptr);
    RegisterSetting(Set_Control1_RemoveDuplicate, Data_DWORD_General, "Remove Duplicate", "Controller 2", Default_RemoveDuplicate, nullptr);
    RegisterSetting(Set_Control1_U_DPAD, Data_String_General, "DPadUp", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_D_DPAD, Data_String_General, "DPadDown", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_L_DPAD, Data_String_General, "DPadLeft", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_R_DPAD, Data_String_General, "DPadRight", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_A_BUTTON, Data_String_General, "ButtonA", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_B_BUTTON, Data_String_General, "ButtonB", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_U_CBUTTON, Data_String_General, "CButtonUp", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_D_CBUTTON, Data_String_General, "CButtonDown", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_L_CBUTTON, Data_String_General, "CButtonLeft", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_R_CBUTTON, Data_String_General, "CButtonRight", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_START_BUTTON, Data_String_General, "ButtonStart", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_Z_TRIG, Data_String_General, "ButtonZ", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_R_TRIG, Data_String_General, "ButtonR", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_L_TRIG, Data_String_General, "ButtonL", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_U_ANALOG, Data_String_General, "AnalogUp", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_D_ANALOG, Data_String_General, "AnalogDown", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_L_ANALOG, Data_String_General, "AnalogLeft", "Controller 2", 0, "");
    RegisterSetting(Set_Control1_R_ANALOG, Data_String_General, "AnalogRight", "Controller 2", 0, "");

    RegisterSetting(Set_Control2_Present, Data_DWORD_General, "Present", "Controller 3", 0, nullptr);
    RegisterSetting(Set_Control2_Plugin, Data_DWORD_General, "Plugin", "Controller 3", Default_Plugin, nullptr);
    RegisterSetting(Set_Control2_Range, Data_DWORD_General, "Range", "Controller 3", Default_Range, nullptr);
    RegisterSetting(Set_Control2_Deadzone, Data_DWORD_General, "Deadzone", "Controller 3", Default_DeadZone, nullptr);
    RegisterSetting(Set_Control2_RealN64Range, Data_DWORD_General, "RealN64Range", "Controller 3", Default_RealN64Range, nullptr);
    RegisterSetting(Set_Control2_RemoveDuplicate, Data_DWORD_General, "Remove Duplicate", "Controller 3", Default_RemoveDuplicate, nullptr);
    RegisterSetting(Set_Control2_U_DPAD, Data_String_General, "DPadUp", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_D_DPAD, Data_String_General, "DPadDown", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_L_DPAD, Data_String_General, "DPadLeft", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_R_DPAD, Data_String_General, "DPadRight", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_A_BUTTON, Data_String_General, "ButtonA", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_B_BUTTON, Data_String_General, "ButtonB", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_U_CBUTTON, Data_String_General, "CButtonUp", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_D_CBUTTON, Data_String_General, "CButtonDown", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_L_CBUTTON, Data_String_General, "CButtonLeft", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_R_CBUTTON, Data_String_General, "CButtonRight", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_START_BUTTON, Data_String_General, "ButtonStart", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_Z_TRIG, Data_String_General, "ButtonZ", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_R_TRIG, Data_String_General, "ButtonR", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_L_TRIG, Data_String_General, "ButtonL", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_U_ANALOG, Data_String_General, "AnalogUp", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_D_ANALOG, Data_String_General, "AnalogDown", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_L_ANALOG, Data_String_General, "AnalogLeft", "Controller 3", 0, "");
    RegisterSetting(Set_Control2_R_ANALOG, Data_String_General, "AnalogRight", "Controller 3", 0, "");

    RegisterSetting(Set_Control3_Present, Data_DWORD_General, "Present", "Controller 4", 0, nullptr);
    RegisterSetting(Set_Control3_Plugin, Data_DWORD_General, "Plugin", "Controller 4", Default_Plugin, nullptr);
    RegisterSetting(Set_Control3_Range, Data_DWORD_General, "Range", "Controller 4", Default_Range, nullptr);
    RegisterSetting(Set_Control3_Deadzone, Data_DWORD_General, "Deadzone", "Controller 4", Default_DeadZone, nullptr);
    RegisterSetting(Set_Control3_RealN64Range, Data_DWORD_General, "RealN64Range", "Controller 4", Default_RealN64Range, nullptr);
    RegisterSetting(Set_Control3_RemoveDuplicate, Data_DWORD_General, "Remove Duplicate", "Controller 4", Default_RemoveDuplicate, nullptr);
    RegisterSetting(Set_Control3_U_DPAD, Data_String_General, "DPadUp", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_D_DPAD, Data_String_General, "DPadDown", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_L_DPAD, Data_String_General, "DPadLeft", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_R_DPAD, Data_String_General, "DPadRight", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_A_BUTTON, Data_String_General, "ButtonA", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_B_BUTTON, Data_String_General, "ButtonB", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_U_CBUTTON, Data_String_General, "CButtonUp", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_D_CBUTTON, Data_String_General, "CButtonDown", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_L_CBUTTON, Data_String_General, "CButtonLeft", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_R_CBUTTON, Data_String_General, "CButtonRight", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_START_BUTTON, Data_String_General, "ButtonStart", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_Z_TRIG, Data_String_General, "ButtonZ", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_R_TRIG, Data_String_General, "ButtonR", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_L_TRIG, Data_String_General, "ButtonL", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_U_ANALOG, Data_String_General, "AnalogUp", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_D_ANALOG, Data_String_General, "AnalogDown", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_L_ANALOG, Data_String_General, "AnalogLeft", "Controller 4", 0, "");
    RegisterSetting(Set_Control3_R_ANALOG, Data_String_General, "AnalogRight", "Controller 4", 0, "");
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
