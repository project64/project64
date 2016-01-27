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
            sprintf(CommandName, "sll\t%s, %s, 0x%X", CRegName::GPR[command.rd],
                CRegName::GPR[command.rt], command.sa);
        }
        else
        {
            sprintf(CommandName, "nop");
        }
        break;
    case R4300i_SPECIAL_SRL:
        sprintf(CommandName, "srl\t%s, %s, 0x%X", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            command.sa);
        break;
    case R4300i_SPECIAL_SRA:
        sprintf(CommandName, "sra\t%s, %s, 0x%X", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            command.sa);
        break;
    case R4300i_SPECIAL_SLLV:
        sprintf(CommandName, "sllv\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_SRLV:
        sprintf(CommandName, "srlv\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_SRAV:
        sprintf(CommandName, "srav\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_JR:
        sprintf(CommandName, "jr\t%s", CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_JALR:
        sprintf(CommandName, "jalr\t%s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_SYSCALL:
        sprintf(CommandName, "system call");
        break;
    case R4300i_SPECIAL_BREAK:
        sprintf(CommandName, "break");
        break;
    case R4300i_SPECIAL_SYNC:
        sprintf(CommandName, "sync");
        break;
    case R4300i_SPECIAL_MFHI:
        sprintf(CommandName, "mfhi\t%s", CRegName::GPR[command.rd]);
        break;
    case R4300i_SPECIAL_MTHI:
        sprintf(CommandName, "mthi\t%s", CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_MFLO:
        sprintf(CommandName, "mflo\t%s", CRegName::GPR[command.rd]);
        break;
    case R4300i_SPECIAL_MTLO:
        sprintf(CommandName, "mtlo\t%s", CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_DSLLV:
        sprintf(CommandName, "dsllv\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_DSRLV:
        sprintf(CommandName, "dsrlv\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_DSRAV:
        sprintf(CommandName, "dsrav\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            CRegName::GPR[command.rs]);
        break;
    case R4300i_SPECIAL_MULT:
        sprintf(CommandName, "mult\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_MULTU:
        sprintf(CommandName, "multu\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DIV:
        sprintf(CommandName, "div\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DIVU:
        sprintf(CommandName, "divu\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DMULT:
        sprintf(CommandName, "dmult\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DMULTU:
        sprintf(CommandName, "dmultu\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DDIV:
        sprintf(CommandName, "ddiv\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DDIVU:
        sprintf(CommandName, "ddivu\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_ADD:
        sprintf(CommandName, "add\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_ADDU:
        sprintf(CommandName, "addu\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_SUB:
        sprintf(CommandName, "sub\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_SUBU:
        sprintf(CommandName, "subu\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_AND:
        sprintf(CommandName, "and\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_OR:
        sprintf(CommandName, "or\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_XOR:
        sprintf(CommandName, "xor\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_NOR:
        sprintf(CommandName, "nor\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_SLT:
        sprintf(CommandName, "slt\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_SLTU:
        sprintf(CommandName, "sltu\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DADD:
        sprintf(CommandName, "dadd\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DADDU:
        sprintf(CommandName, "daddu\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DSUB:
        sprintf(CommandName, "dsub\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DSUBU:
        sprintf(CommandName, "dsubu\t%s, %s, %s", CRegName::GPR[command.rd], CRegName::GPR[command.rs],
            CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TGE:
        sprintf(CommandName, "tge\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TGEU:
        sprintf(CommandName, "tgeu\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TLT:
        sprintf(CommandName, "tlt\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TLTU:
        sprintf(CommandName, "tltu\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TEQ:
        sprintf(CommandName, "teq\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_TNE:
        sprintf(CommandName, "tne\t%s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
        break;
    case R4300i_SPECIAL_DSLL:
        sprintf(CommandName, "dsll\t%s, %s, 0x%X", CRegName::GPR[command.rd],
            CRegName::GPR[command.rt], command.sa);
        break;
    case R4300i_SPECIAL_DSRL:
        sprintf(CommandName, "dsrl\t%s, %s, 0x%X", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            command.sa);
        break;
    case R4300i_SPECIAL_DSRA:
        sprintf(CommandName, "dsra\t%s, %s, 0x%X", CRegName::GPR[command.rd], CRegName::GPR[command.rt],
            command.sa);
        break;
    case R4300i_SPECIAL_DSLL32:
        sprintf(CommandName, "dsll32\t%s, %s, 0x%X", CRegName::GPR[command.rd], CRegName::GPR[command.rt], command.sa);
        break;
    case R4300i_SPECIAL_DSRL32:
        sprintf(CommandName, "dsrl32\t%s, %s, 0x%X", CRegName::GPR[command.rd], CRegName::GPR[command.rt], command.sa);
        break;
    case R4300i_SPECIAL_DSRA32:
        sprintf(CommandName, "dsra32\t%s, %s, 0x%X", CRegName::GPR[command.rd], CRegName::GPR[command.rt], command.sa);
        break;
    default:
        sprintf(CommandName, "Unknown\t%02X %02X %02X %02X",
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
        sprintf(CommandName, "bltz\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_REGIMM_BGEZ:
        if (command.rs == 0)
        {
            sprintf(CommandName, "b\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "bgez\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_REGIMM_BLTZL:
        sprintf(CommandName, "bltzl\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_REGIMM_BGEZL:
        sprintf(CommandName, "bgezl\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_REGIMM_TGEI:
        sprintf(CommandName, "tgei\t%s, 0x%X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_TGEIU:
        sprintf(CommandName, "tgeiu\t%s, 0x%X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_TLTI:
        sprintf(CommandName, "tlti\t%s, 0x%X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_TLTIU:
        sprintf(CommandName, "tltiu\t%s, 0x%X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_TEQI:
        sprintf(CommandName, "teqi\t%s, 0x%X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_TNEI:
        sprintf(CommandName, "tnei\t%s, 0x%X", CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_REGIMM_BLTZAL:
        sprintf(CommandName, "bltzal\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_REGIMM_BGEZAL:
        if (command.rs == 0)
        {
            sprintf(CommandName, "bal\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "bgezal\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_REGIMM_BLTZALL:
        sprintf(CommandName, "bltzall\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_REGIMM_BGEZALL:
        sprintf(CommandName, "bgezall\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    default:
        sprintf(CommandName, "Unknown\t%02X %02X %02X %02X",
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
        sprintf(CommandName, "mfc1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
        break;
    case R4300i_COP1_DMF:
        sprintf(CommandName, "dmfc1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
        break;
    case R4300i_COP1_CF:
        sprintf(CommandName, "cfc1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR_Ctrl[command.fs]);
        break;
    case R4300i_COP1_MT:
        sprintf(CommandName, "mtc1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
        break;
    case R4300i_COP1_DMT:
        sprintf(CommandName, "dmtc1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
        break;
    case R4300i_COP1_CT:
        sprintf(CommandName, "ctc1\t%s, %s", CRegName::GPR[command.rt], CRegName::FPR_Ctrl[command.fs]);
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
            sprintf(CommandName, "Unknown Cop1\t%02X %02X %02X %02X",
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
            sprintf(CommandName, "Unknown Cop1\t%02X %02X %02X %02X",
                command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
        }
        break;
    default:
        sprintf(CommandName, "Unknown Cop1\t%02X %02X %02X %02X",
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
        sprintf(CommandName, "j\t%s", LabelName((PC & 0xF0000000) + (command.target << 2)));
        break;
    case R4300i_JAL:
        sprintf(CommandName, "jal\t%s", LabelName((PC & 0xF0000000) + (command.target << 2)));
        break;
    case R4300i_BEQ:
        if (command.rs == 0 && command.rt == 0)
        {
            sprintf(CommandName, "b\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else if (command.rs == 0 || command.rt == 0)
        {
            sprintf(CommandName, "beqz\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "beq\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_BNE:
        if ((command.rs == 0) ^ (command.rt == 0))
        {
            sprintf(CommandName, "bnez\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "bne\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_BLEZ:
        sprintf(CommandName, "blez\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_BGTZ:
        sprintf(CommandName, "bgtz\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_ADDI:
        sprintf(CommandName, "addi\t%s, %s, 0x%X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_ADDIU:
        sprintf(CommandName, "addiu\t%s, %s, 0x%X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_SLTI:
        sprintf(CommandName, "slti\t%s, %s, 0x%X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_SLTIU:
        sprintf(CommandName, "sltiu\t%s, %s, 0x%X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_ANDI:
        sprintf(CommandName, "andi\t%s, %s, 0x%X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_ORI:
        sprintf(CommandName, "ori\t%s, %s, 0x%X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_XORI:
        sprintf(CommandName, "xori\t%s, %s, 0x%X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_LUI:
        sprintf(CommandName, "lui\t%s, 0x%X", CRegName::GPR[command.rt], command.immediate);
        break;
    case R4300i_CP0:
        switch (command.rs)
        {
        case R4300i_COP0_MF:
            sprintf(CommandName, "mfc0\t%s, %s", CRegName::GPR[command.rt], CRegName::Cop0[command.rd]);
            break;
        case R4300i_COP0_MT:
            sprintf(CommandName, "mtc0\t%s, %s", CRegName::GPR[command.rt], CRegName::Cop0[command.rd]);
            break;
        default:
            if ((command.rs & 0x10) != 0)
            {
                switch (command.funct)
                {
                case R4300i_COP0_CO_TLBR:  sprintf(CommandName, "tlbr"); break;
                case R4300i_COP0_CO_TLBWI: sprintf(CommandName, "tlbwi"); break;
                case R4300i_COP0_CO_TLBWR: sprintf(CommandName, "tlbwr"); break;
                case R4300i_COP0_CO_TLBP:  sprintf(CommandName, "tlbp"); break;
                case R4300i_COP0_CO_ERET:  sprintf(CommandName, "eret"); break;
                default:
                    sprintf(CommandName, "Unknown\t%02X %02X %02X %02X",
                        command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
                }
            }
            else
            {
                sprintf(CommandName, "Unknown\t%02X %02X %02X %02X",
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
            sprintf(CommandName, "b\t%s", LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else if ((command.rs == 0) ^ (command.rt == 0))
        {
            sprintf(CommandName, "beqzl\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "beql\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_BNEL:
        if ((command.rs == 0) ^ (command.rt == 0))
        {
            sprintf(CommandName, "bnezl\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        else
        {
            sprintf(CommandName, "bnel\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
                LabelName(PC + ((int16_t)command.offset << 2) + 4));
        }
        break;
    case R4300i_BLEZL:
        sprintf(CommandName, "blezl\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_BGTZL:
        sprintf(CommandName, "bgtzl\t%s, %s", CRegName::GPR[command.rs], LabelName(PC + ((int16_t)command.offset << 2) + 4));
        break;
    case R4300i_DADDI:
        sprintf(CommandName, "daddi\t%s, %s, 0x%X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_DADDIU:
        sprintf(CommandName, "daddiu\t%s, %s, 0x%X", CRegName::GPR[command.rt], CRegName::GPR[command.rs], command.immediate);
        break;
    case R4300i_LDL:
        sprintf(CommandName, "ldl\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LDR:
        sprintf(CommandName, "ldr\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LB:
        sprintf(CommandName, "lb\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LH:
        sprintf(CommandName, "lh\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LWL:
        sprintf(CommandName, "lwl\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LW:
        sprintf(CommandName, "lw\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LBU:
        sprintf(CommandName, "lbu\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LHU:
        sprintf(CommandName, "lhu\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LWR:
        sprintf(CommandName, "lwr\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LWU:
        sprintf(CommandName, "lwu\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SB:
        sprintf(CommandName, "sb\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SH:
        sprintf(CommandName, "sh\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SWL:
        sprintf(CommandName, "swl\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SW:
        sprintf(CommandName, "sw\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SDL:
        sprintf(CommandName, "sdl\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SDR:
        sprintf(CommandName, "sdr\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SWR:
        sprintf(CommandName, "swr\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_CACHE:
        sprintf(CommandName, "cache\t%d, 0x%X (%s)", command.rt, command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LL:
        sprintf(CommandName, "ll\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LWC1:
        sprintf(CommandName, "lwc1\t%s, 0x%X (%s)", CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LDC1:
        sprintf(CommandName, "ldc1\t%s, 0x%X (%s)", CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_LD:
        sprintf(CommandName, "ld\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SC:
        sprintf(CommandName, "sc\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SWC1:
        sprintf(CommandName, "swc1\t%s, 0x%X (%s)", CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SDC1:
        sprintf(CommandName, "sdc1\t%s, 0x%X (%s)", CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    case R4300i_SD:
        sprintf(CommandName, "sd\t%s, 0x%X (%s)", CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
        break;
    default:
        sprintf(CommandName, "Unknown\t%02X %02X %02X %02X",
            command.Ascii[3], command.Ascii[2], command.Ascii[1], command.Ascii[0]);
    }

    return CommandName;
}