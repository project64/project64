#ifdef toremove

#ifndef __OPCODE_CLASS__H__
#define __OPCODE_CLASS__H__

//typedef struct {
//	DWORD VirtualAddress;
//
//	union {
//		unsigned long Hex;
//		unsigned char Ascii[4];
//		
//		struct {
//			unsigned offset : 16;
//			unsigned rt : 5;
//			unsigned rs : 5;
//			unsigned op : 6;
//		};
//
//		struct {
//			unsigned immediate : 16;
//			unsigned : 5;
//			unsigned base : 5;
//			unsigned : 6;
//		};
//		
//		struct {
//			unsigned target : 26;
//			unsigned : 6;
//		};
//		
//		struct {
//			unsigned funct : 6;
//			unsigned sa : 5;
//			unsigned rd : 5;
//			unsigned : 5;
//			unsigned : 5;
//			unsigned : 6;
//		};
//
//		struct {
//			unsigned : 6;
//			unsigned fd : 5;
//			unsigned fs : 5;
//			unsigned ft : 5;
//			unsigned fmt : 5;
//			unsigned : 6;
//		};	
//	};
//} OPCODE;

#include "OpCode Analysis Class.h"

enum StepType
{
   StepNormal,  StepDelaySlot, StepJump,
   
   //Recompiler Flags
   DoDelaySlot, InsideDelaySlot, DelaySlotDone, BranchCompiled
};

enum { OpCode_Size = 4};
enum PERM_LOOP { PermLoop_None, PermLoop_Jump, PermLoop_Delay };
class CRecompilerOps;

class COpcode : public COpcodeAnalysis {
	friend CRecompilerOps;      //Can manipulate how the opcode moves

	DWORD const m_OpLen;			//Length of the current opcode (MIPS will always be 4)	
	bool  const m_FixedOpcodeCount; //Is the opcode count fixed or is it variable
	float const m_OpcodeCount;      //how many cycles is the fixed opcode count

	StepType    m_NextStep;       //How to go to the next opcode, is a jump or the next opcode ?
	DWORD       m_JumpLocation;   //If the opcode is going to jump it will jump to this address	

public:
	//Constructor/deconstructor
	       COpcode     ( DWORD VirtualAddress );
	
	//Geting/changing details about the opcode stored
	bool   Next        ( void ); //move to the next opcode, if it is a jump returns true
	inline DWORD  PC   ( void ) { return m_opcode.VirtualAddress; } //current VAddr
	StepType NextStep  ( void ) { return m_NextStep; }
	void   SetJump     ( DWORD Target, bool Delay );
	bool   SetPC       ( DWORD VirtualAddress );

	//Info about the opcode
	DWORD  DelaySize   ( void ) { return m_OpLen; } //How big is the delay slot opcode
	float  CycleCount  ( void );
	inline bool   InDelaySlot ( void ) const { return m_NextStep != StepNormal; }

	//Contains the break down of the opcode
	OPCODE    m_opcode;

	//Flags	
	bool m_FlagSet;        //A Flag was set
	bool m_ExectionJumped; //Was the last next moved to a new location from a jump
	PERM_LOOP m_InPermLoop;     //Jumped to same address with delay slot not effecting the operation
};

#endif 

#endif