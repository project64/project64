class CPifRamSettings
{
protected:
	CPifRamSettings();
	virtual ~CPifRamSettings();
	
	inline bool  bShowPifRamErrors    ( void ) const { return m_bShowPifRamErrors; }
	inline bool  bDelaySI             ( void ) const { return m_DelaySI; }
	inline DWORD RdramSize            ( void ) const { return m_RdramSize; }

private:
	static void RefreshSettings ( void * );
	
	static bool  m_bShowPifRamErrors;
	static bool  m_DelaySI;
	static DWORD m_RdramSize;

	static int  m_RefCount;

};

class CPifRam :
	private CPifRamSettings,
	private CEeprom
{
public:
	public:
	     CPifRam      ( bool SavesReadOnly );
		~CPifRam      ( void );

	void Reset        ( void );

	void PifRamWrite  ( void );
	void PifRamRead   ( void );

	void SI_DMA_READ  ( void );
	void SI_DMA_WRITE ( void );

protected:
	BYTE m_PifRom[0x7C0];
	BYTE m_PifRam[0x40];

private:
	#define CHALLENGE_LENGTH 0x20
	void ProcessControllerCommand ( int Control, BYTE * Command );
	void ReadControllerCommand    ( int Control, BYTE * Command );
	void LogControllerPakData     ( char * Description );
	void n64_cic_nus_6105		  (char challenge[], char response[], int length);
};