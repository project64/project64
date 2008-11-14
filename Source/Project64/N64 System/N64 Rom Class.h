#include "N64 Types.h"

class CN64Rom {
	CNotification * const _Notify; //Original Notify member used to notify the user when something occurs
	
	//constant values
	enum { ReadFromRomSection = 0x400000 };
	
	//class variables
	HANDLE  m_hRomFile, m_hRomFileMapping;
	BYTE *  m_ROMImage;
	DWORD   m_RomFileSize;
	Country m_Country;
	CICChip m_CicChip;
	LanguageStringID m_ErrorMsg;
	stdstr m_RomName, m_FileName, m_MD5, m_RomIdent;

	bool   AllocateAndLoadN64Image ( const char * FileLoc, bool LoadBootCodeOnly );
	bool   AllocateAndLoadZipImage ( const char * FileLoc, bool LoadBootCodeOnly );
	void   ByteSwapRom             ( void );
	bool   IsValidRomImage         ( BYTE Test[4] );
    void   SetError                ( LanguageStringID ErrorMsg );
	static void  __stdcall NotificationCB ( LPCSTR Status, CN64Rom * _this );
	void   CalculateCicChip         ( void );

public:
	        CN64Rom            ( CNotification * Notify );
	       ~CN64Rom            ( void );
	bool    LoadN64Image       ( const char * FileLoc, bool LoadBootCodeOnly = false );
	void    SaveRomSettingID   ( void );
	void    ClearRomSettingID  ( void );
	CICChip CicChipID          ( void );
	BYTE *  GetRomAddress      ( void ) { return m_ROMImage; }
	DWORD   GetRomSize         ( void ) { return m_RomFileSize; }
	stdstr  GetRomMD5          ( void ) { return m_MD5; }
	stdstr  GetRomName         ( void ) { return m_RomName; }
	stdstr  GetFileName        ( void ) { return m_FileName; }
	Country GetCountry         ( void ) { return m_Country; }
	void    UnallocateRomImage ( void );
   
	//Get a message id for the reason that you failed to load the rom
	LanguageStringID GetError  ( void ) { return m_ErrorMsg; }
};
