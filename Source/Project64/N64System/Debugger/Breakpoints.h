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

#pragma once

#include <stdint.h>

class CBreakpoints {
private:
	
public:
	typedef struct {
		uint32_t address;
		bool bTemporary;
	} BREAKPOINT;

	enum BPSTATE {
		BP_NOT_SET = FALSE,
		BP_SET,
		BP_SET_TEMP
	};

	CBreakpoints();

	BOOL m_Debugging;
	BOOL m_Skipping;

	std::vector<BREAKPOINT> m_RBP;
	std::vector<BREAKPOINT> m_WBP;
	std::vector<BREAKPOINT> m_EBP;

	int m_nRBP;
	int m_nWBP;
	int m_nEBP;

	void Pause();
	void Resume();
	void Skip();

	BOOL isDebugging();
	void KeepDebugging();
	void StopDebugging();
	inline BOOL isSkipping()
	{
		BOOL ret = m_Skipping;
		m_Skipping = FALSE;
		return ret;
	}

	bool RBPAdd(uint32_t address, bool bTemporary = false);
	void RBPRemove(uint32_t address);
	void RBPToggle(uint32_t address, bool bTemporary = false);
	void RBPClear();

	bool WBPAdd(uint32_t address, bool bTemporary = false);
	void WBPRemove(uint32_t address);
	void WBPToggle(uint32_t address, bool bTemporary = false);
	void WBPClear();

	bool EBPAdd(uint32_t address, bool bTemporary = false);
	void EBPRemove(uint32_t address);
	void EBPToggle(uint32_t address, bool bTemporary = false);
	void EBPClear();
	
	void BPClear();

	// inlines

	inline BPSTATE RBPExists(uint32_t address, bool bRemoveTemp = false)
	{
		for (int i = 0; i < m_nRBP; i++)
		{
			if (m_RBP[i].address != address)
			{
				continue;
			}

			if (m_RBP[i].bTemporary)
			{
				if (bRemoveTemp)
				{
					RBPRemove(address);
				}
				return BP_SET_TEMP;
			}
			return BP_SET;
		}
		return BP_NOT_SET;
	}

	inline BPSTATE WBPExists(uint32_t address, bool bRemoveTemp = false)
	{
		for (int i = 0; i < m_nWBP; i++)
		{
			if (m_WBP[i].address != address)
			{
				continue;
			}

			if (m_WBP[i].bTemporary)
			{
				if (bRemoveTemp)
				{
					WBPRemove(address);
				}
				return BP_SET_TEMP;
			}
			return BP_SET;
		}
		return BP_NOT_SET;
	}

	inline BPSTATE EBPExists(uint32_t address, bool bRemoveTemp = false)
	{
		for (int i = 0; i < m_nEBP; i++)
		{
			if (m_EBP[i].address != address)
			{
				continue;
			}

			if (m_EBP[i].bTemporary)
			{
				if (bRemoveTemp)
				{
					EBPRemove(address);
				}
				return BP_SET_TEMP;
			}
			return BP_SET;
		}
		return BP_NOT_SET;
	}

};