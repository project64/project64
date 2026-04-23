#pragma once

struct SystemSettings
{
    bool basicMode;
    bool displayFrameRate;
    bool showCpuPer;
    bool showDlistAListCount;
    bool limitFps;
    bool updateControllerOnRefresh;
    bool showRecompMemSize;
};

extern SystemSettings g_SystemSettings;

void SetupSystemSettings(void);
void ShutdownSystemSettings(void);
void RefreshSystemSettings(void);
