class CFlashram 
{
	enum Modes {
		FLASHRAM_MODE_NOPES  = 0,
		FLASHRAM_MODE_ERASE  = 1,
		FLASHRAM_MODE_WRITE  = 2,
		FLASHRAM_MODE_READ   = 3,
		FLASHRAM_MODE_STATUS = 4,
	};

public:
	CFlashram ( bool ReadOnly );
   ~CFlashram ( void );

	void  DmaFromFlashram     ( BYTE * dest, int StartOffset, int len );
	void  DmaToFlashram       ( BYTE * Source, int StartOffset, int len );
	DWORD ReadFromFlashStatus ( DWORD PAddr );
	void  WriteToFlashCommand ( DWORD Value );

private:
	DWORD ReadFlashStatus ( DWORD PAddr );
	bool  LoadFlashram ( void );

	BYTE * m_FlashRamPointer;
	Modes  m_FlashFlag;
	QWORD  m_FlashStatus;
	DWORD  m_FlashRAM_Offset;
	bool   m_ReadOnly;
	HANDLE m_hFile;
};
