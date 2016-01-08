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
    SetDlgItemTextW(m_hWnd, IDC_CPU_TYPE_TEXT, wGS(ROM_CPU_STYLE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_FUNCFIND_TEXT, wGS(ROM_FUNC_FIND).c_str());

    SetDlgItemTextW(m_hWnd, IDC_ROM_REGCACHE, wGS(ROM_REG_CACHE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_BLOCK_LINKING, wGS(ADVANCE_ABL).c_str());
    SetDlgItemTextW(m_hWnd, IDC_ROM_FASTSP, wGS(ROM_FAST_SP).c_str());

    SetDlgItemTextW(m_hWnd, IDC_SMM_FRAME, wGS(ADVANCE_SMCM).c_str());
    SetDlgItemTextW(m_hWnd, IDC_SMM_CACHE, wGS(ADVANCE_SMM_CACHE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_SMM_DMA, wGS(ADVANCE_SMM_PIDMA).c_str());
    SetDlgItemTextW(m_hWnd, IDC_SMM_VALIDATE, wGS(ADVANCE_SMM_VALIDATE).c_str());
    SetDlgItemTextW(m_hWnd, IDC_SMM_TLB, wGS(ADVANCE_SMM_TLB).c_str());
    SetDlgItemTextW(m_hWnd, IDC_SMM_PROTECT, wGS(ADVANCE_SMM_PROTECT).c_str());

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
        ComboBox->AddItemW(wGS(CORE_RECOMPILER).c_str(), CPU_Recompiler);
        ComboBox->AddItemW(wGS(CORE_INTERPTER).c_str(), CPU_Interpreter);
        if (g_Settings->LoadBool(Debugger_Enabled))
        {
            ComboBox->AddItemW(wGS(CORE_SYNC).c_str(), CPU_SyncCores);
        }
    }

    ComboBox = AddModComboBox(GetDlgItem(IDC_FUNCFIND), Game_FuncLookupMode);
    if (ComboBox)
    {
        ComboBox->AddItemW(wGS(FLM_PLOOKUP).c_str(), FuncFind_PhysicalLookup);
        ComboBox->AddItemW(wGS(FLM_VLOOKUP).c_str(), FuncFind_VirtualLookup);
        //ComboBox->AddItem(wGS(FLM_CHANGEMEM).c_str(), FuncFind_ChangeMemory);
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