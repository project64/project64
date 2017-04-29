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

import com.bda.controller.Controller;

import java.util.ArrayList;
import java.util.Set;

import emu.project64.AndroidDevice;
import emu.project64.Project64Application;
import emu.project64.R;
import emu.project64.hack.MogaHack;
import emu.project64.input.AbstractController;
import emu.project64.input.PeripheralController;
import emu.project64.input.TouchController;
import emu.project64.input.map.InputMap;
import emu.project64.input.map.VisibleTouchMap;
import emu.project64.input.provider.AbstractProvider;
import emu.project64.input.provider.AxisProvider;
import emu.project64.input.provider.KeyProvider;
import emu.project64.input.provider.KeyProvider.ImeFormula;
import emu.project64.input.provider.MogaProvider;
import emu.project64.jni.NativeExports;
import emu.project64.jni.NativeVideo;
import emu.project64.jni.NativeXperiaTouchpad;
import emu.project64.jni.SettingsID;
import emu.project64.jni.SystemEvent;
import emu.project64.jni.UISettingID;
import emu.project64.jni.VideoSettingID;
import emu.project64.persistent.ConfigFile;
import emu.project64.persistent.ConfigFile.ConfigSection;
import emu.project64.profile.Profile;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.os.Vibrator;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.FrameLayout;

public class GameLifecycleHandler implements View.OnKeyListener, SurfaceHolder.Callback, GameSurface.SurfaceInfo
{
    private final static boolean LOG_GAMELIFECYCLEHANDLER = false;
    public final static int RC_SETTINGS = 10005;

    // Activity and views
    private Activity mActivity;
    private GameSurface mSurface;
    private GameOverlay mOverlay;

    // Input resources
    private ArrayList<AbstractController> mControllers;
    private VisibleTouchMap mTouchscreenMap;
    private KeyProvider mKeyProvider;
    private Controller mMogaController;

    // Internal flags
    private final boolean mIsXperiaPlay;
    private boolean mStarted = false;
    private boolean mStopped = false;

    // Lifecycle state tracking
    private boolean mIsFocused = false; // true if the window is focused
    private boolean mIsResumed = false; // true if the activity is resumed
    private boolean mIsSurface = false; // true if the surface is available

    private float mtouchscreenScale = ((float)NativeExports.UISettingsLoadDword(UISettingID.TouchScreen_ButtonScale.getValue())) / 100.0f;
    private String mlayout = NativeExports.UISettingsLoadString(UISettingID.TouchScreen_Layout.getValue());

    public GameLifecycleHandler(Activity activity)
    {
        mActivity = activity;
        mControllers = new ArrayList<AbstractController>();
        mIsXperiaPlay = !(activity instanceof GameActivity);
        mMogaController = Controller.getInstance(mActivity);
    }

    @TargetApi(11)
    public void onCreateBegin(Bundle savedInstanceState)
    {
        if (LOG_GAMELIFECYCLEHANDLER)
        {
            Log.i("GameLifecycleHandler", "onCreateBegin");
        }
        // Initialize MOGA controller API
        // TODO: Remove hack after MOGA SDK is fixed
        // mMogaController.init();
        MogaHack.init(mMogaController, mActivity);

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
        mSurface = (GameSurface) mActivity.findViewById(R.id.gameSurface);
        mOverlay = (GameOverlay) mActivity.findViewById(R.id.gameOverlay);

        float widthRatio = (float)AndroidDevice.nativeWidth/(float)AndroidDevice.nativeHeight;
        int ScreenRes = NativeExports.SettingsLoadDword(SettingsID.FirstGfxSettings.getValue() + VideoSettingID.Set_Resolution.getValue());
        int videoRenderWidth = Math.round(NativeVideo.GetScreenResHeight(ScreenRes) * widthRatio);
        int videoRenderHeight = Math.round(NativeVideo.GetScreenResHeight(ScreenRes));

        // Update screen res
        mSurface.getHolder().setFixedSize(videoRenderWidth, videoRenderHeight);
        final FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) mSurface.getLayoutParams();
        params.width = AndroidDevice.nativeWidth;
        params.height = AndroidDevice.nativeHeight;
        params.gravity = Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL;
        mSurface.setLayoutParams(params);

        // Listen to game surface events (created, changed, destroyed)
        mSurface.getHolder().addCallback(this);
        mSurface.createGLContext((ActivityManager) mActivity.getSystemService(Context.ACTIVITY_SERVICE));

        // Configure the action bar introduced in higher Android versions
        if (AndroidDevice.IS_ACTION_BAR_AVAILABLE)
        {
            mActivity.getActionBar().hide();
            ColorDrawable color = new ColorDrawable(Color.parseColor("#303030"));
            color.setAlpha(50 /* mGlobalPrefs.displayActionBarTransparency */);
            mActivity.getActionBar().setBackgroundDrawable(color);
        }

        CreateTouchScreenControls();

        // Initialize user interface devices
        View inputSource = mIsXperiaPlay ? new NativeXperiaTouchpad(mActivity) : mOverlay;
        initControllers(inputSource);

        // Override the peripheral controllers' key provider, to add some extra
        // functionality
        inputSource.setOnKeyListener(this);
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

        mMogaController.onResume();
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
            Boolean pause = false;
            int PauseType = 0;
            if (NativeExports.SettingsLoadBool(SettingsID.GameRunning_CPU_Paused.getValue()) == true)
            {
                pause = true;
                PauseType = NativeExports.SettingsLoadDword(SettingsID.GameRunning_CPU_PausedType.getValue());
                NativeExports.ExternalEvent(SystemEvent.SysEvent_ResumeCPU_FromMenu.getValue());
            }
            int CurrentSaveState = NativeExports.SettingsLoadDword(SettingsID.Game_CurrentSaveState.getValue());
            int OriginalSaveTime = NativeExports.SettingsLoadDword(SettingsID.Game_LastSaveTime.getValue());
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 0);
            NativeExports.ExternalEvent(SystemEvent.SysEvent_SaveMachineState.getValue());
            for (int i = 0; i < 100; i++)
            {
                int LastSaveTime = NativeExports.SettingsLoadDword(SettingsID.Game_LastSaveTime.getValue());
                if (LOG_GAMELIFECYCLEHANDLER)
                {
                    Log.i("GameLifecycleHandler", "LastSaveTime = " + LastSaveTime + " OriginalSaveTime = " + OriginalSaveTime);
                }
                if (LastSaveTime != OriginalSaveTime)
                {
                    break;
                }
                try
                {
                    Thread.sleep(100);
                }
                catch (InterruptedException ex)
                {
                    Thread.currentThread().interrupt();
                }
            }
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), CurrentSaveState);
            if (pause)
            {
                NativeExports.ExternalEvent(SystemEvent.SysEvent_PauseCPU_FromMenu.getValue());
                NativeExports.SettingsSaveDword(SettingsID.GameRunning_CPU_PausedType.getValue(), PauseType);
            }
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
        mIsResumed = false;
        if (NativeExports.SettingsLoadBool(SettingsID.GameRunning_CPU_Running.getValue()) == true)
        {
            AutoSave();
        }
        mMogaController.onPause();
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
        mMogaController.exit();
    }

    public void onSettingDone()
    {
        mtouchscreenScale = ((float) NativeExports.UISettingsLoadDword(UISettingID.TouchScreen_ButtonScale.getValue())) / 100.0f;
        mlayout = NativeExports.UISettingsLoadString(UISettingID.TouchScreen_Layout.getValue());
        mControllers = new ArrayList<AbstractController>();
        CreateTouchScreenControls();

        // Initialize user interface devices
        View inputSource = mIsXperiaPlay ? new NativeXperiaTouchpad(mActivity) : mOverlay;
        initControllers(inputSource);

    }

    private void CreateTouchScreenControls()
    {
        boolean isTouchscreenAnimated = false; // mGlobalPrefs.isTouchscreenAnimated
        boolean isTouchscreenHidden = false; // !isTouchscreenEnabled ||
                                                // globalPrefs.touchscreenTransparency
                                                // == 0;
        String profilesDir = AndroidDevice.PACKAGE_DIRECTORY + "/profiles";
        String touchscreenProfiles_cfg = profilesDir + "/touchscreen.cfg";
        ConfigFile touchscreenConfigFile = new ConfigFile(touchscreenProfiles_cfg);
        ConfigSection section = touchscreenConfigFile.get(mlayout);
        if (section == null)
        {
            mlayout = "Analog";
            section = touchscreenConfigFile.get(mlayout);
        }
        Profile touchscreenProfile = new Profile(true, section);
        int touchscreenTransparency = 100;
        String touchscreenSkinsDir = AndroidDevice.PACKAGE_DIRECTORY + "/skins/touchscreen";
        String touchscreenSkin = touchscreenSkinsDir + "/Outline";

        // The touch map and overlay are needed to display frame rate and/or
        // controls
        mTouchscreenMap = new VisibleTouchMap(mActivity.getResources());
        mTouchscreenMap.load(touchscreenSkin, touchscreenProfile, isTouchscreenAnimated, mtouchscreenScale, touchscreenTransparency);
        mOverlay.initialize(mTouchscreenMap, !isTouchscreenHidden, isTouchscreenAnimated);
    }

    @Override
    public boolean onKey(View view, int keyCode, KeyEvent event)
    {
        // If PeripheralControllers exist and handle the event,
        // they return true. Else they return false, signaling
        // Android to handle the event (menu button, vol keys).
        if (mKeyProvider != null)
        {
            return mKeyProvider.onKey(view, keyCode, event);
        }
        return false;
    }

    @SuppressLint("InlinedApi")
    private void initControllers(View inputSource)
    {
        // By default, send Player 1 rumbles through phone vibrator
        Vibrator vibrator = (Vibrator) mActivity.getSystemService(Context.VIBRATOR_SERVICE);
        int touchscreenAutoHold = 0;
        boolean isTouchscreenFeedbackEnabled = false;
        Set<Integer> autoHoldableButtons = null;

        // Create the touchscreen controller
        TouchController touchscreenController = new TouchController(mTouchscreenMap, inputSource, mOverlay, vibrator,
                touchscreenAutoHold, isTouchscreenFeedbackEnabled, autoHoldableButtons);
        mControllers.add(touchscreenController);

        // Create the input providers shared among all peripheral controllers
        String profile_name = NativeExports.UISettingsLoadString(UISettingID.Controller_CurrentProfile.getValue());
        ConfigFile ControllerConfigFile = new ConfigFile(
                NativeExports.UISettingsLoadString(UISettingID.Controller_ConfigFile.getValue()));
        ConfigSection section = ControllerConfigFile.get(profile_name);
        if (section != null)
        {
            Profile ControllerProfile = new Profile(false, section);
            InputMap map = new InputMap(ControllerProfile.get("map"));

            mKeyProvider = new KeyProvider(inputSource, ImeFormula.DEFAULT, AndroidDevice.getUnmappableKeyCodes());
            MogaProvider mogaProvider = new MogaProvider(mMogaController);
            AbstractProvider axisProvider = AndroidDevice.IS_HONEYCOMB_MR1 ? new AxisProvider(inputSource) : null;
            int Deadzone = NativeExports.UISettingsLoadDword(UISettingID.Controller_Deadzone.getValue());
            int Sensitivity = NativeExports.UISettingsLoadDword(UISettingID.Controller_Sensitivity.getValue());
            mControllers.add(new PeripheralController(1, map, Deadzone, Sensitivity, mKeyProvider, axisProvider, mogaProvider));
        }
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