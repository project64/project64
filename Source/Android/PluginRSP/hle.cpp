// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2012 Bobby Smiles
// Copyright(C) 2009 Richard Goedeken
// Copyright(C) 2002 Hacktarux
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#include "stdafx.h"
#include "mem.h"
#include "ucodes.h"
#include <memory.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))

/* helper functions prototypes */
static unsigned int sum_bytes(const uint8_t *bytes, uint32_t size);

CHle::CHle(const RSP_INFO & Rsp_Info) :
    m_dram(Rsp_Info.RDRAM),
    m_dmem(Rsp_Info.DMEM),
    m_imem(Rsp_Info.IMEM),
    m_mi_intr(Rsp_Info.MI_INTR_REG),
    m_sp_mem_addr(Rsp_Info.SP_MEM_ADDR_REG),
    m_sp_dram_addr(Rsp_Info.SP_DRAM_ADDR_REG),
    m_sp_rd_length(Rsp_Info.SP_RD_LEN_REG),
    m_sp_wr_length(Rsp_Info.SP_WR_LEN_REG),
    m_sp_status(Rsp_Info.SP_STATUS_REG),
    m_sp_dma_full(Rsp_Info.SP_DMA_FULL_REG),
    m_sp_dma_busy(Rsp_Info.SP_DMA_BUSY_REG),
    m_sp_pc(Rsp_Info.SP_PC_REG),
    m_sp_semaphore(Rsp_Info.SP_SEMAPHORE_REG),
    m_dpc_start(Rsp_Info.DPC_START_REG),
    m_dpc_end(Rsp_Info.DPC_END_REG),
    m_dpc_current(Rsp_Info.DPC_CURRENT_REG),
    m_dpc_status(Rsp_Info.DPC_STATUS_REG),
    m_dpc_clock(Rsp_Info.DPC_CLOCK_REG),
    m_dpc_bufbusy(Rsp_Info.DPC_BUFBUSY_REG),
    m_dpc_pipebusy(Rsp_Info.DPC_PIPEBUSY_REG),
    m_dpc_tmem(Rsp_Info.DPC_TMEM_REG),
    m_CheckInterrupts(Rsp_Info.CheckInterrupts),
    m_ProcessDList(Rsp_Info.ProcessDList),
    m_ProcessAList(Rsp_Info.ProcessAList),
    m_ProcessRdpList(Rsp_Info.ProcessRdpList),
    m_ShowCFB(Rsp_Info.ShowCFB),
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

void CHle::hle_execute(void)
{
    if (is_task())
    {
        if (!try_fast_task_dispatching())
        {
            normal_task_dispatching();
        }
        rsp_break(SP_STATUS_SIG2);
    }
    else
    {
        non_task_dispatching();
        rsp_break(0);
    }
}

/* local functions */
static unsigned int sum_bytes(const uint8_t * bytes, unsigned int size)
{
    unsigned int sum = 0;
    const unsigned char *const bytes_end = bytes + size;

    while (bytes != bytes_end)
    {
        sum += *bytes++;
    }
    return sum;
}

/**
* Try to figure if the RSP was launched using osSpTask* functions
* and not run directly (in which case DMEM[0xfc0-0xfff] is meaningless).
*
* Previously, the ucode_size field was used to determine this,
* but it is not robust enough (hi Pokemon Stadium !) because games could write anything
* in this field : most ucode_boot discard the value and just use 0xf7f anyway.
*
* Using ucode_boot_size should be more robust in this regard.
**/
bool CHle::is_task(void)
{
    return (*dmem_u32(this, TASK_UCODE_BOOT_SIZE) <= 0x1000);
}

bool CHle::try_fast_task_dispatching(void)
{
    /* identify task ucode by its type */
    switch (*dmem_u32(this, TASK_TYPE))
    {
    case 1:
        if (m_ForwardGFX)
        {
            m_ProcessDList();
            return true;
        }
        break;
    case 2:
        if (m_AudioHle)
        {
            m_ProcessAList();
            return true;
        }
        else if (try_fast_audio_dispatching())
        {
            return true;
        }
        break;
    case 7:
        m_ShowCFB();
        return true;
    }
    return false;
}

bool CHle::try_fast_audio_dispatching(void)
{
    /* identify audio ucode by using the content of ucode_data */
    uint32_t ucode_data = *dmem_u32(this, TASK_UCODE_DATA);
    uint32_t v;

    if (*dram_u32(this, ucode_data) == 0x00000001)
    {
        if (*dram_u32(this, ucode_data + 0x30) == 0xf0000f00)
        {
            v = *dram_u32(this, ucode_data + 0x28);
            switch (v)
            {
            case 0x1e24138c: /* audio ABI (most common) */
                alist_process_audio(this);
                return true;
            case 0x1dc8138c: /* GoldenEye */
                alist_process_audio_ge(this);
                return true;
            case 0x1e3c1390: /* BlastCorp, DiddyKongRacing */
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
            case 0x11181350: /* MarioKart, WaveRace (E) */
                alist_process_nead_mk(this);
                return true;
            case 0x111812e0: /* StarFox (J) */
                alist_process_nead_sfj(this);
                return true;
            case 0x110412ac: /* WaveRace (J RevB) */
                alist_process_nead_wrjb(this);
                return true;
            case 0x110412cc: /* StarFox/LylatWars (except J) */
                alist_process_nead_sf(this);
                return true;
            case 0x1cd01250: /* FZeroX */
                alist_process_nead_fz(this);
                return true;
            case 0x1f08122c: /* YoshisStory */
                alist_process_nead_ys(this);
                return true;
            case 0x1f38122c: /* 1080° Snowboarding */
                alist_process_nead_1080(this);
                return true;
            case 0x1f681230: /* Zelda OoT / Zelda MM (J, J RevA) */
                alist_process_nead_oot(this);
                return true;
            case 0x1f801250: /* Zelda MM (except J, J RevA, E Beta), PokemonStadium 2 */
                alist_process_nead_mm(this);
                return true;
            case 0x109411f8: /* Zelda MM (E Beta) */
                alist_process_nead_mmb(this);
                return true;
            case 0x1eac11b8: /* AnimalCrossing */
                alist_process_nead_ac(this);
                return true;
            case 0x00010010: /* MusyX v2 (IndianaJones, BattleForNaboo) */
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
                         RogueSquadron, ResidentEvil2, PolarisSnoCross,
                         TheWorldIsNotEnough, RugratsInParis, NBAShowTime,
                         HydroThunder, Tarzan, GauntletLegend, Rush2049 */
            musyx_v1_task(this);
            return true;
        case 0x0000127c: /* naudio (many games) */
            alist_process_naudio(this);
            return true;
        case 0x00001280: /* BanjoKazooie */
            alist_process_naudio_bk(this);
            return true;
        case 0x1c58126c: /* DonkeyKong */
            alist_process_naudio_dk(this);
            return true;
        case 0x1ae8143c: /* BanjoTooie, JetForceGemini, MickeySpeedWayUSA, PerfectDark */
            alist_process_naudio_mp3(this);
            return true;
        case 0x1ab0140c: /* ConkerBadFurDay */
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

    switch (sum) {
        /* StoreVe12: found in Zelda Ocarina of Time [misleading task->type == 4] */
    case 0x278:
        /* Nothing to emulate */
        return;

        /* GFX: Twintris [misleading task->type == 0] */
    case 0x212ee:
        if (m_ForwardGFX)
        {
            m_ProcessDList();
            return;
        }
        break;

        /* JPEG: found in Pokemon Stadium J */
    case 0x2c85a:
        jpeg_decode_PS0(this);
        return;

        /* JPEG: found in Zelda Ocarina of Time, Pokemon Stadium 1, Pokemon Stadium 2 */
    case 0x2caa6:
        jpeg_decode_PS(this);
        return;

        /* JPEG: found in Ogre Battle, Bottom of the 9th */
    case 0x130de:
    case 0x278b0:
        jpeg_decode_OB(this);
        return;
    }

    WarnMessage("unknown OSTask: sum: %x PC:%x", sum, *m_sp_pc);
#ifdef ENABLE_TASK_DUMP
    dump_unknown_task(this, sum);
#endif
}

void CHle::non_task_dispatching(void)
{
    const unsigned int sum = sum_bytes(m_imem, 44);

    if (sum == 0x9e2)
    {
        /* CIC x105 ucode (used during boot of CIC x105 games) */
        cicx105_ucode(this);
        return;
    }

    WarnMessage("unknown RSP code: sum: %x PC:%x", sum, *m_sp_pc);
#ifdef ENABLE_TASK_DUMP
    dump_unknown_non_task(hle, sum);
#endif
}

#if defined(_WIN32) && defined(_DEBUG)
#include <Windows.h>
#endif

void CHle::VerboseMessage(const char *message, ...)
{
#if defined(_WIN32) && defined(_DEBUG)
    // These can get annoying.
#if 0
    MessageBox(NULL, message, "HLE Verbose Message", MB_OK);
#endif
#endif
}

void CHle::WarnMessage(const char *message, ...)
{
#if defined(_WIN32) && defined(_DEBUG)
    MessageBoxA(NULL, message, "HLE Warning Message", MB_OK);
#endif
}