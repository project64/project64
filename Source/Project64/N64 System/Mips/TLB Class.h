typedef struct {
	bool EntryDefined;
	union {
		unsigned long Value;
		unsigned char A[4];
		
		struct {
			unsigned zero : 13;
			unsigned Mask : 12;
			unsigned zero2 : 7;
		} ;
		
	} PageMask;
	
	union {
		unsigned long Value;
		unsigned char A[4];
		
		struct {
			unsigned ASID : 8;
			unsigned Zero : 4;
			unsigned G : 1;
			unsigned VPN2 : 19;
		};
		
	} EntryHi;

	union {
		unsigned long Value;
		unsigned char A[4];
		
		struct {
			unsigned GLOBAL: 1;
			unsigned V : 1;
			unsigned D : 1;
			unsigned C : 3;
			unsigned PFN : 20;
			unsigned ZERO: 6;
		} ;
		
	} EntryLo0;
	
	union {
		unsigned long Value;
		unsigned char A[4];
		
		struct {
			unsigned GLOBAL: 1;
			unsigned V : 1;
			unsigned D : 1;
			unsigned C : 3;
			unsigned PFN : 20;
			unsigned ZERO: 6;
		} ;
		
	} EntryLo1;
} TLB;

typedef struct {
   DWORD VSTART;
   DWORD VEND;
   DWORD PHYSSTART;
   DWORD PHYSEND;
   bool  VALID;
   bool  DIRTY;
   bool  GLOBAL;
   bool  ValidEntry;
   bool  Random;
   bool  Probed;
} FASTTLB; 

class CRecompilerOps;
class CC_Core;
class CDebugTlb;

class CMipsTLB_CallBack {
public:
	
	//Protected memory has been written to, returns true if that memory has been unprotected
	virtual void TLB_Changed   ( void ) = 0;
	virtual void TLB_Unmapping ( int TlbEntry, int FastTlbEntry, DWORD Vaddr, DWORD Len ) = 0;
};

class CTLB 
{
	friend CN64System; //Need to manipulate all variables in loading/saveing save state
	friend CRecompilerOps; // so can manipulate for ops
	friend CC_Core;
	friend CDebugTlb; // enable debug window to read class

	BYTE          * const &m_BasePAddr;  //Base Physical Address (eg RDRAM)
	CMipsTLB_CallBack * const CBClass;

	//Registers
	DWORD & PROGRAM_COUNTER;
	DWORD & INDEX_REGISTER;
	DWORD & PAGE_MASK_REGISTER;
	DWORD & ENTRYHI_REGISTER;
	DWORD & ENTRYLO0_REGISTER;
	DWORD & ENTRYLO1_REGISTER;
	
	TLB     tlb[32];
	FASTTLB FastTlb[64];
	
	//BIG look up table to quickly translate the tlb to real mem address
	DWORD * TLB_ReadMap;
	DWORD * TLB_WriteMap;

	void SetupTLB_Entry     ( int index, bool Random );
	
public:
	     CTLB ( CMipsTLB_CallBack * CallBack, BYTE * &BasePAddr, CRegisters *const &  Registers );
		~CTLB ( void );

	void TLB_Reset          ( bool InvalidateTLB );
	
	//Used by opcodes of the same name to manipulate the tlb (reads the registers)
	void TLB_Probe          ( void );
	void TLB_ReadEntry      ( void );
	void TLB_WriteEntry     ( int index, bool Random );

	//See if a VAddr has an entry to translate to a PAddr
	bool TLB_AddressDefined ( DWORD VAddr );
	

	//Change the Virtual address to a Phyiscal Address
	inline bool TranslateVaddr ( DWORD VAddr, DWORD &PAddr) const 
	{
		//Change the Virtual address to a Phyiscal Address
		if (TLB_ReadMap[VAddr >> 12] == 0) { return false; }
		PAddr = (DWORD)((BYTE *)(TLB_ReadMap[VAddr >> 12] + VAddr) - m_BasePAddr);
		return true;
	}
	
	inline DWORD TranslateVaddr ( DWORD VAddr ) const 
	{
		//Change the Virtual address to a Phyiscal Address
		return (DWORD)((BYTE *)(TLB_ReadMap[VAddr >> 12] + VAddr) - m_BasePAddr);
	}

	//Change the Virtual address to a pointer to the physical address in memory
	bool VAddrToRealAddr    ( DWORD VAddr, void * &RealAddress );
	
	// Find a matching Virtual Addres from a phyiscal one
	bool PAddrToVAddr       ( DWORD PAddr, DWORD & VAddr, DWORD & Index );

	//find out information about the tlb
	bool TLB_EntryUsed      ( int index );	//Test to see if TLB entry is valid
	int  TLB_UnusedEntry    ( int MinIndx );//Find a valid entry above MinIndx

	//see if the Vaddr is valid
	inline bool ValidVaddr  ( DWORD VAddr ) const { return TLB_ReadMap[VAddr >> 12] != 0; }
};
