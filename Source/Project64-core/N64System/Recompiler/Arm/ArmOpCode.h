#pragma once
#if defined(__arm__) || defined(_M_ARM) || defined(_M_ARM64)

#pragma warning(push)
#pragma warning(disable : 4201) // warning C4201: nonstandard extension used : nameless struct/union

union ArmThumbOpcode
{
    uint16_t Hex;
    uint8_t Ascii[2];

    struct
    {
        unsigned reserved : 3;
        unsigned rm : 4;
        unsigned opcode : 9;
    } Branch;

    struct
    {
        unsigned imm : 11;
        unsigned opcode : 5;
    } BranchImm;

    struct
    {
        unsigned imm : 8;
        unsigned cond : 4;
        unsigned opcode : 4;
    } BranchImmCond;

    struct
    {
        unsigned rt : 3;
        unsigned rn : 3;
        unsigned rm : 3;
        unsigned opcode : 7;
    } Reg;

    struct
    {
        unsigned rd : 3;
        unsigned rn : 3;
        unsigned imm3 : 3;
        unsigned opcode : 7;
    } Imm3;

    struct
    {
        unsigned rt : 3;
        unsigned rn : 3;
        unsigned imm5 : 5;
        unsigned opcode : 5;
    } Imm5;

    struct
    {
        unsigned imm8 : 8;
        unsigned rdn : 3;
        unsigned opcode : 5;
    } Imm8;

    struct
    {
        unsigned rn : 3;
        unsigned rm : 3;
        unsigned opcode : 10;
    } Reg2;

    struct
    {
        unsigned register_list : 8;
        unsigned m : 1;
        unsigned opcode : 7;
    } Push;

    struct
    {
        unsigned register_list : 8;
        unsigned p : 1;
        unsigned opcode : 7;
    } Pop;

    struct
    {
        unsigned mask : 4;
        unsigned firstcond : 4;
        unsigned opcode : 8;
    } It;
};

union Arm32Opcode
{
    uint32_t Hex;
    uint8_t Ascii[4];

    // uint16 + uint16 type instuction
    struct
    {
        unsigned rn : 4;
        unsigned opcode : 12;
        unsigned rm : 4;
        unsigned imm2 : 2;
        unsigned reserved : 6;
        unsigned rt : 4;
    } uint16;

    struct
    {
        unsigned Rn : 4;
        unsigned s : 1;
        unsigned opcode : 5;
        unsigned i : 1;
        unsigned opcode2 : 5;

        unsigned imm8 : 8;
        unsigned rd : 4;
        unsigned imm3 : 3;
        unsigned reserved : 1;
    } RnRdImm12;

    struct
    {
        unsigned Rn : 4;
        unsigned op3 : 2;
        unsigned D : 1;
        unsigned U : 1;
        unsigned op2 : 8;

        unsigned imm8 : 8;
        unsigned op1 : 4;
        unsigned vd : 4;
    } RnVdImm8;

    struct
    {
        unsigned vn : 4;
        unsigned op1 : 2;
        unsigned d : 1;
        unsigned op2 : 9;

        unsigned vm : 4;
        unsigned op3 : 1;
        unsigned m : 1;
        unsigned op4 : 1;
        unsigned n : 1;
        unsigned sz : 1;
        unsigned op5 : 3;
        unsigned vd : 4;
    } VnVmVd;

    struct
    {
        unsigned rn : 4;
        unsigned opcode : 12;

        unsigned rm : 4;
        unsigned imm : 2;
        unsigned Opcode2 : 6;
        unsigned rt : 4;
    } imm2;

    struct
    {
        unsigned rn : 4;
        unsigned s : 1;
        unsigned opcode : 11;

        unsigned rm : 4;
        unsigned type : 2;
        unsigned imm2 : 2;
        unsigned rd : 4;
        unsigned imm3 : 3;
        unsigned opcode2 : 1;
    } imm5;

    struct
    {
        unsigned rn : 4;
        unsigned opcode : 12;
        unsigned imm : 12;
        unsigned rt : 4;
    } imm12;

    struct
    {
        unsigned rn : 4;
        unsigned s : 1;
        unsigned opcode : 5;
        unsigned i : 1;
        unsigned opcode2 : 5;

        unsigned imm8 : 8;
        unsigned rd : 4;
        unsigned imm3 : 3;
        unsigned opcode3 : 1;
    } imm8_3_1;

    struct
    {
        unsigned imm4 : 4;
        unsigned opcode2 : 6;
        unsigned i : 1;
        unsigned opcode : 5;
        unsigned imm8 : 8;
        unsigned rd : 4;
        unsigned imm3 : 3;
        unsigned reserved : 1;
    } imm16;

    struct
    {
        unsigned imm6 : 6;
        unsigned cond : 4;
        unsigned S : 1;
        unsigned Opcode : 5;

        unsigned imm11 : 11;
        unsigned J2 : 1;
        unsigned val12 : 1;
        unsigned J1 : 1;
        unsigned val14 : 2;
    } Branch20;

    struct
    {
        unsigned rm : 4;
        unsigned opcode2 : 8;
        unsigned rt : 4;
        unsigned rn : 4;
        unsigned opcode : 12;
    } uint32;

    struct
    {
        unsigned rm : 4;
        unsigned opcode3 : 8;
        unsigned rt : 4;
        unsigned rn : 4;
        unsigned opcode2 : 1;
        unsigned w : 1;
        unsigned opcode1 : 1;
        unsigned u : 1;
        unsigned p : 1;
        unsigned opcode : 3;
        unsigned cond : 4;
    } reg_cond;

    struct
    {
        unsigned rm : 4;
        unsigned opcode3 : 1;
        unsigned type : 2;
        unsigned imm5 : 5;
        unsigned rt : 4;
        unsigned rn : 4;
        unsigned opcode2 : 1;
        unsigned w : 1;
        unsigned opcode1 : 1;
        unsigned u : 1;
        unsigned p : 1;
        unsigned opcode : 3;
        unsigned cond : 4;
    } reg_cond_imm5;

    struct
    {
        unsigned imm4l : 4;
        unsigned opcode3 : 4;
        unsigned imm4h : 4;
        unsigned rt : 4;
        unsigned rn : 4;
        unsigned opcode2 : 1;
        unsigned w : 1;
        unsigned opcode1 : 1;
        unsigned u : 1;
        unsigned p : 1;
        unsigned opcode : 3;
        unsigned cond : 4;
    } reg_cond_imm8;

    struct
    {
        unsigned imm12 : 12;
        unsigned rt : 4;
        unsigned rn : 4;
        unsigned opcode2 : 1;
        unsigned w : 1;
        unsigned opcode1 : 1;
        unsigned u : 1;
        unsigned p : 1;
        unsigned opcode : 3;
        unsigned cond : 4;
    } reg_cond_imm12;

    struct
    {
        unsigned opcode : 16;

        unsigned opcode2 : 12;
        unsigned rt : 4;
    } fpscr;

    struct
    {
        unsigned opcode : 16;

        unsigned rm : 4;
        unsigned rotate : 2;
        unsigned opcode2 : 2;
        unsigned rd : 4;
        unsigned opcode3 : 4;
    } rotate;

    struct
    {
        unsigned opcode : 16;
        unsigned register_list : 16;
    } PushPop;
};
#pragma warning(pop)

enum ArmThumbOpCodes
{
    ArmSTR_ThumbImm = 0xC,
    ArmSTR_Reg = 40,
    ArmLDR_ThumbImm = 0xD,
    ArmLDR_Reg = 44,
    ArmLDRH_Reg = 0xE19,
    ArmLDRH_W = 0xF83,
    ArmMOV_IMM16 = 0x1E,
    ArmMOVW_IMM16 = 0x24,
    ArmMOVT_IMM16 = 0x2C,
    ArmPUSH = 0x5A,
    ArmPOP = 0x5E,
};

#endif