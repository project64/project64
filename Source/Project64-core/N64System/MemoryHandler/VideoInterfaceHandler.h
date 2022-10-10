#pragma once
#include "MemoryHandler.h"
#include <Project64-core\Logging.h>
#include <Project64-core\Settings\DebugSettings.h>
#include <Project64-core\Settings\GameSettings.h>
#include <stdint.h>

class VideoInterfaceReg
{
protected:
    VideoInterfaceReg(uint32_t * VideoInterface);

public:
    uint32_t & VI_STATUS_REG;
    uint32_t & VI_CONTROL_REG;
    uint32_t & VI_ORIGIN_REG;
    uint32_t & VI_DRAM_ADDR_REG;
    uint32_t & VI_WIDTH_REG;
    uint32_t & VI_H_WIDTH_REG;
    uint32_t & VI_INTR_REG;
    uint32_t & VI_V_INTR_REG;
    uint32_t & VI_CURRENT_REG;
    uint32_t & VI_V_CURRENT_LINE_REG;
    uint32_t & VI_BURST_REG;
    uint32_t & VI_TIMING_REG;
    uint32_t & VI_V_SYNC_REG;
    uint32_t & VI_H_SYNC_REG;
    uint32_t & VI_LEAP_REG;
    uint32_t & VI_H_SYNC_LEAP_REG;
    uint32_t & VI_H_START_REG;
    uint32_t & VI_H_VIDEO_REG;
    uint32_t & VI_V_START_REG;
    uint32_t & VI_V_VIDEO_REG;
    uint32_t & VI_V_BURST_REG;
    uint32_t & VI_X_SCALE_REG;
    uint32_t & VI_Y_SCALE_REG;

private:
    VideoInterfaceReg();
    VideoInterfaceReg(const VideoInterfaceReg &);
    VideoInterfaceReg & operator=(const VideoInterfaceReg &);
};

class CMipsMemoryVM;
class CPlugins;
class CRegisters;
class CSystemTimer;
class CN64System;

class VideoInterfaceHandler :
    public MemoryHandler,
    public VideoInterfaceReg,
    private CGameSettings,
    private CDebugSettings,
    private CLogging
{
public:
    VideoInterfaceHandler(CN64System & System, CMipsMemoryVM & MMU, CRegisters & Reg);
    ~VideoInterfaceHandler();

    void UpdateFieldSerration(uint32_t interlaced);
    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    VideoInterfaceHandler();
    VideoInterfaceHandler(const VideoInterfaceHandler &);
    VideoInterfaceHandler & operator=(const VideoInterfaceHandler &);

    static void stSystemReset(VideoInterfaceHandler * _this)
    {
        _this->SystemReset();
    }
    static void stLoadedGameState(VideoInterfaceHandler * _this)
    {
        _this->LoadedGameState();
    }

    void UpdateHalfLine();
    void LoadedGameState(void);
    void SystemReset(void);

    CN64System & m_System;
    uint32_t m_FieldSerration;
    uint32_t m_HalfLine;
    uint32_t m_HalfLineCheck;
    CMipsMemoryVM & m_MMU;
    CPlugins * m_Plugins;
    CRegisters & m_Reg;
    CSystemTimer & m_SystemTimer;
    int32_t & m_NextTimer;
    uint32_t & m_PC;
};