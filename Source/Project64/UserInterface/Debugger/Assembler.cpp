/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "stdafx.h"

#include "Assembler.h"
#include "Project64-core\N64System\Mips\OpCode.h"

ASM_PARSE_ERROR CAssembler::m_ParseError = ERR_NONE;
uint32_t CAssembler::m_Address = 0;

char* CAssembler::m_TokContext = NULL;

bool CAssembler::AssembleLine(char* line, uint32_t* opcode, uint32_t address)
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
    
    // attempt to assemble the line
    // if a syntax error occurs, check if the command has alternative syntax forms and retry

    for(int nFallback = 0;; nFallback++)
    {
        const ASM_INSTRUCTION* instruction = LookupInstruction(name, nFallback);

        if (instruction == NULL)
        {
            m_ParseError = ERR_UNKNOWN_CMD;
            return false;
        }
        
        m_ParseError = ERR_NONE;

        if (nFallback > 0)
        {
            // prepare for re-tokenization
            strncpy(line_c, line, 128);
            StrToLower(line_c);
            name = strtok_s(line_c, " \t", &m_TokContext);
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

            if (m_ParseError != ERR_NONE)
            {
                goto next_fallback;
            }
        }

        // assembled without errors
        return true;

    next_fallback:;
    }
}

const ASM_INSTRUCTION* CAssembler::LookupInstruction(char* name, int nFallback)
{
    for (int i = 0; m_Instructions[i].name != NULL; i++)
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
    return NULL;
}

const ASM_REGISTER* CAssembler::LookupRegister(char* name)
{
    for (int i = 0; m_Registers[i].name != NULL; i++)
    {
        if (strcmp(name, m_Registers[i].name) == 0)
        {
            return &m_Registers[i];
        }
    }
    return NULL;
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
    char* r = strtok_s(NULL, " \t,()", &m_TokContext);

    if (r == NULL)
    {
        m_ParseError = ERR_EXPECTED_REG;
        return 0;
    }

    const ASM_REGISTER* reg = LookupRegister(r);

    if (reg == NULL)
    {
        m_ParseError = ERR_INVALID_REG;
        return 0;
    }

    return reg->val;
}

uint32_t CAssembler::pop_val()
{
    char* v = strtok_s(NULL, " \t,()", &m_TokContext);

    if (v == NULL)
    {
        m_ParseError = ERR_EXPECTED_VAL;
        return 0;
    }

    //if (isalpha(*v))
    //{
    //    // todo lookup label value
    //    return 0;
    //}

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