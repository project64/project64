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
#include "stdafx.h"

CLanguage * _Lang = NULL;

void CLanguage::LoadDefaultStrings (void)
{
#define DEF_STR(ID,str) m_DefaultStrings.insert(LANG_STRINGS::value_type(ID,str))

	DEF_STR(EMPTY_STRING,        ""                        );

/*********************************************************************************
* Meta Information                                                               *
*********************************************************************************/
//About DLL
	DEF_STR(LANGUAGE_NAME, "");
	DEF_STR(LANGUAGE_AUTHOR, "");
	DEF_STR(LANGUAGE_VERSION, "");
	DEF_STR(LANGUAGE_DATE, "");

//About DLL Dialog
	DEF_STR(INI_CURRENT_LANG,    "Current Language"        );
	DEF_STR(INI_AUTHOR,          "Author"                  );
	DEF_STR(INI_VERSION,         "Version"                 );
	DEF_STR(INI_DATE,            "Date"                    );
	DEF_STR(INI_HOMEPAGE,        "Visit Home Page"         );
	DEF_STR(INI_CURRENT_RDB,     "ROM Database (.RDB)"     );
	DEF_STR(INI_CURRENT_CHT,     "Cheat Code file (.CHT)"  );
	DEF_STR(INI_CURRENT_RDX,     "Extended Rom Info (.RDX)");

//About INI title
	DEF_STR(INI_TITLE, "About INI Files");

/*********************************************************************************
* Numbers                                                                        *
*********************************************************************************/
	DEF_STR(NUMBER_0,             "0"                      );
	DEF_STR(NUMBER_1,             "1"                      );
	DEF_STR(NUMBER_2,             "2"                      );
	DEF_STR(NUMBER_3,             "3"                      );
	DEF_STR(NUMBER_4,             "4"                      ),
	DEF_STR(NUMBER_5,             "5"                      );
	DEF_STR(NUMBER_6,             "6"                      );
	DEF_STR(NUMBER_7,             "7"                      );
	DEF_STR(NUMBER_8,             "8"                      );
	DEF_STR(NUMBER_9,             "9"                      );
	
/*********************************************************************************
* Menu                                                                           *
*********************************************************************************/
//File Menu
	DEF_STR(MENU_FILE,     "&File"     );
		DEF_STR(MENU_OPEN,       "&Open Rom"               );
		DEF_STR(MENU_ROM_INFO,   "Rom &Info...."           );
		DEF_STR(MENU_START,      "Start Emulation"         );
		DEF_STR(MENU_END,        "&End Emulation"          );
		DEF_STR(MENU_CHOOSE_ROM, "Choose Rom Directory..." );
		DEF_STR(MENU_REFRESH,    "Refresh Rom List"        );
		DEF_STR(MENU_RECENT_ROM, "Recent Rom"              );
		DEF_STR(MENU_RECENT_DIR, "Recent Rom Directories"  );
		DEF_STR(MENU_EXIT,       "E&xit"                   );

//System Menu
	DEF_STR(MENU_SYSTEM,   "&System"   );
		DEF_STR(MENU_RESET,       "&Reset"                 );
		DEF_STR(MENU_PAUSE,       "&Pause"                 );
		DEF_STR(MENU_BITMAP,      "Generate Bitmap"        );
		DEF_STR(MENU_LIMIT_FPS,   "Limit FPS"              );
		DEF_STR(MENU_SAVE,        "&Save"                  );
		DEF_STR(MENU_SAVE_AS,     "Save As..."             );
		DEF_STR(MENU_RESTORE,     "&Restore"               );
		DEF_STR(MENU_LOAD,        "Load..."                );
		DEF_STR(MENU_CURRENT_SAVE,"Current Save S&tate"    );
		DEF_STR(MENU_CHEAT,       "Cheats..."              );
		DEF_STR(MENU_GS_BUTTON,   "GS Button"              );
		DEF_STR(MENU_RESUME, "R&esume");
		DEF_STR(MENU_RESET_SOFT, "&Soft Reset");
		DEF_STR(MENU_RESET_HARD, "&Hard Reset");
		
//Options Menu
	DEF_STR(MENU_OPTIONS,  "&Options"  );
		DEF_STR(MENU_FULL_SCREEN, "&Full Screen"                   );
		DEF_STR(MENU_ON_TOP,      "&Always On &Top"                );
		DEF_STR(MENU_CONFG_GFX,   "Configure Graphics Plugin..."   );
		DEF_STR(MENU_CONFG_AUDIO, "Configure Audio Plugin..."      );
		DEF_STR(MENU_CONFG_CTRL,  "Configure Controller Plugin..." );
		DEF_STR(MENU_CONFG_RSP,   "Configure RSP Plugin..."        );
		DEF_STR(MENU_SHOW_CPU,    "Show CPU usage %"               );
		DEF_STR(MENU_SETTINGS,    "&Settings..."                   );
		
//Debugger Menu
	DEF_STR(MENU_DEBUGGER, "&Debugger" );

//Language Menu
	DEF_STR(MENU_LANGUAGE, "&Language" );

//Help Menu
	DEF_STR(MENU_HELP,     "&Help"     );
		DEF_STR(MENU_ABOUT_INI,   "About &INI Files"   );
		DEF_STR(MENU_ABOUT_PJ64,  "&About Project 64"  );
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

//Pop up Menu
	DEF_STR(POPUP_PLAY, "Play Game");
	DEF_STR(POPUP_INFO, "Rom Information");
	DEF_STR(POPUP_SETTINGS, "Edit Game Settings");
	DEF_STR(POPUP_CHEATS, "Edit Cheats");
	DEF_STR(POPUP_GFX_PLUGIN, "GFX Plugin");

//Alternate Name to save Slot
	DEF_STR(SAVE_SLOT_DEFAULT,"Save Slot - Default"       );
	DEF_STR(SAVE_SLOT_1,      "Save Slot - 1"             );
	DEF_STR(SAVE_SLOT_2,      "Save Slot - 2"             );
	DEF_STR(SAVE_SLOT_3,      "Save Slot - 3"             );
	DEF_STR(SAVE_SLOT_4,      "Save Slot - 4"             );
	DEF_STR(SAVE_SLOT_5,      "Save Slot - 5"             );
	DEF_STR(SAVE_SLOT_6,      "Save Slot - 6"             );
	DEF_STR(SAVE_SLOT_7,      "Save Slot - 7"             );
	DEF_STR(SAVE_SLOT_8,      "Save Slot - 8"             );
	DEF_STR(SAVE_SLOT_9,      "Save Slot - 9"             );
	DEF_STR(SAVE_SLOT_10,     "Save Slot - 10"            );		

/*********************************************************************************
* Rom Browser                                                                    *
*********************************************************************************/
//Rom Browser Fields
	DEF_STR(RB_FILENAME,     "File Name" );
	DEF_STR(RB_INTERNALNAME, "Internal Name" );
	DEF_STR(RB_GOODNAME,     "Good Name" );
	DEF_STR(RB_STATUS,       "Status" );
	DEF_STR(RB_ROMSIZE,      "Rom Size" );
	DEF_STR(RB_NOTES_CORE,   "Notes (Core)" );
	DEF_STR(RB_NOTES_PLUGIN, "Notes (default plugins)" );
	DEF_STR(RB_NOTES_USER,   "Notes (User)" );
	DEF_STR(RB_CART_ID,      "Cartridge ID" );
	DEF_STR(RB_MANUFACTUER,  "Manufacturer" );
	DEF_STR(RB_COUNTRY,      "Country" );
	DEF_STR(RB_DEVELOPER,    "Developer" );
	DEF_STR(RB_CRC1,         "CRC1" );
	DEF_STR(RB_CRC2,         "CRC2" );
	DEF_STR(RB_CICCHIP,      "CIC Chip" );
	DEF_STR(RB_RELEASE_DATE, "Release Date" );
	DEF_STR(RB_GENRE,        "Genre" );
	DEF_STR(RB_PLAYERS,      "Players" );
	DEF_STR(RB_FORCE_FEEDBACK,"Force Feedback" );
	DEF_STR(RB_FILE_FORMAT,   "File Format" );

//Select Rom
	DEF_STR(SELECT_ROM_DIR,  "Select current Rom Directory" );

//Messages
	DEF_STR(RB_NOT_GOOD_FILE,"Bad ROM? Use GoodN64 & check for updated INI" );

/*********************************************************************************
* Options                                                                        *
*********************************************************************************/
//Options Title
	DEF_STR(OPTIONS_TITLE,"Settings");

//Tabs
	DEF_STR(TAB_PLUGIN,      "Plugins");
	DEF_STR(TAB_DIRECTORY,   "Directories");
	DEF_STR(TAB_OPTIONS,     "Options");
	DEF_STR(TAB_ROMSELECTION,"Rom Selection");
	DEF_STR(TAB_ADVANCED,    "Advanced");
	DEF_STR(TAB_ROMSETTINGS, "General Settings");
	DEF_STR(TAB_SHELLINTERGATION, "Shell Integration");
	DEF_STR(TAB_ROMNOTES,    "Notes");
	DEF_STR(TAB_SHORTCUTS,    "Keyboard Shortcuts");
	DEF_STR(TAB_ROMSTATUS,    "Status");
	DEF_STR(TAB_RECOMPILER,   "Recompiler");

//Plugin Dialog
	DEF_STR(PLUG_ABOUT,    "About");
	DEF_STR(PLUG_RSP,      " RSP (reality signal processor) plugin: ");
	DEF_STR(PLUG_GFX,      " Video (graphics) plugin: ");
	DEF_STR(PLUG_AUDIO,    " Audio (sound) plugin: ");
	DEF_STR(PLUG_CTRL,     " Input (controller) plugin: ");
	DEF_STR(PLUG_HLE_GFX,  "Use High Level GFX?");
	DEF_STR(PLUG_HLE_AUDIO,"Use High Level Audio?");
	DEF_STR(PLUG_DEFAULT,  "** Use System Plugin **");

//Directory Dialog
	DEF_STR(DIR_PLUGIN,        " Plugin Directoy: ");
	DEF_STR(DIR_ROM,           " Rom Directory: ");
	DEF_STR(DIR_AUTO_SAVE,     " N64 Auto saves: ");
	DEF_STR(DIR_INSTANT_SAVE,  " Instant saves: ");
	DEF_STR(DIR_SCREEN_SHOT,   " Screen Shots: ");
	DEF_STR(DIR_ROM_DEFAULT,   "Last folder that a rom was open from.");
	DEF_STR(DIR_SELECT_PLUGIN, "Select plugin directory");
	DEF_STR(DIR_SELECT_ROM,    "Select rom directory");
	DEF_STR(DIR_SELECT_AUTO,   "Select automatic save directory");
	DEF_STR(DIR_SELECT_INSTANT,"Select instant save directory");
	DEF_STR(DIR_SELECT_SCREEN, "Select snap shot directory");
	DEF_STR(DIR_TEXTURE,        " Texture Directory: ");
	DEF_STR(DIR_SELECT_TEXTURE, "Select texture pack directory");

//Options (general) Tab
	DEF_STR(OPTION_AUTO_SLEEP,      "Pause emulation when window is not active?");
	DEF_STR(OPTION_AUTO_FULLSCREEN, "On loading a ROM go to full screen");
	DEF_STR(OPTION_BASIC_MODE,      "Hide Advanced Settings");
	DEF_STR(OPTION_REMEMBER_CHEAT,  "Remember selected cheats");
	DEF_STR(OPTION_DISABLE_SS,      "Disable Screen Saver when running rom");
	DEF_STR(OPTION_DISPLAY_FR,      "Display Frame Rate");
	DEF_STR(OPTION_CHANGE_FR,       "Change Frame Rate Display Type");

//Rom Browser Tab
	DEF_STR(RB_MAX_ROMS,         "Max # of Roms Remembered (Max 10):");
	DEF_STR(RB_ROMS,             "roms");
	DEF_STR(RB_MAX_DIRS,         "Max # of Rom Dirs Remembered (Max 10):");
	DEF_STR(RB_DIRS,             "dirs");
	DEF_STR(RB_USE,              "Use Rom Browser");
	DEF_STR(RB_DIR_RECURSION,    "Use Directory recursion");
	DEF_STR(RB_AVALIABLE_FIELDS, "Available fields:");
	DEF_STR(RB_SHOW_FIELDS,      "Show fields in this order:");
	DEF_STR(RB_ADD,              "Add ->");
	DEF_STR(RB_REMOVE,           "<- Remove");
	DEF_STR(RB_UP,               "Up");
	DEF_STR(RB_DOWN,             "Down");
	DEF_STR(RB_REFRESH,          "Automatically refresh browser");

//Advanced Options
	DEF_STR(ADVANCE_INFO,        "Most of these changes will not take effect till a new rom is opened or current rom is reset.");
	DEF_STR(ADVANCE_DEFAULTS,    "Core Defaults");
	DEF_STR(ADVANCE_CPU_STYLE,   "CPU core style:");
	DEF_STR(ADVANCE_SMCM,        "Self-mod code method:");
	DEF_STR(ADVANCE_MEM_SIZE,    "Default Memory Size:");
	DEF_STR(ADVANCE_ABL,         "Advanced Block Linking:");
	DEF_STR(ADVANCE_AUTO_START,  "Start Emulation when rom is opened?");
	DEF_STR(ADVANCE_OVERWRITE,   "Always overwrite default settings with ones from ini?");
	DEF_STR(ADVANCE_COMPRESS,    "Automatically compress instant saves");
	DEF_STR(ADVANCE_DEBUGGER,    "Enable Debugger");
	DEF_STR(ADVANCE_SMM_CACHE,   "Cache");
	DEF_STR(ADVANCE_SMM_PIDMA,   "PI DMA");
	DEF_STR(ADVANCE_SMM_VALIDATE,"Start Changed");
	DEF_STR(ADVANCE_SMM_PROTECT, "Protect Memory");
	DEF_STR(ADVANCE_SMM_TLB,     "TLB Unmapping");

//Rom Options
	DEF_STR(ROM_CPU_STYLE,       "CPU core style:");
	DEF_STR(ROM_MEM_SIZE,        "Memory Size:");
	DEF_STR(ROM_ABL,             "Advanced Block Linking:");
	DEF_STR(ROM_SAVE_TYPE,       "Default Save type:");
	DEF_STR(ROM_COUNTER_FACTOR,  "Counter Factor:");
	DEF_STR(ROM_LARGE_BUFFER,    "Larger Compile Buffer");
	DEF_STR(ROM_USE_TLB,         "Use TLB");
	DEF_STR(ROM_REG_CACHE,       "Register caching");
	DEF_STR(ROM_DELAY_SI,        "Delay SI Interrupt");
	DEF_STR(ROM_SP_HACK,         "SP Hack");
	DEF_STR(ROM_DEFAULT,         "Default");
	DEF_STR(ROM_AUDIO_SIGNAL,    "RSP Audio Signal");
	DEF_STR(ROM_FIXED_AUDIO,     "Fixed Audio Timing");
	DEF_STR(ROM_FUNC_FIND,       "Function lookup method:");
	DEF_STR(ROM_CUSTOM_SMM,      "Custom Self Mod Method");
	DEF_STR(ROM_SYNC_AUDIO,      "Sync using Audio");

//Core Styles
	DEF_STR(CORE_INTERPTER,      "Interpreter");
	DEF_STR(CORE_RECOMPILER,     "Recompiler");
	DEF_STR(CORE_SYNC,           "Synchronise Cores");

//Self Mod Methods
	DEF_STR(SMCM_NONE,           "None");
	DEF_STR(SMCM_CACHE,          "Cache");
	DEF_STR(SMCM_PROECTED,       "Protect Memory");
	DEF_STR(SMCM_CHECK_MEM,      "Check Memory & Cache");
	DEF_STR(SMCM_CHANGE_MEM,     "Change Memory & Cache");
	DEF_STR(SMCM_CHECK_ADV,      "Check Memory Advance");
	DEF_STR(SMCM_CACHE2,         "Clear Code on Cache");

//Function Lookup memthod
	DEF_STR(FLM_PLOOKUP,         "Physical Lookup Table");
	DEF_STR(FLM_VLOOKUP,         "Virtual Lookup Table");
	DEF_STR(FLM_CHANGEMEM,       "Change Memory");

//RDRAM Size
	DEF_STR(RDRAM_4MB,           "4 MB");
	DEF_STR(RDRAM_8MB,           "8 MB");

//Advanced Block Linking
	DEF_STR(ABL_ON,              "On");
	DEF_STR(ABL_OFF,             "Off");

//Save Type
	DEF_STR(SAVE_FIRST_USED,     "Use First Used Save Type");
	DEF_STR(SAVE_4K_EEPROM,      "4kbit Eeprom");
	DEF_STR(SAVE_16K_EEPROM,     "16kbit Eeprom");
	DEF_STR(SAVE_SRAM,           "32kbytes SRAM");
	DEF_STR(SAVE_FLASHRAM,       "Flashram");

//Shell Intergration Tab
	DEF_STR(SHELL_TEXT,          "File extension association:");

//Rom Notes
	DEF_STR(NOTE_STATUS,         "Rom Status:");
	DEF_STR(NOTE_CORE,           "Core Note:");
	DEF_STR(NOTE_PLUGIN,         "Plugin Note:");

// Accelerator Selector
	DEF_STR(ACCEL_CPUSTATE_TITLE,    "CPU State:");
	DEF_STR(ACCEL_MENUITEM_TITLE,    "Menu Item:");
	DEF_STR(ACCEL_CURRENTKEYS_TITLE, "Current Keys:");
	DEF_STR(ACCEL_SELKEY_TITLE,      "Select New Shortcut Key:");
	DEF_STR(ACCEL_ASSIGNEDTO_TITLE,  "Currently Assigned To:");
	DEF_STR(ACCEL_ASSIGN_BTN,        "Assign");
	DEF_STR(ACCEL_REMOVE_BTN,        "Remove");
	DEF_STR(ACCEL_RESETALL_BTN,      "Reset All");
	DEF_STR(ACCEL_CPUSTATE_1,        "Game not playing");
	DEF_STR(ACCEL_CPUSTATE_2,        "Game playing");
	DEF_STR(ACCEL_CPUSTATE_3,        "Game playing (windowed)");
	DEF_STR(ACCEL_CPUSTATE_4,        "Game playing (Fullscreen)");

// Frame Rate Option
	DEF_STR(STR_FR_VIS,              "Vertical Interupts per second");
	DEF_STR(STR_FR_DLS,              "Display Lists per second");
	DEF_STR(STR_FR_PERCENT,          "Percent of Speed");

// Increase speed
	DEF_STR(STR_INSREASE_SPEED,      "Increase Game Speed");
	DEF_STR(STR_DECREASE_SPEED,      "Decrease Game Speed");
	
/*********************************************************************************
* ROM Information                                                                *
*********************************************************************************/
//Rom Info Title Title
	DEF_STR(INFO_TITLE,             "Rom Information");

//Rom Info Text
	DEF_STR(INFO_ROM_NAME_TEXT,     "ROM Name:");
	DEF_STR(INFO_FILE_NAME_TEXT,    "File Name:");
	DEF_STR(INFO_LOCATION_TEXT,     "Location:");
	DEF_STR(INFO_SIZE_TEXT,         "Rom Size:");
	DEF_STR(INFO_CART_ID_TEXT,      "Cartridge ID:");
	DEF_STR(INFO_MANUFACTURER_TEXT, "Manufacturer:");
	DEF_STR(INFO_COUNTRY_TEXT,      "Country:");
	DEF_STR(INFO_CRC1_TEXT,         "CRC1:");
	DEF_STR(INFO_CRC2_TEXT,         "CRC2:");
	DEF_STR(INFO_CIC_CHIP_TEXT,     "CIC Chip:");
	DEF_STR(INFO_MD5_TEXT,          "MD5:");

/*********************************************************************************
* Cheats                                                                         *
*********************************************************************************/
//Cheat List
	DEF_STR(CHEAT_TITLE,           "Cheats");
	DEF_STR(CHEAT_LIST_FRAME,      "Cheats:");
	DEF_STR(CHEAT_NOTES_FRAME,     " Notes: ");
	DEF_STR(CHEAT_MARK_ALL,        "Mark All");
	DEF_STR(CHEAT_MARK_NONE,       "Unmark All");

//Add Cheat
	DEF_STR(CHEAT_ADDCHEAT_FRAME,  "Add Cheat");
	DEF_STR(CHEAT_ADDCHEAT_NAME,   "Name:");
	DEF_STR(CHEAT_ADDCHEAT_CODE,   "Code:");
	DEF_STR(CHEAT_ADDCHEAT_INSERT, "Insert");
	DEF_STR(CHEAT_ADDCHEAT_CLEAR,  "Clear");
	DEF_STR(CHEAT_ADDCHEAT_NOTES,  " Cheat Notes: ");
	DEF_STR(CHEAT_ADD_TO_DB,       "Add to DB");

//Code extension
	DEF_STR(CHEAT_CODE_EXT_TITLE, "Code Extensions");
	DEF_STR(CHEAT_CODE_EXT_TXT, "Please choose a value to be used for:");
	DEF_STR(CHEAT_OK, "OK");
	DEF_STR(CHEAT_CANCEL, "Cancel");

//Digital Value
	DEF_STR(CHEAT_QUANTITY_TITLE,  "Quantity Digit");
	DEF_STR(CHEAT_CHOOSE_VALUE,    "Please choose a value for:");
	DEF_STR(CHEAT_VALUE,           "&Value");
	DEF_STR(CHEAT_FROM,            "from");
	DEF_STR(CHEAT_TO,              "to");
	DEF_STR(CHEAT_NOTES,           "&Notes:");
	DEF_STR(CHEAT_ADDCHEAT_ADD, "Add Cheat");
	DEF_STR(CHEAT_ADDCHEAT_NEW, "New Cheat");
	DEF_STR(CHEAT_ADDCHEAT_CODEDES, "<address> <value>");
	DEF_STR(CHEAT_ADDCHEAT_OPT, "Options:");
	DEF_STR(CHEAT_ADDCHEAT_OPTDES, "<value> <label>");

//Edit Cheat
	DEF_STR(CHEAT_EDITCHEAT_WINDOW,"Edit Cheat");
	DEF_STR(CHEAT_EDITCHEAT_UPDATE,"Update Cheat");
	DEF_STR(CHEAT_CHANGED_MSG,     "Cheat has been changed do you want to update?");
	DEF_STR(CHEAT_CHANGED_TITLE,   "Cheat Updated");

//Cheat Popup Menu
	DEF_STR(CHEAT_ADDNEW,          "Add New Cheat...");
	DEF_STR(CHEAT_EDIT,            "Edit");
	DEF_STR(CHEAT_DELETE,          "Delete");

// short cut editor
	DEF_STR(STR_SHORTCUT_RESET_TITLE, "Reset Short Cuts");
	DEF_STR(STR_SHORTCUT_RESET_TEXT,  "Are you sure you want to reset the short cuts?\n\nThis action cannot be undone.");
	DEF_STR(STR_SHORTCUT_FILEMENU,    "File Menu");
	DEF_STR(STR_SHORTCUT_SYSTEMMENU,  "System Menu");
	DEF_STR(STR_SHORTCUT_OPTIONS,     "Options");
	DEF_STR(STR_SHORTCUT_SAVESLOT,    "Save Slots");

/*********************************************************************************
* Messages                                                                       *
*********************************************************************************/
	DEF_STR(MSG_CPU_PAUSED,         "*** CPU PAUSED ***");
	DEF_STR(MSG_CPU_RESUMED,        "CPU Resumed");
	DEF_STR(MSG_PERM_LOOP,          "In a permanent loop that cannot be exited. \nEmulation will now stop. \n\nVerify ROM and ROM Settings.");
	DEF_STR(MSG_MEM_ALLOC_ERROR,    "Failed to allocate Memory");
	DEF_STR(MSG_FAIL_INIT_GFX,      "The default or selected video plugin is missing or invalid. \n\nYou need to go into Settings and select a video (graphics) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder.");
	DEF_STR(MSG_FAIL_INIT_AUDIO,    "The default or selected audio plugin is missing or invalid. \n\nYou need to go into Settings and select a audio (sound) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder.");
	DEF_STR(MSG_FAIL_INIT_RSP,      "The default or selected RSP plugin is missing or invalid. \n\nYou need to go into Settings and select a RSP (reality signal processor) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder.");
	DEF_STR(MSG_FAIL_INIT_CONTROL,  "The default or selected input plugin is missing or invalid. \n\nYou need to go into Settings and select an input (controller) plugin.\nCheck that you have at least one compatible plugin file in your plugin folder.");
	DEF_STR(MSG_FAIL_LOAD_PLUGIN,   "Failed to load plugin:");
	DEF_STR(MSG_FAIL_LOAD_WORD,     "Failed to load word\n\nVerify ROM and ROM Settings.");
	DEF_STR(MSG_FAIL_OPEN_SAVE,     "Failed to open Save File");
	DEF_STR(MSG_FAIL_OPEN_EEPROM,   "Failed to open Eeprom");
	DEF_STR(MSG_FAIL_OPEN_FLASH,    "Failed to open Flashram");
	DEF_STR(MSG_FAIL_OPEN_MEMPAK,   "Failed to open mempak");
	DEF_STR(MSG_FAIL_OPEN_ZIP,      "Attempt to open zip file failed. \n\nProbably a corrupt zip file - try unzipping ROM manually.");
	DEF_STR(MSG_FAIL_OPEN_IMAGE,    "Attempt to open file failed.");
	DEF_STR(MSG_FAIL_ZIP,           "Error occured when trying to open zip file.");
	DEF_STR(MSG_FAIL_IMAGE,         "File loaded does not appear to be a valid Nintendo64 ROM. \n\nVerify your ROMs with GoodN64.");
	DEF_STR(MSG_UNKNOWN_COUNTRY,    "Unknown country");
	DEF_STR(MSG_UNKNOWN_CIC_CHIP,   "Unknown Cic Chip");
	DEF_STR(MSG_UNKNOWN_FILE_FORMAT,"Unknown file format");
	DEF_STR(MSG_UNKNOWN_MEM_ACTION, "Unknown memory action\n\nEmulation stop");
	DEF_STR(MSG_UNHANDLED_OP,       "Unhandled R4300i OpCode at");
	DEF_STR(MSG_NONMAPPED_SPACE,    "Executing from non-mapped space.\n\nVerify ROM and ROM Settings.");
	DEF_STR(MSG_SAVE_STATE_HEADER,  "State save does not appear to match the running ROM. \n\nState saves must be saved & loaded between 100% identical ROMs, \nin particular the REGION and VERSION need to be the same. \nLoading this state is likely to cause the game and/or emulator to crash. \n\nAre you sure you want to continue loading?");
	DEF_STR(MSG_MSGBOX_TITLE,       "Error");
	DEF_STR(MSG_PIF2_ERROR,         "Copyright sequence not found in LUT.  Game will no longer function.");
	DEF_STR(MSG_PIF2_TITLE,         "Copy Protection Failure");
	DEF_STR(MSG_PLUGIN_CHANGE,      "Changing a plugin requires Project64 to reset a running ROM. \nIf you don't want to lose your place, answer No and make a state save first. \n\nChange plugins and restart game now?");
	DEF_STR(MSG_PLUGIN_CHANGE_TITLE,"Change Plugins");
	DEF_STR(MSG_EMULATION_ENDED,    "Emulation ended");
	DEF_STR(MSG_EMULATION_STARTED,  "Emulation started");
	DEF_STR(MSG_UNABLED_LOAD_STATE, "Unable to load save state");
	DEF_STR(MSG_LOADED_STATE,       "Loaded save state");
	DEF_STR(MSG_SAVED_STATE,        "Saved current state to");
	DEF_STR(MSG_SAVE_SLOT,          "Save state slot");
	DEF_STR(MSG_BYTESWAP,           "Byte swapping image");
	DEF_STR(MSG_CHOOSE_IMAGE,       "Choosing N64 image");
	DEF_STR(MSG_LOADED,             "Loaded");
	DEF_STR(MSG_LOADING,            "Loading image");
	DEF_STR(MSG_PLUGIN_NOT_INIT,    "Cannot open a rom because plugins have not successfully initialised");
	DEF_STR(MSG_DEL_SURE,           "Are you sure you really want to delete this?");
	DEF_STR(MSG_DEL_TITLE,          "Delete Cheat");
	DEF_STR(MSG_CHEAT_NAME_IN_USE,  "Cheat Name is already in use");
	DEF_STR(MSG_MAX_CHEATS,         "You Have reached the Maximum amount of cheats for this rom");
	DEF_STR(MSG_PLUGIN_INIT,		"Plug-in Initializing");
	DEF_STR(MSG_NO_SHORTCUT_SEL,	"You have not selected a virtual key to assign to the menu item");
	DEF_STR(MSG_NO_MENUITEM_SEL,	"You need to select a menu item to assign this key to");
	DEF_STR(MSG_MENUITEM_ASSIGNED,	"Short cut has already been assigned to another menu item");
	DEF_STR(MSG_NO_SEL_SHORTCUT,	"No shortcut has been selected to be removed");
	DEF_STR(MSG_WAITING_FOR_START,	"Rom Loaded. Waiting for emulation to start.");
	DEF_STR(MSG_INVALID_EXE,	    "project64 beta is for members only.\n\nif you have an account at pj64.net, you should not be seeing this error!!\nplease contact us on the site");
	DEF_STR(MSG_INVALID_EXE_TITLE,  "Program Error");
	DEF_STR(MSG_7Z_FILE_NOT_FOUND,  "Failed to find filename in 7z file");
	DEF_STR(MSG_SET_LLE_GFX_TITLE,  "Use Low Level Graphics");
	DEF_STR(MSG_SET_LLE_GFX_MSG,    "Low Level Graphics are not for general use!!!\nIt is advisable that you only use this for testing, not for playing any games with\n\nChange to LLE GFX?");
	DEF_STR(MSG_SET_HLE_AUD_TITLE,  "Use High Level Audio");
	DEF_STR(MSG_SET_HLE_AUD_MSG,    "High level Audio requires a 3rd party plugin!!!\nIf you do not use a 3rd party plugin that supports high level audio then you will hear no sound.\n\nUse high level audio?");
}

LRESULT CALLBACK LangSelectProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

CLanguage::CLanguage() :
	m_emptyString("")
{
	LoadDefaultStrings();
}

void CLanguage::LoadCurrentStrings ( bool ShowSelectDialog ) 
{
	if (ShowSelectDialog)
	{
		m_SelectedLanguage = g_Settings->LoadString(Setting_CurrentLanguage);
	}
	
	LanguageList LangList = GetLangList();
	stdstr       Filename;

	//clear all the current strings loaded
	m_CurrentStrings.clear();

	//Find the file name of the current language
	for (LanguageList::iterator Language = LangList.begin(); Language != LangList.end(); Language++) {
		if (_Lang->IsCurrentLang(*Language))
		{
			Filename = Language->Filename;
			break;
		}
	}
	if (ShowSelectDialog && Filename.length() == 0 && LangList.size() > 0) 
	{ 	
		//Do Dialog box to choose language
		DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_Lang_Select),NULL,(DLGPROC)LangSelectProc, (LPARAM)this);
		return; 
	}
	
	if (Filename.length() == 0)
	{
		return;
	}

	//Process the file
	FILE *file = fopen(Filename.c_str(), "rb");
	if (file == NULL) { return; }

	//String;
	while(!feof(file))
	{
		m_CurrentStrings.insert(GetNextLangString(file));
	}		
	fclose(file);
}

WNDPROC pfnWndLangSelectOkProc = NULL;
HBITMAP hOkButton = NULL;

DWORD CALLBACK LangSelectOkProc (HWND hWnd, DWORD uMsg, DWORD wParam, DWORD lParam) 
{		
	static bool m_fPressed = false;
	static HBITMAP hOkButtonDown = NULL;
	
	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			if (BeginPaint(hWnd,&ps))
			{
				if (m_fPressed)
				{
					if (hOkButtonDown == NULL)
					{
						hOkButtonDown = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_LANG_OK_DOWN)); 
					}
					if (hOkButtonDown)
					{
						RECT rcClient;
						GetClientRect(hWnd, &rcClient);

						BITMAP bmTL1;
						GetObject(hOkButtonDown, sizeof(BITMAP), &bmTL1);
						HDC     memdc	= CreateCompatibleDC(ps.hdc);
						HGDIOBJ save	= SelectObject(memdc, hOkButtonDown);
						BitBlt(ps.hdc, 0, 0, bmTL1.bmWidth, bmTL1.bmHeight, memdc, 0, 0, SRCCOPY);
						SelectObject(memdc, save);
						DeleteDC(memdc);
					}
				} else {
					if (hOkButton)
					{
						RECT rcClient;
						GetClientRect(hWnd, &rcClient);

						BITMAP bmTL1;
						GetObject(hOkButton, sizeof(BITMAP), &bmTL1);
						HDC     memdc	= CreateCompatibleDC(ps.hdc);
						HGDIOBJ save	= SelectObject(memdc, hOkButton);
						BitBlt(ps.hdc, 0, 0, bmTL1.bmWidth, bmTL1.bmHeight, memdc, 0, 0, SRCCOPY);
						SelectObject(memdc, save);
						DeleteDC(memdc);
					}
				}
				EndPaint(hWnd,&ps);
			}
		}
		break;
	case WM_MOUSEMOVE:
		if(::GetCapture() == hWnd) 
		{
			POINT ptCursor = { ((int)(short)LOWORD(lParam)), ((int)(short)HIWORD(lParam)) };
			ClientToScreen(hWnd, &ptCursor);
			RECT rect;
			GetWindowRect(hWnd, &rect);
			bool uPressed = ::PtInRect(&rect, ptCursor)==TRUE;
			if( m_fPressed != uPressed ) 
			{
				m_fPressed = uPressed;
				::InvalidateRect(hWnd, NULL, TRUE);
				UpdateWindow(hWnd);
			}
		}
		break;
	case WM_LBUTTONDOWN:
		{
			LRESULT lRet = 0;
			lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
			SetCapture(hWnd);
			if( ::GetCapture()==hWnd ) 
			{
				m_fPressed = true;

				if (m_fPressed)
				{
					::InvalidateRect(hWnd, NULL, TRUE);
					UpdateWindow(hWnd);
				}
			}
			return lRet;
		}
		break;
	case WM_LBUTTONUP:
		{
			LRESULT lRet = 0;
			lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
			if(::GetCapture() == hWnd ) 
			{
				::ReleaseCapture();
				if( m_fPressed )   
				{
					::SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), BN_CLICKED), (LPARAM)hWnd);
				}
			}
			m_fPressed = false;

			return lRet;
		}
		break;
	}
			
	return CallWindowProc(pfnWndLangSelectOkProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK LangSelectProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HBITMAP hbmpBackgroundTop = NULL;
	static HBITMAP hbmpBackgroundBottom = NULL;
	static HBITMAP hbmpBackgroundMiddle = NULL;
	static HFONT   hTextFont = NULL;
	static CLanguage * lngClass;

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowPos(hDlg,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE);
		{
			lngClass = (CLanguage *)lParam;
			
			LanguageList LangList = lngClass->GetLangList();
			if (LangList.size() == 0) { EndDialog(hDlg,0); }
			for (LanguageList::iterator Language = LangList.begin(); Language != LangList.end(); Language++) 
			{
				int index = SendMessage(GetDlgItem(hDlg,IDC_LANG_SEL),CB_ADDSTRING,0,(WPARAM)Language->LanguageName.c_str());
				if (_stricmp(Language->LanguageName.c_str(),"English") == 0) {
					SendMessage(GetDlgItem(hDlg,IDC_LANG_SEL),CB_SETCURSEL,index,0);
				}
			}

			
			int Index = SendMessage(GetDlgItem(hDlg,IDC_LANG_SEL),CB_GETCURSEL,0,0);
			if (Index < 0) { SendMessage(GetDlgItem(hDlg,IDC_LANG_SEL),CB_SETCURSEL,0,0); }

		
			enum { ROUND_EDGE = 15 };

			DWORD dwStyle = GetWindowLong(hDlg, GWL_STYLE);
			dwStyle &= ~(WS_CAPTION|WS_SIZEBOX);
			SetWindowLong(hDlg, GWL_STYLE, dwStyle);

			// Use the size of the image
			hbmpBackgroundTop    = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT_TOP)); 
			hbmpBackgroundBottom = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT_BOTTOM)); 
			hbmpBackgroundMiddle = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT_MIDDLE)); 
			BITMAP bmTL;
			GetObject(hbmpBackgroundTop, sizeof(BITMAP), &bmTL);


			if (hbmpBackgroundTop)
			{
//				int iHeight = bmTL.bmHeight;
				int iWidth  = bmTL.bmWidth;

				RECT rect;
				GetWindowRect(hDlg, &rect);
				rect.left -= rect.left;
				rect.bottom -= rect.top;
				rect.top -= rect.top;

				// Tweaked
				HRGN hWindowRegion= CreateRoundRectRgn
				(
					rect.left,
					rect.top,
					rect.left+iWidth+GetSystemMetrics(SM_CXEDGE)-1,
					rect.bottom+GetSystemMetrics(SM_CYEDGE)-1,
					ROUND_EDGE,
					ROUND_EDGE
				);

				if (hWindowRegion)
				{
					SetWindowRgn(hDlg, hWindowRegion, TRUE);
					DeleteObject(hWindowRegion);
				}
			}
			hTextFont = ::CreateFont
			(
				18, 
				0,
				0, 
				0, 
				FW_NORMAL,
				0,
				0,
				0,
				DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				PROOF_QUALITY,
				DEFAULT_PITCH|FF_DONTCARE,
				_T("Arial")
			);
			SendDlgItemMessage(hDlg,IDC_SELECT_LANG,WM_SETFONT,(WPARAM)hTextFont,TRUE);
		}
		
		hOkButton                = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_LANG_OK)); 
		pfnWndLangSelectOkProc   = (WNDPROC)::GetWindowLongPtr(GetDlgItem(hDlg,IDOK), GWLP_WNDPROC);
		::SetWindowLongPtr(GetDlgItem(hDlg,IDOK), GWLP_WNDPROC,(LONG_PTR)LangSelectOkProc);
		break;
	case WM_NCHITTEST:
		{
			int xPos = LOWORD(lParam); 
			int yPos = HIWORD(lParam); 
			RECT client, a;
			GetClientRect(hDlg,&a);
			GetClientRect(hDlg,&client);
			ClientToScreen(hDlg,(LPPOINT)&client);
			client.right += client.left;
			client.bottom += client.top;


			int nCaption = GetSystemMetrics(SM_CYCAPTION)*4;

			LRESULT lResult = HTCLIENT;

			//check caption
			if (xPos <= client.right && xPos >= client.left && 
				(yPos >= client.top+ 0)&& (yPos <= client.top + 0+nCaption))
			{
				lResult = HTCAPTION;
			}
			SetWindowLong(hDlg, DWL_MSGRESULT, lResult);

			return TRUE;

		}
		break;
	case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			if (BeginPaint(hDlg,&ps))
			{
				RECT rcClient;
				GetClientRect(hDlg, &rcClient);

				BITMAP bmTL_top, bmTL_bottom, bmTL_Middle;
				GetObject(hbmpBackgroundTop, sizeof(BITMAP), &bmTL_top);
				GetObject(hbmpBackgroundBottom, sizeof(BITMAP), &bmTL_bottom);
				GetObject(hbmpBackgroundMiddle, sizeof(BITMAP), &bmTL_Middle);

				HDC     memdc	= CreateCompatibleDC(ps.hdc);
				HGDIOBJ save	= SelectObject(memdc, hbmpBackgroundTop);
				BitBlt(ps.hdc, 0, 0, bmTL_top.bmWidth, bmTL_top.bmHeight, memdc, 0, 0, SRCCOPY);
				SelectObject(memdc, save);
				DeleteDC(memdc);

				
				memdc	= CreateCompatibleDC(ps.hdc);
				save	= SelectObject(memdc, hbmpBackgroundMiddle);
				for (int x = bmTL_top.bmHeight; x < rcClient.bottom; x += bmTL_Middle.bmHeight)
				{
					//BitBlt(ps.hdc, 0, bmTL_top.bmHeight, bmTL_Middle.bmWidth, rcClient.bottom - (bmTL_bottom.bmHeight + bmTL_top.bmHeight), memdc, 0, 0, SRCCOPY);
					BitBlt(ps.hdc, 0, x, bmTL_Middle.bmWidth, bmTL_Middle.bmHeight, memdc, 0, 0, SRCCOPY);
				}
				SelectObject(memdc, save);
				DeleteDC(memdc);

				BITMAP ;
				memdc	= CreateCompatibleDC(ps.hdc);
				save	= SelectObject(memdc, hbmpBackgroundBottom);
				BitBlt(ps.hdc, 0, rcClient.bottom - bmTL_bottom.bmHeight, bmTL_bottom.bmWidth, bmTL_bottom.bmHeight, memdc, 0, 0, SRCCOPY);
				SelectObject(memdc, save);
				DeleteDC(memdc);

				BITMAP ;

				EndPaint(hDlg,&ps);
			}
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (hbmpBackgroundTop)
			{
				DeleteObject(hbmpBackgroundTop);
			}
			if (hbmpBackgroundBottom)
			{
				DeleteObject(hbmpBackgroundBottom);
			}
			if (hbmpBackgroundMiddle)
			{
				DeleteObject(hbmpBackgroundMiddle);
			}

			if (hTextFont)
				::DeleteObject(hTextFont);

			{
				int Index = SendMessage(GetDlgItem(hDlg,IDC_LANG_SEL),CB_GETCURSEL,0,0);
				
				if (Index >= 0) { 
					char String[255];
					SendMessage(GetDlgItem(hDlg,IDC_LANG_SEL),CB_GETLBTEXT,Index,(LPARAM)String);
					lngClass->SetLanguage(String);
				}
			}

			EndDialog(hDlg,0);
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}


LanguageList & CLanguage::GetLangList (void) 
{
	if (m_LanguageList.size() > 0)
	{
		return m_LanguageList;
	}

	CPath LanguageFiles(g_Settings->LoadString(Setting_LanguageDir),"*.pj.Lang");
	if (LanguageFiles.FindFirst())
	{
		do {
			LanguageFile File; //We temporally store the values in here to added to the list

			File.LanguageName = GetLangString(LanguageFiles,LANGUAGE_NAME);
			File.Filename     = LanguageFiles;

			//get the name of the language from inside the file
			m_LanguageList.push_back(File);
		} while (LanguageFiles.FindNext());
	}
	return m_LanguageList;
}

const stdstr &CLanguage::GetString (LanguageStringID StringID)
{	
	LANG_STRINGS::iterator CurrentString = m_CurrentStrings.find(StringID);
	if (CurrentString != m_CurrentStrings.end()) {
		return CurrentString->second;
	}

	LANG_STRINGS::iterator DefString = m_DefaultStrings.find(StringID);
	if (DefString != m_DefaultStrings.end()) {
		return DefString->second;
	}
#ifdef _DEBUG
	_asm int 3
#endif	
	return m_emptyString;
}

stdstr CLanguage::GetLangString ( const char * FileName, LanguageStringID ID ) 
{
	FILE *file = fopen(FileName, "rb");
	if (file == NULL) { return stdstr(""); }

	//String;
	while(!feof(file))
	{
		LANG_STR String = GetNextLangString(file);
		if (String.first == ID) {
			fclose(file);
			return String.second;
		}
	}		
	fclose(file);
	return stdstr("");
}	

LANG_STR CLanguage::GetNextLangString (void * OpenFile) 
{
	enum { MAX_STRING_LEN = 400 }; 
	int  StringID;
	char szString[MAX_STRING_LEN];  //temp store the string from the file

	FILE * file = (FILE *)OpenFile;
	char token=0;

	//Search for token #
	while(token!='#' && !feof(file)) { fread(&token, 1, 1, file); }
	if(feof(file)){ return LANG_STR(0,""); } 
		
	//get StringID after token
	fscanf(file, "%d", &StringID);
	
	//Search for token #
	while(token!='#' && !feof(file)) { fread(&token, 1, 1, file); }
	if(feof(file)){ StringID = EMPTY_STRING; return LANG_STR(0,""); } 

		//Search for start of string '"'
	while(token!='"' && !feof(file)) { fread(&token, 1, 1, file); }
	if(feof(file)){ StringID = EMPTY_STRING; return LANG_STR(0,""); } 		

	int pos = 0;
	fread(&token, 1, 1, file); 
	while(token!='"' && !feof(file)){ 
		szString[pos++] = token;
		fread(&token, 1, 1, file); 
		if (pos == MAX_STRING_LEN - 2) { token = '"'; }
	}
	szString[pos++] = 0;
	return LANG_STR(StringID,szString);
}

void CLanguage::SetLanguage ( char * LanguageName ) 
{
	m_SelectedLanguage = LanguageName;
	LoadCurrentStrings(false);
	g_Settings->SaveString(Setting_CurrentLanguage,LanguageName);
}

bool CLanguage::IsCurrentLang( LanguageFile & File )
{
	if (m_SelectedLanguage == File.LanguageName)
	{
		return true;
	}
	return false;
}
