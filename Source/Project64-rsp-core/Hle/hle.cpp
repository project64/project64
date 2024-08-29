// Project64 - A Nintendo 64 emulator
// https://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2012 Bobby Smiles
// Copyright(C) 2009 Richard Goedeken
// Copyright(C) 2002 Hacktarux
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#include "hle.h"
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <stdint.h>
#if defined(_WIN32) && defined(_DEBUG)
#include <Windows.h>
#endif
#include "mem.h"
#include "ucodes.h"
#include <memory.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))

// Helper functions prototypes

static unsigned int sum_bytes(const uint8_t * bytes, uint32_t size);

CHle::CHle(CRSPSystem & System) :
    m_dram(System.m_RDRAM),
    m_dmem(System.m_DMEM),
    m_imem(System.m_IMEM),
    m_mi_intr(System.m_MI_INTR_REG),
    m_sp_mem_addr(System.m_SP_MEM_ADDR_REG),
    m_sp_dram_addr(System.m_SP_DRAM_ADDR_REG),
    m_sp_rd_length(System.m_SP_RD_LEN_REG),
    m_sp_wr_length(System.m_SP_WR_LEN_REG),
    m_sp_status(System.m_SP_STATUS_REG),
    m_sp_dma_full(System.m_SP_DMA_FULL_REG),
    m_sp_dma_busy(System.m_SP_DMA_BUSY_REG),
    m_sp_pc(System.m_SP_PC_REG),
    m_sp_semaphore(System.m_SP_SEMAPHORE_REG),
    m_dpc_start(System.m_DPC_START_REG),
    m_dpc_end(System.m_DPC_END_REG),
    m_dpc_current(System.m_DPC_CURRENT_REG),
    m_dpc_status(System.m_DPC_STATUS_REG),
    m_dpc_clock(System.m_DPC_CLOCK_REG),
    m_dpc_bufbusy(System.m_DPC_BUFBUSY_REG),
    m_dpc_pipebusy(System.m_DPC_PIPEBUSY_REG),
    m_dpc_tmem(System.m_DPC_TMEM_REG),
    m_CheckInterrupts(System.CheckInterrupts),
    m_ProcessDList(System.ProcessDList),
    m_ProcessRdpList(System.ProcessRdpList),
    m_AudioHle(false),
    m_GraphicsHle(true),
    m_ForwardAudio(false),
    m_ForwardGFX(true)
{
    //m_AudioHle = ReadCfgInt("Settings", "AudioHle", false);
    //m_GraphicsHle = ReadCfgInt("Settings", "GraphicsHle", true);
    memset(&m_alist_buffer, 0, sizeof(m_alist_buffer));
    memset(&m_alist_audio, 0, sizeof(m_alist_audio));
    memset(&m_alist_naudio, 0, sizeof(m_alist_naudio));
    memset(&m_alist_nead, 0, sizeof(m_alist_nead));
    memset(&m_mp3_buffer, 0, sizeof(m_mp3_buffer));
}

CHle::~CHle()
{
}

void CHle::rsp_break(unsigned int setbits)
{
    *m_sp_status |= setbits | SP_STATUS_BROKE | SP_STATUS_HALT;

    if ((*m_sp_status & SP_STATUS_INTR_BREAK))
    {
        *m_mi_intr |= MI_INTR_SP;
        m_CheckInterrupts();
    }
}

// Local functions

static unsigned int sum_bytes(const uint8_t * bytes, unsigned int size)
{
    unsigned int sum = 0;
    const unsigned char * const bytes_end = bytes + size;

    while (bytes != bytes_end)
    {
        sum += *bytes++;
    }
    return sum;
}

/*
TODO:

Try to figure if the RSP was launched using osSpTask* functions
and not run directly (in which case DMEM[0xfc0-0xfff] is meaningless).

Previously, the ucode_size field was used to determine this,
but it is not robust enough (hi Pokémon Stadium!) because games could write anything
in this field: most ucode_boot discard the value and just use 0xf7f anyway.

Using ucode_boot_size should be more robust in this regard.
*/

bool CHle::is_task(void)
{
    return (*dmem_u32(this, TASK_UCODE_BOOT_SIZE) <= 0x1000);
}

bool CHle::try_fast_audio_dispatching(void)
{
    // Identify audio microcode by using the content of ucode_data
    uint32_t ucode_data = *dmem_u32(this, TASK_UCODE_DATA);
    uint32_t v;

    if (*dram_u32(this, ucode_data) == 0x00000001)
    {
        if (*dram_u32(this, ucode_data + 0x30) == 0xf0000f00)
        {
            v = *dram_u32(this, ucode_data + 0x28);
            switch (v)
            {
            case 0x1e24138c: // Audio ABI (most common)
                alist_process_audio(this);
                return true;
            case 0x1dc8138c: // GoldenEye 007
                alist_process_audio_ge(this);
                return true;
            case 0x1e3c1390: // Blast Corp, Diddy Kong Racing
                alist_process_audio_bc(this);
                return true;
            default:
                WarnMessage("ABI1 identification regression: v=%08x", v);
            }
        }
        else
        {
            v = *dram_u32(this, ucode_data + 0x10);
            switch (v)
            {
            case 0x11181350: // Mario Kart, Wave Race (E)
                alist_process_nead_mk(this);
                return true;
            case 0x111812e0: // StarFox 64 (J)
                alist_process_nead_sfj(this);
                return true;
            case 0x110412ac: // Wave Race (J Rev B)
                alist_process_nead_wrjb(this);
                return true;
            case 0x110412cc: // StarFox/LylatWars (except J)
                alist_process_nead_sf(this);
                return true;
            case 0x1cd01250: // F-Zero X
                alist_process_nead_fz(this);
                return true;
            case 0x1f08122c: // Yoshi's Story
                alist_process_nead_ys(this);
                return true;
            case 0x1f38122c: // 1080° Snowboarding
                alist_process_nead_1080(this);
                return true;
            case 0x1f681230: // Zelda Ocarina of Time / Zelda Majora's Mask (J, J Rev A)
                alist_process_nead_oot(this);
                return true;
            case 0x1f801250: // Zelda Majora's Mask (except J, J Rev A, E Beta), Pokémon Stadium 2
                alist_process_nead_mm(this);
                return true;
            case 0x109411f8: // Zelda Majora's Mask (E Beta)
                alist_process_nead_mmb(this);
                return true;
            case 0x1eac11b8: // Animal Crossing
                alist_process_nead_ac(this);
                return true;
            case 0x00010010: // MusyX v2 (Indiana Jones, Battle For Naboo)
                musyx_v2_task(this);
                return true;
            default:
                WarnMessage("ABI2 identification regression: v=%08x", v);
            }
        }
    }
    else
    {
        v = *dram_u32(this, ucode_data + 0x10);
        switch (v)
        {
        case 0x00000001: /* MusyX v1
                         Rogue Squadron, Resident Evil 2, Polaris Sno Cross,
                         The World Is Not Enough, Rugrats In Paris, NBA Show Time,
                         Hydro Thunder, Tarzan, Gauntlet Legend, Rush 2049 */
            musyx_v1_task(this);
            return true;
        case 0x0000127c: // naudio (many games)
            alist_process_naudio(this);
            return true;
        case 0x00001280: // Banjo Kazooie
            alist_process_naudio_bk(this);
            return true;
        case 0x1c58126c: // Donkey Kong
            alist_process_naudio_dk(this);
            return true;
        case 0x1ae8143c: // Banjo Tooie, Jet Force Gemini, Mickey SpeedWay USA, Perfect Dark
            alist_process_naudio_mp3(this);
            return true;
        case 0x1ab0140c: // Conker's Bad Fur Day
            alist_process_naudio_cbfd(this);
            return true;
        default:
            WarnMessage("ABI3 identification regression: v=%08x", v);
        }
    }
    return false;
}

void CHle::normal_task_dispatching(void)
{
    const unsigned int sum =
        sum_bytes((const uint8_t *)dram_u32(this, *dmem_u32(this, TASK_UCODE)), min(*dmem_u32(this, TASK_UCODE_SIZE), 0xf80) >> 1);

    switch (sum)
    {
        // StoreVe12: found in Zelda Ocarina of Time [misleading task->type == 4]
    case 0x278:
        // Nothing to emulate
        return;

        // GFX: Twintris [misleading task->type == 0]
    case 0x212ee:
        if (m_ForwardGFX)
        {
            m_ProcessDList();
            return;
        }
        break;

        // JPEG: found in Pokémon Stadium J
    case 0x2c85a:
        jpeg_decode_PS0(this);
        return;

        // JPEG: found in Zelda Ocarina of Time, Pokémon Stadium 1, Pokémon Stadium 2
    case 0x2caa6:
        jpeg_decode_PS(this);
        return;

        // JPEG: found in Ogre Battle, Bottom of the 9th
    case 0x130de:
    case 0x278b0:
        jpeg_decode_OB(this);
        return;
    }

    WarnMessage("Unknown OSTask: sum: %x PC:%x", sum, *m_sp_pc);
#ifdef ENABLE_TASK_DUMP
    dump_unknown_task(this, sum);
#endif
}

void CHle::non_task_dispatching(void)
{
    const unsigned int sum = sum_bytes(m_imem, 44);

    if (sum == 0x9e2)
    {
        // CIC x105 microcode (used during boot of CIC x105 games)
        cicx105_ucode(this);
        return;
    }

    WarnMessage("Unknown RSP code: sum: %x PC:%x", sum, *m_sp_pc);
#ifdef ENABLE_TASK_DUMP
    dump_unknown_non_task(hle, sum);
#endif
}

void CHle::VerboseMessage(const char * /*message*/, ...)
{
#if defined(_WIN32) && defined(_DEBUG)
    // These can get annoying
#if 0
    MessageBox(NULL, message, "HLE verbose message", MB_OK);
#endif
#endif
}

void CHle::WarnMessage(const char * message, ...)
{
#if defined(_WIN32) && defined(_DEBUG)
    MessageBoxA(NULL, message, "HLE warning message", MB_OK);
#endif
}