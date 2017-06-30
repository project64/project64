/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include <stdio.h>
#include <Common/stdtypes.h>
#include <Common/path.h>

CLanguage * g_Lang = NULL;

void CLanguage::LoadDefaultStrings(void)
{
#define DEF_STR(ID,str) m_DefaultStrings.insert(LANG_STRINGS::value_type(ID,str))

    DEF_STR(EMPTY_STRING, "");

    /*********************************************************************************
    * Meta Information                                                               *
    *********************************************************************************/
    //About DLL
    DEF_STR(LANGUAGE_NAME, "");
    DEF_STR(LANGUAGE_AUTHOR, "");
    DEF_STR(LANGUAGE_VERSION, "");
    DEF_STR(LANGUAGE_DATE, "");

    //About DLL Dialog
    DEF_STR(INI_CURRENT_LANG, "Current Language");
    DEF_STR(INI_AUTHOR, "Author");
    DEF_STR(INI_VERSION, "Version");
    DEF_STR(INI_DATE, "Date");
    DEF_STR(INI_HOMEPAGE, "Visit Home Page");
    DEF_STR(INI_CURRENT_RDB, "ROM Database (.RDB)");
    DEF_STR(INI_CURRENT_CHT, "Cheat Code File (.CHT)");
    DEF_STR(INI_CURRENT_RDX, "Extended ROM Info (.RDX)");

    //About INI title
    DEF_STR(INI_TITLE, "About Config Files");

    /*********************************************************************************
    * Numbers                                                                        *
    *********************************************************************************/
    DEF_STR(NUMBER_0, "0");
    DEF_STR(NUMBER_1, "1");
    DEF_STR(NUMBER_2, "2");
    DEF_STR(NUMBER_3, "3");
    DEF_STR(NUMBER_4, "4");
    DEF_STR(NUMBER_5, "5");
    DEF_STR(NUMBER_6, "6");
    DEF_STR(NUMBER_7, "7");
    DEF_STR(NUMBER_8, "8");
    DEF_STR(NUMBER_9, "9");

    /*********************************************************************************
    * Menu                                                                           *
    *********************************************************************************/
    //File Menu
    DEF_STR(MENU_FILE, "&File");
    DEF_STR(MENU_OPEN, "&Open ROM");
    DEF_STR(MENU_ROM_INFO, "ROM &Info....");
    DEF_STR(MENU_START, "Start Emulation");
    DEF_STR(MENU_END, "&End Emulation");
    DEF_STR(MENU_CHOOSE_ROM, "Choose ROM Directory...");
    DEF_STR(MENU_REFRESH, "Refresh ROM List");
    DEF_STR(MENU_RECENT_ROM, "Recent ROM");
    DEF_STR(MENU_RECENT_DIR, "Recent ROM Directories");
    DEF_STR(MENU_EXIT, "E&xit");

    //System Menu
    DEF_STR(MENU_SYSTEM, "&System");
    DEF_STR(MENU_RESET, "&Reset");
    DEF_STR(MENU_PAUSE, "&Pause");
    DEF_STR(MENU_BITMAP, "Generate Bitmap");
    DEF_STR(MENU_LIMIT_FPS, "Limit FPS");
    DEF_STR(MENU_SAVE, "&Save State");
    DEF_STR(MENU_SAVE_AS, "Save As...");
    DEF_STR(MENU_RESTORE, "&Load State");
    DEF_STR(MENU_LOAD, "Load...");
    DEF_STR(MENU_CURRENT_SAVE, "Current Save S&tate");
    DEF_STR(MENU_CHEAT, "Cheats...");
    DEF_STR(MENU_GS_BUTTON, "GS Button");
    DEF_STR(MENU_RESUME, "R&esume");
    DEF_STR(MENU_RESET_SOFT, "&Soft Reset");
    DEF_STR(MENU_RESET_HARD, "&Hard Reset");
    DEF_STR(MENU_SWAPDISK, "Swap &Disk");

    //Options Menu
    DEF_STR(MENU_OPTIONS, "&Options");
    DEF_STR(MENU_FULL_SCREEN, "&Full Screen");
    DEF_STR(MENU_ON_TOP, "&Always on &Top");
    DEF_STR(MENU_CONFG_GFX, "Configure Graphics Plugin...");
    DEF_STR(MENU_CONFG_AUDIO, "Configure Audio Plugin...");
    DEF_STR(MENU_CONFG_CTRL, "Configure Controller Plugin...");
    DEF_STR(MENU_CONFG_RSP, "Configure RSP Plugin...");
    DEF_STR(MENU_SHOW_CPU, "Show CPU Usage");
    DEF_STR(MENU_SETTINGS, "&Settings...");

    //Debugger Menu
    DEF_STR(MENU_DEBUGGER, "&Debugger");

    //Language Menu
    DEF_STR(MENU_LANGUAGE, "&Language");

    //Help Menu
    DEF_STR(MENU_HELP, "&Help");
    DEF_STR(MENU_ABOUT_INI, "About Conf&ig Files");
    DEF_STR(MENU_ABOUT_PJ64, "&About Project64");
    DEF_STR(MENU_FORUM, "Support &Forum");
    DEF_STR(MENU_HOMEPAGE, "&Homepage");

    //Current Save Slot menu
    DEF_STR(MENU_SLOT_DEFAULT, "Default");
    DEF_STR(MENU_SLOT_1, "Slot 1");
    DEF_STR(MENU_SLOT_2, "Slot 2");
    DEF_STR(MENU_SLOT_3, "Slot 3");
    DEF_STR(MENU_SLOT_4, "Slot 4");
    DEF_STR(MENU_SLOT_5, "Slot 5");
    DEF_STR(MENU_SLOT_6, "Slot 6");
    DEF_STR(MENU_SLOT_7, "Slot 7");
    DEF_STR(MENU_SLOT_8, "Slot 8");
    DEF_STR(MENU_SLOT_9, "Slot 9");
    DEF_STR(MENU_SLOT_10, "Slot 10");
    DEF_STR(MENU_SLOT_SAVE, "Save slot (%s) selected");

    //Pop up Menu
    DEF_STR(POPUP_PLAY, "Play Game");
    DEF_STR(POPUP_INFO, "ROM Information");
    DEF_STR(POPUP_SETTINGS, "Edit Game Settings");
    DEF_STR(POPUP_CHEATS, "Edit Cheats");
    DEF_STR(POPUP_GFX_PLUGIN, "Graphics Plugin");
    DEF_STR(POPUP_PLAYDISK, "Play Game with Disk");

    //Alternate Name to save Slot
    DEF_STR(SAVE_SLOT_DEFAULT, "Save Slot - Default");
    DEF_STR(SAVE_SLOT_1, "Save Slot - 1");
    DEF_STR(SAVE_SLOT_2, "Save Slot - 2");
    DEF_STR(SAVE_SLOT_3, "Save Slot - 3");
    DEF_STR(SAVE_SLOT_4, "Save Slot - 4");
    DEF_STR(SAVE_SLOT_5, "Save Slot - 5");
    DEF_STR(SAVE_SLOT_6, "Save Slot - 6");
    DEF_STR(SAVE_SLOT_7, "Save Slot - 7");
    DEF_STR(SAVE_SLOT_8, "Save Slot - 8");
    DEF_STR(SAVE_SLOT_9, "Save Slot - 9");
    DEF_STR(SAVE_SLOT_10, "Save Slot - 10");

    /*********************************************************************************
    * ROM Browser                                                                    *
    *********************************************************************************/
    //ROM Browser Fields
    DEF_STR(RB_FILENAME, "File Name");
    DEF_STR(RB_INTERNALNAME, "Internal Name");
    DEF_STR(RB_GOODNAME, "Good Name");
    DEF_STR(RB_STATUS, "Status");
    DEF_STR(RB_ROMSIZE, "ROM Size");
    DEF_STR(RB_NOTES_CORE, "Notes (core)");
    DEF_STR(RB_NOTES_PLUGIN, "Notes (default plugins)");
    DEF_STR(RB_NOTES_USER, "Notes (user)");
    DEF_STR(RB_CART_ID, "Cartridge ID");
    DEF_STR(RB_MANUFACTUER, "Manufacturer");
    DEF_STR(RB_COUNTRY, "Country");
    DEF_STR(RB_DEVELOPER, "Developer");
    DEF_STR(RB_CRC1, "CRC1");
    DEF_STR(RB_CRC2, "CRC2");
    DEF_STR(RB_CICCHIP, "CIC Chip");
    DEF_STR(RB_RELEASE_DATE, "Release Date");
    DEF_STR(RB_GENRE, "Genre");
    DEF_STR(RB_PLAYERS, "Players");
    DEF_STR(RB_FORCE_FEEDBACK, "Force Feedback");
    DEF_STR(RB_FILE_FORMAT, "File Format");

    //Select ROM
    DEF_STR(SELECT_ROM_DIR, "Select current ROM directory");

    //Messages
    DEF_STR(RB_NOT_GOOD_FILE, "Bad ROM? Use GoodN64 & check that the RDB is up-to-date.");

    /*********************************************************************************
    * Options                                                                        *
    *********************************************************************************/
    //Options Title
    DEF_STR(OPTIONS_TITLE, "Settings");

    //Tabs
    DEF_STR(TAB_PLUGIN, "Plugins");
    DEF_STR(TAB_DIRECTORY, "Directories");
    DEF_STR(TAB_OPTIONS, "Options");
    DEF_STR(TAB_ROMSELECTION, "ROM Selection");
    DEF_STR(TAB_ADVANCED, "Advanced");
    DEF_STR(TAB_ROMSETTINGS, "General Settings");
    DEF_STR(TAB_SHELLINTERGATION, "Shell Integration");
    DEF_STR(TAB_ROMNOTES, "Notes");
    DEF_STR(TAB_SHORTCUTS, "Keyboard Shortcuts");
    DEF_STR(TAB_ROMSTATUS, "Status");
    DEF_STR(TAB_RECOMPILER, "Recompiler");

    //Plugin Dialog
    DEF_STR(PLUG_ABOUT, "About");
    DEF_STR(PLUG_RSP, " RSP (Reality Signal Processor) plugin: ");
    DEF_STR(PLUG_GFX, " Video (graphics) plugin: ");
    DEF_STR(PLUG_AUDIO, " Audio (sound) plugin: ");
    DEF_STR(PLUG_CTRL, " Input (controller) plugin: ");
    DEF_STR(PLUG_HLE_GFX, "Graphics HLE");
    DEF_STR(PLUG_HLE_AUDIO, "Audio HLE");
    DEF_STR(PLUG_DEFAULT, "** Use System Plugin **");

    //Directory Dialog
    DEF_STR(DIR_PLUGIN, " Plugin directory: ");
    DEF_STR(DIR_ROM, " ROM directory: ");
    DEF_STR(DIR_AUTO_SAVE, " N64 native saves directory: ");
    DEF_STR(DIR_INSTANT_SAVE, " Saved states directory: ");
    DEF_STR(DIR_SCREEN_SHOT, " Screenshot directory: ");
    DEF_STR(DIR_ROM_DEFAULT, "Last folder that a ROM was open from");
    DEF_STR(DIR_SELECT_PLUGIN, "Select plugin directory");
    DEF_STR(DIR_SELECT_ROM, "Select ROM directory");
    DEF_STR(DIR_SELECT_AUTO, "Select N64 native saves directory");
    DEF_STR(DIR_SELECT_INSTANT, "Select saved states directory");
    DEF_STR(DIR_SELECT_SCREEN, "Select screenshot directory");
    DEF_STR(DIR_TEXTURE, " Texture pack directory: ");
    DEF_STR(DIR_SELECT_TEXTURE, "Select texture pack directory");

    //Options (general) Tab
    DEF_STR(OPTION_AUTO_SLEEP, "Pause emulation when window is not active");
    DEF_STR(OPTION_AUTO_FULLSCREEN, "Enter full-screen mode when loading a ROM");
    DEF_STR(OPTION_BASIC_MODE, "Hide advanced settings");
    DEF_STR(OPTION_REMEMBER_CHEAT, "Remember selected cheats");
    DEF_STR(OPTION_DISABLE_SS, "Disable screen saver when running a ROM");
    DEF_STR(OPTION_DISPLAY_FR, "Display speed");
    DEF_STR(OPTION_CHECK_RUNNING, "Check if Project64 is already running");
    DEF_STR(OPTION_UNIQUE_SAVE_DIR, "Unique Game Save Directory");
    DEF_STR(OPTION_CHANGE_FR, "Speed display:");
    DEF_STR(OPTION_IPL_ROM_PATH, "64DD IPL ROM Path:");

    //ROM Browser Tab
    DEF_STR(RB_MAX_ROMS, "Max # of ROMs remembered (0-10):");
    DEF_STR(RB_ROMS, "ROMs");
    DEF_STR(RB_MAX_DIRS, "Max # of ROM dirs remembered (0-10):");
    DEF_STR(RB_DIRS, "dirs");
    DEF_STR(RB_USE, "Use ROM browser");
    DEF_STR(RB_DIR_RECURSION, "Use directory recursion");
    DEF_STR(RB_AVALIABLE_FIELDS, "Available fields:");
    DEF_STR(RB_SHOW_FIELDS, "Order of fields:");
    DEF_STR(RB_ADD, "Add ->");
    DEF_STR(RB_REMOVE, "<- Remove");
    DEF_STR(RB_UP, "Up");
    DEF_STR(RB_DOWN, "Down");
    DEF_STR(RB_REFRESH, "Automatically refresh browser");

    //Advanced Options
    DEF_STR(ADVANCE_INFO, "Most of these changes will not take effect until a new ROM is opened or current ROM is reset.");
    DEF_STR(ADVANCE_DEFAULTS, "Core Defaults");
    DEF_STR(ADVANCE_CPU_STYLE, "CPU core style:");
    DEF_STR(ADVANCE_SMCM, "Self-mod methods:");
    DEF_STR(ADVANCE_MEM_SIZE, "Default memory size:");
    DEF_STR(ADVANCE_ABL, "Advanced block linking");
    DEF_STR(ADVANCE_AUTO_START, "Start emulation when ROM is opened");
    DEF_STR(ADVANCE_OVERWRITE, "Always override default settings with ones from RDB");
    DEF_STR(ADVANCE_COMPRESS, "Automatically compress saved states");
    DEF_STR(ADVANCE_DEBUGGER, "Enable debugger");
    DEF_STR(ADVANCE_SMM_CACHE, "Cache");
    DEF_STR(ADVANCE_SMM_PIDMA, "PI DMA");
    DEF_STR(ADVANCE_SMM_VALIDATE, "Start changed");
    DEF_STR(ADVANCE_SMM_PROTECT, "Protect memory");
    DEF_STR(ADVANCE_SMM_TLB, "TLB unmapping");

    //ROM Options
    DEF_STR(ROM_CPU_STYLE, "CPU core style:");
    DEF_STR(ROM_VIREFRESH, "VI refresh rate:");
    DEF_STR(ROM_MEM_SIZE, "Memory size:");
    DEF_STR(ROM_ABL, "Advanced block linking");
    DEF_STR(ROM_SAVE_TYPE, "Default save type:");
    DEF_STR(ROM_COUNTER_FACTOR, "Counter factor:");
    DEF_STR(ROM_LARGE_BUFFER, "Larger compile buffer");
    DEF_STR(ROM_USE_TLB, "Use TLB");
    DEF_STR(ROM_REG_CACHE, "Register caching");
    DEF_STR(ROM_DELAY_SI, "Delay SI interrupt");
    DEF_STR(ROM_FAST_SP, "Fast SP");
    DEF_STR(ROM_DEFAULT, "Default");
    DEF_STR(ROM_AUDIO_SIGNAL, "RSP audio signal");
    DEF_STR(ROM_FIXED_AUDIO, "Fixed audio timing");
    DEF_STR(ROM_FUNC_FIND, "Function lookup method:");
    DEF_STR(ROM_CUSTOM_SMM, "Custom self mod Method");
    DEF_STR(ROM_SYNC_AUDIO, "Sync using audio");
    DEF_STR(ROM_COUNTPERBYTE, "AI count per byte:");
    DEF_STR(ROM_32BIT, "32-bit engine:");
    DEF_STR(ROM_DELAY_DP, "Delay DP interrupt:");
    DEF_STR(ROM_OVER_CLOCK_MODIFIER, "Over Clock Modifier:");

    //Core Styles
    DEF_STR(CORE_INTERPTER, "Interpreter");
    DEF_STR(CORE_RECOMPILER, "Recompiler");
    DEF_STR(CORE_SYNC, "Synchronize cores");

    //Self Mod Methods
    DEF_STR(SMCM_NONE, "None");
    DEF_STR(SMCM_CACHE, "Cache");
    DEF_STR(SMCM_PROECTED, "Protect memory");
    DEF_STR(SMCM_CHECK_MEM, "Check memory & cache");
    DEF_STR(SMCM_CHANGE_MEM, "Change memory & cache");
    DEF_STR(SMCM_CHECK_ADV, "Check memory advance");
    DEF_STR(SMCM_CACHE2, "Clear code on cache");

    //Function Lookup method
    DEF_STR(FLM_PLOOKUP, "Physical lookup table");
    DEF_STR(FLM_VLOOKUP, "Virtual lookup table");
    DEF_STR(FLM_CHANGEMEM, "Change memory");

    //RDRAM Size
    DEF_STR(RDRAM_4MB, "4 MB");
    DEF_STR(RDRAM_8MB, "8 MB");

    //Advanced Block Linking
    DEF_STR(ABL_ON, "On");
    DEF_STR(ABL_OFF, "Off");

    //Save Type
    DEF_STR(SAVE_FIRST_USED, "Use first-used save type");
    DEF_STR(SAVE_4K_EEPROM, "4-kbit EEPROM");
    DEF_STR(SAVE_16K_EEPROM, "16-kbit EEPROM");
    DEF_STR(SAVE_SRAM, "SRAM");
    DEF_STR(SAVE_FLASHRAM, "Flash RAM");

    //Shell Integration Tab
    DEF_STR(SHELL_TEXT, "File extension association:");

    //ROM Notes
    DEF_STR(NOTE_STATUS, "ROM status:");
    DEF_STR(NOTE_CORE, "Core note:");
    DEF_STR(NOTE_PLUGIN, "Plugin note:");

    // Accelerator Selector
    DEF_STR(ACCEL_CPUSTATE_TITLE, "CPU state:");
    DEF_STR(ACCEL_MENUITEM_TITLE, "Menu item:");
    DEF_STR(ACCEL_CURRENTKEYS_TITLE, "Current keys:");
    DEF_STR(ACCEL_SELKEY_TITLE, "Select new shortcut key:");
    DEF_STR(ACCEL_ASSIGNEDTO_TITLE, "Currently assigned to:");
    DEF_STR(ACCEL_ASSIGN_BTN, "Assign");
    DEF_STR(ACCEL_REMOVE_BTN, "Remove");
    DEF_STR(ACCEL_RESETALL_BTN, "Reset all");
    DEF_STR(ACCEL_CPUSTATE_1, "Game not playing");
    DEF_STR(ACCEL_CPUSTATE_2, "Game playing");
    DEF_STR(ACCEL_CPUSTATE_3, "Game playing (windowed)");
    DEF_STR(ACCEL_CPUSTATE_4, "Game playing (full-screen)");
    DEF_STR(ACCEL_DETECTKEY, "Detect Key");

    // Frame Rate Option
    DEF_STR(STR_FR_VIS, "Vertical interrupts per second");
    DEF_STR(STR_FR_DLS, "Display lists per second");
    DEF_STR(STR_FR_PERCENT, "Percentage of full speed");
    DEF_STR(STR_FR_DLS_VIS, "VI/s & DL/s");

    // Increase speed
    DEF_STR(STR_INSREASE_SPEED, "Increase Game Speed");
    DEF_STR(STR_DECREASE_SPEED, "Decrease Game Speed");

    //Bottom page buttons
    DEF_STR(BOTTOM_RESET_PAGE, "Reset Page");
    DEF_STR(BOTTOM_RESET_ALL, "Reset All");
    DEF_STR(BOTTOM_APPLY, "Apply");
    DEF_STR(BOTTOM_CLOSE, "Close");

    /*********************************************************************************
    * ROM Information                                                                *
    *********************************************************************************/
    //ROM Info Title
    DEF_STR(INFO_TITLE, "ROM Information");

    //ROM Info Text
    DEF_STR(INFO_ROM_NAME_TEXT, "ROM name:");
    DEF_STR(INFO_FILE_NAME_TEXT, "File name:");
    DEF_STR(INFO_LOCATION_TEXT, "Location:");
    DEF_STR(INFO_SIZE_TEXT, "ROM size:");
    DEF_STR(INFO_CART_ID_TEXT, "Cartridge ID:");
    DEF_STR(INFO_MANUFACTURER_TEXT, "Manufacturer:");
    DEF_STR(INFO_COUNTRY_TEXT, "Country:");
    DEF_STR(INFO_CRC1_TEXT, "CRC1:");
    DEF_STR(INFO_CRC2_TEXT, "CRC2:");
    DEF_STR(INFO_CIC_CHIP_TEXT, "CIC chip:");
    DEF_STR(INFO_MD5_TEXT, "MD5:");

    /*********************************************************************************
    * Cheats                                                                         *
    *********************************************************************************/
    //Cheat List
    DEF_STR(CHEAT_TITLE, "Cheats");
    DEF_STR(CHEAT_LIST_FRAME, "Cheats:");
    DEF_STR(CHEAT_NOTES_FRAME, " Notes: ");
    DEF_STR(CHEAT_MARK_ALL, "Mark All");
    DEF_STR(CHEAT_MARK_NONE, "Unmark All");

    //Add Cheat
    DEF_STR(CHEAT_ADDCHEAT_FRAME, "Add Cheat");
    DEF_STR(CHEAT_ADDCHEAT_NAME, "Name:");
    DEF_STR(CHEAT_ADDCHEAT_CODE, "Code:");
    DEF_STR(CHEAT_ADDCHEAT_INSERT, "Insert");
    DEF_STR(CHEAT_ADDCHEAT_CLEAR, "Clear");
    DEF_STR(CHEAT_ADDCHEAT_NOTES, " Cheat Notes: ");
    DEF_STR(CHEAT_ADD_TO_DB, "Add to DB");

    //Code extension
    DEF_STR(CHEAT_CODE_EXT_TITLE, "Code Extensions");
    DEF_STR(CHEAT_CODE_EXT_TXT, "Please choose a value to be used for:");
    DEF_STR(CHEAT_OK, "OK");
    DEF_STR(CHEAT_CANCEL, "Cancel");

    //Digital Value
    DEF_STR(CHEAT_QUANTITY_TITLE, "Quantity Digit");
    DEF_STR(CHEAT_CHOOSE_VALUE, "Please choose a value for:");
    DEF_STR(CHEAT_VALUE, "&Value");
    DEF_STR(CHEAT_FROM, "from");
    DEF_STR(CHEAT_TO, "to");
    DEF_STR(CHEAT_NOTES, "&Notes:");
    DEF_STR(CHEAT_ADDCHEAT_ADD, "Add Cheat");
    DEF_STR(CHEAT_ADDCHEAT_NEW, "New Cheat");
    DEF_STR(CHEAT_ADDCHEAT_CODEDES, "<address> <value>");
    DEF_STR(CHEAT_ADDCHEAT_OPT, "Options:");
    DEF_STR(CHEAT_ADDCHEAT_OPTDES, "<value> <label>");

    //Edit Cheat
    DEF_STR(CHEAT_EDITCHEAT_WINDOW, "Edit Cheat");
    DEF_STR(CHEAT_EDITCHEAT_UPDATE, "Update Cheat");
    DEF_STR(CHEAT_CHANGED_MSG, "Cheat has been changed.\n\nDo you want to update?");
    DEF_STR(CHEAT_CHANGED_TITLE, "Cheat updated");

    //Cheat Popup Menu
    DEF_STR(CHEAT_ADDNEW, "Add New Cheat...");
    DEF_STR(CHEAT_EDIT, "Edit");
    DEF_STR(CHEAT_DELETE, "Delete");

    // short-cut editor
    DEF_STR(STR_SHORTCUT_RESET_TITLE, "Reset short-cuts");
    DEF_STR(STR_SHORTCUT_RESET_TEXT, "Are you sure you want to reset the short-cuts?\n\nThis action cannot be undone.");
    DEF_STR(STR_SHORTCUT_FILEMENU, "File Menu");
    DEF_STR(STR_SHORTCUT_SYSTEMMENU, "System Menu");
    DEF_STR(STR_SHORTCUT_OPTIONS, "Options");
    DEF_STR(STR_SHORTCUT_SAVESLOT, "Save Slots");

    /*********************************************************************************
    * Support Window                                                                 *
    *********************************************************************************/
    DEF_STR(MSG_SUPPORT_TITLE, "Support Project64");
    DEF_STR(MSG_SUPPORT_INFO, "Project64 is a software package designed to emulate a Nintendo64 video game system on a Microsoft Windows based PC. This allows you to play real N64 software in much the same way as it would be on the original hardware system.\n\nIf you like Project64 and have gotten some value out of it then please support project64 as either a thank you, or your desire to see it continually improved.\n\nIf you have supported project64:");
    DEF_STR(MSG_SUPPORT_ENTER_CODE, "Enter notification code");
    DEF_STR(MSG_SUPPORT_PROJECT64, "Support Project64");
    DEF_STR(MSG_SUPPORT_CONTINUE, "Continue");
    DEF_STR(MSG_SUPPORT_ENTER_SUPPORT_CODE, "Please enter the support code");
    DEF_STR(MSG_SUPPORT_INCORRECT_CODE, "Incorrect support code");
    DEF_STR(MSG_SUPPORT_COMPLETE, "Thank you");
    DEF_STR(MSG_SUPPORT_ENTER_CODE_TITLE, "Enter code");
    DEF_STR(MSG_SUPPORT_ENTER_CODE_DESC, "Please enter the code in the email");
    DEF_STR(MSG_SUPPORT_OK, "OK");
    DEF_STR(MSG_SUPPORT_CANCEL, "Cancel");

    /*********************************************************************************
    * Messages                                                                       *
    *********************************************************************************/
    DEF_STR(MSG_CPU_PAUSED, "*** CPU PAUSED ***");
    DEF_STR(MSG_CPU_RESUMED, "CPU resumed");
    DEF_STR(MSG_PERM_LOOP, "In a permanent loop that cannot be exited.\nEmulation will now stop.\n\nVerify ROM and its settings.");
    DEF_STR(MSG_MEM_ALLOC_ERROR, "Failed to allocate memory");
    DEF_STR(MSG_FAIL_INIT_GFX, "The default or selected video plugin is missing or invalid.\n\nYou need to go into Settings and select a video (graphics) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder.");
    DEF_STR(MSG_FAIL_INIT_AUDIO, "The default or selected audio plugin is missing or invalid.\n\nYou need to go into Settings and select a audio (sound) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder.");
    DEF_STR(MSG_FAIL_INIT_RSP, "The default or selected RSP plugin is missing or invalid.\n\nYou need to go into Settings and select a RSP (Reality Signal Processor) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder.");
    DEF_STR(MSG_FAIL_INIT_CONTROL, "The default or selected input plugin is missing or invalid.\n\nYou need to go into Settings and select an input (controller) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder.");
    DEF_STR(MSG_FAIL_LOAD_PLUGIN, "Failed to load plugin:");
    DEF_STR(MSG_FAIL_LOAD_WORD, "Failed to load word.\n\nVerify ROM and its settings.");
    DEF_STR(MSG_FAIL_OPEN_SAVE, "Failed to open save file");
    DEF_STR(MSG_FAIL_OPEN_EEPROM, "Failed to open EEPROM");
    DEF_STR(MSG_FAIL_OPEN_FLASH, "Failed to open flash RAM");
    DEF_STR(MSG_FAIL_OPEN_MEMPAK, "Failed to open mempak");
    DEF_STR(MSG_FAIL_OPEN_ZIP, "Attempt to open zip file failed.\n\nProbably a corrupt zip file - try unzipping ROM manually.");
    DEF_STR(MSG_FAIL_OPEN_IMAGE, "Attempt to open file failed.");
    DEF_STR(MSG_FAIL_ZIP, "Error occurred when trying to open zip file.");
    DEF_STR(MSG_FAIL_IMAGE, "File loaded does not appear to be a valid N64 ROM.\n\nVerify your ROMs with GoodN64.");
    DEF_STR(MSG_UNKNOWN_COUNTRY, "Unknown country");
    DEF_STR(MSG_UNKNOWN_CIC_CHIP, "Unknown CIC chip");
    DEF_STR(MSG_UNKNOWN_FILE_FORMAT, "Unknown file format");
    DEF_STR(MSG_UNKNOWN_MEM_ACTION, "Unknown memory action\n\nEmulation stopped");
    DEF_STR(MSG_UNHANDLED_OP, "Unhandled R4300i opcode at");
    DEF_STR(MSG_NONMAPPED_SPACE, "Executing from non-mapped space.\n\nVerify ROM and its settings.");
    DEF_STR(MSG_SAVE_STATE_HEADER, "This saved state does not appear to match the running ROM.\n\nStates must be saved & loaded between 100% identical ROMs.\nIn particular, the REGION and VERSION need to be the same.\nLoading this state is likely to cause the game and/or emulator to crash.\n\nAre you sure you want to continue loading?");
    DEF_STR(MSG_MSGBOX_TITLE, "Error");
    DEF_STR(MSG_PIF2_ERROR, "Copyright sequence not found in LUT. Game will no longer function.");
    DEF_STR(MSG_PIF2_TITLE, "Copy Protection Failure");
    DEF_STR(MSG_PLUGIN_CHANGE, "Changing a plugin requires Project64 to reset a running ROM.\nIf you don't want to lose your place, answer No and save the current state first.\n\nChange plugins and reset ROM now?");
    DEF_STR(MSG_PLUGIN_CHANGE_TITLE, "Change Plugins");
    DEF_STR(MSG_EMULATION_ENDED, "Emulation ended");
    DEF_STR(MSG_EMULATION_STARTED, "Emulation started");
    DEF_STR(MSG_UNABLED_LOAD_STATE, "Unable to load state");
    DEF_STR(MSG_LOADED_STATE, "Loaded state");
    DEF_STR(MSG_SAVED_STATE, "Saved current state to");
    DEF_STR(MSG_SAVE_SLOT, "State slot");
    DEF_STR(MSG_BYTESWAP, "Byte-swapping image");
    DEF_STR(MSG_CHOOSE_IMAGE, "Choosing N64 image");
    DEF_STR(MSG_LOADED, "Loaded");
    DEF_STR(MSG_LOADING, "Loading image");
    DEF_STR(MSG_PLUGIN_NOT_INIT, "Cannot open a ROM because plugins have not successfully initialized.");
    DEF_STR(MSG_DEL_SURE, "Are you sure you really want to delete this?");
    DEF_STR(MSG_DEL_TITLE, "Delete Cheat");
    DEF_STR(MSG_CHEAT_NAME_IN_USE, "Cheat name is already in use.");
    DEF_STR(MSG_MAX_CHEATS, "You have reached the maximum amount of cheats for this ROM.");
    DEF_STR(MSG_PLUGIN_INIT, "Plugin initializing");
    DEF_STR(MSG_NO_SHORTCUT_SEL, "You have not selected a virtual key to assign to the menu item.");
    DEF_STR(MSG_NO_MENUITEM_SEL, "You need to select a menu item to assign this key to.");
    DEF_STR(MSG_MENUITEM_ASSIGNED, "Short-cut has already been assigned to another menu item.");
    DEF_STR(MSG_NO_SEL_SHORTCUT, "No shortcut has been selected to be removed.");
    DEF_STR(MSG_WAITING_FOR_START, "ROM loaded. Waiting for emulation to start.");
    DEF_STR(MSG_INVALID_EXE, "Project64 beta versions are for members only.\n\nIf you have an account at www.pj64-emu.com, you should not be seeing this error!!\nPlease contact us on the site.");
    DEF_STR(MSG_INVALID_EXE_TITLE, "Program Error");
    DEF_STR(MSG_7Z_FILE_NOT_FOUND, "Failed to find filename in 7z file");
    DEF_STR(MSG_SET_LLE_GFX_TITLE, "Graphics Low-Level Emulation");
    DEF_STR(MSG_SET_LLE_GFX_MSG, "Graphics LLE is not for general use!!!\nIt is advisable that you only use this for testing and not for playing games.\n\nChange to graphics LLE?");
    DEF_STR(MSG_SET_HLE_AUD_TITLE, "Audio High-Level Emulation");
    DEF_STR(MSG_SET_HLE_AUD_MSG, "Audio HLE requires a third-party plugin!!!\nIf you do not use a third-party audio plugin that supports HLE, you will hear no sound.\n\nChange to audio HLE?");

    /*********************************************************************************
    * Android                                                                        *
    *********************************************************************************/
    DEF_STR(ANDROID_SETTINGS, "Settings");
    DEF_STR(ANDROID_FORUM, "Help/Forum");
    DEF_STR(ANDROID_REPORT_BUG, "Report Issue");
    DEF_STR(ANDROID_ABOUT, "About");
    DEF_STR(ANDROID_GALLERY_RECENTLYPLAYED, "Recently played");
    DEF_STR(ANDROID_GALLERY_LIBRARY, "Games");
    DEF_STR(ANDROID_GAMEDIR, "Game Dir");
    DEF_STR(ANDROID_SELECTDIR, "Select an folder to scan");
    DEF_STR(ANDROID_INCLUDE_SUBDIRECTORIES, "Include subdirectories");
    DEF_STR(ANDROID_PARENTFOLDER, "Parent folder");
    DEF_STR(ANDROID_DIRECTORIES, "Directories");
    DEF_STR(ANDROID_INTERNAL_MEMORY, "Internal memory");
    DEF_STR(ANDROID_TITLE, "Scanning...");
    DEF_STR(ANDROID_OK, "OK");
    DEF_STR(ANDROID_CANCEL, "Cancel");
    DEF_STR(ANDROID_ABOUT_INFO, "Information");
    DEF_STR(ANDROID_ABOUT_APP_NAME, "Project64 for Android");
    DEF_STR(ANDROID_ABOUT_LICENCE, "License");
    DEF_STR(ANDROID_ABOUT_REVISION, "Revision");
    DEF_STR(ANDROID_ABOUT_TEXT, "Project64 for Android\u2122 is a port of the windows version of project64.The Android\u2122 version can play most N64 games.");
    DEF_STR(ANDROID_ABOUT_PJ64_AUTHORS, "Project64 Authors.");

    //In game menu
    DEF_STR(ANDROID_MENU_SETTINGS, "Settings");
    DEF_STR(ANDROID_MENU_SAVESTATE, "Save State");
    DEF_STR(ANDROID_MENU_LOADSTATE, "Load State");
    DEF_STR(ANDROID_MENU_ENDEMULATION, "End Emulation");
    DEF_STR(ANDROID_MENU_PAUSE, "Pause");
    DEF_STR(ANDROID_MENU_RESUME, "Resume");
    DEF_STR(ANDROID_MENU_GAMESPEED, "Game Speed");
    DEF_STR(ANDROID_MENU_CURRENTSAVESTATE, "Current Save State...");
    DEF_STR(ANDROID_MENU_CURRENTSAVEAUTO, "Auto");
    DEF_STR(ANDROID_MENU_CURRENTSAVESLOT, "Slot");
    DEF_STR(ANDROID_MENU_CONSOLERESET, "Reset");
    DEF_STR(ANDROID_MENU_DEBUGGINGOPTIONS, "Debugging Options");
    DEF_STR(ANDROID_MENU_RESETFUNCTIONTIMES, "Reset Function Times");
    DEF_STR(ANDROID_MENU_DUMPFUNCTIONTIMES, "Dump Function Times");

    //Video plugin
    DEF_STR(ANDROID_VIDEO_NATIVE_RES, "Native");
}

CLanguage::CLanguage() :
    m_emptyString(""),
    m_LanguageLoaded(false)
{
    LoadDefaultStrings();
    if (g_Settings)
    {
        m_SelectedLanguage = g_Settings->LoadStringVal(Setting_CurrentLanguage);
        g_Settings->RegisterChangeCB(Debugger_DebugLanguage, this, (CSettings::SettingChangedFunc)StaticResetStrings);
    }
}

CLanguage::~CLanguage()
{
    if (g_Settings)
    {
        g_Settings->UnregisterChangeCB(Debugger_DebugLanguage, this, (CSettings::SettingChangedFunc)StaticResetStrings);
    }
}

bool CLanguage::LoadCurrentStrings(void)
{
    //clear all the current strings loaded
    m_CurrentStrings.clear();

    if (g_Settings->LoadBool(Debugger_DebugLanguage))
    {
        m_LanguageLoaded = true;
        return true;
    }

    LanguageList LangList = GetLangList();
    stdstr Filename;

    //Find the file name of the current language
    for (LanguageList::iterator Language = LangList.begin(); Language != LangList.end(); Language++)
    {
        if (g_Lang->IsCurrentLang(*Language))
        {
            Filename = Language->Filename;
            break;
        }
    }

    if (Filename.length() == 0)
    {
        return false;
    }

    //Process the file
    FILE *file = fopen(Filename.c_str(), "rb");
    if (file == NULL)
    {
        return false;
    }

    //Search for utf8 file marker
    uint8_t utf_bom[3];
    if (fread(&utf_bom, sizeof(utf_bom), 1, file) != 1 ||
        utf_bom[0] != 0xEF ||
        utf_bom[1] != 0xBB ||
        utf_bom[2] != 0xBF)
    {
        fclose(file);
        return false;
    }

    //String;
    while (!feof(file))
    {
        m_CurrentStrings.insert(GetNextLangString(file));
    }
    fclose(file);
    m_LanguageLoaded = true;
    return true;
}

LanguageList & CLanguage::GetLangList(void)
{
    if (m_LanguageList.size() > 0)
    {
        return m_LanguageList;
    }

    CPath LanguageFiles(g_Settings->LoadStringVal(Setting_LanguageDir), "*.pj.Lang");
    if (LanguageFiles.FindFirst())
    {
        do
        {
            LanguageFile File; //We temporally store the values in here to added to the list

            File.Filename = (const std::string &)LanguageFiles;
            File.LanguageName = GetLangString(LanguageFiles, LANGUAGE_NAME);

            if (File.LanguageName.length() == 0)
            {
                continue;
            }

            //get the name of the language from inside the file
            m_LanguageList.push_back(File);
        } while (LanguageFiles.FindNext());
    }
    return m_LanguageList;
}

const std::string & CLanguage::GetString(LanguageStringID StringID)
{
    LANG_STRINGS::iterator CurrentString = m_CurrentStrings.find(StringID);
    if (CurrentString != m_CurrentStrings.end())
    {
        return CurrentString->second;
    }

    if (g_Settings->LoadBool(Debugger_DebugLanguage))
    {
        std::pair<LANG_STRINGS::iterator, bool> ret = m_CurrentStrings.insert(LANG_STRINGS::value_type(StringID, stdstr_f("#%d#", StringID)));
        if (ret.second)
        {
            return ret.first->second;
        }
    }

    LANG_STRINGS::iterator DefString = m_DefaultStrings.find(StringID);
    if (DefString != m_DefaultStrings.end())
    {
        return DefString->second;
    }
#ifdef _DEBUG
    g_Notify->BreakPoint(__FILE__, __LINE__);
#endif
    return m_emptyString;
}

std::string CLanguage::GetLangString(const char * FileName, LanguageStringID ID)
{
    FILE *file = fopen(FileName, "rb");
    if (file == NULL)
    {
        return "";
    }

    //Search for utf8 file marker
    uint8_t utf_bom[3];
    if (fread(&utf_bom, sizeof(utf_bom), 1, file) != 1 ||
        utf_bom[0] != 0xEF ||
        utf_bom[1] != 0xBB ||
        utf_bom[2] != 0xBF)
    {
        fclose(file);
        return "";
    }

    //String;
    while (!feof(file))
    {
        LANG_STR String = GetNextLangString(file);
        if (String.first == ID)
        {
            fclose(file);
            return String.second;
        }
    }
    fclose(file);
    return "";
}

LANG_STR CLanguage::GetNextLangString(void * OpenFile)
{
    enum { MAX_STRING_LEN = 800 };
    int32_t  StringID;
    char szString[MAX_STRING_LEN];  //temp store the string from the file

    FILE * file = (FILE *)OpenFile;

    //while(token!='#' && !feof(file)) { fread(&token, 1, 1, file); }
    if (feof(file))
    {
        return LANG_STR(0, "");
    }

    //Search for token #
    char token = 0;
    while (token != '#' && !feof(file))
    {
        fread(&token, 1, 1, file);
    }
    if (feof(file))
    {
        return LANG_STR(0, "");
    }

    //get StringID after token
    fscanf(file, "%d", &StringID);

    //Search for token #
    while (token != '#' && !feof(file))
    {
        fread(&token, 1, 1, file);
    }
    if (feof(file))
    {
        StringID = EMPTY_STRING; return LANG_STR(0, "");
    }

    //Search for start of string '"'
    while (token != '"' && !feof(file))
    {
        fread(&token, 1, 1, file);
    }
    if (feof(file))
    {
        StringID = EMPTY_STRING; return LANG_STR(0, "");
    }

    int32_t pos = 0;
    fread(&token, 1, 1, file);
    while (token != '"' && !feof(file))
    {
        szString[pos++] = token;
        fread(&token, 1, 1, file);
        if (pos == MAX_STRING_LEN - 2)
        {
            token = '"';
        }
    }
    szString[pos++] = 0;
    stdstr text(szString);
    text.Replace("\\n", "\n");
    return LANG_STR(StringID, text);
}

void CLanguage::SetLanguage(const char * LanguageName)
{
    m_SelectedLanguage = LanguageName;
    if (LoadCurrentStrings())
    {
        g_Settings->SaveString(Setting_CurrentLanguage, LanguageName);
    }
}

bool CLanguage::IsCurrentLang(LanguageFile & File)
{
    if (m_SelectedLanguage == File.LanguageName)
    {
        return true;
    }
    return false;
}

void CLanguage::ResetStrings(void)
{
    m_CurrentStrings.clear();
}

#ifdef _WIN32
const std::wstring wGS(LanguageStringID StringID)
{
    return stdstr(g_Lang->GetString(StringID)).ToUTF16();
}
#endif