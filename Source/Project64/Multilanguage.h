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
#pragma once

enum LanguageStringID{
	EMPTY_STRING           = 0,

/*********************************************************************************
* Meta Information                                                               *
*********************************************************************************/
//About DLL
	LANGUAGE_NAME	       = 1,
	LANGUAGE_AUTHOR	        =2,
	LANGUAGE_VERSION        =3,
	LANGUAGE_DATE	        =4,

//About DLL Dialog
	INI_CURRENT_LANG        =5,
	INI_AUTHOR		        =6,
	INI_VERSION		        =7,
	INI_DATE		        =8,
	INI_HOMEPAGE	        =9,
	INI_CURRENT_RDB         =10,
	INI_CURRENT_CHT         =11,
	INI_CURRENT_RDX         =12,

//About INI title
	INI_TITLE               =20,

/*********************************************************************************
* Numbers                                                                        *
*********************************************************************************/
	NUMBER_0		        =50,
	NUMBER_1		      =  51,
	NUMBER_2		      =  52,
	NUMBER_3		      =  53,
	NUMBER_4		      =  54,
	NUMBER_5		      =  55,
	NUMBER_6		      =  56,
	NUMBER_7		      =  57,
	NUMBER_8		      =  58,
	NUMBER_9		      =  59,

/*********************************************************************************
* Menu                                                                           *
*********************************************************************************/
//File Menu
	MENU_FILE				=100,
		MENU_OPEN			=101,
		MENU_ROM_INFO		=102,
		MENU_START			=103,
		MENU_END			=104,
		MENU_CHOOSE_ROM		=105,
		MENU_REFRESH		=106,
		MENU_RECENT_ROM		=107,
		MENU_RECENT_DIR		=108,
		MENU_EXIT			=109,

//System Menu
	MENU_SYSTEM				=120,
		MENU_RESET			=121,
		MENU_PAUSE			=122,
		MENU_BITMAP			=123,
		MENU_LIMIT_FPS		=124,
		MENU_SAVE			=125,
		MENU_SAVE_AS		=126,
		MENU_RESTORE		=127,
		MENU_LOAD			=128,
		MENU_CURRENT_SAVE	=129,
		MENU_CHEAT			=130,
		MENU_GS_BUTTON		=131,
		MENU_RESUME			=132,
		MENU_RESET_SOFT		=133, //added in build 1.7.50
		MENU_RESET_HARD		=134, //added in build 1.7.50

//Options Menu
	MENU_OPTIONS		=	140,
		MENU_FULL_SCREEN=	141,
		MENU_ON_TOP		=	142,
		MENU_CONFG_GFX	=	143,
		MENU_CONFG_AUDIO=	144,
		MENU_CONFG_CTRL	=	145,
		MENU_CONFG_RSP	=	146,
		MENU_SHOW_CPU	=	147,
		MENU_SETTINGS	=	148,

//Debugger Menu
	MENU_DEBUGGER		=	160,

//Language Menu
	MENU_LANGUAGE		=	175,

//Help Menu
	MENU_HELP			=	180,
		MENU_ABOUT_INI	=	181,
		MENU_ABOUT_PJ64	=	182,
		MENU_FORUM   	=	183,
		MENU_HOMEPAGE	=	184,

//Current Save Slot menu
	MENU_SLOT_DEFAULT	=	190,
	MENU_SLOT_1			=	191,
	MENU_SLOT_2			=	192,
	MENU_SLOT_3			=	193,
	MENU_SLOT_4			=	194,
	MENU_SLOT_5			=	195,
	MENU_SLOT_6			=	196,
	MENU_SLOT_7			=	197,
	MENU_SLOT_8			=	198,
	MENU_SLOT_9			=	199,
	MENU_SLOT_10		=	200,

//Pop up Menu
	POPUP_PLAY			=	210,
	POPUP_INFO			=	211,
	POPUP_SETTINGS		=	212,
	POPUP_CHEATS		=	213,
	POPUP_GFX_PLUGIN	=	214,

//selecting save slot
	SAVE_SLOT_DEFAULT	=	220,
	SAVE_SLOT_1			=	221,
	SAVE_SLOT_2			=	222,
	SAVE_SLOT_3			=	223,
	SAVE_SLOT_4			=	224,
	SAVE_SLOT_5			=	225,
	SAVE_SLOT_6			=	226,
	SAVE_SLOT_7			=	227,
	SAVE_SLOT_8			=	228,
	SAVE_SLOT_9			=	229,
	SAVE_SLOT_10		=	230,

// Menu Descriptions (TODO: unused ? implement or remove)
	MENUDES_OPEN		=	250,
	MENUDES_ROM_INFO	=	251,
	MENUDES_START		=	252,
	MENUDES_END			=	253,
	MENUDES_CHOOSE_ROM	=	254,
	MENUDES_REFRESH		=	255,
	MENUDES_EXIT		=	256,
	MENUDES_RESET		=	257,
	MENUDES_PAUSE		=	258,
	MENUDES_BITMAP		=	259,
	MENUDES_LIMIT_FPS	=	260,
	MENUDES_SAVE		=	261,
	MENUDES_SAVE_AS		=	262,
	MENUDES_RESTORE		=	263,
	MENUDES_LOAD		=	264,
	MENUDES_CHEAT		=	265,
	MENUDES_GS_BUTTON	=	266,
	MENUDES_FULL_SCREEN	=	267,
	MENUDES_ON_TOP		=	268,
	MENUDES_CONFG_GFX	=	269,
	MENUDES_CONFG_AUDIO	=	270,
	MENUDES_CONFG_CTRL	=	271,
	MENUDES_CONFG_RSP	=	272,
	MENUDES_SHOW_CPU	=	273,
	MENUDES_SETTINGS	=	274,
	MENUDES_USER_MAN	=	275,
	MENUDES_GAME_FAQ	=	276,
	MENUDES_ABOUT_INI	=	277,
	MENUDES_ABOUT_PJ64	=	278,
	MENUDES_RECENT_ROM	=	279,
	MENUDES_RECENT_DIR	=	280,
	MENUDES_LANGUAGES	=	281,
	MENUDES_GAME_SLOT	=	282,
	MENUDES_PLAY_GAME	=	283,
	MENUDES_GAME_INFO	=	284,
	MENUDES_GAME_SETTINGS=	285,
	MENUDES_GAME_CHEATS	=	286,

/*********************************************************************************
* Rom Browser                                                                    *
*********************************************************************************/
//Rom Browser Fields
	RB_FILENAME			=	300,
	RB_INTERNALNAME		=	301,
	RB_GOODNAME			=	302,
	RB_STATUS			=	303,
	RB_ROMSIZE			=	304,
	RB_NOTES_CORE		=	305,
	RB_NOTES_PLUGIN		=	306,
	RB_NOTES_USER		=	307,
	RB_CART_ID			=	308,
	RB_MANUFACTUER		=	309,
	RB_COUNTRY			=	310,
	RB_DEVELOPER		=	311,
	RB_CRC1				=	312,
	RB_CRC2				=	313,
	RB_CICCHIP			=	314,
	RB_RELEASE_DATE		=	315,
	RB_GENRE			=	316,
	RB_PLAYERS			=	317,
	RB_FORCE_FEEDBACK	=	318,
	RB_FILE_FORMAT	    =	319,

//Select Rom
	SELECT_ROM_DIR		=	320,

//Messages
	RB_NOT_GOOD_FILE	=	340,

/*********************************************************************************
* Options                                                                        *
*********************************************************************************/
//Options Title
	OPTIONS_TITLE		=	400,

//Tabs
	TAB_PLUGIN			=	401,
	TAB_DIRECTORY		=	402,
	TAB_OPTIONS			=	403,
	TAB_ROMSELECTION	=	404,
	TAB_ADVANCED		=	405,
	TAB_ROMSETTINGS		=	406,
	TAB_SHELLINTERGATION=	407,
	TAB_ROMNOTES		=	408,
	TAB_SHORTCUTS		=	409,
	TAB_ROMSTATUS		=	410,
	TAB_RECOMPILER		=	411, //Added in 1.7.0.50

//Plugin Dialog
	PLUG_ABOUT			=	420,
	PLUG_RSP			=	421,
	PLUG_GFX			=	422,
	PLUG_AUDIO			=	423,
	PLUG_CTRL			=	424,
	PLUG_HLE_GFX		=	425,
	PLUG_HLE_AUDIO		=	426,
	PLUG_DEFAULT		=	427,

//Directory Dialog
	DIR_PLUGIN			=	440,
	DIR_ROM				=	441,
	DIR_AUTO_SAVE		=	442,
	DIR_INSTANT_SAVE	=	443,
	DIR_SCREEN_SHOT		=	444,
	DIR_ROM_DEFAULT		=	445,
	DIR_SELECT_PLUGIN	=	446,
	DIR_SELECT_ROM		=	447,
	DIR_SELECT_AUTO		=	448,
	DIR_SELECT_INSTANT	=	449,
	DIR_SELECT_SCREEN	=	450,
	DIR_TEXTURE			=	451,
	DIR_SELECT_TEXTURE	=	452,

//Options (general) Tab
	OPTION_AUTO_SLEEP		=460,
	OPTION_AUTO_FULLSCREEN	=461,
	OPTION_BASIC_MODE		=462,
	OPTION_REMEMBER_CHEAT	=463,
	OPTION_DISABLE_SS		=464,
	OPTION_DISPLAY_FR		=465,
	OPTION_CHANGE_FR		=466,

//Rom Browser Tab
	RB_MAX_ROMS			=	480,
	RB_ROMS				=	481,
	RB_MAX_DIRS			=	482,
	RB_DIRS				=	483,
	RB_USE				=	484,
	RB_DIR_RECURSION	=	485,
	RB_AVALIABLE_FIELDS	=	486,
	RB_SHOW_FIELDS		=	487,
	RB_ADD				=	488,
	RB_REMOVE			=	489,
	RB_UP				=	490,
	RB_DOWN				=	491,
	RB_REFRESH			=	492,

//Advanced Options
	ADVANCE_INFO		=	500,
	ADVANCE_DEFAULTS	=	501,
	ADVANCE_CPU_STYLE	=	502,
	ADVANCE_SMCM		=	503,
	ADVANCE_MEM_SIZE	=	504,
	ADVANCE_ABL			=	505,
	ADVANCE_AUTO_START	=	506,
	ADVANCE_OVERWRITE	=	507,
	ADVANCE_COMPRESS	=	508,
	ADVANCE_DEBUGGER	=	509,
	ADVANCE_SMM_CACHE	=	510,
	ADVANCE_SMM_PIDMA	=	511,
	ADVANCE_SMM_VALIDATE=	512,
	ADVANCE_SMM_PROTECT	=	513,
	ADVANCE_SMM_TLB	    =	514,

//Rom Options
	ROM_CPU_STYLE		=	520,
	ROM_MEM_SIZE		=	522,
	ROM_ABL				=	523,
	ROM_SAVE_TYPE		=	524,
	ROM_COUNTER_FACTOR	=	525,
	ROM_LARGE_BUFFER	=	526,
	ROM_USE_TLB			=	527,
	ROM_REG_CACHE		=	528,
	ROM_DELAY_SI		=	529,
	ROM_SP_HACK			=	530,
	ROM_DEFAULT			=	531,
	ROM_AUDIO_SIGNAL    =   532,
	ROM_FIXED_AUDIO     =   533,
	ROM_FUNC_FIND       =   534,
	ROM_CUSTOM_SMM      =   535,
	ROM_SYNC_AUDIO      =   536,

//Core Styles
	CORE_INTERPTER		=	540,
	CORE_RECOMPILER		=	541,
	CORE_SYNC			=	542,

//Self Mod Methods
	SMCM_NONE		=		560,
	SMCM_CACHE		=		561,
	SMCM_PROECTED	=		562,
	SMCM_CHECK_MEM	=		563,
	SMCM_CHANGE_MEM	=		564,
	SMCM_CHECK_ADV	=		565,
	SMCM_CACHE2     =       566,

//Function Lookup memthod
	FLM_PLOOKUP		=		570,
	FLM_VLOOKUP		=		571,
	FLM_CHANGEMEM	=		572,

//RDRAM Size
	RDRAM_4MB			=	580,
	RDRAM_8MB			=	581,

//Advanced Block Linking
	ABL_ON			=		600,
	ABL_OFF			=		601,

//Save Type
	SAVE_FIRST_USED	=		620,
	SAVE_4K_EEPROM	=		621,
	SAVE_16K_EEPROM	=		622,
	SAVE_SRAM		=		623,
	SAVE_FLASHRAM	=		624,

//Shell Integration Tab
	SHELL_TEXT			=	640,

//Rom Notes
	NOTE_STATUS			=	660,
	NOTE_CORE			=	661,
	NOTE_PLUGIN			=	662,

// Accelerator Selector
    ACCEL_CPUSTATE_TITLE    =  680,
    ACCEL_MENUITEM_TITLE    =  681,
    ACCEL_CURRENTKEYS_TITLE =  682,
    ACCEL_SELKEY_TITLE      =  683,
    ACCEL_ASSIGNEDTO_TITLE  =  684,
    ACCEL_ASSIGN_BTN        =  685,
    ACCEL_REMOVE_BTN        =  686,
    ACCEL_RESETALL_BTN      =  687,
    ACCEL_CPUSTATE_1        =  688,
    ACCEL_CPUSTATE_2        =  689,
    ACCEL_CPUSTATE_3        =  690,
    ACCEL_CPUSTATE_4        =  691,

// Frame Rate Option
    STR_FR_VIS              = 700,
    STR_FR_DLS              = 701,
    STR_FR_PERCENT          = 702,

// Increase speed
    STR_INSREASE_SPEED      = 710,
    STR_DECREASE_SPEED      = 711,

/*********************************************************************************
* ROM Information                                                                *
*********************************************************************************/
//Rom Info Title Title
	INFO_TITLE			=	800,

//Rom Info Text
	INFO_ROM_NAME_TEXT	=	801,
	INFO_FILE_NAME_TEXT	=	802,
	INFO_LOCATION_TEXT	=	803,
	INFO_SIZE_TEXT		=	804,
	INFO_CART_ID_TEXT	=	805,
	INFO_MANUFACTURER_TEXT=	806,
	INFO_COUNTRY_TEXT	=	807,
	INFO_CRC1_TEXT		=	808,
	INFO_CRC2_TEXT		=	809,
	INFO_CIC_CHIP_TEXT	=	810,
	INFO_MD5_TEXT    	=	811,

/*********************************************************************************
* Cheats                                                                         *
*********************************************************************************/
//Cheat List
	CHEAT_TITLE			=	1000,
	CHEAT_LIST_FRAME	=	1001,
	CHEAT_NOTES_FRAME	=	1002,
	CHEAT_MARK_ALL		=	1003,
	CHEAT_MARK_NONE		=	1004,

//Add Cheat
	CHEAT_ADDCHEAT_FRAME	=1005,
	CHEAT_ADDCHEAT_NAME		=1006,
	CHEAT_ADDCHEAT_CODE	=	1007,
	CHEAT_ADDCHEAT_INSERT=	1008,
	CHEAT_ADDCHEAT_CLEAR=	1009,
	CHEAT_ADDCHEAT_NOTES=	1010,
	CHEAT_ADD_TO_DB 	=	1011,

//Code extension
	CHEAT_CODE_EXT_TITLE	=1012,
	CHEAT_CODE_EXT_TXT		=1013,
	CHEAT_OK				=1014,
	CHEAT_CANCEL			=1015,

//Digital Value
	CHEAT_QUANTITY_TITLE	=1016,
	CHEAT_CHOOSE_VALUE		=1017,
	CHEAT_VALUE				=1018,
	CHEAT_FROM				=1019,
	CHEAT_TO				=1020,
	CHEAT_NOTES				=1021,
	CHEAT_ADDCHEAT_ADD 	=	1022,
	CHEAT_ADDCHEAT_NEW 	=	1023,
	CHEAT_ADDCHEAT_CODEDES 	=1024,
	CHEAT_ADDCHEAT_OPT 		=1025,
	CHEAT_ADDCHEAT_OPTDES 	=1026,

//Edit Cheat
	CHEAT_EDITCHEAT_WINDOW	=1027,
	CHEAT_EDITCHEAT_UPDATE	=1028,
	CHEAT_CHANGED_MSG   	=1029,
	CHEAT_CHANGED_TITLE   	=1030,

//Cheat Popup Menu
	CHEAT_ADDNEW			=1040,
	CHEAT_EDIT			=	1041,
	CHEAT_DELETE		=	1042,

// short cut editor
    STR_SHORTCUT_RESET_TITLE  = 1100,
    STR_SHORTCUT_RESET_TEXT   = 1101,
    STR_SHORTCUT_FILEMENU     = 1102,
    STR_SHORTCUT_SYSTEMMENU   = 1103,
    STR_SHORTCUT_OPTIONS      = 1104,
    STR_SHORTCUT_SAVESLOT     = 1105,

/*********************************************************************************
* Messages                                                                       *
*********************************************************************************/
	MSG_CPU_PAUSED		=	2000,
	MSG_CPU_RESUMED		=	2001,
	MSG_PERM_LOOP        =   2002,
	MSG_MEM_ALLOC_ERROR   =  2003,
	MSG_FAIL_INIT_GFX      = 2004,
	MSG_FAIL_INIT_AUDIO    = 2005,
	MSG_FAIL_INIT_RSP      = 2006,
	MSG_FAIL_INIT_CONTROL  = 2007,
	MSG_FAIL_LOAD_PLUGIN   = 2008,
	MSG_FAIL_LOAD_WORD     = 2009,
	MSG_FAIL_OPEN_SAVE     = 2010,
	MSG_FAIL_OPEN_EEPROM   = 2011,
	MSG_FAIL_OPEN_FLASH    = 2012,
	MSG_FAIL_OPEN_MEMPAK   = 2013,
	MSG_FAIL_OPEN_ZIP      = 2014,
	MSG_FAIL_OPEN_IMAGE    = 2015,
	MSG_FAIL_ZIP           = 2016,
	MSG_FAIL_IMAGE         = 2017,
	MSG_UNKNOWN_COUNTRY    = 2018,
	MSG_UNKNOWN_CIC_CHIP   = 2019,
	MSG_UNKNOWN_FILE_FORMAT= 2020,
	MSG_UNKNOWN_MEM_ACTION = 2021,
	MSG_UNHANDLED_OP       = 2022,
	MSG_NONMAPPED_SPACE    = 2023,
	MSG_SAVE_STATE_HEADER  = 2024,
	MSG_MSGBOX_TITLE        =2025,
	MSG_PIF2_ERROR         = 2026,
	MSG_PIF2_TITLE         = 2027,
	MSG_PLUGIN_CHANGE      = 2028,
	MSG_PLUGIN_CHANGE_TITLE= 2029,
	MSG_EMULATION_ENDED    = 2030,
	MSG_EMULATION_STARTED  = 2031,
	MSG_UNABLED_LOAD_STATE = 2032,
	MSG_LOADED_STATE       = 2033,
	MSG_SAVED_STATE        = 2034,
	MSG_SAVE_SLOT          = 2035,
	MSG_BYTESWAP            =2036,
	MSG_CHOOSE_IMAGE       = 2037,
	MSG_LOADED             = 2038,
	MSG_LOADING            = 2039,
	MSG_PLUGIN_NOT_INIT    = 2040,
	MSG_DEL_SURE           = 2041,
	MSG_DEL_TITLE          = 2042,
	MSG_CHEAT_NAME_IN_USE  = 2043,
	MSG_MAX_CHEATS         = 2044,
	MSG_PLUGIN_INIT        = 2045,		//Added in pj64 1.6
	MSG_NO_SHORTCUT_SEL    = 2046,		//Added in pj64 1.6
	MSG_NO_MENUITEM_SEL    = 2047,		//Added in pj64 1.6
	MSG_MENUITEM_ASSIGNED  = 2048,		//Added in pj64 1.6
	MSG_NO_SEL_SHORTCUT    = 2049,		//Added in pj64 1.6
	MSG_WAITING_FOR_START  = 2050,		//Added in pj64 1.7
	MSG_INVALID_EXE        = 2051,		//Added in pj64 1.7
	MSG_INVALID_EXE_TITLE  = 2052,		//Added in pj64 1.7
	MSG_7Z_FILE_NOT_FOUND  = 2053,		//Added in pj64 1.7
	MSG_SET_LLE_GFX_TITLE  = 2054,		//Added in pj64 1.7
	MSG_SET_LLE_GFX_MSG    = 2055,		//Added in pj64 1.7
	MSG_SET_HLE_AUD_TITLE  = 2056,		//Added in pj64 1.7
	MSG_SET_HLE_AUD_MSG    = 2057,		//Added in pj64 1.7
};

#include ".\\Multilanguage\Language Class.h"
