// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2014 Bobby Smiles
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#pragma once
#include <stdint.h>
#include "Rsp.h"
#include "ucodes.h"

// Signal processor interface flags

enum
{
    SP_CLR_HALT = 0x00001,	            /* Bit  0: Clear halt */
    SP_SET_HALT = 0x00002,	            /* Bit  1: Set halt */
    SP_CLR_BROKE = 0x00004,	            /* Bit  2: Clear broke */
    SP_CLR_INTR = 0x00008,	            /* Bit  3: Clear INTR */
    SP_SET_INTR = 0x00010,	            /* Bit  4: Set INTR */
    SP_CLR_SSTEP = 0x00020,	            /* Bit  5: Clear SSTEP */
    SP_SET_SSTEP = 0x00040,	            /* Bit  6: Set SSTEP */
    SP_CLR_INTR_BREAK = 0x00080,	    /* Bit  7: Clear INTR on break */
    SP_SET_INTR_BREAK = 0x00100,	    /* Bit  8: Set INTR on break */
    SP_CLR_SIG0 = 0x00200,	            /* Bit  9: Clear signal 0 */
    SP_SET_SIG0 = 0x00400,	            /* Bit 10: Set signal 0 */
    SP_CLR_SIG1 = 0x00800,	            /* Bit 11: Clear signal 1 */
    SP_SET_SIG1 = 0x01000,	            /* Bit 12: Set signal 1 */
    SP_CLR_SIG2 = 0x02000,	            /* Bit 13: Clear signal 2 */
    SP_SET_SIG2 = 0x04000,	            /* Bit 14: Set signal 2 */
    SP_CLR_SIG3 = 0x08000,	            /* Bit 15: Clear signal 3 */
    SP_SET_SIG3 = 0x10000,	            /* Bit 16: Set signal 3 */
    SP_CLR_SIG4 = 0x20000,	            /* Bit 17: Clear signal 4 */
    SP_SET_SIG4 = 0x40000,	            /* Bit 18: Set signal 4 */
    SP_CLR_SIG5 = 0x80000,	            /* Bit 19: Clear signal 5 */
    SP_SET_SIG5 = 0x100000,	            /* Bit 20: Set signal 5 */
    SP_CLR_SIG6 = 0x200000,	            /* Bit 21: Clear signal 6 */
    SP_SET_SIG6 = 0x400000,	            /* Bit 22: Set signal 6 */
    SP_CLR_SIG7 = 0x800000,	            /* Bit 23: Clear signal 7 */
    SP_SET_SIG7 = 0x1000000,            /* Bit 24: Set signal 7 */

    SP_STATUS_HALT = 0x001,		        /* Bit  0: Halt */
    SP_STATUS_BROKE = 0x002,		    /* Bit  1: Broke */
    SP_STATUS_DMA_BUSY = 0x004,		    /* Bit  2: DMA busy */
    SP_STATUS_DMA_FULL = 0x008,		    /* Bit  3: DMA full */
    SP_STATUS_IO_FULL = 0x010,		    /* Bit  4: IO full */
    SP_STATUS_SSTEP = 0x020,		    /* Bit  5: Single step */
    SP_STATUS_INTR_BREAK = 0x040,		/* Bit  6: Interrupt on break */
    SP_STATUS_SIG0 = 0x080,		        /* Bit  7: Signal 0 set */
    SP_STATUS_SIG1 = 0x100,		        /* Bit  8: Signal 1 set */
    SP_STATUS_SIG2 = 0x200,		        /* Bit  9: Signal 2 set */
    SP_STATUS_SIG3 = 0x400,		        /* Bit 10: Signal 3 set */
    SP_STATUS_SIG4 = 0x800,		        /* Bit 11: Signal 4 set */
    SP_STATUS_SIG5 = 0x1000,		    /* Bit 12: Signal 5 set */
    SP_STATUS_SIG6 = 0x2000,		    /* Bit 13: Signal 6 set */
    SP_STATUS_SIG7 = 0x4000,		    /* Bit 14: Signal 7 set */
};

// MIPS interface flags

enum
{
    MI_INTR_SP = 0x01,		/* Bit 0: SP INTR */
    MI_INTR_SI = 0x02,		/* Bit 1: SI INTR */
    MI_INTR_AI = 0x04,		/* Bit 2: AI INTR */
    MI_INTR_VI = 0x08,		/* Bit 3: VI INTR */
    MI_INTR_PI = 0x10,		/* Bit 4: PI INTR */
    MI_INTR_DP = 0x20,		/* Bit 5: DP INTR */
};

class CHle
{
public:
    CHle(const RSP_INFO & Rsp_Info);
    ~CHle();

    uint8_t * dram() { return m_dram; }
    uint8_t * dmem() { return m_dmem; }
    uint8_t * imem() { return m_imem; }

    bool AudioHle() { return m_AudioHle; }
    bool GraphicsHle() { return m_GraphicsHle; }
    struct alist_audio_t & alist_audio() { return m_alist_audio; }
    struct alist_naudio_t & alist_naudio() { return m_alist_naudio; }
    struct alist_nead_t & alist_nead() { return m_alist_nead; }
    uint8_t * mp3_buffer() { return &m_mp3_buffer[0]; }

    uint8_t * alist_buffer() { return &m_alist_buffer[0]; }

    void VerboseMessage(const char *message, ...);
    void WarnMessage(const char *message, ...);
    void ErrorMessage(const char *message, ...);

    void rsp_break(uint32_t setbits);
    void hle_execute(void);

private:
    CHle(void);
    CHle(const CHle&);
    CHle& operator=(const CHle&);

    bool is_task(void);
    bool try_fast_audio_dispatching(void);
    bool try_fast_task_dispatching(void);
    void normal_task_dispatching(void);
    void non_task_dispatching(void);

    uint8_t * m_dram;
    uint8_t * m_dmem;
    uint8_t * m_imem;

    uint32_t* m_mi_intr;

    uint32_t* m_sp_mem_addr;
    uint32_t* m_sp_dram_addr;
    uint32_t* m_sp_rd_length;
    uint32_t* m_sp_wr_length;
    uint32_t* m_sp_status;
    uint32_t* m_sp_dma_full;
    uint32_t* m_sp_dma_busy;
    uint32_t* m_sp_pc;
    uint32_t* m_sp_semaphore;

    uint32_t* m_dpc_start;
    uint32_t* m_dpc_end;
    uint32_t* m_dpc_current;
    uint32_t* m_dpc_status;
    uint32_t* m_dpc_clock;
    uint32_t* m_dpc_bufbusy;
    uint32_t* m_dpc_pipebusy;
    uint32_t* m_dpc_tmem;

    void(*m_CheckInterrupts)(void);
    void(*m_ProcessDList)(void);
    void(*m_ProcessAList)(void);
    void(*m_ProcessRdpList)(void);
    void(*m_ShowCFB)(void);

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
