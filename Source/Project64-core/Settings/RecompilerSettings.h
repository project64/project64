#pragma once

#include <Project64-core/N64System/N64Types.h>

class CRecompilerSettings
{
public:
    CRecompilerSettings();
    virtual ~CRecompilerSettings();

    static inline bool bShowRecompMemSize(void) { return m_bShowRecompMemSize; }

private:
    static void StaticRefreshSettings(CRecompilerSettings * _this)
    {
        _this->RefreshSettings();
    }

    void RefreshSettings(void);

    //Settings that can be changed on the fly
    static bool m_bShowRecompMemSize;

    static int32_t m_RefCount;
};
