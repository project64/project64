#ifndef __FILE_CLASS__H__
#define __FILE_CLASS__H__

class CFile
{

// Attributes
	HANDLE  m_hFile;
	bool    m_bCloseOnDelete;
	
public:
// Flag values
	enum OpenFlags {
		modeRead =          0x0000,
		modeWrite =         0x0001,
		modeReadWrite =     0x0002,
		shareCompat =       0x0000,
		shareExclusive =    0x0010,
		shareDenyWrite =    0x0020,
		shareDenyRead =     0x0030,
		shareDenyNone =     0x0040,
		modeNoInherit =     0x0080,
		modeCreate =        0x1000,
		modeNoTruncate =    0x2000,
	};

	enum Attribute {
		normal =    0x00,
		readOnly =  0x01,
		hidden =    0x02,
		system =    0x04,
		volume =    0x08,
		directory = 0x10,
		archive =   0x20
	};

	enum SeekPosition { begin = 0x0, current = 0x1, end = 0x2 };

// Constructors
	CFile();
	CFile(HANDLE hFile);
	CFile(LPCTSTR lpszFileName, DWORD nOpenFlags);

// Deconstructors
	virtual ~CFile();


	// Operations
	virtual bool Open(LPCTSTR lpszFileName, DWORD nOpenFlags );

	DWORD SeekToEnd   ( void );
	void  SeekToBegin ( void );

	// Overridables
	virtual DWORD GetPosition() const;
	virtual long Seek(long lOff, SeekPosition nFrom);
	virtual bool SetLength(DWORD dwNewLen);
	virtual DWORD GetLength() const;

	virtual DWORD Read(void* lpBuf, DWORD nCount);
	virtual bool Write(const void* lpBuf, DWORD nCount);

	virtual bool Flush();
	virtual bool Close();
	virtual bool IsOpen() const;
	virtual bool SetEndOfFile();
};

#endif