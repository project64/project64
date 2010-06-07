#include <windows.h>

#include "Plugin.h"

#include "..\\Types.h"
#include "r4300i Commands.h"
#include "DMA.h"
#include "eeprom.h"
#include "sram.h"
#include "flashram.h"
#include "mempak.h"
#include "pif.h"

extern DWORD JumpToLocation;
extern BOOL TestTimer;

int  DelaySlotEffectsCompare ( DWORD PC, DWORD Reg1, DWORD Reg2 );
int  DelaySlotEffectsJump (DWORD JumpPC);
void InPermLoop         ( void );
void DisplayFPS         ( void );
