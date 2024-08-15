#pragma once
#include <stdint.h>

void StartCPULog(void);
void StopCPULog(void);
void CPU_Message(const char * Message, ...);

class CRSPSystem;
class CLog;

class CRDPLog
{
public:
    CRDPLog(CRSPSystem & System);

    void StartLog(void);
    void StopLog(void);
    void Message(const char * Message, ...);
    void LogDlist(void);
    void LogMT0(uint32_t PC, int Reg, uint32_t Value);
    void LogMF0(uint32_t PC, int Reg);

private:
    CRSPSystem & m_System;
    CLog * m_Log;
    uint32_t *& m_DPC_START_REG;
    uint32_t *& m_DPC_END_REG;
    uint32_t *& m_DPC_CURRENT_REG;
    uint32_t *& m_DPC_STATUS_REG;
    uint32_t *& m_DPC_CLOCK_REG;
    uint8_t *& m_RDRAM;
    uint8_t *& m_DMEM;
};

extern CRDPLog RDPLog;
