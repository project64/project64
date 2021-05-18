#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "stdafx.h"

#include "Assembler.h"
#include "Project64-core/N64System/Mips/OpCode.h"

ASM_PARSE_ERROR CAssembler::m_ParseError = ERR_NONE;
uint32_t CAssembler::m_Address = 0;

char* CAssembler::m_TokContext = nullptr;

const ASM_SYNTAX_FN CAssembler::syn_jump[] = { arg_jump, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_loadstore[] = { arg_reg_t, arg_imm16, arg_reg_s, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_arith[] = { arg_reg_d, arg_reg_s, arg_reg_t, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_arith2[] = { arg_reg_s, arg_reg_t, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_shiftv[] = { arg_reg_d, arg_reg_t, arg_reg_s, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_arith_i[] = { arg_reg_t, arg_reg_s, arg_imm16, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_load_i[] = { arg_reg_t, arg_imm16, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_branch_z[] = { arg_reg_s, arg_bra_target, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_branch[] = { arg_reg_s, arg_reg_t, arg_bra_target, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_branch_unc[] = { arg_bra_target, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_trap_i[] = { arg_reg_s, arg_imm16, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_shift[] = { arg_reg_d, arg_reg_t, arg_shamt, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_mf[] = { arg_reg_d, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_jr[] = { arg_reg_s, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_jalr[] = { arg_reg_d, arg_reg_s, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_cop1_arith[] = { arg_reg_fd, arg_reg_fs, arg_reg_ft, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_cop1[] = { arg_reg_fd, arg_reg_fs, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_cop1_cmp[] = { arg_reg_fs, arg_reg_ft, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_cop_mv[] = { arg_reg_t, arg_reg_d, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_cache[] = { arg_cache_op, arg_imm16, arg_reg_s, nullptr };
const ASM_SYNTAX_FN CAssembler::syn_syscall[] = { arg_syscall_code, nullptr };

const ASM_INSTRUCTION CAssembler::m_Instructions[] =
{
    { "j",      R4300i_J,      base_op, syn_jump },
    { "jal",    R4300i_JAL,    base_op, syn_jump },
    { "beq",    R4300i_BEQ,    base_op, syn_branch },
    { "beqz",   R4300i_BEQ,    base_op, syn_branch_z },
    { "b",      R4300i_BEQ,    base_op, syn_branch_unc },
    { "bne",    R4300i_BNE,    base_op, syn_branch },
    { "bnez",   R4300i_BNE,    base_op, syn_branch_z },
    { "blez",   R4300i_BLEZ,   base_op, syn_branch_z },
    { "bgtz",   R4300i_BGTZ,   base_op, syn_branch_z },
    { "addi",   R4300i_ADDI,   base_op, syn_arith_i },
    { "addiu",  R4300i_ADDIU,  base_op, syn_arith_i },
    { "slti",   R4300i_SLTI,   base_op, syn_arith_i },
    { "sltiu",  R4300i_SLTIU,  base_op, syn_arith_i },
    { "andi",   R4300i_ANDI,   base_op, syn_arith_i },
    { "ori",    R4300i_ORI,    base_op, syn_arith_i },
    { "xori",   R4300i_XORI,   base_op, syn_arith_i },
    { "lui",    R4300i_LUI,    base_op, syn_load_i },
    { "beql",   R4300i_BEQL,   base_op, syn_branch },
    { "beqzl",  R4300i_BEQL,   base_op, syn_branch_z },
    { "bnel",   R4300i_BNEL,   base_op, syn_branch },
    { "bnezl",  R4300i_BNEL,   base_op, syn_branch_z },
    { "blezl",  R4300i_BLEZL,  base_op, syn_branch_z },
    { "bgtzl",  R4300i_BGTZL,  base_op, syn_branch_z },
    { "daddi",  R4300i_DADDI,  base_op, syn_arith_i },
    { "daddiu", R4300i_DADDIU, base_op, syn_arith_i },
    { "ldl",    R4300i_LDL,    base_op, syn_loadstore },
    { "ldr",    R4300i_LDR,    base_op, syn_loadstore },
    { "lb",     R4300i_LB,     base_op, syn_loadstore },
    { "lh",     R4300i_LH,     base_op, syn_loadstore },
    { "lwl",    R4300i_LWL,    base_op, syn_loadstore },
    { "lw",     R4300i_LW,     base_op, syn_loadstore },
    { "lbu",    R4300i_LBU,    base_op, syn_loadstore },
    { "lhu",    R4300i_LHU,    base_op, syn_loadstore },
    { "lwr",    R4300i_LWR,    base_op, syn_loadstore },
    { "lwu",    R4300i_LWU,    base_op, syn_loadstore },
    { "sb",     R4300i_SB,     base_op, syn_loadstore },
    { "sh",     R4300i_SH,     base_op, syn_loadstore },
    { "swl",    R4300i_SWL,    base_op, syn_loadstore },
    { "sw",     R4300i_SW,     base_op, syn_loadstore },
    { "sdl",    R4300i_SDL,    base_op, syn_loadstore },
    { "sdr",    R4300i_SDR,    base_op, syn_loadstore },
    { "swr",    R4300i_SWR,    base_op, syn_loadstore },
    { "cache",  R4300i_CACHE,  base_op, syn_cache },
    { "ll",     R4300i_LL,     base_op, syn_loadstore },
    { "lwc1",   R4300i_LWC1,   base_op, syn_loadstore },
    { "ldc1",   R4300i_LDC1,   base_op, syn_loadstore },
    { "ld",     R4300i_LD,     base_op, syn_loadstore },
    { "sc",     R4300i_SC,     base_op, syn_loadstore },
    { "swc1",   R4300i_SWC1,   base_op, syn_loadstore },
    { "sdc1",   R4300i_SDC1,   base_op, syn_loadstore },
    { "sdc2",   R4300i_SDC2,   base_op, syn_loadstore },
    { "sd",     R4300i_SD,     base_op, syn_loadstore },

    { "sll",     R4300i_SPECIAL_SLL,     base_spec, syn_shift },
    { "nop",     R4300i_SPECIAL_SLL,     base_spec, nullptr },
    { "srl",     R4300i_SPECIAL_SRL,     base_spec, syn_shift },
    { "sra",     R4300i_SPECIAL_SRA,     base_spec, syn_shift },
    { "sllv",    R4300i_SPECIAL_SLLV,    base_spec, syn_shiftv },
    { "srlv",    R4300i_SPECIAL_SRLV,    base_spec, syn_shiftv },
    { "srav",    R4300i_SPECIAL_SRAV,    base_spec, syn_shiftv },
    { "jr",      R4300i_SPECIAL_JR,      base_spec, syn_jr },
    { "jalr",    R4300i_SPECIAL_JALR,    base_spec, syn_jalr },
    { "jalr",    R4300i_SPECIAL_JALR,    base_spec_jalr_ra, syn_jr },
    { "syscall", R4300i_SPECIAL_SYSCALL, base_spec, syn_syscall },
    { "break",   R4300i_SPECIAL_BREAK,   base_spec, syn_syscall },
    { "sync",    R4300i_SPECIAL_SYNC,    base_spec, nullptr },
    { "mfhi",    R4300i_SPECIAL_MFHI,    base_spec, syn_mf },
    { "mthi",    R4300i_SPECIAL_MTHI,    base_spec, syn_mf },
    { "mflo",    R4300i_SPECIAL_MFLO,    base_spec, syn_mf },
    { "mtlo",    R4300i_SPECIAL_MTLO,    base_spec, syn_mf },
    { "dsllv",   R4300i_SPECIAL_DSLLV,   base_spec, syn_shiftv },
    { "dsrlv",   R4300i_SPECIAL_DSRLV,   base_spec, syn_shiftv },
    { "dsrav",   R4300i_SPECIAL_DSRAV,   base_spec, syn_shiftv },
    { "mult",    R4300i_SPECIAL_MULT,    base_spec, syn_arith2 },
    { "multu",   R4300i_SPECIAL_MULTU,   base_spec, syn_arith2 },
    { "div",     R4300i_SPECIAL_DIV,     base_spec, syn_arith2 },
    { "divu",    R4300i_SPECIAL_DIVU,    base_spec, syn_arith2 },
    { "dmult",   R4300i_SPECIAL_DMULT,   base_spec, syn_arith2 },
    { "dmultu",  R4300i_SPECIAL_DMULTU,  base_spec, syn_arith2 },
    { "ddiv",    R4300i_SPECIAL_DDIV,    base_spec, syn_arith2 },
    { "ddivu",   R4300i_SPECIAL_DDIVU,   base_spec, syn_arith2 },
    { "add",     R4300i_SPECIAL_ADD,     base_spec, syn_arith },
    { "addu",    R4300i_SPECIAL_ADDU,    base_spec, syn_arith },
    { "sub",     R4300i_SPECIAL_SUB,     base_spec, syn_arith },
    { "subu",    R4300i_SPECIAL_SUBU,    base_spec, syn_arith },
    { "and",     R4300i_SPECIAL_AND,     base_spec, syn_arith },
    { "or",      R4300i_SPECIAL_OR,      base_spec, syn_arith },
    { "xor",     R4300i_SPECIAL_XOR,     base_spec, syn_arith },
    { "nor",     R4300i_SPECIAL_NOR,     base_spec, syn_arith },
    { "slt",     R4300i_SPECIAL_SLT,     base_spec, syn_arith },
    { "sltu",    R4300i_SPECIAL_SLTU,    base_spec, syn_arith },
    { "dadd",    R4300i_SPECIAL_DADD,    base_spec, syn_arith },
    { "daddu",   R4300i_SPECIAL_DADDU,   base_spec, syn_arith },
    { "dsub",    R4300i_SPECIAL_DSUB,    base_spec, syn_arith },
    { "dsubu",   R4300i_SPECIAL_DSUBU,   base_spec, syn_arith },
    { "tge",     R4300i_SPECIAL_TGE,     base_spec, syn_arith2 }, // Note: no code field
    { "tgeu",    R4300i_SPECIAL_TGEU,    base_spec, syn_arith2 }, //
    { "tlt",     R4300i_SPECIAL_TLT,     base_spec, syn_arith2 }, //
    { "tltu",    R4300i_SPECIAL_TLTU,    base_spec, syn_arith2 }, //
    { "teq",     R4300i_SPECIAL_TEQ,     base_spec, syn_arith2 }, //
    { "tne",     R4300i_SPECIAL_TNE,     base_spec, syn_arith2 }, //
    { "dsll",    R4300i_SPECIAL_DSLL,    base_spec, syn_shift },
    { "dsrl",    R4300i_SPECIAL_DSRL,    base_spec, syn_shift },
    { "dsra",    R4300i_SPECIAL_DSRA,    base_spec, syn_shift },
    { "dsll32",  R4300i_SPECIAL_DSLL32,  base_spec, syn_shift },
    { "dsrl32",  R4300i_SPECIAL_DSRL32,  base_spec, syn_shift },
    { "dsra32",  R4300i_SPECIAL_DSRA32,  base_spec, syn_shift },

    { "bltz",    R4300i_REGIMM_BLTZ,    base_regimm, syn_branch_z },
    { "bgez",    R4300i_REGIMM_BGEZ,    base_regimm, syn_branch_z },
    { "bltzl",   R4300i_REGIMM_BLTZL,   base_regimm, syn_branch_z },
    { "bgezl",   R4300i_REGIMM_BGEZL,   base_regimm, syn_branch_z },
    { "tgei",    R4300i_REGIMM_TGEI,    base_regimm, syn_trap_i },
    { "tgeiu",   R4300i_REGIMM_TGEIU,   base_regimm, syn_trap_i },
    { "tlti",    R4300i_REGIMM_TLTI,    base_regimm, syn_trap_i },
    { "tltiu",   R4300i_REGIMM_TLTIU,   base_regimm, syn_trap_i },
    { "teqi",    R4300i_REGIMM_TEQI,    base_regimm, syn_trap_i },
    { "tnei",    R4300i_REGIMM_TNEI,    base_regimm, syn_trap_i },
    { "bltzal",  R4300i_REGIMM_BLTZAL,  base_regimm, syn_branch_z },
    { "bgezal",  R4300i_REGIMM_BGEZAL,  base_regimm, syn_branch_z },
    { "bal",     R4300i_REGIMM_BGEZAL,  base_regimm, syn_branch_unc },
    { "bltzall", R4300i_REGIMM_BLTZALL, base_regimm, syn_branch_z },
    { "bgezall", R4300i_REGIMM_BGEZALL, base_regimm, syn_branch_z },

    { "mfc0", R4300i_COP0_MF , base_cop0_mv, syn_cop_mv },
    { "mtc0", R4300i_COP0_MT , base_cop0_mv, syn_cop_mv },

    { "tlbr",  R4300i_COP0_CO_TLBR,  base_cop0_co, nullptr },
    { "tlbwi", R4300i_COP0_CO_TLBWI, base_cop0_co, nullptr },
    { "tlbwr", R4300i_COP0_CO_TLBWR, base_cop0_co, nullptr },
    { "tlbp",  R4300i_COP0_CO_TLBP,  base_cop0_co, nullptr },
    { "eret",  R4300i_COP0_CO_ERET,  base_cop0_co, nullptr },

    { "mfc1",  R4300i_COP1_MF,      base_cop1_mv, syn_cop_mv },
    { "dmfc1", R4300i_COP1_DMF,     base_cop1_mv, syn_cop_mv },
    { "cfc1",  R4300i_COP1_CF,      base_cop1_mv, syn_cop_mv },
    { "mtc1",  R4300i_COP1_MT,      base_cop1_mv, syn_cop_mv },
    { "dmtc1", R4300i_COP1_DMT,     base_cop1_mv, syn_cop_mv },
    { "ctc1",  R4300i_COP1_CT,      base_cop1_mv, syn_cop_mv },

    { "bc1f",  R4300i_COP1_BC_BCF,  base_cop1_bc, syn_branch_unc },
    { "bc1t",  R4300i_COP1_BC_BCT,  base_cop1_bc, syn_branch_unc },
    { "bc1fl", R4300i_COP1_BC_BCFL, base_cop1_bc, syn_branch_unc },
    { "bc1tl", R4300i_COP1_BC_BCTL, base_cop1_bc, syn_branch_unc },

    { "add.s",     R4300i_COP1_FUNCT_ADD,     base_cop1_s, syn_cop1_arith },
    { "sub.s",     R4300i_COP1_FUNCT_SUB,     base_cop1_s, syn_cop1_arith },
    { "mul.s",     R4300i_COP1_FUNCT_MUL,     base_cop1_s, syn_cop1_arith },
    { "div.s",     R4300i_COP1_FUNCT_DIV,     base_cop1_s, syn_cop1_arith },
    { "sqrt.s",    R4300i_COP1_FUNCT_SQRT,    base_cop1_s, syn_cop1 },
    { "abs.s",     R4300i_COP1_FUNCT_ABS,     base_cop1_s, syn_cop1 },
    { "mov.s",     R4300i_COP1_FUNCT_MOV,     base_cop1_s, syn_cop1 },
    { "neg.s",     R4300i_COP1_FUNCT_NEG,     base_cop1_s, syn_cop1 },
    { "round.l.s", R4300i_COP1_FUNCT_ROUND_L, base_cop1_s, syn_cop1 },
    { "trunc.l.s", R4300i_COP1_FUNCT_TRUNC_L, base_cop1_s, syn_cop1 },
    { "ceil.l.s",  R4300i_COP1_FUNCT_CEIL_L,  base_cop1_s, syn_cop1 },
    { "floor.l.s", R4300i_COP1_FUNCT_FLOOR_L, base_cop1_s, syn_cop1 },
    { "round.w.s", R4300i_COP1_FUNCT_ROUND_W, base_cop1_s, syn_cop1 },
    { "trunc.w.s", R4300i_COP1_FUNCT_TRUNC_W, base_cop1_s, syn_cop1 },
    { "ceil.w.s",  R4300i_COP1_FUNCT_CEIL_W,  base_cop1_s, syn_cop1 },
    { "floor.w.s", R4300i_COP1_FUNCT_FLOOR_W, base_cop1_s, syn_cop1 },
    { "cvt.s.s",   R4300i_COP1_FUNCT_CVT_S,   base_cop1_s, syn_cop1 },
    { "cvt.d.s",   R4300i_COP1_FUNCT_CVT_D,   base_cop1_s, syn_cop1 },
    { "cvt.w.s",   R4300i_COP1_FUNCT_CVT_W,   base_cop1_s, syn_cop1 },
    { "cvt.l.s",   R4300i_COP1_FUNCT_CVT_L,   base_cop1_s, syn_cop1 },
    { "c.f.s",     R4300i_COP1_FUNCT_C_F,     base_cop1_s, syn_cop1_cmp },
    { "c.un.s",    R4300i_COP1_FUNCT_C_UN,    base_cop1_s, syn_cop1_cmp },
    { "c.eq.s",    R4300i_COP1_FUNCT_C_EQ,    base_cop1_s, syn_cop1_cmp },
    { "c.ueq.s",   R4300i_COP1_FUNCT_C_UEQ,   base_cop1_s, syn_cop1_cmp },
    { "c.olt.s",   R4300i_COP1_FUNCT_C_OLT,   base_cop1_s, syn_cop1_cmp },
    { "c.ult.s",   R4300i_COP1_FUNCT_C_ULT,   base_cop1_s, syn_cop1_cmp },
    { "c.ole.s",   R4300i_COP1_FUNCT_C_OLE,   base_cop1_s, syn_cop1_cmp },
    { "c.ule.s",   R4300i_COP1_FUNCT_C_ULE,   base_cop1_s, syn_cop1_cmp },
    { "c.sf.s",    R4300i_COP1_FUNCT_C_SF,    base_cop1_s, syn_cop1_cmp },
    { "c.ngle.s",  R4300i_COP1_FUNCT_C_NGLE,  base_cop1_s, syn_cop1_cmp },
    { "c.seq.s",   R4300i_COP1_FUNCT_C_SEQ,   base_cop1_s, syn_cop1_cmp },
    { "c.ngl.s",   R4300i_COP1_FUNCT_C_NGL,   base_cop1_s, syn_cop1_cmp },
    { "c.lt.s",    R4300i_COP1_FUNCT_C_LT,    base_cop1_s, syn_cop1_cmp },
    { "c.nge.s",   R4300i_COP1_FUNCT_C_NGE,   base_cop1_s, syn_cop1_cmp },
    { "c.le.s",    R4300i_COP1_FUNCT_C_LE,    base_cop1_s, syn_cop1_cmp },
    { "c.ngt.s",   R4300i_COP1_FUNCT_C_NGT,   base_cop1_s, syn_cop1_cmp },

    { "add.d",     R4300i_COP1_FUNCT_ADD,     base_cop1_d, syn_cop1_arith },
    { "sub.d",     R4300i_COP1_FUNCT_SUB,     base_cop1_d, syn_cop1_arith },
    { "mul.d",     R4300i_COP1_FUNCT_MUL,     base_cop1_d, syn_cop1_arith },
    { "div.d",     R4300i_COP1_FUNCT_DIV,     base_cop1_d, syn_cop1_arith },
    { "sqrt.d",    R4300i_COP1_FUNCT_SQRT,    base_cop1_d, syn_cop1 },
    { "abs.d",     R4300i_COP1_FUNCT_ABS,     base_cop1_d, syn_cop1 },
    { "mov.d",     R4300i_COP1_FUNCT_MOV,     base_cop1_d, syn_cop1 },
    { "neg.d",     R4300i_COP1_FUNCT_NEG,     base_cop1_d, syn_cop1 },
    { "round.l.d", R4300i_COP1_FUNCT_ROUND_L, base_cop1_d, syn_cop1 },
    { "trunc.l.d", R4300i_COP1_FUNCT_TRUNC_L, base_cop1_d, syn_cop1 },
    { "ceil.l.d",  R4300i_COP1_FUNCT_CEIL_L,  base_cop1_d, syn_cop1 },
    { "floor.l.d", R4300i_COP1_FUNCT_FLOOR_L, base_cop1_d, syn_cop1 },
    { "round.w.d", R4300i_COP1_FUNCT_ROUND_W, base_cop1_d, syn_cop1 },
    { "trunc.w.d", R4300i_COP1_FUNCT_TRUNC_W, base_cop1_d, syn_cop1 },
    { "ceil.w.d",  R4300i_COP1_FUNCT_CEIL_W,  base_cop1_d, syn_cop1 },
    { "floor.w.d", R4300i_COP1_FUNCT_FLOOR_W, base_cop1_d, syn_cop1 },
    { "cvt.s.d",   R4300i_COP1_FUNCT_CVT_S,   base_cop1_d, syn_cop1 },
    { "cvt.d.d",   R4300i_COP1_FUNCT_CVT_D,   base_cop1_d, syn_cop1 },
    { "cvt.w.d",   R4300i_COP1_FUNCT_CVT_W,   base_cop1_d, syn_cop1 },
    { "cvt.l.d",   R4300i_COP1_FUNCT_CVT_L,   base_cop1_d, syn_cop1 },
    { "c.f.d",     R4300i_COP1_FUNCT_C_F,     base_cop1_d, syn_cop1_cmp },
    { "c.un.d",    R4300i_COP1_FUNCT_C_UN,    base_cop1_d, syn_cop1_cmp },
    { "c.eq.d",    R4300i_COP1_FUNCT_C_EQ,    base_cop1_d, syn_cop1_cmp },
    { "c.ueq.d",   R4300i_COP1_FUNCT_C_UEQ,   base_cop1_d, syn_cop1_cmp },
    { "c.olt.d",   R4300i_COP1_FUNCT_C_OLT,   base_cop1_d, syn_cop1_cmp },
    { "c.ult.d",   R4300i_COP1_FUNCT_C_ULT,   base_cop1_d, syn_cop1_cmp },
    { "c.ole.d",   R4300i_COP1_FUNCT_C_OLE,   base_cop1_d, syn_cop1_cmp },
    { "c.ule.d",   R4300i_COP1_FUNCT_C_ULE,   base_cop1_d, syn_cop1_cmp },
    { "c.sf.d",    R4300i_COP1_FUNCT_C_SF,    base_cop1_d, syn_cop1_cmp },
    { "c.ngle.d",  R4300i_COP1_FUNCT_C_NGLE,  base_cop1_d, syn_cop1_cmp },
    { "c.seq.d",   R4300i_COP1_FUNCT_C_SEQ,   base_cop1_d, syn_cop1_cmp },
    { "c.ngl.d",   R4300i_COP1_FUNCT_C_NGL,   base_cop1_d, syn_cop1_cmp },
    { "c.lt.d",    R4300i_COP1_FUNCT_C_LT,    base_cop1_d, syn_cop1_cmp },
    { "c.nge.d",   R4300i_COP1_FUNCT_C_NGE,   base_cop1_d, syn_cop1_cmp },
    { "c.le.d",    R4300i_COP1_FUNCT_C_LE,    base_cop1_d, syn_cop1_cmp },
    { "c.ngt.d",   R4300i_COP1_FUNCT_C_NGT,   base_cop1_d, syn_cop1_cmp },

    { "cvt.s.w",   R4300i_COP1_FUNCT_CVT_S,   base_cop1_w, syn_cop1 },
    { "cvt.d.w",   R4300i_COP1_FUNCT_CVT_D,   base_cop1_w, syn_cop1 },
    { "cvt.s.l",   R4300i_COP1_FUNCT_CVT_S,   base_cop1_l, syn_cop1 },
    { "cvt.d.l",   R4300i_COP1_FUNCT_CVT_D,   base_cop1_l, syn_cop1 },
    { nullptr }
};

const ASM_REGISTER CAssembler::m_Registers[] =
{
    { "r0",  0 },{ "at",  1 },{ "v0",  2 },{ "v1",  3 },{ "a0",  4 },{ "a1",  5 },{ "a2",  6 },{ "a3",  7 },
    { "t0",  8 },{ "t1",  9 },{ "t2", 10 },{ "t3", 11 },{ "t4", 12 },{ "t5", 13 },{ "t6", 14 },{ "t7", 15 },
    { "s0", 16 },{ "s1", 17 },{ "s2", 18 },{ "s3", 19 },{ "s4", 20 },{ "s5", 21 },{ "s6", 22 },{ "s7", 23 },
    { "t8", 24 },{ "t9", 25 },{ "k0", 26 },{ "k1", 27 },{ "gp", 28 },{ "sp", 29 },{ "s8", 30 },{ "ra", 31 },

    { "fp", 30 },

    { "r0",   0 },{ "r1",   1 },{ "r2",   2 },{ "r3",   3 },{ "r4",   4 },{ "r5",   5 },{ "r6",   6 },{ "r7",   7 },
    { "r8",   8 },{ "r9",   9 },{ "r10", 10 },{ "r11", 11 },{ "r12", 12 },{ "r13", 13 },{ "r14", 14 },{ "r15", 15 },
    { "r16", 16 },{ "r17", 17 },{ "r18", 18 },{ "r19", 19 },{ "r20", 20 },{ "r21", 21 },{ "r22", 22 },{ "r23", 23 },
    { "r24", 24 },{ "r25", 25 },{ "r26", 26 },{ "r27", 27 },{ "r28", 28 },{ "r29", 29 },{ "r30", 30 },{ "r31", 31 },

    { "f0",   0 },{ "f1",   1 },{ "f2",   2 },{ "f3",   3 },{ "f4",   4 },{ "f5",   5 },{ "f6",   6 },{ "f7",   7 },
    { "f8",   8 },{ "f9",   9 },{ "f10", 10 },{ "f11", 11 },{ "f12", 12 },{ "f13", 13 },{ "f14", 14 },{ "f15", 15 },
    { "f16", 16 },{ "f17", 17 },{ "f18", 18 },{ "f19", 19 },{ "f20", 20 },{ "f21", 21 },{ "f22", 22 },{ "f23", 23 },
    { "f24", 24 },{ "f25", 25 },{ "f26", 26 },{ "f27", 27 },{ "f28", 28 },{ "f29", 29 },{ "f30", 30 },{ "f31", 31 },

    { "revision", 0 },{ "fcsr", 31 },{ "fcs", 31 },

    { "index",    0 },{ "random",    1 },{ "entrylo0", 2 },{ "entrylo1",  3 },{ "context",  4 },{ "pagemask", 5 },
    { "wired",    6 },{ "badvaddr",  8 },{ "count",    9 },{ "entryhi",  10 },{ "compare", 11 },{ "status",  12 },
    { "cause",   13 },{ "epc",      14 },{ "prid",    15 },{ "config",   16 },{ "lladdr",  17 },{ "watchlo", 18 },
    { "watchhi", 19 },{ "xcontext", 20 },{ "ecc",     26 },{ "cacheerr", 27 },{ "taglo",   28 },{ "taghi",   29 },
    { "errepc",  30 },

    { nullptr }
};

bool CAssembler::AssembleLine(const char* line, uint32_t* opcode, uint32_t address)
{
    m_ParseError = ERR_NONE;
    m_Address = address;

    char line_c[128];
    strncpy(line_c, line, 128);
    StrToLower(line_c);

    if (line_c[0] == '\0')
    {
        *opcode = 0;
        return true;
    }

    char* name = strtok_s(line_c, " \t", &m_TokContext);
    
    // Attempt to assemble the line
    // If a syntax error occurs, check if the command has alternative syntax forms and retry

    for(int nFallback = 0;; nFallback++)
    {
        const ASM_INSTRUCTION* instruction = LookupInstruction(name, nFallback);

        if (instruction == nullptr)
        {
            m_ParseError = ERR_UNKNOWN_CMD;
            return false;
        }
        
        m_ParseError = ERR_NONE;

        if (nFallback > 0)
        {
            // Prepare for re-tokenization
            strncpy(line_c, line, 128);
            StrToLower(line_c);
            name = strtok_s(line_c, " \t", &m_TokContext);
        }

        *opcode = instruction->base(instruction->val);

        if (instruction->syntax == nullptr)
        {
            // No parameters
            return true;
        }

        for (int i = 0; instruction->syntax[i]; i++)
        {
            instruction->syntax[i](opcode);

            if (m_ParseError != ERR_NONE)
            {
                goto next_fallback;
            }
        }

        // Assembled without errors
        return true;

    next_fallback:;
    }
}

const ASM_INSTRUCTION* CAssembler::LookupInstruction(char* name, int nFallback)
{
    for (int i = 0; m_Instructions[i].name != nullptr; i++)
    {
        if (strcmp(name, m_Instructions[i].name) == 0)
        {
            if (nFallback != 0)
            {
                nFallback--;
                continue;
            }
            return &m_Instructions[i];
        }
    }
    return nullptr;
}

const ASM_REGISTER* CAssembler::LookupRegister(char* name)
{
    for (int i = 0; m_Registers[i].name != nullptr; i++)
    {
        if (strcmp(name, m_Registers[i].name) == 0)
        {
            return &m_Registers[i];
        }
    }
    return nullptr;
}

void CAssembler::StrToLower(char* str)
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

uint32_t CAssembler::pop_reg()
{
    char* r = strtok_s(nullptr, " \t,()", &m_TokContext);

    if (r == nullptr)
    {
        m_ParseError = ERR_EXPECTED_REG;
        return 0;
    }

    const ASM_REGISTER* reg = LookupRegister(r);

    if (reg == nullptr)
    {
        m_ParseError = ERR_INVALID_REG;
        return 0;
    }

    return reg->val;
}

uint32_t CAssembler::pop_val()
{
    char* v = strtok_s(nullptr, " \t,()", &m_TokContext);

    if (v == nullptr)
    {
        m_ParseError = ERR_EXPECTED_VAL;
        return 0;
    }

    //if (isalpha(*v))
    //{
    //    // TODO: lookup label value
    //    return 0;
    //}

    int base = 0; // Hexadecimal or decimal

    if (*v == '$')
    {
        base = 16; // Hexadecimal
        v++;
    }

    char* endptr;

    uint32_t val = strtoul(v, &endptr, base);

    if (*endptr != '\0')
    {
        m_ParseError = ERR_EXPECTED_VAL;
        return 0;
    }

    return val;
}

uint32_t CAssembler::base_op(uint32_t val)
{
    return val << 26;
}

uint32_t CAssembler::base_spec(uint32_t val)
{
    return val;
}

uint32_t CAssembler::base_spec_jalr_ra(uint32_t val)
{
    return (31 << 11) | val;
}

uint32_t CAssembler::base_regimm(uint32_t val)
{
    return (R4300i_REGIMM << 26) | (val << 16);
}

uint32_t CAssembler::base_cop0_co(uint32_t val)
{
    return (R4300i_CP0 << 26) | (1 << 25) | val;
}

uint32_t CAssembler::base_cop0_mv(uint32_t val)
{
    return (R4300i_CP0 << 26) | (val << 21);
}

uint32_t CAssembler::base_cop1_mv(uint32_t val)
{
    return (R4300i_CP1 << 26) | (val << 21);
}

uint32_t CAssembler::base_cop1_s(uint32_t val)
{
    return (R4300i_CP1 << 26) | (R4300i_COP1_S << 21) | val;
}

uint32_t CAssembler::base_cop1_d(uint32_t val)
{
    return (R4300i_CP1 << 26) | (R4300i_COP1_D << 21) | val;
}

uint32_t CAssembler::base_cop1_w(uint32_t val)
{
    return (R4300i_CP1 << 26) | (R4300i_COP1_W << 21) | val;
}

uint32_t CAssembler::base_cop1_l(uint32_t val)
{
    return (R4300i_CP1 << 26) | (R4300i_COP1_L << 21) | val;
}

uint32_t CAssembler::base_cop1_bc(uint32_t val)
{
    return (R4300i_CP1 << 26) | (R4300i_COP1_BC << 21) | (val << 16);
}

void CAssembler::arg_reg_t(uint32_t* opcode)
{
    *opcode |= pop_reg() << 16;
}

void CAssembler::arg_reg_s(uint32_t* opcode)
{
    *opcode |= pop_reg() << 21;
}

void CAssembler::arg_reg_d(uint32_t* opcode)
{
    *opcode |= pop_reg() << 11;
}

void CAssembler::arg_reg_ft(uint32_t* opcode)
{
    *opcode |= pop_reg() << 16;
}

void CAssembler::arg_reg_fs(uint32_t* opcode)
{
    *opcode |= pop_reg() << 11;
}

void CAssembler::arg_reg_fd(uint32_t* opcode)
{
    *opcode |= pop_reg() << 6;
}

void CAssembler::arg_jump(uint32_t* opcode)
{
    *opcode |= (pop_val() / 4) & 0x3FFFFFF;
}

void CAssembler::arg_imm16(uint32_t* opcode)
{
    *opcode |= (pop_val() & 0xFFFF);
}

void CAssembler::arg_bra_target(uint32_t* opcode)
{
    uint16_t relTarget = (((pop_val() - m_Address) / 4) & 0xFFFF) - 1;
    *opcode |= relTarget;
}

void CAssembler::arg_shamt(uint32_t* opcode)
{
    *opcode |= (pop_val() & 0x1F) << 6;
}

void CAssembler::arg_cache_op(uint32_t* opcode)
{
    *opcode |= (pop_val() & 0x1F) << 16;
}

void CAssembler::arg_syscall_code(uint32_t* opcode)
{
    *opcode |= (pop_val() & 0xFFFFF) << 6;
}
