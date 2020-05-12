/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include "Symbols.h"

CSymbolTable::CSymbolTable(CDebuggerUI* debugger) :
    m_Debugger(debugger),
    m_NextSymbolId(0),
    m_SymFileBuffer(NULL),
    m_SymFileSize(0),
    m_ParserToken(NULL),
    m_ParserTokenLength(0),
    m_ParserDelimeter(0),
    m_SymFileParseBuffer(NULL),
    m_bHaveFirstToken(false),
    m_TokPos(NULL)
{
}

CSymbolTable::~CSymbolTable()
{
    delete[] m_SymFileBuffer;
    delete[] m_SymFileParseBuffer;
}

symbol_type_info_t CSymbolTable::m_SymbolTypes[] = {
    { SYM_CODE,   "code",   1 },
    { SYM_DATA,   "data",   1 },
    { SYM_U8,     "u8",     1 },
    { SYM_U16,    "u16",    2 },
    { SYM_U32,    "u32",    4 },
    { SYM_U64,    "u64",    8 },
    { SYM_S8,     "s8",     1 },
    { SYM_S16,    "s16",    2 },
    { SYM_S32,    "s32",    4 },
    { SYM_S64,    "s64",    8 },
    { SYM_FLOAT,  "float",  4 },
    { SYM_DOUBLE, "double", 8 },
    { SYM_VECTOR2, "v2", 8 },
    { SYM_VECTOR3, "v3", 12 },
    { SYM_VECTOR4, "v4", 16 },
    { SYM_INVALID, NULL,    0 }
};

symbol_type_id_t CSymbolTable::GetTypeId(char* typeName)
{
    const char* name;
    for (int i = 0; (name = m_SymbolTypes[i].name) != NULL; i++)
    {
        if (strcmp(typeName, name) == 0)
        {
            return (symbol_type_id_t)i;
        }
    }
    return SYM_INVALID;
}

const char* CSymbolTable::GetTypeName(int typeId)
{
    if (typeId >= NUM_SYM_TYPES)
    {
        return NULL;
    }
    return m_SymbolTypes[typeId].name;
}

int CSymbolTable::GetTypeSize(int typeId)
{
    if (typeId >= NUM_SYM_TYPES)
    {
        return NULL;
    }
    return m_SymbolTypes[typeId].size;
}

// Open symbols file for game and parse into list
CPath CSymbolTable::GetSymFilePath()
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

void CSymbolTable::ParserFetchToken(const char* delim)
{
    if (!m_bHaveFirstToken)
    {
        m_TokPos = NULL;
        m_ParserToken = strtok_s(m_SymFileParseBuffer, delim, &m_TokPos);
        m_bHaveFirstToken = true;
    }
    else
    {
        m_ParserToken = strtok_s(NULL, delim, &m_TokPos);
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

void CSymbolTable::Load()
{
    CGuard guard(m_CS);

    m_NextSymbolId = 0;
    m_Symbols.clear();

    if (g_Settings->LoadStringVal(Game_GameName).length() == 0)
    {
        MessageBox(NULL, L"Game must be loaded", L"Symbols", MB_ICONWARNING | MB_OK);
        return;
    }
    
    CPath symFilePath = GetSymFilePath();
    
    bool bOpened = m_SymFileHandle.Open(symFilePath, CFileBase::modeRead);
    
    if (!bOpened)
    {
        return;
    }
    
    if (m_SymFileBuffer != NULL)
    {
        delete[] m_SymFileBuffer;
    }

    if (m_SymFileParseBuffer != NULL)
    {
        delete[] m_SymFileParseBuffer;
    }

    m_SymFileSize = m_SymFileHandle.GetLength();
    m_SymFileBuffer = new char[m_SymFileSize + 1];
    m_SymFileParseBuffer = new char[m_SymFileSize + 1];
    m_SymFileHandle.Read(m_SymFileBuffer, m_SymFileSize);
    m_SymFileHandle.Close();
    m_SymFileBuffer[m_SymFileSize] = '\0';
    
    strcpy(m_SymFileParseBuffer, m_SymFileBuffer);
    m_bHaveFirstToken = false;

    symbol_parse_error_t errorCode = ERR_SUCCESS;
    int lineNumber = 1;

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
        type = GetTypeId(m_ParserToken);

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
        AddSymbol(type, address, name, description);

        if (m_ParserDelimeter == '\0')
        {
            errorCode = ERR_SUCCESS;
            break;
        }

        lineNumber++;
    }
    
    delete[] m_SymFileParseBuffer;
    m_SymFileParseBuffer = NULL;

    delete[] m_SymFileBuffer;
    m_SymFileBuffer = NULL;

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

void CSymbolTable::Save()
{
    CGuard guard(m_CS);

    m_SymFileHandle.Open(GetSymFilePath(), CFileBase::modeCreate | CFileBase::modeReadWrite);
    m_SymFileHandle.SeekToBegin();

    for (size_t i = 0; i < m_Symbols.size(); i++)
    {
        CSymbol& symbol = m_Symbols[i];
        stdstr strLine = stdstr_f("%08X,%s,%s", symbol.m_Address, symbol.TypeName(), symbol.m_Name);
        
        if (symbol.m_Description != NULL)
        {
            strLine += stdstr_f(",%s", symbol.m_Description);
        }

        strLine += "\n";
        m_SymFileHandle.Write(strLine.c_str(), strLine.length());
    }

    m_SymFileHandle.SetEndOfFile();
    m_SymFileHandle.Close();
}

void CSymbolTable::GetValueString(char* dst, CSymbol* symbol)
{
    union
    {
        uint8_t   u8;
        int8_t    s8;
        uint16_t u16;
        int16_t  s16;
        uint32_t u32;
        int32_t  s32;
        uint64_t u64;
        int64_t  s64;
        float    f32;
        double   f64;
    } value;

    uint32_t address = symbol->m_Address;

    float xyzw[4];
    switch (symbol->m_Type)
    {
    case SYM_CODE:
    case SYM_DATA:
        sprintf(dst, "");
        break;
    case SYM_U8:
        m_Debugger->DebugLoad_VAddr(address, value.u8);
        sprintf(dst, "%u", value.u8);
        break;
    case SYM_U16:
        m_Debugger->DebugLoad_VAddr(address, value.u16);
        sprintf(dst, "%u", value.u16);
        break;
    case SYM_U32:
        m_Debugger->DebugLoad_VAddr(address, value.u32);
        sprintf(dst, "%u", value.u32);
        break;
    case SYM_U64:
        m_Debugger->DebugLoad_VAddr(address, value.u64);
        sprintf(dst, "%I64u", value.u64);
        break;
    case SYM_S8:
        m_Debugger->DebugLoad_VAddr(address, value.s8);
        sprintf(dst, "%ihh", value.s8);
        break;
    case SYM_S16:
        m_Debugger->DebugLoad_VAddr(address, value.s16);
        sprintf(dst, "%i", value.s16);
        break;
    case SYM_S32:
        m_Debugger->DebugLoad_VAddr(address, value.s32);
        sprintf(dst, "%i", value.s32);
        break;
    case SYM_S64:
        m_Debugger->DebugLoad_VAddr(address, value.s64);
        sprintf(dst, "%I64i", value.s64);
        break;
    case SYM_FLOAT:
        m_Debugger->DebugLoad_VAddr(address, value.f32);
        sprintf(dst, "%f", value.f32);
        break;
    case SYM_DOUBLE:
        m_Debugger->DebugLoad_VAddr(address, value.f64);
        sprintf(dst, "%f", value.f64);
        break;
    case SYM_VECTOR2:
        for (int i = 0; i < 2; i++) {
            m_Debugger->DebugLoad_VAddr(address + (i * sizeof(float)), value.f32);
            xyzw[i] = value.f32;
        }
        sprintf(dst, "%f, %f", xyzw[0], xyzw[1]);
        break;
    case SYM_VECTOR3:
        for (int i = 0; i < 3; i++) {
            m_Debugger->DebugLoad_VAddr(address + (i * sizeof(float)), value.f32);
            xyzw[i] = value.f32;
        }
        sprintf(dst, "%f, %f, %f", xyzw[0], xyzw[1], xyzw[2]);
        break;
    case SYM_VECTOR4:
        for (int i = 0; i < 4; i++) {
            m_Debugger->DebugLoad_VAddr(address + (i * sizeof(float)), value.f32);
            xyzw[i] = value.f32;
        }
        sprintf(dst, "%f, %f, %f, %f", xyzw[0], xyzw[1], xyzw[2], xyzw[3]);
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        break;
    }
}

void CSymbolTable::ParseErrorAlert(char* message, int lineNumber)
{
    stdstr messageFormatted = stdstr_f("%s\nLine %d", message, lineNumber);
    MessageBox(NULL, messageFormatted.ToUTF16().c_str(), L"Parse error", MB_OK | MB_ICONWARNING);
}

void CSymbolTable::Reset()
{
    CGuard guard(m_CS);
    m_Symbols.clear();
}

bool CSymbolTable::CmpSymbolAddresses(CSymbol& a, CSymbol& b)
{
    return (a.m_Address < b.m_Address);
}

void CSymbolTable::AddSymbol(int type, uint32_t address, const char* name, const char* description)
{
    CGuard guard(m_CS);

    if (name == NULL || strlen(name) == 0)
    {
        return;
    }

    if (description == NULL || strlen(description) == 0)
    {
        description = NULL;
    }

    int id = m_NextSymbolId++;

    CSymbol symbol = CSymbol(id, type, address, name, description);
    m_Symbols.push_back(symbol);

    sort(m_Symbols.begin(), m_Symbols.end(), CmpSymbolAddresses);
}

int CSymbolTable::GetCount()
{
    CGuard guard(m_CS);
    return m_Symbols.size();
}

bool CSymbolTable::GetSymbolByIndex(size_t index, CSymbol* symbol)
{
    CGuard guard(m_CS);
    if (index < 0 || index >= m_Symbols.size())
    {
        return false;
    }
    *symbol = m_Symbols[index];
    return true;
}

bool CSymbolTable::GetSymbolByAddress(uint32_t address, CSymbol* symbol)
{
    CGuard guard(m_CS);
    for (size_t i = 0; i < m_Symbols.size(); i++)
    {
        if (m_Symbols[i].m_Address == address)
        {
            *symbol = m_Symbols[i];
            return true;
        }
    }
    return false;
}

bool CSymbolTable::GetSymbolByOverlappedAddress(uint32_t address, CSymbol* symbol)
{
    CGuard guard(m_CS);
    for (size_t i = 0; i < m_Symbols.size(); i++)
    {
        if (address >= m_Symbols[i].m_Address &&
            address < m_Symbols[i].m_Address + m_Symbols[i].TypeSize())
        {
            *symbol = m_Symbols[i];
            return true;
        }
    }
    return false;
}

bool CSymbolTable::GetSymbolById(int id, CSymbol* symbol)
{
    CGuard guard(m_CS);
    for (size_t i = 0; i < m_Symbols.size(); i++)
    {
        if (m_Symbols[i].m_Id == id)
        {
            *symbol = m_Symbols[i];
            return true;
        }
    }
    return false;
}

bool CSymbolTable::RemoveSymbolById(int id)
{
    CGuard guard(m_CS);
    for (size_t i = 0; i < m_Symbols.size(); i++)
    {
        if (m_Symbols[i].m_Id == id)
        {
            m_Symbols.erase(m_Symbols.begin() + i);
            return true;
        }
    }
    return false;
}
