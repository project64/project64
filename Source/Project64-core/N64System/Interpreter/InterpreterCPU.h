#pragma once

#include <Project64-core/N64System/Interpreter/InterpreterOps.h>

class CInterpreterCPU :
    private R4300iOp
{
public:
    static void BuildCPU();
    static void ExecuteCPU();
    static void ExecuteOps(int32_t Cycles);
    static void InPermLoop();
	
private:
    CInterpreterCPU();                                  // Disable default constructor
    CInterpreterCPU(const CInterpreterCPU&);            // Disable copy constructor
    CInterpreterCPU& operator=(const CInterpreterCPU&); // Disable assignment

    static R4300iOp::Func * m_R4300i_Opcode;
};
