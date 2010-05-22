class RomInformation 
{
	bool const    m_DeleteRomInfo;
	stdstr  const m_FileName;
	CN64Rom *     m_pRomInfo;

	friend DWORD CALLBACK RomInfoProc ( WND_HANDLE, DWORD, DWORD, DWORD );

public:
	RomInformation(const char * RomFile );
	RomInformation(CN64Rom * RomInfo );
	~RomInformation(void);
	
	void DisplayInformation (WND_HANDLE hParent) const;
};