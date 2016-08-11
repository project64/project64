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
#if defined(__arm__) || defined(_M_ARM)

#pragma warning(push)
#pragma warning(disable : 4201) // warning C4201: nonstandard extension used : nameless struct/union

union ArmThumbOpcode
{
    uint16_t Hex;
    uint8_t Ascii[2];

    struct
    {
        unsigned reserved : 3;
        unsigned rm : 4;
        unsigned opcode : 9;
    } Branch;

    struct
    {
        unsigned imm : 11;
        unsigned opcode : 5;
    } BranchImm;

    struct
    {
        unsigned imm : 8;
        unsigned cond : 4;
        unsigned opcode : 4;
    } BranchImmCond;

    struct
    {
        unsigned rt : 3;
        unsigned rn : 3;
        unsigned rm : 3;
        unsigned opcode : 7;
    } Reg;

    struct
    {
        unsigned imm8 : 8;
        unsigned rdn : 3;
        unsigned opcode : 5;
    } Imm8;

    struct
    {
        unsigned rd : 3;
        unsigned rn : 3;
        unsigned imm3 : 3;
        unsigned opcode : 7;
    } Imm3;

    struct
    {
        unsigned rt : 3;
        unsigned rn : 3;
        unsigned imm5 : 5;
        unsigned opcode : 5;
    } Imm5;

    struct
    {
        unsigned rn : 3;
        unsigned rm : 3;
        unsigned opcode : 10;
    } Reg2;

    struct
    {
        unsigned register_list : 8;
        unsigned m : 1;
        unsigned opcode : 7;
    } Push;

    struct
    {
        unsigned register_list : 8;
        unsigned p : 1;
        unsigned opcode : 7;
    } Pop;
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
        unsigned Rn : 4;
        unsigned s : 1;
        unsigned opcode : 5;
        unsigned i : 1;
        unsigned opcode2 : 5;

        unsigned imm8 : 8;
        unsigned rd : 4;
        unsigned imm3 : 3;
        unsigned reserved : 1;
    } RnRdImm12;

    struct
    {
        unsigned rn : 4;
        unsigned opcode : 12;
        unsigned imm : 12;
        unsigned rt : 4;
    } imm12;

    struct
    {
        unsigned imm4 : 4;
        unsigned opcode2 : 6;
        unsigned i : 1;
        unsigned opcode : 5;
        unsigned imm8 : 8;
        unsigned rd : 4;
        unsigned imm3 : 3;
        unsigned reserved : 1;
    } imm16;

    struct
    {
        unsigned imm6 : 6;
        unsigned cond : 4;
        unsigned S : 1;
        unsigned Opcode : 5;

        unsigned imm11 : 11;
        unsigned J2 : 1;
        unsigned val12 : 1;
        unsigned J1 : 1;
        unsigned val14 : 2;
    } Branch20;

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
    ArmSTR_ThumbImm = 0xC,
    ArmSTR_Reg = 40,
    ArmLDR_ThumbImm = 0xD,
    ArmLDR_Reg = 44,
    ArmLDRH_Reg = 0xE19,
    ArmLDRH_W = 0xF83,
    ArmMOV_IMM16 = 0x1E,
    ArmMOVW_IMM16 = 0x24,
    ArmMOVT_IMM16 = 0x2C,
    ArmPUSH = 0x5A,
    ArmPOP = 0x5E,
};

#endif