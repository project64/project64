/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
package emu.project64;

import android.content.Context;
import android.content.res.Resources;

public class Project64Application extends android.app.Application
{
    private static Project64Application m_instance;

    @Override
    public void onCreate()
    {
        super.onCreate();
        m_instance = this;

    }
    
    /**
     * @return the main context of the Application
     */
    public static Context getAppContext()
    {
        return m_instance;
    }
    
    /**
     * @return the main resources from the Application
     */
    public static Resources getAppResources()
    {
        return m_instance.getResources();
    }
}
