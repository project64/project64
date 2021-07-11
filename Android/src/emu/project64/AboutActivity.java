package emu.project64;

import java.util.Arrays;
import java.util.List;

import emu.project64.jni.LanguageStringID;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;
import emu.project64.util.Strings;
import emu.project64.util.Utility;
import android.os.Bundle;
import android.support.design.widget.TabLayout;
import android.support.v4.view.ViewPager;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.Html;
import android.view.MenuItem;
import android.view.View;
import android.webkit.WebView;
import android.widget.TextView;
import android.widget.Toast;

public class AboutActivity extends AppCompatActivity 
{
    public final static int MODE_ABOUT = 0;
    public final static int MODE_LICENCE = 1;
    public final static int MODE_TOTAL = 2; // Number of about pages
    private int m_title_clicks = 0;

    @Override
    public void onCreate( Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );
        setContentView( R.layout.about_activity ); 
        
        // Add the tool bar to the activity (which supports the fancy menu/arrow animation)
        Toolbar toolbar = (Toolbar) findViewById( R.id.toolbar );
        toolbar.setTitle( getString(R.string.app_name) + " " + NativeExports.appVersion() );
        setSupportActionBar( toolbar );
        ActionBar actionbar = getSupportActionBar();
        
        if (AndroidDevice.IS_ICE_CREAM_SANDWICH)
        {
            actionbar.setHomeButtonEnabled(true);
            actionbar.setDisplayHomeAsUpEnabled(true);
    	}
        
        View aboutMain = findViewById(R.id.about_main);
        WebView webView = (WebView)findViewById(R.id.webview);

        List<View> lists = Arrays.asList(aboutMain, webView);
        String[] titles = new String[] {Strings.GetString(LanguageStringID.ANDROID_ABOUT), Strings.GetString(LanguageStringID.ANDROID_ABOUT_LICENCE)};
        ViewPager viewPager = (ViewPager) findViewById(R.id.pager);
        viewPager.setOffscreenPageLimit(MODE_TOTAL-1);
        viewPager.setAdapter(new AboutPagerAdapter(lists, titles));
        
        TabLayout tabLayout = (TabLayout) findViewById(R.id.sliding_tabs);
        tabLayout.setupWithViewPager(viewPager);

        TextView link = (TextView)findViewById(R.id.main_link);
        link.setText(Html.fromHtml(getString(R.string.about_link)));
        
        TextView app_name_full = (TextView)findViewById(R.id.app_name_full);
        app_name_full.setText(Strings.GetString(LanguageStringID.ANDROID_ABOUT_APP_NAME));

        TextView about_text = (TextView)findViewById(R.id.about_text);
        about_text.setText(Strings.GetString(LanguageStringID.ANDROID_ABOUT_TEXT));

        TextView Project64_authors = (TextView)findViewById(R.id.Project64_authors);
        Project64_authors.setText(Strings.GetString(LanguageStringID.ANDROID_ABOUT_PJ64_AUTHORS));

        webView.loadData(Utility.readAsset("licence.htm", ""), "text/html", "UTF8");
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) 
    {
        switch (item.getItemId()) 
        {
        case android.R.id.home:
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
    
    public void onAppNameClick(View v) 
    {
    	m_title_clicks += 1;
    	if (m_title_clicks == 6)
    	{
    		NativeExports.SettingsSaveBool(SettingsID.UserInterface_BasicMode.getValue(), false);
    		Toast.makeText(this, "Advanced Mode Enabled", Toast.LENGTH_SHORT).show();
    	}
    }  
}
