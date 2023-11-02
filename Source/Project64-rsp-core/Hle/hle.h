// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2014 Bobby Smiles
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#pragma once
#include "ucodes.h"
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <stdint.h>

// Macro for unused variable warning suppression
#ifdef __GNUC__
#define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#define UNUSED(x) /* x */
#endif

class CHle
{
public:
    CHle(const RSP_INFO & Rsp_Info);
    ~CHle();

    uint8_t * dram()
    {
        return m_dram;
    }
    uint8_t * dmem()
    {
        return m_dmem;
    }
    uint8_t * imem()
    {
        return m_imem;
    }

    bool AudioHle()
    {
        return m_AudioHle;
    }
    bool GraphicsHle()
    {
        return m_GraphicsHle;
    }
    struct alist_audio_t & alist_audio()
    {
        return m_alist_audio;
    }
    struct alist_naudio_t & alist_naudio()
    {
        return m_alist_naudio;
    }
    struct alist_nead_t & alist_nead()
    {
        return m_alist_nead;
    }
    uint8_t * mp3_buffer()
    {
        return &m_mp3_buffer[0];
    }

    uint8_t * alist_buffer()
    {
        return &m_alist_buffer[0];
    }

    void VerboseMessage(const char * message, ...);
    void WarnMessage(const char * message, ...);
    void ErrorMessage(const char * message, ...);

    void rsp_break(uint32_t setbits);
    void hle_execute(void);

    bool try_fast_audio_dispatching(void);

private:
    CHle(void);
    CHle(const CHle &);
    CHle & operator=(const CHle &);

    bool is_task(void);
    bool try_fast_task_dispatching(void);
    void normal_task_dispatching(void);
    void non_task_dispatching(void);

    uint8_t * m_dram;
    uint8_t * m_dmem;
    uint8_t * m_imem;

    uint32_t * m_mi_intr;

    uint32_t * m_sp_mem_addr;
    uint32_t * m_sp_dram_addr;
    uint32_t * m_sp_rd_length;
    uint32_t * m_sp_wr_length;
    uint32_t * m_sp_status;
    uint32_t * m_sp_dma_full;
    uint32_t * m_sp_dma_busy;
    uint32_t * m_sp_pc;
    uint32_t * m_sp_semaphore;

    uint32_t * m_dpc_start;
    uint32_t * m_dpc_end;
    uint32_t * m_dpc_current;
    uint32_t * m_dpc_status;
    uint32_t * m_dpc_clock;
    uint32_t * m_dpc_bufbusy;
    uint32_t * m_dpc_pipebusy;
    uint32_t * m_dpc_tmem;

    void (*m_CheckInterrupts)(void);
    void (*m_ProcessDList)(void);
    void (*m_ProcessAList)(void);
    void (*m_ProcessRdpList)(void);
    void (*m_ShowCFB)(void);

    // alist.cpp
    uint8_t m_alist_buffer[0x1000];

    // alist_audio.cpp
    struct alist_audio_t m_alist_audio;

    // alist_naudio.cpp
    struct alist_naudio_t m_alist_naudio;

    // alist_nead.cpp
    struct alist_nead_t m_alist_nead;

    // mp3.cpp
    uint8_t m_mp3_buffer[0x1000];

    bool m_AudioHle;
    bool m_GraphicsHle;
    bool m_ForwardAudio;
    bool m_ForwardGFX;
};
