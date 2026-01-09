#if defined(__amd64__) || defined(_M_X64)

#include "RspAssembler.h"
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Settings/Settings.h>

RspAssembler::RspAssembler(asmjit::CodeHolder * CodeHolder, std::string & CodeLog) :
    asmjit::x86::Assembler(CodeHolder),
    m_CodeLog(CodeLog)
{
    setErrorHandler(this);
    addFlags(asmjit::FormatFlags::kHexOffsets);
    addFlags(asmjit::FormatFlags::kHexImms);
    addFlags(asmjit::FormatFlags::kExplainImms);
    setIndentation(asmjit::FormatIndentationGroup::kCode, 2);
    setIndentation(asmjit::FormatIndentationGroup::kComment, 2);
}

void RspAssembler::handleError(asmjit::Error /*err*/, const char * /*message*/, asmjit::BaseEmitter * /*origin*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

asmjit::Error RspAssembler::_log(const char * data, size_t size) noexcept
{
    stdstr AsmjitLog(size == (size_t)-1 ? std::string(data) : std::string(data, size));
    AsmjitLog.Trim("\n");
    if (AsmjitLog.empty())
    {
        return asmjit::kErrorOk;
    }

    std::string::size_type Pos = AsmjitLog.find("0x");
    if (m_NumberSymbols.size() > 0 && Pos != std::string::npos)
    {
        uint32_t Value = 0;
        for (int i = 0; i < 8; i++)
        {
            char c = AsmjitLog[Pos + 2 + i];
            if (c >= '0' && c <= '9')
            {
                Value = (Value << 4) | (c - '0');
            }
            else if (c >= 'a' && c <= 'f')
            {
                Value = (Value << 4) | (c - 'a' + 10);
            }
            else if (c >= 'A' && c <= 'F')
            {
                Value = (Value << 4) | (c - 'A' + 10);
            }
            else
            {
                break;
            }
        }
        NumberSymbolMap::iterator itr = m_NumberSymbols.find(Value);
        if (itr != m_NumberSymbols.end())
        {
            std::string::size_type endPos = Pos + 2;
            for (std::string::size_type LenSize = AsmjitLog.length(); (endPos < LenSize && isxdigit((unsigned char)AsmjitLog[endPos])); endPos++)
            {
            }
            std::string hexStr = AsmjitLog.substr(Pos, endPos - Pos);
            AsmjitLog.replace(Pos, hexStr.length(), itr->second.Symbol);
            itr->second.Count -= 1;
            if (itr->second.Count == 0)
            {
                m_NumberSymbols.erase(itr);
            }
        }
    }
    Pos = AsmjitLog.find("L");
    if (m_LabelSymbols.size() > 0 && Pos != std::string::npos)
    {
        size_t len = AsmjitLog.length();
        uint32_t Value = 0, valueLen = 0;
        for (int i = 0; i < 8 && (Pos + 1 + i) < len; i++)
        {
            char c = AsmjitLog[Pos + 1 + i];
            if (c >= '0' && c <= '9')
            {
                Value = (Value * 10) + (c - '0');
                valueLen += 1;
            }
            else
            {
                break;
            }
        }
        if (valueLen > 0)
        {
            LabelSymbolMap::iterator itr = m_LabelSymbols.find(Value);
            if (itr != m_LabelSymbols.end())
            {
                std::string::size_type endPos = Pos + 1;
                for (std::string::size_type LenSize = AsmjitLog.length(); (endPos < LenSize && isdigit((unsigned char)AsmjitLog[endPos])); endPos++)
                {
                }
                std::string LabelStr = AsmjitLog.substr(Pos, endPos - Pos);
                AsmjitLog.replace(Pos, LabelStr.length(), itr->second);
            }
        }
    }
    m_CodeLog.append(stdstr_f("  %s\n", AsmjitLog.c_str()));
    return asmjit::kErrorOk;
}

void RspAssembler::Reset(void)
{
    setLogger(LogAsmCode ? this : nullptr);
}

void RspAssembler::CallFunc(void * FunctPtr, const char * FunctName)
{
    if (LogAsmCode)
    {
        AddNumberSymbol((uint64_t)FunctPtr, FunctName);
    }
    mov(asmjit::x86::r11, (uint64_t)FunctPtr);
    call(asmjit::x86::r11);
}

#ifdef _MSC_VER
void RspAssembler::CallThis(void * ThisPtr, void * FunctPtr, const char * FunctName)
{
    mov(asmjit::x86::rcx, ThisPtr);
    CallFunc(FunctPtr, FunctName);
}
#else
void CX86Ops::CallThis(uint64_t ThisPtr, uint64_t FunctPtr, const char * FunctName)
{
    mov(asmjit::x86::rdi, ThisPtr);
    CallFunc(FunctPtr, FunctName);
}
#endif

void RspAssembler::CompConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    if (LogAsmCode)
    {
        AddNumberSymbol((uint64_t)Variable, VariableName);
    }
    cmp(asmjit::x86::dword_ptr((uint64_t)Variable), Const);
}

void RspAssembler::CompX86regToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg)
{
    if (LogAsmCode)
    {
        AddNumberSymbol((uint64_t)Variable, VariableName);
    }
    cmp(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
}

void RspAssembler::JFunc(void * FunctPtr, const char * FunctName)
{
    if (LogAsmCode)
    {
        AddNumberSymbol((uint64_t)FunctPtr, FunctName);
    }
    mov(asmjit::x86::r11, (uint64_t)FunctPtr);
    jmp(asmjit::x86::r11);
}

void RspAssembler::JeLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    if (LogAsmCode)
    {
        AddLabelSymbol(JumpLabel, LabelName);
    }
    je(JumpLabel);
}

void RspAssembler::JgLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    if (LogAsmCode)
    {
        AddLabelSymbol(JumpLabel, LabelName);
    }
    jg(JumpLabel);
}

void RspAssembler::JleLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    if (LogAsmCode)
    {
        AddLabelSymbol(JumpLabel, LabelName);
    }
    jle(JumpLabel);
}

void RspAssembler::JmpLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    if (LogAsmCode)
    {
        AddLabelSymbol(JumpLabel, LabelName);
    }
    jmp(JumpLabel);
}

void RspAssembler::JneLabel(const char * LabelName, asmjit::Label & JumpLabel)
{
    if (LogAsmCode)
    {
        AddLabelSymbol(JumpLabel, LabelName);
    }
    jne(JumpLabel);
}

void RspAssembler::MoveConstToVariable(void * Variable, const char * VariableName, uint32_t Const)
{
    if (LogAsmCode)
    {
        AddNumberSymbol((uint64_t)Variable, VariableName);
    }
    uint64_t addr = (uint64_t)Variable;
    if (addr <= 0x7FFFFFFF)
    {
        mov(asmjit::x86::dword_ptr(addr), Const);
    }
    else
    {
        mov(asmjit::x86::r11, addr);
        mov(asmjit::x86::dword_ptr(asmjit::x86::r11), Const);
    }
}

void RspAssembler::MoveVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName)
{
    if (LogAsmCode)
    {
        AddNumberSymbol((uint64_t)Variable, VariableName);
    }
    mov(Reg, asmjit::x86::dword_ptr((uint64_t)Variable));
}

void RspAssembler::MoveX86regToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg)
{
    if (LogAsmCode)
    {
        AddNumberSymbol((uint64_t)Variable, VariableName);
    }
    mov(asmjit::x86::dword_ptr((uint64_t)(Variable)), Reg);
}

void RspAssembler::SetgVariable(void * Variable, const char * VariableName)
{
    if (LogAsmCode)
    {
        AddNumberSymbol((uint64_t)Variable, VariableName);
    }
    setg(asmjit::x86::byte_ptr((uint64_t)Variable));
}

void RspAssembler::SetnzVariable(void * Variable, const char * VariableName)
{
    if (LogAsmCode)
    {
        AddNumberSymbol((uint64_t)Variable, VariableName);
    }
    setnz(asmjit::x86::byte_ptr((uint64_t)Variable));
}

void RspAssembler::SetzVariable(void * Variable, const char * VariableName)
{
    if (LogAsmCode)
    {
        AddNumberSymbol((uint64_t)Variable, VariableName);
    }
    setz(asmjit::x86::byte_ptr((uint64_t)Variable));
}

void RspAssembler::AddLabelSymbol(const asmjit::Label & Label, const char * Symbol)
{
    LabelSymbolMap::iterator itr = m_LabelSymbols.find(Label.id());
    if (itr == m_LabelSymbols.end())
    {
        m_LabelSymbols.emplace(std::make_pair(Label.id(), Symbol));
    }
}

void RspAssembler::AddNumberSymbol(uint64_t Value, const char * Symbol)
{
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
#endif
