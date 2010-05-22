class CC_Core;
class CDebugTlb;

class CTLB_CB 
{
public:	
	virtual void TLB_Mapped  ( DWORD VAddr, DWORD Len, DWORD PAddr, bool bReadOnly ) = 0;
	virtual void TLB_Unmaped ( DWORD VAddr, DWORD Len ) = 0;
	virtual void TLB_Changed ( void ) = 0;
};

class CTLB 
{
public:
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
	} TLB_ENTRY;

private:
	typedef struct {
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

	//friend CC_Core;
	friend CDebugTlb; // enable debug window to read class

	CTLB_CB * const m_CB;
	
	TLB_ENTRY m_tlb[32];
	FASTTLB   m_FastTlb[64];
	
	void SetupTLB_Entry     ( int index, bool Random );
	
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
	
	//Change the Virtual address to a Phyiscal Address
	/*inline bool TranslateVaddr ( DWORD VAddr, DWORD &PAddr) const 
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

	//see if the Vaddr is valid
	inline bool ValidVaddr  ( DWORD VAddr ) const { return TLB_ReadMap[VAddr >> 12] != 0; }
	*/
};
