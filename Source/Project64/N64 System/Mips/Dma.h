class CDMA 
{
	CDMA();

public:
	void SP_DMA_READ  ( void ); 
	void SP_DMA_WRITE ( void );
	void PI_DMA_READ  ( void );
	void PI_DMA_WRITE ( void );

protected:
	CDMA (CFlashram & FlashRam, CSram & Sram);

	//void SI_DMA_READ  ( void );
	//void SI_DMA_WRITE ( void );

private:
	CFlashram & m_FlashRam;
	CSram     & m_Sram;
	
	void OnFirstDMA   ( void );
};


