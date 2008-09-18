typedef std::map<DWORD, stdstr> StringMap;

class CMemoryLabel {
	// Variable dealing with Labels
	StringMap m_LabelList;
	int       m_NewLabels;
	stdstr    CurrentLabelFile;

	DWORD  AsciiToHex     ( char * HexValue );
	void   ProcessCODFile ( BYTE * File, DWORD FileLen );

public:
	//Functions related to Labels
	void   AddMemoryLabel  ( DWORD Address, const char * Message, ...  );
	stdstr LabelName       ( DWORD Address ) const;
	stdstr StoredLabelName ( DWORD Address );
	void   LoadLabelList   ( char * file );
	int    NewLabels       ( void ); // How many new labels been added since loading/saveing label file
	void   SaveLabelList   ( void );	
};