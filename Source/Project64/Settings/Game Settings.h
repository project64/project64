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
#pragma once

#include <N64 System/N64 Types.h>

class CGameSettings
{
public:
	void RefreshGameSettings ( void );

	inline static bool  bRomInMemory      ( void ) { return m_bRomInMemory; }
	inline static bool  bRegCaching       ( void ) { return m_RegCaching; }
	inline static bool  bLinkBlocks       ( void ) { return m_bLinkBlocks; }
	inline static FUNC_LOOKUP_METHOD LookUpMode ( void ) { return (FUNC_LOOKUP_METHOD)m_LookUpMode; }
	static inline bool  bUseTlb           ( void ) { return m_bUseTlb; }
	inline static DWORD CountPerOp        ( void ) { return m_CountPerOp; }
	inline static DWORD ViRefreshRate     ( void ) { return m_ViRefreshRate; }
	inline static DWORD AiCountPerBytes   ( void ) { return m_AiCountPerBytes; }
	inline static bool  bDelayDP          ( void ) { return m_DelayDP; }
	inline static bool  bDelaySI          ( void ) { return m_DelaySI; }
	inline static DWORD RdramSize         ( void ) { return m_RdramSize; }
	inline static bool  bFixedAudio       ( void ) { return m_bFixedAudio; }
	inline static bool  bSyncToAudio      ( void ) { return m_bSyncingToAudio; }
	inline static bool  bFastSP           ( void ) { return m_bFastSP; }
	inline static bool  b32BitCore        ( void ) { return m_b32Bit; }
	inline static bool  RspAudioSignal    ( void ) { return m_RspAudioSignal; }
	static inline bool  bSMM_StoreInstruc ( void ) { return m_bSMM_StoreInstruc;  }
	static inline bool  bSMM_Protect      ( void ) { return m_bSMM_Protect;       }
	static inline bool  bSMM_ValidFunc    ( void ) { return m_bSMM_ValidFunc;     }
	static inline bool  bSMM_PIDMA        ( void ) { return m_bSMM_PIDMA;         }
	static inline bool  bSMM_TLB          ( void ) { return m_bSMM_TLB;           }
	inline static SYSTEM_TYPE SystemType  ( void ) { return m_SystemType; }

protected:
	static void SpeedChanged (int SpeedLimit );

private:
	//Settings that can be changed on the fly
	static bool  m_bRomInMemory;
	static bool  m_RegCaching;
	static bool  m_bLinkBlocks;
	static DWORD m_LookUpMode; //FUNC_LOOKUP_METHOD
	static bool  m_bUseTlb;	
	static DWORD m_CountPerOp;	
	static DWORD m_ViRefreshRate;
	static DWORD m_AiCountPerBytes;
	static bool  m_DelayDP;
	static bool  m_DelaySI;
	static DWORD m_RdramSize;
	static bool  m_bFixedAudio;
	static bool  m_bSyncingToAudio;
	static bool  m_bSyncToAudio;
	static bool  m_bFastSP;
	static bool  m_b32Bit;
	static bool  m_RspAudioSignal;
	static bool  m_bSMM_StoreInstruc;
	static bool  m_bSMM_Protect;
	static bool  m_bSMM_ValidFunc;
	static bool  m_bSMM_PIDMA;
	static bool  m_bSMM_TLB;
	static SYSTEM_TYPE m_SystemType;
};
