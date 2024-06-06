#pragma once

#include <Common/Random.h>
#include <Common/SyncEvent.h>
#include <Common/Thread.h>
#include <Project64-core/Logging.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/Mempak.h>
#include <Project64-core/N64System/Mips/SystemEvents.h>
#include <Project64-core/N64System/Mips/SystemTiming.h>
#include <Project64-core/N64System/Profiling.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/Plugin.h>
#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/Settings/N64SystemSettings.h>

#include "FramePerSecond.h"
#include "Mips/TLB.h"
#include "SpeedLimiter.h"

typedef std::list<SystemEvent> EVENT_LIST;

typedef std::map<uint32_t, uint32_t> FUNC_CALLS;

class CPlugins;
class CRSP_Plugin;
class CRecompiler;

class VideoInterfaceHandler;

//#define TEST_SP_TRACKING  // Track the SP to make sure all ops pick it up fine
enum CN64SystemCB
{
    CN64SystemCB_Reset,
    CN64SystemCB_LoadedGameState,
};

class CN64System :
    public CLogging,
    protected CN64SystemSettings,
    public CGameSettings,
    protected CDebugSettings
{
public:
    typedef void (*CallBackFunction)(void *);

    CN64System(CPlugins * Plugins, uint32_t randomizer_seed, bool SavesReadOnly, bool SyncSystem);
    virtual ~CN64System(void);

    bool m_EndEmulation;
    SAVE_CHIP_TYPE m_SaveUsing;

    // Methods
    static bool LoadFileImage(const char * FileLoc);
    static bool LoadFileImageIPL(const char * FileLoc);
    static bool LoadDiskImage(const char * FileLoc, const bool Expansion);
    static bool SelectAndLoadFileImageIPL(Country country, bool combo);
    static bool RunFileImage(const char * FileLoc);
    static bool RunDiskImage(const char * FileLoc);
    static bool RunDiskComboImage(const char * FileLoc, const char * FileLocDisk);
    static void RunLoadedImage(void);
    static void CloseSystem(void);

    void RegisterCallBack(CN64SystemCB Type, void * Data, CallBackFunction Func);
    void UnregisterCallBack(CN64SystemCB Type, void * Data, CallBackFunction Func);

    void CloseCpu();
    void ExternalEvent(SystemEvent action); // Covers GUI interactions and timers etc.
    void StartEmulation(bool NewThread);
    void EndEmulation();
    void AlterSpeed(const CSpeedLimiter::ESpeedChange SpeedChange)
    {
        m_Limiter.AlterSpeed(SpeedChange);
    }
    void SetSpeed(int Speed)
    {
        m_Limiter.SetSpeed(Speed);
    }
    int GetSpeed(void) const
    {
        return m_Limiter.GetSpeed();
    }
    int GetBaseSpeed(void) const
    {
        return m_Limiter.GetBaseSpeed();
    }
    R4300iOpcode Opcode(void) const
    {
        return m_OpCodes.Opcode();
    }
    CTLB & TLB();
    void Reset(bool bInitReg, bool ClearMenory);
    void GameReset();
    void PluginReset();
    void ApplyGSButton(void);

    void Pause();
    bool SaveState();
    bool LoadState(const char * FileName);
    bool LoadState();
    uint32_t GetButtons(int32_t Control) const;

    // Variable used to track that the SP is being handled and stays the same as the real SP in sync core
#ifdef TEST_SP_TRACKING
    uint32_t m_CurrentSP;
#endif
    // For sync CPU
    void UpdateSyncCPU(uint32_t const Cycles);
    void SyncSystem();
    void SyncSystemPC();

    CPlugins * GetPlugins()
    {
        return m_Plugins;
    }
    PIPELINE_STAGE PipelineStage() const
    {
        return m_PipelineStage;
    }
    uint64_t JumpToLocation() const
    {
        return m_JumpToLocation;
    }

private:
    struct SETTING_CHANGED_CB
    {
        void * Data;
        CallBackFunction Func;
    };
    typedef std::vector<SETTING_CHANGED_CB> SETTING_CHANGED_CB_LIST;
    typedef std::map<CN64SystemCB, SETTING_CHANGED_CB_LIST> SETTING_CALLBACK;

    // Make sure plugins can directly access this information
    friend class CGfxPlugin;
    friend class CAudioPlugin;
    friend class CRSP_Plugin;
    friend class CControl_Plugin;

    friend class CSystemTimer;
    friend class CRecompiler;
    friend class CRecompilerOpsBase;
    friend class CX86RecompilerOps;
    friend class CArmRecompilerOps;
    friend class CCodeBlock;
    friend class CMipsMemoryVM;
    friend class R4300iOp;
    friend class CSystemEvents;
    friend class VideoInterfaceHandler;
    friend class PifRamHandler;
    friend class CRegisters;

    // Used for loading and potentially executing the CPU in its own thread
    static void StartEmulationThread(CThread * thread);
    static bool EmulationStarting(CThread * thread);
    static void StartEmulationThead();

    void ExecuteCPU();
    void RefreshScreen();
    void DumpSyncErrors();
    void StartEmulation2(bool NewThread);
    bool SetActiveSystem(bool bActive = true);
    void NotifyCallback(CN64SystemCB Type);
    void DelayedJump(uint64_t JumpLocation);
    void DelayedRelativeJump(uint64_t RelativeLocation);

    // CPU methods
    void ExecuteRecompiler();
    void ExecuteInterpret();
    void ExecuteSyncCPU();

    // Mark information saying that the CPU has stopped
    void CpuStopped();

    // Functions in CTLB_CB
    void TLB_Unmaped(uint32_t VAddr, uint32_t Len);

    SETTING_CALLBACK m_Callback;
    CPlugins * const m_Plugins; // The plugin container
    CPlugins * m_SyncPlugins;
    CN64System * m_SyncCPU;
    CSystemEvents m_SystemEvents;
    CMipsMemoryVM m_MMU_VM;
    CRegisters m_Reg;
    CTLB m_TLB;
    R4300iOp m_OpCodes;
    CMempak m_Mempak;
    CFramePerSecond m_FPS;
    CProfiling m_CPU_Usage; // Used to track the CPU usage
    CRecompiler * m_Recomp;
    CSpeedLimiter m_Limiter;
    bool m_InReset;
    int32_t m_NextTimer;
    CSystemTimer m_SystemTimer;
    bool m_bCleanFrameBox;
    uint32_t m_Buttons[4];
    bool m_TestTimer;
    PIPELINE_STAGE m_PipelineStage;
    uint64_t m_JumpToLocation;
    uint64_t m_JumpDelayLocation;
    uint32_t m_TLBLoadAddress;
    uint32_t m_TLBStoreAddress;
    uint32_t m_SyncCount;
    bool m_SyncSystem;
    CRandom m_Random;

    // When syncing cores this is the PC where it last synced correctly
    uint64_t m_LastSuccessSyncPC[10];
    int32_t m_CyclesToSkip;

    // Handle to the CPU thread
    CThread * m_thread;

    // Handle to pause mutex
    SyncEvent m_hPauseEvent;

    // List of function that have been called (used in profiling)
    FUNC_CALLS m_FunctionCalls;

    // List of save state file IDs
    const uint32_t SaveID_0 = 0x23D8A6C8;   // Main save state info (*.pj)
    const uint32_t SaveID_0_1 = 0x25EF3FAC; // Main save state info (*.pj)
    const uint32_t SaveID_1 = 0x56D2CD23;   // Extra data v1 (system timing) info (*.dat)
    const uint32_t SaveID_2 = 0x750A6BEB;   // Extra data v2 (timing + disk registers) (*.dat)
};
