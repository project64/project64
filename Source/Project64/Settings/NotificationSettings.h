/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
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
    inline bool InFullScreen(void) const { return m_bInFullScreen; }
};
