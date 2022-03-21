#pragma once
#include "MemoryHandler.h"
#include <Project64-core\Settings\DebugSettings.h>
#include <Project64-core\Settings\GameSettings.h>
#include <Project64-core\Logging.h>
#include <stdint.h>

enum
{
    AI_STATUS_FIFO_FULL = 0x80000000,    // Bit 31: Full
    AI_STATUS_DMA_BUSY = 0x40000000,    // Bit 30: Busy
};

class AudioInterfaceReg
{
protected:
    AudioInterfaceReg(uint32_t * _AudioInterface);

public:
    uint32_t & AI_DRAM_ADDR_REG;
    uint32_t & AI_LEN_REG;
    uint32_t & AI_CONTROL_REG;
    uint32_t & AI_STATUS_REG;
    uint32_t & AI_DACRATE_REG;
    uint32_t & AI_BITRATE_REG;

private:
    AudioInterfaceReg();
    AudioInterfaceReg(const AudioInterfaceReg&);
    AudioInterfaceReg& operator=(const AudioInterfaceReg&);
};

class CRegisters;
class CN64System;
class CPlugins;

class AudioInterfaceHandler :
    public MemoryHandler,
    public AudioInterfaceReg,
    private CGameSettings,
    private CDebugSettings,
    private CLogging
{
public:
    AudioInterfaceHandler(CN64System & System, CRegisters & Reg);
    ~AudioInterfaceHandler();

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

    void TimerInterrupt(void);
    void TimerBusy(void);
    void SetViIntr(uint32_t VI_INTR_TIME);
    void SetFrequency(uint32_t Dacrate, uint32_t System);
    uint32_t GetLength();
    uint32_t GetStatus();

private:
    AudioInterfaceHandler();
    AudioInterfaceHandler(const AudioInterfaceHandler &);
    AudioInterfaceHandler & operator=(const AudioInterfaceHandler &);

    static void stSystemReset(AudioInterfaceHandler * _this) { _this->SystemReset(); }
    static void stLoadedGameState(AudioInterfaceHandler * _this) { _this->LoadedGameState(); }

    void LoadedGameState(void);
    void SystemReset(void);
    void LenChanged();

    CN64System & m_System;
    CRegisters & m_Reg;
    CPlugins * m_Plugins;
    uint32_t & m_PC;
    uint32_t m_Status;
    uint32_t m_SecondBuff;
    uint32_t m_BytesPerSecond;
    int32_t m_CountsPerByte;
    int32_t m_FramesPerSecond;
};
