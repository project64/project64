#pragma once
#include <stdint.h>

#pragma warning(push)
#pragma warning(disable : 4201) // warning C4201: nonstandard extension used : nameless struct/union

union R4300iOpcode
{
    uint32_t Value;

    struct
    {
        unsigned offset : 16;
        unsigned rt : 5;
        unsigned rs : 5;
        unsigned op : 6;
    };

    struct
    {
        unsigned immediate : 16;
        unsigned : 5;
        unsigned base : 5;
        unsigned : 6;
    };

    struct
    {
        unsigned target : 26;
        unsigned : 6;
    };

    struct
    {
        unsigned funct : 6;
        unsigned sa : 5;
        unsigned rd : 5;
        unsigned : 5;
        unsigned : 5;
        unsigned : 6;
    };

    struct
    {
        unsigned : 6;
        unsigned fd : 5;
        unsigned fs : 5;
        unsigned ft : 5;
        unsigned fmt : 5;
        unsigned : 6;
    };

    struct
    {
        unsigned : 6;
        unsigned code : 20;
        unsigned : 6;
    };
};
#pragma warning(pop)

enum R4300iOpCodes
{
    R4300i_SPECIAL = 0,
    R4300i_REGIMM = 1,
    R4300i_J = 2,
    R4300i_JAL = 3,
    R4300i_BEQ = 4,
    R4300i_BNE = 5,
    R4300i_BLEZ = 6,
    R4300i_BGTZ = 7,
    R4300i_ADDI = 8,
    R4300i_ADDIU = 9,
    R4300i_SLTI = 10,
    R4300i_SLTIU = 11,
    R4300i_ANDI = 12,
    R4300i_ORI = 13,
    R4300i_XORI = 14,
    R4300i_LUI = 15,
    R4300i_CP0 = 16,
    R4300i_CP1 = 17,
    R4300i_BEQL = 20,
    R4300i_BNEL = 21,
    R4300i_BLEZL = 22,
    R4300i_BGTZL = 23,
    R4300i_DADDI = 24,
    R4300i_DADDIU = 25,
    R4300i_LDL = 26,
    R4300i_LDR = 27,
    R4300i_RESERVED31 = 31,
    R4300i_LB = 32,
    R4300i_LH = 33,
    R4300i_LWL = 34,
    R4300i_LW = 35,
    R4300i_LBU = 36,
    R4300i_LHU = 37,
    R4300i_LWR = 38,
    R4300i_LWU = 39,
    R4300i_SB = 40,
    R4300i_SH = 41,
    R4300i_SWL = 42,
    R4300i_SW = 43,
    R4300i_SDL = 44,
    R4300i_SDR = 45,
    R4300i_SWR = 46,
    R4300i_CACHE = 47,
    R4300i_LL = 48,
    R4300i_LWC1 = 49,
    R4300i_LDC1 = 53,
    R4300i_LD = 55,
    R4300i_SC = 56,
    R4300i_SWC1 = 57,
    R4300i_SDC1 = 61,
    R4300i_SDC2 = 62,
    R4300i_SD = 63
};

enum R4300iSpecialOpCodes
{
    R4300i_SPECIAL_SLL = 0,
    R4300i_SPECIAL_SRL = 2,
    R4300i_SPECIAL_SRA = 3,
    R4300i_SPECIAL_SLLV = 4,
    R4300i_SPECIAL_SRLV = 6,
    R4300i_SPECIAL_SRAV = 7,
    R4300i_SPECIAL_JR = 8,
    R4300i_SPECIAL_JALR = 9,
    R4300i_SPECIAL_SYSCALL = 12,
    R4300i_SPECIAL_BREAK = 13,
    R4300i_SPECIAL_SYNC = 15,
    R4300i_SPECIAL_MFHI = 16,
    R4300i_SPECIAL_MTHI = 17,
    R4300i_SPECIAL_MFLO = 18,
    R4300i_SPECIAL_MTLO = 19,
    R4300i_SPECIAL_DSLLV = 20,
    R4300i_SPECIAL_DSRLV = 22,
    R4300i_SPECIAL_DSRAV = 23,
    R4300i_SPECIAL_MULT = 24,
    R4300i_SPECIAL_MULTU = 25,
    R4300i_SPECIAL_DIV = 26,
    R4300i_SPECIAL_DIVU = 27,
    R4300i_SPECIAL_DMULT = 28,
    R4300i_SPECIAL_DMULTU = 29,
    R4300i_SPECIAL_DDIV = 30,
    R4300i_SPECIAL_DDIVU = 31,
    R4300i_SPECIAL_ADD = 32,
    R4300i_SPECIAL_ADDU = 33,
    R4300i_SPECIAL_SUB = 34,
    R4300i_SPECIAL_SUBU = 35,
    R4300i_SPECIAL_AND = 36,
    R4300i_SPECIAL_OR = 37,
    R4300i_SPECIAL_XOR = 38,
    R4300i_SPECIAL_NOR = 39,
    R4300i_SPECIAL_SLT = 42,
    R4300i_SPECIAL_SLTU = 43,
    R4300i_SPECIAL_DADD = 44,
    R4300i_SPECIAL_DADDU = 45,
    R4300i_SPECIAL_DSUB = 46,
    R4300i_SPECIAL_DSUBU = 47,
    R4300i_SPECIAL_TGE = 48,
    R4300i_SPECIAL_TGEU = 49,
    R4300i_SPECIAL_TLT = 50,
    R4300i_SPECIAL_TLTU = 51,
    R4300i_SPECIAL_TEQ = 52,
    R4300i_SPECIAL_TNE = 54,
    R4300i_SPECIAL_DSLL = 56,
    R4300i_SPECIAL_DSRL = 58,
    R4300i_SPECIAL_DSRA = 59,
    R4300i_SPECIAL_DSLL32 = 60,
    R4300i_SPECIAL_DSRL32 = 62,
    R4300i_SPECIAL_DSRA32 = 63
};

enum R4300iRegImmOpCodes
{
    R4300i_REGIMM_BLTZ = 0,
    R4300i_REGIMM_BGEZ = 1,
    R4300i_REGIMM_BLTZL = 2,
    R4300i_REGIMM_BGEZL = 3,
    R4300i_REGIMM_TGEI = 8,
    R4300i_REGIMM_TGEIU = 9,
    R4300i_REGIMM_TLTI = 10,
    R4300i_REGIMM_TLTIU = 11,
    R4300i_REGIMM_TEQI = 12,
    R4300i_REGIMM_TNEI = 14,
    R4300i_REGIMM_BLTZAL = 16,
    R4300i_REGIMM_BGEZAL = 17,
    R4300i_REGIMM_BLTZALL = 18,
    R4300i_REGIMM_BGEZALL = 19,
};

enum R4300iCOP0OpCodes
{
    R4300i_COP0_MF = 0,
    R4300i_COP0_DMF = 1,
    R4300i_COP0_MT = 4,
    R4300i_COP0_DMT = 5,
};

enum R4300iCOP0C0OpCodes
{
    R4300i_COP0_CO_TLBR = 1,
    R4300i_COP0_CO_TLBWI = 2,
    R4300i_COP0_CO_TLBWR = 6,
    R4300i_COP0_CO_TLBP = 8,
    R4300i_COP0_CO_ERET = 24,
};

enum R4300iCOP1OpCodes
{
    R4300i_COP1_MF = 0,
    R4300i_COP1_DMF = 1,
    R4300i_COP1_CF = 2,
    R4300i_COP1_MT = 4,
    R4300i_COP1_DMT = 5,
    R4300i_COP1_CT = 6,
    R4300i_COP1_BC = 8,
    R4300i_COP1_S = 16,
    R4300i_COP1_D = 17,
    R4300i_COP1_W = 20,
    R4300i_COP1_L = 21,
};

enum R4300iCOP1BcOpCodes
{
    R4300i_COP1_BC_BCF = 0,
    R4300i_COP1_BC_BCT = 1,
    R4300i_COP1_BC_BCFL = 2,
    R4300i_COP1_BC_BCTL = 3,
};

enum R4300iCOP1FuntOpCodes
{
    R4300i_COP1_FUNCT_ADD = 0,
    R4300i_COP1_FUNCT_SUB = 1,
    R4300i_COP1_FUNCT_MUL = 2,
    R4300i_COP1_FUNCT_DIV = 3,
    R4300i_COP1_FUNCT_SQRT = 4,
    R4300i_COP1_FUNCT_ABS = 5,
    R4300i_COP1_FUNCT_MOV = 6,
    R4300i_COP1_FUNCT_NEG = 7,
    R4300i_COP1_FUNCT_ROUND_L = 8,
    R4300i_COP1_FUNCT_TRUNC_L = 9,
    R4300i_COP1_FUNCT_CEIL_L = 10,
    R4300i_COP1_FUNCT_FLOOR_L = 11,
    R4300i_COP1_FUNCT_ROUND_W = 12,
    R4300i_COP1_FUNCT_TRUNC_W = 13,
    R4300i_COP1_FUNCT_CEIL_W = 14,
    R4300i_COP1_FUNCT_FLOOR_W = 15,
    R4300i_COP1_FUNCT_CVT_S = 32,
    R4300i_COP1_FUNCT_CVT_D = 33,
    R4300i_COP1_FUNCT_CVT_W = 36,
    R4300i_COP1_FUNCT_CVT_L = 37,
    R4300i_COP1_FUNCT_C_F = 48,
    R4300i_COP1_FUNCT_C_UN = 49,
    R4300i_COP1_FUNCT_C_EQ = 50,
    R4300i_COP1_FUNCT_C_UEQ = 51,
    R4300i_COP1_FUNCT_C_OLT = 52,
    R4300i_COP1_FUNCT_C_ULT = 53,
    R4300i_COP1_FUNCT_C_OLE = 54,
    R4300i_COP1_FUNCT_C_ULE = 55,
    R4300i_COP1_FUNCT_C_SF = 56,
    R4300i_COP1_FUNCT_C_NGLE = 57,
    R4300i_COP1_FUNCT_C_SEQ = 58,
    R4300i_COP1_FUNCT_C_NGL = 59,
    R4300i_COP1_FUNCT_C_LT = 60,
    R4300i_COP1_FUNCT_C_NGE = 61,
    R4300i_COP1_FUNCT_C_LE = 62,
    R4300i_COP1_FUNCT_C_NGT = 63,
};
