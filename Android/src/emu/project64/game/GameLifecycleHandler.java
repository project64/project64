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
package emu.project64.game;

import java.lang.ref.WeakReference;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import java.util.ArrayList;
import java.util.Set;

import emu.project64.AndroidDevice;
import emu.project64.R;
import emu.project64.input.AbstractController;
import emu.project64.input.TouchController;
import emu.project64.input.map.VisibleTouchMap;
import emu.project64.jni.NativeExports;
import emu.project64.jni.NativeXperiaTouchpad;
import emu.project64.jni.SettingsID;
import emu.project64.jni.SystemEvent;
import emu.project64.jni.UISettingID;
import emu.project64.persistent.ConfigFile;
import emu.project64.profile.Profile;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.os.Vibrator;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.Window;
import android.view.WindowManager.LayoutParams;

public class GameLifecycleHandler implements SurfaceHolder.Callback, GameSurface.SurfaceInfo
{
    private final static boolean LOG_GAMELIFECYCLEHANDLER = true;

    // Activity and views
    private Activity mActivity;
    private GameSurface mSurface;
    private GameOverlay mOverlay;
    private final ArrayList<AbstractController> mControllers;
    private VisibleTouchMap mTouchscreenMap;
    // Internal flags
    private final boolean mIsXperiaPlay;
    private boolean mStarted = false;
    private boolean mStopped = false;

    // Lifecycle state tracking
    private boolean mIsFocused = false; // true if the window is focused
    private boolean mIsResumed = false; // true if the activity is resumed
    private boolean mIsSurface = false; // true if the surface is available
    
    public GameLifecycleHandler(Activity activity) 
    {
        mActivity = activity;
        mControllers = new ArrayList<AbstractController>();
        mIsXperiaPlay = !(activity instanceof GameActivity);
    }

    @TargetApi(11)
    public void onCreateBegin(Bundle savedInstanceState) 
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "onCreateBegin");
        }
        
        // For Honeycomb, let the action bar overlay the rendered view (rather
        // than squeezing it)
        // For earlier APIs, remove the title bar to yield more space
        Window window = mActivity.getWindow();
        window.requestFeature(AndroidDevice.IS_ACTION_BAR_AVAILABLE ? Window.FEATURE_ACTION_BAR_OVERLAY : Window.FEATURE_NO_TITLE);

        // Enable full-screen mode
        window.setFlags(LayoutParams.FLAG_FULLSCREEN, LayoutParams.FLAG_FULLSCREEN);

        // Keep screen from going to sleep
        window.setFlags(LayoutParams.FLAG_KEEP_SCREEN_ON, LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Set the screen orientation
        mActivity.setRequestedOrientation(NativeExports.UISettingsLoadDword(UISettingID.Screen_Orientation.getValue()));
    }

    @TargetApi(11)
    public void onCreateEnd(Bundle savedInstanceState) 
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "onCreateEnd");
        }

        // Take control of the GameSurface if necessary
        if (mIsXperiaPlay)
        {
            mActivity.getWindow().takeSurface(null);
        }

        // Lay out content and get the views
        mActivity.setContentView(R.layout.game_activity);
        mSurface = (GameSurface) mActivity.findViewById( R.id.gameSurface );
        mOverlay = (GameOverlay) mActivity.findViewById(R.id.gameOverlay);
        
        // Listen to game surface events (created, changed, destroyed)
        mSurface.getHolder().addCallback( this );
        mSurface.createGLContext((ActivityManager)mActivity.getSystemService(Context.ACTIVITY_SERVICE));

        // Configure the action bar introduced in higher Android versions
        if (AndroidDevice.IS_ACTION_BAR_AVAILABLE) 
        {
            mActivity.getActionBar().hide();
            ColorDrawable color = new ColorDrawable(Color.parseColor("#303030"));
            color.setAlpha(50 /*mGlobalPrefs.displayActionBarTransparency*/);
            mActivity.getActionBar().setBackgroundDrawable(color);
        }

        boolean isFpsEnabled = false; //mGlobalPrefs.isFpsEnabled
        boolean isTouchscreenAnimated = false; //mGlobalPrefs.isTouchscreenAnimated
        boolean isTouchscreenHidden = false; //!isTouchscreenEnabled || globalPrefs.touchscreenTransparency == 0;
        String profilesDir = AndroidDevice.PACKAGE_DIRECTORY + "/profiles";
        String touchscreenProfiles_cfg = profilesDir + "/touchscreen.cfg";
        ConfigFile touchscreenConfigFile = new ConfigFile( touchscreenProfiles_cfg );
        //SharedPreferences mPreferences = context.getSharedPreferences( sharedPrefsName, Context.MODE_PRIVATE );
        Profile touchscreenProfile = new Profile( true, touchscreenConfigFile.get( "Analog")); //loadProfile( /*mPreferences*/ null, "touchscreenProfile", "Analog", touchscreenProfiles_cfg,touchscreenProfiles_cfg );
        int touchscreenTransparency = 100;
        String touchscreenSkinsDir = AndroidDevice.PACKAGE_DIRECTORY + "/skins/touchscreen";
        String touchscreenSkin = touchscreenSkinsDir + "/Outline";
        float touchscreenScale = 1.0f; //( (float) mPreferences.getInt( "touchscreenScale", 100 ) ) / 100.0f;
        
        // The touch map and overlay are needed to display frame rate and/or controls
        mTouchscreenMap = new VisibleTouchMap( mActivity.getResources() );
        mTouchscreenMap.load(touchscreenSkin, touchscreenProfile,
                isTouchscreenAnimated, isFpsEnabled,
                touchscreenScale, touchscreenTransparency );
        mOverlay.initialize( mTouchscreenMap, !isTouchscreenHidden, isFpsEnabled, isTouchscreenAnimated );
        
        // Initialize user interface devices
        View inputSource = mIsXperiaPlay ? new NativeXperiaTouchpad(mActivity) : mOverlay;
        initControllers(inputSource);
    }

    public void onStart() 
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "onStart");
        }
    }

    public void onResume() 
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "onResume");
        }
        mIsResumed = true;
        tryRunning();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) 
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "surfaceCreated");
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) 
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "surfaceChanged");
        }
        mIsSurface = true;
        tryRunning();
    }

    public void onWindowFocusChanged(boolean hasFocus) 
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "onWindowFocusChanged: " + hasFocus);
        }
        // Only try to run; don't try to pause. User may just be touching the
        // in-game menu.
        mIsFocused = hasFocus;
        if (hasFocus) 
        {
            tryRunning();            
        }
    }

    public void AutoSave()
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "OnAutoSave");
        }
        if (NativeExports.SettingsLoadBool(SettingsID.GameRunning_CPU_Running.getValue()) == true)
        {
	        int CurrentSaveState = NativeExports.SettingsLoadDword(SettingsID.Game_CurrentSaveState.getValue());
	        int OriginalSaveTime = NativeExports.SettingsLoadDword(SettingsID.Game_LastSaveTime.getValue());
	        NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 0);
	        NativeExports.ExternalEvent(SystemEvent.SysEvent_SaveMachineState.getValue());
	        for (int i = 0; i < 100; i++)
	        {
	            int LastSaveTime = NativeExports.SettingsLoadDword(SettingsID.Game_LastSaveTime.getValue());
	            if (LastSaveTime != OriginalSaveTime)
	            {
	                break;
	            }
	            try 
	            {
	                Thread.sleep(100); 
	            }
	            catch(InterruptedException ex)
	            {
	                Thread.currentThread().interrupt();
	            }
	        }
	        NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), CurrentSaveState);
        }
        else if (LOG_GAMELIFECYCLEHANDLER)
        {
            Log.i("GameLifecycleHandler", "CPU not running, not doing anything");        	
        }
        
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "OnAutoSave Done");
        }
    }
    
    public void onPause()
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "onPause");
        }
        AutoSave();
        mIsResumed = false;
        mStopped = true;
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
        	Log.i("GameLifecycleHandler", "Stop Emulation");
        }
        NativeExports.StopEmulation();
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
        	Log.i("GameLifecycleHandler", "onPause - done");
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "surfaceDestroyed");
        }
        mIsSurface = false;
    }

    public void onStop()
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "onStop");
        }
    }

    public void onDestroy()
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "onDestroy");
        }
    }

    @SuppressLint("InlinedApi")
    private void initControllers(View inputSource)
    {
        // By default, send Player 1 rumbles through phone vibrator
        Vibrator vibrator = (Vibrator) mActivity.getSystemService( Context.VIBRATOR_SERVICE );
        int touchscreenAutoHold = 0;
        boolean isTouchscreenFeedbackEnabled = false;
        Set<Integer> autoHoldableButtons = null;
        
        // Create the touchscreen controller
        TouchController touchscreenController = new TouchController( mTouchscreenMap,
                inputSource, mOverlay, vibrator, touchscreenAutoHold,
                isTouchscreenFeedbackEnabled, autoHoldableButtons );
        mControllers.add( touchscreenController );
    }

    private void tryRunning() 
    {
        if (mIsFocused && mIsResumed && mIsSurface && mStopped)
        {
            mStopped = false;
            NativeExports.StartEmulation();
        }
        if (mIsFocused && mIsResumed && mIsSurface && !mStarted)
        {
            mStarted = true;
            final GameLifecycleHandler handler = this;

            NativeExports.StartGame(mActivity, new GameSurface.GLThread(new WeakReference<GameSurface>(mSurface), handler));
        }
    }
        
    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "onSurfaceCreated");
        }
        NativeExports.onSurfaceCreated();
    }
 
    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height)
    {
        if (LOG_GAMELIFECYCLEHANDLER) 
        {
            Log.i("GameLifecycleHandler", "onSurfaceChanged");
        }
        NativeExports.onSurfaceChanged(width, height);
    }
}
