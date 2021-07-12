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
import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Intent;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.ActivityCompat.OnRequestPermissionsResultCallback;
import android.support.v4.content.ContextCompat;
import android.text.Html;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager.LayoutParams;
import android.widget.TextView;

/**
 * The main activity that presents the splash screen, extracts the assets if necessary, and launches
 * the main menu activity.
 */
public class SplashActivity extends Activity implements ExtractAssetsListener, OnRequestPermissionsResultCallback
{
    static final int PERMISSION_REQUEST = 177;
    static final int NUM_PERMISSIONS = 2;

    /**
     * Asset version number, used to determine stale assets. Increment this number every time the
     * assets are updated on disk.
     */
    private static final int ASSET_VERSION = 3;

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

        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        if ((ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED &&
            ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) ||
             !AndroidDevice.IS_LOLLIPOP)
        {
            StartExtraction();
        }
        else if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.READ_EXTERNAL_STORAGE) ||
            ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.WRITE_EXTERNAL_STORAGE))
        {
            new AlertDialog.Builder(this)
            .setTitle(getString(R.string.assetExtractor_permissions_title))
            .setMessage(getString(R.string.assetExtractor_permissions_rationale))
            .setPositiveButton(getString(android.R.string.ok), new OnClickListener()
            {
                @Override
                public void onClick(DialogInterface dialog, int which)
                {
                    actuallyRequestPermissions();
                }

            }).setNegativeButton(getString(android.R.string.cancel), new OnClickListener()
            {
                //Show dialog stating that the app can't continue without proper permissions
                @Override
                public void onClick(DialogInterface dialog, int which)
                {
                    new AlertDialog.Builder(SplashActivity.this).setTitle(getString(R.string.assetExtractor_error))
                        .setMessage(getString(R.string.assetExtractor_failed_permissions))
                        .setPositiveButton(getString( android.R.string.ok ), new OnClickListener()
                        {
                            @Override
                            public void onClick(DialogInterface dialog, int which)
                            {
                                SplashActivity.this.finish();
                            }
                        }).setCancelable(false).show();
                }
            }).setCancelable(false).show();
        }
        else
        {
            actuallyRequestPermissions();
        }
    }

    public void StartExtraction()
    {
        if (mInit)
        {
            return;
        }
        mInit = true;
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

    public void actuallyRequestPermissions()
    {
        ActivityCompat.requestPermissions(this, new String[] {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE }, PERMISSION_REQUEST);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
    {
        switch (requestCode)
        {
        case PERMISSION_REQUEST:
        {
            // If request is cancelled, the result arrays are empty.
            boolean good = true;
            if (permissions.length != NUM_PERMISSIONS || grantResults.length != NUM_PERMISSIONS)
            {
                good = false;
            }

            for (int i = 0; i < grantResults.length && good; i++)
            {
                if (grantResults[i] != PackageManager.PERMISSION_GRANTED)
                {
                    good = false;
                }
            }

            if (!good)
            {
                new AlertDialog.Builder(SplashActivity.this).setTitle(getString(R.string.assetExtractor_error))
                    .setMessage(getString(R.string.assetExtractor_failed_permissions))
                    .setPositiveButton(getString( android.R.string.ok ), new OnClickListener()
                    {
                        @Override
                        public void onClick(DialogInterface dialog, int which)
                        {
                            SplashActivity.this.finish();
                        }

                    }).setCancelable(false).show();
            }
            else
            {
                StartExtraction();
            }
            return;
        }}
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
            Log.e( "Splash", "extractAssetsTaskLauncher - start");
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
