#include "stdafx.h"
#include <Project64-core/N64System/Mips/Register.h>
#include "R4300iInstruction.h"

R4300iInstruction::R4300iInstruction(uint32_t Address, uint32_t Instruction) :
    m_Address(Address)
{
    m_Name[0] = '\0';
    m_Param[0] = '\0';
    m_Instruction.Value = Instruction;
}

const char * R4300iInstruction::Name()
{
    if (m_Name[0] == '\0')
    {
        DecodeName();
    }
    return m_Name;
}

const char * R4300iInstruction::Param()
{
    if (m_Param[0] == '\0')
    {
        DecodeName();
    }
    return m_Param;
}

std::string R4300iInstruction::NameAndParam()
{
    return stdstr_f("%s %s", Name(), Param());
}

const char * R4300iInstruction::FPR_Type(uint32_t COP1OpCode)
{
    if (COP1OpCode == R4300i_COP1_S) { return "S"; };
    if (COP1OpCode == R4300i_COP1_D) { return "D"; };
    if (COP1OpCode == R4300i_COP1_W) { return "W"; };
    if (COP1OpCode == R4300i_COP1_L) { return "L"; };
    return "?";
}

void R4300iInstruction::DecodeName(void)
{
    switch (m_Instruction.op)
    {
    case R4300i_SPECIAL:
        DecodeSpecialName();
        break;
    case R4300i_REGIMM:
        DecodeRegImmName();
        break;
    case R4300i_J:
        strcpy(m_Name, "J");
        sprintf(m_Param, "0x%08X", (m_Address & 0xF0000000) + (m_Instruction.target << 2));
        break;
    case R4300i_JAL:
        strcpy(m_Name, "JAL");
        sprintf(m_Param, "0x%08X", (m_Address & 0xF0000000) + (m_Instruction.target << 2));
        break;
    case R4300i_BEQ:
        if (m_Instruction.rs == 0 && m_Instruction.rt == 0)
        {
            strcpy(m_Name, "B");
            sprintf(m_Param, "0x%08X", m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        else if (m_Instruction.rs == 0 || m_Instruction.rt == 0)
        {
            strcpy(m_Name, "BEQZ");
            sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs == 0 ? m_Instruction.rt : m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        else
        {
            strcpy(m_Name, "BEQ");
            sprintf(m_Param, "%s, %s, 0x%08X", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        break;
    case R4300i_BNE:
        if ((m_Instruction.rs == 0) ^ (m_Instruction.rt == 0))
        {
            strcpy(m_Name, "BNEZ");
            sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs == 0 ? m_Instruction.rt : m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        else
        {
            strcpy(m_Name, "BNE");
            sprintf(m_Param, "%s, %s, 0x%08X", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        break;
    case R4300i_BLEZ:
        strcpy(m_Name, "BLEZ");
        sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        break;
    case R4300i_BGTZ:
        strcpy(m_Name, "BGTZ");
        sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        break;
    case R4300i_ADDI:
        strcpy(m_Name, "ADDI");
        sprintf(m_Param, "%s, %s, 0x%04X", CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_ADDIU:
        // Special case for stack
        strcpy(m_Name, "ADDIU");
        if (m_Instruction.rt == 29)
        {
            short imm = (short)m_Instruction.immediate;
            sprintf(m_Param, "%s, %s, %s0x%02X", CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs], imm < 0 ? "-" : "", abs(imm));
        }
        else
        {
            sprintf(m_Param, "%s, %s, 0x%04X", CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        }
        break;
    case R4300i_SLTI:
        strcpy(m_Name, "SLTI");
        sprintf(m_Param, "%s, %s, 0x%04X", CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_SLTIU:
        strcpy(m_Name, "SLTIU");
        sprintf(m_Param, "%s, %s, 0x%04X", CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_ANDI:
        strcpy(m_Name, "ANDI");
        sprintf(m_Param, "%s, %s, 0x%04X", CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_ORI:
        strcpy(m_Name, "ORI");
        sprintf(m_Param, "%s, %s, 0x%04X", CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_XORI:
        strcpy(m_Name, "XORI");
        sprintf(m_Param, "%s, %s, 0x%04X", CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_LUI:
        strcpy(m_Name, "LUI");
        sprintf(m_Param, "%s, 0x%04X", CRegName::GPR[m_Instruction.rt], m_Instruction.immediate);
        break;
    case R4300i_CP0:
        switch (m_Instruction.rs)
        {
        case R4300i_COP0_MF:
            strcpy(m_Name, "MFC0");
            sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rt], CRegName::Cop0[m_Instruction.rd]);
            break;
        case R4300i_COP1_DMF:
            strcpy(m_Name, "DMFC0");
            sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rt], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP0_MT:
            strcpy(m_Name, "MTC0");
            sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rt], CRegName::Cop0[m_Instruction.rd]);
            break;
        case R4300i_COP0_DMT:
            strcpy(m_Name, "DMTC0");
            sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rt], CRegName::Cop0[m_Instruction.rd]);
            break;
        default:
            if ((m_Instruction.rs & 0x10) != 0)
            {
                switch (m_Instruction.funct)
                {
                case R4300i_COP0_CO_TLBR: strcpy(m_Name, "TLBR"); break;
                case R4300i_COP0_CO_TLBWI: strcpy(m_Name, "TLBWI"); break;
                case R4300i_COP0_CO_TLBWR: strcpy(m_Name, "TLBWR"); break;
                case R4300i_COP0_CO_TLBP: strcpy(m_Name, "TLBP"); break;
                case R4300i_COP0_CO_ERET: strcpy(m_Name, "ERET"); break;
                default:
                    strcpy(m_Name, "UNKNOWN");
                    sprintf(m_Param, "0x%08X", m_Instruction.Value);
                }
            }
            else
            {
                strcpy(m_Name, "UNKNOWN");
                sprintf(m_Param, "0x%08X", m_Instruction.Value);
            }
            break;
        }
        break;
    case R4300i_CP1:
        DecodeCop1Name();
        break;
    case R4300i_BEQL:
        if (m_Instruction.rs == m_Instruction.rt)
        {
            strcpy(m_Name, "B");
            sprintf(m_Param, "0x%08X", m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        else if ((m_Instruction.rs == 0) ^ (m_Instruction.rt == 0))
        {
            strcpy(m_Name, "BEQZL");
            sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs == 0 ? m_Instruction.rt : m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        else
        {
            strcpy(m_Name, "BEQL");
            sprintf(m_Param, "%s, %s, 0x%08X", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        break;
    case R4300i_BNEL:
        if ((m_Instruction.rs == 0) ^ (m_Instruction.rt == 0))
        {
            strcpy(m_Name, "BNEZL");
            sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs == 0 ? m_Instruction.rt : m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        else
        {
            strcpy(m_Name, "BNEL");
            sprintf(m_Param, "%s, %s, 0x%08X", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        break;
    case R4300i_BLEZL:
        strcpy(m_Name, "BLEZL");
        sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        break;
    case R4300i_BGTZL:
        strcpy(m_Name, "BGTZL");
        sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        break;
    case R4300i_DADDI:
        strcpy(m_Name, "DADDI");
        sprintf(m_Param, "%s, %s, 0x%04X", CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_DADDIU:
        strcpy(m_Name, "DADDIU");
        sprintf(m_Param, "%s, %s, 0x%04X", CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_LDL:
        strcpy(m_Name, "LDL");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LDR:
        strcpy(m_Name, "LDR");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LB:
        strcpy(m_Name, "LB");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LH:
        strcpy(m_Name, "LH");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LWL:
        strcpy(m_Name, "LWL");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LW:
        strcpy(m_Name, "LW");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LBU:
        strcpy(m_Name, "LBU");
        sprintf(m_Param, "t%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LHU:
        strcpy(m_Name, "LHU");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LWR:
        strcpy(m_Name, "LWR");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LWU:
        strcpy(m_Name, "LWU");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SB:
        strcpy(m_Name, "SB");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SH:
        strcpy(m_Name, "SH");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SWL:
        strcpy(m_Name, "SWL");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SW:
        strcpy(m_Name, "SW");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SDL:
        strcpy(m_Name, "SDL");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SDR:
        strcpy(m_Name, "SDR");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SWR:
        strcpy(m_Name, "SWR");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_CACHE:
        strcpy(m_Name, "CACHE");
        sprintf(m_Param, "%d, 0x%04X (%s)", m_Instruction.rt, m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LL:
        strcpy(m_Name, "LL");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LWC1:
        strcpy(m_Name, "LWC1");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::FPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LDC1:
        strcpy(m_Name, "LDC1");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::FPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_LD:
        strcpy(m_Name, "LD");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SC:
        strcpy(m_Name, "SC");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SWC1:
        strcpy(m_Name, "SWC1");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::FPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SDC1:
        strcpy(m_Name, "SDC1");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::FPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    case R4300i_SD:
        strcpy(m_Name, "SD");
        sprintf(m_Param, "%s, 0x%04X (%s)", CRegName::GPR[m_Instruction.rt], m_Instruction.offset, CRegName::GPR[m_Instruction.base]);
        break;
    default:
        strcpy(m_Name, "UNKNOWN");
        sprintf(m_Param, "0x%08X", m_Instruction.Value);
    }
}

void R4300iInstruction::DecodeSpecialName(void)
{
    switch (m_Instruction.funct)
    {
    case R4300i_SPECIAL_SLL:
        if (m_Instruction.Value != 0)
        {
            strcpy(m_Name, "SLL");
            sprintf(m_Param, "SLL\t%s, %s, %d", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], m_Instruction.sa);
        }
        else
        {
            strcpy(m_Name, "NOP");
        }
        break;
    case R4300i_SPECIAL_SRL:
        strcpy(m_Name, "SRL");
        sprintf(m_Param, "%s, %s, %d", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], m_Instruction.sa);
        break;
    case R4300i_SPECIAL_SRA:
        strcpy(m_Name, "SRA");
        sprintf(m_Param, "%s, %s, %d", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], m_Instruction.sa);
        break;
    case R4300i_SPECIAL_SLLV:
        strcpy(m_Name, "SLLV");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs]);
        break;
    case R4300i_SPECIAL_SRLV:
        strcpy(m_Name, "SRLV");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs]);
        break;
    case R4300i_SPECIAL_SRAV:
        strcpy(m_Name, "SRAV");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs]);
        break;
    case R4300i_SPECIAL_JR:
        strcpy(m_Name, "JR");
        sprintf(m_Param, "%s", CRegName::GPR[m_Instruction.rs]);
        break;
    case R4300i_SPECIAL_JALR:
        strcpy(m_Name, "JALR");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs]);
        break;
    case R4300i_SPECIAL_SYSCALL:
        strcpy(m_Name, "SYSCALL");
        sprintf(m_Param, "0x%05X", m_Instruction.code);
        break;
    case R4300i_SPECIAL_BREAK:
        strcpy(m_Name, "BREAK");
        sprintf(m_Param, "0x%05X", m_Instruction.code);
        break;
    case R4300i_SPECIAL_SYNC:
        strcpy(m_Name, "SYNC");
        strcpy(m_Param, "");
        break;
    case R4300i_SPECIAL_MFHI:
        strcpy(m_Name, "MFHI");
        sprintf(m_Param, "%s", CRegName::GPR[m_Instruction.rd]);
        break;
    case R4300i_SPECIAL_MTHI:
        strcpy(m_Name, "MTHI");
        sprintf(m_Param, "%s", CRegName::GPR[m_Instruction.rs]);
        break;
    case R4300i_SPECIAL_MFLO:
        strcpy(m_Name, "MFLO");
        sprintf(m_Param, "%s", CRegName::GPR[m_Instruction.rd]);
        break;
    case R4300i_SPECIAL_MTLO:
        strcpy(m_Name, "MTLO");
        sprintf(m_Param, "%s", CRegName::GPR[m_Instruction.rs]);
        break;
    case R4300i_SPECIAL_DSLLV:
        strcpy(m_Name, "DSLLV");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs]);
        break;
    case R4300i_SPECIAL_DSRLV:
        strcpy(m_Name, "DSRLV");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs]);
        break;
    case R4300i_SPECIAL_DSRAV:
        strcpy(m_Name, "DSRAV");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], CRegName::GPR[m_Instruction.rs]);
        break;
    case R4300i_SPECIAL_MULT:
        strcpy(m_Name, "MULT");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_MULTU:
        strcpy(m_Name, "MULTU");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DIV:
        strcpy(m_Name, "DIV");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DIVU:
        strcpy(m_Name, "DIVU");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DMULT:
        strcpy(m_Name, "DMULT");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DMULTU:
        strcpy(m_Name, "DMULTU");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DDIV:
        strcpy(m_Name, "DIVU");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DDIVU:
        strcpy(m_Name, "DDIVU");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_ADD:
        strcpy(m_Name, "ADD");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_ADDU:
        strcpy(m_Name, "ADDU");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_SUB:
        strcpy(m_Name, "SUB");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_SUBU:
        strcpy(m_Name, "SUBU");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_AND:
        strcpy(m_Name, "AND");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_OR:
        strcpy(m_Name, "OR");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_XOR:
        strcpy(m_Name, "XOR");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_NOR:
        strcpy(m_Name, "NOR");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_SLT:
        strcpy(m_Name, "SLT");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_SLTU:
        strcpy(m_Name, "SLTU");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DADD:
        strcpy(m_Name, "DADD");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DADDU:
        strcpy(m_Name, "DADDU");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DSUB:
        strcpy(m_Name, "DSUB");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DSUBU:
        strcpy(m_Name, "DSUBU");
        sprintf(m_Param, "%s, %s, %s", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_TGE:
        strcpy(m_Name, "TGE");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_TGEU:
        strcpy(m_Name, "TGEU");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_TLT:
        strcpy(m_Name, "TLT");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_TLTU:
        strcpy(m_Name, "TLTU");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_TEQ:
        strcpy(m_Name, "TEQ");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_TNE:
        strcpy(m_Name, "TNE");
        sprintf(m_Param, "t%s, %s", CRegName::GPR[m_Instruction.rs], CRegName::GPR[m_Instruction.rt]);
        break;
    case R4300i_SPECIAL_DSLL:
        strcpy(m_Name, "DSLL");
        sprintf(m_Param, "%s, %s, %d", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], m_Instruction.sa);
        break;
    case R4300i_SPECIAL_DSRL:
        strcpy(m_Name, "DSRL");
        sprintf(m_Param, "%s, %s, %d", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], m_Instruction.sa);
        break;
    case R4300i_SPECIAL_DSRA:
        strcpy(m_Name, "DSRA");
        sprintf(m_Param, "%s, %s, %d", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], m_Instruction.sa);
        break;
    case R4300i_SPECIAL_DSLL32:
        strcpy(m_Name, "DSLL32");
        sprintf(m_Param, "%s, %s, %d", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], m_Instruction.sa);
        break;
    case R4300i_SPECIAL_DSRL32:
        strcpy(m_Name, "DSRL32");
        sprintf(m_Param, "%s, %s, %d", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], m_Instruction.sa);
        break;
    case R4300i_SPECIAL_DSRA32:
        strcpy(m_Name, "DSRA32");
        sprintf(m_Param, "%s, %s, %d", CRegName::GPR[m_Instruction.rd], CRegName::GPR[m_Instruction.rt], m_Instruction.sa);
        break;
    default:
        strcpy(m_Name, "UNKNOWN");
        sprintf(m_Param, "0x%08X", m_Instruction.Value);
    }
}

void R4300iInstruction::DecodeRegImmName(void)
{
    switch (m_Instruction.rt)
    {
    case R4300i_REGIMM_BLTZ:
        strcpy(m_Name, "BLTZ");
        sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        break;
    case R4300i_REGIMM_BGEZ:
        if (m_Instruction.rs == 0)
        {
            strcpy(m_Name, "B");
            sprintf(m_Param, "0x%08X", m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        else
        {
            strcpy(m_Name, "BGEZ");
            sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        break;
    case R4300i_REGIMM_BLTZL:
        strcpy(m_Name, "BLTZL");
        sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        break;
    case R4300i_REGIMM_BGEZL:
        strcpy(m_Name, "BGEZL");
        sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        break;
    case R4300i_REGIMM_TGEI:
        strcpy(m_Name, "TGEI");
        sprintf(m_Param, "%s, 0x%04X", CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_REGIMM_TGEIU:
        strcpy(m_Name, "TGEIU");
        sprintf(m_Param, "%s, 0x%04X", CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_REGIMM_TLTI:
        strcpy(m_Name, "TLTI");
        sprintf(m_Param, "%s, 0x%04X", CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_REGIMM_TLTIU:
        strcpy(m_Name, "TLTIU");
        sprintf(m_Param, "%s, 0x%04X", CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_REGIMM_TEQI:
        strcpy(m_Name, "TEQI");
        sprintf(m_Param, "%s, 0x%04X", CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_REGIMM_TNEI:
        strcpy(m_Name, "TNEI");
        sprintf(m_Param, "%s, 0x%04X", CRegName::GPR[m_Instruction.rs], m_Instruction.immediate);
        break;
    case R4300i_REGIMM_BLTZAL:
        strcpy(m_Name, "BLTZAL");
        sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        break;
    case R4300i_REGIMM_BGEZAL:
        if (m_Instruction.rs == 0)
        {
            strcpy(m_Name, "BAL");
            sprintf(m_Param, "0x%08X", m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        else
        {
            strcpy(m_Name, "BGEZAL");
            sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        }
        break;
    case R4300i_REGIMM_BLTZALL:
        strcpy(m_Name, "BLTZALL");
        sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        break;
    case R4300i_REGIMM_BGEZALL:
        strcpy(m_Name, "BGEZALL");
        sprintf(m_Param, "%s, 0x%08X", CRegName::GPR[m_Instruction.rs], m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
        break;
    default:
        strcpy(m_Name, "UNKNOWN");
        sprintf(m_Param, "0x%08X", m_Instruction.Value);
    }
}

void R4300iInstruction::DecodeCop1Name(void)
{
    switch (m_Instruction.fmt)
    {
    case R4300i_COP1_MF:
        strcpy(m_Name, "MFC1");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rt], CRegName::FPR[m_Instruction.fs]);
        break;
    case R4300i_COP1_DMF:
        strcpy(m_Name, "DMFC1");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rt], CRegName::FPR[m_Instruction.fs]);
        break;
    case R4300i_COP1_CF:
        strcpy(m_Name, "CFC1");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rt], CRegName::FPR_Ctrl[m_Instruction.fs]);
        break;
    case R4300i_COP1_MT:
        strcpy(m_Name, "MTC1");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rt], CRegName::FPR[m_Instruction.fs]);
        break;
    case R4300i_COP1_DMT:
        strcpy(m_Name, "DMTC1");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rt], CRegName::FPR[m_Instruction.fs]);
        break;
    case R4300i_COP1_CT:
        strcpy(m_Name, "CTC1");
        sprintf(m_Param, "%s, %s", CRegName::GPR[m_Instruction.rt], CRegName::FPR_Ctrl[m_Instruction.fs]);
        break;
    case R4300i_COP1_BC:
        switch (m_Instruction.ft)
        {
        case R4300i_COP1_BC_BCF:
            strcpy(m_Name, "BC1F");
            sprintf(m_Param, "0x%08X", m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
            break;
        case R4300i_COP1_BC_BCT:
            strcpy(m_Name, "BC1T");
            sprintf(m_Param, "0x%08X", m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
            break;
        case R4300i_COP1_BC_BCFL:
            strcpy(m_Name, "BC1FL");
            sprintf(m_Param, "0x%08X", m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
            break;
        case R4300i_COP1_BC_BCTL:
            strcpy(m_Name, "BC1TL");
            sprintf(m_Param, "0x%08X", m_Address + ((int16_t)m_Instruction.offset << 2) + 4);
            break;
        default:
            strcpy(m_Name, "UNKNOWN COP1");
            sprintf(m_Param, "0x%08X", m_Instruction.Value);
        }
        break;
    case R4300i_COP1_S:
    case R4300i_COP1_D:
    case R4300i_COP1_W:
    case R4300i_COP1_L:
        switch (m_Instruction.funct)
        {
        case R4300i_COP1_FUNCT_ADD:
            sprintf(m_Name, "ADD.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_SUB:
            sprintf(m_Name, "SUB.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_MUL:
            sprintf(m_Name, "MUL.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_DIV:
            sprintf(m_Name, "DIV.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_SQRT:
            sprintf(m_Name, "SQRT.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_ABS:
            sprintf(m_Name, "ABS.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_MOV:
            sprintf(m_Name, "MOV.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_NEG:
            sprintf(m_Name, "NEG.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_ROUND_L:
            sprintf(m_Name, "ROUND.L.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_TRUNC_L:
            sprintf(m_Name, "TRUNC.L.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_CEIL_L:
            sprintf(m_Name, "CEIL.L.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_FLOOR_L:
            sprintf(m_Name, "FLOOR.L.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_ROUND_W:
            sprintf(m_Name, "ROUND.W.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_TRUNC_W:
            sprintf(m_Name, "TRUNC.W.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "t%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_CEIL_W:
            sprintf(m_Name, "CEIL.W.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_FLOOR_W:
            sprintf(m_Name, "FLOOR.W.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_CVT_S:
            sprintf(m_Name, "CVT.S.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_CVT_D:
            sprintf(m_Name, "CVT.D.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_CVT_W:
            sprintf(m_Name, "CVT.W.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_CVT_L:
            sprintf(m_Name, "CVT.L.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fd], CRegName::FPR[m_Instruction.fs]);
            break;
        case R4300i_COP1_FUNCT_C_F:
            sprintf(m_Name, "C.F.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_UN:
            sprintf(m_Name, "C.UN.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_EQ:
            sprintf(m_Name, "C.EQ.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_UEQ:
            sprintf(m_Name, "C.UEQ.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_OLT:
            sprintf(m_Name, "C.OLT.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_ULT:
            sprintf(m_Name, "C.ULT.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_OLE:
            sprintf(m_Name, "C.OLE.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_ULE:
            sprintf(m_Name, "C.ULE.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_SF:
            sprintf(m_Name, "C.SF.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_NGLE:
            sprintf(m_Name, "C.NGLE.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_SEQ:
            sprintf(m_Name, "C.SEQ.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_NGL:
            sprintf(m_Name, "C.NGL.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_LT:
            sprintf(m_Name, "C.LT.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_NGE:
            sprintf(m_Name, "C.NGE.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_LE:
            sprintf(m_Name, "C.LE.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        case R4300i_COP1_FUNCT_C_NGT:
            sprintf(m_Name, "C.NGT.%s", FPR_Type(m_Instruction.fmt));
            sprintf(m_Param, "%s, %s", CRegName::FPR[m_Instruction.fs], CRegName::FPR[m_Instruction.ft]);
            break;
        default:
            strcpy(m_Name, "UNKNOWN COP1");
            sprintf(m_Param, "0x%08X", m_Instruction.Value);
        }
        break;
    default:
        strcpy(m_Name, "UNKNOWN COP1");
        sprintf(m_Param, "0x%08X", m_Instruction.Value);
    }
}

