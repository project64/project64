// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2014 Bobby Smiles
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#pragma once
#include <Common/stdtypes.h>
#include "Rsp.h"
#include "ucodes.h"

//Signal Processor interface flags
enum
{
    SP_CLR_HALT = 0x00001,	    /* Bit  0: clear halt */
    SP_SET_HALT = 0x00002,	    /* Bit  1: set halt */
    SP_CLR_BROKE = 0x00004,	    /* Bit  2: clear broke */
    SP_CLR_INTR = 0x00008,	    /* Bit  3: clear intr */
    SP_SET_INTR = 0x00010,	    /* Bit  4: set intr */
    SP_CLR_SSTEP = 0x00020,	    /* Bit  5: clear sstep */
    SP_SET_SSTEP = 0x00040,	    /* Bit  6: set sstep */
    SP_CLR_INTR_BREAK = 0x00080,	    /* Bit  7: clear intr on break */
    SP_SET_INTR_BREAK = 0x00100,	    /* Bit  8: set intr on break */
    SP_CLR_SIG0 = 0x00200,	    /* Bit  9: clear signal 0 */
    SP_SET_SIG0 = 0x00400,	    /* Bit 10: set signal 0 */
    SP_CLR_SIG1 = 0x00800,	    /* Bit 11: clear signal 1 */
    SP_SET_SIG1 = 0x01000,	    /* Bit 12: set signal 1 */
    SP_CLR_SIG2 = 0x02000,	    /* Bit 13: clear signal 2 */
    SP_SET_SIG2 = 0x04000,	    /* Bit 14: set signal 2 */
    SP_CLR_SIG3 = 0x08000,	    /* Bit 15: clear signal 3 */
    SP_SET_SIG3 = 0x10000,	    /* Bit 16: set signal 3 */
    SP_CLR_SIG4 = 0x20000,	    /* Bit 17: clear signal 4 */
    SP_SET_SIG4 = 0x40000,	    /* Bit 18: set signal 4 */
    SP_CLR_SIG5 = 0x80000,	    /* Bit 19: clear signal 5 */
    SP_SET_SIG5 = 0x100000,	/* Bit 20: set signal 5 */
    SP_CLR_SIG6 = 0x200000,	/* Bit 21: clear signal 6 */
    SP_SET_SIG6 = 0x400000,	/* Bit 22: set signal 6 */
    SP_CLR_SIG7 = 0x800000,	/* Bit 23: clear signal 7 */
    SP_SET_SIG7 = 0x1000000,   /* Bit 24: set signal 7 */

    SP_STATUS_HALT = 0x001,		/* Bit  0: halt */
    SP_STATUS_BROKE = 0x002,		/* Bit  1: broke */
    SP_STATUS_DMA_BUSY = 0x004,		/* Bit  2: dma busy */
    SP_STATUS_DMA_FULL = 0x008,		/* Bit  3: dma full */
    SP_STATUS_IO_FULL = 0x010,		/* Bit  4: io full */
    SP_STATUS_SSTEP = 0x020,		/* Bit  5: single step */
    SP_STATUS_INTR_BREAK = 0x040,		/* Bit  6: interrupt on break */
    SP_STATUS_SIG0 = 0x080,		/* Bit  7: signal 0 set */
    SP_STATUS_SIG1 = 0x100,		/* Bit  8: signal 1 set */
    SP_STATUS_SIG2 = 0x200,		/* Bit  9: signal 2 set */
    SP_STATUS_SIG3 = 0x400,		/* Bit 10: signal 3 set */
    SP_STATUS_SIG4 = 0x800,		/* Bit 11: signal 4 set */
    SP_STATUS_SIG5 = 0x1000,		/* Bit 12: signal 5 set */
    SP_STATUS_SIG6 = 0x2000,		/* Bit 13: signal 6 set */
    SP_STATUS_SIG7 = 0x4000,		/* Bit 14: signal 7 set */
};

//Mips interface flags
enum
{
    MI_INTR_SP = 0x01,		/* Bit 0: SP intr */
    MI_INTR_SI = 0x02,		/* Bit 1: SI intr */
    MI_INTR_AI = 0x04,		/* Bit 2: AI intr */
    MI_INTR_VI = 0x08,		/* Bit 3: VI intr */
    MI_INTR_PI = 0x10,		/* Bit 4: PI intr */
    MI_INTR_DP = 0x20,		/* Bit 5: DP intr */
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
    CHle(void);                     // Disable default constructor
    CHle(const CHle&);              // Disable copy constructor
    CHle& operator=(const CHle&);   // Disable assignment

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

    /* alist.cpp */
    uint8_t m_alist_buffer[0x1000];

    /* alist_audio.cpp */
    struct alist_audio_t m_alist_audio;

    /* alist_naudio.cpp */
    struct alist_naudio_t m_alist_naudio;

    /* alist_nead.cpp */
    struct alist_nead_t m_alist_nead;

    /* mp3.cpp */
    uint8_t m_mp3_buffer[0x1000];

    bool m_AudioHle;
    bool m_GraphicsHle;
    bool m_ForwardAudio;
    bool m_ForwardGFX;
};
