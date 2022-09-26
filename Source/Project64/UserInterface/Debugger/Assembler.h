#pragma once

#include <stdint.h>

typedef void (*ASM_SYNTAX_FN)(uint32_t * opcode);

typedef struct
{
    const char * name;
    uint32_t val;
    uint32_t (*base)(uint32_t val); // Value shift
    const ASM_SYNTAX_FN * syntax;   // Arguments
} ASM_INSTRUCTION;

typedef struct
{
    const char * name;
    uint32_t val;
} ASM_REGISTER;

enum ASM_PARSE_ERROR
{
    ERR_NONE,
    ERR_EXPECTED_REG,
    ERR_INVALID_REG,
    ERR_EXPECTED_VAL,
    ERR_UNKNOWN_CMD
};

class CAssembler
{
private:
    static uint32_t m_Address;
    static ASM_PARSE_ERROR m_ParseError;

public:
    static bool AssembleLine(const char * line, uint32_t * opcode, uint32_t address = 0x00000000);

private:
    static const ASM_SYNTAX_FN syn_jump[];
    static const ASM_SYNTAX_FN syn_loadstore[];
    static const ASM_SYNTAX_FN syn_arith[];
    static const ASM_SYNTAX_FN syn_arith2[];
    static const ASM_SYNTAX_FN syn_shiftv[];
    static const ASM_SYNTAX_FN syn_arith_i[];
    static const ASM_SYNTAX_FN syn_load_i[];
    static const ASM_SYNTAX_FN syn_branch_z[];
    static const ASM_SYNTAX_FN syn_branch[];
    static const ASM_SYNTAX_FN syn_branch_unc[];
    static const ASM_SYNTAX_FN syn_trap_i[];
    static const ASM_SYNTAX_FN syn_shift[];
    static const ASM_SYNTAX_FN syn_mf[];
    static const ASM_SYNTAX_FN syn_jr[];
    static const ASM_SYNTAX_FN syn_jalr[];
    static const ASM_SYNTAX_FN syn_cop1_arith[];
    static const ASM_SYNTAX_FN syn_cop1[];
    static const ASM_SYNTAX_FN syn_cop1_cmp[];
    static const ASM_SYNTAX_FN syn_cop_mv[];
    static const ASM_SYNTAX_FN syn_cache[];
    static const ASM_SYNTAX_FN syn_syscall[];

    static const ASM_INSTRUCTION m_Instructions[];
    static const ASM_REGISTER m_Registers[];

    static char * m_TokContext;

    static const ASM_REGISTER * LookupRegister(char * name);
    static const ASM_INSTRUCTION * LookupInstruction(char * name, int nFallback);

    static void StrToLower(char * str);

    static uint32_t pop_reg();
    static uint32_t pop_val();

    static uint32_t base_op(uint32_t val);
    static uint32_t base_spec(uint32_t val);
    static uint32_t base_spec_jalr_ra(uint32_t val);
    static uint32_t base_regimm(uint32_t val);
    static uint32_t base_cop1_s(uint32_t val);
    static uint32_t base_cop1_d(uint32_t val);
    static uint32_t base_cop1_w(uint32_t val);
    static uint32_t base_cop1_l(uint32_t val);
    static uint32_t base_cop1_bc(uint32_t val);
    static uint32_t base_cop0_co(uint32_t val);
    static uint32_t base_cop0_mv(uint32_t val);
    static uint32_t base_cop1_mv(uint32_t val);

    static void arg_reg_t(uint32_t * opcode);
    static void arg_reg_s(uint32_t * opcode);
    static void arg_reg_d(uint32_t * opcode);
    static void arg_reg_ft(uint32_t * opcode);
    static void arg_reg_fs(uint32_t * opcode);
    static void arg_reg_fd(uint32_t * opcode);
    static void arg_jump(uint32_t * opcode);
    static void arg_imm16(uint32_t * opcode);
    static void arg_bra_target(uint32_t * opcode);
    static void arg_shamt(uint32_t * opcode);
    static void arg_cache_op(uint32_t * opcode);
    static void arg_syscall_code(uint32_t * opcode);
};
