#include "stdafx.h"
#include "Symbols.h"

bool CSymbols::m_bInitialized = false;
vector<CSymbolEntry*> CSymbols::m_Symbols;
int CSymbols::m_NextSymbolId;

CFile CSymbols::m_SymFileHandle;
char* CSymbols::m_SymFileBuffer;
size_t CSymbols::m_SymFileSize;

char CSymbols::m_ParserDelimeter;
char* CSymbols::m_ParserToken;
size_t CSymbols::m_ParserTokenLength;
bool CSymbols::m_bHaveFirstToken;
char* CSymbols::m_SymFileParseBuffer;

CRITICAL_SECTION CSymbols::m_CriticalSection = {0};

int CSymbols::GetTypeNumber(char* typeName)
{
	const char* name;
	for (int i = 0; (name = SymbolTypes[i]) != NULL; i++)
	{
		if (strcmp(typeName, name) == 0)
		{
			return i;
		}
	}
	return -1;
}

const char* CSymbols::GetTypeName(int typeNumber)
{
	if (typeNumber > 11)
	{
		return NULL;
	}
	return SymbolTypes[typeNumber];
}

// Open symbols file for game and parse into list

CPath CSymbols::GetSymFilePath()
{
	stdstr symFileName;
	symFileName.Format("%s.sym", g_Settings->LoadStringVal(Game_GameName).c_str());

	CPath symFilePath(g_Settings->LoadStringVal(Directory_NativeSave).c_str(), symFileName.c_str());

	if (g_Settings->LoadBool(Setting_UniqueSaveDir))
	{
		symFilePath.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
	}
	symFilePath.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
	if (!symFilePath.DirectoryExists())
	{
		symFilePath.DirectoryCreate();
	}

	return symFilePath;
}

void CSymbols::ParserInit()
{
	m_SymFileParseBuffer = (char*)malloc(m_SymFileSize + 1);
	strcpy(m_SymFileParseBuffer, m_SymFileBuffer);
	m_bHaveFirstToken = false;
}

void CSymbols::ParserDone()
{
	free(m_SymFileParseBuffer);
}

void CSymbols::ParserFetchToken(const char* delim)
{
	if (!m_bHaveFirstToken)
	{
		m_ParserToken = strtok(m_SymFileParseBuffer, delim);
		m_bHaveFirstToken = true;
	}
	else
	{
		m_ParserToken = strtok(NULL, delim);
	}
	
	if (m_ParserToken != NULL)
	{
		m_ParserTokenLength = strlen(m_ParserToken);
		m_ParserDelimeter = m_SymFileBuffer[m_ParserToken - m_SymFileParseBuffer + m_ParserTokenLength];
	}
	else
	{
		m_ParserTokenLength = 0;
		m_ParserDelimeter = '\0';
	}
}

void CSymbols::Load()
{
	m_NextSymbolId = 0;
	Reset();

	if (g_Settings->LoadStringVal(Game_GameName).length() == 0)
	{
		MessageBox(NULL, "Game must be loaded", "Symbols", MB_ICONWARNING | MB_OK);
		return;
	}
	
	CPath symFilePath = GetSymFilePath();
	
	bool bOpened = m_SymFileHandle.Open(symFilePath, CFileBase::modeRead);
	
	if (!bOpened)
	{
		return;
	}
	
	m_SymFileSize = m_SymFileHandle.GetLength();
	m_SymFileBuffer = (char*)malloc(m_SymFileSize + 1);
	m_SymFileHandle.Read(m_SymFileBuffer, m_SymFileSize);
	m_SymFileHandle.Close();
	m_SymFileBuffer[m_SymFileSize] = '\0';
	
	ParseError errorCode = ERR_SUCCESS;
	int lineNumber = 1;

	ParserInit();

	while (true)
	{
		uint32_t address = 0;
		int type = 0;
		char* name = NULL;
		char* description = NULL;
		
		// Address
		ParserFetchToken(",\n\0");

		if (m_ParserToken == NULL || m_ParserTokenLength == 0)
		{
			// Empty line @EOF
			errorCode = ERR_SUCCESS;
			break;
		}

		char* endptr;
		address = (uint32_t)strtoull(m_ParserToken, &endptr, 16);
		
		if (endptr == m_ParserToken)
		{
			errorCode = ERR_INVALID_ADDR;
			break;
		}
		
		// Type
		if (m_ParserDelimeter != ',')
		{
			errorCode = ERR_MISSING_FIELDS;
			break;
		}
		
		ParserFetchToken(",\n\0");
		type = GetTypeNumber(m_ParserToken);

		if (type == -1)
		{
			errorCode = ERR_INVALID_TYPE;
			break;
		}
		
		// Name
		if (m_ParserDelimeter != ',')
		{
			errorCode = ERR_MISSING_FIELDS;
			break;
		}

		ParserFetchToken(",\n\0");
		name = m_ParserToken;

		// Optional description
		if (m_ParserDelimeter == ',')
		{
			ParserFetchToken("\n\0");
			description = m_ParserToken;
		}
		
		// Add symbol object to the vector
		Add(type, address, name, description);

		if (m_ParserDelimeter == '\0')
		{
			errorCode = ERR_SUCCESS;
			break;
		}

		lineNumber++;
	}
	
	ParserDone();
	free(m_SymFileBuffer);

	switch (errorCode)
	{
	case ERR_SUCCESS:
		break;
	case ERR_INVALID_ADDR:
		ParseErrorAlert("Invalid address", lineNumber);
		break;
	case ERR_INVALID_TYPE:
		ParseErrorAlert("Invalid type", lineNumber);
		break;
	case ERR_INVALID_NAME:
		ParseErrorAlert("Invalid name", lineNumber);
		break;
	case ERR_MISSING_FIELDS:
		ParseErrorAlert("Missing required field(s)", lineNumber);
		break;
	}
}

void CSymbols::Save()
{
	int nSymbols = m_Symbols.size();
	
	char* symfile;
	int symfile_size = 0;
	int symfile_idx = 0;

	// Determine file size
	for (int i = 0; i < nSymbols; i++)
	{
		CSymbolEntry* symbol = m_Symbols[i];

		symfile_size += 11; // address 8, required commas 2, newline 1
		symfile_size += strlen(symbol->m_Name);
		symfile_size += strlen(symbol->TypeName());

		if (symbol->m_Description != NULL && strlen(symbol->m_Description) != 0)
		{
			symfile_size += 1; // comma
			symfile_size += strlen(symbol->m_Description);
		}
	}

	if (symfile_size == 0)
	{
		return;
	}

	symfile = (char*) malloc(symfile_size + 1);
	symfile[symfile_size] = '\0';

	// Write out
	for (int i = 0; i < nSymbols; i++)
	{
		CSymbolEntry* symbol = m_Symbols[i];
		symfile_idx += sprintf(&symfile[symfile_idx], "%08X,%s,%s", symbol->m_Address, symbol->TypeName(), symbol->m_Name);
		if (symbol->m_Description != NULL)
		{
			symfile_idx += sprintf(&symfile[symfile_idx], ",%s", symbol->m_Description);
		}
		symfile_idx += sprintf(&symfile[symfile_idx], "\n");
	}

	m_SymFileHandle.Open(GetSymFilePath(), CFileBase::modeCreate | CFileBase::modeReadWrite);
	m_SymFileHandle.SeekToBegin();
	m_SymFileHandle.Write(symfile, symfile_size);
	m_SymFileHandle.SetEndOfFile();
	m_SymFileHandle.Close();

	free(symfile);
}

void CSymbols::GetValueString(char* dest, CSymbolEntry* lpSymbol)
{
	uint8_t v8;
	uint16_t v16;
	uint32_t v32;
	uint64_t v64;
	float vf;
	double vd;

	uint32_t address = lpSymbol->m_Address;

	switch (lpSymbol->m_Type)
	{
	case TYPE_CODE:
	case TYPE_DATA:
		sprintf(dest, "");
		break;
	case TYPE_U8:
		g_MMU->LB_VAddr(address, v8);
		sprintf(dest, "%u", v8);
		break;
	case TYPE_U16:
		g_MMU->LH_VAddr(address, v16);
		sprintf(dest, "%u", v16);
		break;
	case TYPE_U32:
		g_MMU->LW_VAddr(address, v32);
		sprintf(dest, "%u", v32);
		break;
	case TYPE_U64:
		g_MMU->LD_VAddr(address, v64);
		sprintf(dest, "%I64u", v64);
		break;
	case TYPE_S8:
		g_MMU->LB_VAddr(address, v8);
		sprintf(dest, "%ihh", v8);
		break;
	case TYPE_S16:
		g_MMU->LH_VAddr(address, v16);
		sprintf(dest, "%i", v16);
		break;
	case TYPE_S32:
		g_MMU->LW_VAddr(address, v32);
		sprintf(dest, "%i", v32);
		break;
	case TYPE_S64:
		g_MMU->LD_VAddr(address, v64);
		sprintf(dest, "%I64i", v64);
		break;
	case TYPE_FLOAT:
		g_MMU->LW_VAddr(address, *(uint32_t*)&vf);
		sprintf(dest, "%f", vf);
		break;
	case TYPE_DOUBLE:
		g_MMU->LD_VAddr(address, *(uint64_t*)&vd);
		sprintf(dest, "%f", vd);
		break;
	default:
		MessageBox(NULL, "unkown type", "", MB_OK);
		break;
	}
}

void CSymbols::ParseErrorAlert(char* message, int lineNumber)
{
	stdstr messageFormatted = stdstr_f("%s\nLine %d", message, lineNumber);
	MessageBox(NULL, messageFormatted.c_str(), "Parse error", MB_OK | MB_ICONWARNING);
}

void CSymbols::Reset()
{
	for (int i = 0; i < GetCount(); i++)
	{
		delete m_Symbols[i];
	}
	m_Symbols.clear();
}

const char* CSymbols::GetNameByAddress(uint32_t address)
{
    uint32_t len = GetCount();
	for (uint32_t i = 0; i < len; i++)
	{
		if (m_Symbols[i]->m_Address == address)
		{
			return m_Symbols[i]->m_Name;
		}
	}
	return NULL;
}

bool CSymbols::SortFunction(CSymbolEntry* a, CSymbolEntry* b)
{
	return (a->m_Address < b->m_Address);
}

void CSymbols::Add(int type, uint32_t address, char* name, char* description)
{
	if (name == NULL || strlen(name) == 0)
	{
		return;
	}

	if (description == NULL || strlen(description) == 0)
	{
		description = NULL;
	}

	int id = m_NextSymbolId++;

	CSymbolEntry* symbol = new CSymbolEntry(id, type, address, name, description);
	m_Symbols.push_back(symbol);

	sort(m_Symbols.begin(), m_Symbols.end(), SortFunction);
}

int CSymbols::GetCount()
{
	return m_Symbols.size();
}

CSymbolEntry* CSymbols::GetEntryByIndex(int index)
{
	if (index < 0 || index >= GetCount())
	{
		return NULL;
	}
	return m_Symbols[index];
}

CSymbolEntry* CSymbols::GetEntryByAddress(uint32_t address)
{
	for (int i = 0; i < GetCount(); i++)
	{
		if (m_Symbols[i]->m_Address == address)
		{
			return m_Symbols[i];
		}
	}
	return NULL;
}

CSymbolEntry* CSymbols::GetEntryById(int id)
{
	for (int i = 0; i < GetCount(); i++)
	{
		if (m_Symbols[i]->m_Id == id)
		{
			return m_Symbols[i];
		}
	}
	return NULL;
}

void CSymbols::RemoveEntryById(int id)
{
	for (int i = 0; i < GetCount(); i++)
	{
		if (m_Symbols[i]->m_Id == id)
		{
			delete m_Symbols[i];
			m_Symbols.erase(m_Symbols.begin() + i);
			break;
		}
	}
}

void CSymbols::EnterCriticalSection()
{
	::EnterCriticalSection(&m_CriticalSection);
}

void CSymbols::LeaveCriticalSection()
{
	::LeaveCriticalSection(&m_CriticalSection);
}

void CSymbols::InitializeCriticalSection()
{
	if (!m_bInitialized)
	{
		m_bInitialized = true;
		::InitializeCriticalSection(&m_CriticalSection);
	}
}

void CSymbols::DeleteCriticalSection()
{
	if (m_bInitialized)
	{
		m_bInitialized = false;
		::DeleteCriticalSection(&m_CriticalSection);
	}
}