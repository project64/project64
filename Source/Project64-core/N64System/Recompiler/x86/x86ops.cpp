#include "stdafx.h"

#if defined(__i386__) || defined(_M_IX86)
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/x86/x86ops.h>
#include <Project64-core/N64System/SystemGlobals.h>

char CX86Ops::m_fpupop[2][2] = {
    "",
    "p",
};

CX86Ops::CX86Ops(CCodeBlock & CodeBlock) :
    m_CodeBlock(CodeBlock)
{
}

// Logging functions
void CX86Ops::WriteX86Comment(const char * Comment)
{
    CodeLog("");
    CodeLog("      // %s", Comment);
}

void CX86Ops::WriteX86Label(const char * Label)
{
    CodeLog("");
    CodeLog("      %s:", Label);
}

void CX86Ops::AdcConstToVariable(void * Variable, const char * VariableName, uint8_t Constant)
{
    CodeLog("      adc dword ptr [%s], %Xh", VariableName, Constant);
    AddCode16(0x1583);
    AddCode32((uint32_t)Variable);
    AddCode8(Constant);
}

void CX86Ops::AdcConstToX86Reg(x86Reg Reg, uint32_t Const)
{
    CodeLog("      adc %s, %Xh", x86_Name(Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        AddCode16((uint16_t)(0xD081 + (Reg * 0x100)));
        AddCode32(Const);
    }
    else
    {
        AddCode16((uint16_t)(0xD083 + (Reg * 0x100)));
        AddCode8((uint8_t)Const);
    }
}

void CX86Ops::AdcVariableToX86reg(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      adc %s, dword ptr [%s]", x86_Name(Reg), VariableName);
    AddCode16((uint16_t)(0x0513 + (Reg * 0x800)));
    AddCode32((uint32_t)Variable);
}

void CX86Ops::AdcX86RegToX86Reg(x86Reg Destination, x86Reg Source)
{
    CodeLog("      adc %s, %s", x86_Name(Destination), x86_Name(Source));
    AddCode16((uint16_t)(0xC013 + (Source * 0x100) + (Destination * 0x800)));
}

void CX86Ops::AddConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    CodeLog("      add dword ptr [%s], 0x%X", VariableName, Const);
    AddCode16(0x0581);
    AddCode32((uint32_t)Variable);
    AddCode32(Const);
}

void CX86Ops::AddConstToX86Reg(x86Reg Reg, uint32_t Const, bool NeedCarry)
{
    if (Const == 0)
    {
    }
    else if (Const == 1 && !NeedCarry)
    {
        IncX86reg(Reg);
    }
    else if (Const == 0xFFFFFFFF && !NeedCarry)
    {
        DecX86reg(Reg);
    }
    else if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        CodeLog("      add %s, %Xh", x86_Name(Reg), Const);
        AddCode16((uint16_t)(0xC081 + (Reg * 0x100)));
        AddCode32(Const);
    }
    else
    {
        CodeLog("      add %s, %Xh", x86_Name(Reg), Const);
        AddCode16((uint16_t)(0xC083 + (Reg * 0x100)));
        AddCode8((uint8_t)Const);
    }
}

void CX86Ops::AddVariableToX86reg(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      add %s, dword ptr [%s]", x86_Name(Reg), VariableName);
    AddCode16((uint16_t)(0x0503 + (Reg * 0x800)));
    AddCode32((uint32_t)Variable);
}

void CX86Ops::AddX86regToVariable(void * Variable, const char * VariableName, x86Reg Reg)
{
    CodeLog("      add dword ptr [%s], %s", VariableName, x86_Name(Reg));
    AddCode16((uint16_t)(0x0501 + (Reg * 0x800)));
    AddCode32((uint32_t)Variable);
}

void CX86Ops::AddX86RegToX86Reg(x86Reg Destination, x86Reg Source)
{
    CodeLog("      add %s, %s", x86_Name(Destination), x86_Name(Source));
    AddCode16((uint16_t)(0xC003 + (Source * 0x100) + (Destination * 0x800)));
}

void CX86Ops::AndConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    CodeLog("      and dword ptr [%s], 0x%X", VariableName, Const);
    AddCode16(0x2581);
    AddCode32((uint32_t)Variable);
    AddCode32(Const);
}

void CX86Ops::AndConstToX86Reg(x86Reg Reg, uint32_t Const)
{
    CodeLog("      and %s, %Xh", x86_Name(Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        AddCode16((uint16_t)(0xE081 + (Reg * 0x100)));
        AddCode32(Const);
    }
    else
    {
        AddCode16((uint16_t)(0xE083 + (Reg * 0x100)));
        AddCode8((uint8_t)Const);
    }
}

void CX86Ops::AndVariableDispToX86Reg(x86Reg Reg, void * Variable, const char * VariableName, x86Reg AddrReg, Multipler Multiply)
{
    CodeLog("      and %s, dword ptr [%s+%s*%i]", x86_Name(Reg), VariableName, x86_Name(AddrReg), Multiply);

    AddCode16((uint16_t)(0x0423 + (Reg * 0x800)));
    AddCode8((uint8_t)(0x05 + CalcMultiplyCode(Multiply) + (AddrReg * 0x8)));
    AddCode32((uint32_t)(Variable));
}

void CX86Ops::AndVariableToX86Reg(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      and %s, dword ptr [%s]", x86_Name(Reg), VariableName);
    AddCode16((uint16_t)(0x0523 + (Reg * 0x800)));
    AddCode32((uint32_t)(Variable));
}

void CX86Ops::AndX86RegToX86Reg(x86Reg Destination, x86Reg Source)
{
    CodeLog("      and %s, %s", x86_Name(Destination), x86_Name(Source));
    AddCode16((uint16_t)(0xC021 + (Destination * 0x100) + (Source * 0x800)));
}

void CX86Ops::BreakPointNotification(const char * FileName, int32_t LineNumber)
{
    g_Notify->BreakPoint(FileName, LineNumber);
}

void CX86Ops::X86HardBreakPoint()
{
    CodeLog("      int 3");
    AddCode8(0xCC);
}

void CX86Ops::X86BreakPoint(const char * FileName, int32_t LineNumber)
{
    Pushad();
    PushImm32(stdstr_f("%d", LineNumber).c_str(), LineNumber);
    PushImm32(FileName, (uint32_t)FileName);
    CallFunc((uint32_t)BreakPointNotification, "BreakPointNotification");
    AddConstToX86Reg(x86_ESP, 8);
    Popad();
}

void CX86Ops::CallFunc(uint32_t FunctPtr, const char * FunctName)
{
    CodeLog("      call offset %s", FunctName);
    AddCode8(0xE8);
    AddCode32(FunctPtr - (uint32_t)*g_RecompPos - 4);
}

#ifdef _MSC_VER
void CX86Ops::CallThis(uint32_t ThisPtr, uint32_t FunctPtr, char * FunctName, uint32_t /*StackSize*/)
{
    MoveConstToX86reg(CX86Ops::x86_ECX, ThisPtr);
    CallFunc(FunctPtr, FunctName);
}
#else
void CX86Ops::CallThis(uint32_t ThisPtr, uint32_t FunctPtr, char * FunctName, uint32_t StackSize)
{
    PushImm32(ThisPtr);
    CallFunc(FunctPtr, FunctName);
    AddConstToX86Reg(CX86Ops::x86_ESP, StackSize);
}
#endif

void CX86Ops::CompConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    CodeLog("      cmp dword ptr [%s], 0x%X", VariableName, Const);
    AddCode16(0x3D81);
    AddCode32((uint32_t)Variable);
    AddCode32(Const);
}

void CX86Ops::CompConstToX86reg(x86Reg Reg, uint32_t Const)
{
    if (Const == 0)
    {
        OrX86RegToX86Reg(Reg, Reg);
    }
    else
    {
        CodeLog("      cmp %s, %Xh", x86_Name(Reg), Const);
        if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
        {
            AddCode16((uint16_t)(0xF881 + (Reg * 0x100)));
            AddCode32(Const);
        }
        else
        {
            AddCode16((uint16_t)(0xF883 + (Reg * 0x100)));
            AddCode8((uint8_t)Const);
        }
    }
}

void CX86Ops::CompConstToX86regPointer(x86Reg Reg, uint32_t Const)
{
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        CodeLog("      cmp dword ptr [%s], %Xh", x86_Name(Reg), Const);
        AddCode16((uint16_t)(0x3881 + (Reg * 0x100)));
        AddCode32(Const);
    }
    else
    {
        CodeLog("      cmp byte ptr [%s], %Xh", x86_Name(Reg), Const);
        AddCode16((uint16_t)(0x3883 + (Reg * 0x100)));
        AddCode8((uint8_t)Const);
    }
}

void CX86Ops::CompX86regToVariable(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      cmp %s, dword ptr [%s]", x86_Name(Reg), VariableName);
    AddCode16((uint16_t)(0x053B + (Reg * 0x800)));
    AddCode32((uint32_t)Variable);
}

void CX86Ops::CompVariableToX86reg(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      cmp dword ptr [%s], %s", VariableName, x86_Name(Reg));
    AddCode16((uint16_t)(0x0539 + (Reg * 0x800)));
    AddCode32((uint32_t)Variable);
}

void CX86Ops::CompX86RegToX86Reg(x86Reg Destination, x86Reg Source)
{
    uint16_t x86Command = 0;

    CodeLog("      cmp %s, %s", x86_Name(Destination), x86_Name(Source));

    switch (Source)
    {
    case x86_EAX: x86Command = 0x003B; break;
    case x86_EBX: x86Command = 0x033B; break;
    case x86_ECX: x86Command = 0x013B; break;
    case x86_EDX: x86Command = 0x023B; break;
    case x86_ESI: x86Command = 0x063B; break;
    case x86_EDI: x86Command = 0x073B; break;
    case x86_ESP: x86Command = 0x043B; break;
    case x86_EBP: x86Command = 0x053B; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    }
    AddCode16(x86Command);
}

void CX86Ops::DecX86reg(x86Reg Reg)
{
    CodeLog("      dec %s", x86_Name(Reg));

    switch (Reg)
    {
    case x86_EAX: AddCode16(0xC8FF); break;
    case x86_EBX: AddCode16(0xCBFF); break;
    case x86_ECX: AddCode16(0xC9FF); break;
    case x86_EDX: AddCode16(0xCAFF); break;
    case x86_ESI: AddCode16(0xCEFF); break;
    case x86_EDI: AddCode16(0xCFFF); break;
    case x86_ESP: AddCode8(0x4C); break;
    case x86_EBP: AddCode8(0x4D); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::DivX86reg(x86Reg Reg)
{
    CodeLog("      div %s", x86_Name(Reg));
    switch (Reg)
    {
    case x86_EBX: AddCode16(0xf3F7); break;
    case x86_ECX: AddCode16(0xf1F7); break;
    case x86_EDX: AddCode16(0xf2F7); break;
    case x86_ESI: AddCode16(0xf6F7); break;
    case x86_EDI: AddCode16(0xf7F7); break;
    case x86_ESP: AddCode16(0xf4F7); break;
    case x86_EBP: AddCode16(0xf5F7); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::idivX86reg(x86Reg Reg)
{
    CodeLog("      idiv %s", x86_Name(Reg));

    switch (Reg)
    {
    case x86_EBX: AddCode16(0xfbF7); break;
    case x86_ECX: AddCode16(0xf9F7); break;
    case x86_EDX: AddCode16(0xfaF7); break;
    case x86_ESI: AddCode16(0xfeF7); break;
    case x86_EDI: AddCode16(0xffF7); break;
    case x86_ESP: AddCode16(0xfcF7); break;
    case x86_EBP: AddCode16(0xfdF7); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::imulX86reg(x86Reg Reg)
{
    CodeLog("      imul %s", x86_Name(Reg));

    switch (Reg)
    {
    case x86_EAX: AddCode16(0xE8F7); break;
    case x86_EBX: AddCode16(0xEBF7); break;
    case x86_ECX: AddCode16(0xE9F7); break;
    case x86_EDX: AddCode16(0xEAF7); break;
    case x86_ESI: AddCode16(0xEEF7); break;
    case x86_EDI: AddCode16(0xEFF7); break;
    case x86_ESP: AddCode16(0xECF7); break;
    case x86_EBP: AddCode16(0xEDF7); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::IncX86reg(x86Reg Reg)
{
    CodeLog("      inc %s", x86_Name(Reg));

    switch (Reg)
    {
    case x86_EAX: AddCode16(0xC0FF); break;
    case x86_EBX: AddCode16(0xC3FF); break;
    case x86_ECX: AddCode16(0xC1FF); break;
    case x86_EDX: AddCode16(0xC2FF); break;
    case x86_ESI: AddCode16(0xC6FF); break;
    case x86_EDI: AddCode16(0xC7FF); break;
    case x86_ESP: AddCode8(0x44); break;
    case x86_EBP: AddCode8(0x45); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::JaeLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jae $%s", Label);
    AddCode8(0x73);
    AddCode8(Value);
}

void CX86Ops::JaeLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jae $%s", Label);
    AddCode16(0x830F);
    AddCode32(Value);
}

void CX86Ops::JaLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      ja $%s", Label);
    AddCode8(0x77);
    AddCode8(Value);
}

void CX86Ops::JaLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      ja $%s", Label);
    AddCode16(0x870F);
    AddCode32(Value);
}

void CX86Ops::JbLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jb $%s", Label);
    AddCode8(0x72);
    AddCode8(Value);
}

void CX86Ops::JbLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jb $%s", Label);
    AddCode16(0x820F);
    AddCode32(Value);
}

void CX86Ops::JecxzLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jecxz $%s", Label);
    AddCode8(0xE3);
    AddCode8(Value);
}

void CX86Ops::JeLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      je $%s", Label);
    AddCode8(0x74);
    AddCode8(Value);
}

void CX86Ops::JeLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      je $%s", Label);
    AddCode16(0x840F);
    AddCode32(Value);
}

void CX86Ops::JgeLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jge $%s", Label);
    AddCode8(0x7D);
    AddCode8(Value);
}

void CX86Ops::JgeLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jge $%s", Label);
    AddCode16(0x8D0F);
    AddCode32(Value);
}

void CX86Ops::JgLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jg $%s", Label);
    AddCode8(0x7F);
    AddCode8(Value);
}

void CX86Ops::JgLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jg $%s", Label);
    AddCode16(0x8F0F);
    AddCode32(Value);
}

void CX86Ops::JleLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jle $%s", Label);
    AddCode8(0x7E);
    AddCode8(Value);
}

void CX86Ops::JleLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jle $%s", Label);
    AddCode16(0x8E0F);
    AddCode32(Value);
}

void CX86Ops::JlLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jl $%s", Label);
    AddCode8(0x7C);
    AddCode8(Value);
}

void CX86Ops::JlLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jl $%s", Label);
    AddCode16(0x8C0F);
    AddCode32(Value);
}

void CX86Ops::JmpDirectReg(x86Reg Reg)
{
    CodeLog("      jmp %s", x86_Name(Reg));

    switch (Reg)
    {
    case x86_EAX: AddCode16(0xE0ff); break;
    case x86_EBX: AddCode16(0xE3ff); break;
    case x86_ECX: AddCode16(0xE1ff); break;
    case x86_EDX: AddCode16(0xE2ff); break;
    case x86_ESI: AddCode16(0xE6ff); break;
    case x86_EDI: AddCode16(0xE7ff); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::JmpIndirectLabel32(const char * Label, uint32_t location)
{
    CodeLog("      jmp dword ptr [%s]", Label);
    AddCode16(0x25ff);
    AddCode32(location);
}

void CX86Ops::JmpIndirectReg(x86Reg Reg)
{
    CodeLog("      jmp dword ptr [%s]", x86_Name(Reg));

    switch (Reg)
    {
    case x86_EAX: AddCode16(0x20ff); break;
    case x86_EBX: AddCode16(0x23ff); break;
    case x86_ECX: AddCode16(0x21ff); break;
    case x86_EDX: AddCode16(0x22ff); break;
    case x86_ESI: AddCode16(0x26ff); break;
    case x86_EDI: AddCode16(0x27ff); break;
    case x86_ESP:
        AddCode8(0xff);
        AddCode16(0x2434);
        //	g_Notify->BreakPoint(__FILEW__,__LINE__);
        break;
    case x86_EBP:
        AddCode8(0xff);
        AddCode16(0x0065);
        //	g_Notify->BreakPoint(__FILEW__,__LINE__);
        break;
    }
}

void CX86Ops::JmpLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jmp $%s", Label);
    AddCode8(0xEB);
    AddCode8(Value);
}

void CX86Ops::JmpLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jmp $%s", Label);
    AddCode8(0xE9);
    AddCode32(Value);
}

void CX86Ops::JneLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jne $%s", Label);
    AddCode8(0x75);
    AddCode8(Value);
}

void CX86Ops::JneLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jne $%s", Label);
    AddCode16(0x850F);
    AddCode32(Value);
}

void CX86Ops::JnsLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jns $%s", Label);
    AddCode8(0x79);
    AddCode8(Value);
}

void CX86Ops::JnsLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jns $%s", Label);
    AddCode16(0x890F);
    AddCode32(Value);
}

void CX86Ops::JnzLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jnz $%s", Label);
    AddCode8(0x75);
    AddCode8(Value);
}

void CX86Ops::JnzLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jnz $%s", Label);
    AddCode16(0x850F);
    AddCode32(Value);
}

void CX86Ops::JsLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      js $%s", Label);
    AddCode16(0x880F);
    AddCode32(Value);
}

void CX86Ops::JoLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jo $%s", Label);
    AddCode16(0x800F);
    AddCode32(Value);
}

void CX86Ops::JzLabel8(const char * Label, uint8_t Value)
{
    CodeLog("      jz $%s", Label);
    AddCode8(0x74);
    AddCode8(Value);
}

void CX86Ops::JzLabel32(const char * Label, uint32_t Value)
{
    CodeLog("      jz $%s", Label);
    AddCode16(0x840F);
    AddCode32(Value);
}

void CX86Ops::LeaRegReg(x86Reg RegDest, x86Reg RegSrc, uint32_t Const, Multipler multiplier)
{
    if (Const != 0)
    {
        CodeLog("      lea %s, [%s*%i+%X]", x86_Name(RegDest), x86_Name(RegSrc), multiplier, Const);
    }
    else
    {
        CodeLog("      lea %s, [%s*%i]", x86_Name(RegDest), x86_Name(RegSrc), multiplier);
    }

    AddCode8(0x8D);
    AddCode8((uint8_t)(0x04 + (RegDest * 8)));
    AddCode8((uint8_t)(0x05 + (RegSrc * 8) + CalcMultiplyCode(multiplier)));
    AddCode32(Const);
}

void CX86Ops::LeaRegReg2(x86Reg RegDest, x86Reg RegSrc, x86Reg RegSrc2, Multipler multiplier)
{
    CodeLog("      lea %s, [%s+%s*%i]", x86_Name(RegDest), x86_Name(RegSrc), x86_Name(RegSrc2), multiplier);

    if (RegSrc2 == x86_ESP || RegSrc2 == x86_EBP)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    AddCode8(0x8D);
    AddCode8((uint8_t)(0x04 + (RegDest * 0x8)));
    AddCode8((uint8_t)(0x05 + (RegSrc * 0x8) + RegSrc2 + CalcMultiplyCode(multiplier)));
}

void CX86Ops::LeaSourceAndOffset(x86Reg x86DestReg, x86Reg x86SourceReg, int32_t offset)
{
    uint16_t x86Command = 0;

    CodeLog("      lea %s, [%s + %0Xh]", x86_Name(x86DestReg), x86_Name(x86SourceReg), offset);

    //	if ((offset & 0xFFFFFF80) != 0 && (offset & 0xFFFFFF80) != 0xFFFFFF80) {
    if (1)
    {
        switch (x86DestReg)
        {
        case x86_EAX: x86Command = 0x808D; break;
        case x86_EBX: x86Command = 0x988D; break;
        case x86_ECX: x86Command = 0x888D; break;
        case x86_EDX: x86Command = 0x908D; break;
        case x86_ESI: x86Command = 0xB08D; break;
        case x86_EDI: x86Command = 0xB88D; break;
        case x86_ESP: x86Command = 0xA08D; break;
        case x86_EBP: x86Command = 0xA88D; break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        switch (x86SourceReg)
        {
        case x86_EAX: x86Command += 0x0000; break;
        case x86_EBX: x86Command += 0x0300; break;
        case x86_ECX: x86Command += 0x0100; break;
        case x86_EDX: x86Command += 0x0200; break;
        case x86_ESI: x86Command += 0x0600; break;
        case x86_EDI: x86Command += 0x0700; break;
        case x86_ESP: x86Command += 0x0400; break;
        case x86_EBP: x86Command += 0x0500; break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        AddCode16(x86Command);
        AddCode32(offset);
    }
    else
    {
        switch (x86DestReg)
        {
        case x86_EAX: x86Command = 0x408D; break;
        case x86_EBX: x86Command = 0x588D; break;
        case x86_ECX: x86Command = 0x488D; break;
        case x86_EDX: x86Command = 0x508D; break;
        case x86_ESI: x86Command = 0x708D; break;
        case x86_EDI: x86Command = 0x788D; break;
        case x86_ESP: x86Command = 0x608D; break;
        case x86_EBP: x86Command = 0x688D; break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        switch (x86SourceReg)
        {
        case x86_EAX: x86Command += 0x0000; break;
        case x86_EBX: x86Command += 0x0300; break;
        case x86_ECX: x86Command += 0x0100; break;
        case x86_EDX: x86Command += 0x0200; break;
        case x86_ESI: x86Command += 0x0600; break;
        case x86_EDI: x86Command += 0x0700; break;
        case x86_ESP: x86Command += 0x0400; break;
        case x86_EBP: x86Command += 0x0500; break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        AddCode16(x86Command);
        AddCode8((uint8_t)offset);
    }
}

void CX86Ops::MoveConstByteToN64Mem(uint8_t Const, x86Reg AddrReg)
{
    CodeLog("      mov byte ptr [%s+N64mem], %Xh", x86_Name(AddrReg), Const);
    switch (AddrReg)
    {
    case x86_EAX: AddCode16(0x80C6); break;
    case x86_EBX: AddCode16(0x83C6); break;
    case x86_ECX: AddCode16(0x81C6); break;
    case x86_EDX: AddCode16(0x82C6); break;
    case x86_ESI: AddCode16(0x86C6); break;
    case x86_EDI: AddCode16(0x87C6); break;
    case x86_ESP: AddCode16(0x84C6); break;
    case x86_EBP: AddCode16(0x85C6); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)g_MMU->Rdram());
    AddCode8(Const);
}

void CX86Ops::MoveConstByteToVariable(uint8_t Const, void * Variable, const char * VariableName)
{
    CodeLog("      mov byte ptr [%s], %Xh", VariableName, Const);
    AddCode16(0x05C6);
    AddCode32((uint32_t)Variable);
    AddCode8(Const);
}

void CX86Ops::MoveConstHalfToN64Mem(uint16_t Const, x86Reg AddrReg)
{
    CodeLog("      mov word ptr [%s+N64mem], %Xh", x86_Name(AddrReg), Const);
    AddCode8(0x66);
    switch (AddrReg)
    {
    case x86_EAX: AddCode16(0x80C7); break;
    case x86_EBX: AddCode16(0x83C7); break;
    case x86_ECX: AddCode16(0x81C7); break;
    case x86_EDX: AddCode16(0x82C7); break;
    case x86_ESI: AddCode16(0x86C7); break;
    case x86_EDI: AddCode16(0x87C7); break;
    case x86_ESP: AddCode16(0x84C7); break;
    case x86_EBP: AddCode16(0x85C7); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)g_MMU->Rdram());
    AddCode16(Const);
}

void CX86Ops::MoveConstHalfToVariable(uint16_t Const, void * Variable, const char * VariableName)
{
    CodeLog("      mov word ptr [%s], %Xh", VariableName, Const);
    AddCode8(0x66);
    AddCode16(0x05C7);
    AddCode32((uint32_t)Variable);
    AddCode16(Const);
}

void CX86Ops::MoveConstHalfToX86regPointer(uint16_t Const, x86Reg AddrReg1, x86Reg AddrReg2)
{
    uint8_t Param = 0;

    CodeLog("      mov word ptr [%s+%s],%Xh", x86_Name(AddrReg1), x86_Name(AddrReg2), Const);

    AddCode8(0x66);
    AddCode16(0x04C7);

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
    AddCode16(Const);
}

void CX86Ops::MoveConstToMemoryDisp(uint32_t Const, x86Reg AddrReg, uint32_t Disp)
{
    CodeLog("      mov dword ptr [%s+%Xh], %Xh", x86_Name(AddrReg), Disp, Const);
    switch (AddrReg)
    {
    case x86_EAX: AddCode16(0x80C7); break;
    case x86_EBX: AddCode16(0x83C7); break;
    case x86_ECX: AddCode16(0x81C7); break;
    case x86_EDX: AddCode16(0x82C7); break;
    case x86_ESI: AddCode16(0x86C7); break;
    case x86_EDI: AddCode16(0x87C7); break;
    case x86_ESP: AddCode16(0x84C7); break;
    case x86_EBP: AddCode16(0x85C7); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32(Disp);
    AddCode32(Const);
}

void CX86Ops::MoveConstToN64Mem(uint32_t Const, x86Reg AddrReg)
{
    CodeLog("      mov dword ptr [%s+N64mem], %Xh", x86_Name(AddrReg), Const);
    switch (AddrReg)
    {
    case x86_EAX: AddCode16(0x80C7); break;
    case x86_EBX: AddCode16(0x83C7); break;
    case x86_ECX: AddCode16(0x81C7); break;
    case x86_EDX: AddCode16(0x82C7); break;
    case x86_ESI: AddCode16(0x86C7); break;
    case x86_EDI: AddCode16(0x87C7); break;
    case x86_ESP: AddCode16(0x84C7); break;
    case x86_EBP: AddCode16(0x85C7); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)g_MMU->Rdram());
    AddCode32(Const);
}

void CX86Ops::MoveConstToN64MemDisp(uint32_t Const, x86Reg AddrReg, uint8_t Disp)
{
    CodeLog("      mov dword ptr [%s+N64mem+%Xh], %Xh", x86_Name(AddrReg), Const, Disp);
    switch (AddrReg)
    {
    case x86_EAX: AddCode16(0x80C7); break;
    case x86_EBX: AddCode16(0x83C7); break;
    case x86_ECX: AddCode16(0x81C7); break;
    case x86_EDX: AddCode16(0x82C7); break;
    case x86_ESI: AddCode16(0x86C7); break;
    case x86_EDI: AddCode16(0x87C7); break;
    case x86_ESP: AddCode16(0x84C7); break;
    case x86_EBP: AddCode16(0x85C7); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)g_MMU->Rdram() + Disp);
    AddCode32(Const);
}

void CX86Ops::MoveConstToVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    CodeLog("      mov dword ptr [%s], %Xh", VariableName, Const);
    AddCode16(0x05C7);
    AddCode32((uint32_t)Variable);
    AddCode32(Const);
}

void CX86Ops::MoveConstToX86Pointer(x86Reg X86Pointer, uint32_t Const)
{
    CodeLog("      mov dword ptr [%s], %Xh", x86_Name(X86Pointer), Const);
    AddCode16((uint16_t)(0x00C7 + (X86Pointer * 0x100)));
    AddCode32(Const);
}

void CX86Ops::MoveConstToX86reg(x86Reg Reg, uint32_t Const)
{
    if (Const == 0)
    {
        XorX86RegToX86Reg(Reg, Reg);
    }
    else
    {
        CodeLog("      mov %s, %Xh", x86_Name(Reg), Const);
        AddCode16((uint16_t)(0xC0C7 + (Reg * 0x100)));
        AddCode32(Const);
    }
}

void CX86Ops::MoveConstByteToX86regPointer(uint8_t Const, x86Reg AddrReg1, x86Reg AddrReg2)
{
    uint8_t Param = 0;

    CodeLog("      mov byte ptr [%s+%s],%Xh", x86_Name(AddrReg1), x86_Name(AddrReg2), Const);

    AddCode16(0x04C6);

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
    AddCode8(Const);
}

void CX86Ops::MoveConstToX86regPointer(uint32_t Const, x86Reg AddrReg1, x86Reg AddrReg2)
{
    uint8_t Param = 0;

    CodeLog("      mov dword ptr [%s+%s],%Xh", x86_Name(AddrReg1), x86_Name(AddrReg2), Const);

    AddCode16(0x04C7);

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
    AddCode32(Const);
}

void CX86Ops::MoveN64MemDispToX86reg(x86Reg Reg, x86Reg AddrReg, uint8_t Disp)
{
    uint16_t x86Command = 0;

    CodeLog("      mov %s, dword ptr [%s+N64mem+%Xh]", x86_Name(Reg), x86_Name(AddrReg), Disp);
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x008B; break;
    case x86_EBX: x86Command = 0x038B; break;
    case x86_ECX: x86Command = 0x018B; break;
    case x86_EDX: x86Command = 0x028B; break;
    case x86_ESI: x86Command = 0x068B; break;
    case x86_EDI: x86Command = 0x078B; break;
    case x86_ESP: x86Command = 0x048B; break;
    case x86_EBP: x86Command = 0x058B; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }
    AddCode16(x86Command);
    AddCode32((uint32_t)(g_MMU->Rdram() + Disp));
}

void CX86Ops::MoveN64MemToX86reg(x86Reg Reg, x86Reg AddrReg)
{
    uint16_t x86Command = 0;

    CodeLog("      mov %s, dword ptr [%s+N64mem]", x86_Name(Reg), x86_Name(AddrReg));

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x008B; break;
    case x86_EBX: x86Command = 0x038B; break;
    case x86_ECX: x86Command = 0x018B; break;
    case x86_EDX: x86Command = 0x028B; break;
    case x86_ESI: x86Command = 0x068B; break;
    case x86_EDI: x86Command = 0x078B; break;
    case x86_ESP: x86Command = 0x048B; break;
    case x86_EBP: x86Command = 0x058B; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }
    AddCode16(x86Command);
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::MoveN64MemToX86regByte(x86Reg Reg, x86Reg AddrReg)
{
    uint16_t x86Command = 0;

    CodeLog("      mov %s, byte ptr [%s+N64mem]", x86_ByteName(Reg), x86_Name(AddrReg));
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x008A; break;
    case x86_EBX: x86Command = 0x038A; break;
    case x86_ECX: x86Command = 0x018A; break;
    case x86_EDX: x86Command = 0x028A; break;
    case x86_ESI: x86Command = 0x068A; break;
    case x86_EDI: x86Command = 0x078A; break;
    case x86_ESP: x86Command = 0x048A; break;
    case x86_EBP: x86Command = 0x058A; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX:
        x86Command += 0x9000;
        break;
        //	case x86_ESI: x86Command += 0xB000; break;
        //	case x86_EDI: x86Command += 0xB800; break;
        //	case x86_ESP: case x86_EBP:
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
    AddCode16(x86Command);
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::MoveN64MemToX86regHalf(x86Reg Reg, x86Reg AddrReg)
{
    uint16_t x86Command = 0;

    CodeLog("      mov %s, word ptr [%s+N64mem]", x86_HalfName(Reg), x86_Name(AddrReg));

    AddCode8(0x66);
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x008B; break;
    case x86_EBX: x86Command = 0x038B; break;
    case x86_ECX: x86Command = 0x018B; break;
    case x86_EDX: x86Command = 0x028B; break;
    case x86_ESI: x86Command = 0x068B; break;
    case x86_EDI: x86Command = 0x078B; break;
    case x86_ESP: x86Command = 0x048B; break;
    case x86_EBP: x86Command = 0x058B; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }
    AddCode16(x86Command);
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::MoveSxByteX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg)
{
    uint8_t Param = 0;

    CodeLog("      movsx %s, byte ptr [%s+%s]", x86_Name(Reg), x86_Name(AddrReg1), x86_Name(AddrReg2));

    AddCode16(0xBE0F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0x04); break;
    case x86_EBX: AddCode8(0x1C); break;
    case x86_ECX: AddCode8(0x0C); break;
    case x86_EDX: AddCode8(0x14); break;
    case x86_ESI: AddCode8(0x34); break;
    case x86_EDI: AddCode8(0x3C); break;
    case x86_ESP: AddCode8(0x24); break;
    case x86_EBP: AddCode8(0x2C); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
}

void CX86Ops::MoveSxHalfX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg)
{
    uint8_t Param = 0;

    CodeLog("      movsx %s, word ptr [%s+%s]", x86_Name(Reg), x86_Name(AddrReg1), x86_Name(AddrReg2));

    AddCode16(0xBF0F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0x04); break;
    case x86_EBX: AddCode8(0x1C); break;
    case x86_ECX: AddCode8(0x0C); break;
    case x86_EDX: AddCode8(0x14); break;
    case x86_ESI: AddCode8(0x34); break;
    case x86_EDI: AddCode8(0x3C); break;
    case x86_ESP: AddCode8(0x24); break;
    case x86_EBP: AddCode8(0x2C); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
}

void CX86Ops::MoveSxN64MemToX86regByte(x86Reg Reg, x86Reg AddrReg)
{
    uint16_t x86Command = 0;

    CodeLog("      movsx %s, byte ptr [%s+Dmem]", x86_Name(Reg), x86_Name(AddrReg));
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00BE; break;
    case x86_EBX: x86Command = 0x03BE; break;
    case x86_ECX: x86Command = 0x01BE; break;
    case x86_EDX: x86Command = 0x02BE; break;
    case x86_ESI: x86Command = 0x06BE; break;
    case x86_EDI: x86Command = 0x07BE; break;
    case x86_ESP: x86Command = 0x04BE; break;
    case x86_EBP: x86Command = 0x05BE; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
    AddCode8(0x0f);
    AddCode16(x86Command);
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::MoveSxN64MemToX86regHalf(x86Reg Reg, x86Reg AddrReg)
{
    uint16_t x86Command = 0;

    CodeLog("      movsx %s, word ptr [%s+Dmem]", x86_Name(Reg), x86_Name(AddrReg));

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00BF; break;
    case x86_EBX: x86Command = 0x03BF; break;
    case x86_ECX: x86Command = 0x01BF; break;
    case x86_EDX: x86Command = 0x02BF; break;
    case x86_ESI: x86Command = 0x06BF; break;
    case x86_EDI: x86Command = 0x07BF; break;
    case x86_ESP: x86Command = 0x04BF; break;
    case x86_EBP: x86Command = 0x05BF; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    }

    AddCode8(0x0f);
    AddCode16(x86Command);
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::MoveSxVariableToX86regByte(void * Variable, const char * VariableName, x86Reg Reg)
{
    CodeLog("      movsx %s, byte ptr [%s]", x86_Name(Reg), VariableName);

    AddCode16(0xbe0f);

    switch (Reg)
    {
    case x86_EAX: AddCode8(0x05); break;
    case x86_EBX: AddCode8(0x1D); break;
    case x86_ECX: AddCode8(0x0D); break;
    case x86_EDX: AddCode8(0x15); break;
    case x86_ESI: AddCode8(0x35); break;
    case x86_EDI: AddCode8(0x3D); break;
    case x86_ESP: AddCode8(0x25); break;
    case x86_EBP: AddCode8(0x2D); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)Variable);
}

void CX86Ops::MoveSxVariableToX86regHalf(void * Variable, const char * VariableName, x86Reg Reg)
{
    CodeLog("      movsx %s, word ptr [%s]", x86_Name(Reg), VariableName);

    AddCode16(0xbf0f);

    switch (Reg)
    {
    case x86_EAX: AddCode8(0x05); break;
    case x86_EBX: AddCode8(0x1D); break;
    case x86_ECX: AddCode8(0x0D); break;
    case x86_EDX: AddCode8(0x15); break;
    case x86_ESI: AddCode8(0x35); break;
    case x86_EDI: AddCode8(0x3D); break;
    case x86_ESP: AddCode8(0x25); break;
    case x86_EBP: AddCode8(0x2D); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)Variable);
}

void CX86Ops::MoveVariableToX86reg(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      mov %s, dword ptr [%s]", x86_Name(Reg), VariableName);
    switch (Reg)
    {
    case x86_EAX: AddCode16(0x058B); break;
    case x86_EBX: AddCode16(0x1D8B); break;
    case x86_ECX: AddCode16(0x0D8B); break;
    case x86_EDX: AddCode16(0x158B); break;
    case x86_ESI: AddCode16(0x358B); break;
    case x86_EDI: AddCode16(0x3D8B); break;
    case x86_ESP: AddCode16(0x258B); break;
    case x86_EBP: AddCode16(0x2D8B); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)Variable);
}

void CX86Ops::MoveVariableDispToX86Reg(void * Variable, const char * VariableName, x86Reg Reg, x86Reg AddrReg, int32_t Multiplier)
{
    int x = 0;
    CodeLog("      mov %s, dword ptr [%s+%s*%i]", x86_Name(Reg), VariableName, x86_Name(AddrReg), Multiplier);

    AddCode8(0x8B);

    switch (Reg)
    {
    case x86_EAX: AddCode8(0x04); break;
    case x86_EBX: AddCode8(0x1C); break;
    case x86_ECX: AddCode8(0x0C); break;
    case x86_EDX: AddCode8(0x14); break;
    case x86_ESI: AddCode8(0x34); break;
    case x86_EDI: AddCode8(0x3C); break;
    case x86_ESP: AddCode8(0x24); break;
    case x86_EBP: AddCode8(0x2C); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    // Put in shifter 2(01), 4(10), 8(11)
    switch (Multiplier)
    {
    case 1: x = 0; break;
    case 2: x = 0x40; break;
    case 4: x = 0x80; break;
    case 8: x = 0xC0; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    // Format xx|000000
    switch (AddrReg)
    {
    case x86_EAX: AddCode8((uint8_t)(0x05 | x)); break;
    case x86_EBX: AddCode8((uint8_t)(0x1D | x)); break;
    case x86_ECX: AddCode8((uint8_t)(0x0D | x)); break;
    case x86_EDX: AddCode8((uint8_t)(0x15 | x)); break;
    case x86_ESI: AddCode8((uint8_t)(0x35 | x)); break;
    case x86_EDI: AddCode8((uint8_t)(0x3D | x)); break;
    case x86_ESP: AddCode8((uint8_t)(0x25 | x)); break;
    case x86_EBP: AddCode8((uint8_t)(0x2D | x)); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode32((uint32_t)Variable);
}

void CX86Ops::MoveVariableToX86regByte(void * Variable, const char * VariableName, x86Reg Reg)
{
    CodeLog("      mov %s, byte ptr [%s]", x86_ByteName(Reg), VariableName);

    switch (Reg)
    {
    case x86_EAX: AddCode16(0x058A); break;
    case x86_EBX: AddCode16(0x1D8A); break;
    case x86_ECX: AddCode16(0x0D8A); break;
    case x86_EDX: AddCode16(0x158A); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)Variable);
}

void CX86Ops::MoveVariableToX86regHalf(void * Variable, const char * VariableName, x86Reg Reg)
{
    CodeLog("      mov %s, word ptr [%s]", x86_HalfName(Reg), VariableName);

    AddCode8(0x66);
    switch (Reg)
    {
    case x86_EAX: AddCode16(0x058B); break;
    case x86_EBX: AddCode16(0x1D8B); break;
    case x86_ECX: AddCode16(0x0D8B); break;
    case x86_EDX: AddCode16(0x158B); break;
    case x86_ESI: AddCode16(0x358B); break;
    case x86_EDI: AddCode16(0x3D8B); break;
    case x86_ESP: AddCode16(0x258B); break;
    case x86_EBP: AddCode16(0x2D8B); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)Variable);
}

void CX86Ops::MoveX86regByteToN64Mem(x86Reg Reg, x86Reg AddrReg)
{
    uint16_t x86Command = 0;

    CodeLog("      mov byte ptr [%s+N64mem], %s", x86_Name(AddrReg), x86_ByteName(Reg));

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x0088; break;
    case x86_EBX: x86Command = 0x0388; break;
    case x86_ECX: x86Command = 0x0188; break;
    case x86_EDX: x86Command = 0x0288; break;
    case x86_ESI: x86Command = 0x0688; break;
    case x86_EDI: x86Command = 0x0788; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::MoveX86regByteToVariable(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      mov byte ptr [%s], %s", VariableName, x86_ByteName(Reg));

    switch (Reg)
    {
    case x86_EAX: AddCode16(0x0588); break;
    case x86_EBX: AddCode16(0x1D88); break;
    case x86_ECX: AddCode16(0x0D88); break;
    case x86_EDX: AddCode16(0x1588); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)Variable);
}

void CX86Ops::MoveX86regByteToX86regPointer(x86Reg Reg, x86Reg AddrReg1, x86Reg AddrReg2)
{
    uint8_t Param = 0;

    CodeLog("      mov byte ptr [%s+%s],%s", x86_Name(AddrReg1), x86_Name(AddrReg2), x86_ByteName(Reg));

    switch (Reg)
    {
    case x86_EAX: AddCode16(0x0488); break;
    case x86_EBX: AddCode16(0x1C88); break;
    case x86_ECX: AddCode16(0x0C88); break;
    case x86_EDX: AddCode16(0x1488); break;
    case x86_ESI: AddCode16(0x3488); break;
    case x86_EDI: AddCode16(0x3C88); break;
    case x86_ESP: AddCode16(0x2488); break;
    case x86_EBP: AddCode16(0x2C88); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
}

void CX86Ops::MoveX86regHalfToN64Mem(x86Reg Reg, x86Reg AddrReg)
{
    uint16_t x86Command = 0;

    CodeLog("      mov word ptr [%s+N64mem], %s", x86_Name(AddrReg), x86_HalfName(Reg));

    AddCode8(0x66);
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x0089; break;
    case x86_EBX: x86Command = 0x0389; break;
    case x86_ECX: x86Command = 0x0189; break;
    case x86_EDX: x86Command = 0x0289; break;
    case x86_ESI: x86Command = 0x0689; break;
    case x86_EDI: x86Command = 0x0789; break;
    case x86_ESP: x86Command = 0x0489; break;
    case x86_EBP: x86Command = 0x0589; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::MoveX86regHalfToVariable(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      mov word ptr [%s], %s", VariableName, x86_HalfName(Reg));
    AddCode8(0x66);
    switch (Reg)
    {
    case x86_EAX: AddCode16(0x0589); break;
    case x86_EBX: AddCode16(0x1D89); break;
    case x86_ECX: AddCode16(0x0D89); break;
    case x86_EDX: AddCode16(0x1589); break;
    case x86_ESI: AddCode16(0x3589); break;
    case x86_EDI: AddCode16(0x3D89); break;
    case x86_ESP: AddCode16(0x2589); break;
    case x86_EBP: AddCode16(0x2D89); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)Variable);
}

void CX86Ops::MoveX86regHalfToX86regPointer(x86Reg Reg, x86Reg AddrReg1, x86Reg AddrReg2)
{
    uint8_t Param = 0;

    CodeLog("      mov word ptr [%s+%s],%s", x86_Name(AddrReg1), x86_Name(AddrReg2), x86_HalfName(Reg));

    AddCode8(0x66);
    switch (Reg)
    {
    case x86_EAX: AddCode16(0x0489); break;
    case x86_EBX: AddCode16(0x1C89); break;
    case x86_ECX: AddCode16(0x0C89); break;
    case x86_EDX: AddCode16(0x1489); break;
    case x86_ESI: AddCode16(0x3489); break;
    case x86_EDI: AddCode16(0x3C89); break;
    case x86_ESP: AddCode16(0x2489); break;
    case x86_EBP: AddCode16(0x2C89); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
}

void CX86Ops::MoveX86PointerToX86reg(x86Reg Reg, x86Reg X86Pointer)
{
    uint16_t x86Command = 0;

    CodeLog("      mov %s, dword ptr [%s]", x86_Name(Reg), x86_Name(X86Pointer));

    switch (X86Pointer)
    {
    case x86_EAX: x86Command = 0x008B; break;
    case x86_EBX: x86Command = 0x038B; break;
    case x86_ECX: x86Command = 0x018B; break;
    case x86_EDX: x86Command = 0x028B; break;
    case x86_ESI: x86Command = 0x068B; break;
    case x86_EDI: x86Command = 0x078B; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (Reg)
    {
    case x86_EAX: x86Command += 0x0000; break;
    case x86_EBX: x86Command += 0x1800; break;
    case x86_ECX: x86Command += 0x0800; break;
    case x86_EDX: x86Command += 0x1000; break;
    case x86_ESI: x86Command += 0x3000; break;
    case x86_EDI: x86Command += 0x3800; break;
    case x86_ESP: x86Command += 0x2000; break;
    case x86_EBP: x86Command += 0x2800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
}

void CX86Ops::MoveX86PointerToX86regDisp(x86Reg Reg, x86Reg X86Pointer, uint8_t Disp)
{
    uint16_t x86Command = 0;

    CodeLog("      mov %s, dword ptr [%s] + %d", x86_Name(Reg), x86_Name(X86Pointer), Disp);

    switch (X86Pointer)
    {
    case x86_EAX: x86Command = 0x408B; break;
    case x86_EBX: x86Command = 0x438B; break;
    case x86_ECX: x86Command = 0x418B; break;
    case x86_EDX: x86Command = 0x428B; break;
    case x86_ESI: x86Command = 0x468B; break;
    case x86_EDI: x86Command = 0x478B; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (Reg)
    {
    case x86_EAX: x86Command += 0x0000; break;
    case x86_EBX: x86Command += 0x1800; break;
    case x86_ECX: x86Command += 0x0800; break;
    case x86_EDX: x86Command += 0x1000; break;
    case x86_ESI: x86Command += 0x3000; break;
    case x86_EDI: x86Command += 0x3800; break;
    case x86_ESP: x86Command += 0x2000; break;
    case x86_EBP: x86Command += 0x2800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
    AddCode8(Disp);
}

void CX86Ops::MoveX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg)
{
    uint8_t Param = 0;

    CodeLog("      mov %s, dword ptr [%s+%s]", x86_Name(Reg), x86_Name(AddrReg1), x86_Name(AddrReg2));

    switch (Reg)
    {
    case x86_EAX: AddCode16(0x048B); break;
    case x86_EBX: AddCode16(0x1C8B); break;
    case x86_ECX: AddCode16(0x0C8B); break;
    case x86_EDX: AddCode16(0x148B); break;
    case x86_ESI: AddCode16(0x348B); break;
    case x86_EDI: AddCode16(0x3C8B); break;
    case x86_ESP: AddCode16(0x248B); break;
    case x86_EBP: AddCode16(0x2C8B); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
}

void CX86Ops::MoveX86regPointerToX86regDisp8(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg, uint8_t offset)
{
    uint8_t Param = 0;

    CodeLog("      mov %s, dword ptr [%s+%s]", x86_Name(Reg), x86_Name(AddrReg1), x86_Name(AddrReg2));

    switch (Reg)
    {
    case x86_EAX: AddCode16(0x448B); break;
    case x86_EBX: AddCode16(0x5C8B); break;
    case x86_ECX: AddCode16(0x4C8B); break;
    case x86_EDX: AddCode16(0x548B); break;
    case x86_ESI: AddCode16(0x748B); break;
    case x86_EDI: AddCode16(0x7C8B); break;
    case x86_ESP: AddCode16(0x648B); break;
    case x86_EBP: AddCode16(0x6C8B); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
    AddCode8(offset);
}

void CX86Ops::MoveX86regToMemory(x86Reg Reg, x86Reg AddrReg, uint32_t Disp)
{
    uint16_t x86Command = 0;

    CodeLog("      mov dword ptr [%s+%X], %s", x86_Name(AddrReg), Disp, x86_Name(Reg));
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x0089; break;
    case x86_EBX: x86Command = 0x0389; break;
    case x86_ECX: x86Command = 0x0189; break;
    case x86_EDX: x86Command = 0x0289; break;
    case x86_ESI: x86Command = 0x0689; break;
    case x86_EDI: x86Command = 0x0789; break;
    case x86_ESP: x86Command = 0x0489; break;
    case x86_EBP: x86Command = 0x0589; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
    AddCode32(Disp);
}

void CX86Ops::MoveX86regToN64Mem(x86Reg Reg, x86Reg AddrReg)
{
    uint16_t x86Command = 0;

    CodeLog("      mov dword ptr [%s+N64mem], %s", x86_Name(AddrReg), x86_Name(Reg));
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x0089; break;
    case x86_EBX: x86Command = 0x0389; break;
    case x86_ECX: x86Command = 0x0189; break;
    case x86_EDX: x86Command = 0x0289; break;
    case x86_ESI: x86Command = 0x0689; break;
    case x86_EDI: x86Command = 0x0789; break;
    case x86_ESP: x86Command = 0x0489; break;
    case x86_EBP: x86Command = 0x0589; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::MoveX86regToN64MemDisp(x86Reg Reg, x86Reg AddrReg, uint8_t Disp)
{
    CodeLog("      mov dword ptr [%s+N64mem+%d], %s", x86_Name(AddrReg), Disp, x86_Name(Reg));
    uint16_t x86Command = 0;

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x0089; break;
    case x86_EBX: x86Command = 0x0389; break;
    case x86_ECX: x86Command = 0x0189; break;
    case x86_EDX: x86Command = 0x0289; break;
    case x86_ESI: x86Command = 0x0689; break;
    case x86_EDI: x86Command = 0x0789; break;
    case x86_ESP: x86Command = 0x0489; break;
    case x86_EBP: x86Command = 0x0589; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
    AddCode32((uint32_t)(g_MMU->Rdram() + Disp));
}

void CX86Ops::MoveX86regToVariable(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      mov dword ptr [%s], %s", VariableName, x86_Name(Reg));
    switch (Reg)
    {
    case x86_EAX: AddCode16(0x0589); break;
    case x86_EBX: AddCode16(0x1D89); break;
    case x86_ECX: AddCode16(0x0D89); break;
    case x86_EDX: AddCode16(0x1589); break;
    case x86_ESI: AddCode16(0x3589); break;
    case x86_EDI: AddCode16(0x3D89); break;
    case x86_ESP: AddCode16(0x2589); break;
    case x86_EBP: AddCode16(0x2D89); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)(Variable));
}

void CX86Ops::MoveX86RegToX86Reg(x86Reg Destination, x86Reg Source)
{
    uint16_t x86Command = 0;

    if (Source == Destination)
    {
        return;
    }
    CodeLog("      mov %s, %s", x86_Name(Destination), x86_Name(Source));

    switch (Destination)
    {
    case x86_EAX: x86Command = 0x0089; break;
    case x86_EBX: x86Command = 0x0389; break;
    case x86_ECX: x86Command = 0x0189; break;
    case x86_EDX: x86Command = 0x0289; break;
    case x86_ESI: x86Command = 0x0689; break;
    case x86_EDI: x86Command = 0x0789; break;
    case x86_ESP: x86Command = 0x0489; break;
    case x86_EBP: x86Command = 0x0589; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (Source)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
}

void CX86Ops::MoveX86regToX86Pointer(x86Reg X86Pointer, x86Reg Reg)
{
    uint16_t x86Command = 0;

    CodeLog("      mov dword ptr [%s], %s", x86_Name(X86Pointer), x86_Name(Reg));

    switch (X86Pointer)
    {
    case x86_EAX: x86Command = 0x0089; break;
    case x86_EBX: x86Command = 0x0389; break;
    case x86_ECX: x86Command = 0x0189; break;
    case x86_EDX: x86Command = 0x0289; break;
    case x86_ESI: x86Command = 0x0689; break;
    case x86_EDI: x86Command = 0x0789; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (Reg)
    {
    case x86_EAX: x86Command += 0x0000; break;
    case x86_EBX: x86Command += 0x1800; break;
    case x86_ECX: x86Command += 0x0800; break;
    case x86_EDX: x86Command += 0x1000; break;
    case x86_ESI: x86Command += 0x3000; break;
    case x86_EDI: x86Command += 0x3800; break;
    case x86_ESP: x86Command += 0x2000; break;
    case x86_EBP: x86Command += 0x2800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
}

void CX86Ops::MoveX86regToX86regPointer(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg)
{
    uint8_t Param = 0;

    CodeLog("      mov dword ptr [%s+%s],%s", x86_Name(AddrReg1), x86_Name(AddrReg2), x86_Name(Reg));

    switch (Reg)
    {
    case x86_EAX: AddCode16(0x0489); break;
    case x86_EBX: AddCode16(0x1C89); break;
    case x86_ECX: AddCode16(0x0C89); break;
    case x86_EDX: AddCode16(0x1489); break;
    case x86_ESI: AddCode16(0x3489); break;
    case x86_EDI: AddCode16(0x3C89); break;
    case x86_ESP: AddCode16(0x2489); break;
    case x86_EBP: AddCode16(0x2C89); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
}

void CX86Ops::MoveZxByteX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg)
{
    uint8_t Param = 0;

    CodeLog("      movzx %s, byte ptr [%s+%s]", x86_Name(Reg), x86_Name(AddrReg1), x86_Name(AddrReg2));

    AddCode16(0xB60F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0x04); break;
    case x86_EBX: AddCode8(0x1C); break;
    case x86_ECX: AddCode8(0x0C); break;
    case x86_EDX: AddCode8(0x14); break;
    case x86_ESI: AddCode8(0x34); break;
    case x86_EDI: AddCode8(0x3C); break;
    case x86_ESP: AddCode8(0x24); break;
    case x86_EBP: AddCode8(0x2C); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
}

void CX86Ops::MoveZxHalfX86regPointerToX86reg(x86Reg AddrReg1, x86Reg AddrReg2, x86Reg Reg)
{
    uint8_t Param = 0;

    CodeLog("      movzx %s, word ptr [%s+%s]", x86_Name(Reg), x86_Name(AddrReg1), x86_Name(AddrReg2));

    AddCode16(0xB70F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0x04); break;
    case x86_EBX: AddCode8(0x1C); break;
    case x86_ECX: AddCode8(0x0C); break;
    case x86_EDX: AddCode8(0x14); break;
    case x86_ESI: AddCode8(0x34); break;
    case x86_EDI: AddCode8(0x3C); break;
    case x86_ESP: AddCode8(0x24); break;
    case x86_EBP: AddCode8(0x2C); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg1)
    {
    case x86_EAX: Param = 0x00; break;
    case x86_EBX: Param = 0x03; break;
    case x86_ECX: Param = 0x01; break;
    case x86_EDX: Param = 0x02; break;
    case x86_ESI: Param = 0x06; break;
    case x86_EDI: Param = 0x07; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (AddrReg2)
    {
    case x86_EAX: Param += 0x00; break;
    case x86_EBX: Param += 0x18; break;
    case x86_ECX: Param += 0x08; break;
    case x86_EDX: Param += 0x10; break;
    case x86_ESI: Param += 0x30; break;
    case x86_EDI: Param += 0x38; break;
    case x86_ESP: Param += 0x20; break;
    case x86_EBP: Param += 0x28; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Param);
}

void CX86Ops::MoveZxN64MemToX86regByte(x86Reg Reg, x86Reg AddrReg)
{
    uint16_t x86Command = 0;

    CodeLog("      movzx %s, byte ptr [%s+g_MMU->Rdram()]", x86_Name(Reg), x86_Name(AddrReg));
    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00B6; break;
    case x86_EBX: x86Command = 0x03B6; break;
    case x86_ECX: x86Command = 0x01B6; break;
    case x86_EDX: x86Command = 0x02B6; break;
    case x86_ESI: x86Command = 0x06B6; break;
    case x86_EDI: x86Command = 0x07B6; break;
    case x86_ESP: x86Command = 0x04B6; break;
    case x86_EBP: x86Command = 0x05B6; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
    AddCode8(0x0f);
    AddCode16(x86Command);
    AddCode32((uint32_t)(g_MMU->Rdram()));
}

void CX86Ops::MoveZxN64MemToX86regHalf(x86Reg Reg, x86Reg AddrReg)
{
    uint16_t x86Command = 0;

    CodeLog("      movzx %s, word ptr [%s+g_MMU->Rdram()]", x86_Name(Reg), x86_Name(AddrReg));

    switch (AddrReg)
    {
    case x86_EAX: x86Command = 0x00B7; break;
    case x86_EBX: x86Command = 0x03B7; break;
    case x86_ECX: x86Command = 0x01B7; break;
    case x86_EDX: x86Command = 0x02B7; break;
    case x86_ESI: x86Command = 0x06B7; break;
    case x86_EDI: x86Command = 0x07B7; break;
    case x86_ESP: x86Command = 0x04B7; break;
    case x86_EBP: x86Command = 0x05B7; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Reg)
    {
    case x86_EAX: x86Command += 0x8000; break;
    case x86_EBX: x86Command += 0x9800; break;
    case x86_ECX: x86Command += 0x8800; break;
    case x86_EDX: x86Command += 0x9000; break;
    case x86_ESI: x86Command += 0xB000; break;
    case x86_EDI: x86Command += 0xB800; break;
    case x86_ESP: x86Command += 0xA000; break;
    case x86_EBP: x86Command += 0xA800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode8(0x0f);
    AddCode16(x86Command);
    AddCode32((uint32_t)(g_MMU->Rdram()));
}

void CX86Ops::MoveZxVariableToX86regByte(void * Variable, const char * VariableName, x86Reg Reg)
{
    CodeLog("      movzx %s, byte ptr [%s]", x86_Name(Reg), VariableName);

    AddCode16(0xb60f);

    switch (Reg)
    {
    case x86_EAX: AddCode8(0x05); break;
    case x86_EBX: AddCode8(0x1D); break;
    case x86_ECX: AddCode8(0x0D); break;
    case x86_EDX: AddCode8(0x15); break;
    case x86_ESI: AddCode8(0x35); break;
    case x86_EDI: AddCode8(0x3D); break;
    case x86_ESP: AddCode8(0x25); break;
    case x86_EBP: AddCode8(0x2D); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)(Variable));
}

void CX86Ops::MoveZxVariableToX86regHalf(void * Variable, const char * VariableName, x86Reg Reg)
{
    CodeLog("      movzx %s, word ptr [%s]", x86_Name(Reg), VariableName);

    AddCode16(0xb70f);

    switch (Reg)
    {
    case x86_EAX: AddCode8(0x05); break;
    case x86_EBX: AddCode8(0x1D); break;
    case x86_ECX: AddCode8(0x0D); break;
    case x86_EDX: AddCode8(0x15); break;
    case x86_ESI: AddCode8(0x35); break;
    case x86_EDI: AddCode8(0x3D); break;
    case x86_ESP: AddCode8(0x25); break;
    case x86_EBP: AddCode8(0x2D); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)(Variable));
}

void CX86Ops::MulX86reg(x86Reg Reg)
{
    CodeLog("      mul %s", x86_Name(Reg));
    switch (Reg)
    {
    case x86_EAX: AddCode16(0xE0F7); break;
    case x86_EBX: AddCode16(0xE3F7); break;
    case x86_ECX: AddCode16(0xE1F7); break;
    case x86_EDX: AddCode16(0xE2F7); break;
    case x86_ESI: AddCode16(0xE6F7); break;
    case x86_EDI: AddCode16(0xE7F7); break;
    case x86_ESP: AddCode16(0xE4F7); break;
    case x86_EBP: AddCode16(0xE5F7); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::NotX86Reg(x86Reg Reg)
{
    CodeLog("      not %s", x86_Name(Reg));
    switch (Reg)
    {
    case x86_EAX: AddCode16(0xD0F7); break;
    case x86_EBX: AddCode16(0xD3F7); break;
    case x86_ECX: AddCode16(0xD1F7); break;
    case x86_EDX: AddCode16(0xD2F7); break;
    case x86_ESI: AddCode16(0xD6F7); break;
    case x86_EDI: AddCode16(0xD7F7); break;
    case x86_ESP: AddCode16(0xD4F7); break;
    case x86_EBP: AddCode16(0xD5F7); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::OrConstToVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    CodeLog("      or dword ptr [%s], 0x%X", VariableName, Const);
    AddCode16(0x0D81);
    AddCode32((uint32_t)(Variable));
    AddCode32(Const);
}

void CX86Ops::OrConstToX86Reg(uint32_t Const, x86Reg Reg)
{
    if (Const == 0)
    {
        return;
    }
    CodeLog("      or %s, %Xh", x86_Name(Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        switch (Reg)
        {
        case x86_EAX: AddCode16(0xC881); break;
        case x86_EBX: AddCode16(0xCB81); break;
        case x86_ECX: AddCode16(0xC981); break;
        case x86_EDX: AddCode16(0xCA81); break;
        case x86_ESI: AddCode16(0xCE81); break;
        case x86_EDI: AddCode16(0xCF81); break;
        case x86_ESP: AddCode16(0xCC81); break;
        case x86_EBP: AddCode16(0xCD81); break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        AddCode32(Const);
    }
    else
    {
        switch (Reg)
        {
        case x86_EAX: AddCode16(0xC883); break;
        case x86_EBX: AddCode16(0xCB83); break;
        case x86_ECX: AddCode16(0xC983); break;
        case x86_EDX: AddCode16(0xCA83); break;
        case x86_ESI: AddCode16(0xCE83); break;
        case x86_EDI: AddCode16(0xCF83); break;
        case x86_ESP: AddCode16(0xCC83); break;
        case x86_EBP: AddCode16(0xCD83); break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        AddCode8((uint8_t)Const);
    }
}

void CX86Ops::OrVariableToX86Reg(void * Variable, const char * VariableName, x86Reg Reg)
{
    CodeLog("      or %s, dword ptr [%s]", x86_Name(Reg), VariableName);
    switch (Reg)
    {
    case x86_EAX: AddCode16(0x050B); break;
    case x86_EBX: AddCode16(0x1D0B); break;
    case x86_ECX: AddCode16(0x0D0B); break;
    case x86_EDX: AddCode16(0x150B); break;
    case x86_ESI: AddCode16(0x350B); break;
    case x86_EDI: AddCode16(0x3D0B); break;
    case x86_ESP: AddCode16(0x250B); break;
    case x86_EBP: AddCode16(0x2D0B); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)(Variable));
}

void CX86Ops::OrX86RegToVariable(void * Variable, const char * VariableName, x86Reg Reg)
{
    CodeLog("      or dword ptr [%s], %s", VariableName, x86_Name(Reg));
    switch (Reg)
    {
    case x86_EAX: AddCode16(0x0509); break;
    case x86_EBX: AddCode16(0x1D09); break;
    case x86_ECX: AddCode16(0x0D09); break;
    case x86_EDX: AddCode16(0x1509); break;
    case x86_ESI: AddCode16(0x3509); break;
    case x86_EDI: AddCode16(0x3D09); break;
    case x86_ESP: AddCode16(0x2509); break;
    case x86_EBP: AddCode16(0x2D09); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)(Variable));
}

void CX86Ops::OrX86RegToX86Reg(x86Reg Destination, x86Reg Source)
{
    uint16_t x86Command = 0;

    CodeLog("      or %s, %s", x86_Name(Destination), x86_Name(Source));
    switch (Source)
    {
    case x86_EAX: x86Command = 0x000B; break;
    case x86_EBX: x86Command = 0x030B; break;
    case x86_ECX: x86Command = 0x010B; break;
    case x86_EDX: x86Command = 0x020B; break;
    case x86_ESI: x86Command = 0x060B; break;
    case x86_EDI: x86Command = 0x070B; break;
    case x86_ESP: x86Command = 0x040B; break;
    case x86_EBP: x86Command = 0x050B; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
}

void CX86Ops::Popad(void)
{
    CodeLog("      popad");
    AddCode8(0x61);
}

void CX86Ops::Pushad(void)
{
    CodeLog("      pushad");
    AddCode8(0x60);
}

void CX86Ops::Push(x86Reg Reg)
{
    CodeLog("      push %s", x86_Name(Reg));

    switch (Reg)
    {
    case x86_EAX: AddCode8(0x50); break;
    case x86_EBX: AddCode8(0x53); break;
    case x86_ECX: AddCode8(0x51); break;
    case x86_EDX: AddCode8(0x52); break;
    case x86_ESI: AddCode8(0x56); break;
    case x86_EDI: AddCode8(0x57); break;
    case x86_ESP: AddCode8(0x54); break;
    case x86_EBP: AddCode8(0x55); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::Pop(x86Reg Reg)
{
    CodeLog("      pop %s", x86_Name(Reg));

    switch (Reg)
    {
    case x86_EAX: AddCode8(0x58); break;
    case x86_EBX: AddCode8(0x5B); break;
    case x86_ECX: AddCode8(0x59); break;
    case x86_EDX: AddCode8(0x5A); break;
    case x86_ESI: AddCode8(0x5E); break;
    case x86_EDI: AddCode8(0x5F); break;
    case x86_ESP: AddCode8(0x5C); break;
    case x86_EBP: AddCode8(0x5D); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::PushImm32(uint32_t Value)
{
    if ((Value & 0xFFFFF000) != 0)
    {
        PushImm32(stdstr_f("0x%X", Value).c_str(), Value);
    }
    else
    {
        PushImm32(stdstr_f("%d", Value).c_str(), Value);
    }
}

void CX86Ops::PushImm32(const char * String, uint32_t Value)
{
    CodeLog("      push %s", String);
    AddCode8(0x68);
    AddCode32(Value);
}

void CX86Ops::Ret(void)
{
    CodeLog("      ret");
    AddCode8(0xC3);
}

void CX86Ops::Seta(x86Reg Reg)
{
    CodeLog("      seta %s", x86_ByteName(Reg));
    AddCode16(0x970F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0xC0); break;
    case x86_EBX: AddCode8(0xC3); break;
    case x86_ECX: AddCode8(0xC1); break;
    case x86_EDX: AddCode8(0xC2); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::SetaVariable(void * Variable, const char * VariableName)
{
    CodeLog("      seta byte ptr [%s]", VariableName);
    AddCode16(0x970F);
    AddCode8(0x05);
    AddCode32((uint32_t)(Variable));
}

void CX86Ops::Setae(x86Reg Reg)
{
    CodeLog("      setae %s", x86_ByteName(Reg));
    AddCode16(0x930F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0xC0); break;
    case x86_EBX: AddCode8(0xC3); break;
    case x86_ECX: AddCode8(0xC1); break;
    case x86_EDX: AddCode8(0xC2); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::Setb(x86Reg Reg)
{
    CodeLog("      setb %s", x86_ByteName(Reg));
    AddCode16(0x920F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0xC0); break;
    case x86_EBX: AddCode8(0xC3); break;
    case x86_ECX: AddCode8(0xC1); break;
    case x86_EDX: AddCode8(0xC2); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::SetbVariable(void * Variable, const char * VariableName)
{
    CodeLog("      setb byte ptr [%s]", VariableName);
    AddCode16(0x920F);
    AddCode8(0x05);
    AddCode32((uint32_t)(Variable));
}

void CX86Ops::Setg(x86Reg Reg)
{
    CodeLog("      setg %s", x86_ByteName(Reg));
    AddCode16(0x9F0F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0xC0); break;
    case x86_EBX: AddCode8(0xC3); break;
    case x86_ECX: AddCode8(0xC1); break;
    case x86_EDX: AddCode8(0xC2); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::SetgVariable(void * Variable, const char * VariableName)
{
    CodeLog("      setg byte ptr [%s]", VariableName);
    AddCode16(0x9F0F);
    AddCode8(0x05);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::Setl(x86Reg Reg)
{
    CodeLog("      setl %s", x86_ByteName(Reg));
    AddCode16(0x9C0F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0xC0); break;
    case x86_EBX: AddCode8(0xC3); break;
    case x86_ECX: AddCode8(0xC1); break;
    case x86_EDX: AddCode8(0xC2); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::SetlVariable(void * Variable, const char * VariableName)
{
    CodeLog("      setl byte ptr [%s]", VariableName);
    AddCode16(0x9C0F);
    AddCode8(0x05);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::Setz(x86Reg Reg)
{
    CodeLog("      setz %s", x86_ByteName(Reg));
    AddCode16(0x940F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0xC0); break;
    case x86_EBX: AddCode8(0xC3); break;
    case x86_ECX: AddCode8(0xC1); break;
    case x86_EDX: AddCode8(0xC2); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::Setnz(x86Reg Reg)
{
    CodeLog("      setnz %s", x86_ByteName(Reg));
    AddCode16(0x950F);
    switch (Reg)
    {
    case x86_EAX: AddCode8(0xC0); break;
    case x86_EBX: AddCode8(0xC3); break;
    case x86_ECX: AddCode8(0xC1); break;
    case x86_EDX: AddCode8(0xC2); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::ShiftLeftDouble(x86Reg Destination, x86Reg Source)
{
    uint8_t s = 0xC0;

    CodeLog("      shld %s, %s, cl", x86_Name(Destination), x86_Name(Source));
    AddCode16(0xA50F);

    switch (Destination)
    {
    case x86_EAX: s |= 0x00; break;
    case x86_EBX: s |= 0x03; break;
    case x86_ECX: s |= 0x01; break;
    case x86_EDX: s |= 0x02; break;
    case x86_ESI: s |= 0x06; break;
    case x86_EDI: s |= 0x07; break;
    case x86_ESP: s |= 0x04; break;
    case x86_EBP: s |= 0x05; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (Source)
    {
    case x86_EAX: s |= 0x00 << 3; break;
    case x86_EBX: s |= 0x03 << 3; break;
    case x86_ECX: s |= 0x01 << 3; break;
    case x86_EDX: s |= 0x02 << 3; break;
    case x86_ESI: s |= 0x06 << 3; break;
    case x86_EDI: s |= 0x07 << 3; break;
    case x86_ESP: s |= 0x04 << 3; break;
    case x86_EBP: s |= 0x05 << 3; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode8(s);
}

void CX86Ops::ShiftLeftDoubleImmed(x86Reg Destination, x86Reg Source, uint8_t Immediate)
{
    uint8_t s = 0xC0;

    CodeLog("      shld %s, %s, %Xh", x86_Name(Destination), x86_Name(Source), Immediate);
    AddCode16(0xA40F);

    switch (Destination)
    {
    case x86_EAX: s |= 0x00; break;
    case x86_EBX: s |= 0x03; break;
    case x86_ECX: s |= 0x01; break;
    case x86_EDX: s |= 0x02; break;
    case x86_ESI: s |= 0x06; break;
    case x86_EDI: s |= 0x07; break;
    case x86_ESP: s |= 0x04; break;
    case x86_EBP: s |= 0x05; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (Source)
    {
    case x86_EAX: s |= 0x00 << 3; break;
    case x86_EBX: s |= 0x03 << 3; break;
    case x86_ECX: s |= 0x01 << 3; break;
    case x86_EDX: s |= 0x02 << 3; break;
    case x86_ESI: s |= 0x06 << 3; break;
    case x86_EDI: s |= 0x07 << 3; break;
    case x86_ESP: s |= 0x04 << 3; break;
    case x86_EBP: s |= 0x05 << 3; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode8(s);
    AddCode8(Immediate);
}

void CX86Ops::ShiftLeftSign(x86Reg Reg)
{
    CodeLog("      shl %s, cl", x86_Name(Reg));
    switch (Reg)
    {
    case x86_EAX: AddCode16(0xE0D3); break;
    case x86_EBX: AddCode16(0xE3D3); break;
    case x86_ECX: AddCode16(0xE1D3); break;
    case x86_EDX: AddCode16(0xE2D3); break;
    case x86_ESI: AddCode16(0xE6D3); break;
    case x86_EDI: AddCode16(0xE7D3); break;
    case x86_ESP: AddCode16(0xE4D3); break;
    case x86_EBP: AddCode16(0xE5D3); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::ShiftLeftSignImmed(x86Reg Reg, uint8_t Immediate)
{
    CodeLog("      shl %s, %Xh", x86_Name(Reg), Immediate);
    switch (Reg)
    {
    case x86_EAX: AddCode16(0xE0C1); break;
    case x86_EBX: AddCode16(0xE3C1); break;
    case x86_ECX: AddCode16(0xE1C1); break;
    case x86_EDX: AddCode16(0xE2C1); break;
    case x86_ESI: AddCode16(0xE6C1); break;
    case x86_EDI: AddCode16(0xE7C1); break;
    case x86_ESP: AddCode16(0xE4C1); break;
    case x86_EBP: AddCode16(0xE5C1); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Immediate);
}

void CX86Ops::ShiftRightSign(x86Reg Reg)
{
    CodeLog("      sar %s, cl", x86_Name(Reg));
    switch (Reg)
    {
    case x86_EAX: AddCode16(0xF8D3); break;
    case x86_EBX: AddCode16(0xFBD3); break;
    case x86_ECX: AddCode16(0xF9D3); break;
    case x86_EDX: AddCode16(0xFAD3); break;
    case x86_ESI: AddCode16(0xFED3); break;
    case x86_EDI: AddCode16(0xFFD3); break;
    case x86_ESP: AddCode16(0xFCD3); break;
    case x86_EBP: AddCode16(0xFDD3); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::ShiftRightSignImmed(x86Reg Reg, uint8_t Immediate)
{
    CodeLog("      sar %s, %Xh", x86_Name(Reg), Immediate);
    switch (Reg)
    {
    case x86_EAX: AddCode16(0xF8C1); break;
    case x86_EBX: AddCode16(0xFBC1); break;
    case x86_ECX: AddCode16(0xF9C1); break;
    case x86_EDX: AddCode16(0xFAC1); break;
    case x86_ESI: AddCode16(0xFEC1); break;
    case x86_EDI: AddCode16(0xFFC1); break;
    case x86_ESP: AddCode16(0xFCC1); break;
    case x86_EBP: AddCode16(0xFDC1); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Immediate);
}

void CX86Ops::ShiftRightUnsign(x86Reg Reg)
{
    CodeLog("      shr %s, cl", x86_Name(Reg));
    switch (Reg)
    {
    case x86_EAX: AddCode16(0xE8D3); break;
    case x86_EBX: AddCode16(0xEBD3); break;
    case x86_ECX: AddCode16(0xE9D3); break;
    case x86_EDX: AddCode16(0xEAD3); break;
    case x86_ESI: AddCode16(0xEED3); break;
    case x86_EDI: AddCode16(0xEFD3); break;
    case x86_ESP: AddCode16(0xECD3); break;
    case x86_EBP: AddCode16(0xEDD3); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::ShiftRightDouble(x86Reg Destination, x86Reg Source)
{
    uint8_t s = 0xC0;

    CodeLog("      shrd %s, %s, cl", x86_Name(Destination), x86_Name(Source));
    AddCode16(0xAD0F);

    switch (Destination)
    {
    case x86_EAX: s |= 0x00; break;
    case x86_EBX: s |= 0x03; break;
    case x86_ECX: s |= 0x01; break;
    case x86_EDX: s |= 0x02; break;
    case x86_ESI: s |= 0x06; break;
    case x86_EDI: s |= 0x07; break;
    case x86_ESP: s |= 0x04; break;
    case x86_EBP: s |= 0x05; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (Source)
    {
    case x86_EAX: s |= 0x00 << 3; break;
    case x86_EBX: s |= 0x03 << 3; break;
    case x86_ECX: s |= 0x01 << 3; break;
    case x86_EDX: s |= 0x02 << 3; break;
    case x86_ESI: s |= 0x06 << 3; break;
    case x86_EDI: s |= 0x07 << 3; break;
    case x86_ESP: s |= 0x04 << 3; break;
    case x86_EBP: s |= 0x05 << 3; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode8(s);
}

void CX86Ops::ShiftRightDoubleImmed(x86Reg Destination, x86Reg Source, uint8_t Immediate)
{
    uint8_t s = 0xC0;

    CodeLog("      shrd %s, %s, %Xh", x86_Name(Destination), x86_Name(Source), Immediate);
    AddCode16(0xAC0F);

    switch (Destination)
    {
    case x86_EAX: s |= 0x00; break;
    case x86_EBX: s |= 0x03; break;
    case x86_ECX: s |= 0x01; break;
    case x86_EDX: s |= 0x02; break;
    case x86_ESI: s |= 0x06; break;
    case x86_EDI: s |= 0x07; break;
    case x86_ESP: s |= 0x04; break;
    case x86_EBP: s |= 0x05; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (Source)
    {
    case x86_EAX: s |= 0x00 << 3; break;
    case x86_EBX: s |= 0x03 << 3; break;
    case x86_ECX: s |= 0x01 << 3; break;
    case x86_EDX: s |= 0x02 << 3; break;
    case x86_ESI: s |= 0x06 << 3; break;
    case x86_EDI: s |= 0x07 << 3; break;
    case x86_ESP: s |= 0x04 << 3; break;
    case x86_EBP: s |= 0x05 << 3; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode8(s);
    AddCode8(Immediate);
}

void CX86Ops::ShiftRightUnsignImmed(x86Reg Reg, uint8_t Immediate)
{
    CodeLog("      shr %s, %Xh", x86_Name(Reg), Immediate);
    switch (Reg)
    {
    case x86_EAX: AddCode16(0xE8C1); break;
    case x86_EBX: AddCode16(0xEBC1); break;
    case x86_ECX: AddCode16(0xE9C1); break;
    case x86_EDX: AddCode16(0xEAC1); break;
    case x86_ESI: AddCode16(0xEEC1); break;
    case x86_EDI: AddCode16(0xEFC1); break;
    case x86_ESP: AddCode16(0xECC1); break;
    case x86_EBP: AddCode16(0xEDC1); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode8(Immediate);
}

void CX86Ops::SbbConstFromX86Reg(x86Reg Reg, uint32_t Const)
{
    CodeLog("      sbb %s, %Xh", x86_Name(Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        switch (Reg)
        {
        case x86_EAX: AddCode16(0xD881); break;
        case x86_EBX: AddCode16(0xDB81); break;
        case x86_ECX: AddCode16(0xD981); break;
        case x86_EDX: AddCode16(0xDA81); break;
        case x86_ESI: AddCode16(0xDE81); break;
        case x86_EDI: AddCode16(0xDF81); break;
        case x86_ESP: AddCode16(0xDC81); break;
        case x86_EBP: AddCode16(0xDD81); break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        AddCode32(Const);
    }
    else
    {
        switch (Reg)
        {
        case x86_EAX: AddCode16(0xD883); break;
        case x86_EBX: AddCode16(0xDB83); break;
        case x86_ECX: AddCode16(0xD983); break;
        case x86_EDX: AddCode16(0xDA83); break;
        case x86_ESI: AddCode16(0xDE83); break;
        case x86_EDI: AddCode16(0xDF83); break;
        case x86_ESP: AddCode16(0xDC83); break;
        case x86_EBP: AddCode16(0xDD83); break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        AddCode8((uint8_t)Const);
    }
}

void CX86Ops::SbbVariableFromX86reg(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      sbb %s, dword ptr [%s]", x86_Name(Reg), VariableName);
    switch (Reg)
    {
    case x86_EAX: AddCode16(0x051B); break;
    case x86_EBX: AddCode16(0x1D1B); break;
    case x86_ECX: AddCode16(0x0D1B); break;
    case x86_EDX: AddCode16(0x151B); break;
    case x86_ESI: AddCode16(0x351B); break;
    case x86_EDI: AddCode16(0x3D1B); break;
    case x86_ESP: AddCode16(0x251B); break;
    case x86_EBP: AddCode16(0x2D1B); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)Variable);
}

void CX86Ops::SbbX86RegToX86Reg(x86Reg Destination, x86Reg Source)
{
    uint16_t x86Command = 0;
    CodeLog("      sbb %s, %s", x86_Name(Destination), x86_Name(Source));
    switch (Source)
    {
    case x86_EAX: x86Command = 0x001B; break;
    case x86_EBX: x86Command = 0x031B; break;
    case x86_ECX: x86Command = 0x011B; break;
    case x86_EDX: x86Command = 0x021B; break;
    case x86_ESI: x86Command = 0x061B; break;
    case x86_EDI: x86Command = 0x071B; break;
    case x86_ESP: x86Command = 0x041B; break;
    case x86_EBP: x86Command = 0x051B; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
}

void CX86Ops::SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    CodeLog("      sub dword ptr [%s], 0x%X", VariableName, Const);

    AddCode16(0x2D81);
    AddCode32((uint32_t)Variable);
    AddCode32(Const);
}

void CX86Ops::SubConstFromX86Reg(x86Reg Reg, uint32_t Const)
{
    CodeLog("      sub %s, %Xh", x86_Name(Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        switch (Reg)
        {
        case x86_EAX: AddCode16(0xE881); break;
        case x86_EBX: AddCode16(0xEB81); break;
        case x86_ECX: AddCode16(0xE981); break;
        case x86_EDX: AddCode16(0xEA81); break;
        case x86_ESI: AddCode16(0xEE81); break;
        case x86_EDI: AddCode16(0xEF81); break;
        case x86_ESP: AddCode16(0xEC81); break;
        case x86_EBP: AddCode16(0xED81); break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        AddCode32(Const);
    }
    else
    {
        switch (Reg)
        {
        case x86_EAX: AddCode16(0xE883); break;
        case x86_EBX: AddCode16(0xEB83); break;
        case x86_ECX: AddCode16(0xE983); break;
        case x86_EDX: AddCode16(0xEA83); break;
        case x86_ESI: AddCode16(0xEE83); break;
        case x86_EDI: AddCode16(0xEF83); break;
        case x86_ESP: AddCode16(0xEC83); break;
        case x86_EBP: AddCode16(0xED83); break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        AddCode8((uint8_t)Const);
    }
}

void CX86Ops::SubVariableFromX86reg(x86Reg Reg, void * Variable, const char * VariableName)
{
    CodeLog("      sub %s, dword ptr [%s]", x86_Name(Reg), VariableName);

    switch (Reg)
    {
    case x86_EAX: AddCode16(0x052B); break;
    case x86_EBX: AddCode16(0x1D2B); break;
    case x86_ECX: AddCode16(0x0D2B); break;
    case x86_EDX: AddCode16(0x152B); break;
    case x86_ESI: AddCode16(0x352B); break;
    case x86_EDI: AddCode16(0x3D2B); break;
    case x86_ESP: AddCode16(0x252B); break;
    case x86_EBP: AddCode16(0x2D2B); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)Variable);
}

void CX86Ops::SubX86RegToX86Reg(x86Reg Destination, x86Reg Source)
{
    uint16_t x86Command = 0;
    CodeLog("      sub %s, %s", x86_Name(Destination), x86_Name(Source));

    switch (Source)
    {
    case x86_EAX: x86Command = 0x002B; break;
    case x86_EBX: x86Command = 0x032B; break;
    case x86_ECX: x86Command = 0x012B; break;
    case x86_EDX: x86Command = 0x022B; break;
    case x86_ESI: x86Command = 0x062B; break;
    case x86_EDI: x86Command = 0x072B; break;
    case x86_ESP: x86Command = 0x042B; break;
    case x86_EBP: x86Command = 0x052B; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
}

void CX86Ops::TestConstToX86Reg(uint32_t Const, x86Reg Reg)
{
    CodeLog("      test %s, 0x%X", x86_Name(Reg), Const);

    switch (Reg)
    {
    case x86_EAX: AddCode8(0xA9); break;
    case x86_EBX: AddCode16(0xC3F7); break;
    case x86_ECX: AddCode16(0xC1F7); break;
    case x86_EDX: AddCode16(0xC2F7); break;
    case x86_ESI: AddCode16(0xC6F7); break;
    case x86_EDI: AddCode16(0xC7F7); break;
    case x86_ESP: AddCode16(0xC4F7); break;
    case x86_EBP: AddCode16(0xC5F7); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32(Const);
}

void CX86Ops::TestVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    CodeLog("      test dword ptr ds:[%s], 0x%X", VariableName, Const);
    AddCode16(0x05F7);
    AddCode32((uint32_t)Variable);
    AddCode32(Const);
}

void CX86Ops::TestX86RegToX86Reg(x86Reg Destination, x86Reg Source)
{
    uint16_t x86Command = 0;
    CodeLog("      test %s, %s", x86_Name(Destination), x86_Name(Source));
    switch (Source)
    {
    case x86_EAX: x86Command = 0x0085; break;
    case x86_EBX: x86Command = 0x0385; break;
    case x86_ECX: x86Command = 0x0185; break;
    case x86_EDX: x86Command = 0x0285; break;
    case x86_ESI: x86Command = 0x0685; break;
    case x86_EDI: x86Command = 0x0785; break;
    case x86_ESP: x86Command = 0x0485; break;
    case x86_EBP: x86Command = 0x0585; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
}

void CX86Ops::TestX86ByteRegToX86Reg(x86Reg Destination, x86Reg Source)
{
    uint16_t x86Command = 0;
    CodeLog("      test %s, %s", x86_ByteName(Destination), x86_ByteName(Source));
    switch (Source)
    {
    case x86_AL: x86Command = 0x0084; break;
    case x86_BL: x86Command = 0x0384; break;
    case x86_CL: x86Command = 0x0184; break;
    case x86_DL: x86Command = 0x0284; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Destination)
    {
    case x86_AL: x86Command += 0xC000; break;
    case x86_BL: x86Command += 0xD800; break;
    case x86_CL: x86Command += 0xC800; break;
    case x86_DL: x86Command += 0xD000; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
}

void CX86Ops::XorConstToX86Reg(x86Reg Reg, uint32_t Const)
{
    CodeLog("      xor %s, %Xh", x86_Name(Reg), Const);
    if ((Const & 0xFFFFFF80) != 0 && (Const & 0xFFFFFF80) != 0xFFFFFF80)
    {
        switch (Reg)
        {
        case x86_EAX: AddCode16(0xF081); break;
        case x86_EBX: AddCode16(0xF381); break;
        case x86_ECX: AddCode16(0xF181); break;
        case x86_EDX: AddCode16(0xF281); break;
        case x86_ESI: AddCode16(0xF681); break;
        case x86_EDI: AddCode16(0xF781); break;
        case x86_ESP: AddCode16(0xF481); break;
        case x86_EBP: AddCode16(0xF581); break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        AddCode32(Const);
    }
    else
    {
        switch (Reg)
        {
        case x86_EAX: AddCode16(0xF083); break;
        case x86_EBX: AddCode16(0xF383); break;
        case x86_ECX: AddCode16(0xF183); break;
        case x86_EDX: AddCode16(0xF283); break;
        case x86_ESI: AddCode16(0xF683); break;
        case x86_EDI: AddCode16(0xF783); break;
        case x86_ESP: AddCode16(0xF483); break;
        case x86_EBP: AddCode16(0xF583); break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        AddCode8((uint8_t)Const);
    }
}

void CX86Ops::XorX86RegToX86Reg(x86Reg Source, x86Reg Destination)
{
    uint16_t x86Command = 0;

    CodeLog("      xor %s, %s", x86_Name(Source), x86_Name(Destination));

    switch (Source)
    {
    case x86_EAX: x86Command = 0x0031; break;
    case x86_EBX: x86Command = 0x0331; break;
    case x86_ECX: x86Command = 0x0131; break;
    case x86_EDX: x86Command = 0x0231; break;
    case x86_ESI: x86Command = 0x0631; break;
    case x86_EDI: x86Command = 0x0731; break;
    case x86_ESP: x86Command = 0x0431; break;
    case x86_EBP: x86Command = 0x0531; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    switch (Destination)
    {
    case x86_EAX: x86Command += 0xC000; break;
    case x86_EBX: x86Command += 0xD800; break;
    case x86_ECX: x86Command += 0xC800; break;
    case x86_EDX: x86Command += 0xD000; break;
    case x86_ESI: x86Command += 0xF000; break;
    case x86_EDI: x86Command += 0xF800; break;
    case x86_ESP: x86Command += 0xE000; break;
    case x86_EBP: x86Command += 0xE800; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode16(x86Command);
}

void CX86Ops::XorVariableToX86reg(void * Variable, const char * VariableName, x86Reg Reg)
{
    CodeLog("      Xor %s, dword ptr [%s]", x86_Name(Reg), VariableName);

    switch (Reg)
    {
    case x86_EAX: AddCode16(0x0533); break;
    case x86_EBX: AddCode16(0x1D33); break;
    case x86_ECX: AddCode16(0x0D33); break;
    case x86_EDX: AddCode16(0x1533); break;
    case x86_ESI: AddCode16(0x3533); break;
    case x86_EDI: AddCode16(0x3D33); break;
    case x86_ESP: AddCode16(0x2533); break;
    case x86_EBP: AddCode16(0x2D33); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuAbs(void)
{
    CodeLog("      fabs ST(0)");
    AddCode16(0xE1D9);
}

void CX86Ops::fpuAddDword(void * Variable, const char * VariableName)
{
    CodeLog("      fadd ST(0), dword ptr [%s]", VariableName);
    AddCode16(0x05D8);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuAddDwordRegPointer(x86Reg x86Pointer)
{
    CodeLog("      fadd ST(0), dword ptr [%s]", x86_Name(x86Pointer));
    switch (x86Pointer)
    {
    case x86_EAX: AddCode16(0x00D8); break;
    case x86_EBX: AddCode16(0x03D8); break;
    case x86_ECX: AddCode16(0x01D8); break;
    case x86_EDX: AddCode16(0x02D8); break;
    case x86_ESI: AddCode16(0x06D8); break;
    case x86_EDI: AddCode16(0x07D8); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuAddQword(void * Variable, const char * VariableName)
{
    CodeLog("      fadd ST(0), qword ptr [%s]", VariableName);
    AddCode16(0x05DC);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuAddQwordRegPointer(x86Reg x86Pointer)
{
    CodeLog("      fadd ST(0), qword ptr [%s]", x86_Name(x86Pointer));
    switch (x86Pointer)
    {
    case x86_EAX: AddCode16(0x00DC); break;
    case x86_EBX: AddCode16(0x03DC); break;
    case x86_ECX: AddCode16(0x01DC); break;
    case x86_EDX: AddCode16(0x02DC); break;
    case x86_ESI: AddCode16(0x06DC); break;
    case x86_EDI: AddCode16(0x07DC); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuAddReg(x86FpuValues x86reg)
{
    CodeLog("      fadd ST(0), %s", fpu_Name(x86reg));
    switch (x86reg)
    {
    case x86_ST0: AddCode16(0xC0D8); break;
    case x86_ST1: AddCode16(0xC1D8); break;
    case x86_ST2: AddCode16(0xC2D8); break;
    case x86_ST3: AddCode16(0xC3D8); break;
    case x86_ST4: AddCode16(0xC4D8); break;
    case x86_ST5: AddCode16(0xC5D8); break;
    case x86_ST6: AddCode16(0xC6D8); break;
    case x86_ST7: AddCode16(0xC7D8); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuAddRegPop(int32_t & StackPos, x86FpuValues Reg)
{
    CodeLog("      faddp ST(0), %s", fpu_Name(Reg));
    StackPos = (StackPos + 1) & 7;
    switch (Reg)
    {
    case x86_ST0: AddCode16(0xC0DE); break;
    case x86_ST1: AddCode16(0xC1DE); break;
    case x86_ST2: AddCode16(0xC2DE); break;
    case x86_ST3: AddCode16(0xC3DE); break;
    case x86_ST4: AddCode16(0xC4DE); break;
    case x86_ST5: AddCode16(0xC5DE); break;
    case x86_ST6: AddCode16(0xC6DE); break;
    case x86_ST7: AddCode16(0xC7DE); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuComDword(void * Variable, const char * VariableName, bool Pop)
{
    CodeLog("      fcom%s ST(0), dword ptr [%s]", m_fpupop[Pop], VariableName);
    AddCode16(Pop ? 0x1DD8 : 0x15D8);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuComDwordRegPointer(x86Reg x86Pointer, bool Pop)
{
    uint16_t x86Command;

    CodeLog("      fcom%s ST(0), dword ptr [%s]", m_fpupop[Pop], x86_Name(x86Pointer));
    switch (x86Pointer)
    {
    case x86_EAX: x86Command = 0x10D8; break;
    case x86_EBX: x86Command = 0x13D8; break;
    case x86_ECX: x86Command = 0x11D8; break;
    case x86_EDX: x86Command = 0x12D8; break;
    case x86_ESI: x86Command = 0x16D8; break;
    case x86_EDI: x86Command = 0x17D8; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (Pop)
    {
        x86Command |= 0x0800;
    }

    AddCode16(x86Command);
}

void CX86Ops::fpuComQword(void * Variable, const char * VariableName, bool Pop)
{
    CodeLog("      fcom%s ST(0), qword ptr [%s]", m_fpupop[Pop], VariableName);
    AddCode16(Pop ? 0x1DDC : 0x15DC);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuComQwordRegPointer(x86Reg x86Pointer, bool Pop)
{
    uint16_t x86Command;

    CodeLog("      fcom%s ST(0), qword ptr [%s]", m_fpupop[Pop], x86_Name(x86Pointer));
    switch (x86Pointer)
    {
    case x86_EAX: x86Command = 0x10DC; break;
    case x86_EBX: x86Command = 0x13DC; break;
    case x86_ECX: x86Command = 0x11DC; break;
    case x86_EDX: x86Command = 0x12DC; break;
    case x86_ESI: x86Command = 0x16DC; break;
    case x86_EDI: x86Command = 0x17DC; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (Pop)
    {
        x86Command |= 0x0800;
    }

    AddCode16(x86Command);
}

void CX86Ops::fpuComReg(x86FpuValues x86reg, bool Pop)
{
    int s = Pop ? 0x0800 : 0x0000;
    CodeLog("      fcom%s ST(0), %s", m_fpupop[Pop], fpu_Name(x86reg));

    switch (x86reg)
    {
    case x86_ST0: AddCode16((uint16_t)(0xD0D8 | s)); break;
    case x86_ST1: AddCode16((uint16_t)(0xD1D8 | s)); break;
    case x86_ST2: AddCode16((uint16_t)(0xD2D8 | s)); break;
    case x86_ST3: AddCode16((uint16_t)(0xD3D8 | s)); break;
    case x86_ST4: AddCode16((uint16_t)(0xD4D8 | s)); break;
    case x86_ST5: AddCode16((uint16_t)(0xD5D8 | s)); break;
    case x86_ST6: AddCode16((uint16_t)(0xD6D8 | s)); break;
    case x86_ST7: AddCode16((uint16_t)(0xD7D8 | s)); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuDivDword(void * Variable, const char * VariableName)
{
    CodeLog("      fdiv ST(0), dword ptr [%s]", VariableName);
    AddCode16(0x35D8);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuDivDwordRegPointer(x86Reg x86Pointer)
{
    CodeLog("      fdiv ST(0), dword ptr [%s]", x86_Name(x86Pointer));
    switch (x86Pointer)
    {
    case x86_EAX: AddCode16(0x30D8); break;
    case x86_EBX: AddCode16(0x33D8); break;
    case x86_ECX: AddCode16(0x31D8); break;
    case x86_EDX: AddCode16(0x32D8); break;
    case x86_ESI: AddCode16(0x36D8); break;
    case x86_EDI: AddCode16(0x37D8); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuDivQword(void * Variable, const char * VariableName)
{
    CodeLog("      fdiv ST(0), qword ptr [%s]", VariableName);
    AddCode16(0x35DC);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuDivQwordRegPointer(x86Reg x86Pointer)
{
    CodeLog("      fdiv ST(0), qword ptr [%s]", x86_Name(x86Pointer));
    switch (x86Pointer)
    {
    case x86_EAX: AddCode16(0x30DC); break;
    case x86_EBX: AddCode16(0x33DC); break;
    case x86_ECX: AddCode16(0x31DC); break;
    case x86_EDX: AddCode16(0x32DC); break;
    case x86_ESI: AddCode16(0x36DC); break;
    case x86_EDI: AddCode16(0x37DC); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuDivReg(x86FpuValues Reg)
{
    CodeLog("      fdiv ST(0), %s", fpu_Name(Reg));
    switch (Reg)
    {
    case x86_ST0: AddCode16(0xF0D8); break;
    case x86_ST1: AddCode16(0xF1D8); break;
    case x86_ST2: AddCode16(0xF2D8); break;
    case x86_ST3: AddCode16(0xF3D8); break;
    case x86_ST4: AddCode16(0xF4D8); break;
    case x86_ST5: AddCode16(0xF5D8); break;
    case x86_ST6: AddCode16(0xF6D8); break;
    case x86_ST7: AddCode16(0xF7D8); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuDivRegPop(x86FpuValues Reg)
{
    CodeLog("      fdivp ST(0), %s", fpu_Name(Reg));
    switch (Reg)
    {
    case x86_ST0: AddCode16(0xF8DE); break;
    case x86_ST1: AddCode16(0xF9DE); break;
    case x86_ST2: AddCode16(0xFADE); break;
    case x86_ST3: AddCode16(0xFBDE); break;
    case x86_ST4: AddCode16(0xFCDE); break;
    case x86_ST5: AddCode16(0xFDDE); break;
    case x86_ST6: AddCode16(0xFEDE); break;
    case x86_ST7: AddCode16(0xFFDE); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuExchange(x86FpuValues Reg)
{
    CodeLog("      fxch ST(0), %s", fpu_Name(Reg));
    switch (Reg)
    {
    case x86_ST0: AddCode16(0xC8D9); break;
    case x86_ST1: AddCode16(0xC9D9); break;
    case x86_ST2: AddCode16(0xCAD9); break;
    case x86_ST3: AddCode16(0xCBD9); break;
    case x86_ST4: AddCode16(0xCCD9); break;
    case x86_ST5: AddCode16(0xCDD9); break;
    case x86_ST6: AddCode16(0xCED9); break;
    case x86_ST7: AddCode16(0xCFD9); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuFree(x86FpuValues Reg)
{
    CodeLog("      ffree %s", fpu_Name(Reg));
    switch (Reg)
    {
    case x86_ST0: AddCode16(0xC0DD); break;
    case x86_ST1: AddCode16(0xC1DD); break;
    case x86_ST2: AddCode16(0xC2DD); break;
    case x86_ST3: AddCode16(0xC3DD); break;
    case x86_ST4: AddCode16(0xC4DD); break;
    case x86_ST5: AddCode16(0xC5DD); break;
    case x86_ST6: AddCode16(0xC6DD); break;
    case x86_ST7: AddCode16(0xC7DD); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuDecStack(int32_t & StackPos)
{
    CodeLog("      fdecstp");
    StackPos = (StackPos - 1) & 7;
    AddCode16(0xF6D9);
}

void CX86Ops::fpuIncStack(int32_t & StackPos)
{
    CodeLog("      fincstp");
    StackPos = (StackPos + 1) & 7;
    AddCode16(0xF7D9);
}

void CX86Ops::fpuLoadControl(void * Variable, const char * VariableName)
{
    CodeLog("      fldcw [%s]", VariableName);
    AddCode16(0x2DD9);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuLoadDword(int32_t & StackPos, void * Variable, const char * VariableName)
{
    CodeLog("      fld dword ptr [%s]", VariableName);
    StackPos = (StackPos - 1) & 7;
    AddCode16(0x05D9);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuLoadDwordFromX86Reg(int32_t & StackPos, x86Reg x86reg)
{
    CodeLog("      fld dword ptr [%s]", x86_Name(x86reg));
    StackPos = (StackPos - 1) & 7;
    AddCode8(0xD9);
    switch (x86reg)
    {
    case x86_EAX: AddCode8(0x00); break;
    case x86_EBX: AddCode8(0x03); break;
    case x86_ECX: AddCode8(0x01); break;
    case x86_EDX: AddCode8(0x02); break;
    case x86_ESI: AddCode8(0x06); break;
    case x86_EDI: AddCode8(0x07); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::fpuLoadDwordFromN64Mem(int32_t & StackPos, x86Reg x86reg)
{
    CodeLog("      fld dword ptr [%s+N64mem]", x86_Name(x86reg));
    StackPos = (StackPos - 1) & 7;
    switch (x86reg)
    {
    case x86_EAX: AddCode16(0x80D9); break;
    case x86_EBX: AddCode16(0x83D9); break;
    case x86_ECX: AddCode16(0x81D9); break;
    case x86_EDX: AddCode16(0x82D9); break;
    case x86_ESI: AddCode16(0x86D9); break;
    case x86_EDI: AddCode16(0x87D9); break;
    case x86_EBP: AddCode16(0x85D9); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::fpuLoadInt32bFromN64Mem(int32_t & StackPos, x86Reg x86reg)
{
    CodeLog("      fild dword ptr [%s+N64mem]", x86_Name(x86reg));
    StackPos = (StackPos - 1) & 7;
    switch (x86reg)
    {
    case x86_EAX: AddCode16(0x80DB); break;
    case x86_EBX: AddCode16(0x83DB); break;
    case x86_ECX: AddCode16(0x81DB); break;
    case x86_EDX: AddCode16(0x82DB); break;
    case x86_ESI: AddCode16(0x86DB); break;
    case x86_EDI: AddCode16(0x87DB); break;
    case x86_EBP: AddCode16(0x85DB); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::fpuLoadIntegerDword(int32_t & StackPos, void * Variable, const char * VariableName)
{
    CodeLog("      fild dword ptr [%s]", VariableName);
    StackPos = (StackPos - 1) & 7;
    AddCode16(0x05DB);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuLoadIntegerDwordFromX86Reg(int32_t & StackPos, x86Reg x86reg)
{
    CodeLog("      fild dword ptr [%s]", x86_Name(x86reg));
    StackPos = (StackPos - 1) & 7;
    AddCode8(0xDB);
    switch (x86reg)
    {
    case x86_EAX: AddCode8(0x00); break;
    case x86_EBX: AddCode8(0x03); break;
    case x86_ECX: AddCode8(0x01); break;
    case x86_EDX: AddCode8(0x02); break;
    case x86_ESI: AddCode8(0x06); break;
    case x86_EDI: AddCode8(0x07); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::fpuLoadIntegerQword(int32_t & StackPos, void * Variable, const char * VariableName)
{
    CodeLog("      fild qword ptr [%s]", VariableName);
    StackPos = (StackPos - 1) & 7;
    AddCode16(0x2DDF);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuLoadIntegerQwordFromX86Reg(int32_t & StackPos, x86Reg x86reg)
{
    CodeLog("      fild qword ptr [%s]", x86_Name(x86reg));
    StackPos = (StackPos - 1) & 7;
    AddCode8(0xDF);
    switch (x86reg)
    {
    case x86_EAX: AddCode8(0x28); break;
    case x86_EBX: AddCode8(0x2B); break;
    case x86_ECX: AddCode8(0x29); break;
    case x86_EDX: AddCode8(0x2A); break;
    case x86_ESI: AddCode8(0x2E); break;
    case x86_EDI: AddCode8(0x2F); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::fpuLoadQword(int32_t & StackPos, void * Variable, const char * VariableName)
{
    CodeLog("      fld qword ptr [%s]", VariableName);
    StackPos = (StackPos - 1) & 7;
    AddCode16(0x05DD);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuLoadQwordFromX86Reg(int32_t & StackPos, x86Reg x86reg)
{
    CodeLog("      fld qword ptr [%s]", x86_Name(x86reg));
    StackPos = (StackPos - 1) & 7;
    AddCode8(0xDD);
    switch (x86reg)
    {
    case x86_EAX: AddCode8(0x00); break;
    case x86_EBX: AddCode8(0x03); break;
    case x86_ECX: AddCode8(0x01); break;
    case x86_EDX: AddCode8(0x02); break;
    case x86_ESI: AddCode8(0x06); break;
    case x86_EDI: AddCode8(0x07); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::fpuLoadQwordFromN64Mem(int32_t & StackPos, x86Reg x86reg)
{
    CodeLog("      fld qword ptr [%s+N64mem]", x86_Name(x86reg));
    StackPos = (StackPos - 1) & 7;
    switch (x86reg)
    {
    case x86_EAX: AddCode16(0x80DD); break;
    case x86_EBX: AddCode16(0x83DD); break;
    case x86_ECX: AddCode16(0x81DD); break;
    case x86_EDX: AddCode16(0x82DD); break;
    case x86_ESI: AddCode16(0x86DD); break;
    case x86_EDI: AddCode16(0x87DD); break;
    case x86_EBP: AddCode16(0x85DD); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::fpuLoadReg(int32_t & StackPos, x86FpuValues Reg)
{
    CodeLog("      fld ST(0), %s", fpu_Name(Reg));
    StackPos = (StackPos - 1) & 7;
    switch (Reg)
    {
    case x86_ST0: AddCode16(0xC0D9); break;
    case x86_ST1: AddCode16(0xC1D9); break;
    case x86_ST2: AddCode16(0xC2D9); break;
    case x86_ST3: AddCode16(0xC3D9); break;
    case x86_ST4: AddCode16(0xC4D9); break;
    case x86_ST5: AddCode16(0xC5D9); break;
    case x86_ST6: AddCode16(0xC6D9); break;
    case x86_ST7: AddCode16(0xC7D9); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuMulDword(void * Variable, const char * VariableName)
{
    CodeLog("      fmul ST(0), dword ptr [%s]", VariableName);
    AddCode16(0x0DD8);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuMulDwordRegPointer(x86Reg x86Pointer)
{
    CodeLog("      fmul ST(0), dword ptr [%s]", x86_Name(x86Pointer));
    switch (x86Pointer)
    {
    case x86_EAX: AddCode16(0x08D8); break;
    case x86_EBX: AddCode16(0x0BD8); break;
    case x86_ECX: AddCode16(0x09D8); break;
    case x86_EDX: AddCode16(0x0AD8); break;
    case x86_ESI: AddCode16(0x0ED8); break;
    case x86_EDI: AddCode16(0x0FD8); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuMulQword(void * Variable, const char * VariableName)
{
    CodeLog("      fmul ST(0), qword ptr [%s]", VariableName);
    AddCode16(0x0DDC);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuMulQwordRegPointer(x86Reg x86Pointer)
{
    CodeLog("      fmul ST(0), qword ptr [%s]", x86_Name(x86Pointer));
    switch (x86Pointer)
    {
    case x86_EAX: AddCode16(0x08DC); break;
    case x86_EBX: AddCode16(0x0BDC); break;
    case x86_ECX: AddCode16(0x09DC); break;
    case x86_EDX: AddCode16(0x0ADC); break;
    case x86_ESI: AddCode16(0x0EDC); break;
    case x86_EDI: AddCode16(0x0FDC); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuMulReg(x86FpuValues x86reg)
{
    CodeLog("      fmul ST(0), %s", fpu_Name(x86reg));
    switch (x86reg)
    {
    case x86_ST0: AddCode16(0xC8D8); break;
    case x86_ST1: AddCode16(0xC9D8); break;
    case x86_ST2: AddCode16(0xCAD8); break;
    case x86_ST3: AddCode16(0xCBD8); break;
    case x86_ST4: AddCode16(0xCCD8); break;
    case x86_ST5: AddCode16(0xCDD8); break;
    case x86_ST6: AddCode16(0xCED8); break;
    case x86_ST7: AddCode16(0xCFD8); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuMulRegPop(x86FpuValues x86reg)
{
    CodeLog("      fmulp ST(0), %s", fpu_Name(x86reg));
    switch (x86reg)
    {
    case x86_ST0: AddCode16(0xC8DE); break;
    case x86_ST1: AddCode16(0xC9DE); break;
    case x86_ST2: AddCode16(0xCADE); break;
    case x86_ST3: AddCode16(0xCBDE); break;
    case x86_ST4: AddCode16(0xCCDE); break;
    case x86_ST5: AddCode16(0xCDDE); break;
    case x86_ST6: AddCode16(0xCEDE); break;
    case x86_ST7: AddCode16(0xCFDE); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuNeg(void)
{
    CodeLog("      fchs ST(0)");
    AddCode16(0xE0D9);
}

void CX86Ops::fpuRound(void)
{
    CodeLog("      frndint ST(0)");
    AddCode16(0xFCD9);
}

void CX86Ops::fpuSqrt(void)
{
    CodeLog("      fsqrt ST(0)");
    AddCode16(0xFAD9);
}

void CX86Ops::fpuStoreControl(void * Variable, const char * VariableName)
{
    CodeLog("      fnstcw [%s]", VariableName);
    AddCode16(0x3DD9);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuStoreDword(int32_t & StackPos, void * Variable, const char * VariableName, bool pop)
{
    CodeLog("      fst%s dword ptr [%s]", m_fpupop[pop], VariableName);

    if (pop)
    {
        StackPos = (StackPos + 1) & 7;
    }

    AddCode16(pop ? 0x1DD9 : 0x15D9);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuStoreDwordFromX86Reg(int32_t & StackPos, x86Reg x86reg, bool pop)
{
    uint8_t Command = 0;

    CodeLog("      fst%s dword ptr [%s]", m_fpupop[pop], x86_Name(x86reg));

    if (pop)
    {
        StackPos = (StackPos + 1) & 7;
    }

    AddCode8(0xD9);

    switch (x86reg)
    {
    case x86_EAX: Command = 0x10; break;
    case x86_EBX: Command = 0x13; break;
    case x86_ECX: Command = 0x11; break;
    case x86_EDX: Command = 0x12; break;
    case x86_ESI: Command = 0x16; break;
    case x86_EDI: Command = 0x17; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode8(pop ? (Command + 0x8) : Command);
}

void CX86Ops::fpuStoreDwordToN64Mem(int32_t & StackPos, x86Reg x86reg, bool Pop)
{
    int s = Pop ? 0x0800 : 0;

    CodeLog("      fst%s dword ptr [%s+N64mem]", m_fpupop[Pop], x86_Name(x86reg));

    if (Pop)
    {
        StackPos = (StackPos + 1) & 7;
    }

    switch (x86reg)
    {
    case x86_EAX: AddCode16((uint16_t)(0x90D9 | s)); break;
    case x86_EBX: AddCode16((uint16_t)(0x93D9 | s)); break;
    case x86_ECX: AddCode16((uint16_t)(0x91D9 | s)); break;
    case x86_EDX: AddCode16((uint16_t)(0x92D9 | s)); break;
    case x86_ESI: AddCode16((uint16_t)(0x96D9 | s)); break;
    case x86_EDI: AddCode16((uint16_t)(0x97D9 | s)); break;
    case x86_EBP: AddCode16((uint16_t)(0x95D9 | s)); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode32((uint32_t)g_MMU->Rdram());
}

void CX86Ops::fpuStoreIntegerDword(int32_t & StackPos, void * Variable, const char * VariableName, bool pop)
{
    CodeLog("      fist%s dword ptr [%s]", m_fpupop[pop], VariableName);

    if (pop)
    {
        StackPos = (StackPos + 1) & 7;
    }
    AddCode16(pop ? 0x1DDB : 0x15DB);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuStoreIntegerDwordFromX86Reg(int32_t & StackPos, x86Reg x86reg, bool pop)
{
    uint8_t Command = 0;

    CodeLog("      fist%s dword ptr [%s]", m_fpupop[pop], x86_Name(x86reg));

    if (pop)
    {
        StackPos = (StackPos + 1) & 7;
    }

    AddCode8(0xDB);

    switch (x86reg)
    {
    case x86_EAX: Command = 0x10; break;
    case x86_EBX: Command = 0x13; break;
    case x86_ECX: Command = 0x11; break;
    case x86_EDX: Command = 0x12; break;
    case x86_ESI: Command = 0x16; break;
    case x86_EDI: Command = 0x17; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode8(pop ? (Command + 0x8) : Command);
}

void CX86Ops::fpuStoreIntegerQword(int32_t & StackPos, void * Variable, const char * VariableName, bool pop)
{
    CodeLog("      fist%s qword ptr [%s]", m_fpupop[pop], VariableName);

    if (pop)
    {
        StackPos = (StackPos + 1) & 7;
    }

    AddCode16(pop ? 0x3DDF : 0x35DF);
    AddCode32((uint32_t)Variable);

    if (!pop)
    {
        X86BreakPoint(__FILE__, __LINE__);
    }
}

void CX86Ops::fpuStoreIntegerQwordFromX86Reg(int32_t & StackPos, x86Reg x86reg, bool pop)
{
    uint8_t Command = 0;

    CodeLog("      fist%s qword ptr [%s]", m_fpupop[pop], x86_Name(x86reg));

    if (pop)
    {
        StackPos = (StackPos + 1) & 7;
    }

    AddCode8(0xDF);

    switch (x86reg)
    {
    case x86_EAX: Command = 0x30; break;
    case x86_EBX: Command = 0x33; break;
    case x86_ECX: Command = 0x31; break;
    case x86_EDX: Command = 0x32; break;
    case x86_ESI: Command = 0x36; break;
    case x86_EDI: Command = 0x37; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode8(pop ? (Command + 0x8) : Command);
}

void CX86Ops::fpuStoreQwordFromX86Reg(int32_t & StackPos, x86Reg x86reg, bool pop)
{
    uint8_t Command = 0;

    CodeLog("      fst%s qword ptr [%s]", m_fpupop[pop], x86_Name(x86reg));

    if (pop)
    {
        StackPos = (StackPos + 1) & 7;
    }

    AddCode8(0xDD);

    switch (x86reg)
    {
    case x86_EAX: Command = 0x10; break;
    case x86_EBX: Command = 0x13; break;
    case x86_ECX: Command = 0x11; break;
    case x86_EDX: Command = 0x12; break;
    case x86_ESI: Command = 0x16; break;
    case x86_EDI: Command = 0x17; break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    AddCode8(pop ? (Command + 0x8) : Command);
}

void CX86Ops::fpuStoreStatus(void)
{
    CodeLog("      fnstsw ax");
    AddCode16(0xE0DF);
}

void CX86Ops::fpuSubDword(void * Variable, const char * VariableName)
{
    CodeLog("      fsub ST(0), dword ptr [%s]", VariableName);
    AddCode16(0x25D8);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuSubDwordRegPointer(x86Reg x86Pointer)
{
    CodeLog("      fsub ST(0), dword ptr [%s]", x86_Name(x86Pointer));
    switch (x86Pointer)
    {
    case x86_EAX: AddCode16(0x20D8); break;
    case x86_EBX: AddCode16(0x23D8); break;
    case x86_ECX: AddCode16(0x21D8); break;
    case x86_EDX: AddCode16(0x22D8); break;
    case x86_ESI: AddCode16(0x26D8); break;
    case x86_EDI: AddCode16(0x27D8); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuSubDwordReverse(void * Variable, const char * VariableName)
{
    CodeLog("      fsubr ST(0), dword ptr [%s]", VariableName);
    AddCode16(0x2DD8);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuSubQword(void * Variable, const char * VariableName)
{
    CodeLog("      fsub ST(0), qword ptr [%s]", VariableName);
    AddCode16(0x25DC);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuSubQwordRegPointer(x86Reg x86Pointer)
{
    CodeLog("      fsub ST(0), qword ptr [%s]", x86_Name(x86Pointer));
    switch (x86Pointer)
    {
    case x86_EAX: AddCode16(0x20DC); break;
    case x86_EBX: AddCode16(0x23DC); break;
    case x86_ECX: AddCode16(0x21DC); break;
    case x86_EDX: AddCode16(0x22DC); break;
    case x86_ESI: AddCode16(0x26DC); break;
    case x86_EDI: AddCode16(0x27DC); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuSubQwordReverse(void * Variable, const char * VariableName)
{
    CodeLog("      fsubr ST(0), qword ptr [%s]", VariableName);
    AddCode16(0x2DDC);
    AddCode32((uint32_t)Variable);
}

void CX86Ops::fpuSubReg(x86FpuValues x86reg)
{
    CodeLog("      fsub ST(0), %s", fpu_Name(x86reg));
    switch (x86reg)
    {
    case x86_ST0: AddCode16(0xE0D8); break;
    case x86_ST1: AddCode16(0xE1D8); break;
    case x86_ST2: AddCode16(0xE2D8); break;
    case x86_ST3: AddCode16(0xE3D8); break;
    case x86_ST4: AddCode16(0xE4D8); break;
    case x86_ST5: AddCode16(0xE5D8); break;
    case x86_ST6: AddCode16(0xE6D8); break;
    case x86_ST7: AddCode16(0xE7D8); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CX86Ops::fpuSubRegPop(x86FpuValues x86reg)
{
    CodeLog("      fsubp ST(0), %s", fpu_Name(x86reg));
    switch (x86reg)
    {
    case x86_ST0: AddCode16(0xE8DE); break;
    case x86_ST1: AddCode16(0xE9DE); break;
    case x86_ST2: AddCode16(0xEADE); break;
    case x86_ST3: AddCode16(0xEBDE); break;
    case x86_ST4: AddCode16(0xECDE); break;
    case x86_ST5: AddCode16(0xEDDE); break;
    case x86_ST6: AddCode16(0xEEDE); break;
    case x86_ST7: AddCode16(0xEFDE); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

const char * CX86Ops::x86_Name(x86Reg Reg)
{
    switch (Reg)
    {
    case x86_EAX: return "eax";
    case x86_EBX: return "ebx";
    case x86_ECX: return "ecx";
    case x86_EDX: return "edx";
    case x86_ESI: return "esi";
    case x86_EDI: return "edi";
    case x86_EBP: return "ebp";
    case x86_ESP: return "esp";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return "???";
}

const char * CX86Ops::x86_ByteName(x86Reg Reg)
{
    switch (Reg)
    {
    case x86_AL: return "al";
    case x86_BL: return "bl";
    case x86_CL: return "cl";
    case x86_DL: return "dl";
    case x86_AH: return "ah";
    case x86_BH: return "bh";
    case x86_CH: return "ch";
    case x86_DH: return "dh";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return "???";
}

const char * CX86Ops::x86_HalfName(x86Reg Reg)
{
    switch (Reg)
    {
    case x86_EAX: return "ax";
    case x86_EBX: return "bx";
    case x86_ECX: return "cx";
    case x86_EDX: return "dx";
    case x86_ESI: return "si";
    case x86_EDI: return "di";
    case x86_EBP: return "bp";
    case x86_ESP: return "sp";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return "???";
}

const char * CX86Ops::fpu_Name(x86FpuValues Reg)
{
    switch (Reg)
    {
    case x86_ST0: return "ST(0)";
    case x86_ST1: return "ST(1)";
    case x86_ST2: return "ST(2)";
    case x86_ST3: return "ST(3)";
    case x86_ST4: return "ST(4)";
    case x86_ST5: return "ST(5)";
    case x86_ST6: return "ST(6)";
    case x86_ST7: return "ST(7)";
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return "???";
}

bool CX86Ops::Is8BitReg(x86Reg Reg)
{
    return (Reg == x86_EAX) ||
           (Reg == x86_EBX) ||
           (Reg == x86_ECX) ||
           (Reg == x86_EDX);
}

uint8_t CX86Ops::CalcMultiplyCode(Multipler Multiply)
{
    switch (Multiply)
    {
    case Multip_x2: return 0x40;
    case Multip_x4: return 0x80;
    case Multip_x8: return 0xC0;
    default: return 0;
    }
}

void CX86Ops::SetJump32(uint32_t * Loc, uint32_t * JumpLoc)
{
    *Loc = (uint32_t)(((uint32_t)JumpLoc) - (((uint32_t)(Loc)) + 4));
}

void CX86Ops::SetJump8(uint8_t * Loc, uint8_t * JumpLoc)
{
    if (Loc == nullptr || JumpLoc == nullptr)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    uint32_t diffrence = (uint32_t)(((uint32_t)JumpLoc) - (((uint32_t)(Loc)) + 1));
    if (diffrence > 255)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    *Loc = (uint8_t)diffrence;
}

uint32_t CX86Ops::GetAddressOf(int value, ...)
{
    void * Address;

    va_list ap;
    va_start(ap, value);
    Address = va_arg(ap, void *);
    va_end(ap);

    return (uint32_t)Address;
}

void CX86Ops::AddCode8(uint8_t value)
{
#ifdef _DEBUG
    if (g_RecompPos == nullptr)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
#endif
    (*((uint8_t *)(*g_RecompPos)) = (uint8_t)(value));
    *g_RecompPos += 1;
}

void CX86Ops::AddCode16(uint16_t value)
{
#ifdef _DEBUG
    if (g_RecompPos == nullptr)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
#endif
    (*((uint16_t *)(*g_RecompPos)) = (uint16_t)(value));
    *g_RecompPos += 2;
}

void CX86Ops::AddCode32(uint32_t value)
{
#ifdef _DEBUG
    if (g_RecompPos == nullptr)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
#endif
    (*((uint32_t *)(*g_RecompPos)) = (uint32_t)(value));
    *g_RecompPos += 4;
}

void CX86Ops::CodeLog(_Printf_format_string_ const char * Text, ...)
{
    if (!CDebugSettings::bRecordRecompilerAsm())
    {
        return;
    }

    va_list args;
    va_start(args, Text);
#pragma warning(push)
#pragma warning(disable : 4996)
    size_t nlen = _vscprintf(Text, args) + 1;
    char * buffer = (char *)alloca(nlen * sizeof(char));
    buffer[nlen - 1] = 0;
    if (buffer != nullptr)
    {
        vsprintf(buffer, Text, args);
        m_CodeBlock.Log(buffer);
    }
#pragma warning(pop)
    va_end(args);
}

#endif
