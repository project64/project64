/****************************************************************************
*                                                                           *
* Project64-video - A Nintendo 64 gfx plugin.                               *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "ScreenResolution.h"
#include "settings.h"
#include "trace.h"

#ifdef ANDROID
#include <Common/StdString.h>
#include <vector>
#endif

struct ResolutionInfo
{
    ResolutionInfo(const char * name = NULL, uint32_t width = 0, uint32_t height = 0, uint32_t frequency = 0, bool default_res = false) :
        m_name(name ? name : ""),
        m_width(width),
        m_height(height),
        m_frequency(frequency),
        m_default_res(default_res)
    {
    }

    const char * Name(void) const { return m_name.c_str(); }
    uint32_t width(void) const { return m_width; }
    uint32_t height(void) const { return m_height; }
    uint32_t frequency(void) const { return m_frequency; }
    bool DefaultRes(void) const { return m_default_res; }

    bool operator == (const ResolutionInfo& rRes) const
    {
        return m_width == rRes.m_width && m_height == rRes.m_height && m_frequency == rRes.m_frequency;
    }
    bool operator != (const ResolutionInfo& rRes) const
    {
        return !(*this == rRes);
    }
private:
    uint32_t m_width, m_height, m_frequency;
    std::string m_name;
    bool m_default_res;
};

#ifdef ANDROID
std::vector<ResolutionInfo> g_resolutions;
#else
static ResolutionInfo g_resolutions[] =
{
    { "320x200", 320, 200, 0, false },
    { "320x240", 320, 240, 0, false },
    { "400x256", 400, 256, 0, false },
    { "512x384", 512, 384, 0, false },
    { "640x200", 640, 200, 0, false },
    { "640x350", 640, 350, 0, false },
    { "640x400", 640, 400, 0, false },
    { "640x480", 640, 480, 0, true },
    { "800x600", 800, 600, 0, false },
    { "960x720", 960, 720, 0, false },
    { "856x480", 856, 480, 0, false },
    { "512x256", 512, 256, 0, false },
    { "1024x768", 1024, 768, 0, false },
    { "1280x1024", 1280, 1024, 0, false },
    { "1600x1200", 1600, 1200, 0, false },
    { "400x300", 400, 300, 0, false },
    { "1152x864", 1152, 864, 0, false },
    { "1280x960", 1280, 960, 0, false },
    { "1600x1024", 1600, 1024, 0, false },
    { "1792x1344", 1792, 1344, 0, false },
    { "1856x1392", 1856, 1392, 0, false },
    { "1920x1440", 1920, 1440, 0, false },
    { "2048x1536", 2048, 1536, 0, false },
    { "2048x2048", 2048, 2048, 0, false },
};
#endif

#ifdef ANDROID
void UpdateScreenResolution(int ScreenWidth, int ScreenHeight)
{
    WriteTrace(TraceResolution, TraceError, "aspectmode: %d", g_settings->aspectmode());
    g_resolutions.clear();
    switch (g_settings->aspectmode())
    {
    case CSettings::Aspect_4x3:
        g_resolutions.push_back(ResolutionInfo(stdstr_f("%dx%d", ScreenHeight * 4 / 3, ScreenHeight).c_str(), ScreenHeight * 4 / 3, ScreenHeight, 0, true));
        g_resolutions.push_back(ResolutionInfo("960x720", 960, 720, 0, false));
        g_resolutions.push_back(ResolutionInfo("800x600", 800, 600, 0, false));
        g_resolutions.push_back(ResolutionInfo("640x480", 640, 480, 0, false));
        g_resolutions.push_back(ResolutionInfo("480x360", 480, 360, 0, false));
        g_resolutions.push_back(ResolutionInfo("320x240", 320, 240, 0, false));
        break;
    case CSettings::Aspect_16x9:
        g_resolutions.push_back(ResolutionInfo(stdstr_f("%dx%d", ScreenHeight * 16 / 9, ScreenHeight).c_str(), ScreenHeight * 16 / 9, ScreenHeight, 0, true));
        g_resolutions.push_back(ResolutionInfo("1280x720", 1280, 720, 0, false));
        g_resolutions.push_back(ResolutionInfo("1067x600", 1067, 600, 0, false));
        g_resolutions.push_back(ResolutionInfo("854x480", 854, 480, 0, false));
        g_resolutions.push_back(ResolutionInfo("640x360", 640, 360, 0, false));
        g_resolutions.push_back(ResolutionInfo("426x240", 426, 240, 0, false));
        break;
    case CSettings::Aspect_Original:
        g_resolutions.push_back(ResolutionInfo("Original", ScreenWidth, ScreenHeight, 0, true));
        break;
    case CSettings::Aspect_Stretch:
    default: //stretch
        g_resolutions.push_back(ResolutionInfo(stdstr_f("%dx%d", ScreenWidth, ScreenHeight).c_str(), ScreenWidth, ScreenHeight, 0, true));
        break;
    }
}
#endif

uint32_t GetScreenResolutionCount()
{
#ifdef ANDROID
    return g_resolutions.size();
#else
    return sizeof(g_resolutions) / sizeof(g_resolutions[0]);
#endif
}

const char * GetScreenResolutionName(uint32_t index)
{
    if (index < GetScreenResolutionCount())
    {
        return g_resolutions[index].Name();
    }
    return "unknown";
}

uint32_t GetDefaultScreenRes()
{
    for (uint32_t i = 0, n = GetScreenResolutionCount(); i < n; i++)
    {
        if (g_resolutions[i].DefaultRes())
        {
            return i;
        }
    }
    return 0;
}

uint32_t GetScreenResWidth(uint32_t index)
{
    if (index < GetScreenResolutionCount())
    {
        return g_resolutions[index].width();
    }
    return 0;
}

uint32_t GetScreenResHeight(uint32_t index)
{
    if (index < GetScreenResolutionCount())
    {
        return g_resolutions[index].height();
    }
    return 0;
}

class FullScreenResolutions
{
public:
    FullScreenResolutions() :
        m_dwNumResolutions(0),
        m_aResolutions(0),
        m_aResolutionsStr(0)
    {
    }
    ~FullScreenResolutions();

    void getResolution(uint32_t _idx, uint32_t * _width, uint32_t * _height, uint32_t * _frequency = 0)
    {
        WriteTrace(TraceResolution, TraceDebug, "_idx: %d", _idx);
        if (m_dwNumResolutions == 0)
        {
            init();
        }
        if (_idx >= m_dwNumResolutions)
        {
            WriteTrace(TraceGlitch, TraceError, "NumResolutions = %d", m_dwNumResolutions);
            _idx = 0;
        }
        *_width = (uint32_t)m_aResolutions[_idx].width();
        *_height = (uint32_t)m_aResolutions[_idx].height();
        if (_frequency != 0)
        {
            *_frequency = (uint32_t)m_aResolutions[_idx].frequency();
        }
    }

    int getCurrentResolutions(void)
    {
        if (m_dwNumResolutions == 0)
        {
            init();
        }
        return m_currentResolutions;
    }

    const char ** getResolutionsList(int32_t * Size)
    {
        if (m_dwNumResolutions == 0)
        {
            init();
        }
        *Size = (int32_t)m_dwNumResolutions;
        return (const char **)m_aResolutionsStr;
    }

    bool changeDisplaySettings(uint32_t _resolution);

private:
    void init();
    unsigned int m_dwNumResolutions;
    ResolutionInfo * m_aResolutions;
    char ** m_aResolutionsStr;
    int m_currentResolutions;
};

FullScreenResolutions::~FullScreenResolutions()
{
    for (unsigned int i = 0; i < m_dwNumResolutions; i++)
    {
        delete[] m_aResolutionsStr[i];
        m_aResolutionsStr[i] = NULL;
    }
    if (m_aResolutionsStr)
    {
        delete[] m_aResolutionsStr;
        m_aResolutionsStr = NULL;
    }
    if (m_aResolutions)
    {
        delete[] m_aResolutions;
        m_aResolutions = NULL;
    }
}

void FullScreenResolutions::init()
{
#ifdef _WIN32
    m_currentResolutions = -1;
    DEVMODE enumMode, currentMode;
    int iModeNum = 0;
    memset(&enumMode, 0, sizeof(DEVMODE));

    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &currentMode);

    ResolutionInfo prevInfo;
    while (EnumDisplaySettings(NULL, iModeNum++, &enumMode) != 0)
    {
        ResolutionInfo curInfo("", enumMode.dmPelsWidth, enumMode.dmPelsHeight, enumMode.dmDisplayFrequency);
        if (enumMode.dmBitsPerPel == 32 && curInfo != prevInfo)
        {
            m_dwNumResolutions++;
            prevInfo = curInfo;
        }
    }

    m_aResolutions = new ResolutionInfo[m_dwNumResolutions];
    m_aResolutionsStr = new char*[m_dwNumResolutions];
    iModeNum = 0;
    int current = 0;
    char smode[256];
    memset(&enumMode, 0, sizeof(DEVMODE));
    memset(&prevInfo, 0, sizeof(ResolutionInfo));
    while (EnumDisplaySettings(NULL, iModeNum++, &enumMode) != 0)
    {
        ResolutionInfo curInfo(NULL, enumMode.dmPelsWidth, enumMode.dmPelsHeight, enumMode.dmDisplayFrequency);
        if (enumMode.dmBitsPerPel == 32 && curInfo != prevInfo)
        {
            if (enumMode.dmPelsHeight == currentMode.dmPelsHeight && enumMode.dmPelsWidth == currentMode.dmPelsWidth)
            {
                m_currentResolutions = current;
            }
            m_aResolutions[current] = curInfo;
            sprintf(smode, curInfo.frequency() > 0 ? "%ix%i 32bpp %iHz" : "%ix%i 32bpp", curInfo.width(), curInfo.height(), curInfo.frequency());
            m_aResolutionsStr[current] = new char[strlen(smode) + 1];
            strcpy(m_aResolutionsStr[current], smode);
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
    ResolutionInfo info(NULL, width, height, frequency);
    DEVMODE enumMode;
    int iModeNum = 0;
    memset(&enumMode, 0, sizeof(DEVMODE));
    while (EnumDisplaySettings(NULL, iModeNum++, &enumMode) != 0)
    {
        ResolutionInfo curInfo(NULL, enumMode.dmPelsWidth, enumMode.dmPelsHeight, enumMode.dmDisplayFrequency);
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

#ifndef ANDROID
const char ** getFullScreenResList(int32_t * Size)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    return g_FullScreenResolutions.getResolutionsList(Size);
}

uint32_t getFullScreenRes(uint32_t * width, uint32_t * height)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    g_FullScreenResolutions.getResolution(g_settings->FullScreenRes(), width, height);
    return g_settings->FullScreenRes();
}
#endif