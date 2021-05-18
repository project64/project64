/*
N-Rage`s Dinput8 Plugin
(C) 2002, 2006  Norbert Wladyka

Author`s Email: norbert.wladyka@chello.at
Website: http://go.to/nrage

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _NRAGEPLUGIN_
#define _NRAGEPLUGIN_

#include <dinput.h>

#include "XInputController.h"

// General plugin

#define TIMER_MESSAGEWINDOW 123

// Maximum number of devices other than SysMouse
#define MAX_DEVICES     32
// Maximum number of modifiers
#define MAX_MODIFIERS   256

#define DEFAULT_STICKRANGE      66
#define DEFAULT_DEADZONE        5
#define DEFAULT_RUMBLETYP       RUMBLE_EFF1
#define DEFAULT_RUMBLESTRENGTH  80
#define DEFAULT_MOUSESENSIVITY  100
#define DEFAULT_PAKTYPE         PAK_MEM
#define DEFAULT_MOUSEMOVE       MM_BUFF

#define PAK_NONE        0
#define PAK_MEM         1
#define PAK_RUMBLE      2
#define PAK_TRANSFER    3
#define PAK_VOICE       4
#define PAK_ADAPTOID    7

// Just used to display text in GUI
#define PAK_NONRAW      16

typedef struct _EMULATOR_INFO
{
    bool fInitialisedPlugin;
    HWND hMainWindow;
    HINSTANCE hinst;
    LANGID Language;
    bool fDisplayShortPop;  // Do we display shortcut message popups?

/*
11/09/2015 Comment by cxd4
Added to keep the real address of the CONTROL array stored.
This became necessary due to conflicts between specs #1.0, #1.1 and #1.2.
*/

    CONTROL * controllers;

    //  BOOL MemoryBswaped;     // If this is set to TRUE, then the memory has been pre-bswap'd on a DWORD (32-bit) boundary, only effects header.
    //  eg. the first 8 bytes are stored like this:
    //  4 3 2 1   8 7 6 5
    //  BYTE * HEADER; // This is the ROM header (first 40h bytes of the ROM)
} EMULATOR_INFO, *LPEMULATOR_INFO;

typedef struct _DEVICE
{
    LPDIRECTINPUTDEVICE8 didHandle;
    TCHAR szProductName[MAX_PATH];
    BYTE bProductCounter;
    GUID guidInstance;
    DWORD dwDevType;                // Can be DI8DEVTYPE_KEYBOARD, DI8DEVTYPE_MOUSE, etc.
    BYTE bEffType;                  // What rumble effects does this device support?
    union INPUTSTATE                // The last polled data from this device
    {
        DIJOYSTATE joyState;
        DIMOUSESTATE2 mouseState;
        BYTE rgbButtons[256];       // Keyboard state
    } stateAs;
} DEVICE, *LPDEVICE;

typedef struct _BUTTON
{
    bool fPrevPressed;      // Was this button pressed last time we checked? (not to be saved in config)
    BYTE bOffset;           // Offset in the DirectInput data structure
    BYTE bAxisID;           // Tells which range of the Axe/POV is important (see AI_AXE_P, AI_POV_UP, etc.)
    BYTE bBtnType;              // Type of device/button: keyboard key, joystick axis, joystick button, mouse axis, etc.
    LPDEVICE parentDevice;  // Pointer to the DEVICE this assignment came from
} BUTTON, *LPBUTTON;

// Modifiers are a feature built into N-Rage.  Emulator turbo buttons, macros, stuff like that.
typedef struct _MODIFIER
{
    BUTTON btnButton;   // Button to associate with
    BYTE bModType;          // Type of modifier (None, Movement, Macro, Config)
    BOOL fToggle;       // False if you have to hold the button down to activate, true if the modifier toggles on button press
    BOOL fStatus;       // If true, control defaults to ACTIVE, and deactivates on button press
    DWORD32 dwSpecific; // Will be cast to MODSPEC_MOVE, MODSPEC_MACRO, or MODSPEC_CONFIG
} MODIFIER, *LPMODIFIER;

// bModType (modifiers)
#define MDT_NONE        0
#define MDT_MOVE        1
#define MDT_MACRO       2
#define MDT_CONFIG      3

// Buffered
#define MM_BUFF     0
// Absolute
#define MM_ABS      1
// Deadpan
#define MM_DEAD     2

// Number of analog axes. Standard N64 controller has just 2: X and Y joystick.
#define PF_AXESETS              2

typedef struct _CONTROLLER      // An N64 controller
{
    unsigned fPlugged;          // Is the controller "plugged" / connected? (i.e. does the emulator see a controller on this port?)
    unsigned fXInput;           // Is the controller an XInput device?
    unsigned fN64Mouse;         // Is the controller an N64 mouse (relative)?
    unsigned fRawData;          // Are we using raw mode for this controller?
    unsigned fIsAdaptoid;       // Is it an Adaptoid?

    unsigned fKeyboard;         // Does it use a keyboard?
    unsigned fMouse;            // Does it use a mouse?
    unsigned fGamePad;          // Does it use a gamepad/joystick?

    unsigned fRealN64Range;     // Does it have the "real N64 range" flag set?
    unsigned bAxisSet;          // Which set of axes are we using? (Control 1, Control 2)
    unsigned bMouseMoveX;       // Does it use buffered/absolute mouse for X?  MM_BUFF, MM_ABS, MM_DEAD
    unsigned bMouseMoveY;       // Does it use buffered/absolute mouse for Y?
    unsigned fKeyAbsoluteX;     // Does it use absolute key for X?
    unsigned fKeyAbsoluteY;     // Does it use absolute key for Y?

    unsigned fPakInitialized;   // Has our pak been initialized?  Used to make sure we don't try to write to a memory pak that doesn't point to anything.
    unsigned fPakCRCError;      // The ROM sends CRC data when it tries to write to a memory pak.  Is the CRC incorrect?  Usually indicates a bad ROM.
    unsigned PakType;           // What type of controller pak? Memory pak? Rumble pak? Transfer pak? etc.
    unsigned fVisualRumble;     // Is visual rumble enabled for this controller?

    unsigned bBackgroundInput;  // Allow input while main window isn't focused?

	unsigned XcheckTime;		// Checks for newly connected gamepads timer

    BYTE bRumbleTyp;                // What type of rumble effect? None, constant, ramp, or direct?

    GUID guidFFDevice;              // GUID of the device that rumble gets sent to

    BYTE bStickRange;               // Our "range modifier"

    long wAxeBuffer[4];             // Makes pseudo-relative movement possible through keyboard or buttons and also acts as a mouse buffer

    WORD wMouseSensitivityX;        // Set per N64 controller, that's OK
    WORD wMouseSensitivityY;
    BYTE bPadDeadZone;              // Our manual dead zone, set per N64 controller
    BYTE bRumbleStrength;           // Set per N64 controller
    unsigned short nModifiers;      // Number of modifiers

    bool bRapidFireEnabled;
    BYTE bRapidFireRate;
    BYTE bRapidFireCounter;

    TCHAR szMempakFile[MAX_PATH + 1];       // MemPak-FileName
    TCHAR szTransferRom[MAX_PATH + 1];  // GameBoyRom-Filename
    TCHAR szTransferSave[MAX_PATH + 1]; // GameBoyEEPRom-Filename

    BUTTON aButton[14 + PF_AXESETS * 4];    // Ten buttons, 4 D-pad directions times two (for Config 1 and Config 2)

    MODIFIER *pModifiers;               // Array of modifiers

    void *pPakData;                     // Pointer to pak Data (specific): see PakIO.h
    // pPakData->bPakType will always be a BYTE indicating what the current pak type is

    XCONTROLLER xiController;           // To handle an XInput-enabled controller (comment by tecnicors)
} CONTROLLER, *LPCONTROLLER;

// This is the index of WORD PROFILE.Button[X]
// Buttons:
#define PF_DPADR    0
#define PF_DPADL    1
#define PF_DPADD    2
#define PF_DPADU    3
#define PF_START    4
#define PF_TRIGGERZ 5
#define PF_BBUTTON  6
#define PF_ABUTTON  7
#define PF_CBUTTONR 8
#define PF_CBUTTONL 9
#define PF_CBUTTOND 10
#define PF_CBUTTONU 11
#define PF_TRIGGERR 12
#define PF_TRIGGERL 13

// Analog stick
// Because you can assign buttons to it, we need 4 of them
#define PF_APADR    14
#define PF_APADL    15
#define PF_APADD    16
#define PF_APADU    17

// Second set
// #define PF_APADR 18
// #define PF_APADL 19
// #define PF_APADD 20
// #define PF_APADU 21

// Data format of DWORD Controller.Button:

// BYTE bBtnType: Determines the device and general type of control
// BYTE bAxisID: AxeIndentifier, which tells which range of the axes/POV is important
// BYTE bOffset: Offset in the DirectInput data structure

// BYTE bBtnType: Determines the device and general type of control
#define DT_UNASSIGNED       0
// Joystick
#define DT_JOYBUTTON        1
#define DT_JOYAXE           2
#define DT_JOYPOV           3
#define DT_JOYSLIDER        4

// Keyboard
#define DT_KEYBUTTON        5

// Mouse
#define DT_MOUSEBUTTON      6
#define DT_MOUSEAXE         7

// BYTE bAxisID: AxeIndentifier, which tells which range of the axes/POV is important

// Positive range of a the axes
#define AI_AXE_P        0
// Negative range
#define AI_AXE_N        1

// Applies to POVs obviously
#define AI_POV_UP       0
#define AI_POV_RIGHT    1
#define AI_POV_DOWN     2
#define AI_POV_LEFT     3

// BYTE bOffset: Offset in the DirectInput data structure

typedef union _MODSPEC_MOVE
{
    DWORD dwValue;
    struct
    {
        short XModification;
        short YModification;
    };
} MODSPEC_MOVE, *LPMODSPEC_MOVE;

typedef union _MODSPEC_MACRO
{
    DWORD dwValue;
    struct
    {
        unsigned short aButtons;
        unsigned short aFlags;
    };
    struct
    {
        unsigned fDigitalRight : 1;
        unsigned fDigitalLeft : 1;
        unsigned fDigitalDown : 1;
        unsigned fDigitalUp : 1;
        unsigned fStart : 1;
        unsigned fTriggerZ : 1;
        unsigned fBButton : 1;
        unsigned fAButton : 1;
        unsigned fCRight : 1;
        unsigned fCLeft : 1;
        unsigned fCDown : 1;
        unsigned fCUp : 1;
        unsigned fTriggerR : 1;
        unsigned fTriggerL : 1;
        unsigned : 2;

        unsigned fAnalogRight : 1;
        unsigned fAnalogLeft : 1;
        unsigned fAnalogDown : 1;
        unsigned fAnalogUp : 1;
        unsigned fRapidFire : 1;
        unsigned fRapidFireRate : 1;
        unsigned fPrevFireState : 1;
        unsigned fPrevFireState2 : 1;
    };
} MODSPEC_MACRO, *LPMODSPEC_MACRO;

typedef union _MODSPEC_CONFIG
{
    DWORD dwValue;
    struct
    {
        BYTE bAnalogStick;
        BYTE bMouse;
        BYTE bKeyboard;
    };
    struct
    {
        unsigned fChangeAnalogConfig : 1;
        unsigned fAnalogStickMode : 7;
        unsigned fChangeMouseXAxis : 1;
        unsigned fChangeMouseYAxis : 1;
        unsigned : 6;
        unsigned fChangeKeyboardXAxis : 1;
        unsigned fChangeKeyboardYAxis : 1;
        unsigned : 6;
    };
} MODSPEC_CONFIG, *LPMODSPEC_CONFIG;

#define SC_NOPAK        0
#define SC_MEMPAK       1
#define SC_RUMBPAK      2
#define SC_TRANSPAK     3
#define SC_VOICEPAK     4
#define SC_ADAPTPAK     5
#define SC_SWMEMRUMB    6
#define SC_SWMEMADAPT   7

// Total array size of aButtons in SHORTCUTSPL;
// Make sure you update this if you change the list above
#define SC_TOTAL        8

typedef struct _SHORTCUTSPL
{
    BUTTON aButtons[SC_TOTAL];
    //BUTTON NoPakButton;
    //BUTTON MemPakButton;
    //BUTTON RumblePakButton;
    //BUTTON TransferPakButton;
    //BUTTON VoicePakButton;
    //BUTTON AdaptoidPakButton;
    //BUTTON SwMemRumbleButton;
    //BUTTON SwMemAdaptoidButton;
} SHORTCUTSPL, *LPSHORTCUTSPL;

typedef struct _SHORTCUTS
{
    SHORTCUTSPL Player[4];
    BUTTON bMouseLock;
} SHORTCUTS, *LPSHORTCUTS;

typedef struct _MSHORTCUT {
    int iControl;
    int iShortcut;
} MSHORTCUT, *LPMSHORTCUT;  // Shortcut message

#define CHECK_WHITESPACES( str ) ( str == '\r' || str == '\n' || str == '\t' )

extern HANDLE g_hHeap;
extern HMODULE g_hDirectInputDLL;
extern HMODULE g_hXInputDLL;
extern HMODULE g_hResourceDLL;
extern EMULATOR_INFO g_strEmuInfo;
extern TCHAR g_aszDefFolders[3][MAX_PATH];
extern TCHAR g_aszLastBrowse[6][MAX_PATH];
extern int g_nDevices;
extern DEVICE g_devList[MAX_DEVICES];
extern DEVICE g_sysMouse;
extern CONTROLLER g_pcControllers[4];
extern SHORTCUTS g_scShortcuts;
extern LPDIRECTINPUTDEVICE8 g_apFFDevice[4];
extern LPDIRECTINPUTEFFECT g_apdiEffect[4];
extern CRITICAL_SECTION g_critical;

extern bool g_bRunning;
extern bool g_bConfiguring;
extern bool g_bExclusiveMouse;

extern int g_iFirstController;

int WarningMessage(UINT uTextID, UINT uType);
int FindDeviceinList(const TCHAR *pszProductName, BYTE bProductCounter, bool fFindSimilar);
int FindDeviceinList(REFGUID rGUID);
void freePakData(CONTROLLER *pcController);
void freeModifiers(CONTROLLER *pcController);
void CheckShortcuts();
bool ErrorMessage(UINT uID, DWORD dwError, bool fUserChoose);

#endif
