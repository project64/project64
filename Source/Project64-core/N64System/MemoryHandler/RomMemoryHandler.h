#pragma once
#include "MemoryHandler.h"
#include <Project64-core\Settings\DebugSettings.h>
#include <Project64-core\Logging.h>
#include <stdint.h>

class CRegisters;
class CN64Rom;
class CN64System;

class RomMemoryHandler :
    public MemoryHandler,
    private CDebugSettings,
    private CLogging
{
public:
    RomMemoryHandler(CN64System & System, CRegisters & Reg, CN64Rom & Rom);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

    void RomWriteDecayed(void);

private:
    RomMemoryHandler();
    RomMemoryHandler(const RomMemoryHandler&);
    RomMemoryHandler& operator=(const RomMemoryHandler&);

    void SystemReset(void);
    void LoadedGameState(void);

    static void stSystemReset(RomMemoryHandler * _this) { _this->SystemReset(); }
    static void stLoadedGameState(RomMemoryHandler * _this) { _this->LoadedGameState(); }

    uint32_t & m_PC;
    CRegisters & m_Reg;
    CN64Rom & m_Rom;
    bool m_RomWrittenTo;
    uint32_t m_RomWroteValue;
};
