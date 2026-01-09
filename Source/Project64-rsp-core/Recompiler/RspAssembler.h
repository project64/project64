#pragma once
#if defined(__amd64__) || defined(_M_X64)

#include <Common/StdString.h>
#include <Project64-rsp-core/Recompiler/asmjit.h>
#include <map>

class RspAssembler :
    public asmjit::x86::Assembler,
    public asmjit::ErrorHandler,
    public asmjit::Logger
{
public:
    RspAssembler(asmjit::CodeHolder * CodeHolder, std::string & CodeLog);

    void Reset(void);

    void CallFunc(void * FunctPtr, const char * FunctName);
    void CallThis(void * ThisPtr, void * FunctPtr, const char * FunctName);
    void CompConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void CompX86regToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void JFunc(void * FunctPtr, const char * FunctName);
    void JeLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JgLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JleLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JmpLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void JneLabel(const char * LabelName, asmjit::Label & JumpLabel);
    void MoveConstToVariable(void * Variable, const char * VariableName, uint32_t Const);
    void MoveVariableToX86reg(const asmjit::x86::Gp & Reg, void * Variable, const char * VariableName);
    void MoveX86regToVariable(void * Variable, const char * VariableName, const asmjit::x86::Gp & Reg);
    void SetgVariable(void * Variable, const char * VariableName);
    void SetnzVariable(void * Variable, const char * VariableName);
    void SetzVariable(void * Variable, const char * VariableName);

    void AddLabelSymbol(const asmjit::Label & Label, const char * Symbol);

private:
    RspAssembler();
    RspAssembler(const RspAssembler &);
    RspAssembler & operator=(const RspAssembler &);

    void AddNumberSymbol(uint64_t Value, const char * Symbol);

    typedef struct
    {
        std::string Symbol;
        uint32_t Count;
    } NumberSymbol;

    typedef std::map<uint64_t, std::string> LabelSymbolMap;
    typedef std::map<uint64_t, NumberSymbol> NumberSymbolMap;

    void handleError(asmjit::Error err, const char * message, asmjit::BaseEmitter * origin);
    asmjit::Error _log(const char * data, size_t size) noexcept;
    std::string & m_CodeLog;
    LabelSymbolMap m_LabelSymbols;
    NumberSymbolMap m_NumberSymbols;
};

#endif