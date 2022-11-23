#include "stdafx.h"

#if defined(__i386__) || defined(_M_IX86)
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/x86/x86ops.h>
#include <Project64-core/N64System/SystemGlobals.h>

CX86Ops::CX86Ops(CCodeBlock & CodeBlock) :
    asmjit::x86::Assembler(&CodeBlock.CodeHolder()),
    m_CodeBlock(CodeBlock)
{
    setLogger(this);
    setErrorHandler(&CodeBlock);
    addFlags(asmjit::FormatFlags::kHexOffsets);
    addFlags(asmjit::FormatFlags::kHexImms);
    addFlags(asmjit::FormatFlags::kExplainImms);
}

void CX86Ops::AdcVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        adc(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        adc(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
    }
}

void CX86Ops::AddConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        add(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        add(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
    }
}

void CX86Ops::AddConstToX86Reg(const asmjit::x86::Gp & Reg, uint32_t Const)
{
    if (Const != 0)
    {
        if (Const == 1)
        {
            inc(Reg);
        }
        else if (Const == 0xFFFFFFFF)
        {
            dec(Reg);
        }
        else
        {
            add(Reg, Const);
        }
    }
}

void CX86Ops::AddVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        add(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        add(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
    }
}

void CX86Ops::AndConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        and_(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        and_(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
    }
}

void CX86Ops::AndVariableDispToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName, const asmjit::x86::Gp & AddrReg, Multipler Multiply)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        and_(Reg, asmjit::x86::dword_ptr((uint64_t)Variable, AddrReg, Multiply >> 1));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        and_(Reg, asmjit::x86::dword_ptr((uint64_t)Variable, AddrReg, Multiply >> 1));
    }
}

void CX86Ops::AndVariableToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        and_(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        and_(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
    }
}

void CX86Ops::BreakPointNotification(const char * FileName, int32_t LineNumber)
{
    g_Notify->BreakPoint(FileName, LineNumber);
}

void CX86Ops::X86HardBreakPoint()
{
    int3();
}

void CX86Ops::X86BreakPoint(const char * FileName, int32_t LineNumber)
{
    pushad();
    PushImm32(stdstr_f("%d", LineNumber).c_str(), LineNumber);
    PushImm32(FileName, (uint32_t)FileName);
    CallFunc((uint32_t)BreakPointNotification, "BreakPointNotification");
    AddConstToX86Reg(asmjit::x86::esp, 8);
    popad();
}

void CX86Ops::CallFunc(uint32_t FunctPtr, const char * FunctName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        stdstr_f SymbolKey("0x%X", FunctPtr);
        AddSymbol(SymbolKey.c_str(), FunctName);
        call((uint64_t)FunctPtr);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        call((uint64_t)FunctPtr);
    }
}

#ifdef _MSC_VER
void CX86Ops::CallThis(uint32_t ThisPtr, uint32_t FunctPtr, char * FunctName, uint32_t /*StackSize*/)
{
    mov(asmjit::x86::ecx, ThisPtr);
    CallFunc(FunctPtr, FunctName);
}
#else
void CX86Ops::CallThis(uint32_t ThisPtr, uint32_t FunctPtr, char * FunctName, uint32_t StackSize)
{
    PushImm32(ThisPtr);
    CallFunc(FunctPtr, FunctName);
    AddConstToX86Reg(CX86Ops::asmjit::x86::esp, StackSize);
}
#endif

void CX86Ops::CompConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        cmp(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        cmp(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
    }
}

void CX86Ops::CompConstToX86reg(const asmjit::x86::Gp & Reg, uint32_t Const)
{
    if (Const == 0)
    {
        or_(Reg, Reg);
    }
    else
    {
        cmp(Reg, Const);
    }
}

void CX86Ops::CompX86regToVariable(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        cmp(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        cmp(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
    }
}

void CX86Ops::JaeLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jae(JumpLabel);
}

void CX86Ops::JaLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    ja(JumpLabel);
}

void CX86Ops::JbLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jb(JumpLabel);
}

void CX86Ops::JecxzLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jecxz(JumpLabel);
}

void CX86Ops::JeLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    je(JumpLabel);
}

void CX86Ops::JgeLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jge(JumpLabel);
}

void CX86Ops::JgLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jg(JumpLabel);
}

void CX86Ops::JleLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jle(JumpLabel);
}

void CX86Ops::JlLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jl(JumpLabel);
}

void CX86Ops::JmpLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jmp(JumpLabel);
}

void CX86Ops::JneLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jne(JumpLabel);
}

void CX86Ops::JnsLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jns(JumpLabel);
}

void CX86Ops::JnzLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jnz(JumpLabel);
}

void CX86Ops::JsLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    js(JumpLabel);
}

void CX86Ops::JoLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jo(JumpLabel);
}

void CX86Ops::JzLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddSymbol(stdstr_f("L%d", JumpLabel.id()).c_str(), LabelName);
    jz(JumpLabel);
}

void CX86Ops::MoveConstByteToVariable(void * Variable, const char * VariableName, uint8_t Const)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        mov(asmjit::x86::byte_ptr((uint64_t)Variable), Const);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        mov(asmjit::x86::byte_ptr((uint64_t)Variable), Const);
    }
}

void CX86Ops::MoveConstHalfToVariable(void * Variable, const char * VariableName, uint16_t Const)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        mov(asmjit::x86::word_ptr((uint64_t)Variable), Const);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        mov(asmjit::x86::word_ptr((uint64_t)Variable), Const);
    }
}

void CX86Ops::MoveConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        mov(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        mov(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
    }
}

void CX86Ops::MoveConstToX86reg(const asmjit::x86::Gp & Reg, uint32_t Const)
{
    if (Const == 0)
    {
        xor_(Reg, Reg);
    }
    else
    {
        mov(Reg, Const);
    }
}

void CX86Ops::MoveSxVariableToX86regByte(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        movsx(Reg, asmjit::x86::byte_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        movsx(Reg, asmjit::x86::byte_ptr((uint64_t)Variable));
    }
}

void CX86Ops::MoveSxVariableToX86regHalf(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        stdstr_f SymbolKey("0x%X", Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        movsx(Reg, asmjit::x86::word_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        movsx(Reg, asmjit::x86::word_ptr((uint64_t)Variable));
    }
}

void CX86Ops::MoveVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        mov(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        mov(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
    }
}

void CX86Ops::MoveVariableDispToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName, const asmjit::x86::Gp & AddrReg, Multipler Multiplier)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        mov(Reg, asmjit::x86::dword_ptr((uint64_t)Variable, AddrReg, Multiplier >> 1));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        mov(Reg, asmjit::x86::dword_ptr((uint64_t)Variable, AddrReg, Multiplier >> 1));
    }
}

void CX86Ops::MoveX86regByteToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);

        AddSymbol(SymbolKey.c_str(), VariableName);
        mov(asmjit::x86::byte_ptr((uint64_t)(Variable)), Reg.r8());
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        mov(asmjit::x86::byte_ptr((uint64_t)(Variable)), Reg.r8());
    }
}

void CX86Ops::MoveX86regHalfToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        
        AddSymbol(SymbolKey.c_str(), VariableName);
        mov(asmjit::x86::word_ptr((uint64_t)(Variable)), Reg.r16());
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        mov(asmjit::x86::word_ptr((uint64_t)(Variable)), Reg.r16());
    }
}

void CX86Ops::MoveX86regToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        mov(asmjit::x86::dword_ptr((uint64_t)(Variable)), Reg);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        mov(asmjit::x86::dword_ptr((uint64_t)(Variable)), Reg);
    }
}

void CX86Ops::MoveZxVariableToX86regByte(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        stdstr_f SymbolKey("0x%X", Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        movzx(Reg, asmjit::x86::byte_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        movzx(Reg, asmjit::x86::byte_ptr((uint64_t)Variable));
    }
}

void CX86Ops::MoveZxVariableToX86regHalf(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        stdstr_f SymbolKey("0x%X", Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        movzx(Reg, asmjit::x86::word_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        movzx(Reg, asmjit::x86::word_ptr((uint64_t)Variable));
    }
}

void CX86Ops::OrConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        stdstr_f SymbolKey("0x%X", Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        or_(asmjit::x86::word_ptr((uint64_t)Variable), Const);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        or_(asmjit::x86::word_ptr((uint64_t)Variable), Const);
    }
}

void CX86Ops::OrVariableToX86Reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        stdstr_f SymbolKey("0x%X", Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        or_(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        or_(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
    }
}

void CX86Ops::OrX86RegToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        stdstr_f SymbolKey("0x%X", Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        or_(asmjit::x86::dword_ptr((uint64_t)Variable), Reg);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        or_(asmjit::x86::dword_ptr((uint64_t)Variable), Reg);
    }
}

void CX86Ops::PushImm32(const char * String, uint32_t Value)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        stdstr_f SymbolKey("0x%X", Value);
        AddSymbol(SymbolKey.c_str(), String);
        push(Value);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        push(Value);
    }
}

void CX86Ops::SetaVariable(void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        seta(asmjit::x86::byte_ptr((uint32_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        seta(asmjit::x86::byte_ptr((uint32_t)Variable));
    }
}

void CX86Ops::SetbVariable(void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        setb(asmjit::x86::byte_ptr((uint32_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        setb(asmjit::x86::byte_ptr((uint32_t)Variable));
    }
}

void CX86Ops::SetgVariable(void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        setg(asmjit::x86::byte_ptr((uint32_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        setg(asmjit::x86::byte_ptr((uint32_t)Variable));
    }
}

void CX86Ops::SetlVariable(void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        setl(asmjit::x86::byte_ptr((uint32_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        setl(asmjit::x86::byte_ptr((uint32_t)Variable));
    }
}

void CX86Ops::SbbVariableFromX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        sbb(Reg, asmjit::x86::dword_ptr((uint32_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        sbb(Reg, asmjit::x86::dword_ptr((uint32_t)Variable));
    }
}

void CX86Ops::SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        sub(asmjit::x86::dword_ptr((uint32_t)Variable), Const);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        sub(asmjit::x86::dword_ptr((uint32_t)Variable), Const);
    }
}

void CX86Ops::SubVariableFromX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        sub(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        sub(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
    }
}

void CX86Ops::TestVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        test(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        test(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
    }
}

void CX86Ops::XorVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        xor_(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        xor_(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
    }
}

void CX86Ops::fpuIncStack(int32_t & StackPos)
{
    StackPos = (StackPos + 1) & 7;
    fincstp();
}

void CX86Ops::fpuLoadControl(void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        fldcw(asmjit::x86::ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        fldcw(asmjit::x86::ptr((uint64_t)Variable));
    }
}

void CX86Ops::fpuLoadDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & x86reg)
{
    fld(asmjit::x86::dword_ptr(x86reg));
    StackPos = (StackPos - 1) & 7;
}

void CX86Ops::fpuLoadIntegerDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & x86reg)
{
    fild(asmjit::x86::dword_ptr(x86reg));
    StackPos = (StackPos - 1) & 7;
}

void CX86Ops::fpuLoadIntegerQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & x86reg)
{
    fild(asmjit::x86::qword_ptr(x86reg));
    StackPos = (StackPos - 1) & 7;
}

void CX86Ops::fpuLoadQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg)
{
    StackPos = (StackPos - 1) & 7;
    fld(asmjit::x86::qword_ptr(Reg));
}

void CX86Ops::fpuLoadReg(int32_t & StackPos, const asmjit::x86::St & Reg)
{
    StackPos = (StackPos - 1) & 7;
    fld(Reg);
}

void CX86Ops::fpuStoreControl(void * Variable, const char * VariableName)
{
    if (CDebugSettings::bRecordRecompilerAsm())
    {
        std::string SymbolKey = VariableSymbol(Variable);
        AddSymbol(SymbolKey.c_str(), VariableName);
        fnstcw(asmjit::x86::ptr((uint64_t)Variable));
        RemoveSymbol(SymbolKey.c_str());
    }
    else
    {
        fnstcw(asmjit::x86::ptr((uint64_t)Variable));
    }
}

void CX86Ops::fpuStoreDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & x86reg, bool pop)
{
    if (pop)
    {
        fstp(asmjit::x86::dword_ptr(x86reg));
        StackPos = (StackPos + 1) & 7;
    }
    else
    {
        fst(asmjit::x86::dword_ptr(x86reg));
    }
}

void CX86Ops::fpuStoreIntegerDwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & x86reg, bool pop)
{
    if (pop)
    {
        fistp(asmjit::x86::dword_ptr(x86reg));
        StackPos = (StackPos + 1) & 7;
    }
    else
    {
        fist(asmjit::x86::dword_ptr(x86reg));
    }
}

void CX86Ops::fpuStoreIntegerQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool pop)
{
    if (pop)
    {
        StackPos = (StackPos + 1) & 7;
        fistp(asmjit::x86::qword_ptr(Reg));
    }
    else
    {
        fist(asmjit::x86::qword_ptr(Reg));
    }
}

void CX86Ops::fpuStoreQwordFromX86Reg(int32_t & StackPos, const asmjit::x86::Gp & Reg, bool pop)
{
    if (pop)
    {
        fstp(asmjit::x86::qword_ptr(Reg));
        StackPos = (StackPos + 1) & 7;
    }
    else
    {
        fst(asmjit::x86::qword_ptr(Reg));
    }
}

const char * CX86Ops::x86_Name(const asmjit::x86::Gp & Reg)
{
    if (Reg == asmjit::x86::eax)
    {
        return "eax";
    }
    if (Reg == asmjit::x86::ebx)
    {
        return "ebx";
    }
    if (Reg == asmjit::x86::ecx)
    {
        return "ecx";
    }
    if (Reg == asmjit::x86::edx)
    {
        return "edx";
    }
    if (Reg == asmjit::x86::esi)
    {
        return "esi";
    }
    if (Reg == asmjit::x86::edi)
    {
        return "edi";
    }
    if (Reg == asmjit::x86::ebp)
    {
        return "ebp";
    }
    if (Reg == asmjit::x86::esp)
    {
        return "esp";
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return "???";
}

bool CX86Ops::Is8BitReg(const asmjit::x86::Gp & Reg)
{
    return (Reg == asmjit::x86::eax) ||
           (Reg == asmjit::x86::ebx) ||
           (Reg == asmjit::x86::ecx) ||
           (Reg == asmjit::x86::edx);
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

CX86Ops::x86Reg CX86Ops::RegValue(const asmjit::x86::Gp & Reg)
{
    if (Reg == asmjit::x86::eax)
    {
        return x86_EAX;
    }
    else if (Reg == asmjit::x86::ebx)
    {
        return x86_EBX;
    }
    else if (Reg == asmjit::x86::ecx)
    {
        return x86_ECX;
    }
    else if (Reg == asmjit::x86::edx)
    {
        return x86_EDX;
    }
    else if (Reg == asmjit::x86::esi)
    {
        return x86_ESI;
    }
    else if (Reg == asmjit::x86::edi)
    {
        return x86_EDI;
    }
    else if (Reg == asmjit::x86::esp)
    {
        return x86_ESP;
    }
    else if (Reg == asmjit::x86::ebp)
    {
        return x86_EBP;
    }
    else if (Reg == asmjit::x86::al)
    {
        return x86_AL;
    }
    else if (Reg == asmjit::x86::bl)
    {
        return x86_BL;
    }
    else if (Reg == asmjit::x86::cl)
    {
        return x86_CL;
    }
    else if (Reg == asmjit::x86::dl)
    {
        return x86_DL;
    }
    else if (Reg == asmjit::x86::ah)
    {
        return x86_AH;
    }
    else if (Reg == asmjit::x86::bh)
    {
        return x86_BH;
    }
    else if (Reg == asmjit::x86::ch)
    {
        return x86_CH;
    }
    else if (Reg == asmjit::x86::dh)
    {
        return x86_DH;
    } 
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return x86_EAX;
}

asmjit::Error CX86Ops::_log(const char* data, size_t size) noexcept
{
    stdstr AsmjitLog(std::string(data, size));
    AsmjitLog.Trim("\n");
    for (SymbolMap::const_iterator itr = m_Symbols.begin(); itr != m_Symbols.end(); itr++)
    {
        AsmjitLog.Replace(itr->first, itr->second);
    }
    m_CodeBlock.Log("      %s", AsmjitLog.c_str());
    return asmjit::kErrorOk;
}

void CX86Ops::AddSymbol(const char * SymbolKey, const char * SymbolValue)
{
    m_Symbols.emplace(std::make_pair(SymbolKey, SymbolValue));
}

void CX86Ops::RemoveSymbol(const char * SymbolKey)
{
    SymbolMap::iterator itr = m_Symbols.find(SymbolKey);
    if (itr != m_Symbols.end())
    {
        m_Symbols.erase(itr);
    }
}

std::string CX86Ops::VariableSymbol(void * Variable) const
{
    if (int64_t(Variable) < 0)
    {
        return stdstr_f("-0x%0X", (uint32_t)(~(int64_t(Variable)) + 1));
    }
    return stdstr_f("0x%X", (uint32_t)Variable);
}

#endif
