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

#pragma warning(push)
#pragma warning(disable : 4201) // warning C4201: nonstandard extension used : nameless struct/union

union ArmThumbOpcode
{
    uint16_t Hex;
    uint8_t Ascii[2];

    struct
    {
        unsigned rt : 3;
        unsigned rn : 3;
        unsigned rm : 3;
        unsigned opcode : 7;
    };
};

union Arm32Opcode
{
    uint32_t Hex;
    uint8_t Ascii[4];

    // uint16 + uint16 type instuction
    struct
    {
        unsigned rn : 4;
        unsigned opcode : 12;
        unsigned rm : 4;
        unsigned imm2 : 2;
        unsigned reserved : 6;
        unsigned rt : 4;
    } uint16;

    struct
    {
        unsigned rm : 4;
        unsigned opcode2 : 8;
        unsigned rt : 4;
        unsigned rn : 4;
        unsigned opcode : 12;
    } uint32;
};
#pragma warning(pop)

enum ArmThumbOpCodes
{
    ArmSTR_Reg = 40,
    ArmLDR_Reg = 44, 
    ArmLDRH_Reg = 0xE19,
    ArmLDRH_W = 0xF83,
};
