class CEeprom 
{
public:
	CEeprom ( bool ReadOnly );
   ~CEeprom ( void );

   void EepromCommand ( BYTE * Command );

private:
	void LoadEeprom ( void );
	void ReadFrom   ( BYTE * Buffer, int line );
	void WriteTo    ( BYTE * Buffer, int line );

	BYTE   m_EEPROM[0x800];
	bool   m_ReadOnly;
	HANDLE m_hFile;
};
