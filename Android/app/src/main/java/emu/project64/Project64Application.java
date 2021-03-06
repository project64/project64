package emu.project64;

import android.content.Context;
import android.content.res.Resources;
import com.google.android.gms.analytics.GoogleAnalytics;
import com.google.android.gms.analytics.Tracker;

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

    private Tracker tracker;
    synchronized public Tracker getDefaultTracker()
    {
    	if (tracker == null) 
    	{ 
    		GoogleAnalytics analytics = GoogleAnalytics.getInstance(this);
    		tracker = analytics.newTracker(R.xml.analytics);
    	}
    	return tracker;
    }
}
