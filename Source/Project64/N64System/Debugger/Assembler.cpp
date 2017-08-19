
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "stdafx.h"

#include "Assembler.h"
#include "Project64-core\N64System\Mips\OpCode.h"

REGISTER* lookup_register(char* name);

static ParseError parse_error = ERR_NONE;
static uint32_t m_Address = 0x00000000;

void to_lower(char* str)
{
	while (*str)
	{
		if (*str >= 'A' && *str <= 'Z')
		{
			*str |= 0x20;
		}
		str++;
	}
}

uint32_t pop_reg()
{
	char* r = strtok(NULL, " \t,()");

	if (r == NULL)
	{
		parse_error = ERR_EXPECTED_REG;
		return 0;
	}

	REGISTER* reg = lookup_register(r);

	if (reg == NULL)
	{
		parse_error = ERR_INVALID_REG;
		return 0;
	}

	return reg->val;
}

uint32_t pop_val()
{
	char* v = strtok(NULL, " \t,()");

	if (v == NULL)
	{
		parse_error = ERR_EXPECTED_VAL;
		return 0;
	}

	if (isalpha(*v))
	{
		// todo lookup label value
		return 0;
	}

	int base = 0; // hex or dec

	if (*v == '$')
	{
		base = 16; // hex
		v++;
	}

	char* endptr;

	uint32_t val = strtoul(v, &endptr, base);

	if (*endptr != '\0')
	{
		parse_error = ERR_EXPECTED_VAL;
		return 0;
	}

	return val;
}

uint32_t base_op(uint32_t val)
{
	return val << 26;
}

uint32_t base_spec(uint32_t val)
{
	return val;
}

uint32_t base_regimm(uint32_t val)
{
	return (R4300i_REGIMM << 26) | (val << 16);
}

void arg_reg_s(uint32_t* opcode)
{
	*opcode |= pop_reg() << 21;
}

void arg_reg_t(uint32_t* opcode)
{
	*opcode |= pop_reg() << 16;
}

void arg_reg_d(uint32_t* opcode)
{
	*opcode |= pop_reg() << 11;
}

void arg_jump(uint32_t* opcode)
{
	*opcode |= (pop_val() / 4) & 0x3FFFFFF;
}

void arg_imm16(uint32_t* opcode)
{
	*opcode |= (pop_val() & 0xFFFF);
}

void arg_bra_target(uint32_t* opcode)
{
	uint16_t relTarget = (((pop_val() - m_Address) / 4) & 0xFFFF) - 1;
	*opcode |= relTarget;
}

void arg_shamt(uint32_t* opcode)
{
	*opcode |= (pop_val() & 0x1F) << 6;
}

SYNTAX syn_jump[]       = { arg_jump, NULL };
SYNTAX syn_loadstore[]  = { arg_reg_t, arg_imm16, arg_reg_s, NULL };
SYNTAX syn_arith[]      = { arg_reg_d, arg_reg_s, arg_reg_t, NULL };
SYNTAX syn_arith_i[]    = { arg_reg_t, arg_reg_s, arg_imm16, NULL };
SYNTAX syn_load_i[]     = { arg_reg_t, arg_imm16, NULL };

SYNTAX syn_branch_z[]   = { arg_reg_s, arg_bra_target, NULL };
SYNTAX syn_branch[]     = { arg_reg_s, arg_reg_t, arg_bra_target, NULL };
SYNTAX syn_branch_unc[] = { arg_bra_target, NULL };

SYNTAX syn_trap_i[]     = { arg_reg_s, arg_imm16, NULL };

SYNTAX syn_shift[]      = { arg_reg_d, arg_reg_t, arg_shamt, NULL };

SYNTAX syn_mf[]         = { arg_reg_d, NULL };
SYNTAX syn_jr[]         = { arg_reg_s, NULL };

INSTRUCTION instructions[] =
{
	{ "j",      R4300i_J,     base_op, syn_jump },
	{ "jal",    R4300i_JAL,   base_op, syn_jump },
	{ "beq",    R4300i_BEQ,   base_op, syn_branch },
	{ "beqz",   R4300i_BEQ,   base_op, syn_branch_z },
	{ "b",      R4300i_BEQ,   base_op, syn_branch_unc },
	{ "bne",    R4300i_BNE,   base_op, syn_branch },
	{ "bnez",   R4300i_BNE,   base_op, syn_branch_z },
	{ "blez",   R4300i_BLEZ,  base_op, syn_branch_z },
	{ "bgtz",   R4300i_BGTZ,  base_op, syn_branch_z },
	{ "addi",   R4300i_ADDI,  base_op, syn_arith_i },
	{ "addiu",  R4300i_ADDIU, base_op, syn_arith_i },
	{ "slti",   R4300i_SLTI,  base_op, syn_arith_i },
	{ "sltiu",  R4300i_SLTIU, base_op, syn_arith_i },
	{ "andi",   R4300i_ANDI,  base_op, syn_arith_i },
	{ "ori",    R4300i_ORI,   base_op, syn_arith_i },
	{ "xori",   R4300i_XORI,  base_op, syn_arith_i },
	{ "lui",    R4300i_LUI,   base_op, syn_load_i },
	// cp0 cp1
	{ "beql",   R4300i_BEQL,  base_op, syn_branch },
	{ "beqzl",  R4300i_BEQL,  base_op, syn_branch_z },
	{ "bnel",   R4300i_BNEL,  base_op, syn_branch },
	{ "bnezl",  R4300i_BNEL,  base_op, syn_branch_z },
	{ "blezl",  R4300i_BLEZL, base_op, syn_branch_z },
	{ "bgtzl",  R4300i_BGTZL, base_op, syn_branch_z },
	{ "daddi",  R4300i_DADDI, base_op, syn_arith_i },
	{ "daddiu", R4300i_DADDI, base_op, syn_arith_i },
	{ "ldl",    R4300i_LDL,   base_op, syn_loadstore },
	{ "ldr",    R4300i_LDR,   base_op, syn_loadstore },
	{ "lb",     R4300i_LB,    base_op, syn_loadstore },
	{ "lh",     R4300i_LH,    base_op, syn_loadstore },
	{ "lwl",    R4300i_LWL,   base_op, syn_loadstore },
	{ "lw",     R4300i_LW,    base_op, syn_loadstore },
	{ "lbu",    R4300i_LBU,   base_op, syn_loadstore },
	{ "lhu",    R4300i_LHU,   base_op, syn_loadstore },
	{ "lwr",    R4300i_LWR,   base_op, syn_loadstore },
	{ "lwu",    R4300i_LWU,   base_op, syn_loadstore },
	{ "sb",     R4300i_SB,    base_op, syn_loadstore },
	{ "sh",     R4300i_SH,    base_op, syn_loadstore },
	{ "swl",    R4300i_SWL,   base_op, syn_loadstore },
	{ "sw",     R4300i_SW,    base_op, syn_loadstore },
	{ "sdl",    R4300i_SDL,   base_op, syn_loadstore },
	{ "sdr",    R4300i_SDR,   base_op, syn_loadstore },
	{ "swr",    R4300i_SWR,   base_op, syn_loadstore },
	//{ "cache",   R4300i_CACHE, base_op,  },
	{ "ll",     R4300i_LL,    base_op, syn_loadstore },
	{ "lwc1",   R4300i_LWC1,  base_op, syn_loadstore },
	{ "ldc1",   R4300i_LDC1,  base_op, syn_loadstore },
	{ "ld",     R4300i_LD,    base_op, syn_loadstore },
	{ "sc",     R4300i_SC,    base_op, syn_loadstore },
	{ "swc1",   R4300i_SWC1,  base_op, syn_loadstore },
	{ "sdc1",   R4300i_SDC1,  base_op, syn_loadstore },
	{ "sdc2",   R4300i_SDC2,  base_op, syn_loadstore },
	{ "sd",     R4300i_SD,    base_op, syn_loadstore },

	{ "bltz",    R4300i_REGIMM_BLTZ,    base_regimm, syn_branch_z },
	{ "bgez",    R4300i_REGIMM_BGEZ,    base_regimm, syn_branch_z },
	{ "bltzl",   R4300i_REGIMM_BLTZL,   base_regimm, syn_branch_z },
	{ "bgezl",   R4300i_REGIMM_BGEZL,   base_regimm, syn_branch_z },
	{ "tgei",    R4300i_REGIMM_TGEI,    base_regimm, syn_trap_i },
	{ "tgeiu",   R4300i_REGIMM_TGEIU,   base_regimm, syn_trap_i },
	{ "tlti",    R4300i_REGIMM_TLTIU,   base_regimm, syn_trap_i },
	{ "tltiu",   R4300i_REGIMM_TLTIU,   base_regimm, syn_trap_i },
	{ "teqi",    R4300i_REGIMM_TEQI,    base_regimm, syn_trap_i },
	{ "tnei",    R4300i_REGIMM_TNEI,    base_regimm, syn_trap_i },
	{ "bltzal",  R4300i_REGIMM_BLTZAL,  base_regimm, syn_branch_z },
	{ "bgezal",  R4300i_REGIMM_BGEZAL,  base_regimm, syn_branch_z },
	{ "bal",     R4300i_REGIMM_BGEZAL,  base_regimm, syn_branch_unc },
	{ "bltzall", R4300i_REGIMM_BLTZALL, base_regimm, syn_branch_z },
	{ "bgezall", R4300i_REGIMM_BGEZALL, base_regimm, syn_branch_z },

	{ "sll",     R4300i_SPECIAL_SLL, base_spec, syn_shift },
	{ "nop",     R4300i_SPECIAL_SLL, base_spec, NULL },
	{ "srl",     R4300i_SPECIAL_SRL, base_spec, syn_shift },
	{ "sra",     R4300i_SPECIAL_SRA, base_spec, syn_shift },

	{ "jr",      R4300i_SPECIAL_JR,      base_spec, syn_jr },
	{ "syscall", R4300i_SPECIAL_SYSCALL, base_spec, NULL },
	{ "break",   R4300i_SPECIAL_BREAK,   base_spec, NULL },
	{ "sync",    R4300i_SPECIAL_SYNC,    base_spec, NULL },

	{ "mfhi",   R4300i_SPECIAL_MFHI,  base_spec, syn_mf },
	{ "mthi",   R4300i_SPECIAL_MTHI,  base_spec, syn_mf },
	{ "mflo",   R4300i_SPECIAL_MFLO,  base_spec, syn_mf },
	{ "mtlo",   R4300i_SPECIAL_MTLO,  base_spec, syn_mf },

	{ "add",   	R4300i_SPECIAL_ADD,   base_spec, syn_arith },
	{ "addu",  	R4300i_SPECIAL_ADDU,  base_spec, syn_arith },
	{ "sub",   	R4300i_SPECIAL_SUB,   base_spec, syn_arith },
	{ "subu",  	R4300i_SPECIAL_SUBU,  base_spec, syn_arith },
	{ "and",   	R4300i_SPECIAL_AND,   base_spec, syn_arith },
	{ "or",    	R4300i_SPECIAL_OR,    base_spec, syn_arith },
	{ "xor",   	R4300i_SPECIAL_XOR,   base_spec, syn_arith },
	{ "nor",   	R4300i_SPECIAL_NOR,   base_spec, syn_arith },
	{ "slt",   	R4300i_SPECIAL_SLT,   base_spec, syn_arith },
	{ "sltu",  	R4300i_SPECIAL_SLTU,  base_spec, syn_arith },
	{ "dadd",  	R4300i_SPECIAL_DADD,  base_spec, syn_arith },
	{ "daddu", 	R4300i_SPECIAL_DADDU, base_spec, syn_arith },
	{ "dsub",  	R4300i_SPECIAL_DSUB,  base_spec, syn_arith },
	{ "dsubu", 	R4300i_SPECIAL_DSUBU, base_spec, syn_arith },

	{ "dsll",    R4300i_SPECIAL_DSLL, base_spec, syn_shift },
	{ "dsrl",    R4300i_SPECIAL_DSRL, base_spec, syn_shift },
	{ "dsra",    R4300i_SPECIAL_DSRA, base_spec, syn_shift },
	{ "dsll32",  R4300i_SPECIAL_DSLL32, base_spec, syn_shift },
	{ "dsrl32",  R4300i_SPECIAL_DSRL32, base_spec, syn_shift },
	{ "dsra32",  R4300i_SPECIAL_DSRA32, base_spec, syn_shift },
	
	{ 0 }
};

REGISTER registers[] =
{
	{ "r0",  0 },{ "at",  1 },{ "v0",  2 },{ "v1",  3 },{ "a0",  4 },{ "a1",  5 },{ "a2",  6 },{ "a3",  7 },
	{ "t0",  8 },{ "t1",  9 },{ "t2", 10 },{ "t3", 11 },{ "t4", 12 },{ "t5", 13 },{ "t6", 14 },{ "t7", 15 },
	{ "s0", 16 },{ "s1", 17 },{ "s2", 18 },{ "s3", 19 },{ "s4", 20 },{ "s5", 21 },{ "s6", 22 },{ "s7", 23 },
	{ "t8", 24 },{ "t9", 25 },{ "k0", 26 },{ "k1", 27 },{ "gp", 28 },{ "sp", 29 },{ "s8", 30 },{ "ra", 31 },

	{ "fp", 30 },

	{ "f0",   0 },{ "f1",   1 },{ "f2",   2 },{ "f3",   3 },{ "f4",   4 },{ "f5",   5 },{ "f6",   6 },{ "f7",   7 },
	{ "f8",   8 },{ "f9",   9 },{ "f10", 10 },{ "f11", 11 },{ "f12", 12 },{ "f13", 13 },{ "f14", 14 },{ "f15", 15 },
	{ "f16", 16 },{ "f17", 17 },{ "f18", 18 },{ "f19", 19 },{ "f20", 20 },{ "f21", 21 },{ "f22", 22 },{ "f23", 23 },
	{ "f24", 24 },{ "f25", 25 },{ "f26", 26 },{ "f27", 27 },{ "f28", 28 },{ "f29", 29 },{ "f30", 30 },{ "f31", 31 },

	{ "r0",   0 },{ "r1",   1 },{ "r2",   2 },{ "r3",   3 },{ "r4",   4 },{ "r5",   5 },{ "r6",   6 },{ "r7",   7 },
	{ "r8",   8 },{ "r9",   9 },{ "r10", 10 },{ "r11", 11 },{ "r12", 12 },{ "r13", 13 },{ "r14", 14 },{ "r15", 15 },
	{ "r16", 16 },{ "r17", 17 },{ "r18", 18 },{ "r19", 19 },{ "r20", 20 },{ "r21", 21 },{ "r22", 22 },{ "r23", 23 },
	{ "r24", 24 },{ "r25", 25 },{ "r26", 26 },{ "r27", 27 },{ "r28", 28 },{ "r29", 29 },{ "r30", 30 },{ "r31", 31 },

	{ 0 }
};

INSTRUCTION* lookup_instruction(char* name)
{
	for (int i = 0; instructions[i].name != NULL; i++)
	{
		if (strcmp(name, instructions[i].name) == 0)
		{
			return &instructions[i];
		}
	}
	return NULL;
}

REGISTER* lookup_register(char* name)
{
	for (int i = 0; registers[i].name != NULL; i++)
	{
		if (strcmp(name, registers[i].name) == 0)
		{
			return &registers[i];
		}
	}
	return NULL;
}

bool CAssembler::AssembleLine(char* line, uint32_t* opcode, uint32_t address)
{
	parse_error = ERR_NONE;

	m_Address = address;
	char line_c[128];
	strncpy(line_c, line, 128);

	to_lower(line_c);

	if (line_c[0] == '\0')
	{
		*opcode = 0;
		return true;
	}

	char* name = strtok(line_c, " \t");

	INSTRUCTION* instruction = lookup_instruction(name);

	if (instruction == NULL)
	{
		return false;
	}

	*opcode = instruction->base(instruction->val);

	if (instruction->syntax == NULL)
	{
		// No parameters
		return true;
	}

	for (int i = 0; instruction->syntax[i]; i++)
	{
		instruction->syntax[i](opcode);

		if (parse_error != ERR_NONE)
		{
			return false;
		}
	}
	return true;
}