#ifndef _7ZIP_H_
#define _7ZIP_H_

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
	typedef void (__stdcall *LP7ZNOTIFICATION)( LPCSTR Status, void * CBInfo );

private:
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
	
	static void __stdcall NotfyCallbackDefault ( LPCSTR Status, void * CBInfo ) { }
	LP7ZNOTIFICATION NotfyCallback;
	void *           NotfyCallbackInfo;

public:
	C7zip (LPCSTR FileName);
	~C7zip ();

	inline int         NumFiles ( void )      const { return m_db.Database.NumFiles; }
	inline CFileItem * FileItem ( int index ) const { return &m_db.Database.Files[index]; }

	bool GetFile    ( int index, Byte * Data, size_t DataLen );
	char * FileName ( char * FileName, int SizeOfFileName ) const;
	inline int  FileSize   ( void ) const { return m_FileSize; }

	void SetNotificationCallback (LP7ZNOTIFICATION NotfyFnc, void * CBInfo);
};

#endif