#pragma once
#include "Types.h"

#pragma warning(push)
#pragma warning(disable : 4201) // Non-standard extension used: nameless struct/union

typedef union tagOPCODE {
	uint32_t Hex;
	unsigned char Ascii[4];

	struct {
		unsigned immediate : 16;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned op : 6;
	};

	struct {
		unsigned offset : 16;
		unsigned : 5;
		unsigned base : 5;
		unsigned : 6;
	};

	struct {
		unsigned target : 26;
		unsigned : 6;
	};

	struct {
		unsigned funct : 6;
		unsigned sa : 5;
		unsigned rd : 5;
		unsigned : 5;
		unsigned : 5;
		unsigned : 6;
	};

	struct {
		signed   voffset : 7;
		unsigned del    : 4;
		unsigned : 5;
		unsigned dest   : 5;
		unsigned : 5;
		unsigned : 6;
	};
} OPCODE;

#pragma warning(pop)

// RSP opcodes

#define	RSP_SPECIAL				 0
#define	RSP_REGIMM				 1
#define RSP_J					 2
#define RSP_JAL					 3
#define RSP_BEQ					 4
#define RSP_BNE					 5
#define RSP_BLEZ				 6
#define RSP_BGTZ				 7
#define RSP_ADDI				 8
#define RSP_ADDIU				 9
#define RSP_SLTI				10
#define RSP_SLTIU				11
#define RSP_ANDI				12
#define RSP_ORI					13
#define RSP_XORI				14
#define RSP_LUI					15
#define	RSP_CP0					16
#define	RSP_CP2					18
#define RSP_LB					32
#define RSP_LH					33
#define RSP_LW					35
#define RSP_LBU					36
#define RSP_LHU					37
#define RSP_SB					40
#define RSP_SH					41
#define RSP_SW					43
#define RSP_LC2					50
#define RSP_SC2					58

// RSP special opcodes

#define RSP_SPECIAL_SLL			 0
#define RSP_SPECIAL_SRL			 2
#define RSP_SPECIAL_SRA			 3
#define RSP_SPECIAL_SLLV		 4
#define RSP_SPECIAL_SRLV		 6
#define RSP_SPECIAL_SRAV		 7
#define RSP_SPECIAL_JR			 8
#define RSP_SPECIAL_JALR		 9
#define RSP_SPECIAL_BREAK		13
#define RSP_SPECIAL_ADD			32
#define RSP_SPECIAL_ADDU		33
#define RSP_SPECIAL_SUB			34
#define RSP_SPECIAL_SUBU		35
#define RSP_SPECIAL_AND			36
#define RSP_SPECIAL_OR			37
#define RSP_SPECIAL_XOR			38
#define RSP_SPECIAL_NOR			39
#define RSP_SPECIAL_SLT			42
#define RSP_SPECIAL_SLTU		43

// RSP RegImm opcodes

#define RSP_REGIMM_BLTZ			 0
#define RSP_REGIMM_BGEZ			 1
#define RSP_REGIMM_BLTZAL		16
#define RSP_REGIMM_BGEZAL		17

// RSP COP0 opcodes

#define	RSP_COP0_MF				 0 
#define	RSP_COP0_MT				 4

// RSP COP2 opcodes

#define	RSP_COP2_MF				 0 
#define	RSP_COP2_CF				 2
#define	RSP_COP2_MT				 4 
#define	RSP_COP2_CT				 6

// RSP vector opcodes

#define	RSP_VECTOR_VMULF		 0
#define	RSP_VECTOR_VMULU		 1
#define	RSP_VECTOR_VRNDP		 2
#define	RSP_VECTOR_VMULQ		 3
#define	RSP_VECTOR_VMUDL		 4
#define	RSP_VECTOR_VMUDM		 5
#define	RSP_VECTOR_VMUDN		 6
#define	RSP_VECTOR_VMUDH		 7
#define	RSP_VECTOR_VMACF		 8
#define	RSP_VECTOR_VMACU		 9
#define	RSP_VECTOR_VRNDN		10
#define	RSP_VECTOR_VMACQ		11
#define	RSP_VECTOR_VMADL		12
#define	RSP_VECTOR_VMADM		13
#define	RSP_VECTOR_VMADN		14
#define	RSP_VECTOR_VMADH		15
#define	RSP_VECTOR_VADD			16
#define	RSP_VECTOR_VSUB			17
#define	RSP_VECTOR_VABS			19
#define	RSP_VECTOR_VADDC		20
#define	RSP_VECTOR_VSUBC		21
#define	RSP_VECTOR_VSAW			29
#define	RSP_VECTOR_VLT			32
#define	RSP_VECTOR_VEQ			33
#define	RSP_VECTOR_VNE			34
#define	RSP_VECTOR_VGE			35
#define	RSP_VECTOR_VCL			36
#define	RSP_VECTOR_VCH			37
#define	RSP_VECTOR_VCR			38
#define	RSP_VECTOR_VMRG			39
#define	RSP_VECTOR_VAND			40
#define	RSP_VECTOR_VNAND		41
#define	RSP_VECTOR_VOR			42
#define	RSP_VECTOR_VNOR			43
#define	RSP_VECTOR_VXOR			44
#define	RSP_VECTOR_VNXOR		45
#define	RSP_VECTOR_VRCP			48
#define	RSP_VECTOR_VRCPL		49
#define	RSP_VECTOR_VRCPH		50
#define	RSP_VECTOR_VMOV			51
#define	RSP_VECTOR_VRSQ			52
#define	RSP_VECTOR_VRSQL		53
#define	RSP_VECTOR_VRSQH		54
#define	RSP_VECTOR_VNOOP		55

// RSP LSC2 opcodes

#define RSP_LSC2_BV				 0
#define RSP_LSC2_SV				 1
#define RSP_LSC2_LV				 2
#define RSP_LSC2_DV				 3
#define RSP_LSC2_QV				 4
#define RSP_LSC2_RV				 5
#define RSP_LSC2_PV				 6
#define RSP_LSC2_UV				 7
#define RSP_LSC2_HV				 8
#define RSP_LSC2_FV				 9
#define RSP_LSC2_WV				10
#define	RSP_LSC2_TV				11
