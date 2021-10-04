#include "stdafx.h"
#include "N64System.h"
#include <Project64-core/3rdParty/zip.h>
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/Mempak.h>
#include <Project64-core/N64System/Mips/Transferpak.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/N64System/Mips/OpcodeName.h>
#include <Project64-core/N64System/Mips/Disk.h>
#include <Project64-core/N64System/N64Disk.h>
#include <Project64-core/N64System/Enhancement/Enhancements.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/ExceptionHandler.h>
#include <Project64-core/Logging.h>
#include <Project64-core/Debugger.h>
#include <Common/Util.h>
#include <float.h>
#include <time.h>
#if defined(ANDROID)
#include <utime.h>
#endif

#pragma warning(disable:4355) // Disable 'this' : used in base member initializer list

CN64System::CN64System(CPlugins * Plugins, uint32_t randomizer_seed, bool SavesReadOnly, bool SyncSystem) :
    CSystemEvents(this, Plugins),
    m_EndEmulation(false),
    m_SaveUsing((SAVE_CHIP_TYPE)g_Settings->LoadDword(Game_SaveChip)),
    m_Plugins(Plugins),
    m_SyncCPU(nullptr),
    m_SyncPlugins(nullptr),
    m_MMU_VM(SavesReadOnly),
    //m_Cheats(m_MMU_VM),
    m_TLB(this),
    m_Reg(this, this),
    m_Recomp(nullptr),
    m_InReset(false),
    m_NextTimer(0),
    m_SystemTimer(m_Reg, m_NextTimer),
    m_bCleanFrameBox(true),
    m_RspBroke(true),
    m_DMAUsed(false),
    m_TestTimer(false),
    m_NextInstruction(0),
    m_JumpToLocation(0),
    m_TLBLoadAddress(0),
    m_TLBStoreAddress(0),
    m_SyncCount(0),
    m_thread(nullptr),
    m_hPauseEvent(true),
    m_SyncSystem(SyncSystem),
    m_Random(randomizer_seed)
{
    WriteTrace(TraceN64System, TraceDebug, "Start");
    memset(m_LastSuccessSyncPC, 0, sizeof(m_LastSuccessSyncPC));

    uint32_t gameHertz = g_Settings->LoadDword(Game_ScreenHertz);
    if (gameHertz == 0)
    {
        gameHertz = (SystemType() == SYSTEM_PAL) ? 50 : 60;
    }
    m_Limiter.SetHertz(gameHertz);
    g_Settings->SaveDword(GameRunning_ScreenHertz, gameHertz);
    WriteTrace(TraceN64System, TraceDebug, "Setting up system");
    CInterpreterCPU::BuildCPU();

    if (!m_MMU_VM.Initialize(SyncSystem))
    {
        WriteTrace(TraceN64System, TraceWarning, "MMU failed to initialize");
        WriteTrace(TraceN64System, TraceDebug, "Done");
        return;
    }

    WriteTrace(TraceN64System, TraceDebug, "Resetting plugins");
    g_Notify->DisplayMessage(5, MSG_PLUGIN_INIT);
    m_Plugins->CreatePlugins();
    bool bRes = m_Plugins->Initiate(this);
    if (!bRes)
    {
        WriteTrace(TraceN64System, TraceError, "g_Plugins->Initiate Failed");
        WriteTrace(TraceN64System, TraceDebug, "Done (Res: false)");
        return;
    }

    if (!SyncSystem)
    {
        uint32_t CpuType = g_Settings->LoadDword(Game_CpuType);
        WriteTrace(TraceN64System, TraceDebug, "CpuType = %d", CpuType);
        if (CpuType == CPU_SyncCores && !g_Settings->LoadBool(Debugger_Enabled))
        {
            g_Settings->SaveDword(Game_CpuType, CPU_Recompiler);
            CpuType = CPU_Recompiler;
        }
        if (CpuType == CPU_SyncCores)
        {
            if (g_Plugins->SyncWindow() == nullptr)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            g_Notify->DisplayMessage(5, "Copy plugins");
            g_Plugins->CopyPlugins(g_Settings->LoadStringVal(Directory_PluginSync));
            m_SyncPlugins = new CPlugins(Directory_PluginSync, true);
            m_SyncPlugins->SetRenderWindows(g_Plugins->SyncWindow(), nullptr);
            m_SyncCPU = new CN64System(m_SyncPlugins, randomizer_seed, true, true);
        }

        Reset(true, true);

        if (CpuType == CPU_Recompiler || CpuType == CPU_SyncCores)
        {
            m_Recomp = new CRecompiler(m_MMU_VM, m_Reg, m_EndEmulation);
        }

        if (g_Settings->LoadBool(Game_LoadSaveAtStart))
        {
            LoadState();
            g_Settings->SaveBool(Game_LoadSaveAtStart, false);
        }
    }
    g_Enhancements->ResetActive(Plugins);
    g_Enhancements->UpdateCheats();

    WriteTrace(TraceN64System, TraceDebug, "Done");
}

CN64System::~CN64System()
{
    SetActiveSystem(false);
    Transferpak::Release();
    if (m_SyncCPU)
    {
        m_SyncCPU->CpuStopped();
        delete m_SyncCPU;
        m_SyncCPU = nullptr;
    }
    if (m_Recomp)
    {
        delete m_Recomp;
        m_Recomp = nullptr;
    }
    if (m_SyncPlugins)
    {
        delete m_SyncPlugins;
        m_SyncPlugins = nullptr;
    }
    if (m_thread != nullptr)
    {
        WriteTrace(TraceN64System, TraceDebug, "Deleting thread object");
        delete m_thread;
        m_thread = nullptr;
    }
}

void CN64System::ExternalEvent(SystemEvent action)
{
    WriteTrace(TraceN64System, TraceDebug, "Action: %s", SystemEventName(action));

    if (action == SysEvent_LoadMachineState &&
        !g_Settings->LoadBool(GameRunning_CPU_Running) &&
        g_BaseSystem != nullptr &&
        g_BaseSystem->LoadState())
    {
        WriteTrace(TraceN64System, TraceDebug, "Ignore event, manually loaded save");
        return;
    }

    if (action == SysEvent_SaveMachineState &&
        !g_Settings->LoadBool(GameRunning_CPU_Running) &&
        g_BaseSystem != nullptr &&
        g_BaseSystem->SaveState())
    {
        WriteTrace(TraceN64System, TraceDebug, "Ignore event, manually saved event");
        return;
    }

    switch (action)
    {
    case SysEvent_ExecuteInterrupt:
    case SysEvent_SaveMachineState:
    case SysEvent_LoadMachineState:
    case SysEvent_ChangingFullScreen:
    case SysEvent_GSButtonPressed:
    case SysEvent_ResetCPU_SoftDone:
    case SysEvent_Interrupt_SP:
    case SysEvent_Interrupt_SI:
    case SysEvent_Interrupt_AI:
    case SysEvent_Interrupt_VI:
    case SysEvent_Interrupt_PI:
    case SysEvent_Interrupt_DP:
    case SysEvent_ResetCPU_Hard:
    case SysEvent_ResetCPU_Soft:
    case SysEvent_CloseCPU:
    case SysEvent_ChangePlugins:
    case SysEvent_PauseCPU_FromMenu:
    case SysEvent_ResetFunctionTimes:
    case SysEvent_DumpFunctionTimes:
    case SysEvent_ResetRecompilerCode:
        QueueEvent(action);
        break;
    case SysEvent_PauseCPU_AppLostFocus:
    case SysEvent_PauseCPU_AppLostActive:
    case SysEvent_PauseCPU_SaveGame:
    case SysEvent_PauseCPU_LoadGame:
    case SysEvent_PauseCPU_DumpMemory:
    case SysEvent_PauseCPU_SearchMemory:
    case SysEvent_PauseCPU_Settings:
    case SysEvent_PauseCPU_Cheats:
    case SysEvent_PauseCPU_Enhancement:
        if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
        {
            QueueEvent(action);
        }
        break;
    case SysEvent_PauseCPU_ChangingBPs:
        if (!WaitingForStep() && !g_Settings->LoadBool(GameRunning_CPU_Paused))
        {
            QueueEvent(action);
            for (int i = 0; i < 100; i++)
            {
                bool paused = g_Settings->LoadBool(GameRunning_CPU_Paused);
                pjutil::Sleep(1);
                if (paused)
                {
                    break;
                }
            }
        }
        break;
    case SysEvent_ResumeCPU_FromMenu:
        // Always resume if from menu
        m_hPauseEvent.Trigger();
        break;
    case SysEvent_ResumeCPU_AppGainedFocus:
        if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_AppLostFocus)
        {
            m_hPauseEvent.Trigger();
        }
        break;
    case SysEvent_ResumeCPU_AppGainedActive:
        if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_AppLostActive)
        {
            m_hPauseEvent.Trigger();
        }
        break;
    case SysEvent_ResumeCPU_SaveGame:
        if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_SaveGame)
        {
            m_hPauseEvent.Trigger();
        }
        break;
    case SysEvent_ResumeCPU_LoadGame:
        if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_LoadGame)
        {
            m_hPauseEvent.Trigger();
        }
        break;
    case SysEvent_ResumeCPU_DumpMemory:
        if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_DumpMemory)
        {
            m_hPauseEvent.Trigger();
        }
        break;
    case SysEvent_ResumeCPU_SearchMemory:
        if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_SearchMemory)
        {
            m_hPauseEvent.Trigger();
        }
        break;
    case SysEvent_ResumeCPU_Settings:
        if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_Settings)
        {
            m_hPauseEvent.Trigger();
        }
        break;
    case SysEvent_ResumeCPU_Cheats:
        if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_Cheats)
        {
            m_hPauseEvent.Trigger();
        }
        break;
    case SysEvent_ResumeCPU_ChangingBPs:
        if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_ChangingBPs)
        {
            m_hPauseEvent.Trigger();
        }
        break;
    case SysEvent_ResumeCPU_Enhancement:
        if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_Enhancement)
        {
            m_hPauseEvent.Trigger();
        }
        break;
    default:
        WriteTrace(TraceN64System, TraceError, "Unknown event %d", action);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

bool CN64System::LoadFileImage(const char * FileLoc)
{
    WriteTrace(TraceN64System, TraceDebug, "Start (FileLoc: %s)", FileLoc);
    CloseSystem();

    g_Settings->SaveDword(Game_CurrentSaveState, g_Settings->LoadDefaultDword(Game_CurrentSaveState));
    if (g_Settings->LoadBool(GameRunning_LoadingInProgress))
    {
        WriteTrace(TraceN64System, TraceError, "Game loading is in progress, cannot load new file");
        return false;
    }

    // Mark the ROM as loading
    WriteTrace(TraceN64System, TraceDebug, "Mark ROM as loading");
    g_Settings->SaveString(Game_File, "");
    g_Settings->SaveBool(GameRunning_LoadingInProgress, true);

    // Try to load the passed N64 ROM
    if (g_Rom == nullptr)
    {
        WriteTrace(TraceN64System, TraceDebug, "Allocating global ROM object");
        g_Rom = new CN64Rom();
    }
    else
    {
        WriteTrace(TraceN64System, TraceDebug, "Use existing global ROM object");
    }

    WriteTrace(TraceN64System, TraceDebug, "Loading \"%s\"", FileLoc);
    if (g_Rom->LoadN64Image(FileLoc))
    {
        if (g_Rom->IsLoadedRomDDIPL())
        {
            // 64DD IPL
            if (g_DDRom == nullptr)
            {
                g_DDRom = new CN64Rom();
            }
            g_DDRom->LoadN64ImageIPL(FileLoc);
            if (g_DDRom->CicChipID() == CIC_NUS_8303)
                g_Settings->SaveString(File_DiskIPLPath, FileLoc);
            else if (g_DDRom->CicChipID() == CIC_NUS_DDUS)
                g_Settings->SaveString(File_DiskIPLUSAPath, FileLoc);
            else if (g_DDRom->CicChipID() == CIC_NUS_DDTL)
                g_Settings->SaveString(File_DiskIPLTOOLPath, FileLoc);
        }

        g_System->RefreshGameSettings();

        if (g_Disk == nullptr || !g_Rom->IsLoadedRomDDIPL())
        {
            g_Settings->SaveString(Game_File, FileLoc);
        }
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
        WriteTrace(TraceN64System, TraceDebug, "Finished loading (GoodName: %s)", g_Settings->LoadStringVal(Rdb_GoodName).c_str());
    }
    else
    {
        WriteTrace(TraceN64System, TraceError, "LoadN64Image failed (\"%s\")", FileLoc);
        g_Notify->DisplayError(g_Rom->GetError());
        delete g_Rom;
        g_Rom = nullptr;
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
        WriteTrace(TraceN64System, TraceDebug, "Done (res: false)");
        return false;
    }
    WriteTrace(TraceN64System, TraceDebug, "Done (res: true)");
    return true;
}

bool CN64System::LoadFileImageIPL(const char * FileLoc)
{
    CloseSystem();
    if (g_Settings->LoadBool(GameRunning_LoadingInProgress))
    {
        return false;
    }

    // Mark the N64DD IPL as loading
    WriteTrace(TraceN64System, TraceDebug, "Mark N64DD IPL as loading");
    //g_Settings->SaveString(Game_File, "");
    g_Settings->SaveBool(GameRunning_LoadingInProgress, true);

    // Try to load the passed N64DD IPL
    if (g_DDRom == nullptr)
    {
        WriteTrace(TraceN64System, TraceDebug, "Allocating global N64DD IPL object");
        g_DDRom = new CN64Rom();
    }
    else
    {
        WriteTrace(TraceN64System, TraceDebug, "Use existing global N64DD IPL object");
    }

    WriteTrace(TraceN64System, TraceDebug, "Loading \"%s\"", FileLoc);
    if (g_DDRom->LoadN64ImageIPL(FileLoc))
    {
        if (!g_DDRom->IsLoadedRomDDIPL())
        {
            // If not 64DD IPL then it's wrong
            WriteTrace(TraceN64System, TraceError, "LoadN64ImageIPL failed (\"%s\")", FileLoc);
            g_Notify->DisplayError(g_DDRom->GetError());
            delete g_DDRom;
            g_DDRom = nullptr;
            g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
            return false;
        }

        g_System->RefreshGameSettings();

        if (g_DDRom->CicChipID() == CIC_NUS_8303)
            g_Settings->SaveString(File_DiskIPLPath, FileLoc);
        else if (g_DDRom->CicChipID() == CIC_NUS_DDUS)
            g_Settings->SaveString(File_DiskIPLUSAPath, FileLoc);
        else if (g_DDRom->CicChipID() == CIC_NUS_DDTL)
            g_Settings->SaveString(File_DiskIPLTOOLPath, FileLoc);

        //g_Settings->SaveString(Game_File, FileLoc);
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
    }
    else
    {
        WriteTrace(TraceN64System, TraceError, "LoadN64ImageIPL failed (\"%s\")", FileLoc);
        g_Notify->DisplayError(g_DDRom->GetError());
        delete g_DDRom;
        g_DDRom = nullptr;
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
        return false;
    }
    return true;
}

bool CN64System::LoadDiskImage(const char * FileLoc, const bool Expansion)
{
    CloseSystem();
    if (g_Settings->LoadBool(GameRunning_LoadingInProgress))
    {
        return false;
    }

    // Mark the disk as loading
    WriteTrace(TraceN64System, TraceDebug, "Mark disk as loading");
    //g_Settings->SaveString(Game_File, "");
    g_Settings->SaveBool(GameRunning_LoadingInProgress, true);

    // Try to load the passed N64 disk
    if (g_Disk == nullptr)
    {
        WriteTrace(TraceN64System, TraceDebug, "Allocating global disk object");
        g_Disk = new CN64Disk();
    }
    else
    {
        WriteTrace(TraceN64System, TraceDebug, "Use existing global disk object");
    }

    WriteTrace(TraceN64System, TraceDebug, "Loading \"%s\"", FileLoc);
    if (g_Disk->LoadDiskImage(FileLoc))
    {
        g_System->RefreshGameSettings();

        if (!Expansion)
        {
            g_Settings->SaveString(Game_File, FileLoc);
        }
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
    }
    else
    {
        WriteTrace(TraceN64System, TraceError, "LoadDiskImage failed (\"%s\")", FileLoc);
        g_Notify->DisplayError(g_Disk->GetError());
        delete g_Disk;
        g_Disk = nullptr;
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
        return false;
    }
    return true;
}

bool CN64System::RunFileImage(const char * FileLoc)
{
    // Uninitialize g_Disk and g_DDRom to prevent exception when ending emulation of a regular ROM after playing 64DD content previously
    if (g_Disk != nullptr)
    {
        g_Disk->UnallocateDiskImage();
        delete g_Disk;
        g_Disk = nullptr;
    }
    if (g_DDRom != nullptr)
    {
        g_DDRom->UnallocateRomImage();
        delete g_DDRom;
        g_DDRom = nullptr;
    }
    if (!LoadFileImage(FileLoc))
    {
        return false;
    }
    g_Settings->SaveBool(Setting_EnableDisk, g_Rom->IsLoadedRomDDIPL());
    if (g_Rom->IsLoadedRomDDIPL())
    {
        if (g_Rom->CicChipID() == CIC_NUS_8303)
            g_Settings->SaveString(File_DiskIPLPath, FileLoc);
        else if (g_Rom->CicChipID() == CIC_NUS_DDUS)
            g_Settings->SaveString(File_DiskIPLUSAPath, FileLoc);
        else if (g_Rom->CicChipID() == CIC_NUS_DDTL)
            g_Settings->SaveString(File_DiskIPLTOOLPath, FileLoc);
    }
    RunLoadedImage();
    return true;
}

bool CN64System::RunDiskImage(const char * FileLoc)
{
    if (!LoadDiskImage(FileLoc, false))
    {
        return false;
    }

    // Select IPL ROM depending on disk country code
    if (!SelectAndLoadFileImageIPL(g_Disk->GetCountry(), false))
    {
        return false;
    }

    g_Settings->SaveBool(Setting_EnableDisk, true);
    RunLoadedImage();
    return true;
}

bool CN64System::RunDiskComboImage(const char * FileLoc, const char * FileLocDisk)
{
    if (!LoadDiskImage(FileLocDisk, true))
    {
        return false;
    }
    if (!LoadFileImage(FileLoc))
    {
        return false;
    }

    // Select IPL ROM depending on disk country code
    if (!SelectAndLoadFileImageIPL(g_Disk->GetCountry(), true))
    {
        return false;
    }

    g_Settings->SaveBool(Setting_EnableDisk, true);
    RunLoadedImage();
    return true;
}

void CN64System::RunLoadedImage(void)
{
    WriteTrace(TraceN64System, TraceDebug, "Start");
    g_BaseSystem = new CN64System(g_Plugins, (uint32_t)time(nullptr), false, false);
    if (g_BaseSystem)
    {
        if (g_Settings->LoadBool(Setting_AutoStart) != 0)
        {
            WriteTrace(TraceN64System, TraceDebug, "Automatically starting ROM");
            g_BaseSystem->StartEmulation(true);
        }
    }
    else
    {
        WriteTrace(TraceN64System, TraceError, "Failed to create CN64System");
    }
    WriteTrace(TraceN64System, TraceDebug, "Done");
}

void CN64System::CloseSystem()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");
    g_Settings->SaveBool(Game_FullSpeed, true);
    if (g_BaseSystem)
    {
        g_BaseSystem->CloseCpu();
        delete g_BaseSystem;
        g_BaseSystem = nullptr;
    }
    WriteTrace(TraceN64System, TraceDebug, "Done");
}

bool CN64System::SelectAndLoadFileImageIPL(Country country, bool combo)
{
    delete g_DDRom;
    g_DDRom = nullptr;

    SettingID IPLROMPathSetting;
    LanguageStringID IPLROMError;
    switch (country)
    {
        case Country_Japan:
            IPLROMPathSetting = File_DiskIPLPath;
            IPLROMError = MSG_IPL_REQUIRED;
            break;
        case Country_NorthAmerica:
            IPLROMPathSetting = File_DiskIPLUSAPath;
            IPLROMError = MSG_USA_IPL_REQUIRED;
            break;
        case Country_Unknown:
        default:
            IPLROMPathSetting = File_DiskIPLTOOLPath;
            IPLROMError = MSG_TOOL_IPL_REQUIRED;
            if (combo && !CPath(g_Settings->LoadStringVal(File_DiskIPLTOOLPath).c_str()).Exists())
            {
                // Development IPL is not needed for combo ROM + disk loading
                if (CPath(g_Settings->LoadStringVal(File_DiskIPLPath).c_str()).Exists())
                    IPLROMPathSetting = File_DiskIPLPath;
                else if (CPath(g_Settings->LoadStringVal(File_DiskIPLUSAPath).c_str()).Exists())
                    IPLROMPathSetting = File_DiskIPLUSAPath;
            }
            break;
    }

    if (!CPath(g_Settings->LoadStringVal(IPLROMPathSetting).c_str()).Exists())
    {
        g_Notify->DisplayWarning(IPLROMError);
        return false;
    }

    if (combo)
    {
        if (!LoadFileImageIPL(g_Settings->LoadStringVal(IPLROMPathSetting).c_str()))
        {
            g_Settings->SaveString(IPLROMPathSetting, "");
            g_Notify->DisplayWarning(IPLROMError);
            return false;
        }
    }
    else
    {
        if (!LoadFileImage(g_Settings->LoadStringVal(IPLROMPathSetting).c_str()))
        {
            g_Settings->SaveString(IPLROMPathSetting, "");
            g_Notify->DisplayWarning(IPLROMError);
            return false;
        }
        else
        {
            if (!g_Rom->IsLoadedRomDDIPL())
            {
                //g_Notify->DisplayError(MSG_FAIL_IMAGE_IPL);
                g_Notify->DisplayWarning(IPLROMError);
                g_Settings->SaveString(IPLROMPathSetting, "");
                return false;
            }
        }
    }
    return true;
}

bool CN64System::EmulationStarting(CThread * thread)
{
    WriteTrace(TraceN64System, TraceDebug, "Starting (hThread: %p ThreadId: %d)", thread, thread->ThreadID());
    bool bRes = true;

    WriteTrace(TraceN64System, TraceDebug, "Setting N64 system as active");
    if (g_BaseSystem->SetActiveSystem(true))
    {
        g_BaseSystem->m_thread = thread;
        WriteTrace(TraceN64System, TraceDebug, "Setting up N64 system is done");
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
        try
        {
            WriteTrace(TraceN64System, TraceDebug, "Game starting");
            g_BaseSystem->StartEmulation2(false);
            WriteTrace(TraceN64System, TraceDebug, "Game done");
            // TODO: Add 64DD saving code?
            if (g_Disk != nullptr)
            {
                g_Disk->SaveDiskImage();
                //g_Notify->DisplayError(g_Disk->GetError());
                WriteTrace(TraceN64System, TraceDebug, "64DD Save Done");
            }
        }
        catch (...)
        {
            g_Notify->DisplayError(stdstr_f("%s: Exception caught\nFile: %s\nLine: %d", __FUNCTION__, __FILE__, __LINE__).c_str());
        }
    }
    else
    {
        WriteTrace(TraceN64System, TraceError, "SetActiveSystem failed");
        g_Notify->DisplayError(stdstr_f("%s: Failed to Initialize N64 System", __FUNCTION__).c_str());
        g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
        bRes = false;
    }
    return bRes;
}

void CN64System::StartEmulation2(bool NewThread)
{
    WriteTrace(TraceN64System, TraceDebug, "Start (NewThread: %s)", NewThread ? "true" : "false");
    if (NewThread)
    {
        if (HaveDebugger())
        {
            StartLog();
        }
        g_Settings->SaveDword(Game_CurrentSaveState, g_Settings->LoadDefaultDword(Game_CurrentSaveState));

        WriteTrace(TraceN64System, TraceDebug, "Setting system as active");
        bool bSetActive = SetActiveSystem();
        if (bSetActive && m_SyncCPU)
        {
            bSetActive = m_SyncCPU->SetActiveSystem();
            if (bSetActive)
            {
                bSetActive = SetActiveSystem();
            }
        }

        WriteTrace(TraceN64System, TraceDebug, "Setting system as active");
        if (!m_Plugins->Reset(this) || !m_Plugins->initilized())
        {
            WriteTrace(TraceN64System, TraceWarning, "Can't run, plugins not initialized");
            g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
            g_Notify->DisplayError(MSG_PLUGIN_NOT_INIT);
        }
        else if (!bSetActive)
        {
            WriteTrace(TraceN64System, TraceWarning, "Failed to set system as active");
            g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
        }
        else
        {
            WriteTrace(TraceN64System, TraceDebug, "Starting emulation thread");
            StartEmulationThead();
        }
    }
    else
    {
        // Mark the emulation as starting and fix up menus
        g_Notify->DisplayMessage(2, MSG_EMULATION_STARTED);
        WriteTrace(TraceN64System, TraceDebug, "Start executing CPU");
        ExecuteCPU();
    }
    WriteTrace(TraceN64System, TraceDebug, "Done");
}

void CN64System::StartEmulation(bool NewThread)
{
    WriteTrace(TraceN64System, TraceDebug, "Start (NewThread: %s)", NewThread ? "true" : "false");
    __except_try()
    {
        StartEmulation2(NewThread);
    }
    __except_catch()
    {
        char message[400];
        sprintf(message, "Exception caught\nFile: %s\nLine: %d", __FILE__, __LINE__);
        g_Notify->DisplayError(message);
    }
    WriteTrace(TraceN64System, TraceDebug, "Done (NewThread: %s)", NewThread ? "true" : "false")
}

void CN64System::EndEmulation(void)
{
    m_EndEmulation = true;
}

void CN64System::Pause()
{
    if (m_EndEmulation)
    {
        return;
    }
    PauseType pause_type = (PauseType)g_Settings->LoadDword(GameRunning_CPU_PausedType);
    m_hPauseEvent.Reset();
    g_Settings->SaveBool(GameRunning_CPU_Paused, true);
    if (pause_type == PauseType_FromMenu)
    {
        g_Notify->DisplayMessage(5, MSG_CPU_PAUSED);
    }
    m_hPauseEvent.IsTriggered(SyncEvent::INFINITE_TIMEOUT);
    m_hPauseEvent.Reset();
    g_Settings->SaveBool(GameRunning_CPU_Paused, (uint32_t)false);
    if (pause_type == PauseType_FromMenu)
    {
        g_Notify->DisplayMessage(2, MSG_CPU_RESUMED);
    }
}

void CN64System::GameReset()
{
    m_SystemTimer.SetTimer(CSystemTimer::SoftResetTimer, 0x3000000, false);
    m_Plugins->Gfx()->ShowCFB();
    m_Reg.FAKE_CAUSE_REGISTER |= CAUSE_IP4;
    m_Plugins->Gfx()->SoftReset();
    if (m_SyncCPU)
    {
        m_SyncCPU->GameReset();
    }
}

void CN64System::PluginReset()
{
    if (!m_Plugins->ResetInUiThread(this))
    {
        g_Notify->DisplayMessage(5, MSG_PLUGIN_NOT_INIT);
        if (g_BaseSystem)
        {
            g_BaseSystem->m_EndEmulation = true;
        }
    }
    if (m_SyncCPU)
    {
        if (!m_SyncCPU->m_Plugins->ResetInUiThread(m_SyncCPU))
        {
            g_Notify->DisplayMessage(5, MSG_PLUGIN_NOT_INIT);
            if (g_BaseSystem)
            {
                g_BaseSystem->m_EndEmulation = true;
            }
        }
    }
    if (m_Recomp)
    {
        m_Recomp->Reset();
    }
    m_Plugins->RomOpened();
    if (m_SyncCPU)
    {
        m_SyncCPU->m_Plugins->RomOpened();
    }
#ifdef _WIN32
    _controlfp(_PC_53, _MCW_PC);
#endif
}

void CN64System::ApplyGSButton(void)
{
    if ((m_Reg.STATUS_REGISTER & STATUS_IE) != 0)
    {
        g_Enhancements->ApplyGSButton(m_MMU_VM, !m_SyncSystem);
    }
}

void CN64System::Reset(bool bInitReg, bool ClearMenory)
{
    WriteTrace(TraceN64System, TraceDebug, "Start (bInitReg: %s, ClearMenory: %s)", bInitReg ? "true" : "false", ClearMenory ? "true" : "false");
    g_Settings->SaveBool(GameRunning_InReset, true);
    RefreshGameSettings();
    m_Audio.Reset();
    m_MMU_VM.Reset(ClearMenory);

    m_CyclesToSkip = 0;
    m_AlistCount = 0;
    m_DlistCount = 0;
    m_UnknownCount = 0;
    m_DMAUsed = false;
    m_RspBroke = true;
    m_SyncCount = 0;

    for (int i = 0, n = (sizeof(m_LastSuccessSyncPC) / sizeof(m_LastSuccessSyncPC[0])); i < n; i++)
    {
        m_LastSuccessSyncPC[i] = 0;
    }

    if (bInitReg)
    {
        bool PostPif = true;

        InitRegisters(PostPif, m_MMU_VM);
        if (PostPif)
        {
            memcpy((m_MMU_VM.Dmem() + 0x40), (g_Rom->GetRomAddress() + 0x040), 0xFBC);
        }
    }
    else
    {
        m_Reg.Reset();
    }

    m_SystemTimer.Reset();
    m_SystemTimer.SetTimer(CSystemTimer::CompareTimer, m_Reg.COMPARE_REGISTER - m_Reg.COUNT_REGISTER, false);

    if (m_Recomp)
    {
        m_Recomp->Reset();
    }
    if (m_Plugins && g_Settings->LoadBool(GameRunning_CPU_Running))
    {
        m_Plugins->RomClosed();
        m_Plugins->RomOpened();
    }
    if (m_SyncCPU && m_SyncCPU->m_MMU_VM.Rdram() != nullptr)
    {
        m_SyncCPU->Reset(bInitReg, ClearMenory);
    }
    g_Settings->SaveBool(GameRunning_InReset, false);

    WriteTrace(TraceN64System, TraceDebug, "Done");
}

bool CN64System::SetActiveSystem(bool bActive)
{
    bool bRes = true;

    if (bActive && g_System == this)
    {
        WriteTrace(TraceN64System, TraceDebug, "Done (Res: true)");
        return true;
    }

    if (bActive)
    {
        m_Reg.SetAsCurrentSystem();

        if (g_System)
        {
            g_System->m_TestTimer = R4300iOp::m_TestTimer;
            g_System->m_NextInstruction = R4300iOp::m_NextInstruction;
            g_System->m_JumpToLocation = R4300iOp::m_JumpToLocation;
        }

        g_System = this;
        if (g_BaseSystem == this)
        {
            g_SyncSystem = m_SyncCPU;
        }
        g_Recompiler = m_Recomp;
        g_MMU = &m_MMU_VM;
        g_TLB = &m_TLB;
        g_Reg = &m_Reg;
        g_Mempak = &m_Mempak;
        g_Audio = &m_Audio;
        g_SystemTimer = &m_SystemTimer;
        g_TransVaddr = &m_MMU_VM;
        g_SystemEvents = this;
        g_NextTimer = &m_NextTimer;
        g_Plugins = m_Plugins;
        g_TLBLoadAddress = &m_TLBLoadAddress;
        g_TLBStoreAddress = &m_TLBStoreAddress;
        g_RecompPos = m_Recomp ? m_Recomp->RecompPos() : nullptr;
        R4300iOp::m_TestTimer = m_TestTimer;
        R4300iOp::m_NextInstruction = m_NextInstruction;
        R4300iOp::m_JumpToLocation = m_JumpToLocation;
        g_Random = &m_Random;
    }
    else
    {
        if (this == g_BaseSystem)
        {
            g_System = nullptr;
            g_SyncSystem = nullptr;
            g_Recompiler = nullptr;
            g_MMU = nullptr;
            g_TLB = nullptr;
            g_Reg = nullptr;
            g_Audio = nullptr;
            g_SystemTimer = nullptr;
            g_TransVaddr = nullptr;
            g_SystemEvents = nullptr;
            g_NextTimer = nullptr;
            g_Plugins = m_Plugins;
            g_TLBLoadAddress = nullptr;
            g_TLBStoreAddress = nullptr;
            g_Random = nullptr;
        }
    }

    return bRes;
}

void CN64System::InitRegisters(bool bPostPif, CMipsMemoryVM & MMU)
{
    m_Reg.Reset();

    // COP0 registers
    m_Reg.RANDOM_REGISTER = 0x1F;
    m_Reg.COUNT_REGISTER = 0x5000;
    m_Reg.MI_VERSION_REG = 0x02020102;
    m_Reg.SP_STATUS_REG = 0x00000001;
    m_Reg.CAUSE_REGISTER = 0x0000005C;
    m_Reg.CONTEXT_REGISTER = 0x007FFFF0;
    m_Reg.EPC_REGISTER = 0xFFFFFFFF;
    m_Reg.BAD_VADDR_REGISTER = 0xFFFFFFFF;
    m_Reg.ERROREPC_REGISTER = 0xFFFFFFFF;
    m_Reg.CONFIG_REGISTER = 0x0006E463;
    m_Reg.STATUS_REGISTER = 0x34000000;

    // N64DD registers

    // Start N64DD in reset state and motor not spinning
    m_Reg.ASIC_STATUS = DD_STATUS_RST_STATE | DD_STATUS_MTR_N_SPIN;
    m_Reg.ASIC_ID_REG = 0x00030000;
    if (g_DDRom && (g_DDRom->CicChipID() == CIC_NUS_DDTL || (g_Disk && g_Disk->GetCountry() == Country_Unknown)))
        m_Reg.ASIC_ID_REG = 0x00040000;

    //m_Reg.REVISION_REGISTER   = 0x00000511;
    m_Reg.FixFpuLocations();

    if (bPostPif)
    {
        m_Reg.m_PROGRAM_COUNTER = 0xA4000040;

        m_Reg.m_GPR[0].DW = 0x0000000000000000;
        m_Reg.m_GPR[6].DW = 0xFFFFFFFFA4001F0C;
        m_Reg.m_GPR[7].DW = 0xFFFFFFFFA4001F08;
        m_Reg.m_GPR[8].DW = 0x00000000000000C0;
        m_Reg.m_GPR[9].DW = 0x0000000000000000;
        m_Reg.m_GPR[10].DW = 0x0000000000000040;
        m_Reg.m_GPR[11].DW = 0xFFFFFFFFA4000040;
        m_Reg.m_GPR[16].DW = 0x0000000000000000;
        m_Reg.m_GPR[17].DW = 0x0000000000000000;
        m_Reg.m_GPR[18].DW = 0x0000000000000000;
        m_Reg.m_GPR[19].DW = 0x0000000000000000;
        m_Reg.m_GPR[21].DW = 0x0000000000000000;
        m_Reg.m_GPR[26].DW = 0x0000000000000000;
        m_Reg.m_GPR[27].DW = 0x0000000000000000;
        m_Reg.m_GPR[28].DW = 0x0000000000000000;
        m_Reg.m_GPR[29].DW = 0xFFFFFFFFA4001FF0;
        m_Reg.m_GPR[30].DW = 0x0000000000000000;

        if (g_Rom->IsPal())
        {
            switch (g_Rom->CicChipID())
            {
            case CIC_UNKNOWN:
            case CIC_NUS_6102:
                m_Reg.m_GPR[5].DW = 0xFFFFFFFFC0F1D859;
                m_Reg.m_GPR[14].DW = 0x000000002DE108EA;
                m_Reg.m_GPR[24].DW = 0x0000000000000000;
                break;
            case CIC_NUS_6103:
                m_Reg.m_GPR[5].DW = 0xFFFFFFFFD4646273;
                m_Reg.m_GPR[14].DW = 0x000000001AF99984;
                m_Reg.m_GPR[24].DW = 0x0000000000000000;
                break;
            case CIC_NUS_6105:
                MMU.SW_VAddr(0xA4001004, 0xBDA807FC);
                m_Reg.m_GPR[5].DW = 0xFFFFFFFFDECAAAD1;
                m_Reg.m_GPR[14].DW = 0x000000000CF85C13;
                m_Reg.m_GPR[24].DW = 0x0000000000000002;
                break;
            case CIC_NUS_6106:
                m_Reg.m_GPR[5].DW = 0xFFFFFFFFB04DC903;
                m_Reg.m_GPR[14].DW = 0x000000001AF99984;
                m_Reg.m_GPR[24].DW = 0x0000000000000002;
                break;
            }
            m_Reg.m_GPR[20].DW = 0x0000000000000000;
            m_Reg.m_GPR[23].DW = 0x0000000000000006;
            m_Reg.m_GPR[31].DW = 0xFFFFFFFFA4001554;
        }
        else
        {
            switch (g_Rom->CicChipID())
            {
            case CIC_UNKNOWN:
            case CIC_NUS_6102:
                m_Reg.m_GPR[5].DW = 0xFFFFFFFFC95973D5;
                m_Reg.m_GPR[14].DW = 0x000000002449A366;
                break;
            case CIC_NUS_6103:
                m_Reg.m_GPR[5].DW = 0xFFFFFFFF95315A28;
                m_Reg.m_GPR[14].DW = 0x000000005BACA1DF;
                break;
            case CIC_NUS_6105:
                MMU.SW_VAddr(0xA4001004, 0x8DA807FC);
                m_Reg.m_GPR[5].DW = 0x000000005493FB9A;
                m_Reg.m_GPR[14].DW = 0xFFFFFFFFC2C20384;
            case CIC_NUS_6106:
                m_Reg.m_GPR[5].DW = 0xFFFFFFFFE067221F;
                m_Reg.m_GPR[14].DW = 0x000000005CD2B70F;
                break;
            case CIC_NUS_6101:
            case CIC_NUS_6104:
            case CIC_NUS_5167:
            case CIC_NUS_8303:
            case CIC_NUS_DDUS:
            case CIC_NUS_DDTL:
            case CIC_NUS_5101:
            default:
                // No specific values
                break;
            }
            m_Reg.m_GPR[20].DW = 0x0000000000000001;
            m_Reg.m_GPR[23].DW = 0x0000000000000000;
            m_Reg.m_GPR[24].DW = 0x0000000000000003;
            m_Reg.m_GPR[31].DW = 0xFFFFFFFFA4001550;
        }

        switch (g_Rom->CicChipID())
        {
        case CIC_NUS_6101:
            m_Reg.m_GPR[22].DW = 0x000000000000003F;
            break;
        case CIC_NUS_8303:        // 64DD IPL CIC
        case CIC_NUS_DDTL:        // 64DD IPL tool CIC
        case CIC_NUS_5167:        // 64DD conversion CIC
            m_Reg.m_GPR[22].DW = 0x00000000000000DD;
            break;
        case CIC_NUS_DDUS:        // 64DD US IPL CIC
            m_Reg.m_GPR[22].DW = 0x00000000000000DE;
            break;
        case CIC_NUS_5101:        // Aleck64 CIC
            m_Reg.m_GPR[22].DW = 0x00000000000000AC;
            break;
        case CIC_UNKNOWN:
        case CIC_NUS_6102:
            m_Reg.m_GPR[1].DW = 0x0000000000000001;
            m_Reg.m_GPR[2].DW = 0x000000000EBDA536;
            m_Reg.m_GPR[3].DW = 0x000000000EBDA536;
            m_Reg.m_GPR[4].DW = 0x000000000000A536;
            m_Reg.m_GPR[12].DW = 0xFFFFFFFFED10D0B3;
            m_Reg.m_GPR[13].DW = 0x000000001402A4CC;
            m_Reg.m_GPR[15].DW = 0x000000003103E121;
            m_Reg.m_GPR[22].DW = 0x000000000000003F;
            m_Reg.m_GPR[25].DW = 0xFFFFFFFF9DEBB54F;
            break;
        case CIC_NUS_6103:
            m_Reg.m_GPR[1].DW = 0x0000000000000001;
            m_Reg.m_GPR[2].DW = 0x0000000049A5EE96;
            m_Reg.m_GPR[3].DW = 0x0000000049A5EE96;
            m_Reg.m_GPR[4].DW = 0x000000000000EE96;
            m_Reg.m_GPR[12].DW = 0xFFFFFFFFCE9DFBF7;
            m_Reg.m_GPR[13].DW = 0xFFFFFFFFCE9DFBF7;
            m_Reg.m_GPR[15].DW = 0x0000000018B63D28;
            m_Reg.m_GPR[22].DW = 0x0000000000000078;
            m_Reg.m_GPR[25].DW = 0xFFFFFFFF825B21C9;
            break;
        case CIC_NUS_6105:
            MMU.SW_VAddr(0xA4001000, 0x3C0DBFC0);
            MMU.SW_VAddr(0xA4001008, 0x25AD07C0);
            MMU.SW_VAddr(0xA400100C, 0x31080080);
            MMU.SW_VAddr(0xA4001010, 0x5500FFFC);
            MMU.SW_VAddr(0xA4001014, 0x3C0DBFC0);
            MMU.SW_VAddr(0xA4001018, 0x8DA80024);
            MMU.SW_VAddr(0xA400101C, 0x3C0BB000);
            m_Reg.m_GPR[1].DW = 0x0000000000000000;
            m_Reg.m_GPR[2].DW = 0xFFFFFFFFF58B0FBF;
            m_Reg.m_GPR[3].DW = 0xFFFFFFFFF58B0FBF;
            m_Reg.m_GPR[4].DW = 0x0000000000000FBF;
            m_Reg.m_GPR[12].DW = 0xFFFFFFFF9651F81E;
            m_Reg.m_GPR[13].DW = 0x000000002D42AAC5;
            m_Reg.m_GPR[15].DW = 0x0000000056584D60;
            m_Reg.m_GPR[22].DW = 0x0000000000000091;
            m_Reg.m_GPR[25].DW = 0xFFFFFFFFCDCE565F;
            break;
        case CIC_NUS_6106:
            m_Reg.m_GPR[1].DW = 0x0000000000000000;
            m_Reg.m_GPR[2].DW = 0xFFFFFFFFA95930A4;
            m_Reg.m_GPR[3].DW = 0xFFFFFFFFA95930A4;
            m_Reg.m_GPR[4].DW = 0x00000000000030A4;
            m_Reg.m_GPR[12].DW = 0xFFFFFFFFBCB59510;
            m_Reg.m_GPR[13].DW = 0xFFFFFFFFBCB59510;
            m_Reg.m_GPR[15].DW = 0x000000007A3C07F4;
            m_Reg.m_GPR[22].DW = 0x0000000000000085;
            m_Reg.m_GPR[25].DW = 0x00000000465E3F72;
            break;
        }
    }
    else
    {
        m_Reg.m_PROGRAM_COUNTER = 0xBFC00000;
        /*        PIF_Ram[36] = 0x00; PIF_Ram[39] = 0x3F; // Common PIF RAM start values

        switch (g_Rom->CicChipID()) {
        case CIC_NUS_6101: PIF_Ram[37] = 0x06; PIF_Ram[38] = 0x3F; break;
        case CIC_UNKNOWN:
        case CIC_NUS_6102: PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x3F; break;
        case CIC_NUS_6103:    PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x78; break;
        case CIC_NUS_6105:    PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x91; break;
        case CIC_NUS_6106:    PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x85; break;
        }*/
    }
}

void CN64System::ExecuteCPU()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");

    // Reset code
    g_Settings->SaveBool(GameRunning_CPU_Paused, false);
    g_Settings->SaveBool(GameRunning_CPU_Running, true);
    g_Notify->DisplayMessage(2, MSG_EMULATION_STARTED);

    m_EndEmulation = false;

    m_Plugins->RomOpened();
    if (m_SyncCPU)
    {
        m_SyncCPU->m_Plugins->RomOpened();
    }

    if (g_Debugger != nullptr && HaveDebugger())
    {
        g_Debugger->EmulationStarted();
    }
#ifdef _WIN32
    _controlfp(_PC_53, _MCW_PC);
#endif

    CPU_TYPE cpuType;

    if (g_Settings->LoadBool(Setting_ForceInterpreterCPU))
    {
        cpuType = CPU_Interpreter;
    }
    else
    {
        cpuType = (CPU_TYPE)g_Settings->LoadDword(Game_CpuType);
    }

    switch (cpuType)
    {
    case CPU_Recompiler: ExecuteRecompiler(); break;
    case CPU_SyncCores:  ExecuteSyncCPU();    break;
    default:             ExecuteInterpret();  break;
    }
    WriteTrace(TraceN64System, TraceDebug, "CPU finished executing");
    CpuStopped();
    WriteTrace(TraceN64System, TraceDebug, "Notifying plugins ROM is done");
    m_Plugins->RomClosed();
    if (m_SyncCPU)
    {
        m_SyncCPU->m_Plugins->RomClosed();
    }

    if (g_Debugger != nullptr && HaveDebugger())
    {
        g_Debugger->EmulationStopped();
    }

    WriteTrace(TraceN64System, TraceDebug, "Done");
}

void CN64System::ExecuteInterpret()
{
    SetActiveSystem();
    CInterpreterCPU::ExecuteCPU();
}

void CN64System::ExecuteRecompiler()
{
    m_Recomp->Run();
}

void CN64System::ExecuteSyncCPU()
{
    m_Recomp->Run();
}

void CN64System::CpuStopped()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");
    if (!m_InReset)
    {
        g_Settings->SaveBool(GameRunning_CPU_Running, (uint32_t)false);
        g_Notify->DisplayMessage(5, MSG_EMULATION_ENDED);
    }
    if (m_SyncCPU)
    {
        m_SyncCPU->CpuStopped();
    }
    WriteTrace(TraceN64System, TraceDebug, "Done");
}

void CN64System::UpdateSyncCPU(CN64System * const SecondCPU, uint32_t const Cycles)
{
    int CyclesToExecute = Cycles - m_CyclesToSkip;

    // Update the number of cycles to skip
    m_CyclesToSkip -= Cycles;
    if (m_CyclesToSkip < 0) { m_CyclesToSkip = 0; }

    // Run the other CPU For the same amount of cycles
    if (CyclesToExecute < 0) { return; }

    SecondCPU->SetActiveSystem(true);

    CInterpreterCPU::ExecuteOps(Cycles);

    SetActiveSystem(true);
}

void CN64System::SyncCPUPC(CN64System * const SecondCPU)
{
    bool ErrorFound = false;

    g_SystemTimer->UpdateTimers();
    if (m_Reg.m_PROGRAM_COUNTER != SecondCPU->m_Reg.m_PROGRAM_COUNTER)
    {
        ErrorFound = true;
    }

    if (m_TLB != SecondCPU->m_TLB) { ErrorFound = true; }
    if (m_SystemTimer != SecondCPU->m_SystemTimer) { ErrorFound = true; }
    if (m_NextTimer != SecondCPU->m_NextTimer) { ErrorFound = true; }

    if (ErrorFound) { DumpSyncErrors(SecondCPU); }

    for (int i = (sizeof(m_LastSuccessSyncPC) / sizeof(m_LastSuccessSyncPC[0])) - 1; i > 0; i--)
    {
        m_LastSuccessSyncPC[i] = m_LastSuccessSyncPC[i - 1];
    }
    m_LastSuccessSyncPC[0] = m_Reg.m_PROGRAM_COUNTER;
}

void CN64System::SyncCPU(CN64System * const SecondCPU)
{
    bool ErrorFound = false;

    m_SyncCount += 1;
    g_SystemTimer->UpdateTimers();

#ifdef TEST_SP_TRACKING
    if (m_CurrentSP != GPR[29].UW[0]) {
        ErrorFound = true;
    }
#endif
    if (m_Reg.m_PROGRAM_COUNTER != SecondCPU->m_Reg.m_PROGRAM_COUNTER)
    {
        ErrorFound = true;
    }
    if (b32BitCore())
    {
        for (int count = 0; count < 32; count++)
        {
            if (m_Reg.m_GPR[count].W[0] != SecondCPU->m_Reg.m_GPR[count].W[0])
            {
                ErrorFound = true;
            }
            if (m_Reg.m_FPR[count].DW != SecondCPU->m_Reg.m_FPR[count].DW)
            {
                ErrorFound = true;
            }
            if (m_Reg.m_CP0[count] != SecondCPU->m_Reg.m_CP0[count])
            {
                ErrorFound = true;
            }
        }
    }
    else
    {
        for (int count = 0; count < 32; count++)
        {
            if (m_Reg.m_GPR[count].DW != SecondCPU->m_Reg.m_GPR[count].DW)
            {
                ErrorFound = true;
            }
            if (m_Reg.m_FPR[count].DW != SecondCPU->m_Reg.m_FPR[count].DW)
            {
                ErrorFound = true;
            }
            if (m_Reg.m_CP0[count] != SecondCPU->m_Reg.m_CP0[count])
            {
                ErrorFound = true;
            }
        }
    }

    if (m_Random.get_state() != SecondCPU->m_Random.get_state()) 
    {
        ErrorFound = true; 
    }
    if (m_TLB != SecondCPU->m_TLB) { ErrorFound = true; }
    if (m_Reg.m_FPCR[0] != SecondCPU->m_Reg.m_FPCR[0]) { ErrorFound = true; }
    if (m_Reg.m_FPCR[31] != SecondCPU->m_Reg.m_FPCR[31]) { ErrorFound = true; }
    if (m_Reg.m_HI.DW != SecondCPU->m_Reg.m_HI.DW) { ErrorFound = true; }
    if (m_Reg.m_LO.DW != SecondCPU->m_Reg.m_LO.DW) { ErrorFound = true; }
    /*if (m_SyncCount > 4788000)
    {
    if (memcmp(m_MMU_VM.Rdram(),SecondCPU->m_MMU_VM.Rdram(),RdramSize()) != 0)
    {
    ErrorFound = true;
    }
    }
    if (memcmp(m_MMU_VM.Imem(),SecondCPU->m_MMU_VM.Imem(),0x1000) != 0)
    {
    ErrorFound = true;
    }
    if (memcmp(m_MMU_VM.Dmem(),SecondCPU->m_MMU_VM.Dmem(),0x1000) != 0)
    {
    ErrorFound = true;
    }*/

    /*for (int z = 0; z < 0x100; z++)
    {
    if (m_MMU_VM.Rdram()[0x00206970 + z] !=  SecondCPU->m_MMU_VM.Rdram()[0x00206970 + z])
    {
    ErrorFound = true;
    break;
    }
    }*/

    if (bFastSP() && m_Recomp)
    {
#if defined(__aarch64__) || defined(__amd64__)
        g_Notify->BreakPoint(__FILE__,__LINE__);
#else
        if (m_Recomp->MemoryStackPos() != (uint32_t)(m_MMU_VM.Rdram() + (m_Reg.m_GPR[29].W[0] & 0x1FFFFFFF)))
        {
            ErrorFound = true;
        }
#endif
    }

    if (m_SystemTimer != SecondCPU->m_SystemTimer) { ErrorFound = true; }
    if (m_NextTimer != SecondCPU->m_NextTimer) { ErrorFound = true; }
    if (m_Reg.m_RoundingModel != SecondCPU->m_Reg.m_RoundingModel) { ErrorFound = true; }

    for (int i = 0, n = sizeof(m_Reg.m_Mips_Interface) / sizeof(m_Reg.m_Mips_Interface[0]); i < n; i++)
    {
        if (m_Reg.m_Mips_Interface[i] != SecondCPU->m_Reg.m_Mips_Interface[i])
        {
            ErrorFound = true;
        }
    }

    for (int i = 0, n = sizeof(m_Reg.m_SigProcessor_Interface) / sizeof(m_Reg.m_SigProcessor_Interface[0]); i < n; i++)
    {
        if (m_Reg.m_SigProcessor_Interface[i] != SecondCPU->m_Reg.m_SigProcessor_Interface[i])
        {
            ErrorFound = true;
        }
    }

    for (int i = 0, n = sizeof(m_Reg.m_Display_ControlReg) / sizeof(m_Reg.m_Display_ControlReg[0]); i < n; i++)
    {
        if (m_Reg.m_Display_ControlReg[i] != SecondCPU->m_Reg.m_Display_ControlReg[i])
        {
            ErrorFound = true;
        }
    }

    if (ErrorFound) { DumpSyncErrors(SecondCPU); }

    for (int i = (sizeof(m_LastSuccessSyncPC) / sizeof(m_LastSuccessSyncPC[0])) - 1; i > 0; i--)
    {
        m_LastSuccessSyncPC[i] = m_LastSuccessSyncPC[i - 1];
    }
    m_LastSuccessSyncPC[0] = m_Reg.m_PROGRAM_COUNTER;
}

void CN64System::SyncSystem()
{
    SyncCPU(g_SyncSystem);
}

void CN64System::SyncSystemPC()
{
    SyncCPUPC(g_SyncSystem);
}

void CN64System::DumpSyncErrors(CN64System * SecondCPU)
{
    int count;

    {
        CPath ErrorFile(g_Settings->LoadStringVal(Directory_Log).c_str(), "Sync Errors.txt");

        CLog Error;
        Error.Open(ErrorFile);
        Error.Log("Errors:\r\n");
        Error.Log("Register,        Recompiler,         Interpreter\r\n");
#ifdef TEST_SP_TRACKING
        if (m_CurrentSP != GPR[29].UW[0])
        {
            Error.Log("m_CurrentSP,%X,%X\r\n", m_CurrentSP, GPR[29].UW[0]);
        }
#endif
        if (m_Reg.m_PROGRAM_COUNTER != SecondCPU->m_Reg.m_PROGRAM_COUNTER)
        {
            Error.LogF("PROGRAM_COUNTER 0x%X,         0x%X\r\n", m_Reg.m_PROGRAM_COUNTER, SecondCPU->m_Reg.m_PROGRAM_COUNTER);
        }
        if (b32BitCore())
        {
            for (count = 0; count < 32; count++)
            {
                if (m_Reg.m_GPR[count].UW[0] != SecondCPU->m_Reg.m_GPR[count].UW[0])
                {
                    Error.LogF("GPR[%s] 0x%08X%08X, 0x%08X%08X\r\n", CRegName::GPR[count],
                        m_Reg.m_GPR[count].W[1], m_Reg.m_GPR[count].W[0],
                        SecondCPU->m_Reg.m_GPR[count].W[1], SecondCPU->m_Reg.m_GPR[count].W[0]);
                }
            }
        }
        else
        {
            for (count = 0; count < 32; count++)
            {
                if (m_Reg.m_GPR[count].DW != SecondCPU->m_Reg.m_GPR[count].DW)
                {
                    Error.LogF("GPR[%s] 0x%08X%08X, 0x%08X%08X\r\n", CRegName::GPR[count],
                        m_Reg.m_GPR[count].W[1], m_Reg.m_GPR[count].W[0],
                        SecondCPU->m_Reg.m_GPR[count].W[1], SecondCPU->m_Reg.m_GPR[count].W[0]);
                }
            }
        }
        for (count = 0; count < 32; count++)
        {
            if (m_Reg.m_FPR[count].DW != SecondCPU->m_Reg.m_FPR[count].DW)
            {
                Error.LogF("FPR[%s] 0x%08X%08X, 0x%08X%08X\r\n", CRegName::FPR[count],
                    m_Reg.m_FPR[count].W[1], m_Reg.m_FPR[count].W[0],
                    SecondCPU->m_Reg.m_FPR[count].W[1], SecondCPU->m_Reg.m_FPR[count].W[0]);
            }
        }
        for (count = 0; count < 32; count++)
        {
            if (m_Reg.m_FPCR[count] != SecondCPU->m_Reg.m_FPCR[count])
            {
                Error.LogF("FPCR[%s] 0x%08X, 0x%08X\r\n", CRegName::FPR_Ctrl[count],
                    m_Reg.m_FPCR[count], SecondCPU->m_Reg.m_FPCR[count]);
            }
        }
        for (count = 0; count < 32; count++)
        {
            if (m_Reg.m_CP0[count] != SecondCPU->m_Reg.m_CP0[count])
            {
                Error.LogF("CP0[%s] 0x%08X, 0x%08X\r\n", CRegName::Cop0[count],
                    m_Reg.m_CP0[count], SecondCPU->m_Reg.m_CP0[count]);
            }
        }
        if (m_Reg.m_HI.DW != SecondCPU->m_Reg.m_HI.DW)
        {
            Error.LogF("HI Reg 0x%08X%08X, 0x%08X%08X\r\n", m_Reg.m_HI.UW[1], m_Reg.m_HI.UW[0], SecondCPU->m_Reg.m_HI.UW[1], SecondCPU->m_Reg.m_HI.UW[0]);
        }
        if (m_Reg.m_LO.DW != SecondCPU->m_Reg.m_LO.DW)
        {
            Error.LogF("LO Reg 0x%08X%08X, 0x%08X%08X\r\n", m_Reg.m_LO.UW[1], m_Reg.m_LO.UW[0], SecondCPU->m_Reg.m_LO.UW[1], SecondCPU->m_Reg.m_LO.UW[0]);
        }
        for (int i = 0, n = sizeof(m_Reg.m_Mips_Interface) / sizeof(m_Reg.m_Mips_Interface[0]); i < n; i++)
        {
            if (m_Reg.m_Mips_Interface[i] != SecondCPU->m_Reg.m_Mips_Interface[i])
            {
                Error.LogF("Mips_Interface[%d] 0x%08X, 0x%08X\r\n", i, m_Reg.m_Mips_Interface[i], SecondCPU->m_Reg.m_Mips_Interface[i]);
            }
        }

        for (int i = 0, n = sizeof(m_Reg.m_SigProcessor_Interface) / sizeof(m_Reg.m_SigProcessor_Interface[0]); i < n; i++)
        {
            if (m_Reg.m_SigProcessor_Interface[i] != SecondCPU->m_Reg.m_SigProcessor_Interface[i])
            {
                Error.LogF("SigProcessor_Interface[%d] 0x%08X, 0x%08X\r\n", i, m_Reg.m_SigProcessor_Interface[i], SecondCPU->m_Reg.m_SigProcessor_Interface[i]);
            }
        }
        for (int i = 0, n = sizeof(m_Reg.m_Display_ControlReg) / sizeof(m_Reg.m_Display_ControlReg[0]); i < n; i++)
        {
            if (m_Reg.m_Display_ControlReg[i] != SecondCPU->m_Reg.m_Display_ControlReg[i])
            {
                Error.LogF("Display_ControlReg[%d] 0x%08X, 0x%08X\r\n", i, m_Reg.m_Display_ControlReg[i], SecondCPU->m_Reg.m_Display_ControlReg[i]);
            }
        }

        if (m_NextTimer != SecondCPU->m_NextTimer)
        {
            Error.LogF("Current Time: %X %X\r\n", (uint32_t)m_NextTimer, (uint32_t)SecondCPU->m_NextTimer);
        }
        m_TLB.RecordDifference(Error, SecondCPU->m_TLB);
        m_SystemTimer.RecordDifference(Error, SecondCPU->m_SystemTimer);
        if (m_Reg.m_RoundingModel != SecondCPU->m_Reg.m_RoundingModel)
        {
            Error.LogF("RoundingModel: %X %X\r\n", m_Reg.m_RoundingModel, SecondCPU->m_Reg.m_RoundingModel);
        }
        if (bFastSP() && m_Recomp)
        {
#if defined(__aarch64__) || defined(__amd64__)
            g_Notify->BreakPoint(__FILE__,__LINE__);
#else
            if (m_Recomp->MemoryStackPos() != (uint32_t)(m_MMU_VM.Rdram() + (m_Reg.m_GPR[29].W[0] & 0x1FFFFFFF)))
            {
                Error.LogF("MemoryStack = %X  should be: %X\r\n", m_Recomp->MemoryStackPos(), (uint32_t)(m_MMU_VM.Rdram() + (m_Reg.m_GPR[29].W[0] & 0x1FFFFFFF)));
            }
#endif
        }

        uint32_t * Rdram = (uint32_t *)m_MMU_VM.Rdram(), *Rdram2 = (uint32_t *)SecondCPU->m_MMU_VM.Rdram();
        for (int z = 0, n = (RdramSize() >> 2); z < n; z++)
        {
            if (Rdram[z] != Rdram2[z])
            {
                Error.LogF("RDRAM[%X]: %X %X\r\n", z << 2, Rdram[z], Rdram2[z]);
            }
        }

        uint32_t * Imem = (uint32_t *)m_MMU_VM.Imem(), *Imem2 = (uint32_t *)SecondCPU->m_MMU_VM.Imem();
        for (int z = 0; z < (0x1000 >> 2); z++)
        {
            if (Imem[z] != Imem2[z])
            {
                Error.LogF("IMEM[%X]: %X %X\r\n", z << 2, Imem[z], Imem2[z]);
            }
        }
        uint32_t * Dmem = (uint32_t *)m_MMU_VM.Dmem(), *Dmem2 = (uint32_t *)SecondCPU->m_MMU_VM.Dmem();
        for (int z = 0; z < (0x1000 >> 2); z++)
        {
            if (Dmem[z] != Dmem2[z])
            {
                Error.LogF("DMEM[%X]: %X %X\r\n", z << 2, Dmem[z], Dmem2[z]);
            }
        }
        Error.Log("\r\n");
        Error.Log("Information:\r\n");
        Error.Log("\r\n");
        Error.LogF("PROGRAM_COUNTER,0x%X\r\n", m_Reg.m_PROGRAM_COUNTER);
        Error.LogF("Current timer,0x%X\r\n", m_NextTimer);
        Error.LogF("Timer type,0x%X\r\n", m_SystemTimer.CurrentType());
        Error.Log("\r\n");
        for (int i = 0; i < (sizeof(m_LastSuccessSyncPC) / sizeof(m_LastSuccessSyncPC[0])); i++)
        {
            Error.LogF("LastSuccessSyncPC[%d],0x%X\r\n", i, m_LastSuccessSyncPC[i]);
        }
        Error.Log("\r\n");
        for (count = 0; count < 32; count++)
        {
            Error.LogF("GPR[%s],         0x%08X%08X, 0x%08X%08X\r\n", CRegName::GPR[count],
                m_Reg.m_GPR[count].W[1], m_Reg.m_GPR[count].W[0],
                SecondCPU->m_Reg.m_GPR[count].W[1], SecondCPU->m_Reg.m_GPR[count].W[0]);
        }
        Error.Log("\r\n");
        for (count = 0; count < 32; count++)
        {
            Error.LogF("FPR[%s],%*s0x%08X%08X, 0x%08X%08X\r\n", CRegName::FPR[count],
                count < 10 ? 9 : 8, " ", m_Reg.m_FPR[count].W[1], m_Reg.m_FPR[count].W[0],
                SecondCPU->m_Reg.m_FPR[count].W[1], SecondCPU->m_Reg.m_FPR[count].W[0]);
        }
        Error.Log("\r\n");
        for (count = 0; count < 32; count++)
        {
            Error.LogF("FPR_S[%s],%*s%f, %f\r\n", CRegName::FPR[count],
                count < 10 ? 7 : 6, " ", *(m_Reg.m_FPR_S[count]), *(SecondCPU->m_Reg.m_FPR_S[count]));
        }
        Error.Log("\r\n");
        for (count = 0; count < 32; count++)
        {
            Error.LogF("FPR_D[%s],%*s%f, %f\r\n", CRegName::FPR[count],
                count < 10 ? 7 : 6, " ", *(m_Reg.m_FPR_D[count]), *(SecondCPU->m_Reg.m_FPR_D[count]));
        }
        Error.Log("\r\n");
        Error.LogF("Rounding model,   0x%08X, 0x%08X\r\n", m_Reg.m_RoundingModel, SecondCPU->m_Reg.m_RoundingModel);
        Error.Log("\r\n");
        for (count = 0; count < 32; count++)
        {
            Error.LogF("CP0[%s],%*s0x%08X, 0x%08X\r\n", CRegName::Cop0[count],
                12 - strlen(CRegName::Cop0[count]), "",
                m_Reg.m_CP0[count], SecondCPU->m_Reg.m_CP0[count]);
        }
        Error.Log("\r\n");
        for (count = 0; count < 32; count++)
        {
            Error.LogF("FPR_Ctrl[%s],%*s0x%08X, 0x%08X\r\n", CRegName::FPR_Ctrl[count],
                12 - strlen(CRegName::FPR_Ctrl[count]), "",
                m_Reg.m_FPCR[count], SecondCPU->m_Reg.m_FPCR[count]);
        }
        Error.Log("\r\n");

        Error.LogF("HI                0x%08X%08X, 0x%08X%08X\r\n", m_Reg.m_HI.UW[1], m_Reg.m_HI.UW[0],
            SecondCPU->m_Reg.m_HI.UW[1], SecondCPU->m_Reg.m_HI.UW[0]);
        Error.LogF("LO                0x%08X%08X, 0x%08X%08X\r\n", m_Reg.m_LO.UW[1], m_Reg.m_LO.UW[0],
            SecondCPU->m_Reg.m_LO.UW[1], SecondCPU->m_Reg.m_LO.UW[0]);
        Error.LogF("CP0[%s],%*s0x%08X, 0x%08X\r\n", CRegName::Cop0[count],
            12 - strlen(CRegName::Cop0[count]), "",
            m_Reg.m_CP0[count], SecondCPU->m_Reg.m_CP0[count]);

        bool bHasTlb = false;
        for (count = 0; count < 32; count++)
        {
            if (!m_TLB.TlbEntry(count).EntryDefined) { continue; }
            if (!bHasTlb)
            {
                Error.Log("\r\n");
                Error.Log("         Hi Recomp, PageMask, Hi Interp, PageMask\r\n");
                bHasTlb = true;
            }
            Error.LogF("TLB[%2d], %08X,  %08X, %08X,  %08X\r\n", count,
                m_TLB.TlbEntry(count).EntryHi.Value, m_TLB.TlbEntry(count).PageMask.Value,
                SecondCPU->m_TLB.TlbEntry(count).EntryHi.Value, SecondCPU->m_TLB.TlbEntry(count).PageMask.Value
            );
        }
        Error.Log("\r\n");
        Error.Log("Code at PC:\r\n");
        for (count = -10; count < 10; count++)
        {
            uint32_t OpcodeValue, Addr = m_Reg.m_PROGRAM_COUNTER + (count << 2);
            if (g_MMU->LW_VAddr(Addr, OpcodeValue))
            {
                Error.LogF("%X: %s\r\n", Addr, R4300iOpcodeName(OpcodeValue, Addr));
            }
        }
        Error.Log("\r\n");
        Error.Log("Code at last sync PC:\r\n");
        for (count = 0; count < 50; count++)
        {
            uint32_t OpcodeValue, Addr = m_LastSuccessSyncPC[0] + (count << 2);
            if (g_MMU->LW_VAddr(Addr, OpcodeValue))
            {
                Error.LogF("%X: %s\r\n", Addr, R4300iOpcodeName(OpcodeValue, Addr));
            }
        }
    }

    g_Notify->DisplayError("Sync error");
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

bool CN64System::SaveState()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");

    //    if (!m_SystemTimer.SaveAllowed()) { return false; }
    if ((m_Reg.STATUS_REGISTER & STATUS_EXL) != 0)
    {
        WriteTrace(TraceN64System, TraceDebug, "Done - STATUS_EXL set, can't save");
        return false;
    }

    CPath SaveFile(g_Settings->LoadStringVal(GameRunning_InstantSaveFile));
    int Slot = 0;
    if (((const std::string &)SaveFile).empty())
    {
        Slot = g_Settings->LoadDword(Game_CurrentSaveState);
        SaveFile = CPath(g_Settings->LoadStringVal(Directory_InstantSave).c_str(), "");
        if (g_Settings->LoadBool(Setting_UniqueSaveDir))
        {
            SaveFile.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
        }
#ifdef _WIN32
        SaveFile.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif
        SaveFile.SetName(g_Settings->LoadStringVal(Rdb_GoodName).c_str());
        g_Settings->SaveDword(Game_LastSaveSlot, g_Settings->LoadDword(Game_CurrentSaveState));
    }
    stdstr_f target_ext("pj%s", Slot != 0 ? stdstr_f("%d", Slot).c_str() : "");
    if (_stricmp(SaveFile.GetExtension().c_str(), target_ext.c_str()) != 0)
    {
        SaveFile.SetNameExtension(stdstr_f("%s.%s", SaveFile.GetNameExtension().c_str(), target_ext.c_str()).c_str());
    }

    CPath ExtraInfo(SaveFile);
    ExtraInfo.SetExtension(".dat");

    CPath ZipFile(SaveFile);
    ZipFile.SetNameExtension(stdstr_f("%s.zip", ZipFile.GetNameExtension().c_str()).c_str());

    // Make sure the target directory exists
    if (!SaveFile.DirectoryExists())
    {
        SaveFile.DirectoryCreate();
    }

    // Open the file
    if (g_Settings->LoadDword(Game_FuncLookupMode) == FuncFind_ChangeMemory)
    {
        if (m_Recomp)
        {
            m_Recomp->ResetRecompCode(true);
        }
    }

    uint32_t RdramSize = g_Settings->LoadDword(Game_RDRamSize);
    uint32_t MiInterReg = g_Reg->MI_INTR_REG;
    uint32_t NextViTimer = m_SystemTimer.GetTimer(CSystemTimer::ViTimer);
    if (g_Settings->LoadDword(Setting_AutoZipInstantSave))
    {
        ZipFile.Delete();
        zipFile file = zipOpen(ZipFile, 0);
        zipOpenNewFileInZip(file, SaveFile.GetNameExtension().c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
        zipWriteInFileInZip(file, &SaveID_0, sizeof(SaveID_0));
        zipWriteInFileInZip(file, &RdramSize, sizeof(uint32_t));
        if (g_Settings->LoadBool(Setting_EnableDisk) && g_Disk)
        {
            // Keep base ROM information (64DD IPL / compatible game ROM)
            zipWriteInFileInZip(file, &g_Rom->GetRomAddress()[0x10], 0x20);
            zipWriteInFileInZip(file, g_Disk->GetDiskAddressID(), 0x20);
        }
        else
        {
            zipWriteInFileInZip(file, g_Rom->GetRomAddress(), 0x40);
        }
        zipWriteInFileInZip(file, &NextViTimer, sizeof(uint32_t));
        zipWriteInFileInZip(file, &m_Reg.m_PROGRAM_COUNTER, sizeof(m_Reg.m_PROGRAM_COUNTER));
        zipWriteInFileInZip(file, m_Reg.m_GPR, sizeof(int64_t) * 32);
        zipWriteInFileInZip(file, m_Reg.m_FPR, sizeof(int64_t) * 32);
        zipWriteInFileInZip(file, m_Reg.m_CP0, sizeof(uint32_t) * 32);
        zipWriteInFileInZip(file, m_Reg.m_FPCR, sizeof(uint32_t) * 32);
        zipWriteInFileInZip(file, &m_Reg.m_HI, sizeof(int64_t));
        zipWriteInFileInZip(file, &m_Reg.m_LO, sizeof(int64_t));
        zipWriteInFileInZip(file, m_Reg.m_RDRAM_Registers, sizeof(uint32_t) * 10);
        zipWriteInFileInZip(file, m_Reg.m_SigProcessor_Interface, sizeof(uint32_t) * 10);
        zipWriteInFileInZip(file, m_Reg.m_Display_ControlReg, sizeof(uint32_t) * 10);
        zipWriteInFileInZip(file, m_Reg.m_Mips_Interface, sizeof(uint32_t) * 4);
        zipWriteInFileInZip(file, m_Reg.m_Video_Interface, sizeof(uint32_t) * 14);
        zipWriteInFileInZip(file, m_Reg.m_Audio_Interface, sizeof(uint32_t) * 6);
        zipWriteInFileInZip(file, m_Reg.m_Peripheral_Interface, sizeof(uint32_t) * 13);
        zipWriteInFileInZip(file, m_Reg.m_RDRAM_Interface, sizeof(uint32_t) * 8);
        zipWriteInFileInZip(file, m_Reg.m_SerialInterface, sizeof(uint32_t) * 4);
        zipWriteInFileInZip(file, (void *const)&m_TLB.TlbEntry(0), sizeof(CTLB::TLB_ENTRY) * 32);
        zipWriteInFileInZip(file, m_MMU_VM.PifRam(), 0x40);
        zipWriteInFileInZip(file, m_MMU_VM.Rdram(), RdramSize);
        zipWriteInFileInZip(file, m_MMU_VM.Dmem(), 0x1000);
        zipWriteInFileInZip(file, m_MMU_VM.Imem(), 0x1000);
        zipCloseFileInZip(file);

        zipOpenNewFileInZip(file, ExtraInfo.GetNameExtension().c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);

        // Extra info v2
        zipWriteInFileInZip(file, &SaveID_2, sizeof(SaveID_2));

        // Disk interface info
        zipWriteInFileInZip(file, m_Reg.m_DiskInterface, sizeof(uint32_t) * 22);

        // System timers info
        m_SystemTimer.SaveData(file);

        zipCloseFileInZip(file);

        zipClose(file, "");
#if defined(ANDROID)
        utimes((const char *)ZipFile, nullptr);
#endif
    }
    else
    {
        WriteTrace(TraceN64System, TraceDebug, "SaveFile: %s", (const char *)SaveFile);
        ExtraInfo.Delete();
        SaveFile.Delete();
        CFile hSaveFile(SaveFile, CFileBase::modeWrite | CFileBase::modeCreate);
        if (!hSaveFile.IsOpen())
        {
            g_Notify->DisplayError(GS(MSG_FAIL_OPEN_SAVE));
            m_Reg.MI_INTR_REG = MiInterReg;
            WriteTrace(TraceN64System, TraceDebug, "Done - Failed to open");
            return true;
        }

        // Write info to file
        hSaveFile.SeekToBegin();
        hSaveFile.Write(&SaveID_0, sizeof(uint32_t));
        hSaveFile.Write(&RdramSize, sizeof(uint32_t));
        if (g_Settings->LoadBool(Setting_EnableDisk) && g_Disk)
        {
            // Keep base ROM information (64DD IPL / compatible game ROM)
            hSaveFile.Write(&g_Rom->GetRomAddress()[0x10], 0x20);
            hSaveFile.Write(g_Disk->GetDiskAddressID(), 0x20);
        }
        else
        {
            hSaveFile.Write(g_Rom->GetRomAddress(), 0x40);
        }
        hSaveFile.Write(&NextViTimer, sizeof(uint32_t));
        hSaveFile.Write(&m_Reg.m_PROGRAM_COUNTER, sizeof(m_Reg.m_PROGRAM_COUNTER));
        hSaveFile.Write(m_Reg.m_GPR, sizeof(int64_t) * 32);
        hSaveFile.Write(m_Reg.m_FPR, sizeof(int64_t) * 32);
        hSaveFile.Write(m_Reg.m_CP0, sizeof(uint32_t) * 32);
        hSaveFile.Write(m_Reg.m_FPCR, sizeof(uint32_t) * 32);
        hSaveFile.Write(&m_Reg.m_HI, sizeof(int64_t));
        hSaveFile.Write(&m_Reg.m_LO, sizeof(int64_t));
        hSaveFile.Write(m_Reg.m_RDRAM_Registers, sizeof(uint32_t) * 10);
        hSaveFile.Write(m_Reg.m_SigProcessor_Interface, sizeof(uint32_t) * 10);
        hSaveFile.Write(m_Reg.m_Display_ControlReg, sizeof(uint32_t) * 10);
        hSaveFile.Write(m_Reg.m_Mips_Interface, sizeof(uint32_t) * 4);
        hSaveFile.Write(m_Reg.m_Video_Interface, sizeof(uint32_t) * 14);
        hSaveFile.Write(m_Reg.m_Audio_Interface, sizeof(uint32_t) * 6);
        hSaveFile.Write(m_Reg.m_Peripheral_Interface, sizeof(uint32_t) * 13);
        hSaveFile.Write(m_Reg.m_RDRAM_Interface, sizeof(uint32_t) * 8);
        hSaveFile.Write(m_Reg.m_SerialInterface, sizeof(uint32_t) * 4);
        hSaveFile.Write(&m_TLB.TlbEntry(0), sizeof(CTLB::TLB_ENTRY) * 32);
        hSaveFile.Write(g_MMU->PifRam(), 0x40);
        hSaveFile.Write(g_MMU->Rdram(), RdramSize);
        hSaveFile.Write(g_MMU->Dmem(), 0x1000);
        hSaveFile.Write(g_MMU->Imem(), 0x1000);
        hSaveFile.Close();

        CFile hExtraInfo(ExtraInfo, CFileBase::modeWrite | CFileBase::modeCreate);
        if (hExtraInfo.IsOpen())
        {
            // Extra info v2
            hExtraInfo.Write(&SaveID_2, sizeof(uint32_t));

            // Disk interface info
            hExtraInfo.Write(m_Reg.m_DiskInterface, sizeof(uint32_t) * 22);

            // System timers info
            m_SystemTimer.SaveData(hExtraInfo);
            hExtraInfo.Close();
        }
    }
    m_Reg.MI_INTR_REG = MiInterReg;
    g_Settings->SaveString(GameRunning_InstantSaveFile, "");
    g_Settings->SaveDword(Game_LastSaveTime, (uint32_t)time(nullptr));
    if (g_Settings->LoadDword(Setting_AutoZipInstantSave))
    {
        SaveFile = ZipFile;
    }
    g_Notify->DisplayMessage(3, stdstr_f("%s %s", g_Lang->GetString(MSG_SAVED_STATE).c_str(), stdstr(SaveFile.GetNameExtension()).c_str()).c_str());
    WriteTrace(TraceN64System, TraceDebug, "Done");
    return true;
}

bool CN64System::LoadState()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");

    stdstr InstantFileName = g_Settings->LoadStringVal(GameRunning_InstantSaveFile);
    if (!InstantFileName.empty())
    {
        bool Result = LoadState(InstantFileName.c_str());
        g_Settings->SaveString(GameRunning_InstantSaveFile, "");
        return Result;
    }

    CPath FileName(g_Settings->LoadStringVal(Directory_InstantSave).c_str(), "");
    if (g_Settings->LoadBool(Setting_UniqueSaveDir))
    {
        FileName.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
    }
#ifdef _WIN32
    FileName.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif
    if (g_Settings->LoadDword(Game_CurrentSaveState) != 0)
    {
        FileName.SetNameExtension(stdstr_f("%s.pj%d", g_Settings->LoadStringVal(Rdb_GoodName).c_str(), g_Settings->LoadDword(Game_CurrentSaveState)).c_str());
    }
    else
    {
        FileName.SetNameExtension(stdstr_f("%s.pj", g_Settings->LoadStringVal(Rdb_GoodName).c_str()).c_str());
    }

    CPath ZipFileName;
    ZipFileName = (const std::string &)FileName + ".zip";

    if (g_Settings->LoadDword(Setting_AutoZipInstantSave))
    {
        FileName = ZipFileName;
    }
    if ((g_Settings->LoadDword(Setting_AutoZipInstantSave) && ZipFileName.Exists()) || FileName.Exists())
    {
        if (LoadState(FileName))
        {
            return true;
        }
    }
    CPath NewFileName = FileName;

    // Use old file Name
    if (g_Settings->LoadDword(Game_CurrentSaveState) != 0)
    {
        FileName.SetNameExtension(stdstr_f("%s.pj%d", g_Settings->LoadStringVal(Game_GameName).c_str(), g_Settings->LoadDword(Game_CurrentSaveState)).c_str());
    }
    else
    {
        FileName.SetNameExtension(stdstr_f("%s.pj", g_Settings->LoadStringVal(Game_GameName).c_str()).c_str());
    }
    bool Result = LoadState(FileName);
    WriteTrace(TraceN64System, TraceDebug, "Done (res: %s)", Result ? "True" : "False");
    if (Result == false)
    {
        if (g_Settings->LoadDword(Setting_AutoZipInstantSave))
        {
            Result = LoadState(ZipFileName);
        }
        else
        {
            Result = LoadState(NewFileName);
        }
    }
    return Result;
}

bool CN64System::LoadState(const char * FileName)
{
    WriteTrace(TraceN64System, TraceDebug, "(%s): Start", FileName);

    uint32_t Value, SaveRDRAMSize, NextVITimer = 0, old_status, old_width, old_dacrate;
    bool LoadedZipFile = false, AudioResetOnLoad;
    old_status = m_Reg.VI_STATUS_REG;
    old_width = m_Reg.VI_WIDTH_REG;
    old_dacrate = m_Reg.AI_DACRATE_REG;

    CPath SaveFile(FileName);

    if (g_Settings->LoadDword(Setting_AutoZipInstantSave) || _stricmp(SaveFile.GetExtension().c_str(), ".zip") == 0)
    {
        // If zipping save add .zip on the end
        if (!SaveFile.Exists() && _stricmp(SaveFile.GetExtension().c_str(), ".zip") != 0)
        {
            SaveFile.SetNameExtension(stdstr_f("%s.zip", SaveFile.GetNameExtension().c_str()).c_str());
        }
        unzFile file = unzOpen(SaveFile);
        int port = -1;
        if (file != nullptr)
        {
            port = unzGoToFirstFile(file);
        }
        while (port == UNZ_OK)
        {
            unz_file_info info;
            char zname[132];

            unzGetCurrentFileInfo(file, &info, zname, 128, nullptr, 0, nullptr, 0);
            if (unzLocateFile(file, zname, 1) != UNZ_OK)
            {
                unzClose(file);
                port = -1;
                continue;
            }
            if (unzOpenCurrentFile(file) != UNZ_OK)
            {
                unzClose(file);
                port = -1;
                continue;
            }
            unzReadCurrentFile(file, &Value, 4);
            if (!LoadedZipFile && Value == SaveID_0 && port == UNZ_OK)
            {
                unzReadCurrentFile(file, &SaveRDRAMSize, sizeof(SaveRDRAMSize));
                // Check header

                uint8_t LoadHeader[64];
                unzReadCurrentFile(file, LoadHeader, 0x40);
                if (g_Settings->LoadBool(Setting_EnableDisk) && g_Disk)
                {
                    // Base ROM information (64DD IPL / compatible game ROM) and disk info check
                    if ((memcmp(LoadHeader, &g_Rom->GetRomAddress()[0x10], 0x20) != 0 ||
                        memcmp(&LoadHeader[0x20], g_Disk->GetDiskAddressID(), 0x20) != 0) &&
                        !g_Notify->AskYesNoQuestion(g_Lang->GetString(MSG_SAVE_STATE_HEADER).c_str()))
                    {
                        return false;
                    }
                }
                else
                {
                    if (memcmp(LoadHeader, g_Rom->GetRomAddress(), 0x40) != 0 &&
                        !g_Notify->AskYesNoQuestion(g_Lang->GetString(MSG_SAVE_STATE_HEADER).c_str()))
                    {
                        return false;
                    }
                }
                Reset(false, true);

                m_MMU_VM.UnProtectMemory(0x80000000, 0x80000000 + g_Settings->LoadDword(Game_RDRamSize) - 4);
                m_MMU_VM.UnProtectMemory(0xA4000000, 0xA4001FFC);
                g_Settings->SaveDword(Game_RDRamSize, SaveRDRAMSize);
                unzReadCurrentFile(file, &NextVITimer, sizeof(NextVITimer));
                unzReadCurrentFile(file, &m_Reg.m_PROGRAM_COUNTER, sizeof(m_Reg.m_PROGRAM_COUNTER));
                unzReadCurrentFile(file, m_Reg.m_GPR, sizeof(int64_t) * 32);
                unzReadCurrentFile(file, m_Reg.m_FPR, sizeof(int64_t) * 32);
                unzReadCurrentFile(file, m_Reg.m_CP0, sizeof(uint32_t) * 32);
                unzReadCurrentFile(file, m_Reg.m_FPCR, sizeof(uint32_t) * 32);
                unzReadCurrentFile(file, &m_Reg.m_HI, sizeof(int64_t));
                unzReadCurrentFile(file, &m_Reg.m_LO, sizeof(int64_t));
                unzReadCurrentFile(file, m_Reg.m_RDRAM_Registers, sizeof(uint32_t) * 10);
                unzReadCurrentFile(file, m_Reg.m_SigProcessor_Interface, sizeof(uint32_t) * 10);
                unzReadCurrentFile(file, m_Reg.m_Display_ControlReg, sizeof(uint32_t) * 10);
                unzReadCurrentFile(file, m_Reg.m_Mips_Interface, sizeof(uint32_t) * 4);
                unzReadCurrentFile(file, m_Reg.m_Video_Interface, sizeof(uint32_t) * 14);
                unzReadCurrentFile(file, m_Reg.m_Audio_Interface, sizeof(uint32_t) * 6);
                unzReadCurrentFile(file, m_Reg.m_Peripheral_Interface, sizeof(uint32_t) * 13);
                unzReadCurrentFile(file, m_Reg.m_RDRAM_Interface, sizeof(uint32_t) * 8);
                unzReadCurrentFile(file, m_Reg.m_SerialInterface, sizeof(uint32_t) * 4);
                unzReadCurrentFile(file, (void *const)&m_TLB.TlbEntry(0), sizeof(CTLB::TLB_ENTRY) * 32);
                unzReadCurrentFile(file, m_MMU_VM.PifRam(), 0x40);
                unzReadCurrentFile(file, m_MMU_VM.Rdram(), SaveRDRAMSize);
                unzReadCurrentFile(file, m_MMU_VM.Dmem(), 0x1000);
                unzReadCurrentFile(file, m_MMU_VM.Imem(), 0x1000);
                unzCloseCurrentFile(file);
                port = unzGoToFirstFile(file);
                LoadedZipFile = true;
                continue;
            }
            if (LoadedZipFile && Value == SaveID_1 && port == UNZ_OK)
            {
                // Extra info v1
                // System timers info
                m_SystemTimer.LoadData(file);
            }
            if (LoadedZipFile && Value == SaveID_2 && port == UNZ_OK)
            {
                // Extra info v2 (Project64 2.4)
                // Disk interface info
                unzReadCurrentFile(file, m_Reg.m_DiskInterface, sizeof(uint32_t) * 22);

                // Recover disk seek address (if the save state is done while loading/saving data)
                if (g_Disk)
                    DiskBMReadWrite(false);

                // System timers info
                m_SystemTimer.LoadData(file);
            }
            unzCloseCurrentFile(file);
            port = unzGoToNextFile(file);
        }
        unzClose(file);
    }
    if (!LoadedZipFile)
    {
        CFile hSaveFile(SaveFile, CFileBase::modeRead);
        if (!hSaveFile.IsOpen())
        {
            g_Notify->DisplayMessage(3, stdstr_f("%s %s", GS(MSG_UNABLED_LOAD_STATE), FileName).c_str());
            return false;
        }
        hSaveFile.SeekToBegin();

        hSaveFile.Read(&Value, sizeof(Value));
        if (Value != SaveID_0)
        {
            return false;
        }

        hSaveFile.Read(&SaveRDRAMSize, sizeof(SaveRDRAMSize));

        // Check header
        uint8_t LoadHeader[64];
        hSaveFile.Read(LoadHeader, 0x40);
        if (g_Settings->LoadBool(Setting_EnableDisk) && g_Disk)
        {
            // Base ROM information (64DD IPL / compatible game ROM) and disk info check
            if ((memcmp(LoadHeader, &g_Rom->GetRomAddress()[0x10], 0x20) != 0 ||
                memcmp(&LoadHeader[0x20], g_Disk->GetDiskAddressID(), 0x20) != 0) &&
                !g_Notify->AskYesNoQuestion(g_Lang->GetString(MSG_SAVE_STATE_HEADER).c_str()))
            {
                return false;
            }
        }
        else
        {
            if (memcmp(LoadHeader, g_Rom->GetRomAddress(), 0x40) != 0 &&
                !g_Notify->AskYesNoQuestion(g_Lang->GetString(MSG_SAVE_STATE_HEADER).c_str()))
            {
                return false;
            }
        }
        Reset(false, true);
        m_MMU_VM.UnProtectMemory(0x80000000, 0x80000000 + g_Settings->LoadDword(Game_RDRamSize) - 4);
        m_MMU_VM.UnProtectMemory(0xA4000000, 0xA4001FFC);
        g_Settings->SaveDword(Game_RDRamSize, SaveRDRAMSize);

        hSaveFile.Read(&NextVITimer, sizeof(NextVITimer));
        hSaveFile.Read(&m_Reg.m_PROGRAM_COUNTER, sizeof(m_Reg.m_PROGRAM_COUNTER));
        hSaveFile.Read(m_Reg.m_GPR, sizeof(int64_t) * 32);
        hSaveFile.Read(m_Reg.m_FPR, sizeof(int64_t) * 32);
        hSaveFile.Read(m_Reg.m_CP0, sizeof(uint32_t) * 32);
        hSaveFile.Read(m_Reg.m_FPCR, sizeof(uint32_t) * 32);
        hSaveFile.Read(&m_Reg.m_HI, sizeof(int64_t));
        hSaveFile.Read(&m_Reg.m_LO, sizeof(int64_t));
        hSaveFile.Read(m_Reg.m_RDRAM_Registers, sizeof(uint32_t) * 10);
        hSaveFile.Read(m_Reg.m_SigProcessor_Interface, sizeof(uint32_t) * 10);
        hSaveFile.Read(m_Reg.m_Display_ControlReg, sizeof(uint32_t) * 10);
        hSaveFile.Read(m_Reg.m_Mips_Interface, sizeof(uint32_t) * 4);
        hSaveFile.Read(m_Reg.m_Video_Interface, sizeof(uint32_t) * 14);
        hSaveFile.Read(m_Reg.m_Audio_Interface, sizeof(uint32_t) * 6);
        hSaveFile.Read(m_Reg.m_Peripheral_Interface, sizeof(uint32_t) * 13);
        hSaveFile.Read(m_Reg.m_RDRAM_Interface, sizeof(uint32_t) * 8);
        hSaveFile.Read(m_Reg.m_SerialInterface, sizeof(uint32_t) * 4);
        hSaveFile.Read((void *const)&m_TLB.TlbEntry(0), sizeof(CTLB::TLB_ENTRY) * 32);
        hSaveFile.Read(m_MMU_VM.PifRam(), 0x40);
        hSaveFile.Read(m_MMU_VM.Rdram(), SaveRDRAMSize);
        hSaveFile.Read(m_MMU_VM.Dmem(), 0x1000);
        hSaveFile.Read(m_MMU_VM.Imem(), 0x1000);
        hSaveFile.Close();

        CPath ExtraInfo(SaveFile);
        ExtraInfo.SetExtension(".dat");
        CFile hExtraInfo(ExtraInfo, CFileBase::modeRead);
        if (hExtraInfo.IsOpen())
        {
            // Extra info version check
            hExtraInfo.Read(&Value, sizeof(Value));
            if (Value != SaveID_1 && Value != SaveID_2)
                hExtraInfo.SeekToBegin();

            // Disk interface info
            if (Value == SaveID_2)
            {
                hExtraInfo.Read(m_Reg.m_DiskInterface, sizeof(uint32_t) * 22);

                // Recover disk seek address (if the save state is done while loading/saving data)
                if (g_Disk)
                    DiskBMReadWrite(false);
            }

            // System timers info
            m_SystemTimer.LoadData(hExtraInfo);

            hExtraInfo.Close();
        }
    }

    // Fix losing audio in certain games with certain plugins
    AudioResetOnLoad = g_Settings->LoadBool(Game_AudioResetOnLoad);
    if (AudioResetOnLoad)
    {
        m_Reg.m_AudioIntrReg |= MI_INTR_AI;
        m_Reg.AI_STATUS_REG &= ~AI_STATUS_FIFO_FULL;
        m_Reg.MI_INTR_REG |= MI_INTR_AI;
    }

    if (bFixedAudio())
    {
        m_Audio.SetFrequency(m_Reg.AI_DACRATE_REG, SystemType());
    }

    if (old_status != m_Reg.VI_STATUS_REG)
    {
        g_Plugins->Gfx()->ViStatusChanged();
    }

    if (old_width != m_Reg.VI_WIDTH_REG)
    {
        g_Plugins->Gfx()->ViWidthChanged();
    }
    g_Plugins->Audio()->DacrateChanged(SystemType());

    // Fix random register
    while ((int)m_Reg.RANDOM_REGISTER < (int)m_Reg.WIRED_REGISTER)
    {
        m_Reg.RANDOM_REGISTER += 32 - m_Reg.WIRED_REGISTER;
    }
    // Fix up timer
    m_SystemTimer.SetTimer(CSystemTimer::CompareTimer, m_Reg.COMPARE_REGISTER - m_Reg.COUNT_REGISTER, false);
    m_SystemTimer.SetTimer(CSystemTimer::ViTimer, NextVITimer, false);
    m_Reg.FixFpuLocations();
    m_TLB.Reset(false);
    if (m_Recomp)
    {
        m_Recomp->ResetFunctionTimes();
    }
    m_CPU_Usage.ResetTimers();
    m_FPS.Reset(true);
    if (bRecordRecompilerAsm())
    {
        Stop_Recompiler_Log();
        Start_Recompiler_Log();
    }

#ifdef TEST_SP_TRACKING
    m_CurrentSP = GPR[29].UW[0];
#endif
    if (bFastSP() && m_Recomp) { m_Recomp->ResetMemoryStackPos(); }

    if (g_Settings->LoadDword(Game_CpuType) == CPU_SyncCores)
    {
        if (m_SyncCPU)
        {
            for (int i = 0; i < (sizeof(m_LastSuccessSyncPC) / sizeof(m_LastSuccessSyncPC[0])); i++)
            {
                m_LastSuccessSyncPC[i] = 0;
            }
            m_SyncCPU->SetActiveSystem(true);
            m_SyncCPU->LoadState(FileName);
            SetActiveSystem(true);
            SyncCPU(m_SyncCPU);
        }
    }
    std::string LoadMsg = g_Lang->GetString(MSG_LOADED_STATE);
    g_Notify->DisplayMessage(3, stdstr_f("%s %s", LoadMsg.c_str(), stdstr(SaveFile.GetNameExtension()).c_str()).c_str());
    WriteTrace(TraceN64System, TraceDebug, "Done");
    return true;
}

uint32_t CN64System::GetButtons(int32_t Control) const
{ 
    CControl_Plugin::fnGetKeys GetKeys = g_Plugins->Control()->GetKeys;
    if (!UpdateControllerOnRefresh() && GetKeys != nullptr)
    {
        BUTTONS Keys;
        memset(&Keys, 0, sizeof(Keys));
        GetKeys(Control, &Keys);
        return Keys.Value;
    }
    return m_Buttons[Control]; 
}

void CN64System::DisplayRSPListCount()
{
    g_Notify->DisplayMessage(0, stdstr_f("Dlist: %d   Alist: %d   Unknown: %d", m_DlistCount, m_AlistCount, m_UnknownCount).c_str());
}

void CN64System::RunRSP()
{
    WriteTrace(TraceRSP, TraceDebug, "Start (SP Status %X)", m_Reg.SP_STATUS_REG);

    PROFILE_TIMERS CPU_UsageAddr = m_CPU_Usage.StopTimer();

    if ((m_Reg.SP_STATUS_REG & SP_STATUS_HALT) == 0)
    {
        if ((m_Reg.SP_STATUS_REG & SP_STATUS_BROKE) == 0)
        {
            HighResTimeStamp StartTime;

            uint32_t Task = 0;
            if (m_RspBroke)
            {
                g_MMU->LW_VAddr(0xA4000FC0, Task);
                if (Task == 1 && UseHleGfx() && (m_Reg.DPC_STATUS_REG & DPC_STATUS_FREEZE) != 0)
                {
                    WriteTrace(TraceRSP, TraceDebug, "Dlist that is frozen");
                    return;
                }

                if (g_Debugger != NULL && HaveDebugger())
                {
                    g_Debugger->RSPReceivedTask();
                }

                switch (Task)
                {
                case 1:
                    WriteTrace(TraceRSP, TraceDebug, "*** Display list ***");
                    m_DlistCount += 1;
                    m_FPS.UpdateDlCounter();
                    break;
                case 2:
                    WriteTrace(TraceRSP, TraceDebug, "*** Audio list ***");
                    m_AlistCount += 1;
                    break;
                default:
                    WriteTrace(TraceRSP, TraceDebug, "*** Unknown list ***");
                    m_UnknownCount += 1;
                    break;
                }

                if (bShowDListAListCount())
                {
                    DisplayRSPListCount();
                }
                if (bRecordExecutionTimes() || bShowCPUPer())
                {
                    StartTime.SetToNow();
                }
            }

            __except_try()
            {
                WriteTrace(TraceRSP, TraceDebug, "Do cycles - starting");
                g_Plugins->RSP()->DoRspCycles(100);
                WriteTrace(TraceRSP, TraceDebug, "Do cycles - done");
            }
            __except_catch()
            {
                WriteTrace(TraceRSP, TraceError, "Exception generated");
                g_Notify->FatalError("CN64System::RunRSP()\nUnknown memory action\n\nEmulation stopping");
            }

            if (Task == 1 && bDelayDP() && ((m_Reg.m_GfxIntrReg & MI_INTR_DP) != 0))
            {
                g_SystemTimer->SetTimer(CSystemTimer::RSPTimerDlist, 0x1000, false);
                m_Reg.m_GfxIntrReg &= ~MI_INTR_DP;
            }
            if (bRecordExecutionTimes() || bShowCPUPer())
            {
                HighResTimeStamp EndTime;
                EndTime.SetToNow();
                uint32_t TimeTaken = (uint32_t)(EndTime.GetMicroSeconds() - StartTime.GetMicroSeconds());

                switch (Task)
                {
                case 1: m_CPU_Usage.RecordTime(Timer_RSP_Dlist, TimeTaken); break;
                case 2: m_CPU_Usage.RecordTime(Timer_RSP_Alist, TimeTaken); break;
                default: m_CPU_Usage.RecordTime(Timer_RSP_Unknown, TimeTaken); break;
                }
            }

            if ((m_Reg.SP_STATUS_REG & SP_STATUS_HALT) == 0 &&
                (m_Reg.SP_STATUS_REG & SP_STATUS_BROKE) == 0 &&
                m_Reg.m_RspIntrReg == 0)
            {
                g_SystemTimer->SetTimer(CSystemTimer::RspTimer, 0x200, false);
                m_RspBroke = false;
            }
            else
            {
                m_RspBroke = true;
            }
            WriteTrace(TraceRSP, TraceDebug, "Check interrupts");
            g_Reg->CheckInterrupts();
        }
    }
    if (bShowCPUPer())
    {
        m_CPU_Usage.StartTimer(CPU_UsageAddr);
    }

    WriteTrace(TraceRSP, TraceDebug, "Done (SP Status %X)", m_Reg.SP_STATUS_REG);
}

void CN64System::RefreshScreen()
{
    PROFILE_TIMERS CPU_UsageAddr = Timer_None/*, ProfilingAddr = Timer_None*/;
    uint32_t VI_INTR_TIME = 500000;

    if (bShowCPUPer()) { CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_RefreshScreen); }

    // Calculate how many cycles to next refresh
    if (m_Reg.VI_V_SYNC_REG == 0)
    {
        VI_INTR_TIME = 500000;
    }
    else
    {
        VI_INTR_TIME = (m_Reg.VI_V_SYNC_REG + 1) * ViRefreshRate();
        if ((m_Reg.VI_V_SYNC_REG % 1) != 0)
        {
            VI_INTR_TIME -= 38;
        }
    }
    g_SystemTimer->SetTimer(CSystemTimer::ViTimer, VI_INTR_TIME, true);
    if (bFixedAudio())
    {
        g_Audio->SetViIntr(VI_INTR_TIME);
    }
    if (UpdateControllerOnRefresh() && g_Plugins->Control()->GetKeys != nullptr)
    {
        BUTTONS Keys;
        memset(&Keys, 0, sizeof(Keys));

        for (int Control = 0; Control < 4; Control++)
        {
            g_Plugins->Control()->GetKeys(Control, &Keys);
            m_Buttons[Control] = Keys.Value;
        }
    }

    if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_UpdateScreen); }

    __except_try()
    {
        WriteTrace(TraceGFXPlugin, TraceDebug, "UpdateScreen starting");
        g_Plugins->Gfx()->UpdateScreen();
		if (g_Debugger != nullptr && HaveDebugger())
		{
			g_Debugger->FrameDrawn();
		}
        WriteTrace(TraceGFXPlugin, TraceDebug, "UpdateScreen done");
    }
    __except_catch()
    {
        WriteTrace(TraceGFXPlugin, TraceError, "Exception caught");
    }
    g_MMU->UpdateFieldSerration((m_Reg.VI_STATUS_REG & 0x40) != 0);

    if ((bBasicMode() || bLimitFPS()) && (!bSyncToAudio() || !FullSpeed()))
    {
        if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_Idel); }
        uint32_t FrameRate;
        if (m_Limiter.Timer_Process(&FrameRate) && bDisplayFrameRate())
        {
            m_FPS.DisplayViCounter(FrameRate, 0);
            m_bCleanFrameBox = true;
        }
        if (bShowCPUPer()) { m_CPU_Usage.StopTimer(); }
    }
    else if (bDisplayFrameRate())
    {
        if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_UpdateFPS); }
        m_FPS.UpdateViCounter();
        m_bCleanFrameBox = true;
    }

    if (m_bCleanFrameBox && !bDisplayFrameRate())
    {
        m_FPS.Reset(true);
        m_bCleanFrameBox = false;
    }

    if (bShowCPUPer())
    {
        m_CPU_Usage.ShowCPU_Usage();
        m_CPU_Usage.StartTimer(CPU_UsageAddr != Timer_None ? CPU_UsageAddr : Timer_R4300);
    }
    if ((m_Reg.STATUS_REGISTER & STATUS_IE) != 0)
    {
        g_Enhancements->ApplyActive(m_MMU_VM, g_BaseSystem->m_Plugins, !m_SyncSystem);
    }
    //    if (bProfiling)    { m_Profile.StartTimer(ProfilingAddr != Timer_None ? ProfilingAddr : Timer_R4300); }
}

void CN64System::TLB_Mapped(uint32_t VAddr, uint32_t Len, uint32_t PAddr, bool bReadOnly)
{
    m_MMU_VM.TLB_Mapped(VAddr, Len, PAddr, bReadOnly);
}

void CN64System::TLB_Unmaped(uint32_t VAddr, uint32_t Len)
{
    m_MMU_VM.TLB_Unmaped(VAddr, Len);
    if (m_Recomp && bSMM_TLB())
    {
        m_Recomp->ClearRecompCode_Virt(VAddr, Len, CRecompiler::Remove_TLB);
    }
}

void CN64System::TLB_Changed()
{
    if (g_Debugger)
    {
        g_Debugger->TLBChanged();
    }
}