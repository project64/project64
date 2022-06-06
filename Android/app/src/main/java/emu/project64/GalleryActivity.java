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

import emu.project64.R;
import emu.project64.dialog.ProgressDialog;
import emu.project64.game.GameActivity;
import emu.project64.jni.LanguageStringID;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;
import emu.project64.jni.UISettingID;
import emu.project64.settings.GameSettingsActivity;
import emu.project64.settings.SettingsActivity;
import emu.project64.util.Strings;
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

import androidx.appcompat.view.menu.MenuBuilder;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.res.ResourcesCompat;
import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

public class GalleryActivity extends AppCompatActivity
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

    public static final int GAME_DIR_REQUEST_CODE = 1;
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

        Log.d("GalleryActivity", "Starting setup.");
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

        Toolbar toolbar = (Toolbar) findViewById( R.id.toolbar );
        toolbar.setTitle( R.string.app_name );
        setSupportActionBar( toolbar );
        ActionBar actionBar = getSupportActionBar();
        actionBar.setDisplayHomeAsUpEnabled(false);
        actionBar.setHomeButtonEnabled(false);
        actionBar.setDisplayShowTitleEnabled(false);
    }

    void UpdateMenuLanguage(Menu menu)
    {
        Strings.SetMenuTitle(menu, R.id.menuItem_GameDir, LanguageStringID.ANDROID_GAMEDIR);
        Strings.SetMenuTitle(menu, R.id.menuItem_settings, LanguageStringID.ANDROID_SETTINGS);
        Strings.SetMenuTitle(menu, R.id.menuItem_discord, LanguageStringID.ANDROID_DISCORD);
        Strings.SetMenuTitle(menu, R.id.menuItem_about, LanguageStringID.ANDROID_ABOUT);
    }

    void alert(String message)
    {
        AlertDialog.Builder bld = new AlertDialog.Builder(this);
        bld.setMessage(message);
        bld.setNeutralButton("OK", null);
        Log.d("GalleryActivity", "Showing alert dialog: " + message);
        bld.create().show();
    }

    @Override
    protected void onPostCreate( Bundle savedInstanceState )
    {
        super.onPostCreate( savedInstanceState );
    }

    @Override
    public void onConfigurationChanged( Configuration newConfig )
    {
        super.onConfigurationChanged( newConfig );
    }

    @Override
    public boolean onCreateOptionsMenu( Menu menu )
    {
        super.onCreateOptionsMenu( menu );
        getMenuInflater().inflate( R.menu.gallery, menu );
        UpdateMenuLanguage(menu);
        if(menu instanceof MenuBuilder)
        {
            MenuBuilder menuBuilder = (MenuBuilder) menu;
            menuBuilder.setOptionalIconsVisible(true);
        }
        return true;
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
            Intent DiscordIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://discord.gg/Cg3zquF"));
            startActivity(DiscordIntent);
            return true;
        case R.id.menuItem_about:
            Intent AboutIntent = new Intent(this, AboutActivity.class);
            startActivity( AboutIntent );
            return true;
        }
        return super.onOptionsItemSelected( item );
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
        File InstantSaveDir = new File(NativeExports.SettingsLoadString(SettingsID.Directory_InstantSave.toString()));
        final File GameSaveDir = new File(InstantSaveDir,NativeExports.SettingsLoadString(SettingsID.Game_UniqueSaveDir.toString()));

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
        if (ShowSettings && !NativeExports.SettingsLoadBool(SettingsID.UserInterface_BasicMode.toString()))
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
        GameMenu.setTitle(NativeExports.SettingsLoadString(SettingsID.Rdb_GoodName.toString()));
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
        File InstantSaveDir = new File(NativeExports.SettingsLoadString(SettingsID.Directory_InstantSave.toString()));
        File GameSaveDir = new File(InstantSaveDir,NativeExports.SettingsLoadString(SettingsID.Game_UniqueSaveDir.toString()));
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
    public void launchGameActivity(boolean ResumeGame)
    {
        if (ResumeGame)
        {
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.toString(), 0);
        }
        NativeExports.SettingsSaveBool(SettingsID.Game_LoadSaveAtStart.toString(), ResumeGame);
        Intent intent = new Intent( this, GameActivity.class );
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

        for (int i = 0, n = NativeExports.SettingsLoadDword(UISettingID.FileRecentGameFileCount.toString()); i < n; i++)
        {
            String RecentFile = NativeExports.SettingsLoadStringIndex(UISettingID.FileRecentGameFileIndex.toString(), i);
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
