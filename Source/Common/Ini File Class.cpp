#include "stdafx.h"

CIniFileBase::CIniFileBase(CFileBase & FileObject, LPCTSTR FileName) :
	m_lastSectionSearch(0),
	m_CurrentSectionFilePos(0),
	m_LineFeed("\r\n"),
	m_ReadOnly(true),
	m_InstantFlush(true),
	m_File(FileObject),
	m_FileName(FileName),
	m_CurrentSectionDirty(false)
{
}

CIniFileBase::~CIniFileBase(void)
{
	SaveCurrentSection();
}

void CIniFileBase::fInsertSpaces ( int Pos, int NoOfSpaces )
{
	enum { fIS_MvSize  = 0x2000 };

	unsigned char Data[fIS_MvSize + 1];
	int SizeToRead, result;
	long end, WritePos;

	m_File.Seek(0,CFileBase::end);
	end = m_File.GetPosition();

	if (NoOfSpaces > 0) {
		stdstr_f SpaceBuffer(_T("%*c"),NoOfSpaces,' ');

		do {
			SizeToRead = end - Pos;
			if (SizeToRead > fIS_MvSize) { SizeToRead = fIS_MvSize; }
			if (SizeToRead > 0) {
				m_File.Seek(SizeToRead * -1,CFileBase::current);
				WritePos = m_File.GetPosition();
				memset(Data,0,sizeof(Data));
				result = m_File.Read(Data,SizeToRead);
				m_File.Seek(WritePos,CFileBase::begin);
				end = WritePos;

				m_File.Write(SpaceBuffer.c_str(),(ULONG)SpaceBuffer.length());
				m_File.Write(Data,result);
				m_File.Seek(WritePos,CFileBase::begin);
			}
		} while (SizeToRead > 0);
	} if (NoOfSpaces < 0) {
		int ReadPos = Pos + (NoOfSpaces * -1);
		int WritePos = Pos;
		do {
			SizeToRead = end - ReadPos;
			if (SizeToRead > fIS_MvSize) { SizeToRead = fIS_MvSize; }
			m_File.Seek(ReadPos,CFileBase::begin);
			m_File.Read(Data,SizeToRead);
			m_File.Seek(WritePos,CFileBase::begin);
			m_File.Write(Data,SizeToRead);
			ReadPos += SizeToRead;
			WritePos += SizeToRead;
		} while (SizeToRead > 0);
		m_File.Seek(WritePos,CFileBase::begin);
		stdstr_f SpaceBuffer(_T("%*c"),(NoOfSpaces * -1),' ');
		m_File.Write(SpaceBuffer.c_str(),(ULONG)SpaceBuffer.length());

		m_File.Seek(WritePos,CFileBase::begin);
		m_File.SetEndOfFile();
		m_File.Seek(0,CFileBase::begin);
	}
}

int CIniFileBase::GetStringFromFile ( char * & String, char * &Data, int & MaxDataSize, int & DataSize, int & ReadPos )
{
	enum { BufferIncrease = 0x2000 };
	if (MaxDataSize == 0) 
	{
		ReadPos = 0;
		MaxDataSize = BufferIncrease;
		Data = new char[MaxDataSize];
		DataSize = m_File.Read(&Data[DataSize],MaxDataSize);
	}

	for (;;) {
		int count;

		for (count = ReadPos; count < DataSize; count ++) {
			if (Data[count] == '\n') 
			{
				int len = (count - ReadPos) + 1;
				String = &Data[ReadPos];
				String[len-1] = 0;
				ReadPos = count + 1;
				return len;
			}
		}

		if (ReadPos != 0)
		{
			if ((DataSize - ReadPos) > 0 )
			{
				memmove(Data,&Data[ReadPos],DataSize - ReadPos);
			}
			DataSize -= ReadPos;
			ReadPos = 0;
		} else {
			//Increase buffer size
			int NewMaxDataSize = MaxDataSize + BufferIncrease;
			char * NewBuffer = new char[NewMaxDataSize];
			if (NewBuffer == NULL) 
			{
				return -1;
			}
			memcpy(NewBuffer,Data,DataSize);
			MaxDataSize = NewMaxDataSize;
			delete [] Data;
			Data = NewBuffer;
		}

		int dwRead = m_File.Read(&Data[DataSize],MaxDataSize - DataSize);
		if (dwRead == 0) 
		{ 
			if (DataSize > 0)
			{
				int len = DataSize + 1;
				String = &Data[ReadPos];
				String[len-1] = 0;
				DataSize = 0;
				ReadPos = 0;
				return len;
			}
			return -1; 
		}
		DataSize += dwRead;
	}
}

void CIniFileBase::SaveCurrentSection ( void )
{
	if (!m_CurrentSectionDirty)
	{
		return;
	}
	m_CurrentSectionDirty = false;
	if (m_CurrentSection.length() == 0)
	{
		m_CurrentSection = "default";
	}

	int lineFeedLen = (int)strlen(m_LineFeed);

	if (m_CurrentSectionFilePos == -1)
	{
		//Section has not been added yet
		m_File.Seek(0,CFileBase::end);

		int len = (int)m_CurrentSection.length() + (lineFeedLen * 2) + 5;
		AUTO_PTR<char> SectionName(new char[len]);
		if (m_File.GetLength() < (int)strlen(m_LineFeed))
		{
			sprintf(SectionName.get(),"[%s]%s",m_CurrentSection.c_str(),m_LineFeed);
		} else {
			sprintf(SectionName.get(),"%s[%s]%s",m_LineFeed,m_CurrentSection.c_str(),m_LineFeed);
		}
		m_File.Write(SectionName.get(),(int)strlen(SectionName.get()));
		m_CurrentSectionFilePos = m_File.GetPosition();
		m_SectionsPos.insert(FILELOC::value_type(m_CurrentSection,m_CurrentSectionFilePos));
	} else {
		//increase/decrease space needed
		int NeededBufferLen = 0;
		{
			AUTO_PTR<char> LineData(NULL);
			int len = 0;

			for (KeyValueList::iterator iter = m_CurrentSectionData.begin(); iter != m_CurrentSectionData.end(); iter++)
			{
				int newLen = (int)iter->first.length() + (int)iter->second.length() + lineFeedLen + 5;
				if (newLen > len)
				{
					LineData.reset(new char[newLen]);
					len = newLen;
				}
				sprintf(LineData.get(),"%s=%s%s",iter->first.c_str(),iter->second.c_str(),m_LineFeed);
				NeededBufferLen += (int)strlen(LineData.get());
			}
		}
		int currentLen = 0;

		m_File.Seek(m_CurrentSectionFilePos, CFileBase::begin);

		int MaxDataSize = 0, DataSize = 0, ReadPos = 0, result;
		char *Input = NULL, *Data = NULL;
	
		//Skip first line as it is the section name
		int StartPos = m_CurrentSectionFilePos;
		int EndPos = StartPos;
		do {
			result = GetStringFromFile(Input,Data,MaxDataSize,DataSize,ReadPos);
			if (result <= 1) { continue; }
			if (strlen(CleanLine(Input)) <= 1 || 
				Input[0] != '[')
			{
				EndPos = ((m_File.GetPosition() - DataSize) + ReadPos);

				continue; 
			}
			if (Input[0] == '[')
			{
				NeededBufferLen += lineFeedLen;
			}
			break;
		} while (result >= 0);
		currentLen = EndPos - StartPos;

		if (Data) {  delete [] Data;  Data = NULL; }

		if (NeededBufferLen != currentLen)
		{
			fInsertSpaces(StartPos,NeededBufferLen - currentLen);
			m_File.Flush();
			ClearSectionPosList(StartPos);
		}
		//set pointer to beginning of the start pos
		m_File.Seek(StartPos, CFileBase::begin);
	}


	{
		AUTO_PTR<char> LineData(NULL);
		int len = 0;
		
		for (KeyValueList::iterator iter = m_CurrentSectionData.begin(); iter != m_CurrentSectionData.end(); iter++)
		{
			int newLen = (int)iter->first.length() + (int)iter->second.length() + lineFeedLen + 5;
			if (newLen > len)
			{
				LineData.reset(new char[newLen]);
				len = newLen;
			}
			sprintf(LineData.get(),"%s=%s%s",iter->first.c_str(),iter->second.c_str(),m_LineFeed);
			m_File.Write(LineData.get(),(int)strlen(LineData.get()));
		}
	}
	m_File.Flush();
}

bool CIniFileBase::MoveToSectionNameData ( LPCSTR lpSectionName, bool ChangeCurrentSection )
{
	if (strcmp(lpSectionName,m_CurrentSection.c_str()) == 0) 
	{
		return true;
	}
	if (ChangeCurrentSection)
	{
		SaveCurrentSection();
		m_CurrentSection = "";
	}
	
	char *Input = NULL, *Data = NULL;
	int MaxDataSize = 0, DataSize = 0, ReadPos = 0, result;

	FILELOC_ITR iter = m_SectionsPos.find(std::string(lpSectionName));
	bool bFoundSection = false;
	if (iter != m_SectionsPos.end())
	{
		if (ChangeCurrentSection)
		{
			m_CurrentSection = iter->first;
			m_CurrentSectionFilePos = iter->second;
		}
		m_File.Seek(iter->second,CFileBase::begin);
		bFoundSection = true;
	} else {
		m_File.Seek(m_lastSectionSearch, CFileBase::begin);

		//long Fpos;
		BYTE pUTF8[3];
		pUTF8[0] = 0xef;
		pUTF8[1] = 0xbb;
		pUTF8[2] = 0xbf;

		do {
			result = GetStringFromFile(Input,Data,MaxDataSize,DataSize,ReadPos);
			if (result <= 1) { continue; }
			if (strlen(CleanLine(Input)) <= 1) { continue; }

			//We Only care about sections
			char * CurrentSection = Input;

			if(m_lastSectionSearch == 0 && !memcmp(CurrentSection, pUTF8, 3))
			{
				CurrentSection += 3;
			}

			if (CurrentSection[0] != '[') { continue; }
			int lineEndPos = (int)strlen(CurrentSection) - 1;  
			if (CurrentSection[lineEndPos] != ']') { continue; }
			//take off the ']' from the end of the string
			CurrentSection[lineEndPos] = 0;
			CurrentSection += 1;
			m_lastSectionSearch = (m_File.GetPosition() - DataSize) + ReadPos;
			m_SectionsPos.insert(FILELOC::value_type(CurrentSection,m_lastSectionSearch));

			if (_stricmp(lpSectionName,CurrentSection) != 0) { 
				continue;
			}
			if (ChangeCurrentSection)
			{
				m_CurrentSection = lpSectionName;
				m_CurrentSectionFilePos = m_lastSectionSearch;
			} else {
				m_File.Seek(m_lastSectionSearch,CFileBase::begin);
			}
			bFoundSection = true;
			break;
		} while (result >= 0);
	}

	if (bFoundSection && ChangeCurrentSection)
	{
		m_CurrentSectionData.clear();
		do {
			result = GetStringFromFile(Input,Data,MaxDataSize,DataSize,ReadPos);
			if (result <= 1) { continue; }
			if (strlen(CleanLine(Input)) <= 1) { continue; }
			if (Input[0] == '[') { break; }
			char * Pos = strchr(Input,'=');
			if (Pos == NULL) { continue; }
			char * Value = &Pos[1];

			char * Pos1 = Pos-1;
			while(((*Pos1 == ' ') || (*Pos1 == '\t')) && (Pos1 > Input))
			{
				Pos1--;
			}
			Pos1[1] = 0;

			m_CurrentSectionData.insert(KeyValueList::value_type(Input,Value));
		} while (result >= 0);
	}

	if (Data) {  delete [] Data;  Data = NULL; }
	return bFoundSection;
}

const char * CIniFileBase::CleanLine ( char * const Line )
{
	char * Pos = Line;

	//Remove any comment from the line
	while (Pos != NULL) 
	{
		Pos = strchr(Pos,'/');
		if (Pos != NULL) 
		{
			if (Pos[1] == '/') 
			{ 
				if(Pos > Line)
				{
					char * Pos_1 = Pos-1;

					if(Pos_1[0] != ':')
					{
						Pos[0] = 0; 
					}
					else
						Pos += 1; 
				}
				else
				{
					Pos[0] = 0; 
				}
			} 
			else 
			{ 
				Pos += 1; 
			}
		}
	}

	//strip any spaces or line feeds from the end of the line
	for (int count = (int)strlen(&Line[0]) - 1; count >= 0; count --) {
		if (Line[count] != ' ' && Line[count] != '\r') { break; }
		Line[count] = 0;
	}
	return Line;
}

void CIniFileBase::OpenIniFileReadOnly()
{
	if (m_File.Open(m_FileName.c_str(),CFileBase::modeRead))
	{
		m_ReadOnly = true;
		m_File.Seek(0,CFileBase::begin);
	}
}

void CIniFileBase::OpenIniFile(bool bCreate)
{
	//Open for reading/Writing
	m_ReadOnly = false;
	if (!m_File.Open(m_FileName.c_str(),CFileBase::modeReadWrite | CFileBase::shareDenyWrite))
	{
		if (!m_File.Open(m_FileName.c_str(),CFileBase::modeRead))
		{
			if(bCreate)
			{
				if (!m_File.Open(m_FileName.c_str(),CFileBase::modeReadWrite | CFileBase::modeCreate | CFileBase::shareDenyWrite))
				{
					return;
				}
			}
		} 
		else
		{
			m_ReadOnly = true;
		}
	}
	m_File.Seek(0,CFileBase::begin);
}

bool CIniFileBase::IsEmpty()
{
	if(m_File.GetLength()==0)
		return true;

	return false;
}

bool CIniFileBase::IsFileOpen ( void ) 
{ 
	return m_File.IsOpen(); 
}

bool CIniFileBase::DeleteSection ( LPCTSTR lpSectionName )
{
	stdstr_f strSection("[%s]",lpSectionName);

	/*if(m_File.IsOpen())
	{
		m_CurrentSectionFilePos = 0;
		m_File.Seek(m_CurrentSectionFilePos,CFileBase::begin);

		ULONG dwSize = m_File.GetLength();
		if(dwSize)
		{
			char *pData = new char[dwSize+1];
			if(pData)
			{
				ULONG dwRet = m_File.Read(pData, dwSize);
				if(dwRet != 0)
				{
					if(dwRet <= dwSize)
					{
						pData[dwRet] = 0;

						char *pSection = strstr(pData, strSection.c_str());
						if(pSection)
						{
							char tmp = pSection[0];
							pSection[0] = 0;

							std::string strNewData = pData;
							pSection[0] = tmp;

							char *pEndSection = pSection + strlen(strSection.c_str());
							char *pNextSection = strstr(pEndSection, "[");
							if(pNextSection)
							{
								strNewData += pNextSection;
							}

							m_File.Seek(m_CurrentSectionFilePos,CFileBase::begin);
							m_File.Write(strNewData.c_str(), (ULONG)strlen(strNewData.c_str()));
							m_File.Flush();
							m_File.SetEndOfFile();
						}
					}
					else
					{
						delete [] pData;

						return false;
					}
				}

				delete [] pData;
			}
			else
				return false;
		}
	}
	else
		return false;*/

	return true;
}

bool CIniFileBase::GetString ( LPCSTR lpSectionName, LPCSTR lpKeyName, LPCSTR lpDefault, stdstr & Value )
{
	CGuard Guard(m_CS);

	if (lpSectionName == NULL || strlen(lpSectionName) == 0)
	{
		lpSectionName = "default";
	}

	if (m_File.IsOpen() && MoveToSectionNameData(lpSectionName,true)) 
	{
		KeyValueList::iterator iter = m_CurrentSectionData.find(lpKeyName);
		if (iter != m_CurrentSectionData.end())
		{
			Value = iter->second.c_str();
			return true;
		}
	}
	Value = lpDefault;
	return false;
}

stdstr CIniFileBase::GetString  ( LPCSTR lpSectionName, LPCSTR lpKeyName, LPCSTR lpDefault )
{
	stdstr Value;
	GetString(lpSectionName,lpKeyName,lpDefault,Value);
	return Value;
}

#ifdef _UNICODE
bool CIniFileBase::GetString ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, LPCWSTR lpDefault, stdstr & Value )
{
	CGuard Guard(m_CS);

	std::string strSection;

	if (lpSectionName == NULL || wcslen(lpSectionName) == 0)
	{
		strSection = "default";
	} else {
		stdstr::fromTString(lpSectionName,strSection);
	}

	if (m_File.IsOpen() && MoveToSectionNameData(strSection.c_str(),true)) 
	{
		KeyValueList::iterator iter = m_CurrentSectionData.find(lpKeyName);
		if (iter != m_CurrentSectionData.end())
		{
			stdstr::toTString(iter->second.c_str(),Value);
			return true;
		}
	}
	Value = lpDefault;
	return false;
}

stdstr CIniFileBase::GetString  ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, LPCWSTR lpDefault )
{
	stdstr Value;
	GetString(lpSectionName,lpKeyName,lpDefault,Value);
	return Value;
}

#endif

ULONG  CIniFileBase::GetString  ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, ULONG nSize )
{
	CGuard Guard(m_CS);

	std::string strSection;

	if (lpSectionName == NULL || _tcslen(lpSectionName) == 0)
	{
		strSection = "default";
	} else {
		strSection = lpSectionName;
	}

	if (m_File.IsOpen() && MoveToSectionNameData(strSection.c_str(),true)) 
	{
		KeyValueList::iterator iter = m_CurrentSectionData.find(lpKeyName);
		if (iter != m_CurrentSectionData.end())
		{
			_tcsncpy(lpReturnedString,iter->second.c_str(),nSize - 1);
			lpReturnedString[nSize - 1] = 0;
			return (ULONG)_tcslen(lpReturnedString);
		}
	}
	_tcsncpy(lpReturnedString,lpDefault,nSize - 1);
	lpReturnedString[nSize - 1] = 0;
	return (ULONG)_tcslen(lpReturnedString);
}

#ifdef _UNICODE
ULONG CIniFileBase::GetNumber ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, ULONG nDefault )
{
	ULONG Value;
	GetNumber(lpSectionName,lpKeyName,nDefault,Value);
	return Value;
}

bool CIniFileBase::GetNumber ( LPCWSTR lpSectionName, LPCWSTR lpKeyName, ULONG nDefault, ULONG & Value )
{
	std::string strSection;

	if (lpSectionName != NULL && wcslen(lpSectionName) > 0)
	{
		stdstr::fromTString(lpSectionName,strSection);
		return GetNumber(strSection.c_str(),lpKeyName.c_str(),nDefault,Value);
	} else {
		return GetNumber(NULL,lpKeyName.c_str(),nDefault,Value);
	}
}
#endif

ULONG CIniFileBase::GetNumber ( LPCSTR lpSectionName, LPCSTR lpKeyName, ULONG nDefault )
{
	ULONG Value;
	GetNumber(lpSectionName,lpKeyName,nDefault,Value);
	return Value;
}

bool CIniFileBase::GetNumber ( LPCSTR lpSectionName, LPCSTR lpKeyName, ULONG nDefault, ULONG & Value )
{
	CGuard Guard(m_CS);

	if (lpSectionName == NULL || strlen(lpSectionName) == 0)
	{
		lpSectionName = "default";
	}

	if (m_File.IsOpen() && MoveToSectionNameData(lpSectionName,true)) 
	{
		KeyValueList::iterator iter = m_CurrentSectionData.find(lpKeyName);
		if (iter != m_CurrentSectionData.end())
		{
			Value = 0;
			sscanf(iter->second.c_str(), "%u", &Value);
			return true;
		}
	}
	Value = nDefault; 
	return false;
}

void  CIniFileBase::SaveString ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, LPCTSTR lpString )
{
	CGuard Guard(m_CS);
	if (!m_File.IsOpen())
	{
		if (lpString)
		{
			OpenIniFile();
		}
		if (!m_File.IsOpen())
		{
			return;
		}
	}
	std::string strSection;

	if (lpSectionName == NULL || _tcslen(lpSectionName) == 0)
	{
		strSection = "default";
	} else {
		strSection = lpSectionName;
	}

	if (!MoveToSectionNameData(strSection.c_str(),true))
	{
		m_CurrentSection = strSection;
		m_CurrentSectionData.clear();
		m_CurrentSectionFilePos = -1;
	}
	
	KeyValueList::iterator iter = m_CurrentSectionData.find(lpKeyName);
	if (iter != m_CurrentSectionData.end())
	{
		if (lpString)
		{
			if (iter->second != lpString)
			{
				iter->second = lpString;
				m_CurrentSectionDirty = true;
			}
		} else {
			m_CurrentSectionData.erase(iter);
			m_CurrentSectionDirty = true;
		}
	} else {
		if (lpString) 
		{
			m_CurrentSectionData.insert(KeyValueList::value_type(lpKeyName,lpString));
			m_CurrentSectionDirty = true;
		}
	}

	if (m_InstantFlush)
	{
		SaveCurrentSection();
	}
}

void CIniFileBase::SaveNumber ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, ULONG Value )
{
	//translate the string to an ascii version and save as text	
	SaveString(lpSectionName,lpKeyName,stdstr_f(_T("%d"),Value).c_str());
}

void CIniFileBase::FlushChanges (void)
{
	SaveCurrentSection();
}

void CIniFileBase::SetAutoFlush (bool AutoFlush)
{
	m_InstantFlush = AutoFlush;
	if (AutoFlush)
	{
		FlushChanges();
	}
}


void CIniFileBase::GetKeyList ( LPCTSTR lpSectionName, strlist &List )
{
	List.clear();

	CGuard Guard(m_CS);
	if (!m_File.IsOpen())
	{
		return; 
	}

	if (lpSectionName == NULL || _tcslen(lpSectionName) == 0)
	{
		lpSectionName = "default";
	}

	if (MoveToSectionNameData(lpSectionName,true))
	{
		for (KeyValueList::iterator iter = m_CurrentSectionData.begin(); iter != m_CurrentSectionData.end(); iter++)
		{
			List.push_back(iter->first);
		}
	}
}

void CIniFileBase::GetKeyValueData ( LPCTSTR lpSectionName, KeyValueData & List ) 
{
	CGuard Guard(m_CS);
	if (!m_File.IsOpen())
	{
		return; 
	}

	std::string strSection; 

	if (lpSectionName == NULL || _tcslen(lpSectionName) == 0)
	{
		strSection = "default";
	} else {
		strSection = lpSectionName;
	}

	if (!MoveToSectionNameData(strSection.c_str(),false)) { return; }

	int MaxDataSize = 0, DataSize = 0, ReadPos = 0, result;
	char *Input = NULL, *Data = NULL;
	do {
		result = GetStringFromFile(Input,Data,MaxDataSize,DataSize,ReadPos);
		if (result <= 1) { continue; }
		if (strlen(CleanLine(Input)) <= 1) { continue; }
		if (Input[0] == '[') { break; }
		char * Pos = strchr(Input,'=');
		if (Pos == NULL) { continue; }
		Pos[0] = 0;

		List.insert(KeyValueData::value_type(Input,&Pos[1]));
	} while (result >= 0);
	if (Data) {  delete [] Data;  Data = NULL; }
}

void CIniFileBase::ClearSectionPosList( long FilePos ) 
{
	if (FilePos <= 0)
	{
		m_SectionsPos.clear();
		m_lastSectionSearch = 0;
	} else {
		FILELOC::iterator iter = m_SectionsPos.begin();
		while (iter != m_SectionsPos.end())
		{
			FILELOC::iterator CurrentIter = iter;
			iter ++;
			long TestFilePos = CurrentIter->second;
			if (TestFilePos > FilePos)
			{
				m_SectionsPos.erase(CurrentIter);
			}
		}
		m_lastSectionSearch = FilePos;
	}

}

void CIniFileBase::GetVectorOfSections( SectionList & sections) 
{
	sections.clear();

	CGuard Guard(m_CS);
	if (!m_File.IsOpen())
	{
		return; 
	}

	{
		stdstr_f DoesNotExist(_T("DoesNotExist%d%d%d"),rand(),rand(),rand());
		MoveToSectionNameData(DoesNotExist.c_str(),false);
	}

	for (FILELOC::const_iterator iter = m_SectionsPos.begin(); iter != m_SectionsPos.end(); iter++)
	{
		sections.push_back(iter->first);
	}
}
