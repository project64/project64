/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "ScreenResolution.h"
#include "settings.h"
#include "trace.h"

struct ResolutionInfo
{
    unsigned int dwW, dwH, dwF;

    ResolutionInfo() : dwW(0), dwH(0), dwF(0) {}

    ResolutionInfo(unsigned int _w, unsigned int _h, unsigned int _f) : dwW(_w), dwH(_h), dwF(_f) {}

    bool operator == (const ResolutionInfo & _other) const
    {
        if (dwW != _other.dwW)
            return false;
        if (dwH != _other.dwH)
            return false;
        if (dwF != _other.dwF)
            return false;
        return true;
    }

    bool operator != (const ResolutionInfo & _other) const
    {
        return !(operator==(_other));
    }

    void toString(char * _str) const
    {
        if (dwF > 0)
            sprintf(_str, "%ix%i 32bpp %iHz", dwW, dwH, dwF);
        else
            sprintf(_str, "%ix%i 32bpp", dwW, dwH);
    }
};

class FullScreenResolutions
{
public:
    FullScreenResolutions() : dwNumResolutions(0), aResolutions(0), aResolutionsStr(0) {}
    ~FullScreenResolutions();

    void getResolution(uint32_t _idx, uint32_t * _width, uint32_t * _height, uint32_t * _frequency = 0)
    {
        WriteTrace(TraceResolution, TraceDebug, "_idx: %d", _idx);
        if (dwNumResolutions == 0)
        {
            init();
        }
        if (_idx >= dwNumResolutions)
        {
            WriteTrace(TraceGlitch, TraceError, "NumResolutions = %d", dwNumResolutions);
            _idx = 0;
        }
        *_width = (uint32_t)aResolutions[_idx].dwW;
        *_height = (uint32_t)aResolutions[_idx].dwH;
        if (_frequency != 0)
        {
            *_frequency = (uint32_t)aResolutions[_idx].dwF;
        }
    }

    int getCurrentResolutions(void)
    {
        if (dwNumResolutions == 0)
        {
            init();
        }
        return currentResolutions;
    }

    char ** getResolutionsList(int32_t * Size)
    {
        if (dwNumResolutions == 0)
        {
            init();
        }
        *Size = (int32_t)dwNumResolutions;
        return aResolutionsStr;
    }

    bool changeDisplaySettings(uint32_t _resolution);

private:
    void init();
    unsigned int dwNumResolutions;
    ResolutionInfo * aResolutions;
    char ** aResolutionsStr;
    int currentResolutions;
};

FullScreenResolutions::~FullScreenResolutions()
{
    for (unsigned int i = 0; i < dwNumResolutions; i++)
    {
        delete[] aResolutionsStr[i];
        aResolutionsStr[i] = NULL;
    }
    if (aResolutionsStr)
    {
        delete[] aResolutionsStr;
        aResolutionsStr = NULL;
    }
    if (aResolutions)
    {
        delete[] aResolutions;
        aResolutions = NULL;
    }
}

void FullScreenResolutions::init()
{
#ifdef _WIN32
    currentResolutions = -1;
    DEVMODE enumMode, currentMode;
    int iModeNum = 0;
    memset(&enumMode, 0, sizeof(DEVMODE));

    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &currentMode);

    ResolutionInfo prevInfo;
    while (EnumDisplaySettings(NULL, iModeNum++, &enumMode) != 0)
    {
        ResolutionInfo curInfo(enumMode.dmPelsWidth, enumMode.dmPelsHeight, enumMode.dmDisplayFrequency);
        if (enumMode.dmBitsPerPel == 32 && curInfo != prevInfo)
        {
            dwNumResolutions++;
            prevInfo = curInfo;
        }
    }

    aResolutions = new ResolutionInfo[dwNumResolutions];
    aResolutionsStr = new char*[dwNumResolutions];
    iModeNum = 0;
    int current = 0;
    char smode[256];
    memset(&enumMode, 0, sizeof(DEVMODE));
    memset(&prevInfo, 0, sizeof(ResolutionInfo));
    while (EnumDisplaySettings(NULL, iModeNum++, &enumMode) != 0)
    {
        ResolutionInfo curInfo(enumMode.dmPelsWidth, enumMode.dmPelsHeight, enumMode.dmDisplayFrequency);
        if (enumMode.dmBitsPerPel == 32 && curInfo != prevInfo)
        {
            if (enumMode.dmPelsHeight == currentMode.dmPelsHeight && enumMode.dmPelsWidth == currentMode.dmPelsWidth)
            {
                currentResolutions = current;
            }
            aResolutions[current] = curInfo;
            curInfo.toString(smode);
            aResolutionsStr[current] = new char[strlen(smode) + 1];
            strcpy(aResolutionsStr[current], smode);
            prevInfo = curInfo;
            current++;
        }
    }
#endif
}

bool FullScreenResolutions::changeDisplaySettings(uint32_t _resolution)
{
#ifdef _WIN32
    uint32_t width, height, frequency;
    getResolution(_resolution, &width, &height, &frequency);
    ResolutionInfo info(width, height, frequency);
    DEVMODE enumMode;
    int iModeNum = 0;
    memset(&enumMode, 0, sizeof(DEVMODE));
    while (EnumDisplaySettings(NULL, iModeNum++, &enumMode) != 0)
    {
        ResolutionInfo curInfo(enumMode.dmPelsWidth, enumMode.dmPelsHeight, enumMode.dmDisplayFrequency);
        if (enumMode.dmBitsPerPel == 32 && curInfo == info) {
            bool bRes = ChangeDisplaySettings(&enumMode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;
            WriteTrace(TraceGlitch, TraceDebug, "width=%d, height=%d, freq=%d %s\r\n", enumMode.dmPelsWidth, enumMode.dmPelsHeight, enumMode.dmDisplayFrequency, bRes ? "Success" : "Failed");
            return bRes;
        }
    }
    return false;
#else // _WIN32
    return false;
#endif // _WIN32
}

FullScreenResolutions g_FullScreenResolutions;

uint32_t GetFullScreenResWidth(uint32_t index)
{
    uint32_t _width, _height;
    g_FullScreenResolutions.getResolution(index, &_width, &_height);
    return _width;
}

uint32_t GetFullScreenResHeight(uint32_t index)
{
    uint32_t _width, _height;
    g_FullScreenResolutions.getResolution(index, &_width, &_height);
    return _height;
}

bool EnterFullScreen(uint32_t index)
{
    return g_FullScreenResolutions.changeDisplaySettings(index);
}

int GetCurrentResIndex(void)
{
    return g_FullScreenResolutions.getCurrentResolutions();
}

char ** grQueryResolutionsExt(int32_t * Size)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    return g_FullScreenResolutions.getResolutionsList(Size);
}

uint32_t grWrapperFullScreenResolutionExt(uint32_t * width, uint32_t * height)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    g_FullScreenResolutions.getResolution(g_settings->wrpResolution, width, height);
    return g_settings->wrpResolution;
}
