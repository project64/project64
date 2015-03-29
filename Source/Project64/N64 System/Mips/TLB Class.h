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

class CDebugTlb;

class CTLB_CB 
{
public:	
	virtual void TLB_Mapped  ( DWORD VAddr, DWORD Len, DWORD PAddr, bool bReadOnly ) = 0;
	virtual void TLB_Unmaped ( DWORD VAddr, DWORD Len ) = 0;
	virtual void TLB_Changed ( void ) = 0;
};

class CTLB :
	protected CSystemRegisters
{
public:
	typedef struct
	{
		bool EntryDefined;
		union
		{
			unsigned long Value;
			unsigned char A[4];
			
			struct
			{
				unsigned zero : 13;
				unsigned Mask : 12;
				unsigned zero2 : 7;
			} ;
			
		} PageMask;
		
		union
		{
			unsigned long Value;
			unsigned char A[4];
			
			struct
			{
				unsigned ASID : 8;
				unsigned Zero : 4;
				unsigned G : 1;
				unsigned VPN2 : 19;
			};
			
		} EntryHi;

		union
		{
			unsigned long Value;
			unsigned char A[4];
			
			struct
			{
				unsigned GLOBAL: 1;
				unsigned V : 1;
				unsigned D : 1;
				unsigned C : 3;
				unsigned PFN : 20;
				unsigned ZERO: 6;
			} ;
			
		} EntryLo0;
		
		union
		{
			unsigned long Value;
			unsigned char A[4];
			
			struct
			{
				unsigned GLOBAL: 1;
				unsigned V : 1;
				unsigned D : 1;
				unsigned C : 3;
				unsigned PFN : 20;
				unsigned ZERO: 6;
			} ;
			
		} EntryLo1;
	} TLB_ENTRY;
	
public:
	     CTLB ( CTLB_CB * CallBack );
		~CTLB ( void );

	void Reset          ( bool InvalidateTLB );
	
	//Used by opcodes of the same name to manipulate the tlb (reads the registers)
	void Probe          ( void );
	void ReadEntry      ( void );
	void WriteEntry     ( int index, bool Random );

	//See if a VAddr has an entry to translate to a PAddr
	bool AddressDefined ( DWORD VAddr );
	
	const TLB_ENTRY & TlbEntry ( int Entry) const
	{
		return m_tlb[Entry];
	}

	bool PAddrToVAddr       ( DWORD PAddr, DWORD & VAddr, DWORD & Index );

	void RecordDifference ( CLog &LogFile, const CTLB& rTLB);

	bool operator == (const CTLB& rTLB) const;
	bool operator != (const CTLB& rTLB) const;

private:
	typedef struct
	{
		DWORD VSTART;
		DWORD VEND;
		DWORD PHYSSTART;
		DWORD PHYSEND;
		DWORD Length;
		bool  VALID;
		bool  DIRTY;
		bool  GLOBAL;
		bool  ValidEntry;
		bool  Random;
		bool  Probed;
	} FASTTLB; 

	friend CDebugTlb; // enable debug window to read class

	CTLB_CB * const m_CB;

	TLB_ENTRY m_tlb[32];
	FASTTLB   m_FastTlb[64];

	void SetupTLB_Entry     ( int index, bool Random );
};
