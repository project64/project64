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

#include "Debugger-RegisterTabs.h"
#include "OpInfo.h"

bool CRegisterTabs::m_bColorsEnabled = false;

void CRegisterTabs::Attach(HWND hWndNew)
{
	m_TabWindows.clear();

	CTabCtrl::Attach(hWndNew);

	HFONT monoFont = CreateFont(-11, 0, 0, 0,
		FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, FF_DONTCARE, "Consolas"
	);

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
	
	InitRegisterEdits64(m_GPRTab, m_GPREdits, GPREditIds, monoFont);
	InitRegisterEdit64(m_GPRTab, m_HIEdit, IDC_HI_EDIT, monoFont);
	InitRegisterEdit64(m_GPRTab, m_LOEdit, IDC_LO_EDIT, monoFont);

	InitRegisterEdits(m_FPRTab, m_FPREdits, FPREditIds, monoFont);
	InitRegisterEdit(m_FPRTab, m_FCSREdit, IDC_FCSR_EDIT, monoFont);

	InitRegisterEdits(m_COP0Tab, m_COP0Edits, COP0EditIds, monoFont);
	m_CauseTip.Attach(m_COP0Tab.GetDlgItem(IDC_CAUSE_TIP));

	InitRegisterEdits(m_RDRAMTab, m_RDRAMEdits, RDRAMEditIds, monoFont);

	InitRegisterEdits(m_SPTab, m_SPEdits, SPEditIds, monoFont);
	InitRegisterEdit(m_SPTab, m_SPPCEdit, IDC_SP_PC_EDIT, monoFont);

	InitRegisterEdits(m_DPCTab, m_DPCEdits, DPCEditIds, monoFont);

	InitRegisterEdits(m_MITab, m_MIEdits, MIEditIds, monoFont);

	InitRegisterEdits(m_VITab, m_VIEdits, VIEditIds, monoFont);

	InitRegisterEdits(m_AITab, m_AIEdits, AIEditIds, monoFont);

	InitRegisterEdits(m_PITab, m_PIEdits, PIEditIds, monoFont);
	
	InitRegisterEdits(m_RITab, m_RIEdits, RIEditIds, monoFont);
	
	InitRegisterEdits(m_SITab, m_SIEdits, SIEditIds, monoFont);

	InitRegisterEdits(m_DDTab, m_DDEdits, DDEditIds, monoFont);
	
	SetColorsEnabled(false);
	RefreshEdits();
	RedrawCurrentTab();
}

void CRegisterTabs::RefreshEdits()
{
	if (g_Reg == NULL)
	{
		ZeroRegisterEdits64(m_GPREdits, GPREditIds);
		ZeroRegisterEdit64(m_HIEdit);
		ZeroRegisterEdit64(m_LOEdit);

		ZeroRegisterEdits(m_FPREdits, FPREditIds);
		ZeroRegisterEdit(m_FCSREdit);
		
		ZeroRegisterEdits(m_COP0Edits, COP0EditIds);

		ZeroRegisterEdits(m_RDRAMEdits, RDRAMEditIds);

		ZeroRegisterEdits(m_SPEdits, SPEditIds);
		ZeroRegisterEdit(m_SPPCEdit);

		ZeroRegisterEdits(m_DPCEdits, DPCEditIds);

		ZeroRegisterEdits(m_MIEdits, MIEditIds);

		ZeroRegisterEdits(m_VIEdits, VIEditIds);

		ZeroRegisterEdits(m_AIEdits, AIEditIds);

		ZeroRegisterEdits(m_PIEdits, PIEditIds);

		ZeroRegisterEdits(m_RIEdits, RIEditIds);

		ZeroRegisterEdits(m_SIEdits, SIEditIds);

		ZeroRegisterEdits(m_DDEdits, DDEditIds);

		return;
	}
	
	for (int i = 0; i < 32; i++)
	{
		m_GPREdits[i].SetValue(g_Reg->m_GPR[i].UDW);
		m_FPREdits[i].SetValue(*(uint32_t *)g_Reg->m_FPR_S[i], false, true);
	}
	m_HIEdit.SetValue(g_Reg->m_HI.UDW);
	m_LOEdit.SetValue(g_Reg->m_LO.UDW);

	m_FCSREdit.SetValue(g_Reg->m_FPCR[31], false, true);

	m_COP0Edits[0].SetValue(g_Reg->INDEX_REGISTER, false, true);
	m_COP0Edits[1].SetValue(g_Reg->RANDOM_REGISTER, false, true);
	m_COP0Edits[2].SetValue(g_Reg->ENTRYLO0_REGISTER, false, true);
	m_COP0Edits[3].SetValue(g_Reg->ENTRYLO1_REGISTER, false, true);
	m_COP0Edits[4].SetValue(g_Reg->CONTEXT_REGISTER, false, true);
	m_COP0Edits[5].SetValue(g_Reg->PAGE_MASK_REGISTER, false, true);
	m_COP0Edits[6].SetValue(g_Reg->WIRED_REGISTER, false, true);
	m_COP0Edits[7].SetValue(g_Reg->BAD_VADDR_REGISTER, false, true);
	m_COP0Edits[8].SetValue(g_Reg->COUNT_REGISTER, false, true);
	m_COP0Edits[9].SetValue(g_Reg->ENTRYHI_REGISTER, false, true);
	m_COP0Edits[10].SetValue(g_Reg->COMPARE_REGISTER, false, true);
	m_COP0Edits[11].SetValue(g_Reg->STATUS_REGISTER, false, true);
	m_COP0Edits[12].SetValue(g_Reg->CAUSE_REGISTER, false, true);
	m_COP0Edits[13].SetValue(g_Reg->EPC_REGISTER, false, true);
	m_COP0Edits[14].SetValue(g_Reg->CONFIG_REGISTER, false, true);
	m_COP0Edits[15].SetValue(g_Reg->TAGLO_REGISTER, false, true);
	m_COP0Edits[16].SetValue(g_Reg->TAGHI_REGISTER, false, true);
	m_COP0Edits[17].SetValue(g_Reg->ERROREPC_REGISTER, false, true);
	m_COP0Edits[18].SetValue(g_Reg->FAKE_CAUSE_REGISTER, false, true);

	CAUSE cause;
	cause.intval = g_Reg->CAUSE_REGISTER;

	const char* szExceptionCode = ExceptionCodes[cause.exceptionCode];
	m_CauseTip.SetWindowTextA(szExceptionCode);

	m_RDRAMEdits[0].SetValue(g_Reg->RDRAM_CONFIG_REG, false, true); // or device type
	m_RDRAMEdits[1].SetValue(g_Reg->RDRAM_DEVICE_ID_REG, false, true);
	m_RDRAMEdits[2].SetValue(g_Reg->RDRAM_DELAY_REG, false, true);
	m_RDRAMEdits[3].SetValue(g_Reg->RDRAM_MODE_REG, false, true);
	m_RDRAMEdits[4].SetValue(g_Reg->RDRAM_REF_INTERVAL_REG, false, true);
	m_RDRAMEdits[5].SetValue(g_Reg->RDRAM_REF_ROW_REG, false, true);
	m_RDRAMEdits[6].SetValue(g_Reg->RDRAM_RAS_INTERVAL_REG, false, true);
	m_RDRAMEdits[7].SetValue(g_Reg->RDRAM_MIN_INTERVAL_REG, false, true);
	m_RDRAMEdits[8].SetValue(g_Reg->RDRAM_ADDR_SELECT_REG, false, true);
	m_RDRAMEdits[9].SetValue(g_Reg->RDRAM_DEVICE_MANUF_REG, false, true);

	m_SPEdits[0].SetValue(g_Reg->SP_MEM_ADDR_REG, false, true);
	m_SPEdits[1].SetValue(g_Reg->SP_DRAM_ADDR_REG, false, true);
	m_SPEdits[2].SetValue(g_Reg->SP_RD_LEN_REG, false, true);
	m_SPEdits[3].SetValue(g_Reg->SP_WR_LEN_REG, false, true);
	m_SPEdits[4].SetValue(g_Reg->SP_STATUS_REG, false, true);
	m_SPEdits[5].SetValue(g_Reg->SP_DMA_FULL_REG, false, true);
	m_SPEdits[6].SetValue(g_Reg->SP_DMA_BUSY_REG, false, true);
	m_SPEdits[7].SetValue(g_Reg->SP_SEMAPHORE_REG, false, true);
	m_SPPCEdit.SetValue(g_Reg->SP_PC_REG, false, true);

	m_DPCEdits[0].SetValue(g_Reg->DPC_START_REG, false, true);
	m_DPCEdits[1].SetValue(g_Reg->DPC_END_REG, false, true);
	m_DPCEdits[2].SetValue(g_Reg->DPC_CURRENT_REG, false, true);
	m_DPCEdits[3].SetValue(g_Reg->DPC_STATUS_REG, false, true);
	m_DPCEdits[4].SetValue(g_Reg->DPC_CLOCK_REG, false, true);
	m_DPCEdits[5].SetValue(g_Reg->DPC_BUFBUSY_REG, false, true);
	m_DPCEdits[6].SetValue(g_Reg->DPC_PIPEBUSY_REG, false, true);
	m_DPCEdits[7].SetValue(g_Reg->DPC_TMEM_REG, false, true);

	m_MIEdits[0].SetValue(g_Reg->MI_INIT_MODE_REG, false, true);
	m_MIEdits[1].SetValue(g_Reg->MI_VERSION_REG, false, true);
	m_MIEdits[2].SetValue(g_Reg->MI_INTR_REG, false, true);
	m_MIEdits[3].SetValue(g_Reg->MI_INTR_MASK_REG, false, true);

	m_VIEdits[0].SetValue(g_Reg->VI_STATUS_REG, false, true);
	m_VIEdits[1].SetValue(g_Reg->VI_ORIGIN_REG, false, true);
	m_VIEdits[2].SetValue(g_Reg->VI_WIDTH_REG, false, true);
	m_VIEdits[3].SetValue(g_Reg->VI_INTR_REG, false, true);
	m_VIEdits[4].SetValue(g_Reg->VI_CURRENT_REG, false, true);
	m_VIEdits[5].SetValue(g_Reg->VI_BURST_REG, false, true);
	m_VIEdits[6].SetValue(g_Reg->VI_V_SYNC_REG, false, true);
	m_VIEdits[7].SetValue(g_Reg->VI_H_SYNC_REG, false, true);
	m_VIEdits[8].SetValue(g_Reg->VI_LEAP_REG, false, true);
	m_VIEdits[9].SetValue(g_Reg->VI_H_START_REG, false, true);
	m_VIEdits[10].SetValue(g_Reg->VI_V_START_REG, false, true);
	m_VIEdits[11].SetValue(g_Reg->VI_V_BURST_REG, false, true);
	m_VIEdits[12].SetValue(g_Reg->VI_X_SCALE_REG, false, true);
	m_VIEdits[13].SetValue(g_Reg->VI_Y_SCALE_REG, false, true);

	m_AIEdits[0].SetValue(g_Reg->AI_DRAM_ADDR_REG, false, true);
	m_AIEdits[1].SetValue(g_Reg->AI_LEN_REG, false, true);
	m_AIEdits[2].SetValue(g_Reg->AI_CONTROL_REG, false, true);
	m_AIEdits[3].SetValue(g_Reg->AI_STATUS_REG, false, true);
	m_AIEdits[4].SetValue(g_Reg->AI_DACRATE_REG, false, true);
	m_AIEdits[5].SetValue(g_Reg->AI_BITRATE_REG, false, true);

	m_PIEdits[0].SetValue(g_Reg->PI_DRAM_ADDR_REG, false, true);
	m_PIEdits[1].SetValue(g_Reg->PI_CART_ADDR_REG, false, true);
	m_PIEdits[2].SetValue(g_Reg->PI_RD_LEN_REG, false, true);
	m_PIEdits[3].SetValue(g_Reg->PI_WR_LEN_REG, false, true);
	m_PIEdits[4].SetValue(g_Reg->PI_STATUS_REG, false, true);
	m_PIEdits[5].SetValue(g_Reg->PI_BSD_DOM1_LAT_REG, false, true);
	m_PIEdits[6].SetValue(g_Reg->PI_BSD_DOM1_PWD_REG, false, true);
	m_PIEdits[7].SetValue(g_Reg->PI_BSD_DOM1_PGS_REG, false, true);
	m_PIEdits[8].SetValue(g_Reg->PI_BSD_DOM1_RLS_REG, false, true);
	m_PIEdits[9].SetValue(g_Reg->PI_BSD_DOM2_LAT_REG, false, true);
	m_PIEdits[10].SetValue(g_Reg->PI_BSD_DOM2_PWD_REG, false, true);
	m_PIEdits[11].SetValue(g_Reg->PI_BSD_DOM2_PGS_REG, false, true);
	m_PIEdits[12].SetValue(g_Reg->PI_BSD_DOM2_RLS_REG, false, true);
	
	m_RIEdits[0].SetValue(g_Reg->RI_MODE_REG, false, true);
	m_RIEdits[1].SetValue(g_Reg->RI_CONFIG_REG, false, true);
	m_RIEdits[2].SetValue(g_Reg->RI_CURRENT_LOAD_REG, false, true);
	m_RIEdits[3].SetValue(g_Reg->RI_SELECT_REG, false, true);
	m_RIEdits[4].SetValue(g_Reg->RI_REFRESH_REG, false, true); // or ri count
	m_RIEdits[5].SetValue(g_Reg->RI_LATENCY_REG, false, true);
	m_RIEdits[6].SetValue(g_Reg->RI_RERROR_REG, false, true);
	m_RIEdits[7].SetValue(g_Reg->RI_WERROR_REG, false, true);
	
	m_SIEdits[0].SetValue(g_Reg->SI_DRAM_ADDR_REG, false, true);
	m_SIEdits[1].SetValue(g_Reg->SI_PIF_ADDR_RD64B_REG, false, true);
	m_SIEdits[2].SetValue(g_Reg->SI_PIF_ADDR_WR64B_REG, false, true);
	m_SIEdits[3].SetValue(g_Reg->SI_STATUS_REG, false, true);

	m_DDEdits[0].SetValue(g_Reg->ASIC_DATA, false, true);
	m_DDEdits[1].SetValue(g_Reg->ASIC_MISC_REG, false, true);
	m_DDEdits[2].SetValue(g_Reg->ASIC_STATUS, false, true);
	m_DDEdits[3].SetValue(g_Reg->ASIC_CUR_TK, false, true);
	m_DDEdits[4].SetValue(g_Reg->ASIC_BM_STATUS, false, true);
	m_DDEdits[5].SetValue(g_Reg->ASIC_ERR_SECTOR, false, true);
	m_DDEdits[6].SetValue(g_Reg->ASIC_SEQ_STATUS, false, true);
	m_DDEdits[7].SetValue(g_Reg->ASIC_CUR_SECTOR, false, true);
	m_DDEdits[8].SetValue(g_Reg->ASIC_HARD_RESET, false, true);
	m_DDEdits[9].SetValue(g_Reg->ASIC_C1_S0, false, true);
	m_DDEdits[10].SetValue(g_Reg->ASIC_HOST_SECBYTE, false, true);
	m_DDEdits[11].SetValue(g_Reg->ASIC_C1_S2, false, true);
	m_DDEdits[12].SetValue(g_Reg->ASIC_SEC_BYTE, false, true);
	m_DDEdits[13].SetValue(g_Reg->ASIC_C1_S4, false, true);
	m_DDEdits[14].SetValue(g_Reg->ASIC_C1_S6, false, true);
	m_DDEdits[15].SetValue(g_Reg->ASIC_CUR_ADDR, false, true);
	m_DDEdits[16].SetValue(g_Reg->ASIC_ID_REG, false, true);
	m_DDEdits[17].SetValue(g_Reg->ASIC_TEST_REG, false, true);
	m_DDEdits[18].SetValue(g_Reg->ASIC_TEST_PIN_SEL, false, true);
}

void CRegisterTabs::RegisterChanged(HWND hDlg, TAB_ID srcTabId, WPARAM wParam)
{
	CBreakpoints* breakpoints = ((CDebuggerUI*)g_Debugger)->Breakpoints();

	if (g_Reg == NULL || !breakpoints->isDebugging())
	{
		return;
	}

	int ctrlId = LOWORD(wParam);

	CWindow editCtrl = ::GetDlgItem(hDlg, ctrlId);
	char text[20];
	editCtrl.GetWindowText(text, 20);

	if (srcTabId == TabGPR)
	{
		uint64_t value = CEditReg64::ParseValue(text);
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
			int nReg = MapEditRegNum(ctrlId, GPREditIds);
			g_Reg->m_GPR[nReg].UDW = value;
		}
		return;
	}

	uint32_t value = strtoul(text, NULL, 16);
	sprintf(text, "%08X", value);
	editCtrl.SetWindowText(text); // reformat text

	if (srcTabId == TabFPR)
	{
		if (ctrlId == IDC_FCSR_EDIT)
		{
			g_Reg->m_FPCR[31] = value;
			return;
		}

		int nReg = MapEditRegNum(ctrlId, FPREditIds);
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
		return TRUE;
	}
	if (msg != WM_COMMAND)
	{
		return FALSE;
	}

	WORD notification = HIWORD(wParam);

	if (notification == EN_KILLFOCUS)
	{
		RegisterChanged(hDlg, TabDefault, wParam);
	}

	return FALSE;
}

INT_PTR CALLBACK CRegisterTabs::TabProcGPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		return TRUE;
	}
	
	if (msg == WM_CTLCOLOREDIT)
	{
		HDC hdc = (HDC)wParam;
		COLORREF colorBg = RGB(255, 255, 255);

		COLORREF colorRead = RGB(200, 200, 255);
		COLORREF colorWrite = RGB(255, 200, 200);
		COLORREF colorBoth = RGB(255, 200, 255);

		if (!m_bColorsEnabled || g_Reg == NULL || g_MMU == NULL)
		{
			return FALSE;
		}

		HWND hWnd = (HWND)lParam;
		int ctrlId = ::GetWindowLong(hWnd, GWL_ID);
		
		COpInfo opInfo;
		g_MMU->LW_VAddr(g_Reg->m_PROGRAM_COUNTER, opInfo.m_OpCode.Hex);

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
			int nReg = MapEditRegNum(ctrlId, GPREditIds);
			bOpReads = opInfo.ReadsGPR(nReg);
			bOpWrites = opInfo.WritesGPR(nReg);
		}
		
		if (bOpReads && bOpWrites)
		{
			colorBg = colorBoth;
		}
		else if(bOpReads)
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

	if (msg != WM_COMMAND)
	{
		return FALSE;
	}

	WORD notification = HIWORD(wParam);

	if (notification == EN_KILLFOCUS)
	{
		RegisterChanged(hDlg, TabGPR, wParam);
	}

	return FALSE;
}

INT_PTR CALLBACK CRegisterTabs::TabProcFPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		return TRUE;
	}
	if (msg != WM_COMMAND)
	{
		return FALSE;
	}

	WORD notification = HIWORD(wParam);

	if (notification == EN_KILLFOCUS)
	{
		RegisterChanged(hDlg, TabFPR, wParam);
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
	AddItem(caption);

	CWindow parentWin = GetParent();
	CWindow tabWin = ::CreateDialog(NULL, MAKEINTRESOURCE(dialogId), parentWin, dlgProc);

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
	for (int i = 0; i < m_TabWindows.size(); i++)
	{
		::ShowWindow(m_TabWindows[i], SW_HIDE);
	}

	CRect pageRect = GetPageRect();

	::SetWindowPos(
		m_TabWindows[nPage],
		HWND_TOP,
		pageRect.left,
		pageRect.top,
		pageRect.Width(),
		pageRect.Height(),
		SWP_SHOWWINDOW
	);
}

void CRegisterTabs::RedrawCurrentTab()
{
	int nPage = GetCurSel();
	CRect pageRect = GetPageRect();

	::SetWindowPos(
		m_TabWindows[nPage],
		HWND_TOP,
		pageRect.left,
		pageRect.top,
		pageRect.Width(),
		pageRect.Height(),
		SWP_SHOWWINDOW
	);
}

void CRegisterTabs::SetColorsEnabled(bool bColorsEnabled)
{
	m_bColorsEnabled = bColorsEnabled;
}

void CRegisterTabs::InitRegisterEdit(CWindow& tab, CEditNumber& edit, WORD ctrlId, HFONT font)
{
	edit.Attach(tab.GetDlgItem(ctrlId));
	edit.SetDisplayType(CEditNumber::DisplayHex);
	edit.SetFont(font);
}

void CRegisterTabs::InitRegisterEdits(CWindow& tab, CEditNumber* edits, const DWORD* ctrlIds, HFONT font)
{
	for (int i = 0; i < ctrlIds[i] != 0; i++)
	{
		InitRegisterEdit(tab, edits[i], ctrlIds[i], font);
	}
}

void CRegisterTabs::InitRegisterEdit64(CWindow& tab, CEditReg64& edit, WORD ctrlId, HFONT font)
{
	edit.Attach(tab.GetDlgItem(ctrlId));
	edit.SetFont(font);
}

void CRegisterTabs::InitRegisterEdits64(CWindow& tab, CEditReg64* edits, const DWORD* ctrlIds, HFONT font)
{
	for (int i = 0; i < ctrlIds[i] != 0; i++)
	{
		InitRegisterEdit64(tab, edits[i], ctrlIds[i], font);
	}
}

void CRegisterTabs::ZeroRegisterEdit(CEditNumber& edit)
{
	edit.SetValue(0, false, true);
}

void CRegisterTabs::ZeroRegisterEdits(CEditNumber* edits, const DWORD* ctrlIds)
{
	for (int i = 0; ctrlIds[i] != 0; i++)
	{
		ZeroRegisterEdit(edits[i]);
	}
}

void CRegisterTabs::ZeroRegisterEdit64(CEditReg64& edit)
{
	edit.SetValue(0);
}

void CRegisterTabs::ZeroRegisterEdits64(CEditReg64* edits, const DWORD* ctrlIds)
{
	for (int i = 0; ctrlIds[i] != 0; i++)
	{
		ZeroRegisterEdit64(edits[i]);
	}
}

// CEditReg64 for GPR

uint64_t CEditReg64::ParseValue(char* wordPair)
{
	uint32_t a, b;
	uint64_t ret;
	a = strtoul(wordPair, &wordPair, 16);
	if (*wordPair == ' ')
	{
		wordPair++;
		b = strtoul(wordPair, NULL, 16);
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
	CBreakpoints* breakpoints = ((CDebuggerUI*)g_Debugger)->Breakpoints();
	if (!breakpoints->isDebugging())
	{
		goto canceled;
	}

	char charCode = wParam;

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

	char text[20];
	GetWindowText(text, 20);
	int textLen = strlen(text);

	if (textLen >= 17)
	{
		int selStart, selEnd;
		GetSel(selStart, selEnd);
		if (selEnd - selStart == 0)
		{
			goto canceled;
		}
	}

	if (charCode == ' ' && strchr(text, ' ') != NULL)
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
	char text[20];
	GetWindowText(text, 20);
	return ParseValue(text);
}

LRESULT CEditReg64::OnLostFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SetValue(GetValue()); // clean up
	bHandled = FALSE;
	return 0;
}

void CEditReg64::SetValue(uint32_t h, uint32_t l)
{
	char text[20];
	sprintf(text, "%08X %08X", h, l);
	SetWindowText(text);
}

void CEditReg64::SetValue(uint64_t value)
{
	uint32_t h = (value & 0xFFFFFFFF00000000LL) >> 32;
	uint32_t l = (value & 0x00000000FFFFFFFFLL);
	SetValue(h, l);
}