#pragma once
#include <Project64-rsp-core/Hle/hle.h>
#include <Project64-rsp-core/cpu/RspTypes.h>
#include <stdint.h>

class CRSPSystem;
class RSPRegisterHandlerPlugin;
class CHle;

class CHleTask
{
    struct TASK_INFO
    {
        uint32_t Type;
        uint32_t Flags;
        uint32_t Boot;
        uint32_t BootSize;
        uint32_t Ucode;
        uint32_t UcodeSize;
        uint32_t UcodeData;
        uint32_t UcodeDataSize;
        uint32_t DramStack;
        uint32_t DramStackSize;
        uint32_t OutputBuff;
        uint32_t OutputBuffSize;
        uint32_t DataPtr;
        uint32_t DataSize;
        uint32_t YieldDataPtr;
        uint32_t YieldDataSize;
    };

    enum class HLETaskType
    {
        Video = 1,
        Audio = 2,
    };

public:
    CHleTask(CRSPSystem & System);

    bool IsHleTask(void);
    bool ProcessHleTask(void);

private:
    CHleTask(void);
    CHleTask(const CHleTask & copy);
    CHleTask & operator=(const CHleTask & rhs);

    void (*&CheckInterrupts)(void);
    void (*&ProcessDList)(void);

    CHle m_hle;
    CRSPSystem & m_System;
    uint32_t *& m_SP_STATUS_REG;
    uint32_t *& m_SP_PC_REG;
    uint32_t *& m_DPC_STATUS_REG;
    uint8_t *& m_DMEM;
    uint8_t *& m_IMEM;
};
