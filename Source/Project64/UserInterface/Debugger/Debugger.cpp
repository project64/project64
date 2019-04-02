/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/

#include "stdafx.h"
#include "DebuggerUI.h"
#include "ScriptHook.h"

#include "CPULog.h"
#include "DMALog.h"
#include "Symbols.h"

CPj64Module _Module;

CDebuggerUI::CDebuggerUI() :
    m_MemoryDump(NULL),
    m_MemoryView(NULL),
    m_MemorySearch(NULL),
    m_DebugTLB(NULL),
    m_CommandsView(NULL),
    m_Scripts(NULL),
    m_Symbols(NULL),
    m_Breakpoints(NULL),
    m_ScriptSystem(NULL),
    m_StackTrace(NULL),
    m_StackView(NULL),
    m_DMALogView(NULL),
    m_CPULogView(NULL),
    m_ExcBreakpoints(NULL),
    m_DMALog(NULL),
    m_CPULog(NULL),
    m_StepEvent(false)
{
    g_Debugger = this;

    m_Breakpoints = new CBreakpoints();
    m_ScriptSystem = new CScriptSystem(this);

    m_DMALog = new CDMALog();
    m_CPULog = new CCPULog();

    CSymbols::InitializeCriticalSection();
    g_Settings->RegisterChangeCB(GameRunning_InReset, this, (CSettings::SettingChangedFunc)GameReset);
    g_Settings->RegisterChangeCB(Debugger_SteppingOps, this, (CSettings::SettingChangedFunc)SteppingOpsChanged);
}

CDebuggerUI::~CDebuggerUI(void)
{
    g_Settings->UnregisterChangeCB(Debugger_SteppingOps, this, (CSettings::SettingChangedFunc)SteppingOpsChanged);
    g_Settings->UnregisterChangeCB(GameRunning_InReset, this, (CSettings::SettingChangedFunc)GameReset);
    Debug_Reset();
    delete m_MemoryView;
    delete m_CommandsView;
    delete m_Scripts;
    delete m_ScriptSystem;
    delete m_Breakpoints;
    delete m_Symbols;
    delete m_MemorySearch;
    delete m_StackTrace;
    delete m_DMALogView;
    delete m_CPULogView;
    delete m_ExcBreakpoints;
    delete m_DMALog;
    delete m_CPULog;

    CSymbols::DeleteCriticalSection();
}

void CDebuggerUI::SteppingOpsChanged(CDebuggerUI * _this)
{
    if (g_Settings->LoadBool(Debugger_SteppingOps))
    {
        _this->OpenCommandWindow();
    }
}

void CDebuggerUI::GameReset(CDebuggerUI * _this)
{
    if (!g_Settings->LoadBool(GameRunning_InReset))
    {
        return;
    }

    if (_this->m_CommandsView)
    {
        _this->m_CommandsView->Reset();
    }

    if (_this->m_DMALogView)
    {
        _this->m_DMALogView->RefreshDMALogWindow(true);
    }

    if (_this->m_StackTrace)
    {
        _this->m_StackTrace->ClearEntries();
    }

    CSymbols::EnterCriticalSection();
    CSymbols::Load();
    CSymbols::LeaveCriticalSection();

    if (_this->m_Symbols)
    {
        _this->m_Symbols->Refresh();
    }
}

void CDebuggerUI::Debug_Reset(void)
{
    if (m_MemoryDump)
    {
        m_MemoryDump->HideWindow();
        delete m_MemoryDump;
        m_MemoryDump = NULL;
    }
    if (m_MemorySearch)
    {
        m_MemorySearch->HideWindow();
        delete m_MemorySearch;
        m_MemorySearch = NULL;
    }
    if (m_DebugTLB)
    {
        m_DebugTLB->HideWindow();
        delete m_DebugTLB;
        m_DebugTLB = NULL;
    }
    if (m_MemoryView)
    {
        m_MemoryView->HideWindow();
        delete m_MemoryView;
        m_MemoryView = NULL;
    }
    if (m_CommandsView)
    {
        m_CommandsView->HideWindow();
        delete m_CommandsView;
        m_CommandsView = NULL;
    }
    if (m_Scripts)
    {
        m_Scripts->HideWindow();
        delete m_Scripts;
        m_Scripts = NULL;
    }
    if (m_Symbols)
    {
        m_Symbols->HideWindow();
        delete m_Symbols;
        m_Symbols = NULL;
    }
    if (m_DMALogView)
    {
        m_DMALogView->HideWindow();
        delete m_DMALogView;
        m_DMALogView = NULL;
    }
    if (m_CPULogView)
    {
        m_CPULogView->HideWindow();
        delete m_CPULogView;
        m_CPULogView = NULL;
    }
    if (m_StackTrace)
    {
        m_StackTrace->HideWindow();
        delete m_StackTrace;
        m_StackTrace = NULL;
    }
    if (m_StackView)
    {
        m_StackView->HideWindow();
        delete m_StackView;
        m_StackView = NULL;
    }
    if (m_ExcBreakpoints)
    {
        m_ExcBreakpoints->HideWindow();
        delete m_ExcBreakpoints;
        m_ExcBreakpoints = NULL;
    }
}

void CDebuggerUI::OpenMemoryDump()
{
    if (g_MMU == NULL)
    {
        return;
    }
    if (m_MemoryDump == NULL)
    {
        m_MemoryDump = new CDumpMemory(this);
    }
    if (m_MemoryDump)
    {
        m_MemoryDump->ShowWindow();
    }
}

void CDebuggerUI::OpenMemoryWindow(void)
{
    if (g_MMU == NULL)
    {
        return;
    }
    if (m_MemoryView == NULL)
    {
        m_MemoryView = new CDebugMemoryView(this);
    }
    if (m_MemoryView)
    {
        m_MemoryView->ShowWindow();
    }
}

void CDebuggerUI::Debug_ShowMemoryLocation(uint32_t Address, bool VAddr)
{
    OpenMemoryWindow();
    if (m_MemoryView)
    {
        m_MemoryView->ShowAddress(Address, VAddr);
    }
}

void CDebuggerUI::OpenTLBWindow(void)
{
    if (g_MMU == NULL)
    {
        return;
    }
    if (m_DebugTLB == NULL)
    {
        m_DebugTLB = new CDebugTlb(this);
    }
    if (m_DebugTLB)
    {
        m_DebugTLB->ShowWindow();
    }
}

void CDebuggerUI::Debug_RefreshTLBWindow(void)
{
    if (m_DebugTLB)
    {
        m_DebugTLB->RefreshTLBWindow();
    }
}

void CDebuggerUI::Debug_RefreshDMALogWindow(void)
{
    if (m_DMALogView)
    {
        m_DMALogView->RefreshDMALogWindow();
    }
}

void CDebuggerUI::OpenMemorySearch()
{
    if (m_MemorySearch == NULL)
    {
        m_MemorySearch = new CDebugMemorySearch(this);
    }
    if (m_MemorySearch)
    {
        m_MemorySearch->ShowWindow();
    }
}

void CDebuggerUI::OpenCommandWindow()
{
    if (m_CommandsView == NULL)
    {
        m_CommandsView = new CDebugCommandsView(this, m_StepEvent);
    }
    m_CommandsView->ShowWindow();
}

void CDebuggerUI::Debug_ShowCommandsLocation(uint32_t address, bool top)
{
    OpenCommandWindow();
    if (m_CommandsView)
    {
        m_CommandsView->ShowAddress(address, top);
    }
}

void CDebuggerUI::OpenScriptsWindow()
{
    if (m_Scripts == NULL)
    {
        m_Scripts = new CDebugScripts(this);
    }
    m_Scripts->ShowWindow();
}

void CDebuggerUI::Debug_RefreshScriptsWindow()
{
    if (m_Scripts != NULL)
    {
        m_Scripts->RefreshList();
    }
}

void CDebuggerUI::Debug_LogScriptsWindow(const char* text)
{
    if (m_Scripts != NULL)
    {
        m_Scripts->ConsolePrint(text);
    }
}

void CDebuggerUI::Debug_ClearScriptsWindow()
{
    if (m_Scripts != NULL)
    {
        m_Scripts->ConsoleClear();
    }
}

void CDebuggerUI::OpenSymbolsWindow()
{
    if (m_Symbols == NULL)
    {
        m_Symbols = new CDebugSymbols(this);
    }
    m_Symbols->ShowWindow();
}

void CDebuggerUI::Debug_RefreshSymbolsWindow()
{
    if (m_Symbols != NULL)
    {
        m_Symbols->Refresh();
    }
}

void CDebuggerUI::OpenDMALogWindow(void)
{
    if (m_DMALogView == NULL)
    {
        m_DMALogView = new CDebugDMALogView(this);
    }
    m_DMALogView->ShowWindow();
}

void CDebuggerUI::OpenCPULogWindow(void)
{
    if (m_CPULogView == NULL)
    {
        m_CPULogView = new CDebugCPULogView(this);
    }
    m_CPULogView->ShowWindow();
}

void CDebuggerUI::OpenExcBreakpointsWindow(void)
{
    if (m_ExcBreakpoints == NULL)
    {
        m_ExcBreakpoints = new CDebugExcBreakpoints(this);
    }
    m_ExcBreakpoints->ShowWindow();
}

void CDebuggerUI::OpenStackTraceWindow(void)
{
    if (m_StackTrace == NULL)
    {
        m_StackTrace = new CDebugStackTrace(this);
    }
    m_StackTrace->ShowWindow();
}

void CDebuggerUI::OpenStackViewWindow(void)
{
    if (m_StackView == NULL)
    {
        m_StackView = new CDebugStackView(this);
    }
    m_StackView->ShowWindow();
}

void CDebuggerUI::Debug_RefreshStackWindow(void)
{
    if (m_StackView != NULL)
    {
        m_StackView->Refresh();
    }
}

void CDebuggerUI::Debug_RefreshStackTraceWindow(void)
{
    if (m_StackTrace != NULL && m_StackTrace->m_hWnd != NULL)
    {
        m_StackTrace->Refresh();
    }
}

void CDebuggerUI::Debug_RefreshCPULogWindow(void)
{
    if (m_CPULogView != NULL)
    {
        m_CPULogView->RefreshList();
    }
}

CBreakpoints* CDebuggerUI::Breakpoints()
{
    return m_Breakpoints;
}

CScriptSystem* CDebuggerUI::ScriptSystem()
{
    return m_ScriptSystem;
}

CDebugScripts* CDebuggerUI::ScriptConsole()
{
    return m_Scripts;
}

CDMALog* CDebuggerUI::DMALog()
{
    return m_DMALog;
}

CCPULog* CDebuggerUI::CPULog()
{
    return m_CPULog;
}

// thread safe LW_PAddr
// does not trigger application breakpoint if paddr is invalid
bool CDebuggerUI::DebugLW_PAddr(uint32_t paddr, uint32_t& value)
{
     if (g_MMU == NULL)
     {
          return false;
     }

     if ((paddr < g_MMU->RdramSize()) || // RDRAM
          (paddr >= 0x04000000 && paddr <= 0x04001FFF)) //  DMEM/IMEM
     {
          value = *(uint32_t*)(g_MMU->Rdram() + paddr);
          return true;
     }
     else if (paddr >= 0x05000000 && paddr <= 0x050004FF) // 64DD buffer
     {
          // todo
          return false;
     }
     else if (paddr >= 0x06000000 && paddr <= 0x06FFFFFF) // Cartridge Domain 1 (Address 1) (64DD IPL ROM)
     {
          uint32_t iplRomOffset = paddr - 0x06000000;

          if (g_DDRom != NULL && iplRomOffset < g_DDRom->GetRomSize())
          {
               value = *(uint32_t*)(g_MMU->Rdram() + paddr);
               return true;
          }
     }
     else if (paddr >= 0x08000000 && paddr < 0x08FFFFFF) // Cartridge Domain 2 (Address 2)
     {
          uint32_t saveOffset = paddr & 0x000FFFFF;

          if (g_System->m_SaveUsing == SaveChip_Sram && saveOffset <= 0x7FFF) // sram
          {
               uint8_t tmp[4] = "";
               CSram *sram = g_MMU->GetSram();
               sram->DmaFromSram(tmp, paddr - 0x08000000, 4);
               value = tmp[3] << 24 | tmp[2] << 16 | tmp[1] << 8 | tmp[0];
               return true;
          }
          else if (g_System->m_SaveUsing == SaveChip_FlashRam && saveOffset == 0) // flash ram status
          {
               CFlashram* flashRam = g_MMU->GetFlashram();
               value = flashRam->ReadFromFlashStatus(0x08000000);
               return true;
          }
     }
     else if (paddr >= 0x10000000 && paddr <= 0x15FFFFFF) // Cartridge ROM
     {
          uint32_t cartRomOffset = paddr - 0x10000000;
          if (g_Rom != NULL && paddr < g_Rom->GetRomSize())
          {
               value = *(uint32_t*)(g_Rom->GetRomAddress() + cartRomOffset);
               return true;
          }
     }
     else if (paddr >= 0x1FC00000 && paddr <= 0x1FC007BF) // PIF ROM
     {
          return false;
     }
     else if (paddr >= 0x1FC007C0 && paddr <= 0x1FC007FF) // PIF RAM
     {
          uint32_t pifRamOffset = paddr - 0x1FC007C0;
          value = *(uint32_t*)(g_MMU->PifRam() + pifRamOffset);
          return true;
     }

     // note: write-only registers are excluded
     switch (paddr)
     {
     case 0x03F00000: value = g_Reg->RDRAM_CONFIG_REG; return true;
     case 0x03F00004: value = g_Reg->RDRAM_DEVICE_ID_REG; return true;
     case 0x03F00008: value = g_Reg->RDRAM_DELAY_REG; return true;
     case 0x03F0000C: value = g_Reg->RDRAM_MODE_REG; return true;
     case 0x03F00010: value = g_Reg->RDRAM_REF_INTERVAL_REG; return true;
     case 0x03F00014: value = g_Reg->RDRAM_REF_ROW_REG; return true;
     case 0x03F00018: value = g_Reg->RDRAM_RAS_INTERVAL_REG; return true;
     case 0x03F0001C: value = g_Reg->RDRAM_MIN_INTERVAL_REG; return true;
     case 0x03F00020: value = g_Reg->RDRAM_ADDR_SELECT_REG; return true;
     case 0x03F00024: value = g_Reg->RDRAM_DEVICE_MANUF_REG; return true;
     case 0x04040010: value = g_Reg->SP_STATUS_REG; return true;
     case 0x04040014: value = g_Reg->SP_DMA_FULL_REG; return true;
     case 0x04040018: value = g_Reg->SP_DMA_BUSY_REG; return true;
     case 0x0404001C: value = g_Reg->SP_SEMAPHORE_REG; return true;
     case 0x04080000: value = g_Reg->SP_PC_REG; return true;
     case 0x0410000C: value = g_Reg->DPC_STATUS_REG; return true;
     case 0x04100010: value = g_Reg->DPC_CLOCK_REG; return true;
     case 0x04100014: value = g_Reg->DPC_BUFBUSY_REG; return true;
     case 0x04100018: value = g_Reg->DPC_PIPEBUSY_REG; return true;
     case 0x0410001C: value = g_Reg->DPC_TMEM_REG; return true;
     case 0x04300000: value = g_Reg->MI_MODE_REG; return true;
     case 0x04300004: value = g_Reg->MI_VERSION_REG; return true;
     case 0x04300008: value = g_Reg->MI_INTR_REG; return true;
     case 0x0430000C: value = g_Reg->MI_INTR_MASK_REG; return true;
     case 0x04400000: value = g_Reg->VI_STATUS_REG; return true;
     case 0x04400004: value = g_Reg->VI_ORIGIN_REG; return true;
     case 0x04400008: value = g_Reg->VI_WIDTH_REG; return true;
     case 0x0440000C: value = g_Reg->VI_INTR_REG; return true;
     case 0x04400010: value = g_Reg->VI_V_CURRENT_LINE_REG; return true;
     case 0x04400014: value = g_Reg->VI_BURST_REG; return true;
     case 0x04400018: value = g_Reg->VI_V_SYNC_REG; return true;
     case 0x0440001C: value = g_Reg->VI_H_SYNC_REG; return true;
     case 0x04400020: value = g_Reg->VI_LEAP_REG; return true;
     case 0x04400024: value = g_Reg->VI_H_START_REG; return true;
     case 0x04400028: value = g_Reg->VI_V_START_REG; return true;
     case 0x0440002C: value = g_Reg->VI_V_BURST_REG; return true;
     case 0x04400030: value = g_Reg->VI_X_SCALE_REG; return true;
     case 0x04400034: value = g_Reg->VI_Y_SCALE_REG; return true;
     case 0x04600000: value = g_Reg->PI_DRAM_ADDR_REG; return true;
     case 0x04600004: value = g_Reg->PI_CART_ADDR_REG; return true;
     case 0x04600008: value = g_Reg->PI_RD_LEN_REG; return true;
     case 0x0460000C: value = g_Reg->PI_WR_LEN_REG; return true;
     case 0x04600010: value = g_Reg->PI_STATUS_REG; return true;
     case 0x04600014: value = g_Reg->PI_DOMAIN1_REG; return true;
     case 0x04600018: value = g_Reg->PI_BSD_DOM1_PWD_REG; return true;
     case 0x0460001C: value = g_Reg->PI_BSD_DOM1_PGS_REG; return true;
     case 0x04600020: value = g_Reg->PI_BSD_DOM1_RLS_REG; return true;
     case 0x04600024: value = g_Reg->PI_DOMAIN2_REG; return true;
     case 0x04600028: value = g_Reg->PI_BSD_DOM2_PWD_REG; return true;
     case 0x0460002C: value = g_Reg->PI_BSD_DOM2_PGS_REG; return true;
     case 0x04600030: value = g_Reg->PI_BSD_DOM2_RLS_REG; return true;
     case 0x04700000: value = g_Reg->RI_MODE_REG; return true;
     case 0x04700004: value = g_Reg->RI_CONFIG_REG; return true;
     case 0x04700008: value = g_Reg->RI_CURRENT_LOAD_REG; return true;
     case 0x0470000C: value = g_Reg->RI_SELECT_REG; return true;
     case 0x04700010: value = g_Reg->RI_REFRESH_REG; return true;
     case 0x04700014: value = g_Reg->RI_LATENCY_REG; return true;
     case 0x04700018: value = g_Reg->RI_RERROR_REG; return true;
     case 0x0470001C: value = g_Reg->RI_WERROR_REG; return true;
     case 0x04800018: value = g_Reg->SI_STATUS_REG; return true;
     case 0x05000500: value = g_Reg->ASIC_DATA; return true;
     case 0x05000504: value = g_Reg->ASIC_MISC_REG; return true;
     case 0x05000508: value = g_Reg->ASIC_STATUS; return true;
     case 0x0500050C: value = g_Reg->ASIC_CUR_TK; return true;
     case 0x05000510: value = g_Reg->ASIC_BM_STATUS; return true;
     case 0x05000514: value = g_Reg->ASIC_ERR_SECTOR; return true;
     case 0x05000518: value = g_Reg->ASIC_SEQ_STATUS; return true;
     case 0x0500051C: value = g_Reg->ASIC_CUR_SECTOR; return true;
     case 0x05000520: value = g_Reg->ASIC_HARD_RESET; return true;
     case 0x05000524: value = g_Reg->ASIC_C1_S0; return true;
     case 0x05000528: value = g_Reg->ASIC_HOST_SECBYTE; return true;
     case 0x0500052C: value = g_Reg->ASIC_C1_S2; return true;
     case 0x05000530: value = g_Reg->ASIC_SEC_BYTE; return true;
     case 0x05000534: value = g_Reg->ASIC_C1_S4; return true;
     case 0x05000538: value = g_Reg->ASIC_C1_S6; return true;
     case 0x0500053C: value = g_Reg->ASIC_CUR_ADDR; return true;
     case 0x05000540: value = g_Reg->ASIC_ID_REG; return true;
     case 0x05000544: value = g_Reg->ASIC_TEST_REG; return true;
     case 0x05000548: value = g_Reg->ASIC_TEST_PIN_SEL; return true;
     case 0x04500004:
          if (g_System->bFixedAudio())
          {
               value = g_Audio->GetLength();
          }
          else
          {
               CAudioPlugin* audioPlg = g_Plugins->Audio();
               value = (audioPlg->AiReadLength != NULL) ? audioPlg->AiReadLength() : 0;
          }
          return true;
     case 0x0450000C:
          value = g_System->bFixedAudio() ? g_Audio->GetStatus() : g_Reg->AI_STATUS_REG;
          return true;
     }

     return false;
}

bool CDebuggerUI::DebugLW_VAddr(uint32_t vaddr, uint32_t& value)
{
     if (vaddr <= 0x7FFFFFFF || vaddr >= 0xC0000000) // KUSEG, KSEG2 (TLB)
     {
          if (g_MMU == NULL)
          {
               return false;
          }

          return g_MMU->LW_VAddr(vaddr, value);
     }

     uint32_t paddr = vaddr & 0x1FFFFFFF;
     return DebugLW_PAddr(paddr, value);
}

// CDebugger implementation

void CDebuggerUI::TLBChanged()
{
    Debug_RefreshTLBWindow();
}


// Exception handling - break on exception vector if exception bp is set
void CDebuggerUI::HandleCPUException(void)
{
    int exc = (g_Reg->CAUSE_REGISTER >> 2) & 0x1F;

    if ((CDebugSettings::ExceptionBreakpoints() & (1 << exc)))
    {
        if (CDebugSettings::bCPULoggingEnabled())
        {
            g_Debugger->OpenCPULogWindow();
        }

        g_Settings->SaveBool(Debugger_SteppingOps, true);
    }
}

void CDebuggerUI::HandleCartToRamDMA(void)
{
    COpInfo opInfo(R4300iOp::m_Opcode);

    uint32_t dmaRomAddr = g_Reg->PI_CART_ADDR_REG & 0x0FFFFFFF;
    uint32_t dmaRamAddr = g_Reg->PI_DRAM_ADDR_REG | 0x80000000;
    uint32_t dmaLen = opInfo.GetStoreValueUnsigned() + 1;

    m_DMALog->AddEntry(dmaRomAddr, dmaRamAddr, dmaLen);
    Debug_RefreshDMALogWindow();
    
    // break if write breakpoint exists anywhere in target buffer
    if (m_Breakpoints->WriteBPExistsInChunk(dmaRamAddr, dmaLen))
    {
        g_Settings->SaveBool(Debugger_SteppingOps, true);
    }
}

// Called from the interpreter core at the beginning of every CPU step
void CDebuggerUI::CPUStepStarted()
{
    if (isStepping() && bCPULoggingEnabled())
    {
        Debug_RefreshCPULogWindow();
    }

    uint32_t pc = g_Reg->m_PROGRAM_COUNTER;
    COpInfo opInfo(R4300iOp::m_Opcode);

    if (opInfo.IsStoreCommand())
    {
        uint32_t memoryAddress = opInfo.GetLoadStoreAddress();

        if (m_Breakpoints->MemLockExists(memoryAddress, opInfo.NumBytesToStore()))
        {
            // Memory is locked, skip op
            g_Settings->SaveBool(Debugger_SkipOp, true);
            return;
        }
    }

    m_ScriptSystem->HookCPUExec()->InvokeByAddressInRange(pc);
    if (SkipOp()) { return; }

    m_ScriptSystem->HookCPUExecOpcode()->InvokeByAddressInRange_MaskedOpcode(pc, R4300iOp::m_Opcode.Hex);
    if (SkipOp()) { return; }

    m_ScriptSystem->HookCPUGPRValue()->InvokeByAddressInRange_GPRValue(pc);
    if (SkipOp()) { return; }
    
    // Memory events, pi cart -> ram dma
    if (opInfo.IsLoadStoreCommand()) // Read and write instructions
    {
        uint32_t memoryAddress = opInfo.GetLoadStoreAddress();

        if (opInfo.IsLoadCommand()) // Read instructions
        {
            m_ScriptSystem->HookCPURead()->InvokeByAddressInRange(memoryAddress);
            if (SkipOp()) { return; }
        }
        else // Write instructions
        {
            m_ScriptSystem->HookCPUWrite()->InvokeByAddressInRange(memoryAddress);
            if (SkipOp()) { return; }

            if (memoryAddress == 0xA460000C) // PI_WR_LEN_REG
            {
                HandleCartToRamDMA();
            }
        }
    }

    if (CDebugSettings::ExceptionBreakpoints() != 0)
    {
        if (pc == 0x80000000 || pc == 0x80000080 ||
            pc == 0xA0000100 || pc == 0x80000180)
        {
            HandleCPUException();
        }
    }

    if (m_Breakpoints->HaveAnyGPRWriteBP())
    {
        int nReg = 0;
        opInfo.WritesGPR(&nReg);

        if (nReg != 0 && m_Breakpoints->HaveGPRWriteBP(nReg))
        {
            g_Settings->SaveBool(Debugger_SteppingOps, true);
        }
    }

    if (m_Breakpoints->HaveAnyGPRReadBP())
    {
        int nReg1 = 0, nReg2 = 0;
        opInfo.ReadsGPR(&nReg1, &nReg2);

        if ((nReg1 != 0 && m_Breakpoints->HaveGPRReadBP(nReg1)) ||
            (nReg2 != 0 && m_Breakpoints->HaveGPRReadBP(nReg2)))
        {
            g_Settings->SaveBool(Debugger_SteppingOps, true);
        }
    }

    if (m_Breakpoints->HaveHIWriteBP() && opInfo.WritesHI() ||
        m_Breakpoints->HaveLOWriteBP() && opInfo.WritesLO() ||
        m_Breakpoints->HaveHIReadBP()  && opInfo.ReadsHI()  ||
        m_Breakpoints->HaveLOReadBP()  && opInfo.ReadsLO())
    {
        g_Settings->SaveBool(Debugger_SteppingOps, true);
    }
}

// Called before opcode is executed (not called if SkipOp is set)
void CDebuggerUI::CPUStep()
{
    if (bCPULoggingEnabled())
    {
        m_CPULog->PushState();
    }
}

// Called after opcode has been executed
void CDebuggerUI::CPUStepEnded()
{
    OPCODE Opcode = R4300iOp::m_Opcode;
    uint32_t op = Opcode.op;
    uint32_t funct = Opcode.funct;

    if (m_StackTrace == NULL)
    {
        m_StackTrace = new CDebugStackTrace(this);
    }

    if (op == R4300i_JAL || ((op == R4300i_SPECIAL) && (funct == R4300i_SPECIAL_JALR) && (Opcode.rd == 31))) // JAL or JALR RA, x
    {
        m_StackTrace->PushEntry(R4300iOp::m_JumpToLocation, g_Reg->m_PROGRAM_COUNTER);
    }
    else if (funct == R4300i_SPECIAL_JR && Opcode.rs == 31) // JR RA
    {
        m_StackTrace->PopEntry();
    }
    else if (op == R4300i_CP0 && funct == R4300i_COP0_CO_ERET) // TODO may need more work
    {
        m_StackTrace->ClearEntries();
    }
}

void CDebuggerUI::FrameDrawn()
{
    static HWND hMainWnd = NULL;

    static HFONT monoFont = CreateFont(-11, 0, 0, 0,
        FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        PROOF_QUALITY, FF_DONTCARE, "Consolas"
    );

    if (hMainWnd == NULL)
    {
        RenderWindow* mainWindow = g_Plugins->MainWindow();

        if (mainWindow == NULL)
        {
            return;
        }

        hMainWnd = (HWND)mainWindow->GetWindowHandle();
    }

    HDC hdc = GetDC(hMainWnd);

    CRect rt;

    GetClientRect(hMainWnd, &rt);
    SetBkColor(hdc, RGB(0, 0, 0));

    SelectObject(hdc, monoFont);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, RGB(0, 0, 0));

    m_ScriptSystem->SetScreenDC(hdc);
    m_ScriptSystem->HookFrameDrawn()->InvokeAll();

    ReleaseDC(hMainWnd, hdc);
}

void CDebuggerUI::WaitForStep(void)
{
    g_Settings->SaveBool(Debugger_WaitingForStep, true);
    m_StepEvent.IsTriggered(SyncEvent::INFINITE_TIMEOUT);
    g_Settings->SaveBool(Debugger_WaitingForStep, false);
}

bool CDebuggerUI::ExecutionBP(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->ExecutionBPExists(address, true) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP8(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->ReadBPExists8(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP16(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->ReadBPExists16(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP32(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->ReadBPExists32(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::ReadBP64(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->ReadBPExists64(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP8(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->WriteBPExists8(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP16(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->WriteBPExists16(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP32(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->WriteBPExists32(address) != CBreakpoints::BP_NOT_SET;
}

bool CDebuggerUI::WriteBP64(uint32_t address)
{
    return m_Breakpoints != NULL && m_Breakpoints->WriteBPExists64(address) != CBreakpoints::BP_NOT_SET;
}
