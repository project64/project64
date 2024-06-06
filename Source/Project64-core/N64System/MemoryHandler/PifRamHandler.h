#pragma once
#include "MemoryHandler.h"
#include <Project64-core\Logging.h>
#include <Project64-core\N64System\SaveType\Eeprom.h>
#include <Project64-core\Settings\DebugSettings.h>
#include <stdint.h>

class CN64System;
class CMipsMemoryVM;
class CRegisters;

class PifRamHandler :
    public MemoryHandler,
    private CDebugSettings,
    private CLogging
{
public:
    PifRamHandler(CN64System & System, bool SavesReadOnly);
    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

    void DMA_READ();
    void DMA_WRITE();

    uint8_t * PifRam(void)
    {
        return m_PifRam;
    }

private:
    PifRamHandler();
    PifRamHandler(const PifRamHandler &);
    PifRamHandler & operator=(const PifRamHandler &);

    void CicNus6105(const char * Challenge, char response[], int32_t length);
    void ControlRead();
    void ControlWrite(void);
    void LogControllerPakData(const char * Description);
    void ReadControllerCommand(int32_t Control, uint8_t * Command);
    void ProcessControllerCommand(int32_t Control, uint8_t * Command);
    void SystemReset(void);

    static uint32_t swap32by8(uint32_t word);

    static void stSystemReset(PifRamHandler * _this)
    {
        _this->SystemReset();
    }

    CMipsMemoryVM & m_MMU;
    uint8_t m_PifRom[0x7C0];
    uint8_t m_PifRam[0x40];
    uint64_t & m_PC;
    CEeprom m_Eeprom;
};