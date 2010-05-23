/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */
#include <STDIO.H>
#include "main.h"
#include "CPU.h"
#include "debugger.h"

BOOL InR4300iCommandsWindow = FALSE;
char CommandName[100];

void SetR4300iCommandToStepping ( void ) { 
}

void SetR4300iCommandViewto ( UINT NewLocation ) {
}

void __cdecl Enter_R4300i_Commands_Window ( void ) {
}

char strLabelName[100];

char * LabelName (DWORD Address) {
	sprintf(strLabelName,"0x%08X",Address);
	return strLabelName;
}

char * R4300iSpecialName ( DWORD OpCode, DWORD PC ) {
	OPCODE command;
	command.Hex = OpCode;

	switch (command.funct) {
	case R4300i_SPECIAL_SLL:
		if (command.Hex != 0) {
			sprintf(CommandName,"sll\t%s, %s, 0x%X",CRegName::GPR[command.rd],
			CRegName::GPR[command.rt], command.sa);
		} else {
			sprintf(CommandName,"nop");
		}
		break;
	case R4300i_SPECIAL_SRL:
		sprintf(CommandName,"srl\t%s, %s, 0x%X",CRegName::GPR[command.rd], CRegName::GPR[command.rt],
			command.sa);
		break;
	case R4300i_SPECIAL_SRA:
		sprintf(CommandName,"sra\t%s, %s, 0x%X",CRegName::GPR[command.rd], CRegName::GPR[command.rt],
				command.sa);
		break;
	case R4300i_SPECIAL_SLLV:
		sprintf(CommandName,"sllv\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rt],
			CRegName::GPR[command.rs]);
		break;
	case R4300i_SPECIAL_SRLV:
		sprintf(CommandName,"srlv\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rt],
			CRegName::GPR[command.rs]);
		break;
	case R4300i_SPECIAL_SRAV:
		sprintf(CommandName,"srav\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rt],
			CRegName::GPR[command.rs]);
		break;
	case R4300i_SPECIAL_JR:
		sprintf(CommandName,"jr\t%s",CRegName::GPR[command.rs]);
		break;
	case R4300i_SPECIAL_JALR:
		sprintf(CommandName,"jalr\t%s, %s",CRegName::GPR[command.rd],CRegName::GPR[command.rs]);
		break;
	case R4300i_SPECIAL_SYSCALL:
		sprintf(CommandName,"system call");
		break;
	case R4300i_SPECIAL_BREAK:
		sprintf(CommandName,"break");
		break;
	case R4300i_SPECIAL_SYNC:
		sprintf(CommandName,"sync");
		break;
	case R4300i_SPECIAL_MFHI:
		sprintf(CommandName,"mfhi\t%s",CRegName::GPR[command.rd]);
		break;
	case R4300i_SPECIAL_MTHI:
		sprintf(CommandName,"mthi\t%s",CRegName::GPR[command.rs]);
		break;
	case R4300i_SPECIAL_MFLO:
		sprintf(CommandName,"mflo\t%s",CRegName::GPR[command.rd]);
		break;
	case R4300i_SPECIAL_MTLO:
		sprintf(CommandName,"mtlo\t%s",CRegName::GPR[command.rs]);
		break;
	case R4300i_SPECIAL_DSLLV:
		sprintf(CommandName,"dsllv\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rt],
			CRegName::GPR[command.rs]);
		break;
	case R4300i_SPECIAL_DSRLV:
		sprintf(CommandName,"dsrlv\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rt],
			CRegName::GPR[command.rs]);
		break;
	case R4300i_SPECIAL_DSRAV:
		sprintf(CommandName,"dsrav\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rt],
			CRegName::GPR[command.rs]);
		break;
	case R4300i_SPECIAL_MULT:
		sprintf(CommandName,"mult\t%s, %s",CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_MULTU:
		sprintf(CommandName,"multu\t%s, %s",CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DIV:
		sprintf(CommandName,"div\t%s, %s",CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DIVU:
		sprintf(CommandName,"divu\t%s, %s",CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DMULT:
		sprintf(CommandName,"dmult\t%s, %s",CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DMULTU:
		sprintf(CommandName,"dmultu\t%s, %s",CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DDIV:
		sprintf(CommandName,"ddiv\t%s, %s",CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DDIVU:
		sprintf(CommandName,"ddivu\t%s, %s",CRegName::GPR[command.rs], CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_ADD:
		sprintf(CommandName,"add\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_ADDU:
		sprintf(CommandName,"addu\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_SUB:
		sprintf(CommandName,"sub\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_SUBU:
		sprintf(CommandName,"subu\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_AND:
		sprintf(CommandName,"and\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_OR:
		sprintf(CommandName,"or\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_XOR:
		sprintf(CommandName,"xor\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_NOR:
		sprintf(CommandName,"nor\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_SLT:
		sprintf(CommandName,"slt\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_SLTU:
		sprintf(CommandName,"sltu\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DADD:
		sprintf(CommandName,"dadd\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DADDU:
		sprintf(CommandName,"daddu\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DSUB:
		sprintf(CommandName,"dsub\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DSUBU:
		sprintf(CommandName,"dsubu\t%s, %s, %s",CRegName::GPR[command.rd], CRegName::GPR[command.rs],
			CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_TGE:
		sprintf(CommandName,"tge\t%s, %s",CRegName::GPR[command.rs],CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_TGEU:
		sprintf(CommandName,"tgeu\t%s, %s",CRegName::GPR[command.rs],CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_TLT:
		sprintf(CommandName,"tlt\t%s, %s",CRegName::GPR[command.rs],CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_TLTU:
		sprintf(CommandName,"tltu\t%s, %s",CRegName::GPR[command.rs],CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_TEQ:
		sprintf(CommandName,"teq\t%s, %s",CRegName::GPR[command.rs],CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_TNE:
		sprintf(CommandName,"tne\t%s, %s",CRegName::GPR[command.rs],CRegName::GPR[command.rt]);
		break;
	case R4300i_SPECIAL_DSLL:
		sprintf(CommandName,"dsll\t%s, %s, 0x%X",CRegName::GPR[command.rd],
			CRegName::GPR[command.rt], command.sa);
		break;
	case R4300i_SPECIAL_DSRL:
		sprintf(CommandName,"dsrl\t%s, %s, 0x%X",CRegName::GPR[command.rd], CRegName::GPR[command.rt],
			command.sa);
		break;
	case R4300i_SPECIAL_DSRA:
		sprintf(CommandName,"dsra\t%s, %s, 0x%X",CRegName::GPR[command.rd], CRegName::GPR[command.rt],
			command.sa);
		break;
	case R4300i_SPECIAL_DSLL32:
		sprintf(CommandName,"dsll32\t%s, %s, 0x%X",CRegName::GPR[command.rd],CRegName::GPR[command.rt], command.sa);
		break;
	case R4300i_SPECIAL_DSRL32:
		sprintf(CommandName,"dsrl32\t%s, %s, 0x%X",CRegName::GPR[command.rd], CRegName::GPR[command.rt], command.sa);
		break;
	case R4300i_SPECIAL_DSRA32:
		sprintf(CommandName,"dsra32\t%s, %s, 0x%X",CRegName::GPR[command.rd], CRegName::GPR[command.rt], command.sa);
		break;
	default:	
		sprintf(CommandName,"Unknown\t%02X %02X %02X %02X",
			command.Ascii[3],command.Ascii[2],command.Ascii[1],command.Ascii[0]);
	}
	return CommandName;
}

char * R4300iRegImmName ( DWORD OpCode, DWORD PC ) {
	OPCODE command;
	command.Hex = OpCode;

	switch (command.rt) {
	case R4300i_REGIMM_BLTZ:
		sprintf(CommandName,"bltz\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		break;
	case R4300i_REGIMM_BGEZ:
		if (command.rs == 0) {
			sprintf(CommandName,"b\t%s", LabelName(PC + ((short)command.offset << 2) + 4));
		} else {
			sprintf(CommandName,"bgez\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		}
		break;
	case R4300i_REGIMM_BLTZL:
		sprintf(CommandName,"bltzl\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		break;
	case R4300i_REGIMM_BGEZL:
		sprintf(CommandName,"bgezl\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		break;
	case R4300i_REGIMM_TGEI:
		sprintf(CommandName,"tgei\t%s, 0x%X",CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_REGIMM_TGEIU:
		sprintf(CommandName,"tgeiu\t%s, 0x%X",CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_REGIMM_TLTI:
		sprintf(CommandName,"tlti\t%s, 0x%X",CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_REGIMM_TLTIU:
		sprintf(CommandName,"tltiu\t%s, 0x%X",CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_REGIMM_TEQI:
		sprintf(CommandName,"teqi\t%s, 0x%X",CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_REGIMM_TNEI:
		sprintf(CommandName,"tnei\t%s, 0x%X",CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_REGIMM_BLTZAL:
		sprintf(CommandName,"bltzal\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		break;
	case R4300i_REGIMM_BGEZAL:
		if (command.rs == 0) {
			sprintf(CommandName,"bal\t%s",LabelName(PC + ((short)command.offset << 2) + 4));
		} else {
			sprintf(CommandName,"bgezal\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		}
		break;
	case R4300i_REGIMM_BLTZALL:
		sprintf(CommandName,"bltzall\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		break;
	case R4300i_REGIMM_BGEZALL:
		sprintf(CommandName,"bgezall\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		break;
	default:	
		sprintf(CommandName,"Unknown\t%02X %02X %02X %02X",
			command.Ascii[3],command.Ascii[2],command.Ascii[1],command.Ascii[0]);
	}
	return CommandName;
}

char * R4300iCop1Name ( DWORD OpCode, DWORD PC ) {
	OPCODE command;
	command.Hex = OpCode;

	switch (command.fmt) {
	case R4300i_COP1_MF:
		sprintf(CommandName,"mfc1\t%s, %s",CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
		break;
	case R4300i_COP1_DMF:
		sprintf(CommandName,"dmfc1\t%s, %s",CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
		break;
	case R4300i_COP1_CF:
		sprintf(CommandName,"cfc1\t%s, %s",CRegName::GPR[command.rt], CRegName::FPR_Ctrl[command.fs]);
		break;
	case R4300i_COP1_MT:
		sprintf(CommandName,"mtc1\t%s, %s",CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
		break;
	case R4300i_COP1_DMT:
		sprintf(CommandName,"dmtc1\t%s, %s",CRegName::GPR[command.rt], CRegName::FPR[command.fs]);
		break;
	case R4300i_COP1_CT:
		sprintf(CommandName,"ctc1\t%s, %s",CRegName::GPR[command.rt], CRegName::FPR_Ctrl[command.fs]);
		break;
	case R4300i_COP1_BC:
		switch (command.ft) {
		case R4300i_COP1_BC_BCF:
			sprintf(CommandName,"BC1F\t%s", LabelName(PC + ((short)command.offset << 2) + 4));
			break;
		case R4300i_COP1_BC_BCT:
			sprintf(CommandName,"BC1T\t%s", LabelName(PC + ((short)command.offset << 2) + 4));
			break;
		case R4300i_COP1_BC_BCFL:
			sprintf(CommandName,"BC1FL\t%s", LabelName(PC + ((short)command.offset << 2) + 4));
			break;
		case R4300i_COP1_BC_BCTL:
			sprintf(CommandName,"BC1TL\t%s", LabelName(PC + ((short)command.offset << 2) + 4));
			break;
		default:
			sprintf(CommandName,"Unknown Cop1\t%02X %02X %02X %02X",
				command.Ascii[3],command.Ascii[2],command.Ascii[1],command.Ascii[0]);
		}
		break;
	case R4300i_COP1_S:
	case R4300i_COP1_D:
	case R4300i_COP1_W:
	case R4300i_COP1_L:
		switch (command.funct) {			
		case R4300i_COP1_FUNCT_ADD:
			sprintf(CommandName,"ADD.%s\t%s, %s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs], 
				CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_SUB:
			sprintf(CommandName,"SUB.%s\t%s, %s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs], 
				CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_MUL:
			sprintf(CommandName,"MUL.%s\t%s, %s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs], 
				CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_DIV:
			sprintf(CommandName,"DIV.%s\t%s, %s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs], 
				CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_SQRT:
			sprintf(CommandName,"SQRT.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_ABS:
			sprintf(CommandName,"ABS.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_MOV:
			sprintf(CommandName,"MOV.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_NEG:
			sprintf(CommandName,"NEG.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_ROUND_L:
			sprintf(CommandName,"ROUND.L.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_TRUNC_L:
			sprintf(CommandName,"TRUNC.L.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_CEIL_L:
			sprintf(CommandName,"CEIL.L.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_FLOOR_L:
			sprintf(CommandName,"FLOOR.L.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_ROUND_W:
			sprintf(CommandName,"ROUND.W.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_TRUNC_W:
			sprintf(CommandName,"TRUNC.W.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_CEIL_W:
			sprintf(CommandName,"CEIL.W.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_FLOOR_W:
			sprintf(CommandName,"FLOOR.W.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_CVT_S:
			sprintf(CommandName,"CVT.S.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_CVT_D:
			sprintf(CommandName,"CVT.D.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_CVT_W:
			sprintf(CommandName,"CVT.W.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_CVT_L:
			sprintf(CommandName,"CVT.L.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fd], CRegName::FPR[command.fs]);
			break;
		case R4300i_COP1_FUNCT_C_F:
			sprintf(CommandName,"C.F.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_UN:
			sprintf(CommandName,"C.UN.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_EQ:
			sprintf(CommandName,"C.EQ.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_UEQ:
			sprintf(CommandName,"C.UEQ.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_OLT:
			sprintf(CommandName,"C.OLT.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_ULT:
			sprintf(CommandName,"C.ULT.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_OLE:
			sprintf(CommandName,"C.OLE.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_ULE:
			sprintf(CommandName,"C.ULE.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_SF:
			sprintf(CommandName,"C.SF.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_NGLE:
			sprintf(CommandName,"C.NGLE.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_SEQ:
			sprintf(CommandName,"C.SEQ.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_NGL:
			sprintf(CommandName,"C.NGL.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_LT:
			sprintf(CommandName,"C.LT.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_NGE:
			sprintf(CommandName,"C.NGE.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_LE:
			sprintf(CommandName,"C.LE.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		case R4300i_COP1_FUNCT_C_NGT:
			sprintf(CommandName,"C.NGT.%s\t%s, %s",FPR_Type(command.fmt),  
				CRegName::FPR[command.fs], CRegName::FPR[command.ft]);
			break;
		default:
			sprintf(CommandName,"Unknown Cop1\t%02X %02X %02X %02X",
				command.Ascii[3],command.Ascii[2],command.Ascii[1],command.Ascii[0]);
		}
		break;
	default:
		sprintf(CommandName,"Unknown Cop1\t%02X %02X %02X %02X",
			command.Ascii[3],command.Ascii[2],command.Ascii[1],command.Ascii[0]);
	}
	return CommandName;
}

char * R4300iOpcodeName ( DWORD OpCode, DWORD PC ) {
	OPCODE command;
	command.Hex = OpCode;
		
	switch (command.op) {
	case R4300i_SPECIAL:
		return R4300iSpecialName ( OpCode, PC );
		break;
	case R4300i_REGIMM:
		return R4300iRegImmName ( OpCode, PC );
		break;
	case R4300i_J:
		sprintf(CommandName,"j\t%s",LabelName((PC & 0xF0000000) + (command.target << 2)));
		break;
	case R4300i_JAL:
		sprintf(CommandName,"jal\t%s",LabelName((PC & 0xF0000000) + (command.target << 2)));
		break;
	case R4300i_BEQ:
		if (command.rs == 0 && command.rt == 0) {
			sprintf(CommandName,"b\t%s", LabelName(PC + ((short)command.offset << 2) + 4));
		} else if (command.rs == 0 || command.rt == 0) {
			sprintf(CommandName,"beqz\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs ],
				LabelName(PC + ((short)command.offset << 2) + 4));
		} else {
			sprintf(CommandName,"beq\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
				LabelName(PC + ((short)command.offset << 2) + 4));
		}
		break;
	case R4300i_BNE:
		if ((command.rs == 0) ^ (command.rt == 0)){
			sprintf(CommandName,"bnez\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs ],
				LabelName(PC + ((short)command.offset << 2) + 4));
		} else {
			sprintf(CommandName,"bne\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
				LabelName(PC + ((short)command.offset << 2) + 4));
		}
		break;
	case R4300i_BLEZ:
		sprintf(CommandName,"blez\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		break;
	case R4300i_BGTZ:
		sprintf(CommandName,"bgtz\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		break;
	case R4300i_ADDI:
		sprintf(CommandName,"addi\t%s, %s, 0x%X",CRegName::GPR[command.rt], CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_ADDIU:
		sprintf(CommandName,"addiu\t%s, %s, 0x%X",CRegName::GPR[command.rt], CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_SLTI:
		sprintf(CommandName,"slti\t%s, %s, 0x%X",CRegName::GPR[command.rt], CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_SLTIU:
		sprintf(CommandName,"sltiu\t%s, %s, 0x%X",CRegName::GPR[command.rt], CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_ANDI:
		sprintf(CommandName,"andi\t%s, %s, 0x%X",CRegName::GPR[command.rt], CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_ORI:
		sprintf(CommandName,"ori\t%s, %s, 0x%X",CRegName::GPR[command.rt], CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_XORI:
		sprintf(CommandName,"xori\t%s, %s, 0x%X",CRegName::GPR[command.rt], CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_LUI:
		sprintf(CommandName,"lui\t%s, 0x%X",CRegName::GPR[command.rt], command.immediate);
		break;
	case R4300i_CP0:
		switch (command.rs) {
		case R4300i_COP0_MF:
			sprintf(CommandName,"mfc0\t%s, %s",CRegName::GPR[command.rt], CRegName::Cop0[command.rd]);
			break;
		case R4300i_COP0_MT:
			sprintf(CommandName,"mtc0\t%s, %s",CRegName::GPR[command.rt], CRegName::Cop0[command.rd]);
			break;
		default:
			if ( (command.rs & 0x10 ) != 0 ) {
				switch( command.funct ) {
				case R4300i_COP0_CO_TLBR:  sprintf(CommandName,"tlbr"); break;
				case R4300i_COP0_CO_TLBWI: sprintf(CommandName,"tlbwi"); break;
				case R4300i_COP0_CO_TLBWR: sprintf(CommandName,"tlbwr"); break;
				case R4300i_COP0_CO_TLBP:  sprintf(CommandName,"tlbp"); break;
				case R4300i_COP0_CO_ERET:  sprintf(CommandName,"eret"); break;
				default:	
					sprintf(CommandName,"Unknown\t%02X %02X %02X %02X",
						command.Ascii[3],command.Ascii[2],command.Ascii[1],command.Ascii[0]);
				}
			} else {
				sprintf(CommandName,"Unknown\t%02X %02X %02X %02X",
				command.Ascii[3],command.Ascii[2],command.Ascii[1],command.Ascii[0]);
			}
			break;
		}
		break;
	case R4300i_CP1:
		return R4300iCop1Name ( OpCode, PC );
	case R4300i_BEQL:
		if (command.rs == command.rt) {
			sprintf(CommandName,"b\t%s", LabelName(PC + ((short)command.offset << 2) + 4));
		} else if ((command.rs == 0) ^ (command.rt == 0)){
			sprintf(CommandName,"beqzl\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs ],
				LabelName(PC + ((short)command.offset << 2) + 4));
		} else {
			sprintf(CommandName,"beql\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
				LabelName(PC + ((short)command.offset << 2) + 4));
		}
		break;
	case R4300i_BNEL:
		if ((command.rs == 0) ^ (command.rt == 0)){
			sprintf(CommandName,"bnezl\t%s, %s", CRegName::GPR[command.rs == 0 ? command.rt : command.rs ],
				LabelName(PC + ((short)command.offset << 2) + 4));
		} else {
			sprintf(CommandName,"bnel\t%s, %s, %s", CRegName::GPR[command.rs], CRegName::GPR[command.rt],
				LabelName(PC + ((short)command.offset << 2) + 4));
		}
		break;
	case R4300i_BLEZL:
		sprintf(CommandName,"blezl\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		break;
	case R4300i_BGTZL:
		sprintf(CommandName,"bgtzl\t%s, %s",CRegName::GPR[command.rs], LabelName(PC + ((short)command.offset << 2) + 4));
		break;
	case R4300i_DADDI:
		sprintf(CommandName,"daddi\t%s, %s, 0x%X",CRegName::GPR[command.rt], CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_DADDIU:
		sprintf(CommandName,"daddiu\t%s, %s, 0x%X",CRegName::GPR[command.rt], CRegName::GPR[command.rs],command.immediate);
		break;
	case R4300i_LDL:
		sprintf(CommandName,"ldl\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LDR:
		sprintf(CommandName,"ldr\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LB:
		sprintf(CommandName,"lb\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LH:
		sprintf(CommandName,"lh\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LWL:
		sprintf(CommandName,"lwl\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LW:
		sprintf(CommandName,"lw\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LBU:
		sprintf(CommandName,"lbu\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LHU:
		sprintf(CommandName,"lhu\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LWR:
		sprintf(CommandName,"lwr\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LWU:
		sprintf(CommandName,"lwu\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SB:
		sprintf(CommandName,"sb\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SH:
		sprintf(CommandName,"sh\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SWL:
		sprintf(CommandName,"swl\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SW:
		sprintf(CommandName,"sw\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SDL:
		sprintf(CommandName,"sdl\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SDR:
		sprintf(CommandName,"sdr\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SWR:
		sprintf(CommandName,"swr\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_CACHE:
		sprintf(CommandName,"cache\t%d, 0x%X (%s)",command.rt, command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LL:
		sprintf(CommandName,"ll\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LWC1:
		sprintf(CommandName,"lwc1\t%s, 0x%X (%s)",CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LDC1:
		sprintf(CommandName,"ldc1\t%s, 0x%X (%s)",CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_LD:
		sprintf(CommandName,"ld\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SC:
		sprintf(CommandName,"sc\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SWC1:
		sprintf(CommandName,"swc1\t%s, 0x%X (%s)",CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SDC1:
		sprintf(CommandName,"sdc1\t%s, 0x%X (%s)",CRegName::FPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	case R4300i_SD:
		sprintf(CommandName,"sd\t%s, 0x%X (%s)",CRegName::GPR[command.rt], command.offset, CRegName::GPR[command.base]);
		break;
	default:	
		sprintf(CommandName,"Unknown\t%02X %02X %02X %02X",
			command.Ascii[3],command.Ascii[2],command.Ascii[1],command.Ascii[0]);
	}

	return CommandName;
}


#ifdef OLD_CODE
#include <Windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "main.h"
#include "CPU.h"
#include "debugger.h"

#if (!defined(EXTERNAL_RELEASE))
#define R4300i_MaxCommandLines		30

typedef struct {
	DWORD Location;
	char  String[150];
    DWORD status;
} R4300ICOMMANDLINE;

#define R4300i_Status_PC            1
#define R4300i_Status_BP            2

#define IDC_LIST					1000
#define IDC_ADDRESS					1001
#define IDCfunctION_COMBO			1002
#define IDC_GO_BUTTON				1003
#define IDC_BREAK_BUTTON			1004
#define IDC_STEP_BUTTON				1005
#define IDC_SKIP_BUTTON				1006
#define IDC_BP_BUTTON				1007
#define IDC_R4300I_REGISTERS_BUTTON	1008
#define IDCrsP_DEBUGGER_BUTTON		1009
#define IDCrsP_REGISTERS_BUTTON	1010
#define IDC_MEMORY_BUTTON			1011
#define IDC_SCRL_BAR				1012

void Paint_R4300i_Commands ( HWND hDlg );
void R4300i_Commands_Setup ( HWND hDlg );
void RefreshR4300iCommands ( void );

LRESULT CALLBACK R4300i_Commands_Proc ( HWND, UINT, WPARAM, LPARAM );

static HWND R4300i_Commands_hDlg, hList, hAddress, hFunctionlist, hGoButton, hBreakButton,
	hStepButton, hSkipButton, hBPButton, hR4300iRegisters, hRSPDebugger, hRSPRegisters,
	hMemory, hScrlBar;
static R4300ICOMMANDLINE r4300iCommandLine[30];
BOOL InR4300iCommandsWindow = FALSE;

void Create_R4300i_Commands_Window ( int Child ) {
	DWORD ThreadID;
	if ( Child ) {
		InR4300iCommandsWindow = TRUE;
		DialogBox( GetModuleHandle(NULL), "BLANK", NULL,(DLGPROC) R4300i_Commands_Proc );
		InR4300iCommandsWindow = FALSE;
		memset(r4300iCommandLine,0,sizeof(r4300iCommandLine));
		SetR4300iCommandToRunning();
	} else {
		if (!InR4300iCommandsWindow) {
			SetCoreToStepping();
			CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Create_R4300i_Commands_Window,
				(LPVOID)TRUE,0, &ThreadID);	
		} else {
			SetForegroundWindow(R4300i_Commands_hDlg);
		}	
	}
}

void Disable_R4300i_Commands_Window ( void ) {
	SCROLLINFO si;

	EnableWindow(hList,            FALSE);
	EnableWindow(hAddress,         FALSE);
	EnableWindow(hScrlBar,         FALSE);
	EnableWindow(hGoButton,        FALSE);
	EnableWindow(hStepButton,      FALSE);
	EnableWindow(hSkipButton,      FALSE);
	EnableWindow(hR4300iRegisters, FALSE);
	EnableWindow(hRSPRegisters,    FALSE);
	EnableWindow(hRSPDebugger,     FALSE);
	EnableWindow(hMemory,          FALSE);
	
	si.cbSize = sizeof(si);
	si.fMask  = SIF_RANGE | SIF_POS | SIF_PAGE;
	si.nMin   = 0;
	si.nMax   = 0;
	si.nPos   = 1;
	si.nPage  = 1;
	SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);
}

int DisplayR4300iCommand (DWORD location, int InsertPos) {
	DWORD OpCode, count, LinesUsed = 1, status;
	BOOL Redraw = FALSE;	

	for (count = 0; count < NoOfMapEntries; count ++ ) {
		if (MapTable[count].VAddr == location) {
			
			if (strcmp(r4300iCommandLine[InsertPos].String, MapTable[count].Label) !=0 ) {
				Redraw = TRUE;
			}
			
			if (Redraw) {
				r4300iCommandLine[InsertPos].Location = -1;
				r4300iCommandLine[InsertPos].status = 0;
				sprintf(r4300iCommandLine[InsertPos].String," %s:",MapTable[count].Label);
				if ( SendMessage(hList,LB_GETCOUNT,0,0) <= InsertPos) {
					SendMessage(hList,LB_INSERTSTRING,(WPARAM)InsertPos, (LPARAM)location); 
				} else {
					RECT ItemRC;
					SendMessage(hList,LB_GETITEMRECT,(WPARAM)InsertPos, (LPARAM)&ItemRC); 
					RedrawWindow(hList,&ItemRC,NULL, RDW_INVALIDATE );
				}
			}
			InsertPos += 1;
			if (InsertPos >= R4300i_MaxCommandLines) {
				return LinesUsed;
			}
			LinesUsed = 2;
			count = NoOfMapEntries;
		}
	}
	

	Redraw = FALSE;
	__try {
		if (!r4300i_LW_VAddr(location, &OpCode)) {
			r4300iCommandLine[InsertPos].Location = location;
			r4300iCommandLine[InsertPos].status = 0;
			sprintf(r4300iCommandLine[InsertPos].String," 0x%08X\tCould not resolve address",location);
			if ( SendMessage(hList,LB_GETCOUNT,0,0) <= InsertPos) {
				SendMessage(hList,LB_INSERTSTRING,(WPARAM)InsertPos, (LPARAM)location); 
			} else {
				RECT ItemRC;
				SendMessage(hList,LB_GETITEMRECT,(WPARAM)InsertPos, (LPARAM)&ItemRC); 
				RedrawWindow(hList,&ItemRC,NULL, RDW_INVALIDATE );
			}
			return LinesUsed;
		}
	} __except( r4300i_Command_MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		DisplayError(GS(MSG_UNKNOWN_MEM_ACTION));
		ExitThread(0);
	}					
	if (SelfModCheck == ModCode_ChangeMemory) {
		if ( (OpCode >> 16) == 0x7C7C) {
			OpCode = OrigMem[(OpCode & 0xFFFF)].OriginalValue;
		}
	}
	
	status = 0;
	if (location == PROGRAM_COUNTER) {status = R4300i_Status_PC; }
	if (CheckForR4300iBPoint(location)) { status |= R4300i_Status_BP; }
	if (r4300iCommandLine[InsertPos].Location != location) { Redraw = TRUE; }
	if (r4300iCommandLine[InsertPos].status != status) { Redraw = TRUE; }
	if (Redraw) {
		r4300iCommandLine[InsertPos].Location = location;
		r4300iCommandLine[InsertPos].status = status;
		sprintf(r4300iCommandLine[InsertPos].String," 0x%08X\t%s",location, 
			R4300iOpcodeName ( OpCode, location ));
		if ( SendMessage(hList,LB_GETCOUNT,0,0) <= InsertPos) {
			SendMessage(hList,LB_INSERTSTRING,(WPARAM)InsertPos, (LPARAM)location); 
		} else {
			RECT ItemRC;
			SendMessage(hList,LB_GETITEMRECT,(WPARAM)InsertPos, (LPARAM)&ItemRC); 
			RedrawWindow(hList,&ItemRC,NULL, RDW_INVALIDATE );
		}
	}
	return LinesUsed;
}

void DrawR4300iCommand ( LPARAM lParam ) {	
	char Command[150], Offset[30], Instruction[30], Arguments[40];
	LPDRAWITEMSTRUCT ditem;
	COLORREF oldColor;
	int ResetColor;
	HBRUSH hBrush;
	RECT TextRect;
	char *p1, *p2;

	ditem  = (LPDRAWITEMSTRUCT)lParam;
	strcpy(Command, r4300iCommandLine[ditem->itemID].String);
	
	if (strchr(Command,'\t')) {
		p1 = strchr(Command,'\t');
		sprintf(Offset,"%.*s",p1 - Command, Command);
		p1++;
		if (strchr(p1,'\t')) {
			p2 = strchr(p1,'\t');
			sprintf(Instruction,"%.*s",p2 - p1, p1);
			sprintf(Arguments,"%s",p2 + 1);
		} else {
			sprintf(Instruction,"%s",p1);
			sprintf(Arguments,"\0");
		}
		sprintf(Command,"\0");
	} else {
		sprintf(Offset,"\0");
		sprintf(Instruction,"\0");
		sprintf(Arguments,"\0");
	}
		
	if (PROGRAM_COUNTER == r4300iCommandLine[ditem->itemID].Location) {
		ResetColor = TRUE;
		hBrush     = (HBRUSH)(COLOR_HIGHLIGHT + 1);
		oldColor   = SetTextColor(ditem->hDC,RGB(255,255,255));
	} else {
		ResetColor = FALSE;
		hBrush     = (HBRUSH)GetStockObject(WHITE_BRUSH);
	}

	if (CheckForR4300iBPoint( r4300iCommandLine[ditem->itemID].Location )) {
		ResetColor = TRUE;
		if (PROGRAM_COUNTER == r4300iCommandLine[ditem->itemID].Location) {
			SetTextColor(ditem->hDC,RGB(255,0,0));
		} else {
			oldColor = SetTextColor(ditem->hDC,RGB(255,0,0));
		}
	}

	FillRect( ditem->hDC, &ditem->rcItem,hBrush);	
	SetBkMode( ditem->hDC, TRANSPARENT );

	if (strlen (Command) == 0 ) {
		SetRect(&TextRect,ditem->rcItem.left,ditem->rcItem.top, ditem->rcItem.left + 83,
			ditem->rcItem.bottom);	
		DrawText(ditem->hDC,Offset,strlen(Offset), &TextRect,DT_SINGLELINE | DT_VCENTER);
		
		SetRect(&TextRect,ditem->rcItem.left + 83,ditem->rcItem.top, ditem->rcItem.left + 165,
			ditem->rcItem.bottom);	
		DrawText(ditem->hDC,Instruction,strlen(Instruction), &TextRect,DT_SINGLELINE | DT_VCENTER);

		SetRect(&TextRect,ditem->rcItem.left + 165,ditem->rcItem.top, ditem->rcItem.right,
			ditem->rcItem.bottom);	
		DrawText(ditem->hDC,Arguments,strlen(Arguments), &TextRect,DT_SINGLELINE | DT_VCENTER);
	} else {
		DrawText(ditem->hDC,Command,strlen(Command), &ditem->rcItem,DT_SINGLELINE | DT_VCENTER);
	}

	if (ResetColor == TRUE) {
		SetTextColor( ditem->hDC, oldColor );
	}

}

void Enable_R4300i_Commands_Window ( void ) {
	SCROLLINFO si;
	char Location[10];

	if (!InR4300iCommandsWindow) { return; }
	EnableWindow(hList,            TRUE);
	EnableWindow(hAddress,         TRUE);
	EnableWindow(hScrlBar,         TRUE);
	EnableWindow(hGoButton,        TRUE);
	EnableWindow(hStepButton,      TRUE);
	EnableWindow(hSkipButton,      FALSE);
	EnableWindow(hR4300iRegisters, TRUE);
	EnableWindow(hRSPRegisters,    FALSE);
	EnableWindow(hRSPDebugger,     FALSE);
	EnableWindow(hMemory,          TRUE);
	Update_r4300iCommandList();
	
	si.cbSize = sizeof(si);
	si.fMask  = SIF_RANGE | SIF_POS | SIF_PAGE;
	si.nMin   = 0;
	si.nMax   = 300;
	si.nPos   = 145;
	si.nPage  = 10;
	SetScrollInfo(hScrlBar,SB_CTL,&si,TRUE);		
	
	sprintf(Location,"%X",PROGRAM_COUNTER);
	SetWindowText(hAddress,Location);

	SetForegroundWindow(R4300i_Commands_hDlg);
}

void __cdecl Enter_R4300i_Commands_Window ( void ) {
	if (!HaveDebugger) { return; }
	Create_R4300i_Commands_Window ( FALSE );
}

void Paint_R4300i_Commands (HWND hDlg) {
	PAINTSTRUCT ps;
	RECT rcBox;
	HFONT hOldFont;
	int OldBkMode;

	BeginPaint( hDlg, &ps );
		
	rcBox.left   = 5;   rcBox.top    = 5;
	rcBox.right  = 343; rcBox.bottom = 463;
	DrawEdge( ps.hdc, &rcBox, EDGE_RAISED, BF_RECT );
		
	rcBox.left   = 8;   rcBox.top    = 8;
	rcBox.right  = 340; rcBox.bottom = 460;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );
		
	rcBox.left   = 347; rcBox.top    = 7;
	rcBox.right  = 446; rcBox.bottom = 42;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );
		
	rcBox.left   = 352; rcBox.top    = 2;
	rcBox.right  = 400; rcBox.bottom = 15;
	FillRect( ps.hdc, &rcBox,(HBRUSH)COLOR_WINDOW);
		
	if (NoOfMapEntries) {
		rcBox.left   = 347; rcBox.top    = 49;
		rcBox.right  = 446; rcBox.bottom = 84;
		DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED, BF_RECT );
		
		rcBox.left   = 352; rcBox.top    = 44;
		rcBox.right  = 390; rcBox.bottom = 57;
		FillRect( ps.hdc, &rcBox,(HBRUSH)COLOR_WINDOW);
	}

	rcBox.left   = 14; rcBox.top    = 14;
	rcBox.right  = 88; rcBox.bottom = 32;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED , BF_RECT );

	rcBox.left   = 86; rcBox.top    = 14;
	rcBox.right  = 173; rcBox.bottom = 32;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED , BF_RECT );

	rcBox.left   = 171; rcBox.top    = 14;
	rcBox.right  = 320; rcBox.bottom = 32;
	DrawEdge( ps.hdc, &rcBox, EDGE_ETCHED , BF_RECT );

	hOldFont = (HFONT)SelectObject( ps.hdc,GetStockObject(DEFAULT_GUI_FONT ) );
	OldBkMode = SetBkMode( ps.hdc, TRANSPARENT );
		
	TextOut( ps.hdc, 23,16,"Offset",6);
	TextOut( ps.hdc, 97,16,"Instruction",11);
	TextOut( ps.hdc, 180,16,"Arguments",9);
	TextOut( ps.hdc, 354,2," Address ",9);
	TextOut( ps.hdc, 354,19,"0x",2);
	
	if (NoOfMapEntries) {
		TextOut( ps.hdc, 354,44," goto:",6);
	}

	SelectObject( ps.hdc,hOldFont );
	SetBkMode( ps.hdc, OldBkMode );
		
	EndPaint( hDlg, &ps );
}

LRESULT CALLBACK R4300i_Commands_Proc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {	
	switch (uMsg) {
	case WM_INITDIALOG:
		R4300i_Commands_hDlg = hDlg;
		R4300i_Commands_Setup( hDlg );
		break;
	case WM_MOVE:
		StoreCurrentWinPos("R4300i Commands",hDlg);
		break;
	case WM_DRAWITEM:
		if (wParam == IDC_LIST) {
			DrawR4300iCommand (lParam);
		}
		break;
	case WM_PAINT:
		Paint_R4300i_Commands( hDlg );
		RedrawWindow(hScrlBar,NULL,NULL, RDW_INVALIDATE |RDW_ERASE);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCfunctION_COMBO:
			if (HIWORD(wParam) == CBN_SELENDOK ) {
				DWORD Selected, Location;
				char Value[20];
		
				Selected = SendMessage(hFunctionlist,CB_GETCURSEL,0,0);
				if ((int)Selected >= 0) { 
					Location = SendMessage(hFunctionlist,CB_GETITEMDATA,(WPARAM)Selected,0);
					sprintf(Value,"%08X",Location);
					SetWindowText(hAddress,Value);
				}
			}
			break;
		case IDC_LIST:
			if (HIWORD(wParam) == LBN_DBLCLK ) {
				DWORD Location, Selected;
				Selected = SendMessage(hList,LB_GETCURSEL,(WPARAM)0, (LPARAM)0); 
				Location = r4300iCommandLine[Selected].Location; 
				if (Location != (DWORD)-1) {
					if (CheckForR4300iBPoint(Location)) {
						RemoveR4300iBreakPoint(Location);
					} else {
						Add_R4300iBPoint(Location, FALSE);
					}
					RefreshR4300iCommands();
				}
			}
			break;
		case IDC_ADDRESS:
			if (HIWORD(wParam) == EN_CHANGE ) {
				RefreshR4300iCommands();
			}
			break;
		case IDC_GO_BUTTON:
			SetR4300iCommandToRunning();
			break;
		case IDC_BREAK_BUTTON:	
			SetR4300iCommandToStepping();
			break;
		case IDC_STEP_BUTTON:			
			StepOpcode();
			break;
		/*case IDC_SKIP_BUTTON:
			SkipNextR4300iOpCode = TRUE;
			WaitingForrsPStep   = FALSE;
			break;*/
		case IDC_BP_BUTTON:	Enter_BPoint_Window(); break;
		case IDC_R4300I_REGISTERS_BUTTON: Enter_R4300i_Register_Window(); break;
		case IDC_MEMORY_BUTTON: Enter_Memory_Window(); break;
		case IDCANCEL:			
			EndDialog( hDlg, IDCANCEL );
			break;
		}
		break;
	case WM_VSCROLL:
		if ((HWND)lParam == hScrlBar) {
			DWORD location;
			char Value[20];

			GetWindowText(hAddress,Value,sizeof(Value));
			location = AsciiToHex(Value) & ~3;
			
			switch (LOWORD(wParam))  {			
			case SB_LINEDOWN:
				if (location < 0xFFFFFFFC) {
					sprintf(Value,"%08X",location + 0x4);
					SetWindowText(hAddress,Value);
				} else {
					sprintf(Value,"%08X",0xFFFFFFFC);
					SetWindowText(hAddress,Value);
				}
				break;
			case SB_LINEUP:
				if (location > 0x4 ) {
					sprintf(Value,"%08X",location - 0x4);
					SetWindowText(hAddress,Value);
				} else {
					sprintf(Value,"%08X",0);
					SetWindowText(hAddress,Value);
				}
				break;
			case SB_PAGEDOWN:				
				if (location < 0xFFFFFF8C) {
					sprintf(Value,"%08X",location + 0x74);
					SetWindowText(hAddress,Value);
				} else {
					sprintf(Value,"%08X",0xFFFFFF8F);
					SetWindowText(hAddress,Value);
				}
				break;			
			case SB_PAGEUP:
				if (location > 0x74 ) {
					sprintf(Value,"%08X",location - 0x74);
					SetWindowText(hAddress,Value);
				} else {
					sprintf(Value,"%08X",0);
					SetWindowText(hAddress,Value);
				}
				break;
			}
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void R4300i_Commands_Setup ( HWND hDlg ) {
#define WindowWidth  457
#define WindowHeight 494
	DWORD X, Y;
	
	hList = CreateWindowEx(WS_EX_STATICEDGE, "LISTBOX","", WS_CHILD | WS_VISIBLE | 
		LBS_OWNERDRAWFIXED | LBS_NOTIFY,14,30,303,445, hDlg, 
		(HMENU)IDC_LIST, GetModuleHandle(NULL),NULL );
	if ( hList) {
		SendMessage(hList,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		SendMessage(hList,LB_SETITEMHEIGHT, (WPARAM)0,(LPARAM)MAKELPARAM(14, 0));
	}

	hAddress = CreateWindowEx(0,"EDIT","", WS_CHILD | ES_UPPERCASE | WS_VISIBLE | 
		WS_BORDER | WS_TABSTOP,372,17,65,18, hDlg,(HMENU)IDC_ADDRESS,GetModuleHandle(NULL), NULL );
	if (hAddress) {
		SendMessage(hAddress,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
		SendMessage(hAddress,EM_SETLIMITTEXT, (WPARAM)8,(LPARAM)0);
	} 

	hFunctionlist = CreateWindowEx(0,"COMBOBOX","", WS_CHILD | WS_VSCROLL |
		CBS_DROPDOWNLIST | CBS_SORT | WS_TABSTOP,352,56,89,150,hDlg,
		(HMENU)IDCfunctION_COMBO,GetModuleHandle(NULL),NULL);		
	if (hFunctionlist) {
		SendMessage(hFunctionlist,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	} 

	hGoButton = CreateWindowEx(WS_EX_STATICEDGE, "BUTTON","&Go", WS_CHILD | 
		BS_DEFPUSHBUTTON | WS_VISIBLE | WS_TABSTOP, 347,56,100,24, hDlg,(HMENU)IDC_GO_BUTTON,
		GetModuleHandle(NULL),NULL );
	if (hGoButton) {
		SendMessage(hGoButton,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	} 
	
	hBreakButton = CreateWindowEx(WS_EX_STATICEDGE, "BUTTON","&Break", WS_DISABLED | 
		WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,85,100,24,hDlg,
		(HMENU)IDC_BREAK_BUTTON,GetModuleHandle(NULL),NULL );
	if (hBreakButton) {
		SendMessage(hBreakButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hStepButton = CreateWindowEx(WS_EX_STATICEDGE, "BUTTON","&Step", WS_CHILD | 
		BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,114,100,24,hDlg,
		(HMENU)IDC_STEP_BUTTON,GetModuleHandle(NULL),NULL );
	if (hStepButton) {
		SendMessage(hStepButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hSkipButton = CreateWindowEx(WS_EX_STATICEDGE, "BUTTON","&Skip", WS_CHILD | 
		BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,143,100,24,hDlg,
		(HMENU)IDC_SKIP_BUTTON,GetModuleHandle(NULL),NULL );
	if (hSkipButton) {
		SendMessage(hSkipButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hBPButton = CreateWindowEx(WS_EX_STATICEDGE, "BUTTON","&Break Points", WS_CHILD | 
		BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,324,100,24,hDlg,
		(HMENU)IDC_BP_BUTTON,GetModuleHandle(NULL),NULL );
	if (hBPButton) {
		SendMessage(hBPButton,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
		
	hR4300iRegisters = CreateWindowEx(WS_EX_STATICEDGE,"BUTTON","R4300i &Registers...",
		WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,353,100,24,hDlg,
		(HMENU)IDC_R4300I_REGISTERS_BUTTON,GetModuleHandle(NULL),NULL );
	if (hR4300iRegisters) {
		SendMessage(hR4300iRegisters,WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hRSPDebugger = CreateWindowEx(WS_EX_STATICEDGE,"BUTTON", "RSP &Debugger...", 
		WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,382,100,24,hDlg,
		(HMENU)IDCrsP_DEBUGGER_BUTTON,GetModuleHandle(NULL),NULL );
	if (hRSPDebugger) {
		SendMessage(hRSPDebugger,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}

	hRSPRegisters = CreateWindowEx(WS_EX_STATICEDGE,"BUTTON", "RSP R&egisters...",
		WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,411,100,24,hDlg,
		(HMENU)IDCrsP_REGISTERS_BUTTON,GetModuleHandle(NULL),NULL );
	if (hRSPRegisters) {
		SendMessage(hRSPRegisters,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	} 

	hMemory = CreateWindowEx(WS_EX_STATICEDGE,"BUTTON", "&Memory...", WS_CHILD | 
		BS_PUSHBUTTON | WS_VISIBLE | WS_TABSTOP | BS_TEXT, 347,440,100,24,hDlg,
		(HMENU)IDC_MEMORY_BUTTON,GetModuleHandle(NULL),NULL );
	if (hMemory) {
		SendMessage(hMemory,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
	}
	
	hScrlBar = CreateWindowEx(WS_EX_STATICEDGE, "SCROLLBAR","", WS_CHILD | WS_VISIBLE | 
		WS_TABSTOP | SBS_VERT, 318,14,18,439, hDlg, (HMENU)IDC_SCRL_BAR, GetModuleHandle(NULL), NULL );

	if ( RomFileSize != 0 ) {
		Enable_R4300i_Commands_Window();
	} else {
		Disable_R4300i_Commands_Window();
	}
	
	if ( !GetStoredWinPos( "R4300i Commands", &X, &Y ) ) {
		X = (GetSystemMetrics( SM_CXSCREEN ) - WindowWidth) / 2;
		Y = (GetSystemMetrics( SM_CYSCREEN ) - WindowHeight) / 2;
	}
	SetWindowText(hDlg,"R4300i Commands");

	SetWindowPos(hDlg,NULL,X,Y,WindowWidth,WindowHeight, SWP_NOZORDER | 
		SWP_SHOWWINDOW);

}
#endif

#if (!defined(EXTERNAL_RELEASE))
void RefreshR4300iCommands ( void ) {
	DWORD location, LinesUsed;
	char AsciiAddress[20];
	int count;

	if (InR4300iCommandsWindow == FALSE) { return; }

	GetWindowText(hAddress,AsciiAddress,sizeof(AsciiAddress));
	location = AsciiToHex(AsciiAddress) & ~3;

	if (location > 0xFFFFFF88) { location = 0xFFFFFF88; }
	for (count = 0 ; count < R4300i_MaxCommandLines; count += LinesUsed ){
		LinesUsed = DisplayR4300iCommand ( location, count );
		location += 4;
	}
}

void SetR4300iCommandToRunning ( void ) { 
	if (CheckForR4300iBPoint(PROGRAM_COUNTER)) {
		StepOpcode();
	}
	SetCoreToRunning();
	if (InR4300iCommandsWindow == FALSE) { return; }
	EnableWindow(hGoButton,    FALSE);
	EnableWindow(hBreakButton, TRUE);
	EnableWindow(hStepButton,  FALSE);
	EnableWindow(hSkipButton,  FALSE);
	SendMessage(hGoButton, BM_SETSTYLE,BS_PUSHBUTTON,TRUE);
	SendMessage(hBreakButton, BM_SETSTYLE,BS_DEFPUSHBUTTON,TRUE);
	SetFocus(hBreakButton);
}

void SetR4300iCommandToStepping ( void ) { 
	EnableWindow(hGoButton,    TRUE);
	EnableWindow(hBreakButton, FALSE);
	EnableWindow(hStepButton,  TRUE);
	EnableWindow(hSkipButton,  TRUE);
	SendMessage(hBreakButton, BM_SETSTYLE, BS_PUSHBUTTON,TRUE);
	SendMessage(hStepButton, BM_SETSTYLE, BS_DEFPUSHBUTTON,TRUE);
	SetFocus(hStepButton);
	SetCoreToStepping();
}

void SetR4300iCommandViewto ( UINT NewLocation ) {
	unsigned int location;
	char Value[20];

	if (InR4300iCommandsWindow == FALSE) { return; }

	GetWindowText(hAddress,Value,sizeof(Value));
	location = AsciiToHex(Value) & ~3;

	if ( NewLocation < location || NewLocation >= location + 120 ) {
		sprintf(Value,"%08X",NewLocation);
		SetWindowText(hAddress,Value);
	} else {
		RefreshR4300iCommands();
	}
}

void Update_r4300iCommandList (void) {
	if (!InR4300iCommandsWindow) { return; }
	
	if (NoOfMapEntries == 0) {
		ShowWindow(hFunctionlist, FALSE);
		SetWindowPos(hGoButton,0,347,56,0,0, SWP_NOZORDER | SWP_NOSIZE| SWP_SHOWWINDOW);
		SetWindowPos(hBreakButton,0,347,85,0,0, SWP_NOZORDER | SWP_NOSIZE| SWP_SHOWWINDOW);
		SetWindowPos(hStepButton,0,347,114,0,0, SWP_NOZORDER | SWP_NOSIZE| SWP_SHOWWINDOW);
		SetWindowPos(hSkipButton,0,347,143,0,0, SWP_NOZORDER | SWP_NOSIZE| SWP_SHOWWINDOW);
	} else {	
		DWORD count, pos;

		ShowWindow(hFunctionlist, TRUE);
		SetWindowPos(hGoButton,0,347,86,0,0, SWP_NOZORDER | SWP_NOSIZE| SWP_SHOWWINDOW);
		SetWindowPos(hBreakButton,0,347,115,0,0, SWP_NOZORDER | SWP_NOSIZE| SWP_SHOWWINDOW);
		SetWindowPos(hStepButton,0,347,144,0,0, SWP_NOZORDER | SWP_NOSIZE| SWP_SHOWWINDOW);
		SetWindowPos(hSkipButton,0,347,173,0,0, SWP_NOZORDER | SWP_NOSIZE| SWP_SHOWWINDOW);
		
		SendMessage(hFunctionlist,CB_RESETCONTENT,(WPARAM)0,(LPARAM)0);		
		for (count = 0; count < NoOfMapEntries; count ++ ) {
			pos = SendMessage(hFunctionlist,CB_ADDSTRING,(WPARAM)0,(LPARAM)MapTable[count].Label);
			SendMessage(hFunctionlist,CB_SETITEMDATA,(WPARAM)pos,(LPARAM)MapTable[count].VAddr);
		}
		SendMessage(hFunctionlist,CB_SETCURSEL,(WPARAM)-1,(LPARAM)0);
		
		InvalidateRect( R4300i_Commands_hDlg, NULL, TRUE );
	}
}

#endif

#endif