#include "stdafx.h"
#if defined(__amd64__) || defined(_M_X64)

#include <cctype>
#include <cstring>

#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/x64-86/x64ops.h>

CX64Ops::CX64Ops(CCodeBlock & CodeBlock) :
    asmjit::x86::Assembler(&CodeBlock.CodeHolder()),
    m_CodeBlock(CodeBlock)
{
    setLogger(g_DebugSettings.recordRecompilerAsm ? this : nullptr);
    setErrorHandler(&CodeBlock);
    addFlags(asmjit::FormatFlags::kHexOffsets);
    addFlags(asmjit::FormatFlags::kHexImms);
    addFlags(asmjit::FormatFlags::kExplainImms);
    setIndentation(asmjit::FormatIndentationGroup::kCode, 2);
    setIndentation(asmjit::FormatIndentationGroup::kComment, 2);

    m_PrimarySection = CodeBlock.CodeHolder().textSection();
    CodeBlock.CodeHolder().newSection(&m_SecondarySection, ".secondary", SIZE_MAX, asmjit::SectionFlags::kNone, 8);
}

asmjit::Error CX64Ops::_log(const char * data, size_t size) noexcept
{
    stdstr AsmjitLog(std::string(data, size));
    AsmjitLog.Trim("\n");

    if (m_NumberSymbols.size() > 0)
    {
        for (std::string::size_type searchAt = 0;;)
        {
            const std::string::size_type Pos = AsmjitLog.find("0x", searchAt);
            if (Pos == std::string::npos)
            {
                break;
            }

            uintptr_t Value = 0;
            std::string::size_type endPos = Pos + 2;
            for (; endPos < AsmjitLog.length(); endPos++)
            {
                const char c = AsmjitLog[endPos];
                if (c >= '0' && c <= '9')
                {
                    Value = (Value << 4) | static_cast<uintptr_t>(c - '0');
                }
                else if (c >= 'a' && c <= 'f')
                {
                    Value = (Value << 4) | static_cast<uintptr_t>(c - 'a' + 10);
                }
                else if (c >= 'A' && c <= 'F')
                {
                    Value = (Value << 4) | static_cast<uintptr_t>(c - 'A' + 10);
                }
                else
                {
                    break;
                }
            }

            NumberSymbolMap::iterator itr = m_NumberSymbols.find(Value);
            if (itr != m_NumberSymbols.end())
            {
                const std::string hexStr = AsmjitLog.substr(Pos, endPos - Pos);
                const std::string symbol = itr->second.Symbol;
                AsmjitLog.replace(Pos, hexStr.length(), symbol);
                itr->second.Count -= 1;
                if (itr->second.Count == 0)
                {
                    m_NumberSymbols.erase(itr);
                }
                searchAt = Pos + symbol.length();
            }
            else
            {
                searchAt = endPos;
            }
        }
    }

    std::string::size_type Pos = AsmjitLog.find("L");
    if (m_LabelSymbols.size() > 0 && Pos != std::string::npos)
    {
        size_t len = AsmjitLog.length();
        uint32_t Value = 0;
        for (int i = 0; i < 8 && (Pos + 1 + i) < len; i++)
        {
            char c = AsmjitLog[Pos + 1 + i];
            if (c >= '0' && c <= '9')
            {
                Value = (Value * 10) + static_cast<uint32_t>(c - '0');
            }
            else
            {
                break;
            }
        }
        NumberSymbolMap::iterator itr = m_LabelSymbols.find(Value);
        if (itr != m_LabelSymbols.end())
        {
            std::string::size_type endPos = Pos + 1;
            for (std::string::size_type LenSize = AsmjitLog.length(); (endPos < LenSize && std::isdigit(static_cast<unsigned char>(AsmjitLog[endPos]))); endPos++)
            {
            }
            std::string LabelStr = AsmjitLog.substr(Pos, endPos - Pos);
            AsmjitLog.replace(Pos, LabelStr.length(), itr->second.Symbol);
            itr->second.Count -= 1;
            if (itr->second.Count == 0)
            {
                m_LabelSymbols.erase(itr);
            }
        }
    }
    m_CodeBlock.Log("      %s", AsmjitLog.c_str());
    return asmjit::kErrorOk;
}

void CX64Ops::AddLabelSymbol(const asmjit::Label & Label, const char * Symbol)
{
    if (!g_DebugSettings.recordRecompilerAsm)
    {
        return;
    }
    NumberSymbolMap::iterator itr = m_LabelSymbols.find(Label.id());
    if (itr != m_LabelSymbols.end())
    {
        if (strcmp(itr->second.Symbol.c_str(), Symbol) == 0)
        {
            itr->second.Count += 2;
        }
        else
        {
            __debugbreak();
        }
    }
    else
    {
        m_LabelSymbols.emplace(std::make_pair(Label.id(), NumberSymbol{Symbol, 2}));
    }
}

void CX64Ops::JoLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddLabelSymbol(JumpLabel, LabelName);
    jo(JumpLabel);
}

void CX64Ops::JsLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddLabelSymbol(JumpLabel, LabelName);
    js(JumpLabel);
}

void CX64Ops::JeLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddLabelSymbol(JumpLabel, LabelName);
    je(JumpLabel);
}

void CX64Ops::JneLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddLabelSymbol(JumpLabel, LabelName);
    jne(JumpLabel);
}

void CX64Ops::JmpLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    AddLabelSymbol(JumpLabel, LabelName);
    jmp(JumpLabel);
}

void CX64Ops::CmpConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        cmp(asmjit::x86::dword_ptr_abs((uintptr_t)Variable), Const);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::CmpRegToVariable(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        cmp(Reg.r32(), asmjit::x86::dword_ptr_abs((uintptr_t)Variable));
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::MoveConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        mov(asmjit::x86::dword_ptr_abs((uintptr_t)Variable), Const);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::MoveConstToX64reg(const asmjit::x86::Gp & Reg, uint64_t Const, const char * ValueName)
{
    if (g_DebugSettings.recordRecompilerAsm && ValueName != nullptr)
    {
        AddNumberSymbol((uintptr_t)Const, ValueName);
    }
    if (Const == 0)
    {
        xor_(Reg, Reg);
    }
    else if (Reg.isType(asmjit::RegType::kX86_Gpd))
    {
        mov(Reg.r32(), static_cast<uint32_t>(Const));
    }
    else
    {
        mov(Reg, Const);
    }
}

void CX64Ops::MoveVariable64ToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        mov(Reg.r64(), asmjit::x86::qword_ptr_abs((uintptr_t)Variable));
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::MovDwordToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Src)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        if (Src.isType(asmjit::RegType::kX86_Gpq))
        {
            mov(asmjit::x86::dword_ptr_abs((uintptr_t)Variable), Src.r32());
        }
        else
        {
            mov(asmjit::x86::dword_ptr_abs((uintptr_t)Variable), Src);
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::MovQwordToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Src)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        mov(asmjit::x86::qword_ptr_abs((uintptr_t)Variable), Src.r64());
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::AddDwordFromVariable(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        add(Reg.r32(), asmjit::x86::dword_ptr_abs((uintptr_t)Variable));
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::SubVariableFromX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        sub(Reg.r32(), asmjit::x86::dword_ptr_abs((uintptr_t)Variable));
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::SubConstFromVariable(uint32_t Const, void * Variable, const char * VariableName)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        sub(asmjit::x86::dword_ptr_abs((uintptr_t)Variable), Const);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::AndConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        and_(asmjit::x86::dword_ptr_abs((uintptr_t)Variable), Const);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::XorVariableToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        xor_(Reg.r32(), asmjit::x86::dword_ptr_abs((uintptr_t)Variable));
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::TestVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        test(asmjit::x86::dword_ptr_abs((uintptr_t)Variable), Const);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64Ops::MoveVariableToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName, bool SignExtend)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    if (asmjit::Support::isUInt32((uintptr_t)Variable))
    {
        if (SignExtend)
        {
            movsxd(Reg.r64(), asmjit::x86::dword_ptr_abs((uintptr_t)Variable));
        }
        else
        {
            mov(Reg.r32(), asmjit::x86::dword_ptr_abs((uintptr_t)Variable));
        }
    }
    else
    {
        mov(Reg.r64(), (uintptr_t)Variable);
        if (SignExtend)
        {
            movsxd(Reg.r64(), asmjit::x86::dword_ptr(Reg.r64()));
        }
        else
        {
            mov(Reg.r32(), asmjit::x86::dword_ptr(Reg.r64()));
        }
    }
}

void CX64Ops::MoveVariable32ToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    const asmjit::x86::Gpq addrReg(Reg.id());
    mov(addrReg, reinterpret_cast<uint64_t>(Variable));
    if (Reg.isType(asmjit::RegType::kX86_Gpq))
    {
        mov(Reg.r32(), asmjit::x86::dword_ptr(addrReg));
    }
    else
    {
        mov(Reg.r32(), asmjit::x86::dword_ptr(addrReg));
    }
}

void CX64Ops::MoveVariable32SignExtendToX64reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    AddNumberSymbol((uintptr_t)Variable, VariableName);
    const asmjit::x86::Gpq addrReg(Reg.id());
    mov(addrReg, reinterpret_cast<uint64_t>(Variable));
    movsxd(Reg.r64(), asmjit::x86::dword_ptr(addrReg));
}

void CX64Ops::AddNumberSymbol(uintptr_t Value, const char * Symbol)
{
    if (Symbol != nullptr)
    {
        AddNumberSymbol(Value, std::string(Symbol));
    }
}

void CX64Ops::AddNumberSymbol(uintptr_t Value, const std::string & Symbol)
{
    if (!g_DebugSettings.recordRecompilerAsm)
    {
        return;
    }
    NumberSymbolMap::iterator itr = m_NumberSymbols.find(Value);
    if (itr != m_NumberSymbols.end())
    {
        itr->second.Count += 1;
    }
    else
    {
        m_NumberSymbols.emplace(std::make_pair(Value, NumberSymbol{Symbol, 1}));
    }
}

void CX64Ops::EnterPrimarySection()
{
    section(m_PrimarySection);
}

void CX64Ops::EnterSecondarySection()
{
    section(m_SecondarySection);
}

void CX64Ops::BreakPointNotification(const char * FileName, int32_t LineNumber)
{
    g_Notify->BreakPoint(FileName, LineNumber);
}

void CX64Ops::X64BreakPoint(const char * FileName, int32_t LineNumber)
{
    push(asmjit::x86::rax);
    push(asmjit::x86::rcx);
    push(asmjit::x86::rdx);
    push(asmjit::x86::rbx);
    push(asmjit::x86::rbp);
    push(asmjit::x86::rsi);
    push(asmjit::x86::rdi);
    push(asmjit::x86::r8);
    push(asmjit::x86::r9);
    push(asmjit::x86::r10);
    push(asmjit::x86::r11);
    push(asmjit::x86::r12);
    push(asmjit::x86::r13);
    push(asmjit::x86::r14);
    push(asmjit::x86::r15);

    MoveConstToX64reg(asmjit::x86::rcx, reinterpret_cast<uintptr_t>(FileName), FileName);
    mov(asmjit::x86::edx, LineNumber);
    sub(asmjit::x86::rsp, 32);
    CallFunc(reinterpret_cast<uintptr_t>(&CX64Ops::BreakPointNotification), "BreakPointNotification");
    add(asmjit::x86::rsp, 32);

    pop(asmjit::x86::r15);
    pop(asmjit::x86::r14);
    pop(asmjit::x86::r13);
    pop(asmjit::x86::r12);
    pop(asmjit::x86::r11);
    pop(asmjit::x86::r10);
    pop(asmjit::x86::r9);
    pop(asmjit::x86::r8);
    pop(asmjit::x86::rdi);
    pop(asmjit::x86::rsi);
    pop(asmjit::x86::rbp);
    pop(asmjit::x86::rbx);
    pop(asmjit::x86::rdx);
    pop(asmjit::x86::rcx);
    pop(asmjit::x86::rax);
}

void CX64Ops::CallFunc(uintptr_t FunctPtr, const char * FunctName)
{
    if (g_DebugSettings.recordRecompilerAsm)
    {
        AddNumberSymbol(FunctPtr, FunctName);
    }
    mov(asmjit::x86::rax, FunctPtr);
    call(asmjit::x86::rax);
}

void CX64Ops::CallThis(void * ThisPtr, uintptr_t FunctPtr, const char * FunctName)
{
    mov(asmjit::x86::rcx, ThisPtr);
    CallFunc(FunctPtr, FunctName);
}

#endif