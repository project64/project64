package emu.project64;

import java.io.File;
import java.io.IOException;
import java.util.List;
import emu.project64.R;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;
import emu.project64.jni.UISettingID;
import emu.project64.task.ExtractAssetsTask;
import emu.project64.task.ExtractAssetsTask.ExtractAssetsListener;
import emu.project64.task.ExtractAssetsTask.Failure;
import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.app.ActivityCompat.OnRequestPermissionsResultCallback;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.text.Html;
import android.util.Log;
import android.view.WindowManager.LayoutParams;
import android.widget.TextView;

public class SplashActivity extends AppCompatActivity implements ExtractAssetsListener, OnRequestPermissionsResultCallback
{
    static final int PERMISSION_REQUEST = 177;
    static final int NUM_PERMISSIONS = 2;
    private int TOTAL_ASSETS = 100;
    private static final int SPLASH_DELAY = 2000;

    private static final String SOURCE_DIR = "project64_data";

    private static boolean mInit = false;
    private static boolean mAppInit = false;

    private TextView mTextView;
    private int mAssetsExtracted;

    @Override
    public void onCreate( Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );

        getWindow().setFlags( LayoutParams.FLAG_KEEP_SCREEN_ON, LayoutParams.FLAG_KEEP_SCREEN_ON );
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        try
        {
            setContentView( R.layout.splash_activity );
        }
        catch (android.view.InflateException e)
        {
            Log.e("SplashActivity", "Resource NOT found");
            return;
        }
        ((TextView) findViewById( R.id.versionText )).setText(NativeExports.appVersion());
        mTextView = (TextView) findViewById( R.id.mainText );

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
        if((new File(ConfigFile)).exists())
        {
            InitProject64();
        }

        Log.i( "Splash", "extractAssetsTaskLauncher - startup");
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

    private final Runnable startGalleryLauncher = new Runnable()
    {
        @Override
        public void run()
        {
            Intent intent = new Intent( SplashActivity.this, GalleryActivity.class );
            SplashActivity.this.startActivity( intent );
            finish();
        }
    };

    private boolean CountTotalAssetFiles(String path)
    {
        String [] list;
        try
        {
            list = getAssets().list(path);
            if (list.length > 0)
            {
                for (String file : list)
                {
                    if (!CountTotalAssetFiles(path + "/" + file))
                    {
                        return false;
                    }
                    else
                    {
                        TOTAL_ASSETS += 1;
                    }
                }
            }
        }
        catch (IOException e)
        {
            return false;
        }
        return true;
    }

    private final Runnable extractAssetsTaskLauncher = new Runnable()
    {
        @Override
        public void run()
        {
            Log.i( "Splash", "extractAssetsTaskLauncher - start");
            TOTAL_ASSETS = 0;
            CountTotalAssetFiles(SOURCE_DIR);
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
            mTextView.setText( R.string.assetExtractor_finished );
            NativeExports.UISettingsSaveDword(UISettingID.Asserts_Version.getValue(), ASSET_VERSION);
            Intent intent = new Intent( this, GalleryActivity.class );
            this.startActivity( intent );
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
