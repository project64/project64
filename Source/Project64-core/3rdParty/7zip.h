#pragma once
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include "7zip/7zCrc.h"
#include "7zip/7z.h"
#include "7zip/7zFile.h"
#include "7zip/Types.h"

#ifdef __cplusplus
}
#endif

class C7zip
{
public:

    C7zip(const char * FileName);
    ~C7zip();

    typedef void (*LP7ZNOTIFICATION)(const char * Status, void * CBInfo);

    inline int           NumFiles(void)      const { return m_db ? m_db->db.NumFiles : 0; }
    inline CSzFileItem * FileItem(int index) const { return m_db ? &m_db->db.Files[index] : nullptr; }
    inline int           FileSize(void)      const { return m_FileSize; }
    inline bool          OpenSuccess(void)    const { return m_Opened; }

    bool   GetFile(int index, Byte * Data, size_t DataLen);
    const char * FileName(char * FileName, int SizeOfFileName) const;
    std::wstring FileNameIndex(int index);

    void SetNotificationCallback(LP7ZNOTIFICATION NotfyFnc, void * CBInfo);

private:
    C7zip(void);					// Disable default constructor
    C7zip(const C7zip&);			// Disable copy constructor
    C7zip& operator=(const C7zip&);	// Disable assignment

    /*typedef struct _CFileInStream
    {
    ISzInStream InStream;
    FILE *File;
    } CFileInStream;
    */
    CSzArEx          * m_db;
    CFileInStream      m_archiveStream;
    CLookToRead        m_archiveLookStream;
    ISzAlloc           m_allocImp;
    ISzAlloc           m_allocTempImp;
    int                m_FileSize;
    char               m_FileName[260];
    int                m_CurrentFile;
    bool               m_Opened;

    //Used for extraction
    UInt32 m_blockIndex; // it can have any value before first call (if outBuffer = 0)
    Byte * m_outBuffer; // it must be 0 before first call for each new archive.
    size_t m_outBufferSize;  // it can have any value before first call (if outBuffer = 0)

    static void * AllocAllocImp(void *p, size_t size);
    static void AllocFreeImp(void *p, void *address); /* address can be 0 */

    static SRes SzFileReadImp(void *object, void *buffer, size_t *processedSize);
    static SRes SzFileSeekImp(void *p, Int64 *pos, ESzSeek origin);

    //static void __stdcall StatusUpdate(_7Z_STATUS status, int Value1, int Value2, C7zip * _this);

    static void NotfyCallbackDefault(const char * /*Status*/, void * /*CBInfo*/) { }

    LP7ZNOTIFICATION m_NotfyCallback;
    void *           m_NotfyCallbackInfo;
};
