#include "stdafx.h"

CTLB::CTLB(CTLB_CB * CallBack ):	
	m_CB(CallBack)
{
	WriteTrace(TraceTLB,"CTLB::CTLB - Start");
	memset(m_tlb,0,sizeof(m_tlb));
	memset(m_FastTlb,0,sizeof(m_FastTlb));
	Reset(true);
	WriteTrace(TraceTLB,"CTLB::CTLB - Done");
}

CTLB::~CTLB (void) {
	WriteTrace(TraceTLB,"CTLB::~CTLB - Done");
	WriteTrace(TraceTLB,"CTLB::~CTLB - Done");
}

void CTLB::Reset (bool InvalidateTLB) {
	DWORD count;

	for (count = 0; count < 64; count++) 
	{ 
		m_FastTlb[count].ValidEntry = false; 
	}
	
	if (InvalidateTLB) 
	{
		for (count = 0; count < 32; count++) 
		{ 
			m_tlb[count].EntryDefined = false; 
		}
	} else {
		for (count = 0; count < 32; count ++) 
		{ 
			SetupTLB_Entry(count,false); 
		}
	}
}

bool CTLB::AddressDefined ( DWORD VAddr) {
	DWORD i;

	if (VAddr >= 0x80000000 && VAddr <= 0xBFFFFFFF) {
		return true;
	}

	for (i = 0; i < 64; i++) 
	{
		if (m_FastTlb[i].ValidEntry && 
			VAddr >= m_FastTlb[i].VSTART && 
			VAddr <= m_FastTlb[i].VEND) 
		{
			return true;
		}
	}	
	return false;
}

void CTLB::Probe (void) {
	int Counter;
	
	WriteTrace(TraceTLB,"TLB Probe");
	_Reg->INDEX_REGISTER |= 0x80000000;
	for (Counter = 0; Counter < 32; Counter ++) 
	{
		if (!m_tlb[Counter].EntryDefined)
		{ 
			continue; 
		}
		
		DWORD & TlbEntryHiValue = m_tlb[Counter].EntryHi.Value;
		DWORD Mask = ~m_tlb[Counter].PageMask.Mask << 13;
		DWORD TlbValueMasked = TlbEntryHiValue & Mask;
		DWORD EntryHiMasked = _Reg->ENTRYHI_REGISTER & Mask;

		if (TlbValueMasked == EntryHiMasked) {			
			if ((TlbEntryHiValue & 0x100) != 0 || //Global
				((TlbEntryHiValue & 0xFF) == (_Reg->ENTRYHI_REGISTER & 0xFF))) //SameAsid 
			{
				_Reg->INDEX_REGISTER = Counter;
				int FastIndx = Counter << 1;
				m_FastTlb[FastIndx].Probed = true;
				m_FastTlb[FastIndx + 1].Probed = true;
				return;
			}
		}
	}
}

void CTLB::ReadEntry (void) {
	DWORD index = _Reg->INDEX_REGISTER & 0x1F;

	_Reg->PAGE_MASK_REGISTER = m_tlb[index].PageMask.Value ;
	_Reg->ENTRYHI_REGISTER = (m_tlb[index].EntryHi.Value & ~m_tlb[index].PageMask.Value) ;
	_Reg->ENTRYLO0_REGISTER = m_tlb[index].EntryLo0.Value;
	_Reg->ENTRYLO1_REGISTER = m_tlb[index].EntryLo1.Value;		
}

void CTLB::WriteEntry (int index, bool Random) {
	int FastIndx;

	WriteTraceF(TraceTLB,"Write Entry %02d %d %08X %08X %08X %08X ",index,Random,_Reg->PAGE_MASK_REGISTER,_Reg->ENTRYHI_REGISTER,_Reg->ENTRYLO0_REGISTER,_Reg->ENTRYLO1_REGISTER);

	//Check to see if entry is unmapping it self
	if (m_tlb[index].EntryDefined) {
		FastIndx = index << 1;
		if (*_PROGRAM_COUNTER >= m_FastTlb[FastIndx].VSTART && 
			*_PROGRAM_COUNTER < m_FastTlb[FastIndx].VEND &&
			m_FastTlb[FastIndx].ValidEntry && m_FastTlb[FastIndx].VALID)
		{
			WriteTraceF(TraceTLB,"Write Entry: Ignored PC: %X VAddr Start: %X VEND: %X",*_PROGRAM_COUNTER,m_FastTlb[FastIndx].VSTART,m_FastTlb[FastIndx].VEND);
			return;
		}
		if (*_PROGRAM_COUNTER >= m_FastTlb[FastIndx + 1].VSTART && 
			*_PROGRAM_COUNTER < m_FastTlb[FastIndx + 1].VEND &&
			m_FastTlb[FastIndx + 1].ValidEntry && m_FastTlb[FastIndx + 1].VALID)
		{
			WriteTraceF(TraceTLB,"Write Entry: Ignored PC: %X VAddr Start: %X VEND: %X",*_PROGRAM_COUNTER,m_FastTlb[FastIndx + 1].VSTART,m_FastTlb[FastIndx + 1].VEND);
			return;
		}
	}

	//Reset old addresses
	if (m_tlb[index].EntryDefined) 
	{
		for ( FastIndx = index << 1; FastIndx <= (index << 1) + 1; FastIndx++) {
			if (!m_FastTlb[FastIndx].ValidEntry) { continue; }
			if (!m_FastTlb[FastIndx].VALID) { continue; }
			if (m_tlb[index].PageMask.Value == _Reg->PAGE_MASK_REGISTER &&
				m_tlb[index].EntryHi.Value == _Reg->ENTRYHI_REGISTER) 
			{
				if (FastIndx == (index << 1) && m_tlb[index].EntryLo0.Value == _Reg->ENTRYLO0_REGISTER) {
					continue;
				}
				if (FastIndx != (index << 1) && m_tlb[index].EntryLo1.Value == _Reg->ENTRYLO1_REGISTER) {
					continue;
				}
			}
			m_CB->TLB_Unmaped(m_FastTlb[FastIndx].VSTART,m_FastTlb[FastIndx].Length);
		}
	}
	
	//fill in m_tlb entry
	m_tlb[index].PageMask.Value = _Reg->PAGE_MASK_REGISTER;
	m_tlb[index].EntryHi.Value = _Reg->ENTRYHI_REGISTER;
	m_tlb[index].EntryLo0.Value = _Reg->ENTRYLO0_REGISTER;
	m_tlb[index].EntryLo1.Value = _Reg->ENTRYLO1_REGISTER;
	m_tlb[index].EntryDefined = true;
	SetupTLB_Entry(index,Random);
	m_CB->TLB_Changed();
}

void CTLB::SetupTLB_Entry (int index, bool Random) {
	//Fix up Fast TLB entries
	if (!m_tlb[index].EntryDefined) { return; }

	int FastIndx = index << 1;
	m_FastTlb[FastIndx].Length = (m_tlb[index].PageMask.Mask << 12) + 0xFFF;
	m_FastTlb[FastIndx].VSTART=m_tlb[index].EntryHi.VPN2 << 13;
	m_FastTlb[FastIndx].VEND = m_FastTlb[FastIndx].VSTART + m_FastTlb[FastIndx].Length;
	m_FastTlb[FastIndx].PHYSSTART = m_tlb[index].EntryLo0.PFN << 12;
	m_FastTlb[FastIndx].PHYSEND = m_FastTlb[FastIndx].PHYSSTART + m_FastTlb[FastIndx].Length;
	m_FastTlb[FastIndx].VALID = m_tlb[index].EntryLo0.V;
	m_FastTlb[FastIndx].DIRTY = m_tlb[index].EntryLo0.D; 
	m_FastTlb[FastIndx].GLOBAL = m_tlb[index].EntryLo0.GLOBAL & m_tlb[index].EntryLo1.GLOBAL;
	m_FastTlb[FastIndx].ValidEntry = FALSE;
	m_FastTlb[FastIndx].Random = Random;
	m_FastTlb[FastIndx].Probed = false;


	FastIndx = (index << 1) + 1;
	m_FastTlb[FastIndx].Length = (m_tlb[index].PageMask.Mask << 12) + 0xFFF;
	m_FastTlb[FastIndx].VSTART=(m_tlb[index].EntryHi.VPN2 << 13) + (m_FastTlb[FastIndx].Length + 1);
	m_FastTlb[FastIndx].VEND = m_FastTlb[FastIndx].VSTART + m_FastTlb[FastIndx].Length;
	m_FastTlb[FastIndx].PHYSSTART = m_tlb[index].EntryLo1.PFN << 12;
	m_FastTlb[FastIndx].PHYSEND = m_FastTlb[FastIndx].PHYSSTART + m_FastTlb[FastIndx].Length;
	m_FastTlb[FastIndx].VALID = m_tlb[index].EntryLo1.V;
	m_FastTlb[FastIndx].DIRTY = m_tlb[index].EntryLo1.D; 
	m_FastTlb[FastIndx].GLOBAL = m_tlb[index].EntryLo0.GLOBAL & m_tlb[index].EntryLo1.GLOBAL;
	m_FastTlb[FastIndx].ValidEntry = FALSE;
	m_FastTlb[FastIndx].Random = Random;
	m_FastTlb[FastIndx].Probed = false;

	//Test both entries to see if they are valid
 	for ( FastIndx = index << 1; FastIndx <= (index << 1) + 1; FastIndx++) {
		if (!m_FastTlb[FastIndx].VALID) { 
			m_FastTlb[FastIndx].ValidEntry = true;
			continue; 
		}
		
		if (m_FastTlb[FastIndx].VEND <= m_FastTlb[FastIndx].VSTART) {
			continue;
		}
		if (m_FastTlb[FastIndx].VSTART >= 0x80000000 && m_FastTlb[FastIndx].VEND <= 0xBFFFFFFF) {
			continue;
		}
		if (m_FastTlb[FastIndx].PHYSSTART > 0x1FFFFFFF) {
			continue;				
		}
			
		//MAP the new m_tlb entry for reading and writing
		m_FastTlb[FastIndx].ValidEntry = true;
		m_CB->TLB_Mapped(m_FastTlb[FastIndx].VSTART,m_FastTlb[FastIndx].Length,m_FastTlb[FastIndx].PHYSSTART, !m_FastTlb[FastIndx].DIRTY);
	}
}

bool CTLB::PAddrToVAddr(DWORD PAddr, DWORD & VAddr, DWORD & Index )
{
	for (int i = Index; i < 64; i++)
	{
		if (m_FastTlb[i].ValidEntry == false) { continue; }
		if (PAddr >= m_FastTlb[i].PHYSSTART && PAddr < m_FastTlb[i].PHYSEND) 
		{
			VAddr = m_FastTlb[i].VSTART + (PAddr - m_FastTlb[i].PHYSSTART);
			Index = i + 1;
			return true;
		}

	}
	return false;
}
