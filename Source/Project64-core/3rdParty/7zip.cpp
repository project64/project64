typedef const char *     LPCSTR;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include "7zip.h"
#include <Common/StdString.h>

C7zip::C7zip(const char * FileName) :
m_FileSize(0),
m_CurrentFile(-1),
m_blockIndex(0xFFFFFFFF),
m_outBuffer(0),
m_outBufferSize(0),
m_NotfyCallback(NotfyCallbackDefault),
m_NotfyCallbackInfo(nullptr),
m_db(nullptr),
m_Opened(false)
{
    memset(&m_FileName, 0, sizeof(m_FileName));
    memset(&m_archiveLookStream, 0, sizeof(m_archiveLookStream));

    m_db = new CSzArEx;
    memset(m_db, 0, sizeof(CSzArEx));

    m_archiveStream.s.Read = SzFileReadImp;
    m_archiveStream.s.Seek = SzFileSeekImp;

    m_allocImp.Alloc = AllocAllocImp;
    m_allocImp.Free = AllocFreeImp;

    m_allocTempImp.Alloc = AllocAllocImp;
    m_allocTempImp.Free = AllocFreeImp;

    InFile_Open(&m_archiveStream.file, FileName);
    if (m_archiveStream.file.handle == INVALID_HANDLE_VALUE)
    {
        //PrintError("Can not open input file");
        return;
    }
    m_FileSize = GetFileSize(m_archiveStream.file.handle, nullptr);

    char drive[_MAX_DRIVE], dir[_MAX_DIR], ext[_MAX_EXT];
    _splitpath(FileName, drive, dir, m_FileName, ext);

    CrcGenerateTable();
    SzArEx_Init(m_db);

    LookToRead_Init(&m_archiveLookStream);
    LookToRead_CreateVTable(&m_archiveLookStream, False);
    m_archiveLookStream.realStream = &m_archiveStream.s;

    SRes res = SzArEx_Open(m_db, &m_archiveLookStream.s, &m_allocImp, &m_allocTempImp);
    if (res == SZ_OK)
    {
        m_Opened = true;
    }
    else
    {
        // SzArEx_Open will delete the passed database if it fails
        m_db = nullptr;
    }
}

C7zip::~C7zip(void)
{
    if (m_db)
    {
        delete m_db;
        m_db = nullptr;
    }
}

void C7zip::SetNotificationCallback(LP7ZNOTIFICATION NotfyFnc, void * CBInfo)
{
    m_NotfyCallback = NotfyFnc ? NotfyFnc : NotfyCallbackDefault;
    m_NotfyCallbackInfo = CBInfo;
}


bool C7zip::GetFile(int index, Byte * Data, size_t DataLen)
{
    m_CurrentFile = -1;
    if (Data == nullptr || DataLen == 0)
    {
        return false;
    }
    if (m_archiveStream.file.handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    m_CurrentFile = index;

    size_t offset;
    size_t outSizeProcessed;

    char Msg[200];
    std::wstring FileName = FileNameIndex(index);
    _snprintf(Msg, sizeof(Msg) / sizeof(Msg[0]), "extracting %s", stdstr().FromUTF16(FileName.c_str()).c_str());
    m_NotfyCallback(Msg, m_NotfyCallbackInfo);

    SRes res = SzArEx_Extract(m_db, &m_archiveLookStream.s, index,
        &m_blockIndex, &m_outBuffer, &m_outBufferSize,
        &offset, &outSizeProcessed,
        &m_allocImp, &m_allocTempImp);
    if (res != SZ_OK)
    {
        m_CurrentFile = -1;
        return false;
    }

    if (DataLen < outSizeProcessed)
    {
        outSizeProcessed = DataLen;
    }
    memcpy(Data, m_outBuffer + offset, outSizeProcessed);
    m_NotfyCallback("", m_NotfyCallbackInfo);
    m_CurrentFile = -1;
    return true;
}

void * C7zip::AllocAllocImp(void * /*p*/, size_t size)
{
    return malloc(size);
    //return new BYTE[size];
}

void C7zip::AllocFreeImp(void * /*p*/, void *address)
{
    if (address != nullptr)
    {
        free(address);
    }
}

SRes C7zip::SzFileReadImp(void *object, void *buffer, size_t *processedSize)
{
    CFileInStream *p = (CFileInStream *)object;
    DWORD dwRead;
    if (!ReadFile(p->file.handle, buffer, (DWORD)*processedSize, &dwRead, nullptr))
    {
        return SZ_ERROR_FAIL;
    }
    //p->s.curpos += read_sz;
    *processedSize = dwRead;
    return SZ_OK;
}

SRes C7zip::SzFileSeekImp(void *p, Int64 *pos, ESzSeek origin)
{
    CFileInStream *s = (CFileInStream *)p;
    DWORD dwMoveMethod;

    switch (origin)
    {
    case SZ_SEEK_SET:
        dwMoveMethod = FILE_BEGIN;
        break;
    case SZ_SEEK_CUR:
        dwMoveMethod = FILE_CURRENT;
        break;
    case SZ_SEEK_END:
        dwMoveMethod = FILE_END;
        break;
    default:
        return SZ_ERROR_FAIL;
    }
    *pos = SetFilePointer(s->file.handle, (LONG)*pos, nullptr, dwMoveMethod);
    if (*pos == INVALID_SET_FILE_POINTER)
    {
        return SZ_ERROR_FAIL;
    }
    return SZ_OK;
}

const char * C7zip::FileName(char * FileName, size_t SizeOfFileName) const
{
    if (FileName == nullptr)
    {
        return nullptr;
    }
    size_t Len = strlen(m_FileName);
    if (Len > (SizeOfFileName - 1))
    {
        Len = (SizeOfFileName - 1);
    }
    strncpy(FileName, m_FileName, Len);
    FileName[Len] = 0;
    return FileName;
}

std::wstring C7zip::FileNameIndex(int index)
{
    std::wstring filename;
    if (m_db == nullptr || m_db->FileNameOffsets == 0)
    {
        // No filename
        return filename;
    }
    size_t namelen = SzArEx_GetFileNameUtf16(m_db, index, nullptr);
    if (namelen <= 0)
    {
        // No filename
        return filename;
    }
    filename.resize(namelen);
    SzArEx_GetFileNameUtf16(m_db, index, (UInt16 *)filename.c_str());
    return filename;
}