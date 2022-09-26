#pragma once

class CGuiSettings
{
protected:
    CGuiSettings();
    virtual ~CGuiSettings();

    static inline bool bCPURunning(void)
    {
        return m_bCPURunning;
    }
    static inline bool bAutoSleep(void)
    {
        return m_bAutoSleep;
    }

private:
    static void RefreshSettings(void *);

    static bool m_bCPURunning;
    static bool m_bAutoSleep;

    static int m_RefCount;
};
