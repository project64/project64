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
#include <stdlib.h>
#include "OpCode.h"
#include <Project64-core/N64System/Mips/RegisterClass.h>

#define FPR_Type(Reg)	(Reg) == R4300i_COP1_S ? "S" : (Reg) == R4300i_COP1_D ? "D" :\
						(Reg) == R4300i_COP1_W ? "W" : "L"

char CommandName[100];

static const char * LabelName(uint32_t Address)
{
    static char strLabelName[100];
    sprintf(strLabelName, "0x%08X", Address);
    return strLabelName;
}

static const char * R4300iSpecialName(uint32_t OpCode, uint32_t /*PC*/)
{
    OPCODE command;
    command.Hex = OpCode;

    switch (command.funct)
    {
    case R4300i_SPECIAL_SLL:
        if (command.Hex != 0)
        {
            sprintf(CommandName, "SLL\t%s, %s, %d", CRegName::GPR[command.rd],
                CRegName::GPR[command.rt], command.sa);
        }
        else
        {
            sprintf(CommandName, "NOP");
        }
        break;
    case R4300i_SPECIAL_SRL:
        sprintf(CommandName, "SRL\t%s, %s, %d", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            command.sa);
        break;
    case R4300i_SPECIAL_SRA:
        sprintf(CommandName, "SRA\t%s, %s, %d", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            command.sa);
        break;
    case R4300i_SPECIAL_SLLV:
        sprintf(CommandName, "SLLV\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_SRLV:
        sprintf(CommandName, "SRLV\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_SRAV:
        sprintf(CommandName, "SRAV\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_JR:
        sprintf(CommandName, "JR\t%s", CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_JALR:
        sprintf(CommandName, "JALR\t%s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_SYSCALL:
        sprintf(CommandName, "SYSCALL\t0x%05X", command.code);
        break;
    case R4300i_SPECIAL_BREAK:
        sprintf(CommandName, "BREAK\t0x%05X", command.code);
        break;
    case R4300i_SPECIAL_SYNC:
        sprintf(CommandName, "SYNC");
        break;
    case R4300i_SPECIAL_MFHI:
        sprintf(CommandName, "MFHI\t%s", CRegName::GPR[command.rd]);
        break;
    case R4300i_SPECIAL_MTHI:
        sprintf(CommandName, "MTHI\t%s", CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_MFLO:
        sprintf(CommandName, "MFLO\t%s", CRegName::GPR[command.rd]);
        break;
    case R4300i_SPECIAL_MTLO:
        sprintf(CommandName, "MTLO\t%s", CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_DSLLV:
        sprintf(CommandName, "DSLLV\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_DSRLV:
        sprintf(CommandName, "DSRLV\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_DSRAV:
        sprintf(CommandName, "DSRAV\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_MULT:
        sprintf(CommandName, "MULT\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_MULTU:
        sprintf(CommandName, "MULTU\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DIV:
        sprintf(CommandName, "DIV\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DIVU:
        sprintf(CommandName, "DIVU\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DMULT:
        sprintf(CommandName, "DMULT\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DMULTU:
        sprintf(CommandName, "DMULTU\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DDIV:
        sprintf(CommandName, "DDIV\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DDIVU:
        sprintf(CommandName, "DDIVU\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_ADD:
        sprintf(CommandName, "ADD\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_ADDU:
        sprintf(CommandName, "ADDU\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_SUB:
        sprintf(CommandName, "SUB\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_SUBU:
        sprintf(CommandName, "SUBU\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_AND:
        sprintf(CommandName, "AND\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_OR:
        sprintf(CommandName, "OR\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_XOR:
        sprintf(CommandName, "XOR\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_NOR:
        sprintf(CommandName, "NOR\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_SLT:
        sprintf(CommandName, "SLT\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_SLTU:
        sprintf(CommandName, "SLTU\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DADD:
        sprintf(CommandName, "DADD\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DADDU:
        sprintf(CommandName, "DADDU\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DSUB:
        sprintf(CommandName, "DSUB\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DSUBU:
        sprintf(CommandName, "DSUBU\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TGE:
        sprintf(CommandName, "TGE\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TGEU:
        sprintf(CommandName, "TGEU\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TLT:
        sprintf(CommandName, "TLT\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TLTU:
        sprintf(CommandName, "TLTU\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TEQ:
        sprintf(CommandName, "TEQ\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TNE:
        sprintf(CommandName, "TNE\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DSLL:
        sprintf(CommandName, "DSLL\t%s, %s, %d", CRegName::GPR[command.rd],
            CRegName::GPR[command.rt], command.sa);
        break;
    case R4300i_SPECIAL_DSRL:
        sprintf(CommandName, "DSRL\t%s, %s, %d", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            command.sa);
        break;
    case R4300i_SPECIAL_DSRA:
        sprintf(CommandName, "DSRA\t%s, %s, %d", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            command.sa);
        break;
    case R4300i_SPECIAL_DSLL32:
        sprintf(CommandName, "DSLL32\t%s, %s, %d", CRegName::GPR[command.rd], CRegName::GPR[command.rt], command.sa);
        break;
    case R4300i_SPECIAL_DSRL32:
        sprintf(CommandName, "DSRL32\t%s, %s, %d", CRegName::GPR[command.rd], CRegName::GPR[command.rt], command.sa);
        break;
    case R4300i_SPECIAL_DSRA32:
        sprintf(CommandName, "DSRA32\t%s, %s, %d", CRegName::GPR[command.rd], CRegName::GPR[command.rt], command.sa);
        break;
    default:
        sprintf(CommandName, "UNKNOWN\t%02X %02X %02X %02X",
            command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
    }
    return CommandName;
}

static const char * R4300iRegImmName(uint32_t OpCode, uint32_t PC)
{
    OPCODE command;
    command.Hex = OpCode;

    switch (command.rt)
    {
    case R4300i_REGIMM_BLTZ:
        sprintf(CommandName, "BLTZ\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_REGIMM_BGEZ:
        if (command.rs == 0)
        {
            sprintf(CommandName, "B\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "BGEZ\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_REGIMM_BLTZL:
        sprintf(CommandName, "BLTZL\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_REGIMM_BGEZL:
        sprintf(CommandName, "BGEZL\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_REGIMM_TGEI:
        sprintf(CommandName, "TGEI\t%s, 0x%04X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_TGEIU:
        sprintf(CommandName, "TGEIU\t%s, 0x%04X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_TLTI:
        sprintf(CommandName, "TLTI\t%s, 0x%04X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_TLTIU:
        sprintf(CommandName, "TLTIU\t%s, 0x%04X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_TEQI:
        sprintf(CommandName, "TEQI\t%s, 0x%04X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_TNEI:
        sprintf(CommandName, "TNEI\t%s, 0x%04X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_BLTZAL:
        sprintf(CommandName, "BLTZAL\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_REGIMM_BGEZAL:
        if (command.rs == 0)
        {
            sprintf(CommandName, "BAL\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "BGEZAL\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_REGIMM_BLTZALL:
        sprintf(CommandName, "BLTZALL\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_REGIMM_BGEZALL:
        sprintf(CommandName, "BGEZALL\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    default:
        sprintf(CommandName, "UNKNOWN\t%02X %02X %02X %02X",
            command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
    }
    return CommandName;
}

static const char * R4300iCop1Name(uint32_t OpCode, uint32_t PC)
{
    OPCODE command;
    command.Hex = OpCode;

    switch (command.fmt)
    {
    case R4300i_COP1_MF:
        sprintf(CommandName, "MFC1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
        break;
    case R4300i_COP1_DMF:
        sprintf(CommandName, "DMFC1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
        break;
    case R4300i_COP1_CF:
        sprintf(CommandName, "CFC1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR_Ctrl[command.fs]);
        break;
    case R4300i_COP1_MT:
        sprintf(CommandName, "MTC1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
        break;
    case R4300i_COP1_DMT:
        sprintf(CommandName, "DMTC1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
        break;
    case R4300i_COP1_CT:
        sprintf(CommandName, "CTC1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR_Ctrl[command.fs]);
        break;
    case R4300i_COP1_BC:
        switch (command.ft)
        {
        case R4300i_COP1_BC_BCF:
            sprintf(CommandName, "BC1F\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
            break;
        case R4300i_COP1_BC_BCT:
            sprintf(CommandName, "BC1T\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
            break;
        case R4300i_COP1_BC_BCFL:
            sprintf(CommandName, "BC1FL\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
            break;
        case R4300i_COP1_BC_BCTL:
            sprintf(CommandName, "BC1TL\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
            break;
        default:
            sprintf(CommandName, "UNKNOWN COP1\t%02X %02X %02X %02X",
                command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
        }
        break;
    case R4300i_COP1_S:
    case R4300i_COP1_D:
    case R4300i_COP1_W:
    case R4300i_COP1_L:
        switch (command.funct)
        {
        case R4300i_COP1_FUNCT_ADD:
            sprintf(CommandName, "ADD.%s\t%s, %s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs],
                CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_SUB:
            sprintf(CommandName, "SUB.%s\t%s, %s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs],
                CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_MUL:
            sprintf(CommandName, "MUL.%s\t%s, %s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs],
                CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_DIV:
            sprintf(CommandName, "DIV.%s\t%s, %s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs],
                CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_SQRT:
            sprintf(CommandName, "SQRT.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_ABS:
            sprintf(CommandName, "ABS.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_MOV:
            sprintf(CommandName, "MOV.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_NEG:
            sprintf(CommandName, "NEG.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_ROUND_L:
            sprintf(CommandName, "ROUND.L.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_TRUNC_L:
            sprintf(CommandName, "TRUNC.L.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_CEIL_L:
            sprintf(CommandName, "CEIL.L.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_FLOOR_L:
            sprintf(CommandName, "FLOOR.L.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_ROUND_W:
            sprintf(CommandName, "ROUND.W.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_TRUNC_W:
            sprintf(CommandName, "TRUNC.W.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_CEIL_W:
            sprintf(CommandName, "CEIL.W.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_FLOOR_W:
            sprintf(CommandName, "FLOOR.W.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_CVT_S:
            sprintf(CommandName, "CVT.S.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_CVT_D:
            sprintf(CommandName, "CVT.D.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_CVT_W:
            sprintf(CommandName, "CVT.W.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_CVT_L:
            sprintf(CommandName, "CVT.L.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
            break;
        case R4300i_COP1_FUNCT_C_F:
            sprintf(CommandName, "C.F.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_UN:
            sprintf(CommandName, "C.UN.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_EQ:
            sprintf(CommandName, "C.EQ.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_UEQ:
            sprintf(CommandName, "C.UEQ.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_OLT:
            sprintf(CommandName, "C.OLT.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_ULT:
            sprintf(CommandName, "C.ULT.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_OLE:
            sprintf(CommandName, "C.OLE.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_ULE:
            sprintf(CommandName, "C.ULE.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_SF:
            sprintf(CommandName, "C.SF.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_NGLE:
            sprintf(CommandName, "C.NGLE.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_SEQ:
            sprintf(CommandName, "C.SEQ.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_NGL:
            sprintf(CommandName, "C.NGL.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_LT:
            sprintf(CommandName, "C.LT.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_NGE:
            sprintf(CommandName, "C.NGE.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_LE:
            sprintf(CommandName, "C.LE.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        case R4300i_COP1_FUNCT_C_NGT:
            sprintf(CommandName, "C.NGT.%s\t%s, %s", FPR_Type(command.fmt),
                CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
            break;
        default:
            sprintf(CommandName, "UNKNOWN COP1\t%02X %02X %02X %02X",
                command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
        }
        break;
    default:
        sprintf(CommandName, "UNKNOWN COP1\t%02X %02X %02X %02X",
            command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
    }
    return CommandName;
}

const char * R4300iOpcodeName(uint32_t OpCode, uint32_t PC)
{
    OPCODE command;
    command.Hex = OpCode;

    switch (command.op)
    {
    case R4300i_SPECIAL:
        return R4300iSpecialName(OpCode, PC);
        break;
    case R4300i_REGIMM:
        return R4300iRegImmName(OpCode, PC);
        break;
    case R4300i_J:
        sprintf(CommandName, "J\t%s", LabelName((PC & 0xF0000000) + (command.target << 2)));
        break;
    case R4300i_JAL:
        sprintf(CommandName, "JAL\t%s", LabelName((PC & 0xF0000000) + (command.target << 2)));
        break;
    case R4300i_BEQ:
        if (command.rs == 0 && command.rt == 0)
        {
            sprintf(CommandName, "B\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else if (command.rs == 0 || command.rt == 0)
        {
            sprintf(CommandName, "BEQZ\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "BEQ\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_BNE:
        if ((command.rs == 0) ^ (command.rt == 0))
        {
            sprintf(CommandName, "BNEZ\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "BNE\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_BLEZ:
        sprintf(CommandName, "BLEZ\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_BGTZ:
        sprintf(CommandName, "BGTZ\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_ADDI:
        sprintf(CommandName, "ADDI\t%s, %s, 0x%04X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_ADDIU:
        // special case for stack
        if (command.rt == 29)
        {
            short imm = (short)command.immediate;
            sprintf(CommandName, "ADDIU\t%s, %s, %s0x%02X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], imm < 0 ? "-" : "", abs(imm));
        }
        else
        {
            sprintf(CommandName, "ADDIU\t%s, %s, 0x%04X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        }
        break;
    case R4300i_SLTI:
        sprintf(CommandName, "SLTI\t%s, %s, 0x%04X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_SLTIU:
        sprintf(CommandName, "SLTIU\t%s, %s, 0x%04X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_ANDI:
        sprintf(CommandName, "ANDI\t%s, %s, 0x%04X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_ORI:
        sprintf(CommandName, "ORI\t%s, %s, 0x%04X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_XORI:
        sprintf(CommandName, "XORI\t%s, %s, 0x%04X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_LUI:
        sprintf(CommandName, "LUI\t%s, 0x%04X", CRegName::GPR[command.rt], command.immediate);
        break;
    case R4300i_CP0:
        switch (command.rs)
        {
        case R4300i_COP0_MF:
            sprintf(CommandName, "MFC0\t%s, %s", CRegName::GPR[command.rt], CRegName::Cop0[command.rd]);
            break;
        case R4300i_COP0_MT:
            sprintf(CommandName, "MTC0\t%s, %s", CRegName::GPR[command.rt], CRegName::Cop0[command.rd]);
            break;
        default:
            if ((command.rs & 0x10) != 0)
            {
                switch (command.funct)
                {
                case R4300i_COP0_CO_TLBR:  sprintf(CommandName, "TLBR"); break;
                case R4300i_COP0_CO_TLBWI: sprintf(CommandName, "TLBWI"); break;
                case R4300i_COP0_CO_TLBWR: sprintf(CommandName, "TLBWR"); break;
                case R4300i_COP0_CO_TLBP:  sprintf(CommandName, "TLBP"); break;
                case R4300i_COP0_CO_ERET:  sprintf(CommandName, "ERET"); break;
                default:
                    sprintf(CommandName, "UNKNOWN\t%02X %02X %02X %02X",
                        command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
                }
            }
            else
            {
                sprintf(CommandName, "UNKNOWN\t%02X %02X %02X %02X",
                    command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
            }
            break;
        }
        break;
    case R4300i_CP1:
        return R4300iCop1Name(OpCode, PC);
    case R4300i_BEQL:
        if (command.rs == command.rt)
        {
            sprintf(CommandName, "B\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else if ((command.rs == 0) ^ (command.rt == 0))
        {
            sprintf(CommandName, "BEQZL\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "BEQL\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_BNEL:
        if ((command.rs == 0) ^ (command.rt == 0))
        {
            sprintf(CommandName, "BNEZL\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "BNEL\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_BLEZL:
        sprintf(CommandName, "BLEZL\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_BGTZL:
        sprintf(CommandName, "BGTZL\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_DADDI:
        sprintf(CommandName, "DADDI\t%s, %s, 0x%04X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_DADDIU:
        sprintf(CommandName, "DADDIU\t%s, %s, 0x%04X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_LDL:
        sprintf(CommandName, "LDL\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LDR:
        sprintf(CommandName, "LDR\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LB:
        sprintf(CommandName, "LB\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LH:
        sprintf(CommandName, "LH\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LWL:
        sprintf(CommandName, "LWL\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LW:
        sprintf(CommandName, "LW\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LBU:
        sprintf(CommandName, "LBU\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LHU:
        sprintf(CommandName, "LHU\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LWR:
        sprintf(CommandName, "LWR\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LWU:
        sprintf(CommandName, "LWU\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SB:
        sprintf(CommandName, "SB\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SH:
        sprintf(CommandName, "SH\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SWL:
        sprintf(CommandName, "SWL\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SW:
        sprintf(CommandName, "SW\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SDL:
        sprintf(CommandName, "SDL\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SDR:
        sprintf(CommandName, "SDR\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SWR:
        sprintf(CommandName, "SWR\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_CACHE:
        sprintf(CommandName, "CACHE\t%d, 0x%04X (%s)", command.rt, command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LL:
        sprintf(CommandName, "LL\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LWC1:
        sprintf(CommandName, "LWC1\t%s, 0x%04X (%s)", CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LDC1:
        sprintf(CommandName, "LDC1\t%s, 0x%04X (%s)", CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LD:
        sprintf(CommandName, "LD\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SC:
        sprintf(CommandName, "SC\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SWC1:
        sprintf(CommandName, "SWC1\t%s, 0x%04X (%s)", CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SDC1:
        sprintf(CommandName, "SDC1\t%s, 0x%04X (%s)", CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SD:
        sprintf(CommandName, "SD\t%s, 0x%04X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    default:
        sprintf(CommandName, "UNKNOWN\t%02X %02X %02X %02X",
            command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
    }

    return CommandName;
}