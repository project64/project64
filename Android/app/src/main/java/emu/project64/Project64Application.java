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
    
    public static Context getAppContext()
    {
        return m_instance;
    }
    
    public static Resources getAppResources()
    {
        return m_instance.getResources();
    }
}
