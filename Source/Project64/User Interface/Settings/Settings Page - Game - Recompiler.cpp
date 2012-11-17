#include "stdafx.h"
#include "Settings Page.h"
#include "Settings Page - Game - Recompiler.h"

CGameRecompilePage::CGameRecompilePage (HWND hParent, const RECT & rcDispay )
{
	if (!Create(hParent,rcDispay))
	{
		return;
	}

	m_SelfModGroup.Attach(GetDlgItem(IDC_SMM_FRAME));

	AddModCheckBox(GetDlgItem(IDC_ROM_REGCACHE),Game_RegCache);
	AddModCheckBox(GetDlgItem(IDC_BLOCK_LINKING),Game_BlockLinking);
	AddModCheckBox(GetDlgItem(IDC_SMM_CACHE),Game_SMM_Cache);
	AddModCheckBox(GetDlgItem(IDC_SMM_DMA),Game_SMM_PIDMA);
	AddModCheckBox(GetDlgItem(IDC_SMM_VALIDATE),Game_SMM_ValidFunc);
	AddModCheckBox(GetDlgItem(IDC_SMM_TLB),Game_SMM_TLB);
	AddModCheckBox(GetDlgItem(IDC_SMM_PROTECT),Game_SMM_Protect);
	::ShowWindow(GetDlgItem(IDC_SMM_STORE),SW_HIDE);
	//AddModCheckBox(GetDlgItem(IDC_SMM_STORE),Game_SMM_StoreInstruc);
	AddModCheckBox(GetDlgItem(IDC_ROM_FASTSP),Game_FastSP);

	CModifiedComboBox * ComboBox;
	ComboBox = AddModComboBox(GetDlgItem(IDC_CPU_TYPE),Game_CpuType);
	if (ComboBox)
	{
		ComboBox->AddItem(GS(CORE_RECOMPILER), CPU_Recompiler);
		ComboBox->AddItem(GS(CORE_INTERPTER), CPU_Interpreter);
		if (g_Settings->LoadBool(Debugger_Enabled))
		{
			ComboBox->AddItem(GS(CORE_SYNC), CPU_SyncCores);
		}
	}

	ComboBox = AddModComboBox(GetDlgItem(IDC_FUNCFIND),Game_FuncLookupMode);
	if (ComboBox)
	{
		ComboBox->AddItem(GS(FLM_PLOOKUP), FuncFind_PhysicalLookup);
		ComboBox->AddItem(GS(FLM_VLOOKUP), FuncFind_VirtualLookup);
		//ComboBox->AddItem(GS(FLM_CHANGEMEM), FuncFind_ChangeMemory);
	}
	UpdatePageSettings();
}

void CGameRecompilePage::ShowPage()
{
	ShowWindow(SW_SHOW);
}

void CGameRecompilePage::HidePage()
{
	ShowWindow(SW_HIDE);
}

void CGameRecompilePage::ApplySettings( bool UpdateScreen )
{
	CSettingsPageImpl<CGameRecompilePage>::ApplySettings(UpdateScreen);
}

bool CGameRecompilePage::EnableReset ( void )
{
	if (CSettingsPageImpl<CGameRecompilePage>::EnableReset()) { return true; }
	return false;
}

void CGameRecompilePage::ResetPage()
{
	CSettingsPageImpl<CGameRecompilePage>::ResetPage();
}
