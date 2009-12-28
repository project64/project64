#pragma once

#include "File Class.h"
#include "CriticalSection.h"
#include "std string.h"
#include <map>

class CIniFileBase 
{
	typedef std::string                       ansi_string;
	typedef std::map<ansi_string,long>        FILELOC;
	typedef FILELOC::iterator                 FILELOC_ITR;
	typedef std::map<ansi_string,ansi_string> KeyValueList;

public:
	typedef std::map<stdstr,stdstr>           KeyValueData;
	typedef std::vector<stdstr>               SectionList;

protected:
	CFileBase   & m_File;
	stdstr m_FileName;

private:
	ansi_string m_CurrentSection;
	bool   m_CurrentSectionDirty;
	int    m_CurrentSectionFilePos; // Where in the file is the current Section
	KeyValueList m_CurrentSectionData;
	
	long   m_lastSectionSearch; // When Scanning for a section, what was the last scanned pos


	bool   m_ReadOnly;
	bool   m_InstantFlush;
	LPCSTR m_LineFeed;

	CriticalSection m_CS;
	FILELOC m_SectionsPos;

	//void AddItemData ( LPCTSTR lpKeyName, LPCTSTR lpString);
	//bool ChangeItemData ( LPCTSTR lpKeyName, LPCTSTR lpString );
	//void DeleteItem ( LPCSTR lpKeyName );
	void fInsertSpaces ( int Pos, int NoOfSpaces );
	int  GetStringFromFile ( char * & String, char * &Data, int & MaxDataSize, int & DataSize, int & ReadPos );
	bool MoveToSectionNameData ( LPCSTR lpSectionName, bool ChangeCurrentSection );
	const char * CleanLine ( char * const Line );
	void SaveCurrentSection ( void );
	void ClearSectionPosList( long FilePos );

protected:
	void OpenIniFileReadOnly();
	void OpenIniFile(bool bCreate = true);

public:
	CIniFileBase( CFileBase & FileObject, LPCTSTR FileName );
	virtual ~CIniFileBase(void);

	bool IsEmpty();
	bool IsFileOpen ( void );
	bool DeleteSection ( LPCSTR lpSectionName );
	bool GetString ( LPCSTR lpSectionName, LPCSTR lpKeyName, LPCSTR lpDefault, stdstr & Value );
	stdstr GetString  ( LPCSTR lpSectionName, LPCSTR lpKeyName, LPCSTR lpDefault );
	ULONG  GetString  ( LPCSTR lpSectionName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, ULONG nSize );
	ULONG GetNumber ( LPCSTR lpSectionName, LPCSTR lpKeyName, ULONG nDefault );
	bool  GetNumber ( LPCSTR lpSectionName, LPCSTR lpKeyName, ULONG nDefault, ULONG & Value );

#ifdef _UNICODE
	bool DeleteSection ( LPCWSTR lpSectionName );
	bool GetString ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, LPCWSTR lpDefault, stdstr & Value );
	stdstr GetString  ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, LPCWSTR lpDefault );
	ULONG  GetString  ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPTSTR lpReturnedString, ULONG nSize );
	ULONG GetNumber ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, ULONG nDefault );
	bool  GetNumber ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, ULONG nDefault, ULONG & Value );

#endif

	virtual void  SaveString ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, LPCTSTR lpString );
	virtual void  SaveNumber ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, ULONG Value );
	void SetAutoFlush (bool AutoFlush);
	void FlushChanges (void);
	void GetKeyList ( LPCTSTR lpSectionName, strlist &List );
	void GetKeyValueData ( LPCTSTR lpSectionName, KeyValueData & List );
	
	void GetVectorOfSections( SectionList & sections); 
	const stdstr &GetFileName() {return m_FileName;}
};

template <class CFileStorage>
class CIniFileT :
	public CIniFileBase
{
public:
	CIniFileT( LPCTSTR FileName ) :
		CIniFileBase(m_FileObject,FileName)
	{
		//Try to open file for reading
		OpenIniFile();

	}
	
	CIniFileT( LPCTSTR FileName, bool bCreate, bool bReadOnly) :
		CIniFileBase(m_FileObject,FileName)
	{
		if(bReadOnly)
		{
			OpenIniFileReadOnly();
		}
		else
		{
			//Try to open file for reading
			OpenIniFile(bCreate);
		}

	}
	virtual ~CIniFileT(void)
	{
	}

protected:
	CFileStorage  m_FileObject;
};

typedef CIniFileT<CFile>   CIniFile;