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

import java.io.File;
import java.sql.Date;
import java.text.SimpleDateFormat;

import emu.project64.R;
import emu.project64.jni.LanguageStringID;
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;
import emu.project64.jni.SystemEvent;
import emu.project64.settings.SettingsActivity;
import emu.project64.util.Strings;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Intent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageButton;
import android.widget.PopupMenu;
import android.widget.SeekBar;
import android.widget.TextView;

public class GameMenuHandler implements PopupMenu.OnMenuItemClickListener, PopupMenu.OnDismissListener
{
    private Activity mActivity = null;
    private GameLifecycleHandler mLifecycleHandler = null;
    private Boolean mOpeningSubmenu = false;

    public GameMenuHandler( Activity activity, GameLifecycleHandler LifecycleHandler )
    {
        mActivity = activity;
        mLifecycleHandler = LifecycleHandler;

        final ImageButton MenuButton = (ImageButton)activity.findViewById( R.id.gameMenu );
        final Activity activityContext = activity;
        final GameMenuHandler menuHandler = this;
        MenuButton.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                Boolean GamePaused = NativeExports.SettingsLoadBool(SettingsID.GameRunning_CPU_Paused.getValue());
                Boolean RecordExecutionTimes = NativeExports.SettingsLoadBool(SettingsID.Debugger_RecordExecutionTimes.getValue());
                Boolean ShowDebugMenu = NativeExports.SettingsLoadBool(SettingsID.Debugger_Enabled.getValue());

                if (!RecordExecutionTimes)
                {
                    ShowDebugMenu = false;
                }

                NativeExports.ExternalEvent( SystemEvent.SysEvent_PauseCPU_AppLostActive.getValue());

                PopupMenu popupMenu = new PopupMenu(activityContext, MenuButton);
                popupMenu.setOnDismissListener(menuHandler);
                popupMenu.setOnMenuItemClickListener(menuHandler);
                popupMenu.inflate(R.menu.game_activity);

                int CurrentSaveState = NativeExports.SettingsLoadDword(SettingsID.Game_CurrentSaveState.getValue());
                Menu menu = popupMenu.getMenu();

                Strings.SetMenuTitle(menu, R.id.menuItem_SaveState, LanguageStringID.ANDROID_MENU_SAVESTATE);
                Strings.SetMenuTitle(menu, R.id.menuItem_LoadState, LanguageStringID.ANDROID_MENU_LOADSTATE);
                Strings.SetMenuTitle(menu, R.id.menuItem_CurrentSaveState, LanguageStringID.ANDROID_MENU_CURRENTSAVESTATE);
                Strings.SetMenuTitle(menu, R.id.menuItem_CurrentSaveAuto, LanguageStringID.ANDROID_MENU_CURRENTSAVESTATE);
                Strings.SetMenuTitle(menu, R.id.menuItem_GameSpeed, LanguageStringID.ANDROID_MENU_GAMESPEED);
                Strings.SetMenuTitle(menu, R.id.menuItem_settings, LanguageStringID.ANDROID_MENU_SETTINGS);
                Strings.SetMenuTitle(menu, R.id.menuItem_DebuggingMenu, LanguageStringID.ANDROID_MENU_DEBUGGINGOPTIONS);
                Strings.SetMenuTitle(menu, R.id.menuItem_ResetFunctionTimes, LanguageStringID.ANDROID_MENU_RESETFUNCTIONTIMES);
                Strings.SetMenuTitle(menu, R.id.menuItem_DumpFunctionTimes, LanguageStringID.ANDROID_MENU_DUMPFUNCTIONTIMES);
                Strings.SetMenuTitle(menu, R.id.menuItem_pause, LanguageStringID.ANDROID_MENU_PAUSE);
                Strings.SetMenuTitle(menu, R.id.menuItem_resume, LanguageStringID.ANDROID_MENU_RESUME);
                Strings.SetMenuTitle(menu, R.id.menuItem_HardReset, LanguageStringID.ANDROID_MENU_CONSOLERESET);
                Strings.SetMenuTitle(menu, R.id.menuItem_EndEmulation, LanguageStringID.ANDROID_MENU_ENDEMULATION);

                menu.findItem(R.id.menuItem_pause).setVisible(GamePaused ? false : true);
                menu.findItem(R.id.menuItem_resume).setVisible(GamePaused ? true : false);
                menu.findItem(R.id.menuItem_ResetFunctionTimes).setVisible(RecordExecutionTimes);
                menu.findItem(R.id.menuItem_DumpFunctionTimes).setVisible(RecordExecutionTimes);
                menu.findItem(R.id.menuItem_DebuggingMenu).setVisible(ShowDebugMenu);

                String SaveDirectory = NativeExports.SettingsLoadString(SettingsID.Directory_InstantSave.getValue());
                if ( NativeExports.SettingsLoadBool(SettingsID.Setting_UniqueSaveDir.getValue()))
                {
                    SaveDirectory += "/" + NativeExports.SettingsLoadString(SettingsID.Game_UniqueSaveDir.getValue());
                }

                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSaveAuto, 0);
                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSave1, 1);
                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSave2, 2);
                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSave3, 3);
                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSave4, 4);
                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSave5, 5);
                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSave6, 6);
                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSave7, 7);
                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSave8, 8);
                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSave9, 9);
                FixSaveStateMenu(SaveDirectory, CurrentSaveState, menu, R.id.menuItem_CurrentSave10, 10);
                popupMenu.show();
            }
        });
    }

    public boolean onMenuItemClick(MenuItem item)
    {
        switch (item.getItemId())
        {
        case R.id.menuItem_CurrentSaveState:
        case R.id.menuItem_DebuggingMenu:
            mOpeningSubmenu = true;
            break;
        case R.id.menuItem_GameSpeed:
            mOpeningSubmenu = true;
            SelectGameSpeed();
            break;
        case R.id.menuItem_SaveState:
            NativeExports.ExternalEvent(SystemEvent.SysEvent_SaveMachineState.getValue());
            break;
        case R.id.menuItem_LoadState:
            NativeExports.ExternalEvent(SystemEvent.SysEvent_LoadMachineState.getValue());
            break;
       case R.id.menuItem_CurrentSaveAuto:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 0);
            break;
        case R.id.menuItem_CurrentSave1:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 1);
            break;
        case R.id.menuItem_CurrentSave2:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 2);
            break;
        case R.id.menuItem_CurrentSave3:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 3);
            break;
        case R.id.menuItem_CurrentSave4:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 4);
            break;
        case R.id.menuItem_CurrentSave5:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 5);
            break;
        case R.id.menuItem_CurrentSave6:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 6);
            break;
        case R.id.menuItem_CurrentSave7:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 7);
            break;
        case R.id.menuItem_CurrentSave8:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 8);
            break;
        case R.id.menuItem_CurrentSave9:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 9);
            break;
        case R.id.menuItem_CurrentSave10:
            NativeExports.SettingsSaveDword(SettingsID.Game_CurrentSaveState.getValue(), 10);
            break;
        case R.id.menuItem_pause:
            NativeExports.ExternalEvent( SystemEvent.SysEvent_PauseCPU_FromMenu.getValue());
            break;
        case R.id.menuItem_resume:
            NativeExports.ExternalEvent( SystemEvent.SysEvent_ResumeCPU_FromMenu.getValue());
            break;
        case R.id.menuItem_HardReset:
            NativeExports.ExternalEvent( SystemEvent.SysEvent_ResetCPU_Hard.getValue());
            break;
        case R.id.menuItem_ResetFunctionTimes:
            NativeExports.ExternalEvent( SystemEvent.SysEvent_ResetFunctionTimes.getValue());
            break;
        case R.id.menuItem_DumpFunctionTimes:
            NativeExports.ExternalEvent( SystemEvent.SysEvent_DumpFunctionTimes.getValue());
            break;
        case R.id.menuItem_EndEmulation:
            NativeExports.ExternalEvent( SystemEvent.SysEvent_ResumeCPU_FromMenu.getValue());
            mLifecycleHandler.AutoSave();
            NativeExports.CloseSystem();
            break;
        case R.id.menuItem_settings:
            Intent SettingsIntent = new Intent(mActivity, SettingsActivity.class);
            mActivity.startActivityForResult( SettingsIntent, GameLifecycleHandler.RC_SETTINGS );
            return true;
        }
        return false;
    }

    public void onDismiss (PopupMenu menu)
    {
        if (!mOpeningSubmenu)
        {
            NativeExports.ExternalEvent( SystemEvent.SysEvent_ResumeCPU_AppGainedActive.getValue());
        }
        mOpeningSubmenu = false;
    }

    @SuppressLint("SimpleDateFormat")
    private void FixSaveStateMenu(String SaveDirectory, int CurrentSaveState,Menu menu, int MenuId, int SaveSlot )
    {
        MenuItem item = menu.findItem(MenuId);
        if (CurrentSaveState == SaveSlot)
        {
            item.setChecked(true);
        }
        String SaveFileName = SaveDirectory + "/" + NativeExports.SettingsLoadString(SettingsID.Rdb_GoodName.getValue()) + ".pj";
        String Timestamp = "";
        if (SaveSlot != 0)
        {
            SaveFileName += SaveSlot;
        }
        File SaveFile = new File(SaveFileName+".zip");
        long LastModified = SaveFile.lastModified();
        if (LastModified == 0)
        {
            SaveFile = new File(SaveFileName);
            LastModified = SaveFile.lastModified();
        }
        if (LastModified != 0)
        {
            Timestamp = new SimpleDateFormat(" [yyyy/MM/dd HH:mm]").format(new Date(LastModified));
        }
        String SlotName = SaveSlot == 0 ? Strings.GetString(LanguageStringID.ANDROID_MENU_CURRENTSAVEAUTO) :
        Strings.GetString(LanguageStringID.ANDROID_MENU_CURRENTSAVESLOT) + " " + SaveSlot;
        item.setTitle(SlotName + Timestamp);
    }

    private void SelectGameSpeed()
    {
        NativeExports.ExternalEvent( SystemEvent.SysEvent_PauseCPU_AppLostActive.getValue());
        final int MAX_SPEED = 300;
        final int MIN_SPEED = 10;
        final int initial = (NativeExports.GetSpeed() * 100) / NativeExports.GetBaseSpeed();
        final View layout = View.inflate( mActivity, R.layout.seek_bar_preference, null );
        final SeekBar seek = (SeekBar) layout.findViewById( R.id.seekbar );
        final TextView text = (TextView) layout.findViewById( R.id.textFeedback );
        final String finalFormat = "%1$d %%";

        text.setText( String.format( finalFormat, initial ) );
        seek.setMax( MAX_SPEED - MIN_SPEED );
        seek.setProgress( initial - MIN_SPEED );
        seek.setOnSeekBarChangeListener( new SeekBar.OnSeekBarChangeListener()
        {
            public void onProgressChanged( SeekBar seekBar, int progress, boolean fromUser )
            {
                text.setText( String.format( finalFormat, progress + MIN_SPEED ) );
            }

            public void onStartTrackingTouch( SeekBar seekBar )
            {
            }

            public void onStopTrackingTouch( SeekBar seekBar )
            {
            }
        });

        Builder builder = new Builder(mActivity);
        builder.setTitle(Strings.GetString(LanguageStringID.ANDROID_MENU_GAMESPEED));
        builder.setPositiveButton("Cancel", null);
        builder.setNeutralButton("OK", null);
        builder.setNegativeButton("Reset", null);
        builder.setCancelable(false);
        builder.setView(layout);

        final AlertDialog dialog = builder.create();
        dialog.show();
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener( new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                dialog.dismiss();
                NativeExports.ExternalEvent( SystemEvent.SysEvent_ResumeCPU_AppGainedActive.getValue());
            }
        });
        dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener( new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                int speed = ((seek.getProgress() + MIN_SPEED) * NativeExports.GetBaseSpeed()) / 100;
                NativeExports.SetSpeed(speed);
                dialog.dismiss();
                NativeExports.ExternalEvent( SystemEvent.SysEvent_ResumeCPU_AppGainedActive.getValue());
            }
        });
        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener( new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                NativeExports.SetSpeed(NativeExports.GetBaseSpeed());
                dialog.dismiss();
                NativeExports.ExternalEvent( SystemEvent.SysEvent_ResumeCPU_AppGainedActive.getValue());
            }
        });
    }
}
