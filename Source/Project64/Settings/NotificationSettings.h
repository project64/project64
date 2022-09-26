#pragma once

class CNotificationSettings
{
    static void StaticRefreshSettings(CNotificationSettings * _this)
    {
        _this->RefreshSettings();
    }

    void RefreshSettings(void);

    static bool m_bInFullScreen;

protected:
    CNotificationSettings();
    virtual ~CNotificationSettings();

    void RegisterNotifications(void);
    inline bool InFullScreen(void) const
    {
        return m_bInFullScreen;
    }
};
