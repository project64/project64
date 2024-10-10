#pragma once
#include <stdint.h>

class CFileBase
{
public:
    enum OpenFlags
    {
        modeRead = 0x0000,
        modeWrite = 0x0001,
        modeReadWrite = 0x0002,
        shareCompat = 0x0000,
        shareExclusive = 0x0010,
        shareDenyWrite = 0x0020,
        shareDenyRead = 0x0030,
        shareDenyNone = 0x0040,
        modeNoInherit = 0x0080,
        modeCreate = 0x1000,
        modeNoTruncate = 0x2000,
    };

    enum SeekPosition
    {
        begin = 0x0,
        current = 0x1,
        end = 0x2
    };

    virtual bool Open(const char * lpszFileName, uint32_t nOpenFlags) = 0;

    virtual uint32_t GetPosition() const = 0;
    virtual int32_t Seek(int32_t lOff, SeekPosition nFrom) = 0;
    virtual bool SetLength(uint32_t dwNewLen) = 0;
    virtual uint32_t GetLength() const = 0;

    virtual uint32_t Read(void * lpBuf, uint32_t nCount) = 0;
    virtual bool Write(const void * lpBuf, uint32_t nCount) = 0;

    virtual bool Flush() = 0;
    virtual bool Close() = 0;
    virtual bool IsOpen() const = 0;
    virtual bool SetEndOfFile() = 0;
};

class CFile : public CFileBase
{
public:
    CFile();
    CFile(void * hFile);
    CFile(const char * lpszFileName, uint32_t nOpenFlags);

    virtual ~CFile();

    virtual bool Open(const char * lpszFileName, uint32_t nOpenFlags);

    uint32_t SeekToEnd(void);
    void SeekToBegin(void);

    virtual uint32_t GetPosition() const;
    virtual int32_t Seek(int32_t lOff, SeekPosition nFrom);
    virtual bool SetLength(uint32_t dwNewLen);
    virtual uint32_t GetLength() const;

    virtual uint32_t Read(void * lpBuf, uint32_t nCount);
    virtual bool ReadInterger(int32_t & Value);
    virtual bool Write(const void * lpBuf, uint32_t nCount);

    virtual bool Flush();
    virtual bool Close();
    virtual bool IsOpen() const;
    virtual bool SetEndOfFile();

private:
    CFile(const CFile &);
    CFile & operator=(const CFile &);

    void * m_hFile;
    bool m_bCloseOnDelete;
};
