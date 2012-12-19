/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

bool  CGameSettings::m_bSMM_StoreInstruc;  
bool  CGameSettings::m_bSMM_Protect;  
bool  CGameSettings::m_bSMM_ValidFunc;
bool  CGameSettings::m_bSMM_PIDMA;  
bool  CGameSettings::m_bSMM_TLB;    
bool  CGameSettings::m_bUseTlb;
DWORD CGameSettings::m_CountPerOp = 2;
DWORD CGameSettings::m_ViRefreshRate = 1500;
DWORD CGameSettings::m_AiCountPerBytes = 500;
bool  CGameSettings::m_DelayDP = false;
bool  CGameSettings::m_DelaySI = false;
DWORD CGameSettings::m_RdramSize = 0;
bool  CGameSettings::m_bFixedAudio;  
bool  CGameSettings::m_bSyncToAudio; 
bool  CGameSettings::m_bFastSP;
bool  CGameSettings::m_b32Bit;
bool  CGameSettings::m_RspAudioSignal;
bool  CGameSettings::m_bRomInMemory;
bool  CGameSettings::m_RegCaching;
bool  CGameSettings::m_bLinkBlocks;
DWORD CGameSettings::m_LookUpMode; //FUNC_LOOKUP_METHOD

void CGameSettings::RefreshGameSettings()
{
	m_bSMM_StoreInstruc = false /*g_Settings->LoadBool(Game_SMM_StoreInstruc)*/;
	m_bSMM_Protect      = g_Settings->LoadBool(Game_SMM_Protect);
	m_bSMM_ValidFunc    = g_Settings->LoadBool(Game_SMM_ValidFunc);
	m_bSMM_PIDMA        = g_Settings->LoadBool(Game_SMM_PIDMA);
	m_bSMM_TLB          = g_Settings->LoadBool(Game_SMM_TLB);
	m_bUseTlb           = g_Settings->LoadBool(Game_UseTlb);
	m_ViRefreshRate     = g_Settings->LoadDword(Game_ViRefreshRate);
	m_AiCountPerBytes   = g_Settings->LoadDword(Game_AiCountPerBytes);
	m_CountPerOp        = g_Settings->LoadDword(Game_CounterFactor);
	m_RdramSize         = g_Settings->LoadDword(Game_RDRamSize);
	m_DelaySI           = g_Settings->LoadBool(Game_DelaySI);
	m_DelayDP           = g_Settings->LoadBool(Game_DelayDP);
	m_bFixedAudio       = g_Settings->LoadBool(Game_FixedAudio);
	m_bSyncToAudio      = m_bFixedAudio ? g_Settings->LoadBool(Game_SyncViaAudio) : false;
	m_b32Bit            = g_Settings->LoadBool(Game_32Bit);
	m_bFastSP           = g_Settings->LoadBool(Game_FastSP);
	m_RspAudioSignal    = g_Settings->LoadBool(Game_RspAudioSignal);
	m_bRomInMemory      = g_Settings->LoadBool(Game_LoadRomToMemory);
	m_bFastSP           = g_Settings->LoadBool(Game_FastSP);
	m_b32Bit            = g_Settings->LoadBool(Game_32Bit);

	m_RegCaching        = g_Settings->LoadBool(Game_RegCache);
	m_bLinkBlocks       = g_Settings->LoadBool(Game_BlockLinking);
	m_LookUpMode        = g_Settings->LoadDword(Game_FuncLookupMode);
}