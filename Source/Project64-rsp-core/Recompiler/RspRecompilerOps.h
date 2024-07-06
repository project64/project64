// Opcode functions

void Compile_SPECIAL(void);
void Compile_REGIMM(void);
void Compile_J(void);
void Compile_JAL(void);
void Compile_BEQ(void);
void Compile_BNE(void);
void Compile_BLEZ(void);
void Compile_BGTZ(void);
void Compile_ADDI(void);
void Compile_ADDIU(void);
void Compile_SLTI(void);
void Compile_SLTIU(void);
void Compile_ANDI(void);
void Compile_ORI(void);
void Compile_XORI(void);
void Compile_LUI(void);
void Compile_COP0(void);
void Compile_COP2(void);
void Compile_LB(void);
void Compile_LH(void);
void Compile_LW(void);
void Compile_LBU(void);
void Compile_LHU(void);
void Compile_LWU(void);
void Compile_SB(void);
void Compile_SH(void);
void Compile_SW(void);
void Compile_LC2(void);
void Compile_SC2(void);

// R4300i Opcodes: Special

void Compile_Special_SLL(void);
void Compile_Special_SRL(void);
void Compile_Special_SRA(void);
void Compile_Special_SLLV(void);
void Compile_Special_SRLV(void);
void Compile_Special_SRAV(void);
void Compile_Special_JR(void);
void Compile_Special_JALR(void);
void Compile_Special_BREAK(void);
void Compile_Special_ADD(void);
void Compile_Special_ADDU(void);
void Compile_Special_SUB(void);
void Compile_Special_SUBU(void);
void Compile_Special_AND(void);
void Compile_Special_OR(void);
void Compile_Special_XOR(void);
void Compile_Special_NOR(void);
void Compile_Special_SLT(void);
void Compile_Special_SLTU(void);

// R4300i Opcodes: RegImm

void Compile_RegImm_BLTZ(void);
void Compile_RegImm_BGEZ(void);
void Compile_RegImm_BLTZAL(void);
void Compile_RegImm_BGEZAL(void);

// COP0 functions

void Compile_Cop0_MF(void);
void Compile_Cop0_MT(void);

// COP2 functions

void Compile_Cop2_MF(void);
void Compile_Cop2_CF(void);
void Compile_Cop2_MT(void);
void Compile_Cop2_CT(void);
void Compile_COP2_VECTOR(void);

// Vector functions

void Compile_Vector_VMULF(void);
void Compile_Vector_VMULU(void);
void Compile_Vector_VRNDN(void);
void Compile_Vector_VRNDP(void);
void Compile_Vector_VMULQ(void);
void Compile_Vector_VMUDL(void);
void Compile_Vector_VMUDM(void);
void Compile_Vector_VMUDN(void);
void Compile_Vector_VMUDH(void);
void Compile_Vector_VMACF(void);
void Compile_Vector_VMACU(void);
void Compile_Vector_VMACQ(void);
void Compile_Vector_VMADL(void);
void Compile_Vector_VMADM(void);
void Compile_Vector_VMADN(void);
void Compile_Vector_VMADH(void);
void Compile_Vector_VADD(void);
void Compile_Vector_VSUB(void);
void Compile_Vector_VABS(void);
void Compile_Vector_VADDC(void);
void Compile_Vector_VSUBC(void);
void Compile_Vector_VSAW(void);
void Compile_Vector_VLT(void);
void Compile_Vector_VEQ(void);
void Compile_Vector_VNE(void);
void Compile_Vector_VGE(void);
void Compile_Vector_VCL(void);
void Compile_Vector_VCH(void);
void Compile_Vector_VCR(void);
void Compile_Vector_VMRG(void);
void Compile_Vector_VAND(void);
void Compile_Vector_VNAND(void);
void Compile_Vector_VOR(void);
void Compile_Vector_VNOR(void);
void Compile_Vector_VXOR(void);
void Compile_Vector_VNXOR(void);
void Compile_Vector_VRCP(void);
void Compile_Vector_VRCPL(void);
void Compile_Vector_VRCPH(void);
void Compile_Vector_VMOV(void);
void Compile_Vector_VRSQ(void);
void Compile_Vector_VRSQL(void);
void Compile_Vector_VRSQH(void);
void Compile_Vector_VNOOP(void);
void Compile_Vector_Reserved(void);

// LC2 functions
void Compile_Opcode_LBV(void);
void Compile_Opcode_LSV(void);
void Compile_Opcode_LLV(void);
void Compile_Opcode_LDV(void);
void Compile_Opcode_LQV(void);
void Compile_Opcode_LRV(void);
void Compile_Opcode_LPV(void);
void Compile_Opcode_LUV(void);
void Compile_Opcode_LHV(void);
void Compile_Opcode_LFV(void);
void Compile_Opcode_LWV(void);
void Compile_Opcode_LTV(void);

// SC2 functions
void Compile_Opcode_SBV(void);
void Compile_Opcode_SSV(void);
void Compile_Opcode_SLV(void);
void Compile_Opcode_SDV(void);
void Compile_Opcode_SQV(void);
void Compile_Opcode_SRV(void);
void Compile_Opcode_SPV(void);
void Compile_Opcode_SUV(void);
void Compile_Opcode_SHV(void);
void Compile_Opcode_SFV(void);
void Compile_Opcode_SWV(void);
void Compile_Opcode_STV(void);

// Other functions

void Compile_UnknownOpcode(void);
