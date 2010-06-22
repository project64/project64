#include "stdafx.h"

#define PUTDST8(dest,value)  (*((BYTE *)(dest))=(BYTE)(value)); dest += 1;
#define PUTDST16(dest,value) (*((WORD *)(dest))=(WORD)(value)); dest += 2;
#define PUTDST32(dest,value) (*((DWORD *)(dest))=(DWORD)(value)); dest += 4;

BYTE * CX86Ops::m_RecompPos;

char CX86Ops::m_fpupop[2][2] = { 
	"", "p"
};

/**************************************************************************
* Logging Functions                                                       *
**************************************************************************/
void CX86Ops::WriteX86Comment ( LPCSTR Comment )
{
	CPU_Message("");
	CPU_Message("      // %s",Comment);
}

void CX86Ops::WriteX86Label ( LPCSTR Label ) 
{
	CPU_Message("");
	CPU_Message("      %s:",Label);
}

void CX86Ops::AdcX86regToVariable(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      adc dword ptr [%s], %s",VariableName, x86_Name(reg));
	PUTDST16(m_RecompPos,0x0511 + (reg * 0x100));
    PUTDST32(m_RecompPos,Variable); 
}

void CX86Ops::AdcConstToVariable(void *Variable, const char * VariableName, BYTE Constant) {
	CPU_Message("      adc dword ptr [%s], %Xh", VariableName, Constant);
	PUTDST16(m_RecompPos,0x1583);
    PUTDST32(m_RecompPos,Variable);
	PUTDST8(m_RecompPos,Constant);
}

void CX86Ops::AdcConstToX86Reg (x86Reg reg, DWORD Const) {
	CPU_Message("      adc %s, %Xh",x86_Name(reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		PUTDST16(m_RecompPos,0xD081 + (reg * 0x100));
		PUTDST32(m_RecompPos, Const);
	} else {
		PUTDST16(m_RecompPos,0xD083 + (reg * 0x100));
		PUTDST8(m_RecompPos, Const);
	}
}

void CX86Ops::AdcVariableToX86reg(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      adc %s, dword ptr [%s]",x86_Name(reg),VariableName);
	PUTDST16(m_RecompPos,0x0513 + (reg * 0x800));
    PUTDST32(m_RecompPos,Variable); 
}

void CX86Ops::AdcX86RegToX86Reg(x86Reg Destination, x86Reg Source) {
	CPU_Message("      adc %s, %s",x86_Name(Destination),x86_Name(Source));
	PUTDST16(m_RecompPos,0xC013 + (Source * 0x100) + (Destination * 0x800));
}

void CX86Ops::AddConstToVariable (DWORD Const, void *Variable, const char * VariableName) {
	CPU_Message("      add dword ptr [%s], 0x%X",VariableName, Const);\
	PUTDST16(m_RecompPos,0x0581);
	PUTDST32(m_RecompPos,Variable);
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::AddConstToX86Reg (x86Reg reg, DWORD Const) {
	if (Const == 0)
	{
	} else if (Const == 1) {
		IncX86reg(reg);
	} else if (Const == 0xFFFFFFFF) {
		DecX86reg(reg);
	} else if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		CPU_Message("      add %s, %Xh",x86_Name(reg),Const);
		PUTDST16(m_RecompPos,0xC081 + (reg * 0x100));
		PUTDST32(m_RecompPos, Const);
	} else {
		CPU_Message("      add %s, %Xh",x86_Name(reg),Const);
		PUTDST16(m_RecompPos,0xC083 + (reg * 0x100));
		PUTDST8(m_RecompPos, Const);
	}
}

void CX86Ops::AddVariableToX86reg(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      add %s, dword ptr [%s]",x86_Name(reg),VariableName);
	PUTDST16(m_RecompPos,0x0503 + (reg * 0x800));
    PUTDST32(m_RecompPos,Variable); 
}

void CX86Ops::AddX86regToVariable(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      add dword ptr [%s], %s",VariableName, x86_Name(reg));
	PUTDST16(m_RecompPos,0x0501 + (reg * 0x800));
    PUTDST32(m_RecompPos,Variable); 
}

void CX86Ops::AddX86RegToX86Reg(x86Reg Destination, x86Reg Source) {
	CPU_Message("      add %s, %s",x86_Name(Destination),x86_Name(Source));
	PUTDST16(m_RecompPos,0xC003 + (Source * 0x100) + (Destination * 0x800));
}

void CX86Ops::AndConstToVariable (DWORD Const, void *Variable, const char * VariableName) 
{
	CPU_Message("      and dword ptr [%s], 0x%X",VariableName, Const);
	PUTDST16(m_RecompPos,0x2581);
	PUTDST32(m_RecompPos,Variable);
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::AndConstToX86Reg(x86Reg reg, DWORD Const) {
	CPU_Message("      and %s, %Xh",x86_Name(reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		PUTDST16(m_RecompPos,0xE081 + (reg * 0x100));
		PUTDST32(m_RecompPos, Const);
	} else {
		PUTDST16(m_RecompPos,0xE083 + (reg * 0x100));
		PUTDST8(m_RecompPos, Const);
	}
}

void CX86Ops::AndVariableDispToX86Reg(void *Variable, const char * VariableName, x86Reg reg, x86Reg AddrReg, Multipler Multiply)
{
	CPU_Message("      and %s, dword ptr [%s+%s*%i]",x86_Name(reg),VariableName, x86_Name(AddrReg), Multiply);
	
	PUTDST16(m_RecompPos,0x0423 + (reg * 0x800));
	PUTDST8(m_RecompPos,0x05 + CalcMultiplyCode(Multiply) + (AddrReg * 0x8)); 
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::AndVariableToX86Reg(void * Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      and %s, dword ptr [%s]",x86_Name(reg),VariableName);
	PUTDST16(m_RecompPos,0x0523 + (reg * 0x800));
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::AndX86RegToX86Reg(x86Reg Destination, x86Reg Source) {
	CPU_Message("      and %s, %s",x86_Name(Destination),x86_Name(Source));
	PUTDST16(m_RecompPos,0xC021 + (Destination * 0x100) + (Source * 0x800));
}

void CX86Ops::BreakPointNotification (const char * const FileName, const int LineNumber) 
{
	_Notify->BreakPoint(FileName,LineNumber);
}

void CX86Ops::X86HardBreakPoint (void)
{
	CPU_Message("      int 3");
	PUTDST8(m_RecompPos,0xCC);
}

void CX86Ops::X86BreakPoint (LPCSTR FileName, int LineNumber)
{
	Pushad();
	PushImm32("LineNumber",LineNumber);
	PushImm32("FileName",(DWORD)FileName);
	Call_Direct(BreakPointNotification,"BreakPointNotification");
	AddConstToX86Reg(x86_ESP, 8);
	Popad();
}

void CX86Ops::Call_Direct(void * FunctAddress, const char * FunctName)
{
	CPU_Message("      call offset %s",FunctName);
	PUTDST8(m_RecompPos,0xE8);
	PUTDST32(m_RecompPos,(DWORD)FunctAddress-(DWORD)m_RecompPos - 4);
}

void CX86Ops::Call_Indirect(void * FunctAddress, const char * FunctName)
{
	CPU_Message("      call [%s]",FunctName);
	PUTDST16(m_RecompPos,0x15FF);
	PUTDST32(m_RecompPos,FunctAddress);
}

void CX86Ops::CompConstToVariable(DWORD Const, void * Variable, const char * VariableName) {
	CPU_Message("      cmp dword ptr [%s], 0x%X",VariableName, Const);
	PUTDST16(m_RecompPos,0x3D81);
	PUTDST32(m_RecompPos,Variable);
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::CompConstToX86reg(x86Reg reg, DWORD Const) {
	if (Const == 0)
	{
		OrX86RegToX86Reg(reg,reg);
	} else {
		CPU_Message("      cmp %s, %Xh",x86_Name(reg),Const);
		if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) 
		{
			PUTDST16(m_RecompPos,0xF881 + (reg * 0x100));
			PUTDST32(m_RecompPos,Const);
		} else {
			PUTDST16(m_RecompPos,0xF883 + (reg * 0x100));
			PUTDST8(m_RecompPos, Const);
		}
	}
}

void CX86Ops::CompConstToX86regPointer(x86Reg reg, DWORD Const) {
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) 
	{
		CPU_Message("      cmp dword ptr [%s], %Xh",x86_Name(reg),Const);
		PUTDST16(m_RecompPos,0x3881 + (reg * 0x100));
		PUTDST32(m_RecompPos,Const);
	} else {
		CPU_Message("      cmp byte ptr [%s], %Xh",x86_Name(reg),Const);
		PUTDST16(m_RecompPos,0x3883 + (reg * 0x100));
		PUTDST8(m_RecompPos, Const);
	}
}

void CX86Ops::CompX86regToVariable(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      cmp %s, dword ptr [%s]",x86_Name(reg),VariableName);
	PUTDST16(m_RecompPos,0x053B + (reg * 0x800));
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::CompVariableToX86reg(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      cmp dword ptr [%s], %s",VariableName, x86_Name(reg));
	PUTDST16(m_RecompPos,0x0539 + (reg * 0x800));
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::CompX86RegToX86Reg(x86Reg Destination, x86Reg Source) {
	WORD x86Command;

	CPU_Message("      cmp %s, %s",x86_Name(Destination),x86_Name(Source));
	
	switch (Source) {
	case x86_EAX: x86Command = 0x003B; break;
	case x86_EBX: x86Command = 0x033B; break;
	case x86_ECX: x86Command = 0x013B; break;
	case x86_EDX: x86Command = 0x023B; break;
	case x86_ESI: x86Command = 0x063B; break;
	case x86_EDI: x86Command = 0x073B; break;
	case x86_ESP: x86Command = 0x043B; break;
	case x86_EBP: x86Command = 0x053B; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::DecX86reg(x86Reg reg) {
	CPU_Message("      dec %s",x86_Name(reg));

	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xC8FF); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xCBFF); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xC9FF); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xCAFF); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xCEFF); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xCFFF); break;
	case x86_ESP: PUTDST8 (m_RecompPos,0x4C);   break;
	case x86_EBP: PUTDST8 (m_RecompPos,0x4D);   break;
	default:
		DisplayError("DecX86reg\nUnknown x86 Register");
	}
}

void CX86Ops::DivX86reg(x86Reg reg) {
	CPU_Message("      div %s",x86_Name(reg));
	switch (reg) {
	case x86_EBX: PUTDST16(m_RecompPos,0xf3F7); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xf1F7); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xf2F7); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xf6F7); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xf7F7); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xf4F7); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xf5F7); break;
	default:
		DisplayError("divX86reg\nUnknown x86 Register");
	}
}

void CX86Ops::idivX86reg(x86Reg reg) {
	CPU_Message("      idiv %s",x86_Name(reg));\
	switch (reg) {
	case x86_EBX: PUTDST16(m_RecompPos,0xfbF7); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xf9F7); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xfaF7); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xfeF7); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xffF7); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xfcF7); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xfdF7); break;
	default:
		DisplayError("idivX86reg\nUnknown x86 Register");
	}
}

void CX86Ops::imulX86reg(x86Reg reg) {
	CPU_Message("      imul %s",x86_Name(reg));
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xE8F7); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xEBF7); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xE9F7); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xEAF7); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xEEF7); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xEFF7); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xECF7); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xEDF7); break;
	default:
		DisplayError("imulX86reg\nUnknown x86 Register");
	}
}

void CX86Ops::IncX86reg(x86Reg reg) {
	CPU_Message("      inc %s",x86_Name(reg));
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xC0FF); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xC3FF); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xC1FF); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xC2FF); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xC6FF); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xC7FF); break;
	case x86_ESP: PUTDST8 (m_RecompPos,0x44);   break;
	case x86_EBP: PUTDST8 (m_RecompPos,0x45);   break;
	default:
		DisplayError("IncX86reg\nUnknown x86 Register");
	}
}

void CX86Ops::JaeLabel8(const char * Label, BYTE Value) {
	CPU_Message("      jae $%s",Label);
	PUTDST8(m_RecompPos,0x73);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JaeLabel32(const char * Label,DWORD Value) {
	CPU_Message("      jae $%s",Label);
	PUTDST16(m_RecompPos,0x830F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JaLabel8(const char * Label, BYTE Value) {
	CPU_Message("      ja $%s",Label);
	PUTDST8(m_RecompPos,0x77);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JaLabel32(const char * Label,DWORD Value) {
	CPU_Message("      ja $%s",Label);
	PUTDST16(m_RecompPos,0x870F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JbLabel8(const char * Label, BYTE Value) {
	CPU_Message("      jb $%s",Label);
	PUTDST8(m_RecompPos,0x72);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JbLabel32(const char * Label,DWORD Value) {
	CPU_Message("      jb $%s",Label);
	PUTDST16(m_RecompPos,0x820F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JecxzLabel8(const char * Label, BYTE Value) {
	CPU_Message("      jecxz $%s",Label);
	PUTDST8(m_RecompPos,0xE3);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JeLabel8(const char * Label, BYTE Value) {
	CPU_Message("      je $%s",Label);
	PUTDST8(m_RecompPos,0x74);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JeLabel32(const char * Label,DWORD Value) {
	CPU_Message("      je $%s",Label);
	PUTDST16(m_RecompPos,0x840F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JgeLabel32(const char * Label,DWORD Value) {
	CPU_Message("      jge $%s",Label);
	PUTDST16(m_RecompPos,0x8D0F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JgLabel8(const char * Label, BYTE Value) {
	CPU_Message("      jg $%s",Label);
	PUTDST8(m_RecompPos,0x7F);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JgLabel32(const char * Label,DWORD Value) {
	CPU_Message("      jg $%s",Label);
	PUTDST16(m_RecompPos,0x8F0F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JleLabel8(const char * Label, BYTE Value) {
	CPU_Message("      jle $%s",Label);
	PUTDST8(m_RecompPos,0x7E);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JleLabel32(const char * Label,DWORD Value) {
	CPU_Message("      jle $%s",Label);
	PUTDST16(m_RecompPos,0x8E0F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JlLabel8(const char * Label, BYTE Value) {
	CPU_Message("      jl $%s",Label);
	PUTDST8(m_RecompPos,0x7C);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JlLabel32(const char * Label,DWORD Value) {
	CPU_Message("      jl $%s",Label);
	PUTDST16(m_RecompPos,0x8C0F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JmpDirectReg( x86Reg reg ) {
	CPU_Message("      jmp %s",x86_Name(reg));

	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xE0ff); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xE3ff); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xE1ff); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xE2ff); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xE6ff); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xE7ff); break;
	default:
		DisplayError("JmpDirectReg\nUnknown x86 Register");		
		break;
	}
}

void CX86Ops::JmpIndirectLabel32(const char * Label,DWORD location) {
	CPU_Message("      jmp dword ptr [%s]", Label);
	PUTDST16(m_RecompPos, 0x25ff);
	PUTDST32(m_RecompPos, location);
}

void CX86Ops::JmpIndirectReg( x86Reg reg ) {
	CPU_Message("      jmp dword ptr [%s]",x86_Name(reg));

	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x20ff); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x23ff); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x21ff); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x22ff); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x26ff); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x27ff); break;
	case x86_ESP: 
		PUTDST8(m_RecompPos,0xff);
		PUTDST16(m_RecompPos,0x2434); 
	/*	BreakPoint(__FILE__,__LINE__);  */
		break;		
	case x86_EBP: 
		PUTDST8(m_RecompPos,0xff);
		PUTDST16(m_RecompPos,0x0065); 
	/*	BreakPoint(__FILE__,__LINE__);  */
		break;
	}
}

void CX86Ops::JmpLabel8(const char * Label, BYTE Value) {
	CPU_Message("      jmp $%s",Label);
	PUTDST8(m_RecompPos,0xEB);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JmpLabel32(const char * Label, DWORD Value) {
	CPU_Message("      jmp $%s",Label);
	PUTDST8(m_RecompPos,0xE9);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JneLabel8(const char * Label, BYTE Value) {
	CPU_Message("      jne $%s",Label);
	PUTDST8(m_RecompPos,0x75);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JneLabel32(const char *Label, DWORD Value) {
	CPU_Message("      jne $%s",Label);
	PUTDST16(m_RecompPos,0x850F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JnsLabel8(const char * Label, BYTE Value) {
	CPU_Message("      jns $%s",Label);
	PUTDST8(m_RecompPos,0x79);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JnsLabel32(const char *Label, DWORD Value) {
	CPU_Message("      jns $%s",Label);
	PUTDST16(m_RecompPos,0x890F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JnzLabel8(const char *Label, BYTE Value) 
{
	CPU_Message("      jnz $%s",Label);
	PUTDST8(m_RecompPos,0x75);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JnzLabel32(const char *Label, DWORD Value) {
	CPU_Message("      jnz $%s",Label);
	PUTDST16(m_RecompPos,0x850F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JsLabel32(const char *Label, DWORD Value) {
	CPU_Message("      js $%s",Label);
	PUTDST16(m_RecompPos,0x880F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::JzLabel8(const char *Label, BYTE Value) 
{
	CPU_Message("      jz $%s",Label);
	PUTDST8(m_RecompPos,0x74);
	PUTDST8(m_RecompPos,Value);
}

void CX86Ops::JzLabel32(const char *Label, DWORD Value) {
	CPU_Message("      jz $%s",Label);
	PUTDST16(m_RecompPos,0x840F);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::LeaRegReg(x86Reg RegDest, x86Reg RegSrc, DWORD Const, Multipler multiplier)
{
	if (Const != 0)
	{
		CPU_Message("      lea %s, [%s*%i+%X]", x86_Name(RegDest), x86_Name(RegSrc), multiplier,Const);
	} else {
		CPU_Message("      lea %s, [%s*%i]", x86_Name(RegDest), x86_Name(RegSrc), multiplier);
	}

	PUTDST8(m_RecompPos,0x8D);
	PUTDST8(m_RecompPos,0x04 + (RegDest * 8));
	PUTDST8(m_RecompPos,0x05 + (RegSrc * 8) + CalcMultiplyCode(multiplier));
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::LeaRegReg2(x86Reg RegDest, x86Reg RegSrc, x86Reg RegSrc2, Multipler multiplier)
{
	CPU_Message("      lea %s, [%s+%s*%i]", x86_Name(RegDest), x86_Name(RegSrc), x86_Name(RegSrc2), multiplier);

	if (RegSrc2 == x86_ESP || RegSrc2 == x86_EBP)
	{
		DisplayError("CX86Ops::LeaRegReg2: %s is invalid for RegSrc2",x86_Name(RegSrc2));
		return;
	}
	PUTDST8(m_RecompPos,0x8D);
	PUTDST8(m_RecompPos,0x04 + (RegDest * 0x8));
	PUTDST8(m_RecompPos,0x05 + (RegSrc * 0x8) + RegSrc2+ CalcMultiplyCode(multiplier));
}

void CX86Ops::LeaSourceAndOffset(x86Reg x86DestReg, x86Reg x86SourceReg, int offset) {
	WORD x86Command;

	CPU_Message("      lea %s, [%s + %0Xh]",x86_Name(x86DestReg),x86_Name(x86SourceReg),offset);

//	if ((offset & 0xFFFFFF80) != 0 && (offset & 0xFFFFFF80) != 0xFFFFFF80) {
	if (1) {
		switch (x86DestReg) {
		case x86_EAX: x86Command = 0x808D; break;
		case x86_EBX: x86Command = 0x988D; break;
		case x86_ECX: x86Command = 0x888D; break;
		case x86_EDX: x86Command = 0x908D; break;
		case x86_ESI: x86Command = 0xB08D; break;
		case x86_EDI: x86Command = 0xB88D; break;
		case x86_ESP: x86Command = 0xA08D; break;
		case x86_EBP: x86Command = 0xA88D; break;
		default:
			DisplayError("LeaSourceAndOffset\nUnknown x86 Register");
		}
		switch (x86SourceReg) {
		case x86_EAX: x86Command += 0x0000; break;
		case x86_EBX: x86Command += 0x0300; break;
		case x86_ECX: x86Command += 0x0100; break;
		case x86_EDX: x86Command += 0x0200; break;
		case x86_ESI: x86Command += 0x0600; break;
		case x86_EDI: x86Command += 0x0700; break;
		case x86_ESP: x86Command += 0x0400; break;
		case x86_EBP: x86Command += 0x0500; break;
		default:
			DisplayError("LeaSourceAndOffset\nUnknown x86 Register");
		}
		PUTDST16(m_RecompPos,x86Command);
		PUTDST32(m_RecompPos,offset);
	} else {
		switch (x86DestReg) {
		case x86_EAX: x86Command = 0x408D; break;
		case x86_EBX: x86Command = 0x588D; break;
		case x86_ECX: x86Command = 0x488D; break;
		case x86_EDX: x86Command = 0x508D; break;
		case x86_ESI: x86Command = 0x708D; break;
		case x86_EDI: x86Command = 0x788D; break;
		case x86_ESP: x86Command = 0x608D; break;
		case x86_EBP: x86Command = 0x688D; break;
		default:
			DisplayError("LeaSourceAndOffset\nUnknown x86 Register");
		}
		switch (x86SourceReg) {
		case x86_EAX: x86Command += 0x0000; break;
		case x86_EBX: x86Command += 0x0300; break;
		case x86_ECX: x86Command += 0x0100; break;
		case x86_EDX: x86Command += 0x0200; break;
		case x86_ESI: x86Command += 0x0600; break;
		case x86_EDI: x86Command += 0x0700; break;
		case x86_ESP: x86Command += 0x0400; break;
		case x86_EBP: x86Command += 0x0500; break;
		default:
			DisplayError("LeaSourceAndOffset\nUnknown x86 Register");
		}
		PUTDST16(m_RecompPos,x86Command);
		PUTDST8(m_RecompPos,offset);
	}
}

void CX86Ops::MoveConstByteToN64Mem(BYTE Const, x86Reg AddrReg) {
	CPU_Message("      mov byte ptr [%s+N64mem], %Xh",x86_Name(AddrReg),Const);
	switch (AddrReg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x80C6); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x83C6); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x81C6); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x82C6); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x86C6); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x87C6); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x84C6); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x85C6); break;
	default:
		DisplayError("MoveConstByteToN64Mem\nUnknown x86 Register");
	}
	PUTDST32(m_RecompPos,_MMU->Rdram());
	PUTDST8(m_RecompPos,Const);
}

void CX86Ops::MoveConstByteToVariable (BYTE Const,void *Variable, const char * VariableName) {
	CPU_Message("      mov byte ptr [%s], %Xh",VariableName,Const);
	PUTDST16(m_RecompPos,0x05C6);
    PUTDST32(m_RecompPos,Variable);
    PUTDST8(m_RecompPos,Const);
}

void CX86Ops::MoveConstHalfToN64Mem(WORD Const, x86Reg AddrReg) {
	CPU_Message("      mov word ptr [%s+N64mem], %Xh",x86_Name(AddrReg),Const);
	PUTDST8(m_RecompPos,0x66);
	switch (AddrReg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x80C7); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x83C7); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x81C7); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x82C7); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x86C7); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x87C7); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x84C7); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x85C7); break;
	default:
		DisplayError("MoveConstToN64Mem\nUnknown x86 Register");
	}
	PUTDST32(m_RecompPos,_MMU->Rdram());
	PUTDST16(m_RecompPos,Const);
}

void CX86Ops::MoveConstHalfToVariable (WORD Const,void *Variable, const char * VariableName) {
	CPU_Message("      mov word ptr [%s], %Xh",VariableName,Const);
	PUTDST8(m_RecompPos,0x66);
	PUTDST16(m_RecompPos,0x05C7);
    PUTDST32(m_RecompPos,Variable);
    PUTDST16(m_RecompPos,Const);
}

void CX86Ops::MoveConstHalfToX86regPointer(WORD Const, x86Reg AddrReg1, x86Reg AddrReg2) {
	BYTE Param;

	CPU_Message("      mov word ptr [%s+%s],%Xh",x86_Name(AddrReg1), x86_Name(AddrReg2), Const);

	PUTDST8(m_RecompPos,0x66);
	PUTDST16(m_RecompPos,0x04C7);

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveConstToX86regPointer\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveConstToX86regPointer\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
    PUTDST16(m_RecompPos,Const); 
}

void CX86Ops::MoveConstToMemoryDisp (DWORD Const, x86Reg AddrReg, DWORD Disp) {
	CPU_Message("      mov dword ptr [%s+%Xh], %Xh",x86_Name(AddrReg),Disp,Const);
	switch (AddrReg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x80C7); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x83C7); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x81C7); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x82C7); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x86C7); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x87C7); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x84C7); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x85C7); break;
	default:
		DisplayError("MoveConstToN64Mem\nUnknown x86 Register");
	}
	PUTDST32(m_RecompPos,Disp);
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::MoveConstToN64Mem(DWORD Const, x86Reg AddrReg) {
	CPU_Message("      mov dword ptr [%s+N64mem], %Xh",x86_Name(AddrReg),Const);
	switch (AddrReg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x80C7); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x83C7); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x81C7); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x82C7); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x86C7); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x87C7); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x84C7); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x85C7); break;
	default:
		DisplayError("MoveConstToN64Mem\nUnknown x86 Register");
	}
	PUTDST32(m_RecompPos,_MMU->Rdram());
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::MoveConstToN64MemDisp (DWORD Const, x86Reg AddrReg, BYTE Disp) {
	CPU_Message("      mov dword ptr [%s+N64mem+%Xh], %Xh",x86_Name(AddrReg),Const,Disp);
	switch (AddrReg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x80C7); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x83C7); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x81C7); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x82C7); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x86C7); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x87C7); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x84C7); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x85C7); break;
	default:
		DisplayError("MoveConstToN64Mem\nUnknown x86 Register");
	}
	PUTDST32(m_RecompPos,_MMU->Rdram() + Disp);
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::MoveConstToVariable (DWORD Const,void *Variable, const char * VariableName) {
	CPU_Message("      mov dword ptr [%s], %Xh",VariableName,Const);
	PUTDST16(m_RecompPos,0x05C7);
    PUTDST32(m_RecompPos,Variable);
    PUTDST32(m_RecompPos,Const);
}

void CX86Ops::MoveConstToX86Pointer(DWORD Const, x86Reg X86Pointer) 
{
	CPU_Message("      mov dword ptr [%s], %Xh",x86_Name(X86Pointer),Const);
	PUTDST16(m_RecompPos,0x00C7 + (X86Pointer * 0x100));
    PUTDST32(m_RecompPos,Const);
}


void CX86Ops::MoveConstToX86reg(DWORD Const, x86Reg reg) {	
	if (Const == 0)
	{
		XorX86RegToX86Reg(reg,reg);
	} else {
		CPU_Message("      mov %s, %Xh",x86_Name(reg),Const);
		PUTDST16(m_RecompPos,0xC0C7 + (reg * 0x100));
		PUTDST32(m_RecompPos,Const); 
	}
}

void CX86Ops::MoveConstByteToX86regPointer(BYTE Const, x86Reg AddrReg1, x86Reg AddrReg2) {
	BYTE Param;

	CPU_Message("      mov byte ptr [%s+%s],%Xh",x86_Name(AddrReg1), x86_Name(AddrReg2), Const);

	PUTDST16(m_RecompPos,0x04C6);

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveConstByteToX86regPointer\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveConstByteToX86regPointer\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
    PUTDST8(m_RecompPos,Const); 
}

void CX86Ops::MoveConstToX86regPointer(DWORD Const, x86Reg AddrReg1, x86Reg AddrReg2) {
	BYTE Param;

	CPU_Message("      mov dword ptr [%s+%s],%Xh",x86_Name(AddrReg1), x86_Name(AddrReg2), Const);

	PUTDST16(m_RecompPos,0x04C7);

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveConstToX86regPointer\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveConstToX86regPointer\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
    PUTDST32(m_RecompPos,Const); 
}

void CX86Ops::MoveN64MemDispToX86reg(x86Reg reg, x86Reg AddrReg, BYTE Disp) {
	WORD x86Command;

	CPU_Message("      mov %s, dword ptr [%s+N64mem+%Xh]",x86_Name(reg),x86_Name(AddrReg),Disp);
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x008B; break;
	case x86_EBX: x86Command = 0x038B; break;
	case x86_ECX: x86Command = 0x018B; break;
	case x86_EDX: x86Command = 0x028B; break;
	case x86_ESI: x86Command = 0x068B; break;
	case x86_EDI: x86Command = 0x078B; break;
	case x86_ESP: x86Command = 0x048B; break;
	case x86_EBP: x86Command = 0x058B; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram() + Disp);
}

void CX86Ops::MoveN64MemToX86reg(x86Reg reg, x86Reg AddrReg) {
	WORD x86Command;

	CPU_Message("      mov %s, dword ptr [%s+N64mem]",x86_Name(reg),x86_Name(AddrReg));
	
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x008B; break;
	case x86_EBX: x86Command = 0x038B; break;
	case x86_ECX: x86Command = 0x018B; break;
	case x86_EDX: x86Command = 0x028B; break;
	case x86_ESI: x86Command = 0x068B; break;
	case x86_EDI: x86Command = 0x078B; break;
	case x86_ESP: x86Command = 0x048B; break;
	case x86_EBP: x86Command = 0x058B; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram());
}
	
void CX86Ops::MoveN64MemToX86regByte(x86Reg reg, x86Reg AddrReg) {
	WORD x86Command;

	CPU_Message("      mov %s, byte ptr [%s+N64mem]",x86_ByteName(reg),x86_Name(AddrReg));
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x008A; break;
	case x86_EBX: x86Command = 0x038A; break;
	case x86_ECX: x86Command = 0x018A; break;
	case x86_EDX: x86Command = 0x028A; break;
	case x86_ESI: x86Command = 0x068A; break;
	case x86_EDI: x86Command = 0x078A; break;
	case x86_ESP: x86Command = 0x048A; break;
	case x86_EBP: x86Command = 0x058A; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
/*	case x86_ESI: x86Command += 0xB000; break; */
/*	case x86_EDI: x86Command += 0xB800; break; */
/*	case x86_ESP: case x86_EBP: */
	default:
		DisplayError("MoveN64MemToX86regByte\nInvalid x86 Register");
		break;
	}
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::MoveN64MemToX86regHalf(x86Reg reg, x86Reg AddrReg) {
	WORD x86Command;

	CPU_Message("      mov %s, word ptr [%s+N64mem]",x86_HalfName(reg),x86_Name(AddrReg));
	
	PUTDST8(m_RecompPos,0x66);
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x008B; break;
	case x86_EBX: x86Command = 0x038B; break;
	case x86_ECX: x86Command = 0x018B; break;
	case x86_EDX: x86Command = 0x028B; break;
	case x86_ESI: x86Command = 0x068B; break;
	case x86_EDI: x86Command = 0x078B; break;
	case x86_ESP: x86Command = 0x048B; break;
	case x86_EBP: x86Command = 0x058B; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::MoveSxByteX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg) {
	BYTE Param;

	CPU_Message("      movsx %s, byte ptr [%s+%s]",x86_Name(reg),x86_Name(AddrReg1), x86_Name(AddrReg2));

	PUTDST16(m_RecompPos,0xBE0F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x04); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x1C); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x0C); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x14); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x34); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x3C); break;
	case x86_ESP: PUTDST8(m_RecompPos,0x24); break;
	case x86_EBP: PUTDST8(m_RecompPos,0x2C); break;
	default:
		DisplayError("MoveZxByteX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveZxByteX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveZxByteX86regPointerToX86reg\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
}

void CX86Ops::MoveSxHalfX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg) {
	BYTE Param;

	CPU_Message("      movsx %s, word ptr [%s+%s]",x86_Name(reg),x86_Name(AddrReg1), x86_Name(AddrReg2));

	PUTDST16(m_RecompPos,0xBF0F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x04); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x1C); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x0C); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x14); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x34); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x3C); break;
	case x86_ESP: PUTDST8(m_RecompPos,0x24); break;
	case x86_EBP: PUTDST8(m_RecompPos,0x2C); break;
	default:
		DisplayError("MoveZxHalfX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveZxHalfX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveZxHalfX86regPointerToX86reg\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
}

void CX86Ops::MoveSxN64MemToX86regByte(x86Reg reg, x86Reg AddrReg) {
	WORD x86Command;

	CPU_Message("      movsx %s, byte ptr [%s+Dmem]",x86_Name(reg),x86_Name(AddrReg));
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00BE; break;
	case x86_EBX: x86Command = 0x03BE; break;
	case x86_ECX: x86Command = 0x01BE; break;
	case x86_EDX: x86Command = 0x02BE; break;
	case x86_ESI: x86Command = 0x06BE; break;
	case x86_EDI: x86Command = 0x07BE; break;
	case x86_ESP: x86Command = 0x04BE; break;
	case x86_EBP: x86Command = 0x05BE; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	default:
		DisplayError("MoveSxN64MemToX86regByte\nInvalid x86 Register");
		break;
	}
	PUTDST8(m_RecompPos,0x0f);
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::MoveSxN64MemToX86regHalf(x86Reg reg, x86Reg AddrReg) {
	WORD x86Command;

	CPU_Message("      movsx %s, word ptr [%s+Dmem]",x86_Name(reg),x86_Name(AddrReg));

	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00BF; break;
	case x86_EBX: x86Command = 0x03BF; break;
	case x86_ECX: x86Command = 0x01BF; break;
	case x86_EDX: x86Command = 0x02BF; break;
	case x86_ESI: x86Command = 0x06BF; break;
	case x86_EDI: x86Command = 0x07BF; break;
	case x86_ESP: x86Command = 0x04BF; break;
	case x86_EBP: x86Command = 0x05BF; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}

	PUTDST8(m_RecompPos, 0x0f);
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::MoveSxVariableToX86regByte(void *Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      movsx %s, byte ptr [%s]",x86_Name(reg),VariableName);

	PUTDST16(m_RecompPos, 0xbe0f);

	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x05); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x1D); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x0D); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x15); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x35); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x3D); break;
	case x86_ESP: PUTDST8(m_RecompPos,0x25); break;
	case x86_EBP: PUTDST8(m_RecompPos,0x2D); break;
	default: DisplayError("MoveSxVariableToX86regHalf\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MoveSxVariableToX86regHalf(void *Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      movsx %s, word ptr [%s]",x86_Name(reg),VariableName);

	PUTDST16(m_RecompPos, 0xbf0f);

	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x05); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x1D); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x0D); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x15); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x35); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x3D); break;
	case x86_ESP: PUTDST8(m_RecompPos,0x25); break;
	case x86_EBP: PUTDST8(m_RecompPos,0x2D); break;
	default: DisplayError("MoveSxVariableToX86regHalf\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MoveVariableToX86reg(void *Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      mov %s, dword ptr [%s]",x86_Name(reg),VariableName);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x058B); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D8B); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D8B); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x158B); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x358B); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3D8B); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x258B); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2D8B); break;
	default: DisplayError("MoveVariableToX86reg\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MoveVariableDispToX86Reg(void *Variable, const char * VariableName, x86Reg reg, x86Reg AddrReg, int Multiplier) {
	int x;
	CPU_Message("      mov %s, dword ptr [%s+%s*%i]",x86_Name(reg),VariableName, x86_Name(AddrReg), Multiplier);
	
	PUTDST8(m_RecompPos,0x8B);

	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x04); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x1C); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x0C); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x14); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x34); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x3C); break;
	case x86_ESP: PUTDST8(m_RecompPos,0x24); break;
	case x86_EBP: PUTDST8(m_RecompPos,0x2C); break;	
	}

	/* put in shifter 2(01), 4(10), 8(11) */
	switch (Multiplier) {
	case 1: x = 0; break;
	case 2: x = 0x40; break;
	case 4: x = 0x80; break;
	case 8: x = 0xC0; break;
	default: DisplayError("Move\nInvalid x86 multiplier");
	}

	/* format xx|000000 */
	switch (AddrReg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x05|x); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x1D|x); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x0D|x); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x15|x); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x35|x); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x3D|x); break;
	case x86_ESP: PUTDST8(m_RecompPos,0x25|x); break;
	case x86_EBP: PUTDST8(m_RecompPos,0x2D|x); break;	
	}

	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MoveVariableToX86regByte(void *Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      mov %s, byte ptr [%s]",x86_ByteName(reg),VariableName);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x058A); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D8A); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D8A); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x158A); break;
	default: DisplayError("MoveVariableToX86regByte\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MoveVariableToX86regHalf(void *Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      mov %s, word ptr [%s]",x86_HalfName(reg),VariableName);
	PUTDST8(m_RecompPos,0x66);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x058B); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D8B); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D8B); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x158B); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x358B); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3D8B); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x258B); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2D8B); break;
	default: DisplayError("MoveVariableToX86reg\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MoveX86regByteToN64Mem(x86Reg reg, x86Reg AddrReg) {
	WORD x86Command;

	CPU_Message("      mov byte ptr [%s+N64mem], %s",x86_Name(AddrReg),x86_ByteName(reg));
	
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x0088; break;
	case x86_EBX: x86Command = 0x0388; break;
	case x86_ECX: x86Command = 0x0188; break;
	case x86_EDX: x86Command = 0x0288; break;
	case x86_ESI: x86Command = 0x0688; break;
	case x86_EDI: x86Command = 0x0788; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	}
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::MoveX86regByteToVariable(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      mov byte ptr [%s], %s",VariableName,x86_ByteName(reg));
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x0588); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D88); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D88); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x1588); break;
	default:
		DisplayError("MoveX86regByteToVariable\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MoveX86regByteToX86regPointer(x86Reg reg, x86Reg AddrReg1, x86Reg AddrReg2) {
	BYTE Param;

	CPU_Message("      mov byte ptr [%s+%s],%s",x86_Name(AddrReg1), x86_Name(AddrReg2), x86_ByteName(reg));

	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x0488); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1C88); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0C88); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x1488); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x3488); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3C88); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x2488); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2C88); break;
	default:
		DisplayError("MoveX86regToX86regPointer\nUnhandled x86 Register");
	}

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveX86regToX86regPointer\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveX86regByteToX86regPointer\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
}

void CX86Ops::MoveX86regHalfToN64Mem(x86Reg reg, x86Reg AddrReg) {
	WORD x86Command;

	CPU_Message("      mov word ptr [%s+N64mem], %s",x86_Name(AddrReg),x86_HalfName(reg));

	PUTDST8(m_RecompPos,0x66);
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x0089; break;
	case x86_EBX: x86Command = 0x0389; break;
	case x86_ECX: x86Command = 0x0189; break;
	case x86_EDX: x86Command = 0x0289; break;
	case x86_ESI: x86Command = 0x0689; break;
	case x86_EDI: x86Command = 0x0789; break;
	case x86_ESP: x86Command = 0x0489; break;
	case x86_EBP: x86Command = 0x0589; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::MoveX86regHalfToVariable(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      mov word ptr [%s], %s",VariableName,x86_HalfName(reg));
	PUTDST8(m_RecompPos,0x66);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x0589); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D89); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D89); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x1589); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x3589); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3D89); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x2589); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2D89); break;
	default:
		DisplayError("MoveX86regToVariable\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MoveX86regHalfToX86regPointer(x86Reg reg, x86Reg AddrReg1, x86Reg AddrReg2) {
	BYTE Param;

	CPU_Message("      mov word ptr [%s+%s],%s",x86_Name(AddrReg1), x86_Name(AddrReg2), x86_HalfName(reg));

	PUTDST8(m_RecompPos,0x66);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x0489); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1C89); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0C89); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x1489); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x3489); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3C89); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x2489); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2C89); break;
	default:
		DisplayError("MoveX86regHalfToX86regPointer\nUnhandled x86 Register");
	}

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveX86regHalfToX86regPointer\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveX86regHalfToX86regPointer\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
}

void CX86Ops::MoveX86PointerToX86reg(x86Reg reg, x86Reg X86Pointer) {
	WORD x86Command;

	CPU_Message("      mov %s, dword ptr [%s]",x86_Name(reg),x86_Name(X86Pointer));

	switch (X86Pointer) {
	case x86_EAX: x86Command = 0x008B; break;
	case x86_EBX: x86Command = 0x038B; break;
	case x86_ECX: x86Command = 0x018B; break;
	case x86_EDX: x86Command = 0x028B; break;
	case x86_ESI: x86Command = 0x068B; break;
	case x86_EDI: x86Command = 0x078B; break;
	}
	
	switch (reg) {
	case x86_EAX: x86Command += 0x0000; break;
	case x86_EBX: x86Command += 0x1800; break;
	case x86_ECX: x86Command += 0x0800; break;
	case x86_EDX: x86Command += 0x1000; break;
	case x86_ESI: x86Command += 0x3000; break;
	case x86_EDI: x86Command += 0x3800; break;
	case x86_ESP: x86Command += 0x2000; break;
	case x86_EBP: x86Command += 0x2800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::MoveX86PointerToX86regDisp(x86Reg reg, x86Reg X86Pointer, BYTE Disp) {
	WORD x86Command;

	CPU_Message("      mov %s, dword ptr [%s] + %d",x86_Name(reg),x86_Name(X86Pointer),Disp);

	switch (X86Pointer) {
	case x86_EAX: x86Command = 0x408B; break;
	case x86_EBX: x86Command = 0x438B; break;
	case x86_ECX: x86Command = 0x418B; break;
	case x86_EDX: x86Command = 0x428B; break;
	case x86_ESI: x86Command = 0x468B; break;
	case x86_EDI: x86Command = 0x478B; break;
	}
	
	switch (reg) {
	case x86_EAX: x86Command += 0x0000; break;
	case x86_EBX: x86Command += 0x1800; break;
	case x86_ECX: x86Command += 0x0800; break;
	case x86_EDX: x86Command += 0x1000; break;
	case x86_ESI: x86Command += 0x3000; break;
	case x86_EDI: x86Command += 0x3800; break;
	case x86_ESP: x86Command += 0x2000; break;
	case x86_EBP: x86Command += 0x2800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
	PUTDST8(m_RecompPos,Disp);
}

void CX86Ops::MoveX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg) {
	BYTE Param;

	CPU_Message("      mov %s, dword ptr [%s+%s]",x86_Name(reg),x86_Name(AddrReg1), x86_Name(AddrReg2));

	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x048B); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1C8B); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0C8B); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x148B); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x348B); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3C8B); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x248B); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2C8B); break;
	default:
		DisplayError("MoveX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveX86regPointerToX86reg\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
}

void CX86Ops::MoveX86regPointerToX86regDisp8(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg, BYTE offset) {
	BYTE Param;

	CPU_Message("      mov %s, dword ptr [%s+%s]",x86_Name(reg),x86_Name(AddrReg1), x86_Name(AddrReg2));

	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x448B); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x5C8B); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x4C8B); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x548B); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x748B); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x7C8B); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x648B); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x6C8B); break;
	default:
		DisplayError("MoveX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveX86regPointerToX86reg\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
	PUTDST8(m_RecompPos,offset);
}

void CX86Ops::MoveX86regToMemory(x86Reg reg, x86Reg AddrReg, DWORD Disp) {
	WORD x86Command;

	CPU_Message("      mov dword ptr [%s+%X], %s",x86_Name(AddrReg),Disp,x86_Name(reg));
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x0089; break;
	case x86_EBX: x86Command = 0x0389; break;
	case x86_ECX: x86Command = 0x0189; break;
	case x86_EDX: x86Command = 0x0289; break;
	case x86_ESI: x86Command = 0x0689; break;
	case x86_EDI: x86Command = 0x0789; break;
	case x86_ESP: x86Command = 0x0489; break;
	case x86_EBP: x86Command = 0x0589; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,Disp);
}

void CX86Ops::MoveX86regToN64Mem(x86Reg reg, x86Reg AddrReg) {
	WORD x86Command;

	CPU_Message("      mov dword ptr [%s+N64mem], %s",x86_Name(AddrReg),x86_Name(reg));\
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x0089; break;
	case x86_EBX: x86Command = 0x0389; break;
	case x86_ECX: x86Command = 0x0189; break;
	case x86_EDX: x86Command = 0x0289; break;
	case x86_ESI: x86Command = 0x0689; break;
	case x86_EDI: x86Command = 0x0789; break;
	case x86_ESP: x86Command = 0x0489; break;
	case x86_EBP: x86Command = 0x0589; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::MoveX86regToN64MemDisp(x86Reg reg, x86Reg AddrReg, BYTE Disp) {
	WORD x86Command;

	CPU_Message("      mov dword ptr [%s+N64mem+%d], %s",x86_Name(AddrReg),Disp,x86_Name(reg));\
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x0089; break;
	case x86_EBX: x86Command = 0x0389; break;
	case x86_ECX: x86Command = 0x0189; break;
	case x86_EDX: x86Command = 0x0289; break;
	case x86_ESI: x86Command = 0x0689; break;
	case x86_EDI: x86Command = 0x0789; break;
	case x86_ESP: x86Command = 0x0489; break;
	case x86_EBP: x86Command = 0x0589; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram()+Disp);
}

void CX86Ops::MoveX86regToVariable(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      mov dword ptr [%s], %s",VariableName,x86_Name(reg));
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x0589); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D89); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D89); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x1589); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x3589); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3D89); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x2589); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2D89); break;
	default:
		DisplayError("MoveX86regToVariable\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MoveX86RegToX86Reg(x86Reg Source, x86Reg Destination) {
	WORD x86Command;
	
	if (Source == Destination)
	{
		return;
	}
	CPU_Message("      mov %s, %s",x86_Name(Destination),x86_Name(Source));

	switch (Destination) {\
	case x86_EAX: x86Command = 0x0089; break;
	case x86_EBX: x86Command = 0x0389; break;
	case x86_ECX: x86Command = 0x0189; break;
	case x86_EDX: x86Command = 0x0289; break;
	case x86_ESI: x86Command = 0x0689; break;
	case x86_EDI: x86Command = 0x0789; break;
	case x86_ESP: x86Command = 0x0489; break;
	case x86_EBP: x86Command = 0x0589; break;
	}
	
	switch (Source) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::MoveX86regToX86Pointer(x86Reg reg, x86Reg X86Pointer) {
	WORD x86Command;

	CPU_Message("      mov dword ptr [%s], %s",x86_Name(X86Pointer),x86_Name(reg));

	switch (X86Pointer) {
	case x86_EAX: x86Command = 0x0089; break;
	case x86_EBX: x86Command = 0x0389; break;
	case x86_ECX: x86Command = 0x0189; break;
	case x86_EDX: x86Command = 0x0289; break;
	case x86_ESI: x86Command = 0x0689; break;
	case x86_EDI: x86Command = 0x0789; break;
	}
	
	switch (reg) {
	case x86_EAX: x86Command += 0x0000; break;
	case x86_EBX: x86Command += 0x1800; break;
	case x86_ECX: x86Command += 0x0800; break;
	case x86_EDX: x86Command += 0x1000; break;
	case x86_ESI: x86Command += 0x3000; break;
	case x86_EDI: x86Command += 0x3800; break;
	case x86_ESP: x86Command += 0x2000; break;
	case x86_EBP: x86Command += 0x2800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::MoveX86regToX86regPointer(x86Reg reg, x86Reg AddrReg1, x86Reg AddrReg2) {
	BYTE Param;

	CPU_Message("      mov dword ptr [%s+%s],%s",x86_Name(AddrReg1), x86_Name(AddrReg2), x86_Name(reg));

	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x0489); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1C89); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0C89); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x1489); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x3489); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3C89); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x2489); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2C89); break;
	default:
		DisplayError("MoveX86regToX86regPointer\nUnhandled x86 Register");
	}

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveX86regToX86regPointer\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveX86regToX86regPointer\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
}

void CX86Ops::MoveZxByteX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg) {
	BYTE Param;

	CPU_Message("      movzx %s, byte ptr [%s+%s]",x86_Name(reg),x86_Name(AddrReg1), x86_Name(AddrReg2));

	PUTDST16(m_RecompPos,0xB60F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x04); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x1C); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x0C); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x14); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x34); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x3C); break;
	case x86_ESP: PUTDST8(m_RecompPos,0x24); break;
	case x86_EBP: PUTDST8(m_RecompPos,0x2C); break;
	default:
		DisplayError("MoveZxByteX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveZxByteX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveZxByteX86regPointerToX86reg\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
}

void CX86Ops::MoveZxHalfX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg reg) {
	BYTE Param;

	CPU_Message("      movzx %s, word ptr [%s+%s]",x86_Name(reg),x86_Name(AddrReg1), x86_Name(AddrReg2));

	PUTDST16(m_RecompPos,0xB70F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x04); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x1C); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x0C); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x14); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x34); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x3C); break;
	case x86_ESP: PUTDST8(m_RecompPos,0x24); break;
	case x86_EBP: PUTDST8(m_RecompPos,0x2C); break;
	default:
		DisplayError("MoveZxHalfX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg1) {
	case x86_EAX: Param = 0x00; break;
	case x86_EBX: Param = 0x03; break;
	case x86_ECX: Param = 0x01; break;
	case x86_EDX: Param = 0x02; break;
	case x86_ESI: Param = 0x06; break;
	case x86_EDI: Param = 0x07; break;
	default:
		DisplayError("MoveZxHalfX86regPointerToX86reg\nUnhandled x86 Register");
	}

	switch (AddrReg2) {
	case x86_EAX: Param += 0x00; break;
	case x86_EBX: Param += 0x18; break;
	case x86_ECX: Param += 0x08; break;
	case x86_EDX: Param += 0x10; break;
	case x86_ESI: Param += 0x30; break;
	case x86_EDI: Param += 0x38; break;
	case x86_ESP: Param += 0x20; break;
	case x86_EBP: Param += 0x28; break;
	default:
		DisplayError("MoveZxHalfX86regPointerToX86reg\nUnhandled x86 Register");
	}
	PUTDST8(m_RecompPos,Param);
}

void CX86Ops::MoveZxN64MemToX86regByte(x86Reg reg, x86Reg AddrReg) {
	WORD x86Command;

	CPU_Message("      movzx %s, byte ptr [%s+_MMU->Rdram()]",x86_Name(reg),x86_Name(AddrReg));
	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00B6; break;
	case x86_EBX: x86Command = 0x03B6; break;
	case x86_ECX: x86Command = 0x01B6; break;
	case x86_EDX: x86Command = 0x02B6; break;
	case x86_ESI: x86Command = 0x06B6; break;
	case x86_EDI: x86Command = 0x07B6; break;
	case x86_ESP: x86Command = 0x04B6; break;
	case x86_EBP: x86Command = 0x05B6; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	default:
		DisplayError("MoveZxN64MemToX86regByte\nInvalid x86 Register");
		break;
	}
	PUTDST8(m_RecompPos,0x0f);
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::MoveZxN64MemToX86regHalf(x86Reg reg, x86Reg AddrReg) {
	WORD x86Command;

	CPU_Message("      movzx %s, word ptr [%s+_MMU->Rdram()]",x86_Name(reg),x86_Name(AddrReg));

	switch (AddrReg) {
	case x86_EAX: x86Command = 0x00B7; break;
	case x86_EBX: x86Command = 0x03B7; break;
	case x86_ECX: x86Command = 0x01B7; break;
	case x86_EDX: x86Command = 0x02B7; break;
	case x86_ESI: x86Command = 0x06B7; break;
	case x86_EDI: x86Command = 0x07B7; break;
	case x86_ESP: x86Command = 0x04B7; break;
	case x86_EBP: x86Command = 0x05B7; break;
	}
	switch (reg) {
	case x86_EAX: x86Command += 0x8000; break;
	case x86_EBX: x86Command += 0x9800; break;
	case x86_ECX: x86Command += 0x8800; break;
	case x86_EDX: x86Command += 0x9000; break;
	case x86_ESI: x86Command += 0xB000; break;
	case x86_EDI: x86Command += 0xB800; break;
	case x86_ESP: x86Command += 0xA000; break;
	case x86_EBP: x86Command += 0xA800; break;
	}

	PUTDST8(m_RecompPos, 0x0f);
	PUTDST16(m_RecompPos,x86Command);
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::MoveZxVariableToX86regByte(void *Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      movzx %s, byte ptr [%s]",x86_Name(reg),VariableName);

	PUTDST16(m_RecompPos, 0xb60f);

	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x05); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x1D); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x0D); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x15); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x35); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x3D); break;
	case x86_ESP: PUTDST8(m_RecompPos,0x25); break;
	case x86_EBP: PUTDST8(m_RecompPos,0x2D); break;
	default: DisplayError("MoveZxVariableToX86regHalf\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MoveZxVariableToX86regHalf(void *Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      movzx %s, word ptr [%s]",x86_Name(reg),VariableName);

	PUTDST16(m_RecompPos, 0xb70f);

	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x05); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x1D); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x0D); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x15); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x35); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x3D); break;
	case x86_ESP: PUTDST8(m_RecompPos,0x25); break;
	case x86_EBP: PUTDST8(m_RecompPos,0x2D); break;
	default: DisplayError("MoveZxVariableToX86regHalf\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::MulX86reg(x86Reg reg) {
	CPU_Message("      mul %s",x86_Name(reg));\
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xE0F7); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xE3F7); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xE1F7); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xE2F7); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xE6F7); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xE7F7); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xE4F7); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xE5F7); break;
	default:
		DisplayError("MulX86reg\nUnknown x86 Register");
	}
}

void CX86Ops::NotX86Reg(x86Reg reg) {
	CPU_Message("      not %s",x86_Name(reg));
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xD0F7); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xD3F7); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xD1F7); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xD2F7); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xD6F7); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xD7F7); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xD4F7); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xD5F7); break;
	}
}

void CX86Ops::OrConstToVariable(DWORD Const, void * Variable, const char * VariableName) {
	CPU_Message("      or dword ptr [%s], 0x%X",VariableName, Const);
	PUTDST16(m_RecompPos,0x0D81);
	PUTDST32(m_RecompPos,Variable);
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::OrConstToX86Reg(DWORD Const, x86Reg reg) {
	CPU_Message("      or %s, %Xh",x86_Name(reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		switch (reg) {
		case x86_EAX: PUTDST16(m_RecompPos,0xC881); break;
		case x86_EBX: PUTDST16(m_RecompPos,0xCB81); break;
		case x86_ECX: PUTDST16(m_RecompPos,0xC981); break;
		case x86_EDX: PUTDST16(m_RecompPos,0xCA81); break;
		case x86_ESI: PUTDST16(m_RecompPos,0xCE81); break;
		case x86_EDI: PUTDST16(m_RecompPos,0xCF81); break;
		case x86_ESP: PUTDST16(m_RecompPos,0xCC81); break;
		case x86_EBP: PUTDST16(m_RecompPos,0xCD81); break;
		}
		PUTDST32(m_RecompPos, Const);
	} else {
		switch (reg) {
		case x86_EAX: PUTDST16(m_RecompPos,0xC883); break;
		case x86_EBX: PUTDST16(m_RecompPos,0xCB83); break;
		case x86_ECX: PUTDST16(m_RecompPos,0xC983); break;
		case x86_EDX: PUTDST16(m_RecompPos,0xCA83); break;
		case x86_ESI: PUTDST16(m_RecompPos,0xCE83); break;
		case x86_EDI: PUTDST16(m_RecompPos,0xCF83); break;
		case x86_ESP: PUTDST16(m_RecompPos,0xCC83); break;
		case x86_EBP: PUTDST16(m_RecompPos,0xCD83); break;
		}
		PUTDST8(m_RecompPos, Const);
	}
}

void CX86Ops::OrVariableToX86Reg(void * Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      or %s, dword ptr [%s]",x86_Name(reg),VariableName);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x050B); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D0B); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D0B); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x150B); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x350B); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3D0B); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x250B); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2D0B); break;
	}
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::OrX86RegToVariable(void * Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      or dword ptr [%s], %s",VariableName, x86_Name(reg));
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x0509); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D09); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D09); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x1509); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x3509); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3D09); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x2509); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2D09); break;
	}
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::OrX86RegToX86Reg(x86Reg Destination, x86Reg Source) {
	WORD x86Command;

	CPU_Message("      or %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Source) {
	case x86_EAX: x86Command = 0x000B; break;
	case x86_EBX: x86Command = 0x030B; break;
	case x86_ECX: x86Command = 0x010B; break;
	case x86_EDX: x86Command = 0x020B; break;
	case x86_ESI: x86Command = 0x060B; break;
	case x86_EDI: x86Command = 0x070B; break;
	case x86_ESP: x86Command = 0x040B; break;
	case x86_EBP: x86Command = 0x050B; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::Popad(void) {
	CPU_Message("      popad");
	PUTDST8(m_RecompPos,0x61);
}

void CX86Ops::Pushad(void) {
	CPU_Message("      pushad");
	PUTDST8(m_RecompPos,0x60);
}

void CX86Ops::Push(x86Reg reg) {
	CPU_Message("      push %s", x86_Name(reg));

	switch(reg) {
	case x86_EAX: PUTDST8(m_RecompPos, 0x50); break;
	case x86_EBX: PUTDST8(m_RecompPos, 0x53); break;
	case x86_ECX: PUTDST8(m_RecompPos, 0x51); break;
	case x86_EDX: PUTDST8(m_RecompPos, 0x52); break;
	case x86_ESI: PUTDST8(m_RecompPos, 0x56); break;
	case x86_EDI: PUTDST8(m_RecompPos, 0x57); break;
	case x86_ESP: PUTDST8(m_RecompPos, 0x54); break;
	case x86_EBP: PUTDST8(m_RecompPos, 0x55); break;
	}
}

void CX86Ops::Pop(x86Reg reg) {
	CPU_Message("      pop %s", x86_Name(reg));

	switch(reg) {
	case x86_EAX: PUTDST8(m_RecompPos, 0x58); break;
	case x86_EBX: PUTDST8(m_RecompPos, 0x5B); break;
	case x86_ECX: PUTDST8(m_RecompPos, 0x59); break;
	case x86_EDX: PUTDST8(m_RecompPos, 0x5A); break;
	case x86_ESI: PUTDST8(m_RecompPos, 0x5E); break;
	case x86_EDI: PUTDST8(m_RecompPos, 0x5F); break;
	case x86_ESP: PUTDST8(m_RecompPos, 0x5C); break;
	case x86_EBP: PUTDST8(m_RecompPos, 0x5D); break;
	}
}

void CX86Ops::PushImm32(const char * String, DWORD Value) {
	CPU_Message("      push %s",String);
	PUTDST8(m_RecompPos,0x68);
	PUTDST32(m_RecompPos,Value);
}

void CX86Ops::Ret(void) {
	CPU_Message("      ret");
	PUTDST8(m_RecompPos,0xC3);
}

void CX86Ops::Seta(x86Reg reg) {
	CPU_Message("      seta %s",x86_ByteName(reg));
	PUTDST16(m_RecompPos,0x970F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0xC0); break;
	case x86_EBX: PUTDST8(m_RecompPos,0xC3); break;
	case x86_ECX: PUTDST8(m_RecompPos,0xC1); break;
	case x86_EDX: PUTDST8(m_RecompPos,0xC2); break;
	default:
		DisplayError("Seta\nUnknown x86 Register");
	}
}

void CX86Ops::SetaVariable(void * Variable, const char * VariableName) {
	CPU_Message("      seta byte ptr [%s]",VariableName);
	PUTDST16(m_RecompPos,0x970F);
	PUTDST8(m_RecompPos,0x05);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::Setae(x86Reg reg) {
	CPU_Message("      setae %s",x86_ByteName(reg));
	PUTDST16(m_RecompPos,0x930F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0xC0); break;
	case x86_EBX: PUTDST8(m_RecompPos,0xC3); break;
	case x86_ECX: PUTDST8(m_RecompPos,0xC1); break;
	case x86_EDX: PUTDST8(m_RecompPos,0xC2); break;
	default:
		DisplayError("Seta\nUnknown x86 Register");
	}
}

void CX86Ops::Setb(x86Reg reg) {
	CPU_Message("      setb %s",x86_ByteName(reg));
	PUTDST16(m_RecompPos,0x920F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0xC0); break;
	case x86_EBX: PUTDST8(m_RecompPos,0xC3); break;
	case x86_ECX: PUTDST8(m_RecompPos,0xC1); break;
	case x86_EDX: PUTDST8(m_RecompPos,0xC2); break;
	default:
		DisplayError("Setb\nUnknown x86 Register");
	}
}

void CX86Ops::SetbVariable(void * Variable, const char * VariableName) {
	CPU_Message("      setb byte ptr [%s]",VariableName);
	PUTDST16(m_RecompPos,0x920F);
	PUTDST8(m_RecompPos,0x05);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::Setg(x86Reg reg) {
	CPU_Message("      setg %s",x86_ByteName(reg));
	PUTDST16(m_RecompPos,0x9F0F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0xC0); break;
	case x86_EBX: PUTDST8(m_RecompPos,0xC3); break;
	case x86_ECX: PUTDST8(m_RecompPos,0xC1); break;
	case x86_EDX: PUTDST8(m_RecompPos,0xC2); break;
	default:
		DisplayError("Setg\nUnknown x86 Register");
	}
}

void CX86Ops::SetgVariable(void * Variable, const char * VariableName) {
	CPU_Message("      setg byte ptr [%s]",VariableName);
	PUTDST16(m_RecompPos,0x9F0F);
	PUTDST8(m_RecompPos,0x05);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::Setl(x86Reg reg) {
	CPU_Message("      setl %s",x86_ByteName(reg));
	PUTDST16(m_RecompPos,0x9C0F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0xC0); break;
	case x86_EBX: PUTDST8(m_RecompPos,0xC3); break;
	case x86_ECX: PUTDST8(m_RecompPos,0xC1); break;
	case x86_EDX: PUTDST8(m_RecompPos,0xC2); break;
	default:
		DisplayError("Setl\nUnknown x86 Register");
	}
}

void CX86Ops::SetlVariable(void * Variable, const char * VariableName) {
	CPU_Message("      setl byte ptr [%s]",VariableName);
	PUTDST16(m_RecompPos,0x9C0F);
	PUTDST8(m_RecompPos,0x05);
	PUTDST32(m_RecompPos,Variable);
}


void CX86Ops::Setz(x86Reg reg) {
	CPU_Message("      setz %s",x86_ByteName(reg));
	PUTDST16(m_RecompPos,0x940F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0xC0); break;
	case x86_EBX: PUTDST8(m_RecompPos,0xC3); break;
	case x86_ECX: PUTDST8(m_RecompPos,0xC1); break;
	case x86_EDX: PUTDST8(m_RecompPos,0xC2); break;
	default:
		DisplayError("Setz\nUnknown x86 Register");
	}
}

void CX86Ops::Setnz(x86Reg reg) {
	CPU_Message("      setnz %s",x86_ByteName(reg));
	PUTDST16(m_RecompPos,0x950F);
	switch (reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0xC0); break;
	case x86_EBX: PUTDST8(m_RecompPos,0xC3); break;
	case x86_ECX: PUTDST8(m_RecompPos,0xC1); break;
	case x86_EDX: PUTDST8(m_RecompPos,0xC2); break;
	default:
		DisplayError("Setnz\nUnknown x86 Register");
	}
}

void CX86Ops::ShiftLeftDouble(x86Reg Destination, x86Reg Source) {
	unsigned char s = 0xC0;

	CPU_Message("      shld %s, %s, cl", x86_Name(Destination),x86_Name(Source));
	PUTDST16(m_RecompPos,0xA50F);

	switch (Destination) {
	case x86_EAX: s |= 0x00; break;
	case x86_EBX: s |= 0x03; break;
	case x86_ECX: s |= 0x01; break;
	case x86_EDX: s |= 0x02; break;
	case x86_ESI: s |= 0x06; break;
	case x86_EDI: s |= 0x07; break;
	case x86_ESP: s |= 0x04; break;
	case x86_EBP: s |= 0x05; break;
	}

	switch (Source) {
	case x86_EAX: s |= 0x00 << 3; break;
	case x86_EBX: s |= 0x03 << 3; break;
	case x86_ECX: s |= 0x01 << 3; break;
	case x86_EDX: s |= 0x02 << 3; break;
	case x86_ESI: s |= 0x06 << 3; break;
	case x86_EDI: s |= 0x07 << 3; break;
	case x86_ESP: s |= 0x04 << 3; break;
	case x86_EBP: s |= 0x05 << 3; break;
	}

	PUTDST8(m_RecompPos,s);
}

void CX86Ops::ShiftLeftDoubleImmed(x86Reg Destination, x86Reg Source, BYTE Immediate) {
	unsigned char s = 0xC0;

	CPU_Message("      shld %s, %s, %Xh", x86_Name(Destination),x86_Name(Source), Immediate);
	PUTDST16(m_RecompPos,0xA40F);

	switch (Destination) {
	case x86_EAX: s |= 0x00; break;
	case x86_EBX: s |= 0x03; break;
	case x86_ECX: s |= 0x01; break;
	case x86_EDX: s |= 0x02; break;
	case x86_ESI: s |= 0x06; break;
	case x86_EDI: s |= 0x07; break;
	case x86_ESP: s |= 0x04; break;
	case x86_EBP: s |= 0x05; break;
	}

	switch (Source) {
	case x86_EAX: s |= 0x00 << 3; break;
	case x86_EBX: s |= 0x03 << 3; break;
	case x86_ECX: s |= 0x01 << 3; break;
	case x86_EDX: s |= 0x02 << 3; break;
	case x86_ESI: s |= 0x06 << 3; break;
	case x86_EDI: s |= 0x07 << 3; break;
	case x86_ESP: s |= 0x04 << 3; break;
	case x86_EBP: s |= 0x05 << 3; break;
	}

	PUTDST8(m_RecompPos,s);
	PUTDST8(m_RecompPos,Immediate);
}

void CX86Ops::ShiftLeftSign(x86Reg reg) {
	CPU_Message("      shl %s, cl",x86_Name(reg));
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xE0D3); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xE3D3); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xE1D3); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xE2D3); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xE6D3); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xE7D3); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xE4D3); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xE5D3); break;
	}
}

void CX86Ops::ShiftLeftSignImmed(x86Reg reg, BYTE Immediate) {
	CPU_Message("      shl %s, %Xh",x86_Name(reg),Immediate);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xE0C1); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xE3C1); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xE1C1); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xE2C1); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xE6C1); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xE7C1); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xE4C1); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xE5C1); break;
	}
	PUTDST8(m_RecompPos,Immediate);
}

void CX86Ops::ShiftRightSign(x86Reg reg) {
	CPU_Message("      sar %s, cl",x86_Name(reg));
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xF8D3); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xFBD3); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xF9D3); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xFAD3); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xFED3); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xFFD3); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xFCD3); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xFDD3); break;
	}
}

void CX86Ops::ShiftRightSignImmed(x86Reg reg, BYTE Immediate) {
	CPU_Message("      sar %s, %Xh",x86_Name(reg),Immediate);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xF8C1); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xFBC1); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xF9C1); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xFAC1); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xFEC1); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xFFC1); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xFCC1); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xFDC1); break;
	default:
		DisplayError("ShiftRightSignImmed\nUnknown x86 Register");
	}
	PUTDST8(m_RecompPos,Immediate);
}

void CX86Ops::ShiftRightUnsign(x86Reg reg) {
	CPU_Message("      shr %s, cl",x86_Name(reg));
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xE8D3); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xEBD3); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xE9D3); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xEAD3); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xEED3); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xEFD3); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xECD3); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xEDD3); break;
	}
}

void CX86Ops::ShiftRightDouble(x86Reg Destination, x86Reg Source) {
	unsigned char s = 0xC0;

	CPU_Message("      shrd %s, %s, cl", x86_Name(Destination),x86_Name(Source));
	PUTDST16(m_RecompPos,0xAD0F);

	switch (Destination) {
	case x86_EAX: s |= 0x00; break;
	case x86_EBX: s |= 0x03; break;
	case x86_ECX: s |= 0x01; break;
	case x86_EDX: s |= 0x02; break;
	case x86_ESI: s |= 0x06; break;
	case x86_EDI: s |= 0x07; break;
	case x86_ESP: s |= 0x04; break;
	case x86_EBP: s |= 0x05; break;
	}

	switch (Source) {
	case x86_EAX: s |= 0x00 << 3; break;
	case x86_EBX: s |= 0x03 << 3; break;
	case x86_ECX: s |= 0x01 << 3; break;
	case x86_EDX: s |= 0x02 << 3; break;
	case x86_ESI: s |= 0x06 << 3; break;
	case x86_EDI: s |= 0x07 << 3; break;
	case x86_ESP: s |= 0x04 << 3; break;
	case x86_EBP: s |= 0x05 << 3; break;
	}

	PUTDST8(m_RecompPos,s);
}

void CX86Ops::ShiftRightDoubleImmed(x86Reg Destination, x86Reg Source, BYTE Immediate) {
	unsigned char s = 0xC0;

	CPU_Message("      shrd %s, %s, %Xh", x86_Name(Destination),x86_Name(Source), Immediate);
	PUTDST16(m_RecompPos,0xAC0F);

	switch (Destination) {
	case x86_EAX: s |= 0x00; break;
	case x86_EBX: s |= 0x03; break;
	case x86_ECX: s |= 0x01; break;
	case x86_EDX: s |= 0x02; break;
	case x86_ESI: s |= 0x06; break;
	case x86_EDI: s |= 0x07; break;
	case x86_ESP: s |= 0x04; break;
	case x86_EBP: s |= 0x05; break;
	}

	switch (Source) {
	case x86_EAX: s |= 0x00 << 3; break;
	case x86_EBX: s |= 0x03 << 3; break;
	case x86_ECX: s |= 0x01 << 3; break;
	case x86_EDX: s |= 0x02 << 3; break;
	case x86_ESI: s |= 0x06 << 3; break;
	case x86_EDI: s |= 0x07 << 3; break;
	case x86_ESP: s |= 0x04 << 3; break;
	case x86_EBP: s |= 0x05 << 3; break;
	}

	PUTDST8(m_RecompPos,s);
	PUTDST8(m_RecompPos,Immediate);
}

void CX86Ops::ShiftRightUnsignImmed(x86Reg reg, BYTE Immediate) {
	CPU_Message("      shr %s, %Xh",x86_Name(reg),Immediate);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0xE8C1); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xEBC1); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xE9C1); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xEAC1); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xEEC1); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xEFC1); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xECC1); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xEDC1); break;
	}
	PUTDST8(m_RecompPos,Immediate);
}

void CX86Ops::SbbConstFromX86Reg (x86Reg reg, DWORD Const) {
	CPU_Message("      sbb %s, %Xh",x86_Name(reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		switch (reg) {
		case x86_EAX: PUTDST16(m_RecompPos,0xD881); break;
		case x86_EBX: PUTDST16(m_RecompPos,0xDB81); break;
		case x86_ECX: PUTDST16(m_RecompPos,0xD981); break;
		case x86_EDX: PUTDST16(m_RecompPos,0xDA81); break;
		case x86_ESI: PUTDST16(m_RecompPos,0xDE81); break;
		case x86_EDI: PUTDST16(m_RecompPos,0xDF81); break;
		case x86_ESP: PUTDST16(m_RecompPos,0xDC81); break;
		case x86_EBP: PUTDST16(m_RecompPos,0xDD81); break;
		}
		PUTDST32(m_RecompPos, Const);
	} else {
		switch (reg) {
		case x86_EAX: PUTDST16(m_RecompPos,0xD883); break;
		case x86_EBX: PUTDST16(m_RecompPos,0xDB83); break;
		case x86_ECX: PUTDST16(m_RecompPos,0xD983); break;
		case x86_EDX: PUTDST16(m_RecompPos,0xDA83); break;
		case x86_ESI: PUTDST16(m_RecompPos,0xDE83); break;
		case x86_EDI: PUTDST16(m_RecompPos,0xDF83); break;
		case x86_ESP: PUTDST16(m_RecompPos,0xDC83); break;
		case x86_EBP: PUTDST16(m_RecompPos,0xDD83); break;
		}
		PUTDST8(m_RecompPos, Const);
	}
}

void CX86Ops::SbbVariableFromX86reg(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      sbb %s, dword ptr [%s]",x86_Name(reg),VariableName);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x051B); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D1B); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D1B); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x151B); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x351B); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3D1B); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x251B); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2D1B); break;
	default:
		DisplayError("SbbVariableFromX86reg\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable); 
}

void CX86Ops::SbbX86RegToX86Reg(x86Reg Destination, x86Reg Source) {
	WORD x86Command;
	CPU_Message("      sbb %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Source) {
	case x86_EAX: x86Command = 0x001B; break;
	case x86_EBX: x86Command = 0x031B; break;
	case x86_ECX: x86Command = 0x011B; break;
	case x86_EDX: x86Command = 0x021B; break;
	case x86_ESI: x86Command = 0x061B; break;
	case x86_EDI: x86Command = 0x071B; break;
	case x86_ESP: x86Command = 0x041B; break;
	case x86_EBP: x86Command = 0x051B; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::SubConstFromVariable (DWORD Const, void *Variable, const char * VariableName) {
	CPU_Message("      sub dword ptr [%s], 0x%X",VariableName, Const);\
	PUTDST16(m_RecompPos,0x2D81);
	PUTDST32(m_RecompPos,Variable);
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::SubConstFromX86Reg (x86Reg reg, DWORD Const) {
	CPU_Message("      sub %s, %Xh",x86_Name(reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		switch (reg) {
		case x86_EAX: PUTDST16(m_RecompPos,0xE881); break;
		case x86_EBX: PUTDST16(m_RecompPos,0xEB81); break;
		case x86_ECX: PUTDST16(m_RecompPos,0xE981); break;
		case x86_EDX: PUTDST16(m_RecompPos,0xEA81); break;
		case x86_ESI: PUTDST16(m_RecompPos,0xEE81); break;
		case x86_EDI: PUTDST16(m_RecompPos,0xEF81); break;
		case x86_ESP: PUTDST16(m_RecompPos,0xEC81); break;
		case x86_EBP: PUTDST16(m_RecompPos,0xED81); break;
		}
		PUTDST32(m_RecompPos, Const);
	} else {
		switch (reg) {
		case x86_EAX: PUTDST16(m_RecompPos,0xE883); break;
		case x86_EBX: PUTDST16(m_RecompPos,0xEB83); break;
		case x86_ECX: PUTDST16(m_RecompPos,0xE983); break;
		case x86_EDX: PUTDST16(m_RecompPos,0xEA83); break;
		case x86_ESI: PUTDST16(m_RecompPos,0xEE83); break;
		case x86_EDI: PUTDST16(m_RecompPos,0xEF83); break;
		case x86_ESP: PUTDST16(m_RecompPos,0xEC83); break;
		case x86_EBP: PUTDST16(m_RecompPos,0xED83); break;
		}
		PUTDST8(m_RecompPos, Const);
	}
}

void CX86Ops::SubVariableFromX86reg(x86Reg reg, void * Variable, const char * VariableName) {
	CPU_Message("      sub %s, dword ptr [%s]",x86_Name(reg),VariableName);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x052B); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D2B); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D2B); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x152B); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x352B); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3D2B); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x252B); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2D2B); break;
	default:
		DisplayError("SubVariableFromX86reg\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable); 
}

void CX86Ops::SubX86RegToX86Reg(x86Reg Destination, x86Reg Source) {
	WORD x86Command;
	CPU_Message("      sub %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Source) {
	case x86_EAX: x86Command = 0x002B; break;
	case x86_EBX: x86Command = 0x032B; break;
	case x86_ECX: x86Command = 0x012B; break;
	case x86_EDX: x86Command = 0x022B; break;
	case x86_ESI: x86Command = 0x062B; break;
	case x86_EDI: x86Command = 0x072B; break;
	case x86_ESP: x86Command = 0x042B; break;
	case x86_EBP: x86Command = 0x052B; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::TestConstToX86Reg(DWORD Const, x86Reg reg) {
	CPU_Message("      test %s, 0x%X",x86_Name(reg), Const);
	
	switch (reg) {
	case x86_EAX: PUTDST8 (m_RecompPos,0xA9); break;
	case x86_EBX: PUTDST16(m_RecompPos,0xC3F7); break;
	case x86_ECX: PUTDST16(m_RecompPos,0xC1F7); break;
	case x86_EDX: PUTDST16(m_RecompPos,0xC2F7); break;
	case x86_ESI: PUTDST16(m_RecompPos,0xC6F7); break;
	case x86_EDI: PUTDST16(m_RecompPos,0xC7F7); break;
	case x86_ESP: PUTDST16(m_RecompPos,0xC4F7); break;
	case x86_EBP: PUTDST16(m_RecompPos,0xC5F7); break;
	}
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::TestVariable(DWORD Const, void * Variable, const char * VariableName) {
	CPU_Message("      test dword ptr ds:[%s], 0x%X",VariableName, Const);
	PUTDST16(m_RecompPos,0x05F7);
	PUTDST32(m_RecompPos,Variable);
	PUTDST32(m_RecompPos,Const);
}

void CX86Ops::TestX86RegToX86Reg(x86Reg Destination, x86Reg Source) {
	WORD x86Command;
	CPU_Message("      test %s, %s",x86_Name(Destination),x86_Name(Source));
	switch (Source) {
	case x86_EAX: x86Command = 0x0085; break;
	case x86_EBX: x86Command = 0x0385; break;
	case x86_ECX: x86Command = 0x0185; break;
	case x86_EDX: x86Command = 0x0285; break;
	case x86_ESI: x86Command = 0x0685; break;
	case x86_EDI: x86Command = 0x0785; break;
	case x86_ESP: x86Command = 0x0485; break;
	case x86_EBP: x86Command = 0x0585; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::XorConstToX86Reg(x86Reg reg, DWORD Const) {
	CPU_Message("      xor %s, %Xh",x86_Name(reg),Const);
	if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80) {
		switch (reg) {
		case x86_EAX: PUTDST16(m_RecompPos,0xF081); break;
		case x86_EBX: PUTDST16(m_RecompPos,0xF381); break;
		case x86_ECX: PUTDST16(m_RecompPos,0xF181); break;
		case x86_EDX: PUTDST16(m_RecompPos,0xF281); break;
		case x86_ESI: PUTDST16(m_RecompPos,0xF681); break;
		case x86_EDI: PUTDST16(m_RecompPos,0xF781); break;
		case x86_ESP: PUTDST16(m_RecompPos,0xF481); break;
		case x86_EBP: PUTDST16(m_RecompPos,0xF581); break;
		}
		PUTDST32(m_RecompPos, Const);
	} else {
		switch (reg) {
		case x86_EAX: PUTDST16(m_RecompPos,0xF083); break;
		case x86_EBX: PUTDST16(m_RecompPos,0xF383); break;
		case x86_ECX: PUTDST16(m_RecompPos,0xF183); break;
		case x86_EDX: PUTDST16(m_RecompPos,0xF283); break;
		case x86_ESI: PUTDST16(m_RecompPos,0xF683); break;
		case x86_EDI: PUTDST16(m_RecompPos,0xF783); break;
		case x86_ESP: PUTDST16(m_RecompPos,0xF483); break;
		case x86_EBP: PUTDST16(m_RecompPos,0xF583); break;
		}
		PUTDST8(m_RecompPos, Const);
	}
}

void CX86Ops::XorX86RegToX86Reg(x86Reg Source, x86Reg Destination) {
	WORD x86Command;

	CPU_Message("      xor %s, %s",x86_Name(Source),x86_Name(Destination));
		
	switch (Source) {
	case x86_EAX: x86Command = 0x0031; break;
	case x86_EBX: x86Command = 0x0331; break;
	case x86_ECX: x86Command = 0x0131; break;
	case x86_EDX: x86Command = 0x0231; break;
	case x86_ESI: x86Command = 0x0631; break;
	case x86_EDI: x86Command = 0x0731; break;
	case x86_ESP: x86Command = 0x0431; break;
	case x86_EBP: x86Command = 0x0531; break;
	}
	switch (Destination) {
	case x86_EAX: x86Command += 0xC000; break;
	case x86_EBX: x86Command += 0xD800; break;
	case x86_ECX: x86Command += 0xC800; break;
	case x86_EDX: x86Command += 0xD000; break;
	case x86_ESI: x86Command += 0xF000; break;
	case x86_EDI: x86Command += 0xF800; break;
	case x86_ESP: x86Command += 0xE000; break;
	case x86_EBP: x86Command += 0xE800; break;
	}
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::XorVariableToX86reg(void *Variable, const char * VariableName, x86Reg reg) {
	CPU_Message("      Xor %s, dword ptr [%s]",x86_Name(reg),VariableName);
	switch (reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x0533); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x1D33); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x0D33); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x1533); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x3533); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x3D33); break;
	case x86_ESP: PUTDST16(m_RecompPos,0x2533); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x2D33); break;
	default: DisplayError("XorVariableToX86reg\nUnknown x86 Register");
	}
    PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuAbs(void) {
	CPU_Message("      fabs ST(0)");
	PUTDST16(m_RecompPos,0xE1D9);
}

void CX86Ops::fpuAddDword(void *Variable, const char * VariableName) {
	CPU_Message("      fadd ST(0), dword ptr [%s]", VariableName);
	PUTDST16(m_RecompPos,0x05D8);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuAddDwordRegPointer(x86Reg x86Pointer) {
	CPU_Message("      fadd ST(0), dword ptr [%s]",x86_Name(x86Pointer));
	switch (x86Pointer) {
	case x86_EAX: PUTDST16(m_RecompPos,0x00D8); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x03D8); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x01D8); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x02D8); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x06D8); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x07D8); break;
	default:
		DisplayError("fpuAddDwordRegPointer\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuAddQword(void *Variable, const char * VariableName) {
	CPU_Message("      fadd ST(0), qword ptr [%s]", VariableName);
	PUTDST16(m_RecompPos,0x05DC);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuAddQwordRegPointer(x86Reg x86Pointer) {
	CPU_Message("      fadd ST(0), qword ptr [%s]",x86_Name(x86Pointer));
	switch (x86Pointer) {
	case x86_EAX: PUTDST16(m_RecompPos,0x00DC); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x03DC); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x01DC); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x02DC); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x06DC); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x07DC); break;
	default:
		DisplayError("fpuAddQwordRegPointer\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuAddReg(x86FpuValues x86reg) {
	CPU_Message("      fadd ST(0), %s",fpu_Name(x86reg));
	switch (x86reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xC0D8); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xC1D8); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xC2D8); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xC3D8); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xC4D8); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xC5D8); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xC6D8); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xC7D8); break;
	default:
		DisplayError("fpuAddReg\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuAddRegPop(int * StackPos, x86FpuValues reg) {
	CPU_Message("      faddp ST(0), %s",fpu_Name(reg));
	*StackPos = (*StackPos + 1) & 7;
	switch (reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xC0DE); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xC1DE); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xC2DE); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xC3DE); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xC4DE); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xC5DE); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xC6DE); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xC7DE); break;
	default:
		DisplayError("fpuAddReg\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuComDword(void *Variable, const char * VariableName, BOOL Pop) {
	CPU_Message("      fcom%s ST(0), dword ptr [%s]", m_fpupop[Pop], VariableName);
	PUTDST16(m_RecompPos, (Pop == TRUE) ? 0x1DD8 : 0x15D8);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuComDwordRegPointer(x86Reg x86Pointer, BOOL Pop) {
	WORD x86Command;

	CPU_Message("      fcom%s ST(0), dword ptr [%s]",m_fpupop[Pop],x86_Name(x86Pointer));
	switch (x86Pointer) {
	case x86_EAX: x86Command = 0x10D8; break;
	case x86_EBX: x86Command = 0x13D8; break;
	case x86_ECX: x86Command = 0x11D8; break;
	case x86_EDX: x86Command = 0x12D8; break;
	case x86_ESI: x86Command = 0x16D8; break;
	case x86_EDI: x86Command = 0x17D8; break;
	}
	if (Pop) { x86Command |= 0x0800; }
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::fpuComQword(void *Variable, const char * VariableName, BOOL Pop) {
	CPU_Message("      fcom%s ST(0), qword ptr [%s]", m_fpupop[Pop], VariableName);
	PUTDST16(m_RecompPos, (Pop == TRUE) ? 0x1DDC : 0x15DC);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuComQwordRegPointer(x86Reg x86Pointer, BOOL Pop) {
	WORD x86Command;

	CPU_Message("      fcom%s ST(0), qword ptr [%s]",m_fpupop[Pop],x86_Name(x86Pointer));
	switch (x86Pointer) {
	case x86_EAX: x86Command = 0x10DC; break;
	case x86_EBX: x86Command = 0x13DC; break;
	case x86_ECX: x86Command = 0x11DC; break;
	case x86_EDX: x86Command = 0x12DC; break;
	case x86_ESI: x86Command = 0x16DC; break;
	case x86_EDI: x86Command = 0x17DC; break;
	}
	if (Pop) { x86Command |= 0x0800; }
	PUTDST16(m_RecompPos,x86Command);
}

void CX86Ops::fpuComReg(x86FpuValues x86reg, BOOL Pop) {
	int s = (Pop == TRUE) ? 0x0800 : 0x0000;
	CPU_Message("      fcom%s ST(0), %s", m_fpupop[Pop], fpu_Name(x86reg));

	switch (x86reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xD0D8|s); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xD1D8|s); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xD2D8|s); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xD3D8|s); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xD4D8|s); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xD5D8|s); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xD6D8|s); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xD7D8|s); break;
	default:
		DisplayError("fpuComReg\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuDivDword(void *Variable, const char * VariableName) {
	CPU_Message("      fdiv ST(0), dword ptr [%s]", VariableName);
	PUTDST16(m_RecompPos,0x35D8);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuDivDwordRegPointer(x86Reg x86Pointer) {
	CPU_Message("      fdiv ST(0), dword ptr [%s]",x86_Name(x86Pointer));
	switch (x86Pointer) {
	case x86_EAX: PUTDST16(m_RecompPos,0x30D8); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x33D8); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x31D8); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x32D8); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x36D8); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x37D8); break;
	default:
		DisplayError("fpuDivDwordRegPointer\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuDivQword(void *Variable, const char * VariableName) {
	CPU_Message("      fdiv ST(0), qword ptr [%s]", VariableName);
	PUTDST16(m_RecompPos,0x35DC);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuDivQwordRegPointer(x86Reg x86Pointer) {
	CPU_Message("      fdiv ST(0), qword ptr [%s]",x86_Name(x86Pointer));
	switch (x86Pointer) {
	case x86_EAX: PUTDST16(m_RecompPos,0x30DC); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x33DC); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x31DC); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x32DC); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x36DC); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x37DC); break;
	default:
		DisplayError("fpuDivQwordRegPointer\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuDivReg(x86FpuValues Reg) {
	CPU_Message("      fdiv ST(0), %s", fpu_Name(Reg));
	switch (Reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xF0D8); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xF1D8); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xF2D8); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xF3D8); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xF4D8); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xF5D8); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xF6D8); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xF7D8); break;
	default:
		DisplayError("fpuDivReg\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuDivRegPop(x86FpuValues reg) {
	CPU_Message("      fdivp ST(0), %s",fpu_Name(reg));
	switch (reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xF8DE); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xF9DE); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xFADE); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xFBDE); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xFCDE); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xFDDE); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xFEDE); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xFFDE); break;
	default:
		DisplayError("fpuDivReg\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuExchange(x86FpuValues Reg) {
	CPU_Message("      fxch ST(0), %s",fpu_Name(Reg));
	switch (Reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xC8D9); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xC9D9); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xCAD9); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xCBD9); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xCCD9); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xCDD9); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xCED9); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xCFD9); break;
	default:
		DisplayError("fpuExchange\nUnknown x86 Register: %i", Reg);
		break;
	}
}

void CX86Ops::fpuFree(x86FpuValues Reg) {
	CPU_Message("      ffree %s",fpu_Name(Reg));
	switch (Reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xC0DD); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xC1DD); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xC2DD); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xC3DD); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xC4DD); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xC5DD); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xC6DD); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xC7DD); break;
	default:
		DisplayError("fpuFree\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuDecStack(int * StackPos) {
	CPU_Message("      fdecstp");
	*StackPos = (*StackPos - 1) & 7;
	PUTDST16(m_RecompPos,0xF6D9);
}

void CX86Ops::fpuIncStack(int * StackPos) {
	CPU_Message("      fincstp");
	*StackPos = (*StackPos + 1) & 7;
	PUTDST16(m_RecompPos,0xF7D9);
}

void CX86Ops::fpuLoadControl(void *Variable, const char * VariableName) {
	CPU_Message("      fldcw [%s]",VariableName);
	PUTDST16(m_RecompPos,0x2DD9);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuLoadDword(int * StackPos,void *Variable, const char * VariableName) {
	CPU_Message("      fld dword ptr [%s]",VariableName);
	*StackPos = (*StackPos - 1) & 7;
	PUTDST16(m_RecompPos,0x05D9);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuLoadDwordFromX86Reg(int * StackPos, x86Reg x86reg) {
	CPU_Message("      fld dword ptr [%s]",x86_Name(x86reg));
	*StackPos = (*StackPos - 1) & 7;
	PUTDST8(m_RecompPos,0xD9);
	switch (x86reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x00); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x03); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x01); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x02); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x06); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x07); break;
	default:
		DisplayError("fpuLoadDwordFromX86Reg\nUnknown x86 Register");
	}
}

void CX86Ops::fpuLoadDwordFromN64Mem(int * StackPos,x86Reg x86reg) {
	CPU_Message("      fld dword ptr [%s+N64mem]",x86_Name(x86reg));
	*StackPos = (*StackPos - 1) & 7;
	switch (x86reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x80D9); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x83D9); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x81D9); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x82D9); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x86D9); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x87D9); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x85D9); break;
	default:
		DisplayError("fpuLoadDwordFromN64Mem\nUnknown x86 Register");
	}
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::fpuLoadInt32bFromN64Mem(int * StackPos,x86Reg x86reg) {
	CPU_Message("      fild dword ptr [%s+N64mem]",x86_Name(x86reg));
	*StackPos = (*StackPos - 1) & 7;
	switch (x86reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x80DB); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x83DB); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x81DB); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x82DB); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x86DB); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x87DB); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x85DB); break;
	default:
		DisplayError("fpuLoadIntDwordFromN64Mem\nUnknown x86 Register");
	}
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::fpuLoadIntegerDword(int * StackPos,void *Variable, const char * VariableName) {
	CPU_Message("      fild dword ptr [%s]",VariableName);
	*StackPos = (*StackPos - 1) & 7;
	PUTDST16(m_RecompPos,0x05DB);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuLoadIntegerDwordFromX86Reg(int * StackPos,x86Reg x86reg) {
	CPU_Message("      fild dword ptr [%s]",x86_Name(x86reg));
	*StackPos = (*StackPos - 1) & 7;
	PUTDST8(m_RecompPos,0xDB);
	switch (x86reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x00); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x03); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x01); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x02); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x06); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x07); break;
	default:
		DisplayError("fpuLoadIntegerDwordFromX86Reg\nUnknown x86 Register");
	}
}

void CX86Ops::fpuLoadIntegerQword(int * StackPos,void *Variable, const char * VariableName) {
	CPU_Message("      fild qword ptr [%s]",VariableName);
	*StackPos = (*StackPos - 1) & 7;
	PUTDST16(m_RecompPos,0x2DDF);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuLoadIntegerQwordFromX86Reg(int * StackPos,x86Reg x86reg) {
	CPU_Message("      fild qword ptr [%s]",x86_Name(x86reg));
	*StackPos = (*StackPos - 1) & 7;
	PUTDST8(m_RecompPos,0xDF);
	switch (x86reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x28); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x2B); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x29); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x2A); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x2E); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x2F); break;
	default:
		DisplayError("fpuLoadIntegerDwordFromX86Reg\nUnknown x86 Register");
	}
}

void CX86Ops::fpuLoadQword(int * StackPos,void *Variable, const char * VariableName) {
	CPU_Message("      fld qword ptr [%s]",VariableName);
	*StackPos = (*StackPos - 1) & 7;
	PUTDST16(m_RecompPos,0x05DD);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuLoadQwordFromX86Reg(int * StackPos, x86Reg x86reg) {
	CPU_Message("      fld qword ptr [%s]",x86_Name(x86reg));
	*StackPos = (*StackPos - 1) & 7;
	PUTDST8(m_RecompPos,0xDD);
	switch (x86reg) {
	case x86_EAX: PUTDST8(m_RecompPos,0x00); break;
	case x86_EBX: PUTDST8(m_RecompPos,0x03); break;
	case x86_ECX: PUTDST8(m_RecompPos,0x01); break;
	case x86_EDX: PUTDST8(m_RecompPos,0x02); break;
	case x86_ESI: PUTDST8(m_RecompPos,0x06); break;
	case x86_EDI: PUTDST8(m_RecompPos,0x07); break;
	default:
		DisplayError("fpuLoadQwordFromX86Reg\nUnknown x86 Register");
	}
}

void CX86Ops::fpuLoadQwordFromN64Mem(int * StackPos,x86Reg x86reg) {
	CPU_Message("      fld qword ptr [%s+N64mem]",x86_Name(x86reg));
	*StackPos = (*StackPos - 1) & 7;
	switch (x86reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x80DD); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x83DD); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x81DD); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x82DD); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x86DD); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x87DD); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x85DD); break;
	default:
		DisplayError("fpuLoadQwordFromN64Mem\nUnknown x86 Register");
	}
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::fpuLoadReg(int * StackPos,x86FpuValues Reg) {
	CPU_Message("      fld ST(0), %s",fpu_Name(Reg));
	*StackPos = (*StackPos - 1) & 7;
	switch (Reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xC0D9); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xC1D9); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xC2D9); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xC3D9); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xC4D9); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xC5D9); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xC6D9); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xC7D9); break;
	default:
		DisplayError("fpuLoadReg\nUnknown x86 Register:%i", Reg);
		break;
	}
}

void CX86Ops::fpuMulDword(void *Variable, const char * VariableName) {
	CPU_Message("      fmul ST(0), dword ptr [%s]", VariableName);
	PUTDST16(m_RecompPos,0x0DD8);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuMulDwordRegPointer(x86Reg x86Pointer) {
	CPU_Message("      fmul ST(0), dword ptr [%s]",x86_Name(x86Pointer));
	switch (x86Pointer) {
	case x86_EAX: PUTDST16(m_RecompPos,0x08D8); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x0BD8); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x09D8); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x0AD8); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x0ED8); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x0FD8); break;
	default:
		DisplayError("fpuMulDwordRegPointer\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuMulQword(void *Variable, const char * VariableName) {
	CPU_Message("      fmul ST(0), qword ptr [%s]", VariableName);
	PUTDST16(m_RecompPos,0x0DDC);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuMulQwordRegPointer(x86Reg x86Pointer) {
	CPU_Message("      fmul ST(0), qword ptr [%s]",x86_Name(x86Pointer));
	switch (x86Pointer) {
	case x86_EAX: PUTDST16(m_RecompPos,0x08DC); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x0BDC); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x09DC); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x0ADC); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x0EDC); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x0FDC); break;
	default:
		DisplayError("fpuMulQwordRegPointer\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuMulReg(x86FpuValues x86reg) {
	CPU_Message("      fmul ST(0), %s",fpu_Name(x86reg));
	switch (x86reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xC8D8); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xC9D8); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xCAD8); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xCBD8); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xCCD8); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xCDD8); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xCED8); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xCFD8); break;
	default:
		DisplayError("fpuMulReg\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuMulRegPop(x86FpuValues x86reg) {
	CPU_Message("      fmulp ST(0), %s",fpu_Name(x86reg));
	switch (x86reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xC8DE); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xC9DE); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xCADE); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xCBDE); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xCCDE); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xCDDE); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xCEDE); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xCFDE); break;
	default:
		DisplayError("fpuMulReg\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuNeg(void) {
	CPU_Message("      fchs ST(0)");
	PUTDST16(m_RecompPos,0xE0D9);
}

void CX86Ops::fpuRound(void) {
	CPU_Message("      frndint ST(0)");
	PUTDST16(m_RecompPos,0xFCD9);
}

void CX86Ops::fpuSqrt(void) {
	CPU_Message("      fsqrt ST(0)");
	PUTDST16(m_RecompPos,0xFAD9);
}

void CX86Ops::fpuStoreControl(void *Variable, const char * VariableName) {
	CPU_Message("      fnstcw [%s]",VariableName);
	PUTDST16(m_RecompPos,0x3DD9);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuStoreDword(int * StackPos,void *Variable, const char * VariableName, BOOL pop) {
	CPU_Message("      fst%s dword ptr [%s]", m_fpupop[pop], VariableName);
	if (pop) { *StackPos = (*StackPos + 1) & 7; }
	PUTDST16(m_RecompPos,(pop == FALSE) ? 0x15D9 : 0x1DD9);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuStoreDwordFromX86Reg(int * StackPos,x86Reg x86reg, BOOL pop) {
	BYTE Command;

	CPU_Message("      fst%s dword ptr [%s]", m_fpupop[pop], x86_Name(x86reg));
	if (pop) { *StackPos = (*StackPos + 1) & 7; }
	PUTDST8(m_RecompPos,0xD9);

	switch (x86reg) {
	case x86_EAX: Command = 0x10; break;
	case x86_EBX: Command = 0x13; break;
	case x86_ECX: Command = 0x11; break;
	case x86_EDX: Command = 0x12; break;
	case x86_ESI: Command = 0x16; break;
	case x86_EDI: Command = 0x17; break;
	default:
		DisplayError("fpuStoreIntegerQwordFromX86Reg\nUnknown x86 Register");
	}
	PUTDST8(m_RecompPos, (pop == FALSE) ? Command : (Command + 0x8));
}

void CX86Ops::fpuStoreDwordToN64Mem(int * StackPos,x86Reg x86reg, BOOL Pop) {
	int s = (Pop == TRUE) ? 0x0800 : 0;

	CPU_Message("      fst%s dword ptr [%s+N64mem]", m_fpupop[Pop], x86_Name(x86reg));
	if (Pop) { *StackPos = (*StackPos + 1) & 7; }

	switch (x86reg) {
	case x86_EAX: PUTDST16(m_RecompPos,0x90D9|s); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x93D9|s); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x91D9|s); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x92D9|s); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x96D9|s); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x97D9|s); break;
	case x86_EBP: PUTDST16(m_RecompPos,0x95D9|s); break;
	default:
		DisplayError("fpuStoreDwordToN64Mem\nUnknown x86 Register");
	}
	PUTDST32(m_RecompPos,_MMU->Rdram());
}

void CX86Ops::fpuStoreIntegerDword(int * StackPos,void *Variable, const char * VariableName, BOOL pop) {
	CPU_Message("      fist%s dword ptr [%s]", m_fpupop[pop], VariableName);
	if (pop) { *StackPos = (*StackPos + 1) & 7; }
	PUTDST16(m_RecompPos, (pop == FALSE) ? 0x15DB : 0x1DDB);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuStoreIntegerDwordFromX86Reg(int * StackPos,x86Reg x86reg, BOOL pop) {
	BYTE Command;

	CPU_Message("      fist%s dword ptr [%s]", m_fpupop[pop], x86_Name(x86reg));
	if (pop) { *StackPos = (*StackPos + 1) & 7; }
	PUTDST8(m_RecompPos,0xDB);
	
	switch (x86reg) {
	case x86_EAX: Command = 0x10; break;
	case x86_EBX: Command = 0x13; break;
	case x86_ECX: Command = 0x11; break;
	case x86_EDX: Command = 0x12; break;
	case x86_ESI: Command = 0x16; break;
	case x86_EDI: Command = 0x17; break;
	default:
		DisplayError("fpuStoreIntegerDwordFromX86Reg\nUnknown x86 Register");
	}
	PUTDST8(m_RecompPos, (pop == FALSE) ? Command : (Command + 0x8));
}

void CX86Ops::fpuStoreIntegerQword(int * StackPos,void *Variable, const char * VariableName, BOOL pop) {
	CPU_Message("      fist%s qword ptr [%s]", m_fpupop[pop], VariableName);
	if (pop) { *StackPos = (*StackPos + 1) & 7; }
	PUTDST16(m_RecompPos, (pop == FALSE) ? 0x35DF : 0x3DDF);
	PUTDST32(m_RecompPos,Variable);
	if (!pop) { X86BreakPoint(__FILE__,__LINE__); }
}

void CX86Ops::fpuStoreIntegerQwordFromX86Reg(int * StackPos, x86Reg x86reg, BOOL pop) {
	BYTE Command;

	CPU_Message("      fist%s qword ptr [%s]", m_fpupop[pop], x86_Name(x86reg));
	if (pop) { *StackPos = (*StackPos + 1) & 7; }
	PUTDST8(m_RecompPos,0xDF);

	switch (x86reg) {
	case x86_EAX: Command = 0x30; break;
	case x86_EBX: Command = 0x33; break;
	case x86_ECX: Command = 0x31; break;
	case x86_EDX: Command = 0x32; break;
	case x86_ESI: Command = 0x36; break;
	case x86_EDI: Command = 0x37; break;
	default:
		DisplayError("fpuStoreIntegerQwordFromX86Reg\nUnknown x86 Register");
	}
	PUTDST8(m_RecompPos, (pop == FALSE) ? Command : (Command + 0x8));
}

void CX86Ops::fpuStoreQwordFromX86Reg(int * StackPos, x86Reg x86reg, BOOL pop) {
	BYTE Command;

	CPU_Message("      fst%s qword ptr [%s]", m_fpupop[pop], x86_Name(x86reg));
	if (pop) { *StackPos = (*StackPos + 1) & 7; }
	PUTDST8(m_RecompPos,0xDD);

	switch (x86reg) {
	case x86_EAX: Command = 0x10; break;
	case x86_EBX: Command = 0x13; break;
	case x86_ECX: Command = 0x11; break;
	case x86_EDX: Command = 0x12; break;
	case x86_ESI: Command = 0x16; break;
	case x86_EDI: Command = 0x17; break;
	default:
		DisplayError("fpuStoreQwordFromX86Reg\nUnknown x86 Register");
	}
	PUTDST8(m_RecompPos, (pop == FALSE) ? Command : (Command + 0x8));
}

void CX86Ops::fpuStoreStatus(void) {
	CPU_Message("      fnstsw ax");
	PUTDST16(m_RecompPos,0xE0DF);
}

void CX86Ops::fpuSubDword(void *Variable, const char * VariableName) {
	CPU_Message("      fsub ST(0), dword ptr [%s]", VariableName);
	PUTDST16(m_RecompPos,0x25D8);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuSubDwordRegPointer(x86Reg x86Pointer) {
	CPU_Message("      fsub ST(0), dword ptr [%s]",x86_Name(x86Pointer));
	switch (x86Pointer) {
	case x86_EAX: PUTDST16(m_RecompPos,0x20D8); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x23D8); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x21D8); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x22D8); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x26D8); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x27D8); break;
	default:
		DisplayError("fpuSubDwordRegPointer\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuSubDwordReverse(void *Variable, const char * VariableName) {
	CPU_Message("      fsubr ST(0), dword ptr [%s]", VariableName);
	PUTDST16(m_RecompPos,0x2DD8);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuSubQword(void *Variable, const char * VariableName) {
	CPU_Message("      fsub ST(0), qword ptr [%s]", VariableName);
	PUTDST16(m_RecompPos,0x25DC);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuSubQwordRegPointer(x86Reg x86Pointer) {
	CPU_Message("      fsub ST(0), qword ptr [%s]",x86_Name(x86Pointer));
	switch (x86Pointer) {
	case x86_EAX: PUTDST16(m_RecompPos,0x20DC); break;
	case x86_EBX: PUTDST16(m_RecompPos,0x23DC); break;
	case x86_ECX: PUTDST16(m_RecompPos,0x21DC); break;
	case x86_EDX: PUTDST16(m_RecompPos,0x22DC); break;
	case x86_ESI: PUTDST16(m_RecompPos,0x26DC); break;
	case x86_EDI: PUTDST16(m_RecompPos,0x27DC); break;
	default:
		DisplayError("fpuSubQwordRegPointer\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuSubQwordReverse(void *Variable, const char * VariableName) {
	CPU_Message("      fsubr ST(0), qword ptr [%s]", VariableName);
	PUTDST16(m_RecompPos,0x2DDC);
	PUTDST32(m_RecompPos,Variable);
}

void CX86Ops::fpuSubReg(x86FpuValues x86reg) {
	CPU_Message("      fsub ST(0), %s",fpu_Name(x86reg));
	switch (x86reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xE0D8); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xE1D8); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xE2D8); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xE3D8); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xE4D8); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xE5D8); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xE6D8); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xE7D8); break;
	default:
		DisplayError("fpuSubReg\nUnknown x86 Register");
		break;
	}
}

void CX86Ops::fpuSubRegPop(x86FpuValues x86reg) {
	CPU_Message("      fsubp ST(0), %s",fpu_Name(x86reg));
	switch (x86reg) {
	case x86_ST0: PUTDST16(m_RecompPos,0xE8DE); break;
	case x86_ST1: PUTDST16(m_RecompPos,0xE9DE); break;
	case x86_ST2: PUTDST16(m_RecompPos,0xEADE); break;
	case x86_ST3: PUTDST16(m_RecompPos,0xEBDE); break;
	case x86_ST4: PUTDST16(m_RecompPos,0xECDE); break;
	case x86_ST5: PUTDST16(m_RecompPos,0xEDDE); break;
	case x86_ST6: PUTDST16(m_RecompPos,0xEEDE); break;
	case x86_ST7: PUTDST16(m_RecompPos,0xEFDE); break;
	default:
		DisplayError("fpuSubRegPop\nUnknown x86 Register");
		break;
	}
}

const char * CX86Ops::x86_Name ( x86Reg Reg ) {
	switch (Reg) {
	case x86_EAX: return "eax";
	case x86_EBX: return "ebx";
	case x86_ECX: return "ecx";
	case x86_EDX: return "edx";
	case x86_ESI: return "esi";
	case x86_EDI: return "edi";
	case x86_EBP: return "ebp";
	case x86_ESP: return "esp";
	}
	return "???";
}

const char * CX86Ops::x86_ByteName ( x86Reg Reg ) {
	switch (Reg) {
	case x86_AL: return "al";
	case x86_BL: return "bl";
	case x86_CL: return "cl";
	case x86_DL: return "dl";
	case x86_AH: return "ah";
	case x86_BH: return "bh";
	case x86_CH: return "ch";
	case x86_DH: return "dh";
	}
	return "???";
}

const char * CX86Ops::x86_HalfName ( x86Reg Reg ) {
	switch (Reg) {
	case x86_EAX: return "ax";
	case x86_EBX: return "bx";
	case x86_ECX: return "cx";
	case x86_EDX: return "dx";
	case x86_ESI: return "si";
	case x86_EDI: return "di";
	case x86_EBP: return "bp";
	case x86_ESP: return "sp";
	}
	return "???";
}

const char * CX86Ops::fpu_Name ( x86FpuValues Reg ) {
	switch (Reg) {
	case x86_ST0: return "ST(0)";
	case x86_ST1: return "ST(1)";
	case x86_ST2: return "ST(2)";
	case x86_ST3: return "ST(3)";
	case x86_ST4: return "ST(4)";
	case x86_ST5: return "ST(5)";
	case x86_ST6: return "ST(6)";
	case x86_ST7: return "ST(7)";
	}
	return "???";
}

BOOL CX86Ops::Is8BitReg ( x86Reg Reg )
{
	if (Reg == x86_EAX) { return TRUE; }
	if (Reg == x86_EBX) { return TRUE; }
	if (Reg == x86_ECX) { return TRUE; }
	if (Reg == x86_EDX) { return TRUE; }
	return FALSE;
}

BYTE CX86Ops::CalcMultiplyCode (Multipler Multiply)
{
	switch (Multiply) {
	case Multip_x2: return 0x40;
	case Multip_x4: return 0x80;
	case Multip_x8: return 0xC0;
	default: return 0;
	}
}

//#define SetJump32(Loc,JumpLoc) *(DWORD *)(Loc)= (DWORD)(((DWORD)(JumpLoc)) - (((DWORD)(Loc)) + 4));
//#define SetJump8(Loc,JumpLoc)  *(BYTE  *)(Loc)= (BYTE )(((BYTE )(JumpLoc)) - (((BYTE )(Loc)) + 1));

void CX86Ops::SetJump32(DWORD * Loc, DWORD * JumpLoc)
{
	 *Loc = (DWORD)(((DWORD)JumpLoc) - (((DWORD)(Loc)) + 4));
}

void CX86Ops::SetJump8(BYTE * Loc, BYTE * JumpLoc)
{
	 *Loc = (BYTE )((BYTE)JumpLoc - ((BYTE )Loc + 1));
}


void * CX86Ops::GetAddressOf(int value, ...) {
	void * Address;

	va_list ap;
	va_start( ap, value );
	Address = va_arg(ap,void *);
	va_end( ap );
	
	return Address;
}

