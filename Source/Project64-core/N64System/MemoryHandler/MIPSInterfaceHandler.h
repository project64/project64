#pragma once
#include "MemoryHandler.h"
#include <Project64-core\Settings\DebugSettings.h>
#include <Project64-core\Logging.h>
#include <stdint.h>

class MIPSInterfaceReg
{
protected:
    MIPSInterfaceReg(uint32_t * MipsInterface);

public:
    uint32_t & MI_INIT_MODE_REG;
    uint32_t & MI_MODE_REG;
    uint32_t & MI_VERSION_REG;
    uint32_t & MI_NOOP_REG;
    uint32_t & MI_INTR_REG;
    uint32_t & MI_INTR_MASK_REG;

private:
    MIPSInterfaceReg();
    MIPSInterfaceReg(const MIPSInterfaceReg&);
    MIPSInterfaceReg& operator=(const MIPSInterfaceReg&);
};

class CRegisters;

class MIPSInterfaceHandler :
    public MemoryHandler,
    private MIPSInterfaceReg,
    private CDebugSettings,
    private CLogging
{
public:
    MIPSInterfaceHandler(CRegisters & Reg);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    MIPSInterfaceHandler();
    MIPSInterfaceHandler(const MIPSInterfaceHandler &);
    MIPSInterfaceHandler & operator=(const MIPSInterfaceHandler &);

    CRegisters & m_Reg;
    uint32_t & m_PC;
};
