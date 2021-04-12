#include "stdafx.h"

#include "Debugger-RegisterTabs.h"
#include "OpInfo.h"

bool CRegisterTabs::m_bColorsEnabled = false;
CDebuggerUI* CRegisterTabs::m_Debugger = nullptr;

CRegisterTabs::CRegisterTabs() :
    m_attached(false)
{
}

CRegisterTabs::~CRegisterTabs()
{
}

void CRegisterTabs::Attach(HWND hWndNew, CDebuggerUI* debugger)
{
    m_Debugger = debugger;
    CTabCtrl::Attach(hWndNew);

    m_GPRTab = AddTab("GPR", IDD_Debugger_RegGPR, TabProcGPR);
    m_FPRTab = AddTab("FPR", IDD_Debugger_RegFPR, TabProcFPR);
    m_COP0Tab = AddTab("COP0", IDD_Debugger_RegCOP0, TabProcDefault);
    m_RDRAMTab = AddTab("RDRAM", IDD_Debugger_RegRDRAM, TabProcDefault);
    m_SPTab = AddTab("SP", IDD_Debugger_RegSP, TabProcDefault);
    m_DPCTab = AddTab("DPC", IDD_Debugger_RegDPC, TabProcDefault);
    m_MITab = AddTab("MI", IDD_Debugger_RegMI, TabProcDefault);
    m_VITab = AddTab("VI", IDD_Debugger_RegVI, TabProcDefault);
    m_AITab = AddTab("AI", IDD_Debugger_RegAI, TabProcDefault);
    m_PITab = AddTab("PI", IDD_Debugger_RegPI, TabProcDefault);
    m_RITab = AddTab("RI", IDD_Debugger_RegRI, TabProcDefault);
    m_SITab = AddTab("SI", IDD_Debugger_RegSI, TabProcDefault);
    m_DDTab = AddTab("DD", IDD_Debugger_RegDD, TabProcDefault);

    InitRegisterEdits64(m_GPRTab, m_GPREdits, &TabData::GPR);
    InitRegisterEdit64(m_GPRTab, m_HIEdit, GPRHi);
    InitRegisterEdit64(m_GPRTab, m_LOEdit, GPRLo);

    InitRegisterEdits(m_FPRTab, m_FPREdits, &TabData::FPR);
    InitRegisterEdit(m_FPRTab, m_FCSREdit, FPRFCSR);

    InitRegisterEdits(m_COP0Tab, m_COP0Edits, &TabData::COP0);
    m_CauseTip.Attach(m_COP0Tab.GetDlgItem(IDC_CAUSE_TIP));

    InitRegisterEdits(m_RDRAMTab, m_RDRAMEdits, &TabData::RDRAM);

    InitRegisterEdits(m_SPTab, m_SPEdits, &TabData::SP);
    InitRegisterEdit(m_SPTab, m_SPPCEdit, SPPC);

    InitRegisterEdits(m_DPCTab, m_DPCEdits, &TabData::DPC);

    InitRegisterEdits(m_MITab, m_MIEdits, &TabData::MI);

    InitRegisterEdits(m_VITab, m_VIEdits, &TabData::VI);

    InitRegisterEdits(m_AITab, m_AIEdits, &TabData::AI);

    InitRegisterEdits(m_PITab, m_PIEdits, &TabData::PI);

    InitRegisterEdits(m_RITab, m_RIEdits, &TabData::RI);

    InitRegisterEdits(m_SITab, m_SIEdits, &TabData::SI);

    InitRegisterEdits(m_DDTab, m_DDEdits, &TabData::DD);

    SetColorsEnabled(false);
    RefreshEdits();
    RedrawCurrentTab();
    m_attached = true;
}

HWND CRegisterTabs::Detach(void)
{
    m_attached = false;
    m_GPRTab = nullptr;
    m_FPRTab = nullptr;
    m_COP0Tab = nullptr;
    m_RDRAMTab = nullptr;
    m_SPTab = nullptr;
    m_DPCTab = nullptr;
    m_MITab = nullptr;
    m_VITab = nullptr;
    m_AITab = nullptr;
    m_PITab = nullptr;
    m_RITab = nullptr;
    m_SITab = nullptr;
    m_DDTab = nullptr;
    for (size_t i = 0; i < m_TabWindows.size(); i++)
    {
        m_TabWindows[i].DestroyWindow();
    }
    m_TabWindows.clear();
    m_CauseTip.Detach();
    CTabCtrl::Detach();
    return nullptr;
}

void CRegisterTabs::RefreshEdits()
{
    if (g_Reg == nullptr)
    {
        ZeroRegisterEdits64(m_GPREdits, TabData::GPR.FieldCount);
        ZeroRegisterEdit64(m_HIEdit);
        ZeroRegisterEdit64(m_LOEdit);
        ZeroRegisterEdits(m_FPREdits, TabData::FPR.FieldCount);
        ZeroRegisterEdit(m_FCSREdit);
        ZeroRegisterEdits(m_COP0Edits, TabData::COP0.FieldCount);
        ZeroRegisterEdits(m_RDRAMEdits, TabData::RDRAM.FieldCount);
        ZeroRegisterEdits(m_SPEdits, TabData::SP.FieldCount);
        ZeroRegisterEdit(m_SPPCEdit);
        ZeroRegisterEdits(m_DPCEdits, TabData::DPC.FieldCount);
        ZeroRegisterEdits(m_MIEdits, TabData::MI.FieldCount);
        ZeroRegisterEdits(m_VIEdits, TabData::VI.FieldCount);
        ZeroRegisterEdits(m_AIEdits, TabData::AI.FieldCount);
        ZeroRegisterEdits(m_PIEdits, TabData::PI.FieldCount);
        ZeroRegisterEdits(m_RIEdits, TabData::RI.FieldCount);
        ZeroRegisterEdits(m_SIEdits, TabData::SI.FieldCount);
        ZeroRegisterEdits(m_DDEdits, TabData::DD.FieldCount);
        return;
    }

    for (int i = 0; i < 32; i++)
    {
        m_GPREdits[i].SetValue(g_Reg->m_GPR[i].UDW);
        m_FPREdits[i].SetValue(*(uint32_t *)g_Reg->m_FPR_S[i], DisplayMode::ZeroExtend);
    }
    m_HIEdit.SetValue(g_Reg->m_HI.UDW);
    m_LOEdit.SetValue(g_Reg->m_LO.UDW);

    m_FCSREdit.SetValue(g_Reg->m_FPCR[31], DisplayMode::ZeroExtend);

    m_COP0Edits[0].SetValue(g_Reg->INDEX_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[1].SetValue(g_Reg->RANDOM_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[2].SetValue(g_Reg->ENTRYLO0_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[3].SetValue(g_Reg->ENTRYLO1_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[4].SetValue(g_Reg->CONTEXT_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[5].SetValue(g_Reg->PAGE_MASK_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[6].SetValue(g_Reg->WIRED_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[7].SetValue(g_Reg->BAD_VADDR_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[8].SetValue(g_Reg->COUNT_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[9].SetValue(g_Reg->ENTRYHI_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[10].SetValue(g_Reg->COMPARE_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[11].SetValue(g_Reg->STATUS_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[12].SetValue(g_Reg->CAUSE_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[13].SetValue(g_Reg->EPC_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[14].SetValue(g_Reg->CONFIG_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[15].SetValue(g_Reg->TAGLO_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[16].SetValue(g_Reg->TAGHI_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[17].SetValue(g_Reg->ERROREPC_REGISTER, DisplayMode::ZeroExtend);
    m_COP0Edits[18].SetValue(g_Reg->FAKE_CAUSE_REGISTER, DisplayMode::ZeroExtend);

    CAUSE cause;
    cause.intval = g_Reg->CAUSE_REGISTER;

    const char* szExceptionCode = ExceptionCodes[cause.exceptionCode];
    m_CauseTip.SetWindowText(stdstr(szExceptionCode).ToUTF16().c_str());

    m_RDRAMEdits[0].SetValue(g_Reg->RDRAM_CONFIG_REG, DisplayMode::ZeroExtend); // or device type
    m_RDRAMEdits[1].SetValue(g_Reg->RDRAM_DEVICE_ID_REG, DisplayMode::ZeroExtend);
    m_RDRAMEdits[2].SetValue(g_Reg->RDRAM_DELAY_REG, DisplayMode::ZeroExtend);
    m_RDRAMEdits[3].SetValue(g_Reg->RDRAM_MODE_REG, DisplayMode::ZeroExtend);
    m_RDRAMEdits[4].SetValue(g_Reg->RDRAM_REF_INTERVAL_REG, DisplayMode::ZeroExtend);
    m_RDRAMEdits[5].SetValue(g_Reg->RDRAM_REF_ROW_REG, DisplayMode::ZeroExtend);
    m_RDRAMEdits[6].SetValue(g_Reg->RDRAM_RAS_INTERVAL_REG, DisplayMode::ZeroExtend);
    m_RDRAMEdits[7].SetValue(g_Reg->RDRAM_MIN_INTERVAL_REG, DisplayMode::ZeroExtend);
    m_RDRAMEdits[8].SetValue(g_Reg->RDRAM_ADDR_SELECT_REG, DisplayMode::ZeroExtend);
    m_RDRAMEdits[9].SetValue(g_Reg->RDRAM_DEVICE_MANUF_REG, DisplayMode::ZeroExtend);

    m_SPEdits[0].SetValue(g_Reg->SP_MEM_ADDR_REG, DisplayMode::ZeroExtend);
    m_SPEdits[1].SetValue(g_Reg->SP_DRAM_ADDR_REG, DisplayMode::ZeroExtend);
    m_SPEdits[2].SetValue(g_Reg->SP_RD_LEN_REG, DisplayMode::ZeroExtend);
    m_SPEdits[3].SetValue(g_Reg->SP_WR_LEN_REG, DisplayMode::ZeroExtend);
    m_SPEdits[4].SetValue(g_Reg->SP_STATUS_REG, DisplayMode::ZeroExtend);
    m_SPEdits[5].SetValue(g_Reg->SP_DMA_FULL_REG, DisplayMode::ZeroExtend);
    m_SPEdits[6].SetValue(g_Reg->SP_DMA_BUSY_REG, DisplayMode::ZeroExtend);
    m_SPEdits[7].SetValue(g_Reg->SP_SEMAPHORE_REG, DisplayMode::ZeroExtend);
    m_SPPCEdit.SetValue(g_Reg->SP_PC_REG, DisplayMode::ZeroExtend);

    m_DPCEdits[0].SetValue(g_Reg->DPC_START_REG, DisplayMode::ZeroExtend);
    m_DPCEdits[1].SetValue(g_Reg->DPC_END_REG, DisplayMode::ZeroExtend);
    m_DPCEdits[2].SetValue(g_Reg->DPC_CURRENT_REG, DisplayMode::ZeroExtend);
    m_DPCEdits[3].SetValue(g_Reg->DPC_STATUS_REG, DisplayMode::ZeroExtend);
    m_DPCEdits[4].SetValue(g_Reg->DPC_CLOCK_REG, DisplayMode::ZeroExtend);
    m_DPCEdits[5].SetValue(g_Reg->DPC_BUFBUSY_REG, DisplayMode::ZeroExtend);
    m_DPCEdits[6].SetValue(g_Reg->DPC_PIPEBUSY_REG, DisplayMode::ZeroExtend);
    m_DPCEdits[7].SetValue(g_Reg->DPC_TMEM_REG, DisplayMode::ZeroExtend);

    m_MIEdits[0].SetValue(g_Reg->MI_INIT_MODE_REG, DisplayMode::ZeroExtend);
    m_MIEdits[1].SetValue(g_Reg->MI_VERSION_REG, DisplayMode::ZeroExtend);
    m_MIEdits[2].SetValue(g_Reg->MI_INTR_REG, DisplayMode::ZeroExtend);
    m_MIEdits[3].SetValue(g_Reg->MI_INTR_MASK_REG, DisplayMode::ZeroExtend);

    m_VIEdits[0].SetValue(g_Reg->VI_STATUS_REG, DisplayMode::ZeroExtend);
    m_VIEdits[1].SetValue(g_Reg->VI_ORIGIN_REG, DisplayMode::ZeroExtend);
    m_VIEdits[2].SetValue(g_Reg->VI_WIDTH_REG, DisplayMode::ZeroExtend);
    m_VIEdits[3].SetValue(g_Reg->VI_INTR_REG, DisplayMode::ZeroExtend);
    m_VIEdits[4].SetValue(g_Reg->VI_CURRENT_REG, DisplayMode::ZeroExtend);
    m_VIEdits[5].SetValue(g_Reg->VI_BURST_REG, DisplayMode::ZeroExtend);
    m_VIEdits[6].SetValue(g_Reg->VI_V_SYNC_REG, DisplayMode::ZeroExtend);
    m_VIEdits[7].SetValue(g_Reg->VI_H_SYNC_REG, DisplayMode::ZeroExtend);
    m_VIEdits[8].SetValue(g_Reg->VI_LEAP_REG, DisplayMode::ZeroExtend);
    m_VIEdits[9].SetValue(g_Reg->VI_H_START_REG, DisplayMode::ZeroExtend);
    m_VIEdits[10].SetValue(g_Reg->VI_V_START_REG, DisplayMode::ZeroExtend);
    m_VIEdits[11].SetValue(g_Reg->VI_V_BURST_REG, DisplayMode::ZeroExtend);
    m_VIEdits[12].SetValue(g_Reg->VI_X_SCALE_REG, DisplayMode::ZeroExtend);
    m_VIEdits[13].SetValue(g_Reg->VI_Y_SCALE_REG, DisplayMode::ZeroExtend);

    m_AIEdits[0].SetValue(g_Reg->AI_DRAM_ADDR_REG, DisplayMode::ZeroExtend);
    m_AIEdits[1].SetValue(g_Reg->AI_LEN_REG, DisplayMode::ZeroExtend);
    m_AIEdits[2].SetValue(g_Reg->AI_CONTROL_REG, DisplayMode::ZeroExtend);
    m_AIEdits[3].SetValue(g_Reg->AI_STATUS_REG, DisplayMode::ZeroExtend);
    m_AIEdits[4].SetValue(g_Reg->AI_DACRATE_REG, DisplayMode::ZeroExtend);
    m_AIEdits[5].SetValue(g_Reg->AI_BITRATE_REG, DisplayMode::ZeroExtend);

    m_PIEdits[0].SetValue(g_Reg->PI_DRAM_ADDR_REG, DisplayMode::ZeroExtend);
    m_PIEdits[1].SetValue(g_Reg->PI_CART_ADDR_REG, DisplayMode::ZeroExtend);
    m_PIEdits[2].SetValue(g_Reg->PI_RD_LEN_REG, DisplayMode::ZeroExtend);
    m_PIEdits[3].SetValue(g_Reg->PI_WR_LEN_REG, DisplayMode::ZeroExtend);
    m_PIEdits[4].SetValue(g_Reg->PI_STATUS_REG, DisplayMode::ZeroExtend);
    m_PIEdits[5].SetValue(g_Reg->PI_BSD_DOM1_LAT_REG, DisplayMode::ZeroExtend);
    m_PIEdits[6].SetValue(g_Reg->PI_BSD_DOM1_PWD_REG, DisplayMode::ZeroExtend);
    m_PIEdits[7].SetValue(g_Reg->PI_BSD_DOM1_PGS_REG, DisplayMode::ZeroExtend);
    m_PIEdits[8].SetValue(g_Reg->PI_BSD_DOM1_RLS_REG, DisplayMode::ZeroExtend);
    m_PIEdits[9].SetValue(g_Reg->PI_BSD_DOM2_LAT_REG, DisplayMode::ZeroExtend);
    m_PIEdits[10].SetValue(g_Reg->PI_BSD_DOM2_PWD_REG, DisplayMode::ZeroExtend);
    m_PIEdits[11].SetValue(g_Reg->PI_BSD_DOM2_PGS_REG, DisplayMode::ZeroExtend);
    m_PIEdits[12].SetValue(g_Reg->PI_BSD_DOM2_RLS_REG, DisplayMode::ZeroExtend);

    m_RIEdits[0].SetValue(g_Reg->RI_MODE_REG, DisplayMode::ZeroExtend);
    m_RIEdits[1].SetValue(g_Reg->RI_CONFIG_REG, DisplayMode::ZeroExtend);
    m_RIEdits[2].SetValue(g_Reg->RI_CURRENT_LOAD_REG, DisplayMode::ZeroExtend);
    m_RIEdits[3].SetValue(g_Reg->RI_SELECT_REG, DisplayMode::ZeroExtend);
    m_RIEdits[4].SetValue(g_Reg->RI_REFRESH_REG, DisplayMode::ZeroExtend); // or ri count
    m_RIEdits[5].SetValue(g_Reg->RI_LATENCY_REG, DisplayMode::ZeroExtend);
    m_RIEdits[6].SetValue(g_Reg->RI_RERROR_REG, DisplayMode::ZeroExtend);
    m_RIEdits[7].SetValue(g_Reg->RI_WERROR_REG, DisplayMode::ZeroExtend);

    m_SIEdits[0].SetValue(g_Reg->SI_DRAM_ADDR_REG, DisplayMode::ZeroExtend);
    m_SIEdits[1].SetValue(g_Reg->SI_PIF_ADDR_RD64B_REG, DisplayMode::ZeroExtend);
    m_SIEdits[2].SetValue(g_Reg->SI_PIF_ADDR_WR64B_REG, DisplayMode::ZeroExtend);
    m_SIEdits[3].SetValue(g_Reg->SI_STATUS_REG, DisplayMode::ZeroExtend);

    m_DDEdits[0].SetValue(g_Reg->ASIC_DATA, DisplayMode::ZeroExtend);
    m_DDEdits[1].SetValue(g_Reg->ASIC_MISC_REG, DisplayMode::ZeroExtend);
    m_DDEdits[2].SetValue(g_Reg->ASIC_STATUS, DisplayMode::ZeroExtend);
    m_DDEdits[3].SetValue(g_Reg->ASIC_CUR_TK, DisplayMode::ZeroExtend);
    m_DDEdits[4].SetValue(g_Reg->ASIC_BM_STATUS, DisplayMode::ZeroExtend);
    m_DDEdits[5].SetValue(g_Reg->ASIC_ERR_SECTOR, DisplayMode::ZeroExtend);
    m_DDEdits[6].SetValue(g_Reg->ASIC_SEQ_STATUS, DisplayMode::ZeroExtend);
    m_DDEdits[7].SetValue(g_Reg->ASIC_CUR_SECTOR, DisplayMode::ZeroExtend);
    m_DDEdits[8].SetValue(g_Reg->ASIC_HARD_RESET, DisplayMode::ZeroExtend);
    m_DDEdits[9].SetValue(g_Reg->ASIC_C1_S0, DisplayMode::ZeroExtend);
    m_DDEdits[10].SetValue(g_Reg->ASIC_HOST_SECBYTE, DisplayMode::ZeroExtend);
    m_DDEdits[11].SetValue(g_Reg->ASIC_C1_S2, DisplayMode::ZeroExtend);
    m_DDEdits[12].SetValue(g_Reg->ASIC_SEC_BYTE, DisplayMode::ZeroExtend);
    m_DDEdits[13].SetValue(g_Reg->ASIC_C1_S4, DisplayMode::ZeroExtend);
    m_DDEdits[14].SetValue(g_Reg->ASIC_C1_S6, DisplayMode::ZeroExtend);
    m_DDEdits[15].SetValue(g_Reg->ASIC_CUR_ADDR, DisplayMode::ZeroExtend);
    m_DDEdits[16].SetValue(g_Reg->ASIC_ID_REG, DisplayMode::ZeroExtend);
    m_DDEdits[17].SetValue(g_Reg->ASIC_TEST_REG, DisplayMode::ZeroExtend);
    m_DDEdits[18].SetValue(g_Reg->ASIC_TEST_PIN_SEL, DisplayMode::ZeroExtend);
}

void CRegisterTabs::RegisterChanged(HWND hDlg, TAB_ID srcTabId, WPARAM wParam)
{
    if (g_Reg == nullptr || !isStepping())
    {
        return;
    }

    WORD ctrlId = LOWORD(wParam);

    CWindow editCtrl = ::GetDlgItem(hDlg, ctrlId);
    wchar_t text[20];
    editCtrl.GetWindowText(text, 20);

    if (srcTabId == TabGPR)
    {
        uint64_t value = CEditReg64::ParseValue(stdstr().FromUTF16(text).c_str());
        if (ctrlId == IDC_HI_EDIT)
        {
            g_Reg->m_HI.UDW = value;
        }
        else if (ctrlId == IDC_LO_EDIT)
        {
            g_Reg->m_LO.UDW = value;
        }
        else
        {
            int nReg = TabData::GPR.GetEditIndex(ctrlId);
            g_Reg->m_GPR[nReg].UDW = value;
        }
        return;
    }

    uint32_t value = wcstoul(text, nullptr, 16);
    wsprintf(text, L"%08X", value);
    editCtrl.SetWindowText(text); // Reformat text

    if (srcTabId == TabFPR)
    {
        if (ctrlId == IDC_FCSR_EDIT)
        {
            g_Reg->m_FPCR[31] = value;
            return;
        }

        int nReg = TabData::FPR.GetEditIndex(ctrlId);
        *(uint32_t*)g_Reg->m_FPR_S[nReg] = value;
        return;
    }

    switch (ctrlId)
    {
    case IDC_COP0_0_EDIT:  g_Reg->INDEX_REGISTER = value; break;
    case IDC_COP0_1_EDIT:  g_Reg->RANDOM_REGISTER = value; break;
    case IDC_COP0_2_EDIT:  g_Reg->ENTRYLO0_REGISTER = value; break;
    case IDC_COP0_3_EDIT:  g_Reg->ENTRYLO1_REGISTER = value; break;
    case IDC_COP0_4_EDIT:  g_Reg->CONTEXT_REGISTER = value; break;
    case IDC_COP0_5_EDIT:  g_Reg->PAGE_MASK_REGISTER = value; break;
    case IDC_COP0_6_EDIT:  g_Reg->WIRED_REGISTER = value; break;
    case IDC_COP0_7_EDIT:  g_Reg->BAD_VADDR_REGISTER = value; break;
    case IDC_COP0_8_EDIT:  g_Reg->COUNT_REGISTER = value; break;
    case IDC_COP0_9_EDIT:  g_Reg->ENTRYHI_REGISTER = value; break;
    case IDC_COP0_10_EDIT: g_Reg->COMPARE_REGISTER = value; break;
    case IDC_COP0_11_EDIT: g_Reg->STATUS_REGISTER = value; break;
    case IDC_COP0_12_EDIT: g_Reg->CAUSE_REGISTER = value; break;
    case IDC_COP0_13_EDIT: g_Reg->EPC_REGISTER = value; break;
    case IDC_COP0_14_EDIT: g_Reg->CONFIG_REGISTER = value; break;
    case IDC_COP0_15_EDIT: g_Reg->TAGLO_REGISTER = value; break;
    case IDC_COP0_16_EDIT: g_Reg->TAGHI_REGISTER = value; break;
    case IDC_COP0_17_EDIT: g_Reg->ERROREPC_REGISTER = value; break;
    case IDC_COP0_18_EDIT: g_Reg->FAKE_CAUSE_REGISTER = value; break;

    case IDC_RDRAM00_EDIT: g_Reg->RDRAM_CONFIG_REG = value; break; // or device_type
    case IDC_RDRAM04_EDIT: g_Reg->RDRAM_DEVICE_ID_REG = value; break;
    case IDC_RDRAM08_EDIT: g_Reg->RDRAM_DELAY_REG = value; break;
    case IDC_RDRAM0C_EDIT: g_Reg->RDRAM_MODE_REG = value; break;
    case IDC_RDRAM10_EDIT: g_Reg->RDRAM_REF_INTERVAL_REG = value; break;
    case IDC_RDRAM14_EDIT: g_Reg->RDRAM_REF_ROW_REG = value; break;
    case IDC_RDRAM18_EDIT: g_Reg->RDRAM_RAS_INTERVAL_REG = value; break;
    case IDC_RDRAM1C_EDIT: g_Reg->RDRAM_MIN_INTERVAL_REG = value; break;
    case IDC_RDRAM20_EDIT: g_Reg->RDRAM_ADDR_SELECT_REG = value; break;
    case IDC_RDRAM24_EDIT: g_Reg->RDRAM_DEVICE_MANUF_REG = value; break;

    case IDC_SP00_EDIT:  g_Reg->SP_MEM_ADDR_REG = value; break;
    case IDC_SP04_EDIT:  g_Reg->SP_DRAM_ADDR_REG = value; break;
    case IDC_SP08_EDIT:  g_Reg->SP_RD_LEN_REG = value; break;
    case IDC_SP0C_EDIT:  g_Reg->SP_WR_LEN_REG = value; break;
    case IDC_SP10_EDIT:  g_Reg->SP_STATUS_REG = value; break;
    case IDC_SP14_EDIT:  g_Reg->SP_DMA_FULL_REG = value; break;
    case IDC_SP18_EDIT:  g_Reg->SP_DMA_BUSY_REG = value; break;
    case IDC_SP1C_EDIT:  g_Reg->SP_SEMAPHORE_REG = value; break;
    case IDC_SP_PC_EDIT: g_Reg->SP_PC_REG = value; break;

    case IDC_DPC00_EDIT: g_Reg->DPC_START_REG = value; break;
    case IDC_DPC04_EDIT: g_Reg->DPC_END_REG = value; break;
    case IDC_DPC08_EDIT: g_Reg->DPC_CURRENT_REG = value; break;
    case IDC_DPC0C_EDIT: g_Reg->DPC_STATUS_REG = value; break;
    case IDC_DPC10_EDIT: g_Reg->DPC_CLOCK_REG = value; break;
    case IDC_DPC14_EDIT: g_Reg->DPC_BUFBUSY_REG = value; break;
    case IDC_DPC18_EDIT: g_Reg->DPC_PIPEBUSY_REG = value; break;
    case IDC_DPC1C_EDIT: g_Reg->DPC_TMEM_REG = value; break;

    case IDC_MI00_EDIT: g_Reg->MI_INIT_MODE_REG = value; break; // or MI_MODE ?
    case IDC_MI04_EDIT: g_Reg->MI_VERSION_REG = value; break; // or MI_NOOP ?
    case IDC_MI08_EDIT: g_Reg->MI_INTR_REG = value; break; // or MI_INTR ?
    case IDC_MI0C_EDIT: g_Reg->MI_INTR_MASK_REG = value; break; // or MI_INTR_MASK ?

    case IDC_VI00_EDIT: g_Reg->VI_STATUS_REG = value; break;
    case IDC_VI04_EDIT: g_Reg->VI_ORIGIN_REG = value; break;
    case IDC_VI08_EDIT: g_Reg->VI_WIDTH_REG = value; break;
    case IDC_VI0C_EDIT: g_Reg->VI_INTR_REG = value; break;
    case IDC_VI10_EDIT: g_Reg->VI_CURRENT_REG = value; break;
    case IDC_VI14_EDIT: g_Reg->VI_BURST_REG = value; break;
    case IDC_VI18_EDIT: g_Reg->VI_V_SYNC_REG = value; break;
    case IDC_VI1C_EDIT: g_Reg->VI_H_SYNC_REG = value; break;
    case IDC_VI20_EDIT: g_Reg->VI_LEAP_REG = value; break;
    case IDC_VI24_EDIT: g_Reg->VI_H_START_REG = value; break;
    case IDC_VI28_EDIT: g_Reg->VI_V_START_REG = value; break;
    case IDC_VI2C_EDIT: g_Reg->VI_V_BURST_REG = value; break;
    case IDC_VI30_EDIT: g_Reg->VI_X_SCALE_REG = value; break;
    case IDC_VI34_EDIT: g_Reg->VI_Y_SCALE_REG = value; break;

    case IDC_AI00_EDIT: g_Reg->AI_DRAM_ADDR_REG = value; break;
    case IDC_AI04_EDIT: g_Reg->AI_LEN_REG = value; break;
    case IDC_AI08_EDIT: g_Reg->AI_CONTROL_REG = value; break;
    case IDC_AI0C_EDIT: g_Reg->AI_STATUS_REG = value; break;
    case IDC_AI10_EDIT: g_Reg->AI_DACRATE_REG = value; break;
    case IDC_AI14_EDIT: g_Reg->AI_BITRATE_REG = value; break;

    case IDC_PI00_EDIT: g_Reg->PI_DRAM_ADDR_REG = value; break;
    case IDC_PI04_EDIT: g_Reg->PI_CART_ADDR_REG = value; break;
    case IDC_PI08_EDIT: g_Reg->PI_RD_LEN_REG = value; break;
    case IDC_PI0C_EDIT: g_Reg->PI_WR_LEN_REG = value; break;
    case IDC_PI10_EDIT: g_Reg->PI_STATUS_REG = value; break;
    case IDC_PI14_EDIT: g_Reg->PI_BSD_DOM1_LAT_REG = value; break;
    case IDC_PI18_EDIT: g_Reg->PI_BSD_DOM1_PWD_REG = value; break;
    case IDC_PI1C_EDIT: g_Reg->PI_BSD_DOM1_PGS_REG = value; break;
    case IDC_PI20_EDIT: g_Reg->PI_BSD_DOM1_RLS_REG = value; break;
    case IDC_PI24_EDIT: g_Reg->PI_BSD_DOM2_LAT_REG = value; break;
    case IDC_PI28_EDIT: g_Reg->PI_BSD_DOM2_PWD_REG = value; break;
    case IDC_PI2C_EDIT: g_Reg->PI_BSD_DOM2_PGS_REG = value; break;
    case IDC_PI30_EDIT: g_Reg->PI_BSD_DOM2_RLS_REG = value; break;

    case IDC_RI00_EDIT: g_Reg->RI_MODE_REG = value; break;
    case IDC_RI04_EDIT: g_Reg->RI_CONFIG_REG = value; break;
    case IDC_RI08_EDIT: g_Reg->RI_CURRENT_LOAD_REG = value; break;
    case IDC_RI0C_EDIT: g_Reg->RI_SELECT_REG = value; break;
    case IDC_RI10_EDIT: g_Reg->RI_REFRESH_REG = value; break;
    case IDC_RI14_EDIT: g_Reg->RI_LATENCY_REG = value; break;
    case IDC_RI18_EDIT: g_Reg->RI_RERROR_REG = value; break;
    case IDC_RI1C_EDIT: g_Reg->RI_WERROR_REG = value; break;

    case IDC_SI00_EDIT: g_Reg->SI_DRAM_ADDR_REG = value; break;
    case IDC_SI04_EDIT: g_Reg->SI_PIF_ADDR_RD64B_REG = value; break;
    case IDC_SI08_EDIT: g_Reg->SI_PIF_ADDR_WR64B_REG = value; break;
    case IDC_SI0C_EDIT: g_Reg->SI_STATUS_REG = value; break;

    case IDC_DD00_EDIT: g_Reg->ASIC_DATA = value; break;
    case IDC_DD04_EDIT: g_Reg->ASIC_MISC_REG = value; break;
    case IDC_DD08_EDIT: g_Reg->ASIC_STATUS = value; break;
    case IDC_DD0C_EDIT: g_Reg->ASIC_CUR_TK = value; break;
    case IDC_DD10_EDIT: g_Reg->ASIC_BM_STATUS = value; break;
    case IDC_DD14_EDIT: g_Reg->ASIC_ERR_SECTOR = value; break;
    case IDC_DD18_EDIT: g_Reg->ASIC_SEQ_STATUS = value; break;
    case IDC_DD1C_EDIT: g_Reg->ASIC_CUR_SECTOR = value; break;
    case IDC_DD20_EDIT: g_Reg->ASIC_HARD_RESET = value; break;
    case IDC_DD24_EDIT: g_Reg->ASIC_C1_S0 = value; break;
    case IDC_DD28_EDIT: g_Reg->ASIC_HOST_SECBYTE = value; break;
    case IDC_DD2C_EDIT: g_Reg->ASIC_C1_S2 = value; break;
    case IDC_DD30_EDIT: g_Reg->ASIC_SEC_BYTE = value; break;
    case IDC_DD34_EDIT: g_Reg->ASIC_C1_S4 = value; break;
    case IDC_DD38_EDIT: g_Reg->ASIC_C1_S6 = value; break;
    case IDC_DD3C_EDIT: g_Reg->ASIC_CUR_ADDR = value; break;
    case IDC_DD40_EDIT: g_Reg->ASIC_ID_REG = value; break;
    case IDC_DD44_EDIT: g_Reg->ASIC_TEST_REG = value; break;
    case IDC_DD48_EDIT: g_Reg->ASIC_TEST_PIN_SEL = value; break;
    }
}

INT_PTR CALLBACK CRegisterTabs::TabProcDefault(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INITDIALOG)
    {
        SetProp(hDlg, L"attached", (HANDLE)lParam);
        return TRUE;
    }
    if (msg == WM_COMMAND && HIWORD(wParam) == EN_KILLFOCUS)
    {
        bool * attached = (bool *)GetProp(hDlg, L"attached");
        if (attached != nullptr && *attached)
        {
            RegisterChanged(hDlg, TabDefault, wParam);
        }
    }

    return FALSE;
}

INT_PTR CALLBACK CRegisterTabs::TabProcGPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INITDIALOG)
    {
        SetProp(hDlg, L"attached", (HANDLE)lParam);
        return TRUE;
    }

    // Color textboxes
    if (msg == WM_CTLCOLOREDIT)
    {
        HDC hdc = (HDC)wParam;
        COLORREF colorBg = RGB(255, 255, 255);

        COLORREF colorRead = RGB(200, 200, 255);
        COLORREF colorWrite = RGB(255, 200, 200);
        COLORREF colorBoth = RGB(220, 170, 255);

        if (!m_bColorsEnabled || g_Reg == nullptr || g_MMU == nullptr)
        {
            return FALSE;
        }

        HWND hWnd = (HWND)lParam;
        WORD ctrlId = (WORD) ::GetWindowLong(hWnd, GWL_ID);

        COpInfo opInfo;
        m_Debugger->DebugLoad_VAddr(g_Reg->m_PROGRAM_COUNTER, opInfo.m_OpCode.Hex);

        bool bOpReads = false;
        bool bOpWrites = false;

        if (ctrlId == IDC_LO_EDIT)
        {
            bOpReads = opInfo.ReadsLO();
            bOpWrites = !bOpReads && opInfo.WritesLO();
        }
        else if (ctrlId == IDC_HI_EDIT)
        {
            bOpReads = opInfo.ReadsHI();
            bOpWrites = !bOpReads && opInfo.WritesHI();
        }
        else
        {
            int nReg = TabData::GPR.GetEditIndex(ctrlId);

            if (nReg == -1)
            {
                return (LRESULT)GetStockObject(DC_BRUSH);
            }

            int nRegRead1, nRegRead2, nRegWrite;

            opInfo.ReadsGPR(&nRegRead1, &nRegRead2);
            opInfo.WritesGPR(&nRegWrite);

            bOpReads = (nReg == nRegRead1) || (nReg == nRegRead2);
            bOpWrites = (nReg == nRegWrite);
        }

        if (bOpReads && bOpWrites)
        {
            colorBg = colorBoth;
        }
        else if (bOpReads)
        {
            colorBg = colorRead;
        }
        else if (bOpWrites)
        {
            colorBg = colorWrite;
        }

        SetBkColor(hdc, colorBg);
        SetDCBrushColor(hdc, colorBg);
        return (LRESULT)GetStockObject(DC_BRUSH);
    }

    if (msg == WM_COMMAND && HIWORD(wParam) == EN_KILLFOCUS)
    {
        bool * attached = (bool *)GetProp(hDlg, L"attached");
        if (attached != nullptr && *attached)
        {
            RegisterChanged(hDlg, TabGPR, wParam);
        }

        return FALSE;
    }

    // Right click labels
    if (msg == WM_CONTEXTMENU)
    {
        if (m_Debugger == nullptr)
        {
            return FALSE;
        }

        HWND hWnd = (HWND)wParam;
        WORD ctrlId = (WORD) ::GetWindowLong(hWnd, GWL_ID);

        CBreakpoints* breakpoints = m_Debugger->Breakpoints();

        if (ctrlId == IDC_HI_LBL)
        {
            breakpoints->ToggleHIWriteBP();
        }
        else if (ctrlId == IDC_LO_LBL)
        {
            breakpoints->ToggleLOWriteBP();
        }
        else
        {
            int nReg = TabData::GPR.GetLabelIndex(ctrlId);

            if (nReg <= 0) // Ignore R0
            {
                return FALSE;
            }

            breakpoints->ToggleGPRWriteBP(nReg);
        }

        ::InvalidateRect(hWnd, nullptr, true);
        return FALSE;
    }

    // Click labels
    if (msg == WM_COMMAND && HIWORD(wParam) == STN_CLICKED || HIWORD(wParam) == STN_DBLCLK)
    {
        if (m_Debugger == nullptr)
        {
            return FALSE;
        }

        HWND hWnd = (HWND)lParam;
        WORD ctrlId = LOWORD(wParam);

        CBreakpoints* breakpoints = m_Debugger->Breakpoints();

        if (ctrlId == IDC_HI_LBL)
        {
            breakpoints->ToggleHIReadBP();
        }
        else if (ctrlId == IDC_LO_LBL)
        {
            breakpoints->ToggleLOReadBP();
        }
        else
        {
            int nReg = TabData::GPR.GetLabelIndex(ctrlId);

            if (nReg <= 0) // Ignore R0
            {
                return FALSE;
            }

            breakpoints->ToggleGPRReadBP(nReg);
        }

        ::InvalidateRect(hWnd, nullptr, true);
        return FALSE;
    }

    // Color labels
    if (msg == WM_CTLCOLORSTATIC)
    {
        if (m_Debugger == nullptr)
        {
            return FALSE;
        }

        HWND hWnd = (HWND)lParam;
        WORD ctrlId = (WORD) ::GetWindowLong(hWnd, GWL_ID);
        

        HDC hdc = (HDC)wParam;

        COLORREF colorRead = RGB(200, 200, 255);
        COLORREF colorWrite = RGB(255, 200, 200);
        COLORREF colorBoth = RGB(220, 170, 255);

        CBreakpoints* breakpoints = m_Debugger->Breakpoints();
        
        bool haveRead, haveWrite;

        if (ctrlId == IDC_HI_LBL)
        {
            haveRead = breakpoints->HaveHIReadBP();
            haveWrite = breakpoints->HaveHIWriteBP();
        }
        else if (ctrlId == IDC_LO_LBL)
        {
            haveRead = breakpoints->HaveLOReadBP();
            haveWrite = breakpoints->HaveLOWriteBP();
        }
        else
        {
            int nReg = TabData::GPR.GetLabelIndex(ctrlId);

            if (nReg == -1)
            {
                return FALSE;
            }

            haveRead = breakpoints->HaveGPRReadBP(nReg);
            haveWrite = breakpoints->HaveGPRWriteBP(nReg);
        }

        if (haveRead && haveWrite)
        {
            SetBkColor(hdc, colorBoth);
        }
        else if(haveRead)
        {
            SetBkColor(hdc, colorRead);
        }
        else if(haveWrite)
        {
            SetBkColor(hdc, colorWrite);
        }
        else
        {
            return FALSE;
        }

        return (LRESULT)GetStockObject(DC_BRUSH);
    }
    
    return FALSE;
}

INT_PTR CALLBACK CRegisterTabs::TabProcFPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INITDIALOG)
    {
        SetProp(hDlg, L"attached", (HANDLE)lParam);
        return TRUE;
    }
    if (msg == WM_COMMAND && HIWORD(wParam) == EN_KILLFOCUS)
    {
        bool * attached = (bool *)GetProp(hDlg, L"attached");
        if (attached != nullptr && *attached)
        {
            RegisterChanged(hDlg, TabFPR, wParam);
        }
    }
    return FALSE;
}

CRect CRegisterTabs::GetPageRect()
{
    CWindow parentWin = GetParent();
    CRect pageRect;
    GetWindowRect(&pageRect);
    parentWin.ScreenToClient(&pageRect);
    AdjustRect(FALSE, &pageRect);
    return pageRect;
}

CWindow CRegisterTabs::AddTab(char* caption, int dialogId, DLGPROC dlgProc)
{
    AddItem(stdstr(caption).ToUTF16().c_str());

    CWindow parentWin = GetParent();
    CWindow tabWin = ::CreateDialogParam(nullptr, MAKEINTRESOURCE(dialogId), parentWin, dlgProc, (LPARAM)&m_attached);

    CRect pageRect = GetPageRect();

    ::SetParent(tabWin, parentWin);

    ::SetWindowPos(
        tabWin,
        m_hWnd,
        pageRect.left,
        pageRect.top,
        pageRect.Width(),
        pageRect.Height(),
        SWP_HIDEWINDOW
    );

    m_TabWindows.push_back(tabWin);

    int index = m_TabWindows.size() - 1;

    if (index == 0)
    {
        ShowTab(0);
    }

    return tabWin;
}

void CRegisterTabs::ShowTab(int nPage)
{
    for (size_t i = 0; i < m_TabWindows.size(); i++)
    {
        ::ShowWindow(m_TabWindows[i], SW_HIDE);
    }

    CRect pageRect = GetPageRect();

    ::SetWindowPos(m_TabWindows[nPage], HWND_TOP, pageRect.left, pageRect.top, pageRect.Width(), pageRect.Height(), SWP_SHOWWINDOW);
}

void CRegisterTabs::RedrawCurrentTab()
{
    int nPage = GetCurSel();
    CRect pageRect = GetPageRect();

    ::SetWindowPos(m_TabWindows[nPage], HWND_TOP, pageRect.left, pageRect.top, pageRect.Width(), pageRect.Height(), SWP_SHOWWINDOW);
}

void CRegisterTabs::SetColorsEnabled(bool bColorsEnabled)
{
    m_bColorsEnabled = bColorsEnabled;
}

void CRegisterTabs::InitRegisterEdit(CWindow& tab, CEditNumber32& edit, FieldPair ctrl)
{
    edit.Attach(tab.GetDlgItem(ctrl.EditId));
    edit.SetDisplayType(CEditNumber32::DisplayHex);
}

void CRegisterTabs::InitRegisterEdits(CWindow& tab, CEditNumber32* edits, const TabRecord* ctrlIds)
{
    for (int i = 0, n = ctrlIds->FieldCount; i < n; i++)
    {
        InitRegisterEdit(tab, edits[i], ctrlIds->Fields[i]);
    }
}

void CRegisterTabs::InitRegisterEdit64(CWindow& tab, CEditReg64& edit, FieldPair ctrl)
{
    edit.Attach(tab.GetDlgItem(ctrl.EditId));
}

void CRegisterTabs::InitRegisterEdits64(CWindow& tab, CEditReg64* edits, const TabRecord* ctrlIds)
{
    for (int i = 0, n = ctrlIds->FieldCount; i < n; i++)
    {
        InitRegisterEdit64(tab, edits[i], ctrlIds->Fields[i]);
    }
}

void CRegisterTabs::ZeroRegisterEdit(CEditNumber32& edit)
{
    edit.SetValue(0, DisplayMode::ZeroExtend);
}

void CRegisterTabs::ZeroRegisterEdits(CEditNumber32* edits, uint32_t ctrlIdsCount)
{
    for (int i = 0, n = ctrlIdsCount; i < n; i++)
    {
        ZeroRegisterEdit(edits[i]);
    }
}

void CRegisterTabs::ZeroRegisterEdit64(CEditReg64& edit)
{
    edit.SetValue(0);
}

void CRegisterTabs::ZeroRegisterEdits64(CEditReg64* edits, uint32_t ctrlIdsCount)
{
    for (uint32_t i = 0, n = ctrlIdsCount; i < n; i++)
    {
        ZeroRegisterEdit64(edits[i]);
    }
}

void CRegisterTabs::CopyTabRegisters() {
    int nPage = GetCurSel();
    stdstr str = CopyTabRegisters(nPage);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, str.length() + 1);
    strncpy((char*)GlobalLock(hMem), str.c_str(), str.length());
    GlobalUnlock(hMem);
    OpenClipboard();
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

void CRegisterTabs::CopyAllRegisters() {
    stdstr str;
    for (int i = 0; i <= 12; i++) {
        if (!str.empty()) str += "\r\n";
        str += CopyTabRegisters(i);
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, str.length() + 1);
    strncpy((char*)GlobalLock(hMem), str.c_str(), str.length());
    GlobalUnlock(hMem);
    OpenClipboard();
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

stdstr CRegisterTabs::CopyTabRegisters(int id)
{
    CWindow tab = m_TabWindows[id];
    stdstr str;

    TCITEM item = {0};
    TCHAR buffer[30];
    item.mask = TCIF_TEXT;
    item.cchTextMax = 30;
    item.pszText = buffer;
    GetItem(id, &item);

    str += stdstr().FromUTF16(item.pszText);

    const TabRecord *record = nullptr;
    switch (id)
    {
        case 0: record = &TabData::GPR; break;
        case 1: record = &TabData::FPR; break;
        case 2: record = &TabData::COP0; break;
        case 3:
            record = &TabData::RDRAM;
            str += " (A3F00000)";
            break;
        case 4:
            record = &TabData::SP;
            str += " (A4040000)";
            break;
        case 5:
            record = &TabData::DPC;
            str += " (A4100000)";
            break;
        case 6:
            record = &TabData::MI;
            str += " (A4300000)";
            break;
        case 7:
            record = &TabData::VI;
            str += " (A4400000)";
            break;
        case 8:
            record = &TabData::AI;
            str += " (A4500000)";
            break;
        case 9:
            record = &TabData::PI;
            str += " (A4600000)";
            break;
        case 10:
            record = &TabData::RI;
            str += " (A4700000)";
            break;
        case 11:
            record = &TabData::SI;
            str += " (A4800000)";
            break;
        case 12:
            record = &TabData::DD;
            str += " (A5000500)";
            break;
    }

    record->Iterate(&tab, [&str](const CWindow *label, const CWindow *edit)
    {
        str += stdstr_f(
            "\r\n%s %s",
            ::GetCWindowText(*label).c_str(),
            ::GetCWindowText(*edit).c_str());
    });

    switch (id)
    {
        case 0:
            str += stdstr_f("\r\nHI %s", m_HIEdit.GetValueText().c_str());
            str += stdstr_f("\r\nLO %s", m_LOEdit.GetValueText().c_str());
            break;
        case 1:
            str += stdstr_f("\r\nFCSR %s", m_FCSREdit.GetValueText().c_str());
            break;
        case 4:
            str += stdstr_f("\r\nSP (A4080000)\r\n00 SP_PC %s", m_SPPCEdit.GetValueText().c_str());
            break;
    }

    str += "\r\n";
    return str;
}

// CEditReg64 for GPR
uint64_t CEditReg64::ParseValue(const char* wordPair)
{
    uint32_t a, b;
    uint64_t ret;
    char * end = nullptr;
    a = strtoul(wordPair, &end, 16);
    if (*end == ' ')
    {
        end++;
        b = strtoul(end, nullptr, 16);
        ret = (uint64_t)a << 32;
        ret |= b;
        return ret;
    }
    return (uint64_t)a;
}

BOOL CEditReg64::Attach(HWND hWndNew)
{
    return SubclassWindow(hWndNew);
}

LRESULT CEditReg64::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!isStepping())
    {
        goto canceled;
    }

    char charCode = (char)wParam;

    if (!isxdigit(charCode) && charCode != ' ')
    {
        if (!isalnum(charCode))
        {
            goto unhandled;
        }
        goto canceled;
    }

    if (isalpha(charCode) && !isupper(charCode))
    {
        SendMessage(uMsg, toupper(wParam), lParam);
        goto canceled;
    }

    wchar_t text[20];
    GetWindowText(text, 20);
    int textLen = wcslen(text);

    if (textLen >= 17)
    {
        int selStart, selEnd;
        GetSel(selStart, selEnd);
        if (selEnd - selStart == 0)
        {
            goto canceled;
        }
    }

    if (charCode == ' ' && wcschr(text, ' ') != nullptr)
    {
        goto canceled;
    }

unhandled:
    bHandled = FALSE;
    return 0;

canceled:
    bHandled = TRUE;
    return 0;
}

uint64_t CEditReg64::GetValue()
{
    wchar_t text[20];
    GetWindowText(text, 20);
    return ParseValue(stdstr().FromUTF16(text).c_str());
}

stdstr CEditReg64::GetValueText()
{
    return ::GetCWindowText(*this);
}

LRESULT CEditReg64::OnLostFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    SetValue(GetValue()); // Clean up
    bHandled = FALSE;
    return 0;
}

void CEditReg64::SetValue(uint32_t h, uint32_t l)
{
    stdstr text;
    text.Format("%08X %08X", h, l);
    SetWindowText(text.ToUTF16().c_str());
}

void CEditReg64::SetValue(uint64_t value)
{
    uint32_t h = (value & 0xFFFFFFFF00000000LL) >> 32;
    uint32_t l = (value & 0x00000000FFFFFFFFLL);
    SetValue(h, l);
}
