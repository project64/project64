#pragma once
#include <stdint.h>

#pragma warning(push)
#pragma warning(disable : 4201) // Non-standard extension used: nameless struct/union

union RSPOpcode
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
        signed voffset : 7;
        unsigned del : 4;
        unsigned : 5;
        unsigned dest : 5;
        unsigned : 5;
        unsigned : 6;
    };
};

#pragma warning(pop)

enum RSPOpCodes
{
    RSP_SPECIAL = 0,
    RSP_REGIMM = 1,
    RSP_J = 2,
    RSP_JAL = 3,
    RSP_BEQ = 4,
    RSP_BNE = 5,
    RSP_BLEZ = 6,
    RSP_BGTZ = 7,
    RSP_ADDI = 8,
    RSP_ADDIU = 9,
    RSP_SLTI = 10,
    RSP_SLTIU = 11,
    RSP_ANDI = 12,
    RSP_ORI = 13,
    RSP_XORI = 14,
    RSP_LUI = 15,
    RSP_CP0 = 16,
    RSP_CP2 = 18,
    RSP_LB = 32,
    RSP_LH = 33,
    RSP_LW = 35,
    RSP_LBU = 36,
    RSP_LHU = 37,
    RSP_SB = 40,
    RSP_SH = 41,
    RSP_SW = 43,
    RSP_LC2 = 50,
    RSP_SC2 = 58,
};

enum RSPSpecialCodes
{
    RSP_SPECIAL_SLL = 0,
    RSP_SPECIAL_SRL = 2,
    RSP_SPECIAL_SRA = 3,
    RSP_SPECIAL_SLLV = 4,
    RSP_SPECIAL_SRLV = 6,
    RSP_SPECIAL_SRAV = 7,
    RSP_SPECIAL_JR = 8,
    RSP_SPECIAL_JALR = 9,
    RSP_SPECIAL_BREAK = 13,
    RSP_SPECIAL_ADD = 32,
    RSP_SPECIAL_ADDU = 33,
    RSP_SPECIAL_SUB = 34,
    RSP_SPECIAL_SUBU = 35,
    RSP_SPECIAL_AND = 36,
    RSP_SPECIAL_OR = 37,
    RSP_SPECIAL_XOR = 38,
    RSP_SPECIAL_NOR = 39,
    RSP_SPECIAL_SLT = 42,
    RSP_SPECIAL_SLTU = 43,
};

enum RSPRegImmOpCodes
{
    RSP_REGIMM_BLTZ = 0,
    RSP_REGIMM_BGEZ = 1,
    RSP_REGIMM_BLTZAL = 16,
    RSP_REGIMM_BGEZAL = 17,
};

enum RSPCOP0OpCodes
{
    RSP_COP0_MF = 0,
    RSP_COP0_MT = 4,
};

enum RSPCOP2OpCodes
{
    RSP_COP2_MF = 0,
    RSP_COP2_CF = 2,
    RSP_COP2_MT = 4,
    RSP_COP2_CT = 6,
};

enum RSPVectorOpCodes
{
    RSP_VECTOR_VMULF = 0,
    RSP_VECTOR_VMULU = 1,
    RSP_VECTOR_VRNDP = 2,
    RSP_VECTOR_VMULQ = 3,
    RSP_VECTOR_VMUDL = 4,
    RSP_VECTOR_VMUDM = 5,
    RSP_VECTOR_VMUDN = 6,
    RSP_VECTOR_VMUDH = 7,
    RSP_VECTOR_VMACF = 8,
    RSP_VECTOR_VMACU = 9,
    RSP_VECTOR_VRNDN = 10,
    RSP_VECTOR_VMACQ = 11,
    RSP_VECTOR_VMADL = 12,
    RSP_VECTOR_VMADM = 13,
    RSP_VECTOR_VMADN = 14,
    RSP_VECTOR_VMADH = 15,
    RSP_VECTOR_VADD = 16,
    RSP_VECTOR_VSUB = 17,
    RSP_VECTOR_VABS = 19,
    RSP_VECTOR_VADDC = 20,
    RSP_VECTOR_VSUBC = 21,
    RSP_VECTOR_VSAW = 29,
    RSP_VECTOR_VLT = 32,
    RSP_VECTOR_VEQ = 33,
    RSP_VECTOR_VNE = 34,
    RSP_VECTOR_VGE = 35,
    RSP_VECTOR_VCL = 36,
    RSP_VECTOR_VCH = 37,
    RSP_VECTOR_VCR = 38,
    RSP_VECTOR_VMRG = 39,
    RSP_VECTOR_VAND = 40,
    RSP_VECTOR_VNAND = 41,
    RSP_VECTOR_VOR = 42,
    RSP_VECTOR_VNOR = 43,
    RSP_VECTOR_VXOR = 44,
    RSP_VECTOR_VNXOR = 45,
    RSP_VECTOR_VRCP = 48,
    RSP_VECTOR_VRCPL = 49,
    RSP_VECTOR_VRCPH = 50,
    RSP_VECTOR_VMOV = 51,
    RSP_VECTOR_VRSQ = 52,
    RSP_VECTOR_VRSQL = 53,
    RSP_VECTOR_VRSQH = 54,
    RSP_VECTOR_VNOP = 55,
};

enum RSPLSC2OpCodes
{
    RSP_LSC2_BV = 0,
    RSP_LSC2_SV = 1,
    RSP_LSC2_LV = 2,
    RSP_LSC2_DV = 3,
    RSP_LSC2_QV = 4,
    RSP_LSC2_RV = 5,
    RSP_LSC2_PV = 6,
    RSP_LSC2_UV = 7,
    RSP_LSC2_HV = 8,
    RSP_LSC2_FV = 9,
    RSP_LSC2_WV = 10,
    RSP_LSC2_TV = 11,
};