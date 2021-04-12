#pragma once
#include <stdint.h>
#include <string>
#include "N64Controller.h"
#include "ControllerSpec1.1.h"

class CInputSettings
{
public:
    CInputSettings();
    ~CInputSettings();

    void LoadController(uint32_t ControlIndex, CONTROL & ControllerInfo, N64CONTROLLER & Controller);
    void SaveController(uint32_t ControlIndex, const CONTROL & ControllerInfo, const N64CONTROLLER & Controller);
    void ResetController(uint32_t ControlIndex, CONTROL & ControllerInfo, N64CONTROLLER & Controller);

private:
    CInputSettings(const CInputSettings&);
    CInputSettings& operator=(const CInputSettings&);

    static BUTTON StrToButton(const char * Buffer);
    static std::string ButtonToStr(const BUTTON & Button);
    static std::string GUIDtoString(const GUID & guid);

    void RegisterSettings(void);
};

extern CInputSettings * g_Settings;

void SetupInputSettings(void);
void CleanupInputSettings(void);
