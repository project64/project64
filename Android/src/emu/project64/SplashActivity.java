/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
package emu.project64;

import java.io.File;
import java.util.List;

import com.google.android.gms.analytics.HitBuilders;

import emu.project64.R;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;
import emu.project64.jni.UISettingID;
import emu.project64.task.ExtractAssetsTask;
import emu.project64.task.ExtractAssetsTask.ExtractAssetsListener;
import emu.project64.task.ExtractAssetsTask.Failure;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.os.Handler;
import android.text.Html;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager.LayoutParams;
import android.widget.TextView;

/**
 * The main activity that presents the splash screen, extracts the assets if necessary, and launches
 * the main menu activity.
 */
public class SplashActivity extends Activity implements ExtractAssetsListener
{
    /**
     * Asset version number, used to determine stale assets. Increment this number every time the
     * assets are updated on disk.
     */
    private static final int ASSET_VERSION = 2;

    /** The total number of assets to be extracted (for computing progress %). */
    private static final int TOTAL_ASSETS = 89;

    /** The minimum duration that the splash screen is shown, in milliseconds. */
    private static final int SPLASH_DELAY = 2000;

    /**
     * The subdirectory within the assets directory to extract. A subdirectory is necessary to avoid
     * extracting all the default system assets in addition to ours.
     */
    private static final String SOURCE_DIR = "project64_data";

    private static boolean mInit = false;
    private static boolean mAppInit = false;

    /** The text view that displays extraction progress info. */
    private static TextView mTextView;

    /** The running count of assets extracted. */
    private int mAssetsExtracted;

    @Override
    public void onCreate( Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );
        Window window = getWindow();

        // Don't let the activity sleep in the middle of extraction
        window.setFlags( LayoutParams.FLAG_KEEP_SCREEN_ON, LayoutParams.FLAG_KEEP_SCREEN_ON );

        // Lay out the content
        setContentView( R.layout.splash_activity );
        ((TextView) findViewById( R.id.versionText )).setText(NativeExports.appVersion());
        mTextView = (TextView) findViewById( R.id.mainText );

        if (!mInit)
        {
            mInit = true;
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

            String ConfigFile = AndroidDevice.PACKAGE_DIRECTORY + "/Config/Project64.cfg";
            if(( new File( ConfigFile ) ).exists())
            {
                InitProject64();
            }

            ((Project64Application) getApplication()).getDefaultTracker().send(new HitBuilders.EventBuilder()
                .setCategory("start")
                .setLabel(NativeExports.appVersion())
                .build());

            // Extract the assets in a separate thread and launch the menu activity
            // Handler.postDelayed ensures this runs only after activity has resumed
            Log.e( "Splash", "extractAssetsTaskLauncher - startup");
            final Handler handler = new Handler();
            if (!mAppInit || NativeExports.UISettingsLoadDword(UISettingID.Asserts_Version.getValue()) != ASSET_VERSION)
            {
                handler.post( extractAssetsTaskLauncher );
            }
            else
            {
                handler.postDelayed( startGalleryLauncher, SPLASH_DELAY );
            }
        }
    }

    @Override
    public void onBackPressed()
    {
        moveTaskToBack(true);
    }

    static public void Reset ()
    {
        mInit = false;
        mAppInit = false;
    }

    private void InitProject64()
    {
        String LibsDir = this.getFilesDir().getParentFile().getAbsolutePath() + "/lib/";
        if( !( new File( LibsDir ) ).exists() && AndroidDevice.IS_GINGERBREAD )
        {
            LibsDir = this.getApplicationInfo().nativeLibraryDir;
        }
        String SyncDir = this.getFilesDir().getParentFile().getAbsolutePath() + "/lib-sync/";
        NativeExports.appInit(AndroidDevice.PACKAGE_DIRECTORY);
        NativeExports.SettingsSaveString(SettingsID.Directory_PluginSelected.getValue(), LibsDir);
        NativeExports.SettingsSaveBool(SettingsID.Directory_PluginUseSelected.getValue(), true);
        NativeExports.SettingsSaveString(SettingsID.Directory_PluginSyncSelected.getValue(), SyncDir);
        NativeExports.SettingsSaveBool(SettingsID.Directory_PluginSyncUseSelected.getValue(), true);
        String SaveDir = AndroidDevice.EXTERNAL_PUBLIC_DIRECTORY + "/Project64/Save";
        if (!NativeExports.IsSettingSet(SettingsID.Directory_NativeSave.getValue()))
        {
            NativeExports.SettingsSaveString(SettingsID.Directory_NativeSaveSelected.getValue(), SaveDir);
            NativeExports.SettingsSaveBool(SettingsID.Directory_NativeSaveUseSelected.getValue(), true);
        }

        if (!NativeExports.IsSettingSet(SettingsID.Directory_InstantSave.getValue()))
        {
            NativeExports.SettingsSaveString(SettingsID.Directory_InstantSaveSelected.getValue(), SaveDir);
            NativeExports.SettingsSaveBool(SettingsID.Directory_InstantSaveUseSelected.getValue(), true);
        }

        if (!NativeExports.IsSettingSet(SettingsID.Directory_Log.getValue()))
        {
            String LogDir = AndroidDevice.EXTERNAL_PUBLIC_DIRECTORY + "/Project64/Logs";
            NativeExports.SettingsSaveString(SettingsID.Directory_LogSelected.getValue(), LogDir);
            NativeExports.SettingsSaveBool(SettingsID.Directory_LogUseSelected.getValue(), true);
        }

        if (!NativeExports.IsSettingSet(SettingsID.Directory_SnapShot.getValue()))
        {
            String SnapShotDir = AndroidDevice.EXTERNAL_PUBLIC_DIRECTORY + "/Project64/Screenshots";
            NativeExports.SettingsSaveString(SettingsID.Directory_SnapShotSelected.getValue(), SnapShotDir);
            NativeExports.SettingsSaveBool(SettingsID.Directory_SnapShotUseSelected.getValue(), true);
        }
        mAppInit = true;
    }

    /** Runnable that launches the non-UI thread from the UI thread after the activity has resumed. */
    private final Runnable startGalleryLauncher = new Runnable()
    {
        @Override
        public void run()
        {
            // Assets already extracted, just launch gallery activity
            Intent intent = new Intent( SplashActivity.this, GalleryActivity.class );
            SplashActivity.this.startActivity( intent );

            // We never want to come back to this activity, so finish it
            finish();
        }
    };

    private final Runnable extractAssetsTaskLauncher = new Runnable()
    {
        @Override
        public void run()
        {
            // Extract and merge the assets if they are out of date
            mAssetsExtracted = 0;
            new ExtractAssetsTask( getAssets(), SOURCE_DIR, AndroidDevice.PACKAGE_DIRECTORY, SplashActivity.this ).execute();
        }
    };

    @Override
    public void onExtractAssetsProgress( String nextFileToExtract )
    {
        final float percent = ( 100f * mAssetsExtracted ) / (float) TOTAL_ASSETS;
        final String text = getString( R.string.assetExtractor_progress, percent, nextFileToExtract );
        mTextView.setText(text);
        mAssetsExtracted++;
    }

    @Override
    public void onExtractAssetsFinished( List<Failure> failures )
    {
        if( failures.size() == 0 )
        {
            if (!mAppInit)
            {
                InitProject64();
            }

            // Extraction succeeded, record new asset version and merge cheats
            mTextView.setText( R.string.assetExtractor_finished );
            NativeExports.UISettingsSaveDword(UISettingID.Asserts_Version.getValue(), ASSET_VERSION);

            // Launch gallery activity
            Intent intent = new Intent( this, GalleryActivity.class );
            this.startActivity( intent );

            // We never want to come back to this activity, so finish it
            finish();
        }
        else
        {
            // Extraction failed, update the on-screen text and don't start next activity
            String weblink = getResources().getString( R.string.assetExtractor_uriHelp );
            String message = getString( R.string.assetExtractor_failed, weblink );
            String textHtml = message.replace( "\n", "<br/>" ) + "<p><small>";
            for( Failure failure : failures )
            {
                textHtml += failure.toString() + "<br/>";
            }
            textHtml += "</small>";
            mTextView.setText( Html.fromHtml( textHtml ) );
        }
    }
}
