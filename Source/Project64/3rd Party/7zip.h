#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "7zip\7zCrc.h"
#include "7zip\7zIn.h"
#include "7zip\7zExtract.h"

#ifdef __cplusplus
}
#endif


class C7zip
{
public:

	C7zip (LPCSTR FileName);
	~C7zip ();

	typedef void (__stdcall *LP7ZNOTIFICATION)( LPCSTR Status, void * CBInfo );

	inline int         NumFiles ( void )      const { return m_db.Database.NumFiles; }
	inline CFileItem * FileItem ( int index ) const { return &m_db.Database.Files[index]; }
	inline int         FileSize ( void )      const { return m_FileSize; }

	bool   GetFile    ( int index, Byte * Data, size_t DataLen );
	const char * FileName ( char * FileName, int SizeOfFileName ) const;

	void SetNotificationCallback (LP7ZNOTIFICATION NotfyFnc, void * CBInfo);

private:
	C7zip(void);					// Disable default constructor
	C7zip(const C7zip&);			// Disable copy constructor
	C7zip& operator=(const C7zip&);	// Disable assignment

	typedef struct _CFileInStream
	{
	  ISzInStream InStream;
	  FILE *File;
	} CFileInStream;

	CArchiveDatabaseEx m_db;
	CFileInStream      m_archiveStream;
	ISzAlloc           m_allocImp;
	ISzAlloc           m_allocTempImp;
	int                m_FileSize;
	char               m_FileName[260];
	int                m_CurrentFile;

	//Used for extraction
	UInt32 m_blockIndex ; // it can have any value before first call (if outBuffer = 0) 
	Byte * m_outBuffer; // it must be 0 before first call for each new archive. 
	size_t m_outBufferSize;  // it can have any value before first call (if outBuffer = 0) 

	static SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize);
	static SZ_RESULT SzFileSeekImp(void *object, CFileSize pos);

	static void __stdcall StatusUpdate(_7Z_STATUS status, int Value1, int Value2, C7zip * _this);
	
	static void __stdcall NotfyCallbackDefault ( LPCSTR /*Status*/, void * /*CBInfo*/ ) { }

	LP7ZNOTIFICATION m_NotfyCallback;
	void *           m_NotfyCallbackInfo;
};
