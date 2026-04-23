#pragma once

// Cached UI / runtime toggles from CSettings (refreshed on change).
struct SystemSettings
{
    bool basicMode;
    bool displayFrameRate;
    bool showCpuPer;
    bool showDlistAListCount;
    bool limitFps;
    bool updateControllerOnRefresh;
};

extern SystemSettings g_SystemSettings;

void SetupSystemSettings(void);
void ShutdownSystemSettings(void);
void RefreshSystemSettings(void);
