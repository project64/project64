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
#pragma once

#include <Common/stdtypes.h>

/*
 * The limits of COP1 extend to native SSE2 register capabilities, but for
 * now this is only being included to dodge the MSVC inline asm for x86.
 *
 * As better cross-platform methods of handling FP precision are implemented
 * for non-Intel-architecture builds, this #include may become obsolete.
 */
#if defined(__i386) || defined(__x86_64__) || defined(_M_X64)
#include <emmintrin.h>
#endif

enum PauseType
{
    PauseType_None,
    PauseType_FromMenu,
    PauseType_AppLostActive,
    PauseType_AppLostFocus,
    PauseType_SaveGame,
    PauseType_LoadGame,
    PauseType_DumpMemory,
    PauseType_SearchMemory,
    PauseType_Settings,
    PauseType_Cheats,
    PauseType_ChangingBPs,
};

enum CPU_TYPE
{
    CPU_Default = -1, CPU_Interpreter = 1, CPU_Recompiler = 2, CPU_SyncCores = 3
};

enum FRAMERATE_TYPE
{
    FR_VIs = 0, FR_DLs = 1, FR_PERCENT = 2, FR_VIs_DLs = 3,
};

enum SAVE_CHIP_TYPE
{
    SaveChip_Auto = -1, SaveChip_Eeprom_4K, SaveChip_Eeprom_16K, SaveChip_Sram, SaveChip_FlashRam
};

enum FUNC_LOOKUP_METHOD
{
    FuncFind_Default = -1, FuncFind_PhysicalLookup = 1, FuncFind_VirtualLookup = 2, FuncFind_ChangeMemory = 3,
};

enum SYSTEM_TYPE
{
    SYSTEM_NTSC = 0, SYSTEM_PAL = 1, SYSTEM_MPAL = 2
};

enum CICChip
{
    CIC_UNKNOWN = -1, CIC_NUS_6101 = 1, CIC_NUS_6102 = 2, CIC_NUS_6103 = 3,
    CIC_NUS_6104 = 4, CIC_NUS_6105 = 5, CIC_NUS_6106 = 6, CIC_NUS_5167 = 7,
    CIC_NUS_8303 = 8, CIC_NUS_DDUS = 9, CIC_NUS_DDTL = 10
};

enum Country
{
    NTSC_BETA = 0x37, X_NTSC = 0x41, Germany = 0x44, USA = 0x45, french = 0x46, Italian = 0x49,
    Japan = 0x4A, Europe = 0x50, Spanish = 0x53, Australia = 0x55, X_PAL = 0x58, Y_PAL = 0x59,
    UnknownCountry = 0
};

enum PROFILE_TIMERS
{
    Timer_None = 0,
    Timer_R4300 = 1,
    Timer_RSP_Dlist = 2,
    Timer_RSP_Alist = 3,
    Timer_RSP_Unknown = 4,
    Timer_RefreshScreen = 5,
    Timer_UpdateScreen = 6,
    Timer_UpdateFPS = 7,
    Timer_Idel = 8,
    Timer_Max = 9,
};

enum STEP_TYPE
{
    NORMAL = 0,
    DO_DELAY_SLOT = 1,
    DO_END_DELAY_SLOT = 2,
    DELAY_SLOT = 3,
    END_DELAY_SLOT = 4,
    LIKELY_DELAY_SLOT = 5,
    JUMP = 6,
    DELAY_SLOT_DONE = 7,
    LIKELY_DELAY_SLOT_DONE = 8,
    END_BLOCK = 9,
    PERMLOOP_DO_DELAY = 10,
    PERMLOOP_DELAY_DONE = 11,
};

union MIPS_WORD
{
    int32_t  W;
    uint32_t UW;
    int16_t  HW[2];
    uint16_t UHW[2];
    int8_t   B[4];
    uint8_t  UB[4];

    float    F;
};

union MIPS_DWORD
{
    int64_t  DW;
    uint64_t UDW;
    int32_t  W[2];
    uint32_t UW[2];
    int16_t  HW[4];
    uint16_t UHW[4];
    int8_t   B[8];
    uint8_t  UB[8];

    double   D;
    float    F[2];
};
