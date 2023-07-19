// Opcode functions

void RSP_Opcode_SPECIAL(void);
void RSP_Opcode_REGIMM(void);
void RSP_Opcode_J(void);
void RSP_Opcode_JAL(void);
void RSP_Opcode_BEQ(void);
void RSP_Opcode_BNE(void);
void RSP_Opcode_BLEZ(void);
void RSP_Opcode_BGTZ(void);
void RSP_Opcode_ADDI(void);
void RSP_Opcode_ADDIU(void);
void RSP_Opcode_SLTI(void);
void RSP_Opcode_SLTIU(void);
void RSP_Opcode_ANDI(void);
void RSP_Opcode_ORI(void);
void RSP_Opcode_XORI(void);
void RSP_Opcode_LUI(void);
void RSP_Opcode_COP0(void);
void RSP_Opcode_COP2(void);
void RSP_Opcode_LB(void);
void RSP_Opcode_LH(void);
void RSP_Opcode_LW(void);
void RSP_Opcode_LBU(void);
void RSP_Opcode_LHU(void);
void RSP_Opcode_LWU(void);
void RSP_Opcode_SB(void);
void RSP_Opcode_SH(void);
void RSP_Opcode_SW(void);
void RSP_Opcode_LC2(void);
void RSP_Opcode_SC2(void);

// R4300i Opcodes: Special

void RSP_Special_SLL(void);
void RSP_Special_SRL(void);
void RSP_Special_SRA(void);
void RSP_Special_SLLV(void);
void RSP_Special_SRLV(void);
void RSP_Special_SRAV(void);
void RSP_Special_JR(void);
void RSP_Special_JALR(void);
void RSP_Special_BREAK(void);
void RSP_Special_ADD(void);
void RSP_Special_ADDU(void);
void RSP_Special_SUB(void);
void RSP_Special_SUBU(void);
void RSP_Special_AND(void);
void RSP_Special_OR(void);
void RSP_Special_XOR(void);
void RSP_Special_NOR(void);
void RSP_Special_SLT(void);
void RSP_Special_SLTU(void);

// R4300i Opcodes: RegImm

void RSP_Opcode_BLTZ(void);
void RSP_Opcode_BGEZ(void);
void RSP_Opcode_BLTZAL(void);
void RSP_Opcode_BGEZAL(void);

// COP0 functions

void RSP_Cop0_MF(void);
void RSP_Cop0_MT(void);

// COP2 functions

void RSP_Cop2_MF(void);
void RSP_Cop2_CF(void);
void RSP_Cop2_MT(void);
void RSP_Cop2_CT(void);
void RSP_COP2_VECTOR(void);

// Vector functions

void RSP_Vector_VMULF(void);
void RSP_Vector_VMULU(void);
void RSP_Vector_VMUDL(void);
void RSP_Vector_VMUDM(void);
void RSP_Vector_VMUDN(void);
void RSP_Vector_VMUDH(void);
void RSP_Vector_VMACF(void);
void RSP_Vector_VMACU(void);
void RSP_Vector_VMACQ(void);
void RSP_Vector_VMADL(void);
void RSP_Vector_VMADM(void);
void RSP_Vector_VMADN(void);
void RSP_Vector_VMADH(void);
void RSP_Vector_VADD(void);
void RSP_Vector_VSUB(void);
void RSP_Vector_VABS(void);
void RSP_Vector_VADDC(void);
void RSP_Vector_VSUBC(void);
void RSP_Vector_VSAW(void);
void RSP_Vector_VLT(void);
void RSP_Vector_VEQ(void);
void RSP_Vector_VNE(void);
void RSP_Vector_VGE(void);
void RSP_Vector_VCL(void);
void RSP_Vector_VCH(void);
void RSP_Vector_VCR(void);
void RSP_Vector_VMRG(void);
void RSP_Vector_VAND(void);
void RSP_Vector_VNAND(void);
void RSP_Vector_VOR(void);
void RSP_Vector_VNOR(void);
void RSP_Vector_VXOR(void);
void RSP_Vector_VNXOR(void);
void RSP_Vector_VRCP(void);
void RSP_Vector_VRCPL(void);
void RSP_Vector_VRCPH(void);
void RSP_Vector_VMOV(void);
void RSP_Vector_VRSQ(void);
void RSP_Vector_VRSQL(void);
void RSP_Vector_VRSQH(void);
void RSP_Vector_VNOOP(void);

// LC2 functions

void RSP_Opcode_LBV(void);
void RSP_Opcode_LSV(void);
void RSP_Opcode_LLV(void);
void RSP_Opcode_LDV(void);
void RSP_Opcode_LQV(void);
void RSP_Opcode_LRV(void);
void RSP_Opcode_LPV(void);
void RSP_Opcode_LUV(void);
void RSP_Opcode_LHV(void);
void RSP_Opcode_LFV(void);
void RSP_Opcode_LWV(void);
void RSP_Opcode_LTV(void);

// LC2 functions

void RSP_Opcode_SBV(void);
void RSP_Opcode_SSV(void);
void RSP_Opcode_SLV(void);
void RSP_Opcode_SDV(void);
void RSP_Opcode_SQV(void);
void RSP_Opcode_SRV(void);
void RSP_Opcode_SPV(void);
void RSP_Opcode_SUV(void);
void RSP_Opcode_SHV(void);
void RSP_Opcode_SFV(void);
void RSP_Opcode_STV(void);
void RSP_Opcode_SWV(void);

// Other functions

void rsp_UnknownOpcode(void);
