package emu.project64;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.analytics.Tracker;

import emu.project64.R;
import emu.project64.dialog.ProgressDialog;
import emu.project64.game.GameActivity;
import emu.project64.game.GameActivityXperiaPlay;
import emu.project64.inAppPurchase.IabBroadcastReceiver;
import emu.project64.inAppPurchase.IabBroadcastReceiver.IabBroadcastListener;
import emu.project64.inAppPurchase.IabHelper;
import emu.project64.inAppPurchase.IabHelper.IabAsyncInProgressException;
import emu.project64.inAppPurchase.IabResult;
import emu.project64.inAppPurchase.Inventory;
import emu.project64.inAppPurchase.Purchase;
import emu.project64.jni.LanguageStringID;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;
import emu.project64.jni.SystemEvent;
import emu.project64.jni.UISettingID;
import emu.project64.settings.GameSettingsActivity;
import emu.project64.settings.SettingsActivity;
import emu.project64.util.Strings;
import emu.project64.util.Utility;
import android.annotation.TargetApi;
import android.app.Activity;
import androidx.appcompat.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.SpannableStringBuilder;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.webkit.WebView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListAdapter;
import android.widget.TextView;
import android.widget.TextView.BufferType;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.res.ResourcesCompat;
import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

public class GalleryActivity extends AppCompatActivity implements IabBroadcastListener
{
    //Progress dialog for ROM scan
    private ProgressDialog mProgress = null;

    // Widgets
    private RecyclerView mGridView;
    private DrawerLayout mDrawerLayout;
    private ActionBarDrawerToggle mDrawerToggle;
    private MenuListView mDrawerList;

    // Resizable gallery thumb nails
    public int galleryWidth;
    public int galleryMaxWidth;
    public int galleryHalfSpacing;
    public int galleryColumns = 2;

    // Misc.
    private static List<GalleryItem> mGalleryItems = new ArrayList<GalleryItem>();
    private static List<GalleryItem> mRecentItems = new ArrayList<GalleryItem>();
    private static GalleryActivity mActiveGalleryActivity = null;

    // The IAB helper object
    IabHelper mIabHelper;
    private boolean mPj64Supporter = false;

    // Provides purchase notification while this app is running
    IabBroadcastReceiver mBroadcastReceiver;

    public static final int GAME_DIR_REQUEST_CODE = 1;
    static final String SKU_SAVESUPPORT = "save_support";
    static final String SKU_PJ64SUPPORTOR_2 = "supportproject64_2";
    static final String SKU_PJ64SUPPORTOR_5 = "supportproject64_5";
    static final String SKU_PJ64SUPPORTOR_8 = "supportproject64_8";
    static final String SKU_PJ64SUPPORTOR_10 = "supportproject64_10";

    // (arbitrary) request code for the purchase flow
    static final int RC_REQUEST = 10001;
    static final int RC_SETTINGS = 10002;

    @Override
    protected void onNewIntent( Intent intent )
    {
        // If the activity is already running and is launched again (e.g. from a file manager app),
        // the existing instance will be reused rather than a new one created. This behavior is
        // specified in the manifest (launchMode = singleTask). In that situation, any activities
        // above this on the stack (e.g. GameActivity, GamePrefsActivity) will be destroyed
        // gracefully and onNewIntent() will be called on this instance. onCreate() will NOT be
        // called again on this instance. Currently, the only info that may be passed via the intent
        // is the selected game path, so we only need to refresh that aspect of the UI. This will
        // happen anyhow in onResume(), so we don't really need to do much here.
        super.onNewIntent( intent );

        // Only remember the last intent used
        setIntent( intent );
    }

    @Override
    protected void onCreate( Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );
        mActiveGalleryActivity = this;

        mIabHelper = new IabHelper(this, "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAnfHFIq+X0oIvV+bwcvdqQv5GmpWLL6Bw8xE6MLFzXzUGUIUZBwQS6Cz5IC0UM76ujPDPqQPeGy/8oq/bswB5pHCz2iS4ySGalzFfYfeIDklOe+R1pLEqmHuwsR5o4b8rLePLGmUI7hA0kozOTb0i+epANV3Pj63i5XFZLA7RMi5I+YysoE9Fob6kCx0kb02AATacF0OXI9paE1izvsHhZcOIrT4TRMbGlZjBVE/pcJtoBDh33QKz/JBOXWvwnh+efqhVsq/UfA6jYI+U4Z4tsnWhem8DB6Kqj5EhClC6qCPmkBFiOabyKaqhI/urBtYOwxkW9erwtA6OcDoHm5J/JwIDAQAB");

        // enable debug logging (for a production application, you should set this to false).
        mIabHelper.enableDebugLogging(true);

        Log.d("GalleryActivity", "Starting setup.");
        mIabHelper.startSetup(new IabHelper.OnIabSetupFinishedListener()
        {
            public void onIabSetupFinished(IabResult result)
            {
                Log.d("GalleryActivity", "onIabSetupFinished.");

                if (!result.isSuccess())
                {
                    // Oh noes, there was a problem.
                    Log.d("GalleryActivity", "Problem setting up in-app billing: " + result);
                    // complain("Problem setting up in-app billing: " + result);
                    mPj64Supporter = true;
                    return;
                }
                // Have we been disposed of in the meantime? If so, quit.
                if (mIabHelper == null) return;

                // Important: Dynamically register for broadcast messages about updated purchases.
                // We register the receiver here instead of as a <receiver> in the Manifest
                // because we always call getPurchases() at startup, so therefore we can ignore
                // any broadcasts sent while the app isn't running.
                // Note: registering this listener in an Activity is a bad idea, but is done here
                // because this is a SAMPLE. Regardless, the receiver must be registered after
                // IabHelper is setup, but before first call to getPurchases().
                mBroadcastReceiver = new IabBroadcastReceiver(GalleryActivity.this);
                IntentFilter broadcastFilter = new IntentFilter(IabBroadcastReceiver.ACTION);
                registerReceiver(mBroadcastReceiver, broadcastFilter);

                // IAB is fully set up. Now, let's get an inventory of stuff we own.
                Log.d("GalleryActivity", "Setup successful. Querying inventory.");
                try
                {
                    mIabHelper.queryInventoryAsync(mGotInventoryListener);
                }
                catch (IabAsyncInProgressException e)
                {
                    //complain("Error querying inventory. Another async operation in progress.");
                }
            }
        });

        // Lay out the content
        setContentView( R.layout.gallery_activity );
        mGridView = (RecyclerView) findViewById( R.id.gridview );
        mProgress = new ProgressDialog( null, this, getString( R.string.scanning_title ), "", getString( R.string.toast_pleaseWait ), false );

        // Load Cached Rom List
        NativeExports.LoadRomList();
        refreshGrid();

        // Update the grid layout
        galleryMaxWidth = (int) getResources().getDimension( R.dimen.galleryImageWidth );
        galleryHalfSpacing = (int) getResources().getDimension( R.dimen.galleryHalfSpacing );

        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics( metrics );

        int width = metrics.widthPixels - galleryHalfSpacing * 2;
        galleryColumns = (int) Math.ceil( width * 1.0 / ( galleryMaxWidth + galleryHalfSpacing * 2 ) );
        galleryWidth = width / galleryColumns - galleryHalfSpacing * 2;

        GridLayoutManager layoutManager = (GridLayoutManager) mGridView.getLayoutManager();
        layoutManager.setSpanCount( galleryColumns );

        // Add the toolbar to the activity (which supports the fancy menu/arrow animation)
        Toolbar toolbar = (Toolbar) findViewById( R.id.toolbar );
        toolbar.setTitle( R.string.app_name );
        setSupportActionBar( toolbar );

        // Configure the navigation drawer
        mDrawerLayout = (DrawerLayout) findViewById( R.id.drawerLayout );
        mDrawerToggle = new ActionBarDrawerToggle( this, mDrawerLayout, toolbar, 0, 0 );
        mDrawerLayout.addDrawerListener( mDrawerToggle );

        // Configure the list in the navigation drawer
        mDrawerList = (MenuListView) findViewById( R.id.drawerNavigation );
        mDrawerList.setMenuResource( R.menu.gallery_drawer );
        // Handle menu item selections
        mDrawerList.setOnClickListener( new MenuListView.OnClickListener()
        {
            @Override
            public void onClick( MenuItem menuItem )
            {
                GalleryActivity.this.onOptionsItemSelected( menuItem );
            }
        });
        UpdateLanguage();

        int RunCount = NativeExports.UISettingsLoadDword(UISettingID.AppInfo_RunCount.getValue()) + 1;
        if (RunCount < 1) { RunCount = 1; }
        NativeExports.UISettingsSaveDword(UISettingID.AppInfo_RunCount.getValue(), RunCount);
        Log.d("GalleryActivity", "ShowSupportWindow RunCount = " + RunCount);
        if (RunCount == 5 || RunCount == 10)
        {
            ShowReviewOptions();
        }
    }

    void UpdateLanguage()
    {
        Strings.SetMenuTitle(mDrawerList.getMenu(), R.id.menuItem_settings, LanguageStringID.ANDROID_SETTINGS);
        Strings.SetMenuTitle(mDrawerList.getMenu(), R.id.menuItem_discord, LanguageStringID.ANDROID_DISCORD);
        Strings.SetMenuTitle(mDrawerList.getMenu(), R.id.menuItem_reportBug, LanguageStringID.ANDROID_REPORT_BUG);
        Strings.SetMenuTitle(mDrawerList.getMenu(), R.id.menuItem_review, LanguageStringID.ANDROID_REVIEW_PJ64);
        Strings.SetMenuTitle(mDrawerList.getMenu(), R.id.menuItem_support, LanguageStringID.ANDROID_SUPPORT_PJ64);
        Strings.SetMenuTitle(mDrawerList.getMenu(), R.id.menuItem_about, LanguageStringID.ANDROID_ABOUT);
    }

    // Listener that's called when we finish querying the items and subscriptions we own
    IabHelper.QueryInventoryFinishedListener mGotInventoryListener = new IabHelper.QueryInventoryFinishedListener()
    {
        public void onQueryInventoryFinished(IabResult result, Inventory inventory)
        {
            Log.d("GalleryActivity", "Query inventory finished.");

            // Have we been disposed of in the meantime? If so, quit.
            if (mIabHelper == null) return;

            // Is it a failure?
            if (result.isFailure())
            {
                //complain("Failed to query inventory: " + result);
                return;
            }

            Log.d("GalleryActivity", "Query inventory was successful.");

            /*
             * Check for items we own. Notice that for each purchase, we check
             * the developer payload to see if it's correct! See
             * verifyDeveloperPayload().
             */

            /*IabHelper.OnConsumeFinishedListener listener = new IabHelper.OnConsumeFinishedListener()
            {
                @Override
                public
                void onConsumeFinished(Purchase purchase, IabResult result)
                {
                    Log.d("GalleryActivity", "SKU_SAVESUPPORT consumed");
                }
            };
            try {
                mIabHelper.consumeAsync(inventory.getPurchase(SKU_SAVESUPPORT), listener);
            } catch (IabAsyncInProgressException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }*/
            Purchase ItemPurchase = inventory.getPurchase(SKU_SAVESUPPORT);
            Log.d("GalleryActivity", "Purchased SKU_SAVESUPPORT " + (ItemPurchase!= null ? "Yes" : "No"));
            if (ItemPurchase != null)
            {
                mPj64Supporter = true;
            }
            if (!mPj64Supporter)
            {
                ItemPurchase = inventory.getPurchase(SKU_PJ64SUPPORTOR_2);
                Log.d("GalleryActivity", "Purchased SKU_PJ64SUPPORTOR_2 " + (ItemPurchase != null ? "Yes" : "No"));
                if (ItemPurchase != null)
                {
                    mPj64Supporter = true;
                }
            }
            if (!mPj64Supporter)
            {
                ItemPurchase = inventory.getPurchase(SKU_PJ64SUPPORTOR_5);
                Log.d("GalleryActivity", "Purchased SKU_PJ64SUPPORTOR_5 " + (ItemPurchase != null ? "Yes" : "No"));
                if (ItemPurchase != null)
                {
                    mPj64Supporter = true;
                }
            }
            if (!mPj64Supporter)
            {
                ItemPurchase = inventory.getPurchase(SKU_PJ64SUPPORTOR_8);
                Log.d("GalleryActivity", "Purchased SKU_PJ64SUPPORTOR_8 " + (ItemPurchase != null ? "Yes" : "No"));
                if (ItemPurchase != null)
                {
                    mPj64Supporter = true;
                }
            }
            if (!mPj64Supporter)
            {
                ItemPurchase = inventory.getPurchase(SKU_PJ64SUPPORTOR_10);
                Log.d("GalleryActivity", "Purchased SKU_PJ64SUPPORTOR_10 " + (ItemPurchase != null ? "Yes" : "No"));
                if (ItemPurchase != null)
                {
                    mPj64Supporter = true;
                }
            }
        }
    };

    void alert(String message)
    {
        AlertDialog.Builder bld = new AlertDialog.Builder(this);
        bld.setMessage(message);
        bld.setNeutralButton("OK", null);
        Log.d("GalleryActivity", "Showing alert dialog: " + message);
        bld.create().show();
    }

    public void receivedBroadcast()
    {
        // Received a broadcast notification that the inventory of items has changed
        Log.d("GalleryActivity", "Received broadcast notification. Querying inventory.");
        try
        {
            mIabHelper.queryInventoryAsync(mGotInventoryListener);
        }
        catch (IabAsyncInProgressException e)
        {
            //complain("Error querying inventory. Another async operation in progress.");
        }
    }

    @Override
    protected void onPostCreate( Bundle savedInstanceState )
    {
        super.onPostCreate( savedInstanceState );
        mDrawerToggle.syncState();
    }

    @Override
    public void onConfigurationChanged( Configuration newConfig )
    {
        super.onConfigurationChanged( newConfig );
        mDrawerToggle.onConfigurationChanged( newConfig );
    }

    @Override
    public boolean onCreateOptionsMenu( Menu menu )
    {
        getMenuInflater().inflate( R.menu.gallery_activity, menu );
        Strings.SetMenuTitle(menu, R.id.menuItem_gameDir, LanguageStringID.ANDROID_GAMEDIR);

        return super.onCreateOptionsMenu( menu );
    }

    @Override
    public boolean onOptionsItemSelected( MenuItem item )
    {
        switch( item.getItemId() )
        {
        case R.id.menuItem_GameDir:
            Intent ScanRomsIntent = new Intent(this, ScanRomsActivity.class);
            startActivityForResult( ScanRomsIntent, GAME_DIR_REQUEST_CODE );
            return true;
            case R.id.menuItem_settings:
                Intent SettingsIntent = new Intent(this, SettingsActivity.class);
                startActivity( SettingsIntent );
                return true;
            case R.id.menuItem_discord:
                Intent ForumIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://discord.gg/sbYbnda"));
                startActivity(ForumIntent);
                return true;
            case R.id.menuItem_reportBug:
                Intent IssueIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/project64/project64/issues"));
                startActivity(IssueIntent);
                return true;
            case R.id.menuItem_review:
                ShowReviewOptions();
                return true;
            case R.id.menuItem_support:
                ShowPaymentOptions();
                return true;
            case R.id.menuItem_about:
                Intent AboutIntent = new Intent(this, AboutActivity.class);
                startActivity( AboutIntent );
                return true;
            default:
                return super.onOptionsItemSelected( item );
        }
    }

    private boolean HasAutoSave(File GameSaveDir)
    {
        if (!GameSaveDir.exists() || !GameSaveDir.isDirectory())
        {
            return false;
        }

        File[] fList = GameSaveDir.listFiles();
        for (File file : fList)
        {
            String extension = "";

            int i = file.getName().lastIndexOf('.');
            if (i > 0)
            {
                extension = file.getName().substring(i+1);
            }
            if (extension.equals("zip"))
            {
                i = file.getName().lastIndexOf('.', i - 1);
                if (i > 0)
                {
                    extension = file.getName().substring(i+1);
                }
            }
            if (extension.equals("pj.zip") ||
                extension.equals("pj"))
            {
                return true;
            }
        }
        return false;
    }

    private void StartGameMenu (boolean ShowSettings)
    {
        File InstantSaveDir = new File(NativeExports.SettingsLoadString(SettingsID.Directory_InstantSave.getValue()));
        final File GameSaveDir = new File(InstantSaveDir,NativeExports.SettingsLoadString(SettingsID.Game_UniqueSaveDir.getValue()));

        class Item
        {
            public final String text;
            public final int icon;
            public Item(String text, Integer icon)
            {
                this.text = text;
                this.icon = icon;
            }
            @Override
            public String toString()
            {
                return text;
            }
        }

        List<Item>menuItemLst = new ArrayList<Item>();
        menuItemLst.add(new Item("Resume from Native save", R.drawable.ic_controller));
        menuItemLst.add(new Item("Resume from Auto save", R.drawable.ic_play));
        menuItemLst.add(new Item("Restart", R.drawable.ic_refresh));
        if (ShowSettings && !NativeExports.SettingsLoadBool(SettingsID.UserInterface_BasicMode.getValue()))
        {
            menuItemLst.add(new Item("Settings", R.drawable.ic_sliders));
        }

        Item[] itemsDynamic = new Item[menuItemLst .size()];
        itemsDynamic = menuItemLst.toArray(itemsDynamic);

        final Item[] items = itemsDynamic;
        final File SaveDir = GameSaveDir;
        ListAdapter adapter = new ArrayAdapter<Item>( this, android.R.layout.select_dialog_item, android.R.id.text1, items)
        {
            public View getView(int position, View convertView, android.view.ViewGroup parent)
            {
                //Use super class to create the View
                View v = super.getView(position, convertView, parent);
                TextView tv = (TextView)v.findViewById(android.R.id.text1);

                // Get Drawable icon
                Drawable d = items[position].icon != 0 ? ResourcesCompat.getDrawable(getResources(), items[position].icon, null) : null;
                tv.setTextColor(Color.parseColor("#FFFFFF"));
                if (d != null)
                {
                    d.setColorFilter(Color.parseColor("#FFFFFF"), android.graphics.PorterDuff.Mode.SRC_ATOP);
                }
                if (!isEnabled(position))
                {
                    tv.setTextColor(Color.parseColor("#555555"));
                    if (d != null)
                    {
                        d.setColorFilter(Color.parseColor("#555555"), android.graphics.PorterDuff.Mode.SRC_ATOP);
                    }
                }

                //Put the image on the TextView
                tv.setCompoundDrawablesWithIntrinsicBounds(d, null, null, null);

                //Add margin between image and text (support various screen densities)
                int dp5 = (int) (5 * getResources().getDisplayMetrics().density + 0.5f);
                tv.setCompoundDrawablePadding(dp5);

                return v;
            }

            @Override
            public boolean areAllItemsEnabled()
            {
                return true;
            }

            @Override
            public boolean isEnabled(int position)
            {
                if (position == 1 && HasAutoSave(SaveDir) == false)
                {
                    return false;
                }
                if (position == 2 && SaveDir.exists() == false)
                {
                    return false;
                }
                return true;
            }
        };

        final Context finalContext = this;
        AlertDialog.Builder GameMenu = new AlertDialog.Builder(finalContext);
        GameMenu.setTitle(NativeExports.SettingsLoadString(SettingsID.Rdb_GoodName.getValue()));
        GameMenu.setAdapter(adapter, new DialogInterface.OnClickListener()
        {
            @Override
            public void onClick(DialogInterface dialog, int item)
            {
                if (item == 0)
                {
                    launchGameActivity(false);
                }
                else if (item == 1)
                {
                    launchGameActivity(true);
                }
                else if (item == 2)
                {
                    AlertDialog.Builder ResetPrompt = new AlertDialog.Builder(finalContext);
                    ResetPrompt
                    .setTitle(getText(R.string.confirmResetGame_title))
                    .setMessage(getText(R.string.confirmResetGame_message))
                    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener()
                    {
                        public void onClick(DialogInterface dialog, int id)
                        {
                            String[]entries = SaveDir.list();
                            for(String s: entries)
                            {
                                File currentFile = new File(SaveDir.getPath(),s);
                                currentFile.delete();
                            }
                            SaveDir.delete();
                            launchGameActivity(false);
                        }
                    })
                    .setNegativeButton(android.R.string.cancel, this)
                    .show();
                }
                else if (item == 3)
                {
                    Intent SettingsIntent = new Intent(finalContext, GameSettingsActivity.class);
                    startActivityForResult( SettingsIntent, RC_SETTINGS );
                }
            }
        });
        GameMenu.show();
    }

    public void onGalleryItemClick( GalleryItem item )
    {
        NativeExports.LoadGame(item.romFile.getAbsolutePath());
        File InstantSaveDir = new File(NativeExports.SettingsLoadString(SettingsID.Directory_InstantSave.getValue()));
        File GameSaveDir = new File(InstantSaveDir,NativeExports.SettingsLoadString(SettingsID.Game_UniqueSaveDir.getValue()));
        Boolean ResumeGame = HasAutoSave(GameSaveDir);
        launchGameActivity(ResumeGame);
    }

    public boolean onGalleryItemLongClick( GalleryItem item )
    {
        NativeExports.LoadGame(item.romFile.getAbsolutePath());
        StartGameMenu(true);
        return true;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        Log.d("GalleryActivity", "onActivityResult(" + requestCode + "," + resultCode + "," + data);

        if (requestCode == RC_SETTINGS)
        {
            StartGameMenu(true);
            return;
        }
        // Check which request we're responding to
        if (requestCode == GAME_DIR_REQUEST_CODE)
        {
            // Make sure the request was successful
            if (resultCode == RESULT_OK && data != null)
            {
                Bundle extras = data.getExtras();
                String searchPath = extras.getString( ScanRomsActivity.GAME_DIR_PATH );
                boolean searchRecursively = extras.getBoolean( ScanRomsActivity.GAME_DIR_RECURSIVELY );

                if (searchPath != null)
                {
                    NativeExports.RefreshRomDir(searchPath, searchRecursively);
                }
            }
        }
        // Pass on the activity result to the helper for handling
        if (mIabHelper != null && !mIabHelper.handleActivityResult(requestCode, resultCode, data))
        {
            // not handled, so handle it ourselves (here's where you'd
            // perform any handling of activity results not related to in-app
            // billing...
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    void refreshGrid( )
    {
        List<GalleryItem> items;
        items = new ArrayList<GalleryItem>();

        if (mRecentItems.size() > 0)
        {
            items.add( new GalleryItem( this, Strings.GetString(LanguageStringID.ANDROID_GALLERY_RECENTLYPLAYED)));
            items.addAll( mRecentItems );

            items.add( new GalleryItem( this, Strings.GetString(LanguageStringID.ANDROID_GALLERY_LIBRARY)));
        }
        Collections.sort( mGalleryItems, new GalleryItem.NameComparator() );
        items.addAll( mGalleryItems );

        mGridView.setAdapter( new GalleryItem.Adapter( this, items ) );

        // Allow the headings to take up the entire width of the layout
        final List<GalleryItem> finalItems = items;
        GridLayoutManager layoutManager = new GridLayoutManager( this, galleryColumns );
        layoutManager.setSpanSizeLookup( new GridLayoutManager.SpanSizeLookup()
        {
            @Override
            public int getSpanSize( int position )
            {
                // Headings will take up every span (column) in the grid
                if( finalItems.get( position ).isHeading )
                    return galleryColumns;

                // Games will fit in a single column
                return 1;
            }
        } );

        mGridView.setLayoutManager( layoutManager );
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        refreshViews();
    }

    @TargetApi( 11 )
    private void refreshViews()
    {
        // Refresh the gallery
        refreshGrid();
    }

    @Override
    public boolean onKeyDown( int keyCode, KeyEvent event )
    {
        if( keyCode == KeyEvent.KEYCODE_MENU )
        {
            if( mDrawerLayout.isDrawerOpen( GravityCompat.START ) )
            {
                mDrawerLayout.closeDrawer( GravityCompat.START );
            }
            else
            {
                mDrawerLayout.openDrawer( GravityCompat.START );
            }
            return true;
        }
        return super.onKeyDown( keyCode, event );
    }

    @Override
    public void onBackPressed()
    {
        if( mDrawerLayout.isDrawerOpen( GravityCompat.START ) )
        {
            mDrawerLayout.closeDrawer( GravityCompat.START );
        }
        else
        {
            moveTaskToBack(true);
        }
    }

    // Enables or disables the "please wait" screen.
    void setWaitScreen(boolean set)
    {
        if (set)
        {
            WebView webView = (WebView)findViewById(R.id.screen_wait);
            webView.loadData(Utility.readAsset("loading.htm", ""), "text/html", "UTF8");
        }
        findViewById(R.id.screen_main).setVisibility(set ? View.GONE : View.VISIBLE);
        findViewById(R.id.screen_wait).setVisibility(set ? View.VISIBLE : View.GONE);
    }

    IabHelper.OnIabPurchaseFinishedListener mPurchaseFinishedListener = new IabHelper.OnIabPurchaseFinishedListener()
    {
        public void onIabPurchaseFinished(IabResult result, Purchase purchase)
        {
            Log.d("GalleryActivity", "Purchase finished: " + result + ", purchase: " + purchase);
            // if we were disposed of in the meantime, quit.
            if (mIabHelper == null) return;

            if (result.isFailure())
            {
                Log.e("GalleryActivity", "**** Purcahse Error: " + result);
                alert("Save Support Upgrade failed\n\n" + result.getMessage());
                setWaitScreen(false);
                return;
            }

            Log.d("GalleryActivity", "Purchase successful.");

            if (purchase.getSku().equals(SKU_SAVESUPPORT))
            {
                // bought the premium upgrade!
                Log.d("GalleryActivity", "Purchase is save support. Congratulating user.");
                alert("Thank you for upgrading to have save support!");
                setWaitScreen(false);
            }

            if (purchase.getSku().equals(SKU_PJ64SUPPORTOR_2) ||
                purchase.getSku().equals(SKU_PJ64SUPPORTOR_5) ||
                purchase.getSku().equals(SKU_PJ64SUPPORTOR_8) ||
                purchase.getSku().equals(SKU_PJ64SUPPORTOR_10))
            {
                // bought the premium upgrade!
                Log.d("GalleryActivity", "Purchase is project64 support. Congratulating user.");
                alert("Thank you for supporting Project64!");
                mPj64Supporter = true;
                setWaitScreen(false);
            }
        }
    };

    public void PurcahseProject64Support(Activity activity, String sku)
    {
        setWaitScreen(true);
        //Purchase save support
        try
        {
            String payload = NativeExports.appVersion();
            mIabHelper.launchPurchaseFlow(activity, sku, RC_REQUEST, mPurchaseFinishedListener, payload);
        }
        catch (IabAsyncInProgressException e)
        {
            setWaitScreen(false);
        }
    }

    public void ShowReviewOptions()
    {
        new AlertDialog.Builder(GalleryActivity.this).setTitle(getString(R.string.review_title))
        .setMessage(getString(R.string.review_decription))
        .setPositiveButton(getString( R.string.review_ok ), new OnClickListener()
        {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
                Intent IssueIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://play.google.com/store/apps/details?id=emu.project64&hl=en"));
                startActivity(IssueIntent);
            }

        })
        .setNegativeButton( getString( R.string.review_cancel), new OnClickListener()
        {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
            }
        })
        .setCancelable(false).show();
    }

    public void ShowPaymentOptions()
    {
        ArrayList<String> skuList = new ArrayList<String>();
        skuList.add(SKU_PJ64SUPPORTOR_2);
        skuList.add(SKU_PJ64SUPPORTOR_5);
        skuList.add(SKU_PJ64SUPPORTOR_8);
        skuList.add(SKU_PJ64SUPPORTOR_10);
        Bundle querySkus = new Bundle();
        querySkus.putStringArrayList("ITEM_ID_LIST", skuList);

        final Context context = this;
        final Activity activity = this;
        IabHelper.QueryInventoryFinishedListener GotPaymentOptionListener = new IabHelper.QueryInventoryFinishedListener()
        {
            public void onQueryInventoryFinished(IabResult result, Inventory inventory)
            {
                Log.d("GalleryActivity", "Query inventory finished.");

                // Have we been disposed of in the meantime? If so, quit.
                if (mIabHelper == null) return;

                // Is it a failure?
                if (result.isFailure())
                {
                    //complain("Failed to query inventory: " + result);
                    return;
                }

                Log.d("GalleryActivity", "SKU_PJ64SUPPORTOR_2 price: " + inventory.getSkuDetails(SKU_PJ64SUPPORTOR_2).getPrice());
                Log.d("GalleryActivity", "SKU_PJ64SUPPORTOR_5 price: " + inventory.getSkuDetails(SKU_PJ64SUPPORTOR_5).getPrice());
                Log.d("GalleryActivity", "SKU_PJ64SUPPORTOR_8 price: " + inventory.getSkuDetails(SKU_PJ64SUPPORTOR_8).getPrice());
                Log.d("GalleryActivity", "SKU_PJ64SUPPORTOR_10 price: " + inventory.getSkuDetails(SKU_PJ64SUPPORTOR_10).getPrice());

                CharSequence options[] = new CharSequence[]
                {
                    inventory.getSkuDetails(SKU_PJ64SUPPORTOR_10).getPrice(),
                    inventory.getSkuDetails(SKU_PJ64SUPPORTOR_8).getPrice(),
                    inventory.getSkuDetails(SKU_PJ64SUPPORTOR_5).getPrice(),
                    inventory.getSkuDetails(SKU_PJ64SUPPORTOR_2).getPrice()
                };

                AlertDialog.Builder builder = new AlertDialog.Builder(context);
                builder.setTitle("Select an amount");
                builder.setItems(options, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        switch (which)
                        {
                        case 0: PurcahseProject64Support(activity,SKU_PJ64SUPPORTOR_10); break;
                        case 1: PurcahseProject64Support(activity,SKU_PJ64SUPPORTOR_8); break;
                        case 2: PurcahseProject64Support(activity,SKU_PJ64SUPPORTOR_5); break;
                        case 3: PurcahseProject64Support(activity,SKU_PJ64SUPPORTOR_2); break;
                        }
                    }
                });
                final AlertDialog dialog = builder.create();
                dialog.show();
            }
        };

        try
        {
            mIabHelper.queryInventoryAsync(true, skuList, null, GotPaymentOptionListener);
        }
        catch (IabAsyncInProgressException e)
        {
            e.printStackTrace();
        }
    }


    public void launchGameActivity(boolean ResumeGame)
    {
        if (ResumeGame)
        {
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 0);
        }
        NativeExports.SettingsSaveBool(SettingsID.Game_LoadSaveAtStart.getValue(), ResumeGame);
        // Launch the game activity
        boolean isXperiaPlay = false;

        Intent intent = isXperiaPlay ? new Intent( this, GameActivityXperiaPlay.class ) : new Intent( this, GameActivity.class );
        this.startActivity( intent );
    }

    public static void LanguageChanged()
    {
        if (mActiveGalleryActivity != null)
        {
            mActiveGalleryActivity.finish();
            mActiveGalleryActivity.startActivity( mActiveGalleryActivity.getIntent() );
        }
    }

    public static void RomListReset ()
    {
        mGalleryItems = new ArrayList<GalleryItem>();
        if (mActiveGalleryActivity != null && mActiveGalleryActivity.mProgress != null)
        {
            Handler h = new Handler(Looper.getMainLooper());
            h.post(new Runnable()
            {
                public void run()
                {
                    mActiveGalleryActivity.mProgress.show();
                }
            });
        }
    }

    public static void RomListAddItem (String FullFileName, String FileName, String GoodName, int TextColor)
    {
        GalleryItem item = new GalleryItem( mActiveGalleryActivity, GoodName, FileName, FullFileName, TextColor );
        mGalleryItems.add( item );
        if (mActiveGalleryActivity != null && mActiveGalleryActivity.mProgress != null)
        {
            Handler h = new Handler(Looper.getMainLooper());
            final String ProgressText = new String(FileName);
            final String ProgressSubText = new String(FullFileName);
            final String ProgressMessage = new String("Added " + GoodName);

            h.post(new Runnable()
            {
                public void run()
                {
                    mActiveGalleryActivity.mProgress.setText(ProgressText);
                    mActiveGalleryActivity.mProgress.setSubtext(ProgressSubText);
                    mActiveGalleryActivity.mProgress.setMessage(ProgressMessage);
                }
            });
        }
    }

    private static void refreshRecentRoms()
    {
        mRecentItems = new ArrayList<GalleryItem>();

        for (int i = 0, n = NativeExports.UISettingsLoadDword(UISettingID.File_RecentGameFileCount.getValue()); i < n; i++)
        {
            String RecentFile = NativeExports.UISettingsLoadStringIndex(UISettingID.File_RecentGameFileIndex.getValue(), i);
            if (RecentFile.length() == 0)
            {
                break;
            }
            for (int z = 0; z < mGalleryItems.size(); z++)
            {
                if (RecentFile.equals(mGalleryItems.get(z).romFile.getAbsolutePath()))
                {
                    mRecentItems.add(mGalleryItems.get(z));
                    break;
                }
            }
        }
    }

    public static void RomListLoadDone()
    {
        refreshRecentRoms();
        if (mActiveGalleryActivity != null && mActiveGalleryActivity.mProgress != null)
        {
            Handler h = new Handler(Looper.getMainLooper());
            h.post(new Runnable()
            {
                public void run()
                {
                    mActiveGalleryActivity.refreshGrid();
                    mActiveGalleryActivity.mProgress.dismiss();
                }
            });
        }
    }

    public static void RecentRomsUpdated()
    {
        refreshRecentRoms();
        Handler h = new Handler(Looper.getMainLooper());
        h.post(new Runnable()
        {
            public void run()
            {
                mActiveGalleryActivity.refreshGrid();
            }
        });
    }
}
