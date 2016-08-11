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
import emu.project64.jni.NativeExports;
import emu.project64.jni.SettingsID;
import emu.project64.jni.SystemEvent;
import emu.project64.settings.SettingsActivity;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageButton;
import android.widget.PopupMenu;

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

                NativeExports.ExternalEvent( SystemEvent.SysEvent_PauseCPU_AppLostActive.getValue());

                PopupMenu popupMenu = new PopupMenu(activityContext, MenuButton);
                popupMenu.setOnDismissListener(menuHandler);
                popupMenu.setOnMenuItemClickListener(menuHandler);
                popupMenu.inflate(R.menu.game_activity);
                
                int CurrentSaveState = NativeExports.SettingsLoadDword(SettingsID.Game_CurrentSaveState.getValue());
                Menu menu = popupMenu.getMenu();

                menu.findItem(R.id.menuItem_pause).setVisible(GamePaused ? false : true);
                menu.findItem(R.id.menuItem_resume).setVisible(GamePaused ? true : false);
                
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
            mOpeningSubmenu = true;
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
        case R.id.menuItem_EndEmulation:
            NativeExports.ExternalEvent( SystemEvent.SysEvent_ResumeCPU_FromMenu.getValue());
            mLifecycleHandler.AutoSave();
            NativeExports.CloseSystem();
            break;
        case R.id.menuItem_settings:
            Intent SettingsIntent = new Intent(mActivity, SettingsActivity.class);
            mActivity.startActivity( SettingsIntent );
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
        String SaveFileName = SaveDirectory + "/" + NativeExports.SettingsLoadString(SettingsID.Game_GoodName.getValue()) + ".pj";
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
        String SlotName = SaveSlot == 0 ? "Auto" : "Slot " + SaveSlot;
        item.setTitle(SlotName + Timestamp);        
    }
}
