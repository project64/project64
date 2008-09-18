#pragma once
//#include <list>     //stl list
//
//typedef const char *      LPCTSTR;
//typedef char *            LPTSTR;
//typedef unsigned char     BYTE;
//typedef std::map<stdstr,long> SECTION_FILELOC;
//
//class CriticalSection;

#include <map>

template <class CFileStorage>
class CIniFileT {
	typedef std::map<std::string,long> SECTION_FILELOC;

protected:
	CFileStorage m_File;
	stdstr m_FileName;
private:
	int    m_SectionDataPos;
	long   m_LastSectionPos;
	long   m_lastSectionSearch;
	bool   m_ReadOnly;
	bool   m_FlushFileOnWrite;
	LPCTSTR m_LineFeed;

	CriticalSection m_CS;
	SECTION_FILELOC m_SectionsPos;

	void  AddItemData           ( LPCTSTR lpKeyName, LPCTSTR lpString,ULONG &BytesMoved )
	{
		char *Input = NULL, *Data = NULL;
		int DataLen = 0, DataLeft, result;
	
		m_File.Seek(m_SectionDataPos,CFileStorage::begin);
		int WritePos = m_SectionDataPos;
		do {
			result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
			if (result <= 1) { continue; }
			if (strlen(CleanLine(Input)) <= 1) { continue; }
			if (Input[0] == '[') { break; }
			WritePos = m_File.GetPosition() - DataLeft;
			continue;
		} while (result >= 0);
	
		//Write the data at the end of the section
		int len = (int)(_tcslen(lpKeyName) + _tcslen(lpString) + _tcslen(m_LineFeed) + 1);
		fInsertSpaces(WritePos,len);
		BytesMoved = len;
		m_File.Seek(WritePos,CFileStorage::begin);
		stdstr_f LineData(_T("%s=%s%s"),lpKeyName,lpString,m_LineFeed);

		std::string strTmpString = stdstr::fromTString(LineData);

		m_File.Write(strTmpString.c_str(),(int)strTmpString.length());
		if (m_FlushFileOnWrite)
		{
			m_File.Flush();
		}
	
		if (Input) { delete [] Input;  Input = NULL; }
		if (Data) {  delete [] Data;  Data = NULL; }
	}

	void  AddSectionName ( LPCTSTR lpSectionName )
	{
		m_File.Seek(0,CFileStorage::end);
		stdstr_f SectionName(_T("%s[%s]%s"),m_LineFeed,lpSectionName,m_LineFeed);

		std::string strTmpString = stdstr::fromTString(SectionName);

		m_File.Write(strTmpString.c_str(),(DWORD)strTmpString.length());
		m_SectionDataPos = m_File.GetPosition();
	}
	bool  ChangeItemData ( LPCTSTR lpKeyName, LPCTSTR lpString, ULONG & BytesMoved )
	{
		std::string strKeyName = stdstr::fromTString(lpKeyName);

		char *Input = NULL, *Data = NULL, * Pos;
		int DataLen = 0, DataLeft, result;
	
		m_File.Seek(m_SectionDataPos,CFileStorage::begin);
		int WritePos = m_SectionDataPos;
		do {
			result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
			if (result <= 1) { continue; }
			
			if (strlen(CleanLine(Input)) <= 1) { continue; }
			if (Input[0] == '[') { break; }
		
			//see if it is a Item
			Pos = strchr(Input,'=');
			if (Pos == NULL) { continue; }
			Pos[0] = 0;
			
			//See if it is the selected item
			if (_stricmp(Input,strKeyName.c_str()) != 0) { 
				WritePos = m_File.GetPosition() - DataLeft;
				continue;
			}
			if (strcmp(&Pos[1],lpString) == 0)
			{
				BytesMoved = 0;
			} else {
				long OldLen = (m_File.GetPosition() - DataLeft) - WritePos;
				int Newlen = (int)(_tcslen(lpKeyName) + _tcslen(lpString) + _tcslen(m_LineFeed) + 1);
				
				if (OldLen != Newlen) {
					fInsertSpaces(WritePos,Newlen - OldLen);
					BytesMoved = Newlen - OldLen;
				}
		
				m_File.Seek(WritePos,CFileStorage::begin);
				stdstr_f NewData(_T("%s=%s%s"),lpKeyName,lpString,m_LineFeed);

				std::string strTmpString = stdstr::fromTString(NewData);

				m_File.Write(strTmpString.c_str(),(DWORD)strTmpString.length());
				if (m_FlushFileOnWrite)
				{
					m_File.Flush();
				}
			}
			if (Input) { delete [] Input;  Input = NULL; }
			if (Data) {  delete [] Data;  Data = NULL; }
			return true;
	
		} while (result >= 0);
		if (Input) { delete [] Input;  Input = NULL; }
		if (Data) {  delete [] Data;  Data = NULL; }
	
		return false;	
	}
	void  DeleteItem ( LPCSTR lpKeyName )
	{
		char *Input = NULL, *Data = NULL, * Pos;
		int DataLen = 0, DataLeft, result, count;
	
		m_lastSectionSearch = 0;
		m_SectionsPos.clear();
	
		m_File.Seek(m_SectionDataPos,CFileStorage::begin);
		int WritePos = m_SectionDataPos;
		do {
			result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
			if (result <= 1) { continue; }
			
			Pos = Input;
			while (Pos != NULL) {
				Pos = strchr(Pos,'/');
				if (Pos != NULL) {
					if (Pos[1] == '/') { Pos[0] = 0; } else { Pos += 1; }
				}
			}
			
			for (count = (int)strlen(&Input[0]) - 1; count >= 0; count --) {
				if (Input[count] != ' ' && Input[count] != '\r') { break; }
				Input[count] = 0;
			}
			//stip leading spaces
			if (strlen(Input) <= 1) { continue; }
			if (Input[0] == '[') { break; }
		
			//see if it is a Item
			Pos = strchr(Input,'=');
			if (Pos == NULL) { continue; }
			Pos[0] = 0;
			
			//See if it is the selected item
			if (_stricmp(Input,lpKeyName) != 0) { 
				WritePos = m_File.GetPosition() - DataLeft;
				continue;
			}
	
			long Length = (m_File.GetPosition() - DataLeft) - WritePos;		
			fInsertSpaces(WritePos,Length * -1);
			if (Input) { delete [] Input;  Input = NULL; }
			if (Data) {  delete [] Data;  Data = NULL; }
			return;
		} while (result >= 0);
		if (Input) { delete [] Input;  Input = NULL; }
		if (Data) {  delete [] Data;  Data = NULL; }
	}
	void  fInsertSpaces         ( int Pos, int NoOfSpaces )
	{
		enum { fIS_MvSize  = 0x1000 };
	
		unsigned char Data[fIS_MvSize + 1];
		int SizeToRead, result;
		long end, WritePos;
	
		m_File.Seek(0,CFileStorage::end);
		end = m_File.GetPosition();
		
		if (NoOfSpaces > 0) {
			do {
				SizeToRead = end - Pos;
				if (SizeToRead > fIS_MvSize) { SizeToRead = fIS_MvSize; }
				if (SizeToRead > 0) {
					m_File.Seek(SizeToRead * -1,CFileStorage::current);
					WritePos = m_File.GetPosition();
					memset(Data,0,sizeof(Data));
					result = m_File.Read(Data,SizeToRead);
					m_File.Seek(WritePos,CFileStorage::begin);
					end = WritePos;
					stdstr_f SpaceBuffer(_T("%*c"),NoOfSpaces,' ');
					
					std::string strTmpString = stdstr::fromTString(SpaceBuffer);

					m_File.Write(strTmpString.c_str(),(DWORD)strTmpString.length());
					m_File.Write(Data,result);
					m_File.Seek(WritePos,CFileStorage::begin);
				}
			} while (SizeToRead > 0);
		} if (NoOfSpaces < 0) {
			int ReadPos = Pos + (NoOfSpaces * -1);
			int WritePos = Pos;
			do {
				SizeToRead = end - ReadPos;
				if (SizeToRead > fIS_MvSize) { SizeToRead = fIS_MvSize; }
				m_File.Seek(ReadPos,CFileStorage::begin);
				m_File.Read(Data,SizeToRead);
				m_File.Seek(WritePos,CFileStorage::begin);
				m_File.Write(Data,SizeToRead);
				ReadPos += SizeToRead;
				WritePos += SizeToRead;
			} while (SizeToRead > 0);
			m_File.Seek(WritePos,CFileStorage::begin);
			stdstr_f SpaceBuffer(_T("%*c"),(NoOfSpaces * -1),' ');
			
			std::string strTmpString = stdstr::fromTString(SpaceBuffer);

			m_File.Write(strTmpString.c_str(),(DWORD)strTmpString.length());
			if (m_FlushFileOnWrite)
			{
				m_File.Flush();
			}
	
			m_File.Seek(WritePos,CFileStorage::begin);
			m_File.SetEndOfFile();
			m_File.Seek(0,CFileStorage::begin);
		}
	}
	int   GetStringFromFile     ( char **String, char **Data, int * DataSize, int *Left )
	{
		enum { BufferIncrease = 1024 };
		int dwRead = BufferIncrease;
	
		if (*DataSize == 0) { 
			*DataSize = BufferIncrease;
			*Data = new char[*DataSize];
			*Left = 0;
		}
	
		for (;;) {
			int count;
	
			for (count = 0; count < *Left; count ++) {
				if ((*Data)[count] == '\n') {
					if (*String != NULL) { 
						delete [] *String; 
						*String = NULL;
					}
					*String = new char[count + 1];
					strncpy(*String,*Data,count);
					(*String)[count] = 0;
					*Left -= count + 1;
					if (*Left > 0) {
						memmove(&((*Data)[0]),&((*Data)[count + 1]),*Left);
					}
					return count + 1;
				}
			}
			
			if (dwRead == 0) { return -1; }
			if ((*DataSize - *Left) == 0) {
				*DataSize += BufferIncrease;
				char * NewBuffer = new char[*DataSize];
				memcpy(NewBuffer,*Data,*Left);
				delete [] *Data;
				*Data = NewBuffer;
				if (*Data == NULL) {
					return -1;
				}
			}
			dwRead = m_File.Read(&((*Data)[*Left]),*DataSize - *Left);
			*Left += dwRead;
		}
	}

	bool  MoveToSectionNameData ( LPCSTR lpSectionName )
	{
		char *Input = NULL, *Data = NULL, CurrentSection[300] = "";
		int DataLen = 0, DataLeft = 0, result;
	
		if (m_LastSectionPos != 0)
		{
			m_File.Seek(m_LastSectionPos,CFileStorage::begin);
			result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
			if (result > 0 && strlen(CleanLine(Input)) > 0)
			{
				int nIndex = 0;
				BYTE pUTF8[3];
				pUTF8[0] = 0xef;
				pUTF8[1] = 0xbb;
				pUTF8[2] = 0xbf;
				if(!memcmp(Input, pUTF8, 3))
					nIndex = 3;

				if (Input[nIndex] == '[' && Input[strlen(Input) - 1] == ']')
				{
					strcpy(CurrentSection,&Input[nIndex+1]);
					//take off the ']' from the end of the string
					CurrentSection[strlen(CurrentSection) - 1] = 0;
				}
			}
			if (strcmp(lpSectionName,CurrentSection) == 0) 
			{
				//Set the file position at the beginning of the data
				m_File.Seek(DataLeft * -1,CFileStorage::current);
				if (Input) { delete [] Input;  Input = NULL; }
				if (Data) {  delete [] Data;  Data = NULL; }
				//Pointer to data section
				m_SectionDataPos = m_File.GetPosition();
				return true;
			}
			if (Input) { delete [] Input;  Input = NULL; }
			if (Data) {  delete [] Data;  Data = NULL; }
			DataLen = 0;
			DataLeft = 0;
			strcpy(CurrentSection,"");
		}
	
	
		SECTION_FILELOC::iterator iter = m_SectionsPos.find(std::string(lpSectionName));
		if (iter != m_SectionsPos.end())
		{
			m_File.Seek(iter->second,CFileStorage::begin);
			result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
			if (result > 0 && strlen(CleanLine(Input)) > 0)
			{
				int nIndex = 0;
				BYTE pUTF8[3];
				pUTF8[0] = 0xef;
				pUTF8[1] = 0xbb;
				pUTF8[2] = 0xbf;
				if(!memcmp(Input, pUTF8, 3))
					nIndex = 3;

				if (Input[nIndex] == '[' && Input[strlen(Input) - 1] == ']')
				{
					strcpy(CurrentSection,&Input[nIndex+1]);
					//take off the ']' from the end of the string
					CurrentSection[strlen(CurrentSection) - 1] = 0;
				}
			}
			if (strcmp(lpSectionName,CurrentSection) == 0) 
			{
				//Set the file position at the beginning of the data
				m_File.Seek(DataLeft * -1,CFileStorage::current);
				if (Input) { delete [] Input;  Input = NULL; }
				if (Data) {  delete [] Data;  Data = NULL; }
				//Pointer to data section
				m_SectionDataPos = m_File.GetPosition();
				return true;
			}
			if (Input) { delete [] Input;  Input = NULL; }
			if (Data) {  delete [] Data;  Data = NULL; }
			DataLen = 0;
			DataLeft = 0;
			strcpy(CurrentSection,"");
			m_lastSectionSearch = 0;
			m_SectionsPos.clear();
		}
	
		m_File.Seek(m_lastSectionSearch, CFileStorage::begin);
			
		long Fpos;
		do {
			Fpos = m_File.GetPosition() - DataLeft;
			result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
			if (result <= 1) { continue; }
			
			if (strlen(CleanLine(Input)) <= 1) { continue; }
			
			//We Only care about sections
			int nIndex = 0;
			BYTE pUTF8[3];
			pUTF8[0] = 0xef;
			pUTF8[1] = 0xbb;
			pUTF8[2] = 0xbf;
			if(!memcmp(Input, pUTF8, 3))
				nIndex = 3;

			if (Input[nIndex] != '[') { continue; }
	
	
			if (Input[strlen(Input) - 1] != ']') { continue; }
			strcpy(CurrentSection,&Input[nIndex+1]);
			//take off the ']' from the end of the string
			CurrentSection[strlen(CurrentSection) - 1] = 0;
			m_SectionsPos.insert(SECTION_FILELOC::value_type(CurrentSection,Fpos));
			m_lastSectionSearch = m_File.GetPosition() - DataLeft;
			
			if (_stricmp(lpSectionName,CurrentSection) != 0) { 
				continue;
			}
			m_LastSectionPos = Fpos;
	
			//Set the file position at the beginning of the data
			m_File.Seek(DataLeft * -1,CFileStorage::current);
			if (Input) { delete [] Input;  Input = NULL; }
			if (Data) {  delete [] Data;  Data = NULL; }
			//Pointer to data section
			m_SectionDataPos = m_File.GetPosition();
			return true;
		} while (result >= 0);
		if (Input) { delete [] Input;  Input = NULL; }
		if (Data) {  delete [] Data;  Data = NULL; }
	
		return false;
	}
	const char *  CleanLine     ( char * const Line )
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

	void OpenIniFileReadOnly()
	{
		if (m_File.Open(m_FileName.c_str(),CFileStorage::modeRead))
		{
			m_ReadOnly = true;
			m_File.Seek(0,CFileStorage::begin);
		}
	}
	void OpenIniFile(bool bCreate = true)
	{
		//Open for reading/Writing
		m_ReadOnly = false;
		if (!m_File.Open(m_FileName.c_str(),CFileStorage::modeReadWrite))
		{
			if (!m_File.Open(m_FileName.c_str(),CFileStorage::modeRead))
			{
				if(bCreate)
				{
					if (!m_File.Open(m_FileName.c_str(),CFileStorage::modeReadWrite | CFileStorage::modeCreate))
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
		m_File.Seek(0,CFileStorage::begin);
	}
	

public:
	 CIniFileT(LPCTSTR FileName) :
	  //m_File(NULL),
	  //m_CS(new CriticalSection),
		m_LastSectionPos(0),
		m_lastSectionSearch(0),
		m_SectionDataPos(0),
		m_LineFeed(_T("\r\n")),
		m_ReadOnly(true),
		m_FlushFileOnWrite(true)
	{
		m_FileName = FileName;

		//Try to open file for reading
		OpenIniFile();
	}

	CIniFileT(LPCTSTR FileName, bool bCreate, bool bReadOnly) :
	//m_File(NULL),
	//m_CS(new CriticalSection),
	m_LastSectionPos(0),
		m_lastSectionSearch(0),
		m_SectionDataPos(0),
		m_LineFeed(_T("\r\n")),
		m_ReadOnly(bReadOnly),
		m_FlushFileOnWrite(true)
	{
		m_FileName = FileName;

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

	~CIniFileT(void)
	{
	}

	bool IsEmpty()
	{
		if(m_File.GetLength()==0)
			return true;

		return false;
	}

	bool    IsFileOpen ( void ) { return m_File.IsOpen(); }

	bool  DeleteSection ( LPCTSTR lpSectionName )
	{
		std::string strSection;
		
		strSection = "[";
		strSection += stdstr::fromTString(lpSectionName); 
		strSection += "]";

		if(m_File.IsOpen())
		{
			m_SectionDataPos = 0;
			m_File.Seek(m_SectionDataPos,CFileStorage::begin);

			DWORD dwSize = m_File.GetLength();
			if(dwSize)
			{
				char *pData = new char[dwSize+1];
				if(pData)
				{
					DWORD dwRet = m_File.Read(pData, dwSize);
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

								m_File.Seek(m_SectionDataPos,CFileStorage::begin);
								m_File.Write(strNewData.c_str(), (DWORD)strlen(strNewData.c_str()));
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
			return false;

		return true;
	}

	bool GetString ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, LPCTSTR lpDefault, stdstr & Value )
	{
		CGuard Guard(m_CS);

		std::string strSection, strKeyName = stdstr::fromTString(lpKeyName); 

		if (lpSectionName == NULL || _tcslen(lpSectionName) == 0)
		{
			strSection = "default";
		}
		else
			strSection = stdstr::fromTString(lpSectionName);
        
		if (m_File.IsOpen() && MoveToSectionNameData(strSection.c_str())) 
		{
			char *Input = NULL, *Data = NULL, * Pos, *Pos1;
			int DataLen = 0, DataLeft, result;

			m_File.Seek(m_SectionDataPos,CFileStorage::begin);
			int WritePos = m_SectionDataPos;
			do {
				result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
				if (result <= 1) { continue; }

				if (strlen(CleanLine(Input)) <= 1) { continue; }
				if (Input[0] == '[') { break; }

				Pos = strchr(Input,'=');
				if (Pos == NULL) { continue; }
				Pos1 = Pos-1;
				while(((*Pos1 == ' ') || (*Pos1 == '\t')) && (Pos1 > Input))
					Pos1--;
				Pos1[1] = 0;
				if (strcmp(Input,strKeyName.c_str()) != 0) { continue; }
				Value = stdstr::toTString(&Pos[1]);
				if (Input) { delete [] Input;  Input = NULL; }
				if (Data) {  delete [] Data;  Data = NULL; }
				return true;
			} while (result >= 0);
			if (Input) { delete [] Input;  Input = NULL; }
			if (Data) {  delete [] Data;  Data = NULL; }
		}
		Value = lpDefault;
		return false;
	}

	stdstr  GetString  ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, LPCTSTR lpDefault )
	{
		stdstr Value;
		GetString(lpSectionName,lpKeyName,lpDefault,Value);
		return Value;
	}

	DWORD   GetString  ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize )
	{
		CGuard Guard(m_CS);
	
		std::string strSection, strKeyName = stdstr::fromTString(lpKeyName);

		if (lpSectionName == NULL || _tcslen(lpSectionName) == 0)
		{
			strSection = "default";
		}
		else
			strSection = stdstr::fromTString(lpSectionName);
	
		if (m_File.IsOpen() && MoveToSectionNameData(strSection.c_str())) 
		{
			char *Input = NULL, *Data = NULL, * Pos, *Pos1;
			int DataLen = 0, DataLeft, result;
	
			m_File.Seek(m_SectionDataPos,CFileStorage::begin);
			int WritePos = m_SectionDataPos;
			do {
				result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
				if (result <= 1) { continue; }
				
				if (strlen(CleanLine(Input)) <= 1) { continue; }
				if (Input[0] == '[') { break; }
			
				Pos = strchr(Input,'=');
				if (Pos == NULL) { continue; }
				Pos1 = Pos-1;
				while(((*Pos1 == ' ') || (*Pos1 == '\t')) && (Pos1 > Input))
					Pos1--;
				Pos1[1] = 0;
				if (strcmp(Input,strKeyName.c_str()) != 0) { continue; }

				stdstr ReturnedString = stdstr::toTString(&Pos[1]);

				_tcsncpy(lpReturnedString,ReturnedString.c_str(),nSize - 1);
				lpReturnedString[nSize - 1] = 0;
				if (Input) { delete [] Input;  Input = NULL; }
				if (Data) {  delete [] Data;  Data = NULL; }
				return (DWORD)_tcslen(lpReturnedString);
			} while (result >= 0);
			if (Input) { delete [] Input;  Input = NULL; }
			if (Data) {  delete [] Data;  Data = NULL; }
		}
	
		_tcsncpy(lpReturnedString,lpDefault,nSize - 1);
		lpReturnedString[nSize - 1] = 0;
		return (DWORD)_tcslen(lpReturnedString);
	}

	bool GetNumber ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, DWORD nDefault, DWORD & Value )
	{
		CGuard Guard(m_CS);
	
		std::string strSection, strKeyName = stdstr::fromTString(lpKeyName);

		if (lpSectionName == NULL || _tcslen(lpSectionName) == 0)
		{
			strSection = "default";
		}
		else
			strSection = stdstr::fromTString(lpSectionName);
	
		if (m_File.IsOpen() && MoveToSectionNameData(strSection.c_str())) 
		{
			char *Input = NULL, *Data = NULL, * Pos, *Pos1;
			int DataLen = 0, DataLeft, result;
	
			m_File.Seek(m_SectionDataPos,CFileStorage::begin);
			int WritePos = m_SectionDataPos;
			do {
				result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
				if (result <= 1) { continue; }
				
				if (strlen(CleanLine(Input)) <= 1) { continue; }
				if (Input[0] == '[') { break; }
			
				Pos = strchr(Input,'=');
				if (Pos == NULL) { continue; }
				Pos1 = Pos-1;
				while(((*Pos1 == ' ') || (*Pos1 == '\t')) && (Pos1 > Input))
					Pos1--;
				Pos1[1] = 0;
	
				if (_stricmp(Input,strKeyName.c_str()) != 0) { continue; }
				result = atoi(&Pos[1]);
				if (Input) { delete [] Input;  Input = NULL; }
				if (Data) {  delete [] Data;  Data = NULL; }
				Value = result;
				return true;
			} while (result >= 0);
			if (Input) { delete [] Input;  Input = NULL; }
			if (Data) {  delete [] Data;  Data = NULL; }
		}
	
		Value = nDefault; 
		return false;
	}
	virtual void    SaveString ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, LPCTSTR lpString )
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
		std::string strSection, strKeyName = stdstr::fromTString(lpKeyName);
		std::string strString =  stdstr::fromTString(lpString);

		if (lpSectionName == NULL || _tcslen(lpSectionName) == 0)
		{
			strSection = "default";
		}
		else
			strSection = stdstr::fromTString(lpSectionName);
		
		if (lpString == NULL) {
			if (!MoveToSectionNameData(strSection.c_str())) { return; }
			DeleteItem(strKeyName.c_str());
			return;
		}
		if (!MoveToSectionNameData(strSection.c_str())) {
			AddSectionName(lpSectionName);
		}
		ULONG BytesMoved;
		if (!ChangeItemData(lpKeyName,lpString, BytesMoved))
		{
			AddItemData(lpKeyName,lpString,BytesMoved);
		}
		
		if (BytesMoved != 0)
		{
			m_lastSectionSearch = 0;
			ClearSectionPosList(m_SectionDataPos);
		}
	}
	virtual void    SaveNumber ( LPCTSTR lpSectionName, LPCTSTR lpKeyName, DWORD Value )
	{
		//translate the string to an ascii version and save as text	
		SaveString(lpSectionName,lpKeyName,stdstr_f(_T("%d"),Value).c_str());
	}
	void SetAutoFlush (bool AutoFlush)
	{
		m_FlushFileOnWrite = AutoFlush;
		if (AutoFlush)
		{
			m_File.Flush();
		}
	}

	
	void GetKeyList ( LPCTSTR lpSectionName, strlist &List ) {

		CGuard Guard(m_CS);
		if (!m_File.IsOpen())
		{
			return; 
		}
		
		std::string strSection; 

		if (lpSectionName == NULL || _tcslen(lpSectionName) == 0)
		{
			strSection = "default";
		}
		else
			strSection = stdstr::fromTString(lpSectionName);

		if (!MoveToSectionNameData(strSection.c_str())) { return; }
		
		int DataLen = 0, DataLeft, result;
		char *Input = NULL, *Data = NULL;
		do {
			result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
			if (result <= 1) { continue; }
			if (strlen(CleanLine(Input)) <= 1) { continue; }
			if (Input[0] == '[') { break; }
			char * Pos = strchr(Input,'=');
			if (Pos == NULL) { continue; }
			Pos[0] = 0;
			List.push_back(stdstr::toTString(Input));
		} while (result >= 0);
		if (Input) { delete [] Input;  Input = NULL; }
		if (Data) {  delete [] Data;  Data = NULL; }
	}

	typedef std::map<stdstr,stdstr> KeyValueList;

	void GetKeyValueList ( LPCTSTR lpSectionName, KeyValueList & List ) {
		CGuard Guard(m_CS);
		if (!m_File.IsOpen())
		{
			return; 
		}
		
		std::string strSection; 

		if (lpSectionName == NULL || _tcslen(lpSectionName) == 0)
		{
			strSection = "default";
		}
		else
			strSection = stdstr::fromTString(lpSectionName);

		if (!MoveToSectionNameData(strSection.c_str())) { return; }
		
		int DataLen = 0, DataLeft, result;
		char *Input = NULL, *Data = NULL;
		do {
			result = GetStringFromFile(&Input,&Data,&DataLen,&DataLeft);
			if (result <= 1) { continue; }
			if (strlen(CleanLine(Input)) <= 1) { continue; }
			if (Input[0] == '[') { break; }
			char * Pos = strchr(Input,'=');
			if (Pos == NULL) { continue; }
			Pos[0] = 0;

			List.insert(KeyValueList::value_type(stdstr::toTString(Input),stdstr::toTString(&Pos[1])));
		} while (result >= 0);
		if (Input) { delete [] Input;  Input = NULL; }
		if (Data) {  delete [] Data;  Data = NULL; }
	}

	void ClearSectionPosList( long FilePos ) 
	{
		if (FilePos <= 0)
		{
			m_SectionsPos.clear();
			return;
		}

		SECTION_FILELOC::iterator iter = m_SectionsPos.begin();
		while (iter != m_SectionsPos.end())
		{
			SECTION_FILELOC::iterator CurrentIter = iter;
			iter ++;
			if (CurrentIter->second >= FilePos)
			{
				m_SectionsPos.erase(CurrentIter);
			}
		}
	}

	void GetVectorOfSections( std::vector<stdstr> & sections) 
	{
		sections.clear();

		CGuard Guard(m_CS);
		if (!m_File.IsOpen())
		{
			return; 
		}

		stdstr_f DoesNotExist(_T("DoesNotExist%d%d%d"),rand(),rand(),rand());
		MoveToSectionNameData(stdstr::fromTString(DoesNotExist).c_str());

		for (SECTION_FILELOC::const_iterator iter = m_SectionsPos.begin(); iter != m_SectionsPos.end(); iter++)
		{
			const std::string & Section = iter->first;
			sections.push_back(stdstr::toTString(Section.c_str()));
		}
	}

	const stdstr &GetFileName() {return m_FileName;}
};

typedef CIniFileT<CFile>   CIniFile;

