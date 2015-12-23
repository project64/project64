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

#include "SettingsPage.h"
#include "SettingsPage-Game-Recompiler.h"

CGameRecompilePage::CGameRecompilePage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    //Set the text for all gui Items
    SetDlgItemTextW(m_hWnd, IDC_CPU_TYPE_TEXT, GS(ROM_CPU_STYLE));
    SetDlgItemTextW(m_hWnd, IDC_FUNCFIND_TEXT, GS(ROM_FUNC_FIND));

    SetDlgItemTextW(m_hWnd, IDC_ROM_REGCACHE, GS(ROM_REG_CACHE));
    SetDlgItemTextW(m_hWnd, IDC_BLOCK_LINKING, GS(ADVANCE_ABL));
    SetDlgItemTextW(m_hWnd, IDC_ROM_FASTSP, GS(ROM_FAST_SP));

    SetDlgItemTextW(m_hWnd, IDC_SMM_FRAME, GS(ADVANCE_SMCM));
    SetDlgItemTextW(m_hWnd, IDC_SMM_CACHE, GS(ADVANCE_SMM_CACHE));
    SetDlgItemTextW(m_hWnd, IDC_SMM_DMA, GS(ADVANCE_SMM_PIDMA));
    SetDlgItemTextW(m_hWnd, IDC_SMM_VALIDATE, GS(ADVANCE_SMM_VALIDATE));
    SetDlgItemTextW(m_hWnd, IDC_SMM_TLB, GS(ADVANCE_SMM_TLB));
    SetDlgItemTextW(m_hWnd, IDC_SMM_PROTECT, GS(ADVANCE_SMM_PROTECT));

    m_SelfModGroup.Attach(GetDlgItem(IDC_SMM_FRAME));

    AddModCheckBox(GetDlgItem(IDC_ROM_REGCACHE), Game_RegCache);
    AddModCheckBox(GetDlgItem(IDC_BLOCK_LINKING), Game_BlockLinking);
    AddModCheckBox(GetDlgItem(IDC_SMM_CACHE), Game_SMM_Cache);
    AddModCheckBox(GetDlgItem(IDC_SMM_DMA), Game_SMM_PIDMA);
    AddModCheckBox(GetDlgItem(IDC_SMM_VALIDATE), Game_SMM_ValidFunc);
    AddModCheckBox(GetDlgItem(IDC_SMM_TLB), Game_SMM_TLB);
    AddModCheckBox(GetDlgItem(IDC_SMM_PROTECT), Game_SMM_Protect);
    ::ShowWindow(GetDlgItem(IDC_SMM_STORE), SW_HIDE);
    //AddModCheckBox(GetDlgItem(IDC_SMM_STORE),Game_SMM_StoreInstruc);
    AddModCheckBox(GetDlgItem(IDC_ROM_FASTSP), Game_FastSP);

    CModifiedComboBox * ComboBox;
    ComboBox = AddModComboBox(GetDlgItem(IDC_CPU_TYPE), Game_CpuType);
    if (ComboBox)
    {
        ComboBox->AddItemW(GS(CORE_RECOMPILER), CPU_Recompiler);
        ComboBox->AddItemW(GS(CORE_INTERPTER), CPU_Interpreter);
        if (g_Settings->LoadBool(Debugger_Enabled))
        {
            ComboBox->AddItemW(GS(CORE_SYNC), CPU_SyncCores);
        }
    }

    ComboBox = AddModComboBox(GetDlgItem(IDC_FUNCFIND), Game_FuncLookupMode);
    if (ComboBox)
    {
        ComboBox->AddItemW(GS(FLM_PLOOKUP), FuncFind_PhysicalLookup);
        ComboBox->AddItemW(GS(FLM_VLOOKUP), FuncFind_VirtualLookup);
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

void CGameRecompilePage::ApplySettings(bool UpdateScreen)
{
    CSettingsPageImpl<CGameRecompilePage>::ApplySettings(UpdateScreen);
}

bool CGameRecompilePage::EnableReset(void)
{
    if (CSettingsPageImpl<CGameRecompilePage>::EnableReset()) { return true; }
    return false;
}

void CGameRecompilePage::ResetPage()
{
    CSettingsPageImpl<CGameRecompilePage>::ResetPage();
}