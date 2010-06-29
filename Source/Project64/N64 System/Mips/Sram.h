class CSram 
{
public:
	CSram ( bool ReadOnly );
   ~CSram ( void );

	void DmaFromSram ( BYTE * dest, int StartOffset, int len);
	void DmaToSram   ( BYTE * Source, int StartOffset, int len);

private:
	BOOL LoadSram    ( void );

	bool   m_ReadOnly;
	HANDLE m_hFile;
};